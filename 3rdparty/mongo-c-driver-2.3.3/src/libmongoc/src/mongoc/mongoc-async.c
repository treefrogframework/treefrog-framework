/*
 * Copyright 2009-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <mongoc/mongoc-async-cmd-private.h>
#include <mongoc/mongoc-async-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-socket-private.h>
#include <mongoc/mongoc-stream-private.h>
#include <mongoc/mongoc-util-private.h>

#include <mongoc/mongoc.h>
#include <mongoc/utlist.h>

#include <bson/bson.h>

#include <mlib/duration.h>
#include <mlib/time_point.h>
#include <mlib/timer.h>


mongoc_async_t *
mongoc_async_new(void)
{
   mongoc_async_t *async = (mongoc_async_t *)bson_malloc0(sizeof(*async));

   return async;
}

void
mongoc_async_destroy(mongoc_async_t *async)
{
   mongoc_async_cmd_t *acmd, *tmp;

   DL_FOREACH_SAFE(async->cmds, acmd, tmp)
   {
      mongoc_async_cmd_destroy(acmd);
   }

   bson_free(async);
}

void
mongoc_async_run(mongoc_async_t *async)
{
   mongoc_async_cmd_t *acmd, *tmp;
   mongoc_async_cmd_t **acmds_polled = NULL;
   mongoc_stream_poll_t *poller = NULL;
   ssize_t nactive = 0;
   size_t poll_size = 0;

   DL_FOREACH(async->cmds, acmd)
   {
      // CDRIVER-1571: See _acmd_reset_elapsed doc comment to explain this hack
      _acmd_reset_elapsed(acmd);
   }

   while (async->ncmds) {
      /* ncmds grows if we discover a replica & start calling hello on it */
      if (poll_size < async->ncmds) {
         poller = (mongoc_stream_poll_t *)bson_realloc(poller, sizeof(*poller) * async->ncmds);
         acmds_polled = (mongoc_async_cmd_t **)bson_realloc(acmds_polled, sizeof(*acmds_polled) * async->ncmds);
         poll_size = async->ncmds;
      }

      // Number of streams in the poller object
      unsigned nstreams = 0;

      // The timer to wake up the poll()
      mlib_timer poll_timer = mlib_expires_never();

      /* check if any cmds are ready to be initiated. */
      DL_FOREACH_SAFE(async->cmds, acmd, tmp)
      {
         if (acmd->state == MONGOC_ASYNC_CMD_PENDING_CONNECT) {
            // Command is waiting to be initiated.
            // Timer for when the command should be initiated:
            // Should not yet have an associated stream
            BSON_ASSERT(!acmd->stream);
            if (mlib_timer_is_expired(acmd->_connect_delay_timer)) {
               /* time to initiate. */
               if (mongoc_async_cmd_run(acmd)) {
                  // We should now have an associated stream
                  BSON_ASSERT(acmd->stream);
               } else {
                  /* this command was removed. */
                  continue;
               }
            } else {
               // Wake up poll() when the initiation timeout is hit
               poll_timer = mlib_soonest_timer(poll_timer, acmd->_connect_delay_timer);
            }
         }

         if (acmd->stream) {
            acmds_polled[nstreams] = acmd;
            poller[nstreams].stream = acmd->stream;
            poller[nstreams].events = acmd->events;
            poller[nstreams].revents = 0;
            // Wake up poll() if the object's overall timeout is hit
            poll_timer = mlib_soonest_timer(poll_timer, _acmd_deadline(acmd));
            ++nstreams;
         }
      }

      if (async->ncmds == 0) {
         /* all cmds failed to initiate and removed themselves. */
         break;
      }

      if (nstreams > 0) {
         /* we need at least one stream to poll. */
         nactive = _mongoc_stream_poll_internal(poller, nstreams, poll_timer);
      } else {
         /* currently this does not get hit. we always have at least one command
          * initialized with a stream. */
         mlib_sleep_until(poll_timer.expires_at);
      }

      if (nactive > 0) {
         mlib_foreach_urange (i, nstreams) {
            mongoc_async_cmd_t *const iter = acmds_polled[i];
            if (poller[i].revents & (POLLERR | POLLHUP)) {
               int hup = poller[i].revents & POLLHUP;
               if (iter->state == MONGOC_ASYNC_CMD_SEND) {
                  _mongoc_set_error(&iter->error,
                                    MONGOC_ERROR_STREAM,
                                    MONGOC_ERROR_STREAM_CONNECT,
                                    hup ? "connection refused" : "unknown connection error");
               } else {
                  _mongoc_set_error(&iter->error,
                                    MONGOC_ERROR_STREAM,
                                    MONGOC_ERROR_STREAM_SOCKET,
                                    hup ? "connection closed" : "unknown socket error");
               }

               iter->state = MONGOC_ASYNC_CMD_ERROR_STATE;
            }

            if ((poller[i].revents & poller[i].events) || iter->state == MONGOC_ASYNC_CMD_ERROR_STATE) {
               (void)mongoc_async_cmd_run(iter);
               nactive--;
            }

            if (!nactive) {
               break;
            }
         }
      }

      DL_FOREACH_SAFE(async->cmds, acmd, tmp)
      {
         /* check if an initiated cmd has passed the connection timeout.  */
         if (acmd->state != MONGOC_ASYNC_CMD_PENDING_CONNECT && _acmd_has_timed_out(acmd)) {
            _mongoc_set_error(&acmd->error,
                              MONGOC_ERROR_STREAM,
                              MONGOC_ERROR_STREAM_CONNECT,
                              acmd->state == MONGOC_ASYNC_CMD_SEND ? "connection timeout" : "socket timeout");

            acmd->_event_callback(acmd, MONGOC_ASYNC_CMD_TIMEOUT, NULL, _acmd_elapsed(acmd));

            /* Remove acmd from the async->cmds doubly-linked list */
            mongoc_async_cmd_destroy(acmd);
         } else if (acmd->state == MONGOC_ASYNC_CMD_CANCELLED_STATE) {
            acmd->_event_callback(acmd, MONGOC_ASYNC_CMD_ERROR, NULL, _acmd_elapsed(acmd));

            /* Remove acmd from the async->cmds doubly-linked list */
            mongoc_async_cmd_destroy(acmd);
         }
      }
   }

   bson_free(poller);
   bson_free(acmds_polled);
}
