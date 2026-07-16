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
#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-cursor-private.h>
#include <mongoc/mongoc-error-private.h>

#include <mongoc/mongoc.h>

typedef struct _data_cmd_t {
   mongoc_cursor_response_t response;
   bson_t cmd;
} data_cmd_t;


static mongoc_cursor_state_t
_prime(mongoc_cursor_t *cursor)
{
   data_cmd_t *data = (data_cmd_t *)cursor->impl.data;
   bson_t copied_opts;
   bson_init(&copied_opts);

   cursor->operation_id = ++cursor->client->cluster.operation_id;
   /* commands like agg have a cursor field, so copy opts without "batchSize" */
   bson_copy_to_excluding_noinit(&cursor->opts, &copied_opts, "batchSize", "tailable", NULL);

   /* server replies to aggregate/listIndexes/listCollections with:
    * {cursor: {id: N, firstBatch: []}} */
   _mongoc_cursor_response_refresh(cursor, &data->cmd, &copied_opts, &data->response);
   bson_destroy(&copied_opts);
   return IN_BATCH;
}


static mongoc_cursor_state_t
_pop_from_batch(mongoc_cursor_t *cursor)
{
   data_cmd_t *data = (data_cmd_t *)cursor->impl.data;

   _mongoc_cursor_response_read(cursor, &data->response, &cursor->current);

   if (cursor->current) {
      return IN_BATCH;
   } else {
      return cursor->cursor_id ? END_OF_BATCH : DONE;
   }
}


static mongoc_cursor_state_t
_get_next_batch(mongoc_cursor_t *cursor)
{
   data_cmd_t *data = (data_cmd_t *)cursor->impl.data;
   bson_t getmore_cmd;

   _mongoc_cursor_prepare_getmore_command(cursor, &getmore_cmd);
   _mongoc_cursor_response_refresh(cursor, &getmore_cmd, NULL /* opts */, &data->response);
   bson_destroy(&getmore_cmd);

   return IN_BATCH;
}


static void
_destroy(mongoc_cursor_impl_t *impl)
{
   data_cmd_t *data = (data_cmd_t *)impl->data;
   bson_destroy(&data->response.reply);
   bson_destroy(&data->cmd);
   bson_free(data);
}


static void
_clone(mongoc_cursor_impl_t *dst, const mongoc_cursor_impl_t *src)
{
   data_cmd_t *data_src = (data_cmd_t *)src->data;
   data_cmd_t *data_dst = BSON_ALIGNED_ALLOC0(data_cmd_t);
   bson_init(&data_dst->response.reply);
   bson_copy_to(&data_src->cmd, &data_dst->cmd);
   dst->data = data_dst;
}


mongoc_cursor_t *
_mongoc_cursor_cmd_new(mongoc_client_t *client,
                       const char *db_and_coll,
                       const bson_t *cmd,
                       const bson_t *opts,
                       const mongoc_read_prefs_t *user_prefs,
                       const mongoc_read_prefs_t *default_prefs,
                       const mongoc_read_concern_t *read_concern)
{
   BSON_ASSERT_PARAM(client);

   mongoc_cursor_t *cursor;
   data_cmd_t *data = BSON_ALIGNED_ALLOC0(data_cmd_t);

   cursor = _mongoc_cursor_new_with_opts(client, db_and_coll, opts, user_prefs, default_prefs, read_concern);
   _mongoc_cursor_check_and_copy_to(cursor, "command", cmd, &data->cmd);
   bson_init(&data->response.reply);
   cursor->impl.prime = _prime;
   cursor->impl.pop_from_batch = _pop_from_batch;
   cursor->impl.get_next_batch = _get_next_batch;
   cursor->impl.destroy = _destroy;
   cursor->impl.clone = _clone;
   cursor->impl.data = (void *)data;
   return cursor;
}


mongoc_cursor_t *
_mongoc_cursor_cmd_new_from_reply(mongoc_client_t *client, const bson_t *cmd, const bson_t *opts, bson_t *reply)
{
   BSON_ASSERT_PARAM(client);

   mongoc_cursor_t *cursor = _mongoc_cursor_cmd_new(client, NULL, cmd, opts, NULL, NULL, NULL);
   data_cmd_t *data = (data_cmd_t *)cursor->impl.data;

   cursor->state = IN_BATCH;

   bson_destroy(&data->response.reply);
   if (!bson_steal(&data->response.reply, reply)) {
      bson_destroy(&data->response.reply);
      BSON_ASSERT(bson_steal(&data->response.reply, bson_copy(reply)));
   }

   if (!_mongoc_cursor_start_reading_response(cursor, &data->response)) {
      _mongoc_set_error(
         &cursor->error, MONGOC_ERROR_CURSOR, MONGOC_ERROR_CURSOR_INVALID_CURSOR, "Couldn't parse cursor document");
   }

   if (0 != cursor->cursor_id && 0 == cursor->server_id) {
      // A non-zero cursor_id means the cursor is still open on the server.
      // Expect the "serverId" option to have been passed. The "serverId" option
      // identifies the server with the cursor.
      // The server with the cursor is required to send a "getMore" or
      // "killCursors" command.
      _mongoc_set_error(&cursor->error,
                        MONGOC_ERROR_CURSOR,
                        MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                        "Expected `serverId` option to identify server with open cursor "
                        "(cursor ID is %" PRId64 "). "
                        "Consider using `mongoc_client_select_server` and using the "
                        "resulting server ID to create the cursor.",
                        cursor->cursor_id);
      // Reset cursor_id to 0 to avoid an assertion error in
      // `mongoc_cursor_destroy` when attempting to send "killCursors".
      cursor->cursor_id = 0;
   }

   return cursor;
}
