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

#include <bson/bson.h>
#include "mongoc/mongoc-change-stream-private.h"
#include "mongoc/mongoc-collection-private.h"
#include "mongoc/mongoc-client-private.h"
#include "mongoc/mongoc-client-session-private.h"
#include "mongoc/mongoc-cursor-private.h"
#include "mongoc/mongoc-database-private.h"
#include "mongoc/mongoc-error.h"

#define CHANGE_STREAM_ERR(_str)         \
   bson_set_error (&stream->err,        \
                   MONGOC_ERROR_CURSOR, \
                   MONGOC_ERROR_BSON,   \
                   "Could not set " _str);

/* the caller knows either a client or server error has occurred.
 * `reply` contains the server reply or an empty document. */
static bool
_is_resumable_error (const bson_t *reply)
{
   bson_error_t error = {0};

   /* Change Streams Spec resumable criteria: "any error encountered which is
    * not a server error (e.g. a timeout error or network error)" */
   if (bson_empty (reply)) {
      return true;
   }

   if (_mongoc_cmd_check_ok (reply, MONGOC_ERROR_API_VERSION_2, &error)) {
      return true;
   }

   /* Change Streams Spec resumable criteria: "a server error response with an
    * error message containing the substring 'not master' or 'node is
    * recovering' */
   if (strstr (error.message, "not master") ||
       strstr (error.message, "node is recovering")) {
      return true;
   }

   /* Change Streams Spec resumable criteria: "any server error response from a
    * getMore command excluding those containing the following error codes" */
   switch (error.code) {
   case 11601:                      /* Interrupted */
   case 136:                        /* CappedPositionLost */
   case 237:                        /* CursorKilled */
   case MONGOC_ERROR_QUERY_FAILURE: /* error code omitted */
      return false;
   default:
      return true;
   }
}
/* construct the aggregate command in cmd. looks like one of the following:
 * for a collection change stream:
 *   { aggregate: collname, pipeline: [], cursor: { batchSize: x } }
 * for a database change stream:
 *   { aggregate: 1, pipeline: [], cursor: { batchSize: x } }
 * for a client change stream:
 *   { aggregate: 1, pipeline: [{$changeStream: {allChangesForCluster: true}}],
 *     cursor: { batchSize: x } }
 */
static void
_make_command (mongoc_change_stream_t *stream, bson_t *command)
{
   bson_iter_t iter;
   bson_t change_stream_stage; /* { $changeStream: <change_stream_doc> } */
   bson_t change_stream_doc;
   bson_t pipeline;
   bson_t cursor_doc;

   bson_init (command);
   if (stream->change_stream_type == MONGOC_CHANGE_STREAM_COLLECTION) {
      bson_append_utf8 (
         command, "aggregate", 9, stream->coll, (int) strlen (stream->coll));
   } else {
      bson_append_int32 (command, "aggregate", 9, 1);
   }

   bson_append_array_begin (command, "pipeline", 8, &pipeline);

   /* append the $changeStream stage. */
   bson_append_document_begin (&pipeline, "0", 1, &change_stream_stage);
   bson_append_document_begin (
      &change_stream_stage, "$changeStream", 13, &change_stream_doc);
   bson_concat (&change_stream_doc, stream->full_document);
   if (!bson_empty (&stream->resume_token)) {
      bson_concat (&change_stream_doc, &stream->resume_token);
   }
   /* Change streams spec: "startAtOperationTime and resumeAfter are mutually
    * exclusive; if both startAtOperationTime and resumeAfter are set, the
    * server will return an error. Drivers MUST NOT throw a custom error, and
    * MUST defer to the server error." */
   if (!_mongoc_timestamp_empty (&stream->operation_time)) {
      _mongoc_timestamp_append (
         &stream->operation_time, &change_stream_doc, "startAtOperationTime");
   }

   if (stream->change_stream_type == MONGOC_CHANGE_STREAM_CLIENT) {
      bson_append_bool (&change_stream_doc, "allChangesForCluster", 20, true);
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

      BSON_ASSERT (bson_iter_recurse (&iter, &child_iter));
      while (bson_iter_next (&child_iter)) {
         /* the user pipeline may consist of invalid stages or non-documents.
          * append anyway, and rely on the server error. */
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

/*---------------------------------------------------------------------------
 *
 * _make_cursor --
 *
 *       Construct and send the aggregate command and create the resulting
 *       cursor. On error, stream->cursor remains NULL, otherwise it is
 *       created and must be destroyed.
 *
 * Return:
 *       False on error and sets stream->err.
 *
 *--------------------------------------------------------------------------
 */
static bool
_make_cursor (mongoc_change_stream_t *stream)
{
   mongoc_client_session_t *cs = NULL;
   bson_t command_opts;
   bson_t command; /* { aggregate: "coll", pipeline: [], ... } */
   bson_t reply;
   bson_t getmore_opts = BSON_INITIALIZER;
   bson_iter_t iter;
   mongoc_server_description_t *sd;
   uint32_t server_id;
   int32_t max_wire_version = -1;

   BSON_ASSERT (stream);
   BSON_ASSERT (!stream->cursor);
   _make_command (stream, &command);
   bson_copy_to (&(stream->opts.extra), &command_opts);
   sd = mongoc_client_select_server (
      stream->client, false /* for_writes */, stream->read_prefs, &stream->err);
   if (!sd) {
      goto cleanup;
   }
   server_id = mongoc_server_description_id (sd);
   bson_append_int32 (&command_opts, "serverId", 8, server_id);
   bson_append_int32 (&getmore_opts, "serverId", 8, server_id);
   max_wire_version = sd->max_wire_version;
   mongoc_server_description_destroy (sd);

   if (bson_iter_init_find (&iter, &command_opts, "sessionId")) {
      if (!_mongoc_client_session_from_iter (
             stream->client, &iter, &cs, &stream->err)) {
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
      cs = mongoc_client_start_session (stream->client, session_opts, NULL);
      stream->implicit_session = cs;
      mongoc_session_opts_destroy (session_opts);
      if (cs &&
          !mongoc_client_session_append (cs, &command_opts, &stream->err)) {
         goto cleanup;
      }
   }

   if (cs && !mongoc_client_session_append (cs, &getmore_opts, &stream->err)) {
      goto cleanup;
   }

   if (stream->read_concern && !bson_has_field (&command_opts, "readConcern")) {
      mongoc_read_concern_append (stream->read_concern, &command_opts);
   }

   /* even though serverId has already been set, still pass the read prefs.
    * they are necessary for OP_MSG if sending to a secondary. */
   if (!mongoc_client_read_command_with_opts (stream->client,
                                              stream->db,
                                              &command,
                                              stream->read_prefs,
                                              &command_opts,
                                              &reply,
                                              &stream->err)) {
      bson_destroy (&stream->err_doc);
      bson_copy_to (&reply, &stream->err_doc);
      bson_destroy (&reply);
      goto cleanup;
   }

   bson_append_bool (
      &getmore_opts, MONGOC_CURSOR_TAILABLE, MONGOC_CURSOR_TAILABLE_LEN, true);
   bson_append_bool (&getmore_opts,
                     MONGOC_CURSOR_AWAIT_DATA,
                     MONGOC_CURSOR_AWAIT_DATA_LEN,
                     true);

   /* maxTimeMS is only appended to getMores if these are set in cursor opts. */
   if (stream->max_await_time_ms > 0) {
      bson_append_int64 (&getmore_opts,
                         MONGOC_CURSOR_MAX_AWAIT_TIME_MS,
                         MONGOC_CURSOR_MAX_AWAIT_TIME_MS_LEN,
                         stream->max_await_time_ms);
   }

   if (stream->batch_size > 0) {
      bson_append_int32 (&getmore_opts,
                         MONGOC_CURSOR_BATCH_SIZE,
                         MONGOC_CURSOR_BATCH_SIZE_LEN,
                         stream->batch_size);
   }

   /* Change streams spec: "If neither startAtOperationTime nor resumeAfter are
    * specified, and the max wire version is >= 7, and the initial aggregate
    * command does not return a resumeToken (indicating no results), the
    * ChangeStream MUST save the operationTime from the initial aggregate
    * command when it returns." */
   if (bson_empty (&stream->resume_token) &&
       _mongoc_timestamp_empty (&stream->operation_time) &&
       max_wire_version >= 7 &&
       bson_iter_init_find (&iter, &reply, "operationTime")) {
      _mongoc_timestamp_set_from_bson (&stream->operation_time, &iter);
   }
   /* steals reply. */
   stream->cursor = mongoc_cursor_new_from_command_reply_with_opts (
      stream->client, &reply, &getmore_opts);

cleanup:
   bson_destroy (&command);
   bson_destroy (&command_opts);
   bson_destroy (&getmore_opts);
   return stream->err.code == 0;
}

/*---------------------------------------------------------------------------
 *
 * _change_stream_init --
 *
 *       Called after @stream has the collection name, database name, read
 *       preferences, and read concern set. Creates the change streams
 *       cursor.
 *
 *--------------------------------------------------------------------------
 */
void
_change_stream_init (mongoc_change_stream_t *stream,
                     const bson_t *pipeline,
                     const bson_t *opts)
{
   BSON_ASSERT (pipeline);

   stream->max_await_time_ms = -1;
   stream->batch_size = -1;
   bson_init (&stream->pipeline_to_append);
   bson_init (&stream->resume_token);
   bson_init (&stream->err_doc);

   if (!_mongoc_change_stream_opts_parse (
          stream->client, opts, &stream->opts, &stream->err)) {
      return;
   }

   stream->full_document = BCON_NEW ("fullDocument", stream->opts.fullDocument);

   if (!bson_empty (&(stream->opts.resumeAfter))) {
      bson_append_document (
         &stream->resume_token, "resumeAfter", 11, &(stream->opts.resumeAfter));
   }

   _mongoc_timestamp_set (&stream->operation_time,
                          &(stream->opts.startAtOperationTime));

   stream->batch_size = stream->opts.batchSize;
   stream->max_await_time_ms = stream->opts.maxAwaitTimeMS;

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
         if (!BSON_APPEND_VALUE (&stream->pipeline_to_append,
                                 "pipeline",
                                 bson_iter_value (&iter))) {
            CHANGE_STREAM_ERR ("pipeline");
         }
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
}

mongoc_change_stream_t *
_mongoc_change_stream_new_from_collection (const mongoc_collection_t *coll,
                                           const bson_t *pipeline,
                                           const bson_t *opts)
{
   mongoc_change_stream_t *stream;
   BSON_ASSERT (coll);

   stream =
      (mongoc_change_stream_t *) bson_malloc0 (sizeof (mongoc_change_stream_t));
   bson_strncpy (stream->db, coll->db, sizeof (stream->db));
   bson_strncpy (stream->coll, coll->collection, sizeof (stream->coll));
   stream->read_prefs = mongoc_read_prefs_copy (coll->read_prefs);
   stream->read_concern = mongoc_read_concern_copy (coll->read_concern);
   stream->client = coll->client;
   stream->change_stream_type = MONGOC_CHANGE_STREAM_COLLECTION;
   _change_stream_init (stream, pipeline, opts);
   return stream;
}

mongoc_change_stream_t *
_mongoc_change_stream_new_from_database (const mongoc_database_t *db,
                                         const bson_t *pipeline,
                                         const bson_t *opts)
{
   mongoc_change_stream_t *stream;
   BSON_ASSERT (db);

   stream =
      (mongoc_change_stream_t *) bson_malloc0 (sizeof (mongoc_change_stream_t));
   bson_strncpy (stream->db, db->name, sizeof (stream->db));
   stream->coll[0] = '\0';
   stream->read_prefs = mongoc_read_prefs_copy (db->read_prefs);
   stream->read_concern = mongoc_read_concern_copy (db->read_concern);
   stream->client = db->client;
   stream->change_stream_type = MONGOC_CHANGE_STREAM_DATABASE;
   _change_stream_init (stream, pipeline, opts);
   return stream;
}

mongoc_change_stream_t *
_mongoc_change_stream_new_from_client (mongoc_client_t *client,
                                       const bson_t *pipeline,
                                       const bson_t *opts)
{
   mongoc_change_stream_t *stream;
   BSON_ASSERT (client);

   stream =
      (mongoc_change_stream_t *) bson_malloc0 (sizeof (mongoc_change_stream_t));
   bson_strncpy (stream->db, "admin", sizeof (stream->db));
   stream->coll[0] = '\0';
   stream->read_prefs = mongoc_read_prefs_copy (client->read_prefs);
   stream->read_concern = mongoc_read_concern_copy (client->read_concern);
   stream->client = client;
   stream->change_stream_type = MONGOC_CHANGE_STREAM_CLIENT;
   _change_stream_init (stream, pipeline, opts);
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
         /* no error occurred, just no documents left. */
         goto end;
      }

      resumable = _is_resumable_error (err_doc);
      if (resumable) {
         /* recreate the cursor. */
         mongoc_cursor_destroy (stream->cursor);
         stream->cursor = NULL;
         if (!_make_cursor (stream)) {
            goto end;
         }
         if (!mongoc_cursor_next (stream->cursor, bson)) {
            resumable =
               !mongoc_cursor_error_document (stream->cursor, &err, &err_doc);
            if (resumable) {
               /* empty batch. */
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

   /* we have received documents, either from the first call to next or after a
    * resume. */
   if (!bson_iter_init_find (&iter, *bson, "_id")) {
      bson_set_error (&stream->err,
                      MONGOC_ERROR_CURSOR,
                      MONGOC_ERROR_CHANGE_STREAM_NO_RESUME_TOKEN,
                      "Cannot provide resume functionality when the resume "
                      "token is missing");
      goto end;
   }

   /* copy the resume token. */
   bson_reinit (&stream->resume_token);
   BSON_APPEND_VALUE (
      &stream->resume_token, "resumeAfter", bson_iter_value (&iter));
   /* clear out the operation time, since we no longer need it to resume. */
   _mongoc_timestamp_clear (&stream->operation_time);
   ret = true;

end:
   /* Driver Sessions Spec: "When an implicit session is associated with a
    * cursor for use with getMore operations, the session MUST be returned to
    * the pool immediately following a getMore operation that indicates that the
    * cursor has been exhausted." */
   if (stream->implicit_session) {
      /* if creating the change stream cursor errored, it may be null. */
      if (!stream->cursor || stream->cursor->cursor_id == 0) {
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

   if (bson) {
      *bson = NULL;
   }
   return false;
}

void
mongoc_change_stream_destroy (mongoc_change_stream_t *stream)
{
   if (!stream) {
      return;
   }

   bson_destroy (&stream->pipeline_to_append);
   bson_destroy (&stream->resume_token);
   bson_destroy (stream->full_document);
   bson_destroy (&stream->err_doc);
   _mongoc_change_stream_opts_cleanup (&stream->opts);
   mongoc_cursor_destroy (stream->cursor);
   mongoc_client_session_destroy (stream->implicit_session);
   mongoc_read_prefs_destroy (stream->read_prefs);
   mongoc_read_concern_destroy (stream->read_concern);

   bson_free (stream);
}
