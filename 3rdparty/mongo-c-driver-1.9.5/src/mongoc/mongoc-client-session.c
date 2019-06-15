/*
 * Copyright 2017 MongoDB, Inc.
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


#include "mongoc-client-session-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-client-private.h"
#include "mongoc-rand-private.h"

#define SESSION_NEVER_USED (-1)

mongoc_session_opt_t *
mongoc_session_opts_new (void)
{
   mongoc_session_opt_t *opts = bson_malloc0 (sizeof (mongoc_session_opt_t));

   /* Driver Sessions Spec: causal consistency is true by default */
   mongoc_session_opts_set_causal_consistency (opts, true);

   return opts;
}


void
mongoc_session_opts_set_causal_consistency (mongoc_session_opt_t *opts,
                                            bool causal_consistency)
{
   ENTRY;

   BSON_ASSERT (opts);

   if (causal_consistency) {
      opts->flags |= MONGOC_SESSION_CAUSAL_CONSISTENCY;
   } else {
      opts->flags &= ~MONGOC_SESSION_CAUSAL_CONSISTENCY;
   }

   EXIT;
}

bool
mongoc_session_opts_get_causal_consistency (const mongoc_session_opt_t *opts)
{
   ENTRY;

   BSON_ASSERT (opts);

   RETURN (!!(opts->flags & MONGOC_SESSION_CAUSAL_CONSISTENCY));
}


static void
_mongoc_session_opts_copy (const mongoc_session_opt_t *src,
                           mongoc_session_opt_t *dst)
{
   dst->flags = src->flags;
}


mongoc_session_opt_t *
mongoc_session_opts_clone (const mongoc_session_opt_t *opts)
{
   mongoc_session_opt_t *cloned_opts;

   ENTRY;

   BSON_ASSERT (opts);

   cloned_opts = bson_malloc (sizeof (mongoc_session_opt_t));
   _mongoc_session_opts_copy (opts, cloned_opts);

   RETURN (cloned_opts);
}


void
mongoc_session_opts_destroy (mongoc_session_opt_t *opts)
{
   ENTRY;

   BSON_ASSERT (opts);

   bson_free (opts);

   EXIT;
}


static bool
_mongoc_server_session_uuid (uint8_t *data /* OUT */, bson_error_t *error)
{
#ifdef MONGOC_ENABLE_CRYPTO
   /* https://tools.ietf.org/html/rfc4122#page-14
    *   o  Set the two most significant bits (bits 6 and 7) of the
    *      clock_seq_hi_and_reserved to zero and one, respectively.
    *
    *   o  Set the four most significant bits (bits 12 through 15) of the
    *      time_hi_and_version field to the 4-bit version number from
    *      Section 4.1.3.
    *
    *   o  Set all the other bits to randomly (or pseudo-randomly) chosen
    *      values.
    */

   if (!_mongoc_rand_bytes (data, 16)) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_SESSION_FAILURE,
                      "Could not generate UUID for logical session id");

      return false;
   }

   data[6] = (uint8_t) (0x40 | (data[6] & 0xf));
   data[8] = (uint8_t) (0x80 | (data[8] & 0x3f));

   return true;
#else
   /* no _mongoc_rand_bytes without a crypto library */
   bson_set_error (error,
                   MONGOC_ERROR_CLIENT,
                   MONGOC_ERROR_CLIENT_SESSION_FAILURE,
                   "Could not generate UUID for logical session id, we need a"
                   " cryptography library like libcrypto, Common Crypto, or"
                   " CNG");

   return false;
#endif
}


bool
_mongoc_parse_cluster_time (const bson_t *cluster_time,
                            uint32_t *timestamp,
                            uint32_t *increment)
{
   bson_iter_t iter;
   char *s;

   if (!cluster_time ||
       !bson_iter_init_find (&iter, cluster_time, "clusterTime") ||
       !BSON_ITER_HOLDS_TIMESTAMP (&iter)) {
      s = bson_as_json (cluster_time, NULL);
      MONGOC_ERROR ("Cannot parse cluster time from %s\n", s);
      bson_free (s);
      return false;
   }

   bson_iter_timestamp (&iter, timestamp, increment);

   return true;
}


bool
_mongoc_cluster_time_greater (const bson_t *new, const bson_t *old)
{
   uint32_t new_t, new_i, old_t, old_i;

   if (!_mongoc_parse_cluster_time (new, &new_t, &new_i) ||
       !_mongoc_parse_cluster_time (old, &old_t, &old_i)) {
      return false;
   }

   return (new_t > old_t) || (new_t == old_t && new_i > old_i);
}


void
_mongoc_client_session_handle_reply (mongoc_client_session_t *session,
                                     bool is_acknowledged,
                                     const bson_t *reply)
{
   bson_iter_t iter;
   uint32_t len;
   const uint8_t *data;
   bson_t cluster_time;
   uint32_t t;
   uint32_t i;

   BSON_ASSERT (session);

   if (!reply || !bson_iter_init (&iter, reply)) {
      return;
   }

   while (bson_iter_next (&iter)) {
      if (!strcmp (bson_iter_key (&iter), "$clusterTime") &&
          BSON_ITER_HOLDS_DOCUMENT (&iter)) {
         bson_iter_document (&iter, &len, &data);
         bson_init_static (&cluster_time, data, (size_t) len);

         mongoc_client_session_advance_cluster_time (session, &cluster_time);
      } else if (!strcmp (bson_iter_key (&iter), "operationTime") &&
                 BSON_ITER_HOLDS_TIMESTAMP (&iter) && is_acknowledged) {
         bson_iter_timestamp (&iter, &t, &i);
         mongoc_client_session_advance_operation_time (session, t, i);
      }
   }
}


mongoc_server_session_t *
_mongoc_server_session_new (bson_error_t *error)
{
   uint8_t uuid_data[16];
   mongoc_server_session_t *s;

   ENTRY;

   if (!_mongoc_server_session_uuid (uuid_data, error)) {
      RETURN (NULL);
   }

   s = bson_malloc0 (sizeof (mongoc_server_session_t));
   s->last_used_usec = SESSION_NEVER_USED;
   s->prev = NULL;
   s->next = NULL;
   bson_init (&s->lsid);
   bson_append_binary (
      &s->lsid, "id", 2, BSON_SUBTYPE_UUID, uuid_data, sizeof uuid_data);

   /* transaction number is a positive integer and will be incremented before
    * each use, so ensure it is initialized to zero. */
   s->txn_number = 0;

   RETURN (s);
}


bool
_mongoc_server_session_timed_out (const mongoc_server_session_t *server_session,
                                  int64_t session_timeout_minutes)
{
   int64_t timeout_usec;
   const int64_t minute_to_usec = 60 * 1000 * 1000;

   ENTRY;

   if (session_timeout_minutes == MONGOC_NO_SESSIONS) {
      /* not connected right now; keep the session */
      return false;
   }

   if (server_session->last_used_usec == SESSION_NEVER_USED) {
      return false;
   }

   /* Driver Sessions Spec: if a session has less than one minute left before
    * becoming stale, discard it */
   timeout_usec =
      server_session->last_used_usec + session_timeout_minutes * minute_to_usec;

   RETURN (timeout_usec - bson_get_monotonic_time () < 1 * minute_to_usec);
}


void
_mongoc_server_session_destroy (mongoc_server_session_t *server_session)
{
   ENTRY;

   bson_destroy (&server_session->lsid);
   bson_free (server_session);

   EXIT;
}


mongoc_client_session_t *
_mongoc_client_session_new (mongoc_client_t *client,
                            mongoc_server_session_t *server_session,
                            const mongoc_session_opt_t *opts,
                            uint32_t client_session_id)
{
   mongoc_client_session_t *session;

   ENTRY;

   BSON_ASSERT (client);

   session = bson_malloc0 (sizeof (mongoc_client_session_t));
   session->client = client;
   session->server_session = server_session;
   session->client_session_id = client_session_id;
   bson_init (&session->cluster_time);

   if (opts) {
      _mongoc_session_opts_copy (opts, &session->opts);
   } else {
      /* sessions are causally consistent by default */
      session->opts.flags = MONGOC_SESSION_CAUSAL_CONSISTENCY;
   }

   RETURN (session);
}


mongoc_client_t *
mongoc_client_session_get_client (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   return session->client;
}


const mongoc_session_opt_t *
mongoc_client_session_get_opts (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   return &session->opts;
}


const bson_t *
mongoc_client_session_get_lsid (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   return &session->server_session->lsid;
}

const bson_t *
mongoc_client_session_get_cluster_time (const mongoc_client_session_t *session)
{
   BSON_ASSERT (session);

   if (bson_empty (&session->cluster_time)) {
      return NULL;
   }

   return &session->cluster_time;
}

void
mongoc_client_session_advance_cluster_time (mongoc_client_session_t *session,
                                            const bson_t *cluster_time)
{
   uint32_t t, i;

   ENTRY;

   if (bson_empty (&session->cluster_time) &&
       _mongoc_parse_cluster_time (cluster_time, &t, &i)) {
      bson_destroy (&session->cluster_time);
      bson_copy_to (cluster_time, &session->cluster_time);
      EXIT;
   }

   if (_mongoc_cluster_time_greater (cluster_time, &session->cluster_time)) {
      bson_destroy (&session->cluster_time);
      bson_copy_to (cluster_time, &session->cluster_time);
   }

   EXIT;
}

void
mongoc_client_session_get_operation_time (
   const mongoc_client_session_t *session,
   uint32_t *timestamp,
   uint32_t *increment)
{
   BSON_ASSERT (session);
   BSON_ASSERT (timestamp);
   BSON_ASSERT (increment);

   *timestamp = session->operation_timestamp;
   *increment = session->operation_increment;
}

void
mongoc_client_session_advance_operation_time (mongoc_client_session_t *session,
                                              uint32_t timestamp,
                                              uint32_t increment)
{
   ENTRY;

   BSON_ASSERT (session);

   if (timestamp > session->operation_timestamp ||
       (timestamp == session->operation_timestamp &&
        increment > session->operation_increment)) {
      session->operation_timestamp = timestamp;
      session->operation_increment = increment;
   }

   EXIT;
}

bool
_mongoc_client_session_from_iter (mongoc_client_t *client,
                                  bson_iter_t *iter,
                                  mongoc_client_session_t **cs,
                                  bson_error_t *error)
{
   ENTRY;

   /* must be int64 that fits in uint32 */
   if (!BSON_ITER_HOLDS_INT64 (iter) || bson_iter_int64 (iter) > 0xffffffff) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Invalid sessionId");
      RETURN (false);
   }

   RETURN (_mongoc_client_lookup_session (
      client, (uint32_t) bson_iter_int64 (iter), cs, error));
}


bool
mongoc_client_session_append (const mongoc_client_session_t *client_session,
                              bson_t *opts,
                              bson_error_t *error)
{
   ENTRY;

   BSON_ASSERT (client_session);
   BSON_ASSERT (opts);

   if (!bson_append_int64 (
          opts, "sessionId", 9, client_session->client_session_id)) {
      bson_set_error (
         error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, "invalid opts");

      RETURN (false);
   }

   RETURN (true);
}


void
mongoc_client_session_destroy (mongoc_client_session_t *session)
{
   ENTRY;

   BSON_ASSERT (session);

   _mongoc_client_unregister_session (session->client, session);
   _mongoc_client_push_server_session (session->client,
                                       session->server_session);

   bson_destroy (&session->cluster_time);
   bson_free (session);

   EXIT;
}
