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

#include <mongoc.h>
#include "mongoc-client-private.h"
#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "mongoc-change-stream-private.h"
#include "mongoc-cursor-private.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"
#include "TestSuite.h"

#define DESTROY_CHANGE_STREAM(cursor_id)                                      \
   do {                                                                       \
      future_t *_future = future_change_stream_destroy (stream);              \
      request_t *_request = mock_server_receives_command (                    \
         server,                                                              \
         "db",                                                                \
         MONGOC_QUERY_SLAVE_OK,                                               \
         "{ 'killCursors' : 'coll', 'cursors' : [ " #cursor_id " ] }");       \
      mock_server_replies_simple (_request,                                   \
                                  "{ 'cursorsKilled': [ " #cursor_id " ] }"); \
      future_wait (_future);                                                  \
      future_destroy (_future);                                               \
      request_destroy (_request);                                             \
   } while (0);

static int
test_framework_skip_if_not_single_version_5 (void)
{
   if (!TestSuite_CheckLive ()) {
      return 0;
   }
   return (test_framework_max_wire_version_at_least (5) &&
           !test_framework_is_replset () && !test_framework_is_mongos ())
             ? 1
             : 0;
}

static mongoc_collection_t *
drop_and_get_coll (mongoc_client_t *client,
                   const char *db_name,
                   const char *coll_name)
{
   mongoc_collection_t *coll =
      mongoc_client_get_collection (client, db_name, coll_name);
   mongoc_collection_drop (coll, NULL);
   return coll;
}

/* From Change Streams Spec tests:
 * "$changeStream must be the first stage in a change stream pipeline sent
 * to the server" */
static void
test_change_stream_pipeline (void)
{
   mock_server_t *server;
   request_t *request;
   future_t *future;
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   const bson_t *next_doc = NULL;
   bson_t *nonempty_pipeline =
      tmp_bson ("{ 'pipeline' : [ { '$project' : { 'ns': false } } ] }");

   server = mock_server_with_autoismaster (5);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   ASSERT (client);

   coll = mongoc_client_get_collection (client, "db", "coll");
   ASSERT (coll);

   future = future_collection_watch (coll, tmp_bson ("{}"), NULL);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{"
      "'aggregate' : 'coll',"
      "'pipeline' : "
      "   ["
      "      { '$changeStream':{ 'fullDocument' : 'default' } }"
      "   ],"
      "'cursor' : {}"
      "}");

   mock_server_replies_simple (
      request,
      "{'cursor' : {'id': 123, 'ns': 'db.coll', 'firstBatch': []}, 'ok': 1 }");

   stream = future_get_mongoc_change_stream_ptr (future);
   ASSERT (stream);

   future_destroy (future);
   request_destroy (request);

   future = future_change_stream_next (stream, &next_doc);

   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{'getMore': 123, 'collection': 'coll'}");
   mock_server_replies_simple (request,
                               "{'cursor' : { 'nextBatch' : [] }, 'ok': 1}");
   ASSERT (!future_get_bool (future));
   ASSERT (!mongoc_change_stream_error_document (stream, NULL, NULL));
   ASSERT (next_doc == NULL);
   future_destroy (future);
   request_destroy (request);

   /* Another call to next should produce another getMore */
   future = future_change_stream_next (stream, &next_doc);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 123, 'collection': 'coll' }");
   mock_server_replies_simple (request,
                               "{ 'cursor': { 'nextBatch': [] }, 'ok': 1 }");
   ASSERT (!future_get_bool (future));
   ASSERT (!mongoc_change_stream_error_document (stream, NULL, NULL));
   ASSERT (next_doc == NULL);
   future_destroy (future);
   request_destroy (request);

   DESTROY_CHANGE_STREAM (123);

   /* Test non-empty pipeline */
   future = future_collection_watch (coll, nonempty_pipeline, NULL);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{"
      "'aggregate' : 'coll',"
      "'pipeline' : "
      "   ["
      "      { '$changeStream':{ 'fullDocument' : 'default' } },"
      "      { '$project': { 'ns': false } }"
      "   ],"
      "'cursor' : {}"
      "}");
   mock_server_replies_simple (
      request,
      "{'cursor': {'id': 123, 'ns': 'db.coll','firstBatch': []},'ok': 1}");

   stream = future_get_mongoc_change_stream_ptr (future);
   ASSERT (stream);

   future_destroy (future);
   request_destroy (request);


   future = future_change_stream_next (stream, &next_doc);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 123, 'collection': 'coll' }");
   mock_server_replies_simple (request,
                               "{ 'cursor': { 'nextBatch': [] }, 'ok': 1 }");
   ASSERT (!future_get_bool (future));
   ASSERT (!mongoc_change_stream_error_document (stream, NULL, NULL));
   ASSERT (next_doc == NULL);
   future_destroy (future);
   request_destroy (request);

   DESTROY_CHANGE_STREAM (123);

   mongoc_client_destroy (client);
   mongoc_collection_destroy (coll);
   mock_server_destroy (server);
}

/* From Change Streams Spec tests:
 * "The watch helper must not throw a custom exception when executed against a
 * single server topology, but instead depend on a server error"
 */
static void
test_change_stream_live_single_server (void *test_ctx)
{
/* Temporarily skip on arm64 until mongod tested against is updated */
#ifndef __aarch64__
   mongoc_client_t *client = test_framework_client_new ();
   mongoc_collection_t *coll;
   bson_error_t error;
   mongoc_change_stream_t *stream;
   const bson_t *next_doc = NULL;
   const bson_t *reported_err_doc = NULL;
   const char *not_replset_doc = "{'errmsg': 'The $changeStream stage is "
                                 "only supported on replica sets', 'code': "
                                 "40573, 'ok': 0}";

   /* Don't use the errmsg field since it contains quotes. */
   const char *not_supported_doc = "{'code' : 40324, 'ok' : 0 }";

   ASSERT (client);

   coll = mongoc_client_get_collection (client, "db", "coll");
   ASSERT (coll);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &error),
      error);

   stream = mongoc_collection_watch (coll, tmp_bson ("{}"), NULL);
   ASSERT (stream);

   ASSERT (
      mongoc_change_stream_error_document (stream, NULL, &reported_err_doc));
   ASSERT (next_doc == NULL);

   if (test_framework_max_wire_version_at_least (6)) {
      ASSERT_MATCH (reported_err_doc, not_replset_doc);
   } else {
      ASSERT_MATCH (reported_err_doc, not_supported_doc);
      ASSERT_CONTAINS (bson_lookup_utf8 (reported_err_doc, "errmsg"),
                       "Unrecognized pipeline stage");
   }

   mongoc_change_stream_destroy (stream);
   mongoc_client_destroy (client);
   mongoc_collection_destroy (coll);
#endif
}


typedef struct _test_resume_token_ctx_t {
   bool expecting_resume_token;
   const bson_t *expected_resume_token_bson;
} test_resume_token_ctx_t;

static void
test_resume_token_command_start (const mongoc_apm_command_started_t *event)
{
   const bson_t *cmd = mongoc_apm_command_started_get_command (event);
   const char *cmd_name = mongoc_apm_command_started_get_command_name (event);

   test_resume_token_ctx_t *ctx =
      (test_resume_token_ctx_t *) mongoc_apm_command_started_get_context (
         event);

   if (strcmp (cmd_name, "aggregate") == 0) {
      if (ctx->expecting_resume_token) {
         char *rt_pattern = bson_as_canonical_extended_json (
            ctx->expected_resume_token_bson, NULL);
         char *pattern =
            bson_strdup_printf ("{'aggregate': 'coll_resume', 'pipeline': "
                                "[{'$changeStream': %s }]}",
                                rt_pattern);
         ASSERT_MATCH (cmd, pattern);
         bson_free (pattern);
         bson_free (rt_pattern);
      } else {
         ASSERT_MATCH (cmd,
                       "{'aggregate': 'coll_resume', 'pipeline': [{ "
                       "'$changeStream': { 'resumeAfter': { '$exists': "
                       "false } }}]}");
      }
   }
}

/* From Change Streams Spec tests:
 * "ChangeStream must continuously track the last seen resumeToken"
 * Note: we should not inspect the resume token, since the format may change.
 */
static void
test_change_stream_live_track_resume_token (void *test_ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   bson_error_t error;
   test_resume_token_ctx_t ctx = {0};
   const bson_t *next_doc = NULL;
   mongoc_apm_callbacks_t *callbacks;
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   bson_t opts = BSON_INITIALIZER;
   bson_t doc0_rt, doc1_rt, doc2_rt;

   client = test_framework_client_new ();
   ASSERT (client);

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks,
                                      test_resume_token_command_start);
   mongoc_client_set_apm_callbacks (client, callbacks, &ctx);

   coll = drop_and_get_coll (client, "db", "coll_resume");
   ASSERT (coll);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &error),
      error);

   /* Set the batch size to 1 so we only get one document per call to next. */
   stream = mongoc_collection_watch (
      coll, tmp_bson ("{}"), tmp_bson ("{'batchSize': 1}"));
   ASSERT (stream);
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &error, NULL),
                    error);

   /* Insert a few docs to listen for. Use write concern majority, so subsequent
    * call to watch will be guaranteed to retrieve them. */
   mongoc_write_concern_set_wmajority (wc, 30000);
   mongoc_write_concern_append (wc, &opts);
   ASSERT_OR_PRINT (mongoc_collection_insert_one (
                       coll, tmp_bson ("{'_id': 0}"), &opts, NULL, &error),
                    error);

   ASSERT_OR_PRINT (mongoc_collection_insert_one (
                       coll, tmp_bson ("{'_id': 1}"), &opts, NULL, &error),
                    error);

   ASSERT_OR_PRINT (mongoc_collection_insert_one (
                       coll, tmp_bson ("{'_id': 2}"), &opts, NULL, &error),
                    error);

   /* The resume token should be updated to the most recently iterated doc */
   ASSERT (mongoc_change_stream_next (stream, &next_doc));
   ASSERT (next_doc);
   ASSERT (!bson_empty (&stream->resume_token));
   bson_copy_to (&stream->resume_token, &doc0_rt);

   ASSERT (mongoc_change_stream_next (stream, &next_doc));
   ASSERT (next_doc);
   ASSERT (!bson_empty (&stream->resume_token));
   ASSERT (bson_compare (&stream->resume_token, &doc0_rt) != 0);
   bson_copy_to (&stream->resume_token, &doc1_rt);

   _mongoc_client_kill_cursor (client,
                               stream->cursor->server_id,
                               mongoc_cursor_get_id (stream->cursor),
                               1 /* operation id */,
                               "db",
                               "coll_resume",
                               NULL /* session */);

   /* Now that the cursor has been killed, the next call to next will have to
    * resume, forcing it to send the resumeAfter token in the aggregate cmd. */
   ctx.expecting_resume_token = true;
   ctx.expected_resume_token_bson = &doc1_rt;
   ASSERT (mongoc_change_stream_next (stream, &next_doc));

   ASSERT (next_doc);
   ASSERT (!bson_empty (&stream->resume_token));
   ASSERT (bson_compare (&stream->resume_token, &doc0_rt) != 0);
   ASSERT (bson_compare (&stream->resume_token, &doc1_rt) != 0);
   bson_copy_to (&stream->resume_token, &doc2_rt);

   /* There are no docs left. But the next call should still keep the same
    * resume token */
   ASSERT (!mongoc_change_stream_next (stream, &next_doc));
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &error, NULL),
                    error);
   ASSERT (!next_doc);
   ASSERT (!bson_empty (&stream->resume_token));
   ASSERT (bson_compare (&stream->resume_token, &doc2_rt) == 0);

   bson_destroy (&doc0_rt);
   bson_destroy (&doc1_rt);
   bson_destroy (&doc2_rt);
   bson_destroy (&opts);
   mongoc_write_concern_destroy (wc);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_change_stream_destroy (stream);
   mongoc_client_destroy (client);
   mongoc_collection_destroy (coll);
}

typedef struct _test_batch_size_ctx {
   uint32_t num_get_mores;
   uint32_t expected_getmore_batch_size;
   uint32_t expected_agg_batch_size;
} test_batch_size_ctx_t;

static void
test_batch_size_command_succeeded (const mongoc_apm_command_succeeded_t *event)
{
   const bson_t *reply = mongoc_apm_command_succeeded_get_reply (event);
   const char *cmd_name = mongoc_apm_command_succeeded_get_command_name (event);

   test_batch_size_ctx_t *ctx =
      (test_batch_size_ctx_t *) mongoc_apm_command_succeeded_get_context (
         event);

   if (strcmp (cmd_name, "getMore") == 0) {
      bson_t next_batch;
      ++ctx->num_get_mores;
      bson_lookup_doc (reply, "cursor.nextBatch", &next_batch);
      ASSERT (bson_count_keys (&next_batch) ==
              ctx->expected_getmore_batch_size);
   } else if (strcmp (cmd_name, "aggregate") == 0) {
      bson_t first_batch;
      bson_lookup_doc (reply, "cursor.firstBatch", &first_batch);
      ASSERT (bson_count_keys (&first_batch) == ctx->expected_agg_batch_size);
   }
}
/* Test that the batch size option applies to both the initial aggregate and
 * subsequent getMore commands.
 */
static void
test_change_stream_live_batch_size (void *test_ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   test_batch_size_ctx_t ctx = {0};
   const bson_t *next_doc = NULL;
   mongoc_apm_callbacks_t *callbacks;
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   bson_t opts = BSON_INITIALIZER;
   bson_error_t err;
   uint32_t i;

   client = test_framework_client_new ();
   ASSERT (client);

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_succeeded_cb (callbacks,
                                        test_batch_size_command_succeeded);
   mongoc_client_set_apm_callbacks (client, callbacks, &ctx);

   coll = drop_and_get_coll (client, "db", "coll_batch");
   ASSERT (coll);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &err),
      err);

   stream = mongoc_collection_watch (
      coll, tmp_bson ("{}"), tmp_bson ("{'batchSize': 1}"));
   ASSERT (stream);

   ctx.expected_agg_batch_size = 0;
   ctx.expected_getmore_batch_size = 0;

   ASSERT (!mongoc_change_stream_next (stream, &next_doc));
   ASSERT (!mongoc_change_stream_error_document (stream, NULL, NULL));
   ASSERT (next_doc == NULL);

   ctx.expected_getmore_batch_size = 1;

   mongoc_write_concern_set_wmajority (wc, 30000);
   mongoc_write_concern_append (wc, &opts);
   for (i = 0; i < 10; i++) {
      bson_t *doc = BCON_NEW ("_id", BCON_INT32 (i));
      ASSERT_OR_PRINT (
         mongoc_collection_insert_one (coll, doc, &opts, NULL, &err), err);
      bson_free (doc);
   }

   ctx.expected_getmore_batch_size = 1;
   for (i = 0; i < 10; i++) {
      mongoc_change_stream_next (stream, &next_doc);
   }

   ctx.expected_getmore_batch_size = 0;
   ASSERT (!mongoc_change_stream_next (stream, &next_doc));
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, NULL),
                    err);
   ASSERT (next_doc == NULL);

   /* 10 getMores for results, 1 for initial next, 1 for last empty next */
   ASSERT (ctx.num_get_mores == 12);

   bson_destroy (&opts);
   mongoc_write_concern_destroy (wc);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_change_stream_destroy (stream);
   mongoc_client_destroy (client);
   mongoc_collection_destroy (coll);
}


/* From Change Streams Spec tests:
 * "ChangeStream will throw an exception if the server response is missing the
 * resume token." In the C driver case, return an error.
 */
static void
test_change_stream_live_missing_resume_token (void *test_ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   const bson_t *next_doc = NULL;
   mongoc_change_stream_t *stream;
   bson_error_t err;
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   bson_t opts = BSON_INITIALIZER;

   client = test_framework_client_new ();
   ASSERT (client);

   coll = drop_and_get_coll (client, "db", "coll_missing_resume");
   ASSERT (coll);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &err),
      err);

   stream = mongoc_collection_watch (
      coll, tmp_bson ("{'pipeline': [{'$project': {'_id': 0 }}]}"), NULL);

   ASSERT (stream);
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, NULL),
                    err);

   mongoc_write_concern_set_wmajority (wc, 30000);
   mongoc_write_concern_append (wc, &opts);
   ASSERT_OR_PRINT (mongoc_collection_insert_one (
                       coll, tmp_bson ("{'_id': 2}"), &opts, NULL, &err),
                    err);

   ASSERT (!mongoc_change_stream_next (stream, &next_doc));
   ASSERT (mongoc_change_stream_error_document (stream, &err, NULL));
   ASSERT_ERROR_CONTAINS (err,
                          MONGOC_ERROR_CURSOR,
                          MONGOC_ERROR_CHANGE_STREAM_NO_RESUME_TOKEN,
                          "Cannot provide resume functionality");

   bson_destroy (&opts);
   mongoc_write_concern_destroy (wc);
   mongoc_change_stream_destroy (stream);
   mongoc_client_destroy (client);
   mongoc_collection_destroy (coll);
}

/* From Change Streams Spec tests:
 * "ChangeStream will automatically resume one time on a resumable error
 * (including not master) with the initial pipeline and options, except for the
 * addition/update of a resumeToken"
 * "The killCursors command sent during the “Resume Process” must not be
 * allowed to throw an exception."
 */
static void
test_change_stream_resumable_error ()
{
   mock_server_t *server;
   request_t *request;
   future_t *future;
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   mongoc_uri_t *uri;
   bson_error_t err;
   const bson_t *err_doc = NULL;
   const bson_t *next_doc = NULL;
   const char *not_master_err =
      "{ 'code': 10107, 'errmsg': 'not master', 'ok': 0 }";
   const char *watch_cmd =
      "{ 'aggregate': 'coll', 'pipeline' "
      ": [ { '$changeStream': { 'fullDocument': 'default' } } ], "
      "'cursor': {  } }";

   server = mock_server_with_autoismaster (5);
   mock_server_run (server);

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "socketTimeoutMS", 100);
   client = mongoc_client_new_from_uri (uri);
   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);
   coll = mongoc_client_get_collection (client, "db", "coll");

   future = future_collection_watch (coll, tmp_bson ("{}"), NULL);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, watch_cmd);

   mock_server_replies_simple (request,
                               "{'cursor': {'id': 123, 'ns': "
                               "'db.coll','firstBatch': []},'ok': 1 "
                               "}");

   stream = future_get_mongoc_change_stream_ptr (future);
   ASSERT (stream);

   future_destroy (future);
   request_destroy (request);

   future = future_change_stream_next (stream, &next_doc);

   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 123, 'collection': 'coll' }");
   mock_server_replies_simple (request, not_master_err);
   request_destroy (request);
   /* On a resumable error, the change stream establishes a new cursor with the
    * same command. No killCursors since "not master" caused the driver to
    * disconnect from the server.
    */
   /* Retry command */
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, watch_cmd);
   mock_server_replies_simple (
      request,
      "{'cursor': {'id': 124,'ns': 'db.coll','firstBatch': []},'ok': 1 }");
   request_destroy (request);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 124, 'collection': 'coll' }");
   mock_server_replies_simple (request,
                               "{ 'cursor': { 'nextBatch': [] }, 'ok': 1 }");
   request_destroy (request);
   ASSERT (!future_get_bool (future));
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, NULL),
                    err);
   ASSERT (next_doc == NULL);
   future_destroy (future);

   /* Test that a network hangup also results in a resumable error */
   future = future_change_stream_next (stream, &next_doc);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 124, 'collection': 'coll' }");
   mock_server_hangs_up (request);
   request_destroy (request);

   /* Retry command */
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, watch_cmd);
   mock_server_replies_simple (
      request,
      "{'cursor': {'id': 125,'ns': 'db.coll','firstBatch': []},'ok': 1 }");
   request_destroy (request);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 125, 'collection': 'coll' }");
   mock_server_replies_simple (request,
                               "{ 'cursor': { 'nextBatch': [] }, 'ok': 1 }");
   request_destroy (request);
   ASSERT (!future_get_bool (future));
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, NULL),
                    err);
   ASSERT (next_doc == NULL);
   future_destroy (future);

   /* Test the "ismaster" resumable error occuring twice in a row */
   future = future_change_stream_next (stream, &next_doc);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 125, 'collection': 'coll' }");
   mock_server_replies_simple (request, not_master_err);

   request_destroy (request);

   /* Retry command */
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, watch_cmd);
   mock_server_replies_simple (request,
                               "{'cursor': {'id': 126, 'ns': "
                               "'db.coll','firstBatch': []},'ok': 1 "
                               "}");
   request_destroy (request);

   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 126, 'collection': 'coll' }");
   mock_server_replies_simple (request, not_master_err);
   request_destroy (request);

   /* Check that error is returned */
   ASSERT (!future_get_bool (future));
   ASSERT (mongoc_change_stream_error_document (stream, &err, &err_doc));
   ASSERT (next_doc == NULL);
   ASSERT_ERROR_CONTAINS (err, MONGOC_ERROR_SERVER, 10107, "not master");
   ASSERT_MATCH (err_doc, not_master_err);
   future_destroy (future);
   mongoc_change_stream_destroy (stream);

   /* Test an error on the initial aggregate when resuming. */
   future = future_collection_watch (coll, tmp_bson ("{}"), NULL);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, watch_cmd);
   mock_server_replies_simple (request,
                               "{'cursor': {'id': 123, 'ns': "
                               "'db.coll','firstBatch': []},'ok': 1 "
                               "}");
   stream = future_get_mongoc_change_stream_ptr (future);
   ASSERT (stream);
   request_destroy (request);
   future_destroy (future);

   future = future_change_stream_next (stream, &next_doc);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 123, 'collection': 'coll' }");
   mock_server_replies_simple (
      request, "{ 'code': 10107, 'errmsg': 'not master', 'ok': 0 }");
   request_destroy (request);

   /* Retry command */
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, watch_cmd);
   mock_server_replies_simple (request,
                               "{'code': 123, 'errmsg': 'bad cmd', 'ok': 0}");
   request_destroy (request);

   /* Check that error is returned */
   ASSERT (!future_get_bool (future));
   ASSERT (mongoc_change_stream_error_document (stream, &err, &err_doc));
   ASSERT (next_doc == NULL);
   ASSERT_ERROR_CONTAINS (err, MONGOC_ERROR_SERVER, 123, "bad cmd");
   ASSERT_MATCH (err_doc, "{'code': 123, 'errmsg': 'bad cmd', 'ok': 0}");
   future_destroy (future);

   mongoc_change_stream_destroy (stream);
   mongoc_uri_destroy (uri);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

/* From Change Streams Spec tests:
 * "ChangeStream will not attempt to resume on a server error"
 */
static void
test_change_stream_nonresumable_error ()
{
   mock_server_t *server;
   request_t *request;
   future_t *future;
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   const bson_t *next_doc = NULL;

   server = mock_server_with_autoismaster (5);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   ASSERT (client);

   coll = mongoc_client_get_collection (client, "db", "coll");
   ASSERT (coll);

   future = future_collection_watch (coll, tmp_bson ("{}"), NULL);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{ 'aggregate': 'coll', 'pipeline' "
      ": [ { '$changeStream': { 'fullDocument': 'default' } } ], "
      "'cursor': {  } }");

   mock_server_replies_simple (request,
                               "{'cursor': {'id': 123, 'ns': "
                               "'db.coll','firstBatch': []},'ok': 1 "
                               "}");
   stream = future_get_mongoc_change_stream_ptr (future);
   ASSERT (stream);
   future_destroy (future);
   request_destroy (request);

   future = future_change_stream_next (stream, &next_doc);
   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'getMore': 123, 'collection': 'coll' }");
   mock_server_replies_simple (
      request, "{ 'code': 1, 'errmsg': 'Internal Error', 'ok': 0 }");
   request_destroy (request);
   ASSERT (!future_get_bool (future));
   ASSERT (mongoc_change_stream_error_document (stream, NULL, NULL));
   ASSERT (next_doc == NULL);

   future_destroy (future);

   DESTROY_CHANGE_STREAM (123);

   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

/* Test that options are sent correctly.
 */
static void
test_change_stream_options (void)
{
   mock_server_t *server;
   request_t *request;
   future_t *future;
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   const bson_t *next_doc = NULL;
   bson_error_t err;

   server = mock_server_with_autoismaster (5);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   ASSERT (client);

   coll = mongoc_client_get_collection (client, "db", "coll");
   ASSERT (coll);


   /*
    * fullDocument: 'default'|'updateLookup', passed to $changeStream stage
    * resumeAfter: optional<Doc>, passed to $changeStream stage
    * maxAwaitTimeMS: Optional<Int64>, passed to cursor
    * batchSize: Optional<Int32>, passed as agg option, {cursor: { batchSize: }}
    * collation: Optional<Document>, passed as agg option
    */

   /* fullDocument */
   future = future_collection_watch (
      coll,
      tmp_bson ("{}"),
      tmp_bson ("{ 'fullDocument': 'updateLookup', "
                "'resumeAfter': {'_id': 0 }, "
                "'maxAwaitTimeMS': 5000, 'batchSize': "
                "5, 'collation': { 'locale': 'en' }}"));

   request = mock_server_receives_command (server,
                                           "db",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{"
                                           "'aggregate': 'coll',"
                                           "'pipeline': "
                                           "   ["
                                           "      { '$changeStream':{ "
                                           "'fullDocument': 'updateLookup', "
                                           "'resumeAfter': {'_id': 0 } } }"
                                           "   ],"
                                           "'cursor': { 'batchSize': 5 },"
                                           "'collation': { 'locale': 'en' }"
                                           "}");

   mock_server_replies_simple (
      request,
      "{'cursor': {'id': 123,'ns': 'db.coll','firstBatch': []},'ok': 1 }");

   stream = future_get_mongoc_change_stream_ptr (future);
   ASSERT (stream);
   future_destroy (future);
   request_destroy (request);

   future = future_change_stream_next (stream, &next_doc);
   request = mock_server_receives_command (server,
                                           "db",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{ 'getMore': 123, 'collection': "
                                           "'coll', 'maxTimeMS': 5000, "
                                           "'batchSize': 5 }");
   mock_server_replies_simple (request,
                               "{ 'cursor': { 'nextBatch': [] }, 'ok': 1 }");
   request_destroy (request);
   ASSERT (!future_get_bool (future));
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, NULL),
                    err);
   ASSERT (next_doc == NULL);
   future_destroy (future);

   DESTROY_CHANGE_STREAM (123);

   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

/* Test basic watch functionality and validate the server documents */
static void
test_change_stream_live_watch (void *test_ctx)
{
   mongoc_client_t *client = test_framework_client_new ();
   bson_t *inserted_doc = tmp_bson ("{ 'x': 'y'}");
   const bson_t *next_doc = NULL;
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   bson_t opts = BSON_INITIALIZER;
   bson_error_t err;

   mongoc_write_concern_set_wmajority (wc, 30000);

   coll = drop_and_get_coll (client, "db", "coll_watch");
   ASSERT (coll);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &err),
      err);

   stream = mongoc_collection_watch (coll, tmp_bson ("{}"), NULL);
   ASSERT (stream);
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, NULL),
                    err);

   /* Test that inserting a doc produces the expected change stream doc */
   mongoc_write_concern_append (wc, &opts);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, inserted_doc, &opts, NULL, &err),
      err);

   ASSERT (mongoc_change_stream_next (stream, &next_doc));

   /* Validation rules as follows:
    * { _id: <present>, operationType: "insert", ns: <doc>, documentKey:
    * <present>,
    *   updateDescription: <missing>, fullDocument: <inserted doc> }
    */
   ASSERT_HAS_FIELD (next_doc, "_id");
   ASSERT (!strcmp (bson_lookup_utf8 (next_doc, "operationType"), "insert"));

   ASSERT_MATCH (
      next_doc,
      "{ '_id': { '$exists': true },'operationType': 'insert', 'ns': "
      "{ 'db': 'db', 'coll': 'coll_watch' },'documentKey': { "
      "'$exists': true }, 'updateDescription': { '$exists': false }, "
      "'fullDocument': { '_id': { '$exists': true }, 'x': 'y' }}");

   /* Test updating a doc */
   ASSERT_OR_PRINT (
      mongoc_collection_update (coll,
                                MONGOC_UPDATE_NONE,
                                tmp_bson ("{}"),
                                tmp_bson ("{'$set': {'x': 'z'} }"),
                                wc,
                                &err),
      err);

   ASSERT (mongoc_change_stream_next (stream, &next_doc));

   ASSERT_MATCH (
      next_doc,
      "{ '_id': { '$exists': true },'operationType': 'update', 'ns': { 'db': "
      "'db', 'coll': 'coll_watch' },'documentKey': { '$exists': "
      "true }, 'updateDescription': { 'updatedFields': { 'x': 'z' } "
      "}, 'fullDocument': { '$exists': false }}");

   bson_destroy (&opts);
   mongoc_write_concern_destroy (wc);
   mongoc_change_stream_destroy (stream);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}

/* From Change Streams Spec tests:
 * "ChangeStream will resume after a killCursors command is issued for its child
 * cursor."
 * "ChangeStream will perform server selection before attempting to resume,
 * using initial readPreference"
 */
static void
test_change_stream_live_read_prefs (void *test_ctx)
{
   /*
    - connect with secondary read preference
    - verify we are connected to a secondary
    - issue a killCursors to trigger a resume
    - after resume, check that the cursor connected to a secondary
    */

   mongoc_read_prefs_t *prefs;
   mongoc_client_t *client = test_framework_client_new ();
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   mongoc_cursor_t *raw_cursor;
   const bson_t *next_doc = NULL;
   bson_error_t err;
   uint64_t first_cursor_id;

   coll = drop_and_get_coll (client, "db", "coll_read_prefs");
   ASSERT (coll);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &err),
      err);

   prefs = mongoc_read_prefs_copy (mongoc_collection_get_read_prefs (coll));
   mongoc_read_prefs_set_mode (prefs, MONGOC_READ_SECONDARY);
   mongoc_collection_set_read_prefs (coll, prefs);

   stream = mongoc_collection_watch (coll, tmp_bson ("{}"), NULL);
   ASSERT (stream);
   mongoc_change_stream_next (stream, &next_doc);

   raw_cursor = stream->cursor;
   ASSERT (raw_cursor);

   ASSERT (test_framework_server_is_secondary (client, raw_cursor->server_id));
   first_cursor_id = mongoc_cursor_get_id (raw_cursor);

   /* Call next to create the cursor, should return no documents. */
   ASSERT (!mongoc_change_stream_next (stream, &next_doc));
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, NULL),
                    err);

   _mongoc_client_kill_cursor (client,
                               raw_cursor->server_id,
                               mongoc_cursor_get_id (raw_cursor),
                               1 /* operation_id */,
                               "db",
                               "coll_read_prefs",
                               NULL /* session */);

   /* Change stream client will resume with another cursor. */
   ASSERT (!mongoc_change_stream_next (stream, &next_doc));
   ASSERT (!mongoc_change_stream_error_document (stream, &err, &next_doc));

   raw_cursor = stream->cursor;
   ASSERT (first_cursor_id != mongoc_cursor_get_id (raw_cursor));
   ASSERT (test_framework_server_is_secondary (client, raw_cursor->server_id));

   mongoc_read_prefs_destroy (prefs);
   mongoc_change_stream_destroy (stream);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}

/* Test that a failed server selection returns an error. This verifies a bug
 * is fixed, which would trigger an assert in this case. */
static void
test_change_stream_server_selection_fails (void)
{
   const bson_t *bson;
   bson_error_t err;
   mongoc_client_t *client = mongoc_client_new ("mongodb://localhost:12345/");
   mongoc_collection_t *coll =
      mongoc_client_get_collection (client, "test", "test");
   mongoc_change_stream_t *cs =
      mongoc_collection_watch (coll, tmp_bson ("{}"), NULL);

   mongoc_change_stream_next (cs, &bson);
   BSON_ASSERT (mongoc_change_stream_error_document (cs, &err, &bson));
   ASSERT_ERROR_CONTAINS (err,
                          MONGOC_ERROR_SERVER_SELECTION,
                          MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                          "No suitable servers found");
   mongoc_change_stream_destroy (cs);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}

/* Test calling next on a change stream which errors after construction. This
 * verifies a bug is fixed, which would try to access a NULL cursor. */
static void
test_change_stream_next_after_error (void *test_ctx)
{
   mongoc_client_t *client = test_framework_client_new ();
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   const bson_t *bson;
   bson_error_t err;

   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);
   coll = mongoc_client_get_collection (client, "db", "coll");
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &err),
      err);
   stream = mongoc_collection_watch (
      coll, tmp_bson ("{'pipeline': ['invalid_stage']}"), NULL);
   BSON_ASSERT (!mongoc_change_stream_next (stream, &bson));
   BSON_ASSERT (mongoc_change_stream_error_document (stream, &err, &bson));
   BSON_ASSERT (err.domain == MONGOC_ERROR_SERVER);
   mongoc_change_stream_destroy (stream);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}

typedef struct {
   char *pattern;
   int agg_count;
} array_started_ctx_t;

static void
_accepts_array_started (const mongoc_apm_command_started_t *event)
{
   const bson_t *cmd = mongoc_apm_command_started_get_command (event);
   const char *cmd_name = mongoc_apm_command_started_get_command_name (event);
   array_started_ctx_t *ctx =
      (array_started_ctx_t *) mongoc_apm_command_started_get_context (event);
   if (strcmp (cmd_name, "aggregate") != 0) {
      return;
   }
   ctx->agg_count++;
   ASSERT_MATCH (cmd, ctx->pattern);
}

/* Test that watch accepts an array document {0: {}, 1: {}} as the pipeline,
 * similar to mongoc_collection_aggregate */
static void
test_change_stream_accepts_array (void *test_ctx)
{
   mongoc_client_t *client = test_framework_client_new ();
   mongoc_apm_callbacks_t *callbacks = mongoc_apm_callbacks_new ();
   array_started_ctx_t ctx = {0};
   mongoc_collection_t *coll;
   mongoc_change_stream_t *stream;
   const bson_t *bson;
   bson_error_t err;
   bson_t *opts =
      tmp_bson ("{'maxAwaitTimeMS': 1}"); /* to speed up the test. */

   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);
   /* set up apm callbacks to listen for the agg commands. */
   ctx.pattern =
      bson_strdup ("{'aggregate': 'coll', 'pipeline': [ {'$changeStream': {}}, "
                   "{'$match': {'x': 1}}, {'$project': {'x': 1}}]}");
   mongoc_apm_set_command_started_cb (callbacks, _accepts_array_started);
   mongoc_client_set_apm_callbacks (client, callbacks, &ctx);
   coll = mongoc_client_get_collection (client, "db", "coll");
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, tmp_bson (NULL), NULL, NULL, &err),
      err);
   /* try starting a change stream with a { "pipeline": [...] } argument */
   stream = mongoc_collection_watch (
      coll,
      tmp_bson ("{'pipeline': [{'$match': {'x': 1}}, {'$project': {'x': 1}}]}"),
      opts);
   (void) mongoc_change_stream_next (stream, &bson);
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, &bson),
                    err);
   ASSERT_CMPINT32 (ctx.agg_count, ==, 1);
   mongoc_change_stream_destroy (stream);
   /* try with an array like document. */
   stream = mongoc_collection_watch (
      coll,
      tmp_bson ("{'0': {'$match': {'x': 1}}, '1': {'$project': {'x': 1}}}"),
      opts);
   (void) mongoc_change_stream_next (stream, &bson);
   ASSERT_OR_PRINT (!mongoc_change_stream_error_document (stream, &err, &bson),
                    err);
   ASSERT_CMPINT32 (ctx.agg_count, ==, 2);
   mongoc_change_stream_destroy (stream);
   /* try with malformed { "pipeline": [...] } argument. */
   bson_free (ctx.pattern);
   ctx.pattern = bson_strdup (
      "{'aggregate': 'coll', 'pipeline': [ {'$changeStream': {}}, 42 ]}");
   stream =
      mongoc_collection_watch (coll, tmp_bson ("{'pipeline': [42] }"), NULL);
   (void) mongoc_change_stream_next (stream, &bson);
   BSON_ASSERT (mongoc_change_stream_error_document (stream, &err, &bson));
   ASSERT_ERROR_CONTAINS (
      err,
      MONGOC_ERROR_SERVER,
      14,
      "Each element of the 'pipeline' array must be an object");
   ASSERT_CMPINT32 (ctx.agg_count, ==, 3);
   mongoc_change_stream_destroy (stream);
   /* try with malformed array doc argument. */
   stream = mongoc_collection_watch (coll, tmp_bson ("{'0': 42 }"), NULL);
   (void) mongoc_change_stream_next (stream, &bson);
   BSON_ASSERT (mongoc_change_stream_error_document (stream, &err, &bson));
   ASSERT_ERROR_CONTAINS (
      err,
      MONGOC_ERROR_SERVER,
      14,
      "Each element of the 'pipeline' array must be an object");
   ASSERT_CMPINT32 (ctx.agg_count, ==, 4);
   mongoc_change_stream_destroy (stream);
   bson_free (ctx.pattern);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}

void
test_change_stream_install (TestSuite *suite)
{
   TestSuite_AddMockServerTest (
      suite, "/change_stream/pipeline", test_change_stream_pipeline);

   TestSuite_AddFull (suite,
                      "/change_stream/live/single_server",
                      test_change_stream_live_single_server,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_single_version_5);

   TestSuite_AddFull (suite,
                      "/change_stream/live/track_resume_token",
                      test_change_stream_live_track_resume_token,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);

   TestSuite_AddFull (suite,
                      "/change_stream/live/batch_size",
                      test_change_stream_live_batch_size,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);

   TestSuite_AddFull (suite,
                      "/change_stream/live/missing_resume_token",
                      test_change_stream_live_missing_resume_token,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);

   TestSuite_AddMockServerTest (suite,
                                "/change_stream/resumable_error",
                                test_change_stream_resumable_error);

   TestSuite_AddMockServerTest (suite,
                                "/change_stream/nonresumable_error",
                                test_change_stream_nonresumable_error);

   TestSuite_AddMockServerTest (
      suite, "/change_stream/options", test_change_stream_options);

   TestSuite_AddFull (suite,
                      "/change_stream/live/watch",
                      test_change_stream_live_watch,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);

   TestSuite_AddFull (suite,
                      "/change_stream/live/read_prefs",
                      test_change_stream_live_read_prefs,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);

   TestSuite_Add (suite,
                  "/change_stream/server_selection_fails",
                  test_change_stream_server_selection_fails);

   TestSuite_AddFull (suite,
                      "/change_stream/next_after_error",
                      test_change_stream_next_after_error,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);

   TestSuite_AddFull (suite,
                      "/change_stream/accepts_array",
                      test_change_stream_accepts_array,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);
}
