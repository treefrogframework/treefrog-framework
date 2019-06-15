/*
 * Copyright 2017-present MongoDB, Inc.
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

#include <bson.h>
#include "mongoc-change-stream-private.h"
#include "mongoc-error.h"
#include "mongoc-cursor-private.h"
#include "mongoc-collection-private.h"
#include "mongoc-client-session-private.h"

#define CHANGE_STREAM_ERR(_str)         \
   bson_set_error (&stream->err,        \
                   MONGOC_ERROR_CURSOR, \
                   MONGOC_ERROR_BSON,   \
                   "Could not set " _str);


#define SET_BSON_OR_ERR(_dst, _str)                                   \
   do {                                                               \
      if (!BSON_APPEND_VALUE (_dst, _str, bson_iter_value (&iter))) { \
         CHANGE_STREAM_ERR (_str);                                    \
      }                                                               \
   } while (0);

/* Construct the aggregate command in cmd:
 * { aggregate: collname, pipeline: [], cursor: { batchSize: x } } */
static void
_make_command (mongoc_change_stream_t *stream, bson_t *command)
{
   bson_iter_t iter;
   bson_t change_stream_stage; /* { $changeStream: <change_stream_doc> } */
   bson_t change_stream_doc;
   bson_t pipeline;
   bson_t cursor_doc;

   bson_init (command);
   bson_append_utf8 (command,
                     "aggregate",
                     9,
                     stream->coll->collection,
                     stream->coll->collectionlen);
   bson_append_array_begin (command, "pipeline", 8, &pipeline);

   /* Append the $changeStream stage */
   bson_append_document_begin (&pipeline, "0", 1, &change_stream_stage);
   bson_append_document_begin (
      &change_stream_stage, "$changeStream", 13, &change_stream_doc);
   bson_concat (&change_stream_doc, &stream->full_document);
   if (!bson_empty (&stream->resume_token)) {
      bson_concat (&change_stream_doc, &stream->resume_token);
   }
   bson_append_document_end (&change_stream_stage, &change_stream_doc);
   bson_append_document_end (&pipeline, &change_stream_stage);

   /* Append user pipeline if it exists */
   if (bson_iter_init_find (&iter, &stream->pipeline_to_append, "pipeline") &&
       BSON_ITER_HOLDS_ARRAY (&iter)) {
      bson_iter_t child_iter;
      uint32_t key_int = 1;
      char buf[16];
      const char *key_str;

      bson_iter_recurse (&iter, &child_iter);
      while (bson_iter_next (&child_iter)) {
         /* The user pipeline may consist of invalid stages or non-documents.
          * Append anyway, and rely on the server error. */
         size_t keyLen =
            bson_uint32_to_string (key_int, &key_str, buf, sizeof (buf));
         bson_append_value (
            &pipeline, key_str, (int) keyLen, bson_iter_value (&child_iter));
         ++key_int;
      }
   }

   bson_append_array_end (command, &pipeline);

   /* Add batch size if needed */
   bson_append_document_begin (command, "cursor", 6, &cursor_doc);
   if (stream->batch_size > 0) {
      bson_append_int32 (&cursor_doc, "batchSize", 9, stream->batch_size);
   }
   bson_append_document_end (command, &cursor_doc);
}

/* Construct and send the aggregate command and create the resulting cursor.
 * Returns false on error, and sets stream->err. On error, stream->cursor
 * remains NULL, otherwise it is created and must be destroyed. */
static bool
_make_cursor (mongoc_change_stream_t *stream)
{
   mongoc_client_session_t *cs = NULL;
   bson_t command_opts;
   bson_t command; /* { aggregate: "coll", pipeline: [], ... } */
   bson_t reply;
   bson_iter_t iter;
   mongoc_server_description_t *sd;
   uint32_t server_id;

   BSON_ASSERT (stream);
   BSON_ASSERT (!stream->cursor);
   _make_command (stream, &command);
   bson_copy_to (&stream->opts, &command_opts);
   sd = mongoc_client_select_server (stream->coll->client,
                                     false /* for_writes */,
                                     stream->coll->read_prefs,
                                     &stream->err);
   if (!sd) {
      goto cleanup;
   }
   server_id = mongoc_server_description_id (sd);
   bson_append_int32 (&command_opts, "serverId", 8, server_id);
   mongoc_server_description_destroy (sd);

   if (bson_iter_init_find (&iter, &command_opts, "sessionId")) {
      if (!_mongoc_client_session_from_iter (
             stream->coll->client, &iter, &cs, &stream->err)) {
         goto cleanup;
      }
   } else if (stream->implicit_session) {
      /* If an implicit session was created before, and this cursor is now
       * being recreated after resuming, then use the same session as before. */
      cs = stream->implicit_session;
      if (!mongoc_client_session_append (cs, &command_opts, &stream->err)) {
         goto cleanup;
      }
   } else {
      /* Create an implicit session. This session lsid must be the same for the
       * agg command and the subsequent getMores. Thus, this implicit session is
       * passed as if it were an explicit session to
       * collection_read_command_with_opts and cursor_new_from_reply, but it is
       * still implicit and its lifetime is owned by this change_stream_t. */
      mongoc_session_opt_t *session_opts;
      session_opts = mongoc_session_opts_new ();
      mongoc_session_opts_set_causal_consistency (session_opts, false);
      /* returns NULL if sessions aren't supported. ignore errors. */
      cs =
         mongoc_client_start_session (stream->coll->client, session_opts, NULL);
      stream->implicit_session = cs;
      mongoc_session_opts_destroy (session_opts);
      if (cs &&
          !mongoc_client_session_append (cs, &command_opts, &stream->err)) {
         goto cleanup;
      }
   }

   /* use inherited read preference and read concern of the collection */
   if (!mongoc_collection_read_command_with_opts (
          stream->coll, &command, NULL, &command_opts, &reply, &stream->err)) {
      bson_destroy (&stream->err_doc);
      bson_copy_to (&reply, &stream->err_doc);
      bson_destroy (&reply);
      goto cleanup;
   }

   stream->cursor = mongoc_cursor_new_from_command_reply (
      stream->coll->client, &reply, server_id); /* steals reply */

   if (cs) {
      stream->cursor->client_session = cs;
      stream->cursor->explicit_session = 1;
   }

   /* maxTimeMS is only appended to getMores if these are set in cursor opts */
   bson_append_bool (&stream->cursor->opts,
                     MONGOC_CURSOR_TAILABLE,
                     MONGOC_CURSOR_TAILABLE_LEN,
                     true);
   bson_append_bool (&stream->cursor->opts,
                     MONGOC_CURSOR_AWAIT_DATA,
                     MONGOC_CURSOR_AWAIT_DATA_LEN,
                     true);

   if (stream->max_await_time_ms > 0) {
      BSON_ASSERT (
         _mongoc_cursor_set_opt_int64 (stream->cursor,
                                       MONGOC_CURSOR_MAX_AWAIT_TIME_MS,
                                       stream->max_await_time_ms));
   }

   if (stream->batch_size > 0) {
      mongoc_cursor_set_batch_size (stream->cursor,
                                    (uint32_t) stream->batch_size);
   }

cleanup:
   bson_destroy (&command);
   bson_destroy (&command_opts);
   return stream->err.code == 0;
}

mongoc_change_stream_t *
_mongoc_change_stream_new (const mongoc_collection_t *coll,
                           const bson_t *pipeline,
                           const bson_t *opts)
{
   bool full_doc_set = false;
   mongoc_change_stream_t *stream =
      (mongoc_change_stream_t *) bson_malloc0 (sizeof (mongoc_change_stream_t));

   BSON_ASSERT (coll);
   BSON_ASSERT (pipeline);

   stream->max_await_time_ms = -1;
   stream->batch_size = -1;
   stream->coll = mongoc_collection_copy ((mongoc_collection_t *) coll);
   bson_init (&stream->pipeline_to_append);
   bson_init (&stream->full_document);
   bson_init (&stream->opts);
   bson_init (&stream->resume_token);
   bson_init (&stream->err_doc);

   /*
    * The passed options may consist of:
    * fullDocument: 'default'|'updateLookup', passed to $changeStream stage
    * resumeAfter: optional<Doc>, passed to $changeStream stage
    * maxAwaitTimeMS: Optional<Int64>, set on the cursor
    * batchSize: Optional<Int32>, passed as agg option, {cursor: { batchSize: }}
    * standard command options like "sessionId", "maxTimeMS", or "collation"
    */

   if (opts) {
      bson_iter_t iter;

      if (bson_iter_init_find (&iter, opts, "fullDocument")) {
         SET_BSON_OR_ERR (&stream->full_document, "fullDocument");
         full_doc_set = true;
      }

      if (bson_iter_init_find (&iter, opts, "resumeAfter")) {
         SET_BSON_OR_ERR (&stream->resume_token, "resumeAfter");
      }

      if (bson_iter_init_find (&iter, opts, "batchSize")) {
         if (BSON_ITER_HOLDS_INT32 (&iter)) {
            stream->batch_size = bson_iter_int32 (&iter);
         }
      }

      if (bson_iter_init_find (&iter, opts, "maxAwaitTimeMS") &&
          BSON_ITER_HOLDS_INT (&iter)) {
         stream->max_await_time_ms = bson_iter_as_int64 (&iter);
      }

      /* save the remaining opts for mongoc_collection_read_command_with_opts */
      bson_copy_to_excluding_noinit (opts,
                                     &stream->opts,
                                     "fullDocument",
                                     "resumeAfter",
                                     "batchSize",
                                     "maxAwaitTimeMS",
                                     NULL);
   }

   if (!full_doc_set) {
      if (!BSON_APPEND_UTF8 (
             &stream->full_document, "fullDocument", "default")) {
         CHANGE_STREAM_ERR ("fullDocument");
      }
   }

   /* Accept two forms of user pipeline:
    * 1. A document like: { "pipeline": [...] }
    * 2. An array-like document: { "0": {}, "1": {}, ... }
    * If the passed pipeline is invalid, we pass it along and let the server
    * error instead.
    */
   if (!bson_empty (pipeline)) {
      bson_iter_t iter;
      if (bson_iter_init_find (&iter, pipeline, "pipeline") &&
          BSON_ITER_HOLDS_ARRAY (&iter)) {
         SET_BSON_OR_ERR (&stream->pipeline_to_append, "pipeline");
      } else {
         if (!BSON_APPEND_ARRAY (
                &stream->pipeline_to_append, "pipeline", pipeline)) {
            CHANGE_STREAM_ERR ("pipeline");
         }
      }
   }

   if (stream->err.code == 0) {
      (void) _make_cursor (stream);
   }

   return stream;
}

bool
mongoc_change_stream_next (mongoc_change_stream_t *stream, const bson_t **bson)
{
   bson_iter_t iter;
   bool ret = false;

   BSON_ASSERT (stream);
   BSON_ASSERT (bson);

   if (stream->err.code != 0) {
      goto end;
   }

   BSON_ASSERT (stream->cursor);
   if (!mongoc_cursor_next (stream->cursor, bson)) {
      const bson_t *err_doc;
      bson_error_t err;
      bool resumable = false;

      if (!mongoc_cursor_error_document (stream->cursor, &err, &err_doc)) {
         /* No error occurred, just no documents left */
         goto end;
      }

      /* Change Streams Spec: An error is resumable if it is not a server error,
       * or if it has error code 43 (cursor not found) or is "not master" */
      if (!bson_empty (err_doc)) {
         /* This is a server error */
         bson_iter_t err_iter;
         if (bson_iter_init_find (&err_iter, err_doc, "errmsg") &&
             BSON_ITER_HOLDS_UTF8 (&err_iter)) {
            uint32_t len;
            const char *errmsg = bson_iter_utf8 (&err_iter, &len);
            if (strncmp (errmsg, "not master", len) == 0) {
               resumable = true;
            }
         }

         if (bson_iter_init_find (&err_iter, err_doc, "code") &&
             BSON_ITER_HOLDS_INT (&err_iter)) {
            if (bson_iter_as_int64 (&err_iter) == 43) {
               resumable = true;
            }
         }
      } else {
         /* This is a client error */
         resumable = true;
      }

      if (resumable) {
         /* recreate the cursor */
         mongoc_cursor_destroy (stream->cursor);
         stream->cursor = NULL;
         if (!_make_cursor (stream)) {
            goto end;
         }
         if (!mongoc_cursor_next (stream->cursor, bson)) {
            resumable =
               !mongoc_cursor_error_document (stream->cursor, &err, &err_doc);
            if (resumable) {
               /* Empty batch. */
               goto end;
            }
         }
      }

      if (!resumable) {
         stream->err = err;
         bson_destroy (&stream->err_doc);
         bson_copy_to (err_doc, &stream->err_doc);
         goto end;
      }
   }

   /* We have received documents, either from the first call to next
    * or after a resume. */
   if (!bson_iter_init_find (&iter, *bson, "_id")) {
      bson_set_error (&stream->err,
                      MONGOC_ERROR_CURSOR,
                      MONGOC_ERROR_CHANGE_STREAM_NO_RESUME_TOKEN,
                      "Cannot provide resume functionality when the resume "
                      "token is missing");
      goto end;
   }

   /* Copy the resume token */
   bson_reinit (&stream->resume_token);
   BSON_APPEND_VALUE (
      &stream->resume_token, "resumeAfter", bson_iter_value (&iter));
   ret = true;

end:
   /* Driver Sessions Spec: "When an implicit session is associated with a
    * cursor for use with getMore operations, the session MUST be returned to
    * the pool immediately following a getMore operation that indicates that the
    * cursor has been exhausted." */
   if (stream->implicit_session) {
      /* If creating the change stream cursor errored, it may be null. */
      if (!stream->cursor || stream->cursor->rpc.reply.cursor_id == 0) {
         mongoc_client_session_destroy (stream->implicit_session);
         stream->implicit_session = NULL;
      }
   }
   return ret;
}

bool
mongoc_change_stream_error_document (const mongoc_change_stream_t *stream,
                                     bson_error_t *err,
                                     const bson_t **bson)
{
   BSON_ASSERT (stream);

   if (stream->err.code != 0) {
      if (err) {
         *err = stream->err;
      }
      if (bson) {
         *bson = &stream->err_doc;
      }
      return true;
   }
   return false;
}

void
mongoc_change_stream_destroy (mongoc_change_stream_t *stream)
{
   BSON_ASSERT (stream);
   bson_destroy (&stream->pipeline_to_append);
   bson_destroy (&stream->full_document);
   bson_destroy (&stream->opts);
   bson_destroy (&stream->resume_token);
   bson_destroy (&stream->err_doc);
   if (stream->cursor) {
      mongoc_cursor_destroy (stream->cursor);
   }
   if (stream->implicit_session) {
      mongoc_client_session_destroy (stream->implicit_session);
   }
   mongoc_collection_destroy (stream->coll);
   bson_free (stream);
}
