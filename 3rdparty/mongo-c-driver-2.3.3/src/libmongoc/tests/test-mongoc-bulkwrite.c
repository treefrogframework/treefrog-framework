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

// This file includes tests `mongoc_bulkwrite_t` for basic usage and libmongoc-specific behavior.
// The specification tests (prose and JSON) include more coverage of driver-agnostic behavior.

#include <mongoc/mongoc-bulkwrite.h>
#include <mongoc/mongoc.h>

#include <TestSuite.h>
#include <mock_server/future-functions.h>
#include <mock_server/future.h>
#include <mock_server/mock-server.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

static void
test_bulkwrite_insert(void *unused)
{
   BSON_UNUSED(unused);

   bson_error_t error;
   bool ok;
   mongoc_client_t *client = test_framework_new_default_client();

   // Drop prior data.
   {
      mongoc_collection_t *coll = mongoc_client_get_collection(client, "db", "coll");
      mongoc_collection_drop(coll, NULL); // Ignore return.
      mongoc_collection_destroy(coll);
   }

   // Insert two documents with verbose results.
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{'_id': 123}"), NULL /* opts */, &error);
   ASSERT_OR_PRINT(ok, error);
   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{'_id': 456}"), NULL /* opts */, &error);
   ASSERT_OR_PRINT(ok, error);

   // Do the bulk write.
   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_verboseresults(opts, true);
   mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, opts);

   ASSERT_NO_BULKWRITEEXCEPTION(bwr);

   // Ensure results report IDs inserted.
   {
      ASSERT(bwr.res);
      const bson_t *insertResults = mongoc_bulkwriteresult_insertresults(bwr.res);
      ASSERT(insertResults);
      ASSERT_MATCH(insertResults, BSON_STR({"0" : {"insertedId" : 123}, "1" : {"insertedId" : 456}}));
   }

   mongoc_bulkwriteexception_destroy(bwr.exc);
   mongoc_bulkwriteresult_destroy(bwr.res);
   mongoc_bulkwrite_destroy(bw);
   mongoc_bulkwriteopts_destroy(opts);
   mongoc_client_destroy(client);
}

static void
test_bulkwrite_upsert_with_null(void *unused)
{
   BSON_UNUSED(unused);

   bson_error_t error;
   bool ok;
   mongoc_client_t *client = test_framework_new_default_client();

   // Drop prior data.
   {
      mongoc_collection_t *coll = mongoc_client_get_collection(client, "db", "coll");
      mongoc_collection_drop(coll, NULL); // Ignore return.
      mongoc_collection_destroy(coll);
   }

   // Upsert document with an `_id` of null.
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   mongoc_bulkwrite_replaceoneopts_t *roo = mongoc_bulkwrite_replaceoneopts_new();
   mongoc_bulkwrite_replaceoneopts_set_upsert(roo, true);
   ok = mongoc_bulkwrite_append_replaceone(bw, "db.coll", tmp_bson("{}"), tmp_bson("{'_id': null}"), roo, &error);
   ASSERT_OR_PRINT(ok, error);

   // Do the bulk write.
   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_verboseresults(opts, true);
   mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, opts);

   ASSERT_NO_BULKWRITEEXCEPTION(bwr);

   // Ensure results report null ID inserted.
   {
      ASSERT(bwr.res);
      const bson_t *updateResults = mongoc_bulkwriteresult_updateresults(bwr.res);
      ASSERT(updateResults);
      ASSERT_MATCH(updateResults, BSON_STR({"0" : {"matchedCount" : 1, "modifiedCount" : 0, "upsertedId" : null}}));
   }

   mongoc_bulkwriteexception_destroy(bwr.exc);
   mongoc_bulkwriteresult_destroy(bwr.res);
   mongoc_bulkwrite_destroy(bw);
   mongoc_bulkwriteopts_destroy(opts);
   mongoc_bulkwrite_replaceoneopts_destroy(roo);
   mongoc_client_destroy(client);
}

static void
test_bulkwrite_writeError(void *unused)
{
   BSON_UNUSED(unused);

   bson_error_t error;
   bool ok;
   mongoc_client_t *client = test_framework_new_default_client();

   // Drop prior data.
   {
      mongoc_collection_t *coll = mongoc_client_get_collection(client, "db", "coll");
      mongoc_collection_drop(coll, NULL); // Ignore return.
      mongoc_collection_destroy(coll);
   }

   // Insert two documents with verbose results.
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{'_id': 123}"), NULL /* opts */, &error);
   ASSERT_OR_PRINT(ok, error);
   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{'_id': 123}"), NULL /* opts */, &error);
   ASSERT_OR_PRINT(ok, error);

   // Do the bulk write.
   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_verboseresults(opts, true);
   mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, opts);

   // Expect an error.
   ASSERT(bwr.exc);
   const bson_t *ed = mongoc_bulkwriteexception_writeerrors(bwr.exc);
   ASSERT_MATCH(ed, BSON_STR({
                   "1" : {
                      "code" : 11000,
                      "message" : "E11000 duplicate key error collection: db.coll index: _id_ dup key: { _id: 123 }",
                      "details" : {}
                   }
                }));

   // Ensure results report only one ID inserted.
   ASSERT(bwr.res);
   const bson_t *insertResults = mongoc_bulkwriteresult_insertresults(bwr.res);
   ASSERT(insertResults);
   ASSERT_MATCH(insertResults, BSON_STR({"0" : {"insertedId" : 123}}));

   mongoc_bulkwriteexception_destroy(bwr.exc);
   mongoc_bulkwriteresult_destroy(bwr.res);
   mongoc_bulkwrite_destroy(bw);
   mongoc_bulkwriteopts_destroy(opts);
   mongoc_client_destroy(client);
}

static void
test_bulkwrite_session_with_unacknowledged(void *ctx)
{
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   mongoc_client_t *client = test_framework_new_default_client();
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   mongoc_client_session_t *session = mongoc_client_start_session(client, NULL, &error);
   ASSERT_OR_PRINT(session, error);
   mongoc_write_concern_t *wc = mongoc_write_concern_new();
   mongoc_write_concern_set_w(wc, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_ordered(opts, false);
   mongoc_bulkwriteopts_set_writeconcern(opts, wc);

   // Execute bulk write:
   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);
   mongoc_bulkwrite_set_session(bw, session);
   mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute(bw, opts);
   // Expect no result and an error:
   ASSERT(!ret.res);
   ASSERT(ret.exc);
   ASSERT(mongoc_bulkwriteexception_error(ret.exc, &error));
   ASSERT_ERROR_CONTAINS(error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "Cannot use client session with unacknowledged command");
   mongoc_bulkwriteresult_destroy(ret.res);
   mongoc_bulkwriteexception_destroy(ret.exc);
   mongoc_client_session_destroy(session);
   mongoc_bulkwriteopts_destroy(opts);
   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
   mongoc_write_concern_destroy(wc);
}

static void
test_bulkwrite_double_execute(void *ctx)
{
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   mongoc_client_t *client = test_framework_new_default_client();
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);
   // Execute.
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, NULL);
      ASSERT(bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
   }

   // Expect an error on reuse.
   ASSERT(!mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
   memset(&error, 0, sizeof(error));

   ASSERT(!mongoc_bulkwrite_append_updateone(bw, "db.coll", tmp_bson("{}"), tmp_bson("{}"), NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
   memset(&error, 0, sizeof(error));

   ASSERT(!mongoc_bulkwrite_append_updatemany(bw, "db.coll", tmp_bson("{}"), tmp_bson("{}"), NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
   memset(&error, 0, sizeof(error));

   ASSERT(!mongoc_bulkwrite_append_replaceone(bw, "db.coll", tmp_bson("{}"), tmp_bson("{}"), NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
   memset(&error, 0, sizeof(error));

   ASSERT(!mongoc_bulkwrite_append_deleteone(bw, "db.coll", tmp_bson("{}"), NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
   memset(&error, 0, sizeof(error));

   ASSERT(!mongoc_bulkwrite_append_deletemany(bw, "db.coll", tmp_bson("{}"), NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
   memset(&error, 0, sizeof(error));

   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, NULL);
      ASSERT(!bwr.res); // No result due to no successful writes.
      ASSERT(bwr.exc);
      ASSERT(mongoc_bulkwriteexception_error(bwr.exc, &error));
      ASSERT_ERROR_CONTAINS(
         error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
      mongoc_bulkwriteexception_destroy(bwr.exc);
      mongoc_bulkwriteresult_destroy(bwr.res);
   }

   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
}

static uint32_t
_select_server_and_get_id(mongoc_client_t *client)
{
   bson_error_t error;
   mongoc_server_description_t *const sd = mongoc_client_select_server(client, true /* for_writes */, NULL, &error);
   ASSERT_OR_PRINT(sd, error);

   uint32_t const selected_serverid = mongoc_server_description_id(sd);

   mongoc_server_description_destroy(sd);

   return selected_serverid;
}

static void
_capture_last_bulkWrite_serverid(const mongoc_apm_command_started_t *event)
{
   if (0 == strcmp(mongoc_apm_command_started_get_command_name(event), "bulkWrite")) {
      uint32_t *last_captured = mongoc_apm_command_started_get_context(event);
      *last_captured = mongoc_apm_command_started_get_server_id(event);
   }
}

static void
_setup_last_captured_serverid_callback(mongoc_client_t *client, uint32_t *id)
{
   mongoc_apm_callbacks_t *const cbs = mongoc_apm_callbacks_new();
   mongoc_apm_set_command_started_cb(cbs, _capture_last_bulkWrite_serverid);
   mongoc_client_set_apm_callbacks(client, cbs, id);
   mongoc_apm_callbacks_destroy(cbs);
}

static void
test_bulkwrite_serverid(void *ctx)
{
   mongoc_client_t *client;
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client();

   uint32_t const selected_serverid = _select_server_and_get_id(client);

   uint32_t last_captured = 0;
   _setup_last_captured_serverid_callback(client, &last_captured);

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   mongoc_bulkwriteopts_t *bwo = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_serverid(bwo, selected_serverid);

   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);

   // Getting the server ID before calling `mongoc_bulkwrite_execute` is an error
   {
      mongoc_bulkwrite_serverid_t const serverid_maybe = mongoc_bulkwrite_serverid(bw, &error);
      ASSERT(!serverid_maybe.is_ok);
      ASSERT_ERROR_CONTAINS(error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_COMMAND_INVALID_ARG,
                            "bulk write has not been executed or execution failed");
   }

   // Execute.
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, bwo);
      ASSERT(bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      // Expect the selected server is reported as used.
      mongoc_bulkwrite_serverid_t const serverid_maybe = mongoc_bulkwrite_serverid(bw, &error);
      ASSERT_OR_PRINT(serverid_maybe.is_ok, error);
      uint32_t const used_serverid = serverid_maybe.serverid;
      ASSERT_CMPUINT32(selected_serverid, ==, used_serverid);
      // Expect both mongoc_bulkwrite_t and mongoc_bulkwriteresult_t report the same server ID
      uint32_t const used_serverid_res = mongoc_bulkwriteresult_serverid(bwr.res);
      ASSERT_CMPUINT32(used_serverid, ==, used_serverid_res);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
   }

   // Expect the selected server is reported as used in command monitoring.
   ASSERT_CMPUINT32(last_captured, ==, selected_serverid);

   mongoc_bulkwriteopts_destroy(bwo);
   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
}

static void
_set_opts_for_unacknowledged_writes(mongoc_bulkwriteopts_t *opts)
{
   mongoc_bulkwriteopts_set_ordered(opts, false);

   mongoc_write_concern_t *const wc = mongoc_write_concern_new();
   mongoc_write_concern_set_w(wc, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);

   mongoc_bulkwriteopts_set_writeconcern(opts, wc);

   mongoc_write_concern_destroy(wc);
}

static void
test_bulkwrite_serverid_unacknowledged(void *ctx)
{
   mongoc_client_t *client;
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client();

   uint32_t const selected_serverid = _select_server_and_get_id(client);

   uint32_t last_captured = 0;
   _setup_last_captured_serverid_callback(client, &last_captured);

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   mongoc_bulkwriteopts_t *bwo = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_serverid(bwo, selected_serverid);

   _set_opts_for_unacknowledged_writes(bwo);

   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);
   // Execute.
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, bwo);
      ASSERT(!bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      // Expect the selected server is reported as used.
      mongoc_bulkwrite_serverid_t const serverid_maybe = mongoc_bulkwrite_serverid(bw, &error);
      ASSERT_OR_PRINT(serverid_maybe.is_ok, error);
      uint32_t const used_serverid = serverid_maybe.serverid;
      ASSERT_CMPUINT32(selected_serverid, ==, used_serverid);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
   }

   // Expect the selected server is reported as used in command monitoring.
   ASSERT_CMPUINT32(last_captured, ==, selected_serverid);

   mongoc_bulkwriteopts_destroy(bwo);
   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
}

static void
_setup_bulkwrite_fail_point(mongoc_client_t *client, uint32_t serverid)
{
   bson_error_t error;
   bool const ret =
      mongoc_client_command_simple_with_server_id(client,
                                                  "admin",
                                                  tmp_bson(BSON_STR({
                                                     "configureFailPoint" : "failCommand",
                                                     "mode" : {"times" : 1},
                                                     "data" : {"failCommands" : ["bulkWrite"], "closeConnection" : true}
                                                  })),
                                                  NULL,
                                                  serverid,
                                                  NULL,
                                                  &error);
   ASSERT_OR_PRINT(ret, error);
}

static void
test_bulkwrite_serverid_on_retry(void *ctx)
{
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   mongoc_uri_t *uri = test_framework_get_uri();
   ASSERT_OR_PRINT(test_framework_uri_apply_multi_mongos(uri, true, &error), error);
   mongoc_client_t *client = mongoc_client_new_from_uri(uri);
   test_framework_set_ssl_opts(client);

   uint32_t const selected_serverid = _select_server_and_get_id(client);

   _setup_bulkwrite_fail_point(client, selected_serverid);

   uint32_t last_captured = 0;
   _setup_last_captured_serverid_callback(client, &last_captured);

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   mongoc_bulkwriteopts_t *bwo = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_serverid(bwo, selected_serverid);

   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);
   // Execute.
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, bwo);
      ASSERT(bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      // Expect a different server was used due to retry.
      mongoc_bulkwrite_serverid_t const serverid_maybe = mongoc_bulkwrite_serverid(bw, &error);
      ASSERT_OR_PRINT(serverid_maybe.is_ok, error);
      uint32_t const used_serverid = serverid_maybe.serverid;
      ASSERT_CMPUINT32(selected_serverid, !=, used_serverid);
      // Expect both mongoc_bulkwrite_t and mongoc_bulkwriteresult_t report the same server ID
      uint32_t const used_serverid_res = mongoc_bulkwriteresult_serverid(bwr.res);
      ASSERT_CMPUINT32(used_serverid, ==, used_serverid_res);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
      // Expect the used server was reported in command monitoring.
      ASSERT_CMPUINT32(last_captured, ==, used_serverid);
   }

   mongoc_uri_destroy(uri);
   mongoc_bulkwriteopts_destroy(bwo);
   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
}

static void
capture_last_bulkWrite_command(const mongoc_apm_command_started_t *event)
{
   if (0 == strcmp(mongoc_apm_command_started_get_command_name(event), "bulkWrite")) {
      bson_t *last_captured = mongoc_apm_command_started_get_context(event);
      bson_destroy(last_captured);
      const bson_t *cmd = mongoc_apm_command_started_get_command(event);
      bson_copy_to(cmd, last_captured);
   }
}

static void
test_bulkwrite_extra(void *ctx)
{
   mongoc_client_t *client;
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client();

   bson_t last_captured = BSON_INITIALIZER;
   // Set callback to capture the last `bulkWrite` command.
   {
      mongoc_apm_callbacks_t *cbs = mongoc_apm_callbacks_new();
      mongoc_apm_set_command_started_cb(cbs, capture_last_bulkWrite_command);
      mongoc_client_set_apm_callbacks(client, cbs, &last_captured);
      mongoc_apm_callbacks_destroy(cbs);
   }

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   // Create bulk write.
   {
      ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
      ASSERT_OR_PRINT(ok, error);
   }

   mongoc_bulkwriteopts_t *bwo = mongoc_bulkwriteopts_new();
   // Create bulk write options with extra options.
   {
      bson_t *extra = tmp_bson("{'comment': 'foo'}");
      mongoc_bulkwriteopts_set_extra(bwo, extra);
   }

   // Execute.
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, bwo);
      ASSERT(bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
   }

   // Expect `bulkWrite` command was sent with extra option.
   ASSERT_MATCH(&last_captured, "{'comment': 'foo'}");

   mongoc_bulkwriteopts_destroy(bwo);
   mongoc_bulkwrite_destroy(bw);
   bson_destroy(&last_captured);
   mongoc_client_destroy(client);
}

static void
test_bulkwrite_no_verbose_results(void *ctx)
{
   mongoc_client_t *client;
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client();

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   // Create bulk write.
   {
      ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
      ASSERT_OR_PRINT(ok, error);

      ok =
         mongoc_bulkwrite_append_updateone(bw, "db.coll", tmp_bson("{}"), tmp_bson("{'$set': {'x': 1}}"), NULL, &error);
      ASSERT_OR_PRINT(ok, error);

      ok = mongoc_bulkwrite_append_deleteone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
      ASSERT_OR_PRINT(ok, error);
   }


   // Execute.
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, NULL /* opts */);
      ASSERT(bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      // Expect no verbose results.
      ASSERT(NULL == mongoc_bulkwriteresult_insertresults(bwr.res));
      ASSERT(NULL == mongoc_bulkwriteresult_updateresults(bwr.res));
      ASSERT(NULL == mongoc_bulkwriteresult_deleteresults(bwr.res));
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
   }

   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
}

static void
capture_all_bulkWrite_commands(const mongoc_apm_command_started_t *event)
{
   if (0 == strcmp(mongoc_apm_command_started_get_command_name(event), "bulkWrite")) {
      mongoc_array_t *captured = mongoc_apm_command_started_get_context(event);
      bson_t *cmd = bson_copy(mongoc_apm_command_started_get_command(event));
      _mongoc_array_append_val(captured, cmd);
   }
}

// `test_bulkwrite_many_namespaces` tests a bulk write with many unique namespace entries.
// An early implementation used linear look-up for namespaces. It resulted in a very long test runtime (30 minutes+).
// A hash map was used to improve the look-up.
static void
test_bulkwrite_many_namespaces(void *ctx)
{
   mongoc_client_t *client;
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client();

   mongoc_array_t captured;
   _mongoc_array_init(&captured, sizeof(bson_t *));
   // Set callback to capture all `bulkWrite` commands.
   {
      mongoc_apm_callbacks_t *cbs = mongoc_apm_callbacks_new();
      mongoc_apm_set_command_started_cb(cbs, capture_all_bulkWrite_commands);
      mongoc_client_set_apm_callbacks(client, cbs, &captured);
      mongoc_apm_callbacks_destroy(cbs);
   }

   // Get `maxWriteBatchSize` from the server.
   int32_t maxWriteBatchSize;
   {
      bson_t reply;

      ok = mongoc_client_command_simple(client, "admin", tmp_bson("{'hello': 1}"), NULL, &reply, &error);
      ASSERT_OR_PRINT(ok, error);

      maxWriteBatchSize = bson_lookup_int32(&reply, "maxWriteBatchSize");
      bson_destroy(&reply);
   }

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   // Create bulk write large enough to split into two batches. Use a unique namespace per model.
   {
      for (int32_t i = 0; i < maxWriteBatchSize + 1; i++) {
         char *ns = bson_strdup_printf("db.coll%" PRId32, i);
         ok = mongoc_bulkwrite_append_deleteone(bw, ns, tmp_bson("{}"), NULL, &error);
         ASSERT_OR_PRINT(ok, error);
         bson_free(ns);
      }
   }

   // Execute.
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, NULL /* opts */);
      ASSERT(bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
   }


   // Expect two `bulkWrite` commands were sent.
   ASSERT_CMPSIZE_T(captured.len, ==, 2);
   bson_t *first = _mongoc_array_index(&captured, bson_t *, 0);
   // Expect the first contains maxWriteBatchSize `nsInfo` entries:
   {
      bson_t *nsInfo = bson_lookup_bson(first, "nsInfo");
      ASSERT_CMPUINT32(bson_count_keys(nsInfo), ==, maxWriteBatchSize);
      bson_destroy(nsInfo);
   }
   // Expect the second only contains one `nsInfo` entry:
   bson_t *second = _mongoc_array_index(&captured, bson_t *, 1);
   {
      bson_t *nsInfo = bson_lookup_bson(second, "nsInfo");
      ASSERT_CMPUINT32(bson_count_keys(nsInfo), ==, 1);
      bson_destroy(nsInfo);
   }

   for (size_t i = 0; i < captured.len; i++) {
      bson_t *el = _mongoc_array_index(&captured, bson_t *, i);
      bson_destroy(el);
   }
   _mongoc_array_destroy(&captured);

   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
}

static void
test_bulkwrite_execute_requires_client(void *ctx)
{
   BSON_UNUSED(ctx);
   bool ok;
   bson_error_t error;

   mongoc_client_t *client = test_framework_new_default_client();
   mongoc_bulkwrite_t *bw = mongoc_bulkwrite_new();
   ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);

   // Attempt execution without assigning a client
   {
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, NULL);
      ASSERT(!bwr.res); // No result due to no successful writes.
      ASSERT(bwr.exc);
      ASSERT(mongoc_bulkwriteexception_error(bwr.exc, &error));
      ASSERT_ERROR_CONTAINS(error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_COMMAND_INVALID_ARG,
                            "bulk write requires a client and one has not been set");
      mongoc_bulkwriteexception_destroy(bwr.exc);
      mongoc_bulkwriteresult_destroy(bwr.res);
   }

   // Assign a client and execute successfully
   {
      mongoc_bulkwrite_set_client(bw, client);
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, NULL);
      ASSERT(bwr.res);
      ASSERT_NO_BULKWRITEEXCEPTION(bwr);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
   }

   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
}

// `test_bulkwrite_two_large_inserts` is a regression test for CDRIVER-5869.
static void
test_bulkwrite_two_large_inserts(void *unused)
{
   BSON_UNUSED(unused);

   bson_error_t error;
   mongoc_client_t *client = test_framework_new_default_client();

   // Drop prior collection:
   {
      mongoc_collection_t *coll = mongoc_client_get_collection(client, "db", "coll");
      mongoc_collection_drop(coll, NULL);
      mongoc_collection_destroy(coll);
   }

   // Allocate a large string:
   size_t large_len = 2095652;
   char *large_string = bson_malloc(large_len + 1);
   memset(large_string, 'a', large_len);
   large_string[large_len] = '\0';
   ASSERT(mlib_in_range(int, large_len));

   // Create two large documents:
   bson_t *docs[2];
   docs[0] = BCON_NEW("_id", "over_2mib_1");
   bson_append_utf8(docs[0], "unencrypted", -1, large_string, (int)large_len);
   docs[1] = BCON_NEW("_id", "over_2mib_2");
   bson_append_utf8(docs[1], "unencrypted", -1, large_string, (int)large_len);

   mongoc_bulkwriteopts_t *bw_opts = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_verboseresults(bw_opts, true);

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   ASSERT_OR_PRINT(mongoc_bulkwrite_append_insertone(bw, "db.coll", docs[0], NULL, &error), error);
   ASSERT_OR_PRINT(mongoc_bulkwrite_append_insertone(bw, "db.coll", docs[1], NULL, &error), error);

   mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, bw_opts);
   ASSERT(bwr.res);
   ASSERT_NO_BULKWRITEEXCEPTION(bwr);
   const bson_t *insertresults = mongoc_bulkwriteresult_insertresults(bwr.res);
   ASSERT_MATCH(insertresults,
                BSON_STR({"0" : {"insertedId" : "over_2mib_1"}}, {"1" : {"insertedId" : "over_2mib_2"}}));
   bson_destroy(docs[0]);
   bson_destroy(docs[1]);
   mongoc_bulkwrite_destroy(bw);
   mongoc_bulkwriteresult_destroy(bwr.res);
   mongoc_bulkwriteexception_destroy(bwr.exc);
   mongoc_bulkwriteopts_destroy(bw_opts);
   mongoc_client_destroy(client);
   bson_free(large_string);
}


// `test_bulkwrite_client_error_no_result` is a regression test for CDRIVER-5969.
static void
test_bulkwrite_client_error_no_result(void *unused)
{
   BSON_UNUSED(unused);

   bson_error_t error;
   mongoc_client_t *client = test_framework_new_default_client();
   // Trigger a client-side error by adding a too-big document.
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
      bson_t too_big = BSON_INITIALIZER;
      const size_t maxMessageSizeByte = 48000000;
      char *big_string = bson_malloc(maxMessageSizeByte + 1);
      memset(big_string, 'a', maxMessageSizeByte);
      big_string[maxMessageSizeByte] = '\0';
      BSON_APPEND_UTF8(&too_big, "big", big_string);
      ASSERT_OR_PRINT(mongoc_bulkwrite_append_insertone(bw, "db.coll", &too_big, NULL, &error), error);
      mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute(bw, NULL);
      ASSERT(!bwr.res); // No result due to no successful writes.
      ASSERT(bwr.exc);
      ASSERT(mongoc_bulkwriteexception_error(bwr.exc, &error));
      ASSERT_ERROR_CONTAINS(
         error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Sending would exceed maxMessageSizeBytes");
      bson_free(big_string);
      bson_destroy(&too_big);
      mongoc_bulkwriteresult_destroy(bwr.res);
      mongoc_bulkwriteexception_destroy(bwr.exc);
      mongoc_bulkwrite_destroy(bw);
   }

   mongoc_client_destroy(client);
}

static void
_do_bulkwrite_check_acknowledged_test_case(mongoc_client_t *client,
                                           mongoc_bulkwriteopts_t *opts,
                                           bool is_acknowledged_expected)
{
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);
   bson_error_t error;
   bool const ok = mongoc_bulkwrite_append_insertone(bw, "db.coll", tmp_bson("{}"), NULL /* opts */, &error);
   ASSERT_OR_PRINT(ok, error);

   mongoc_bulkwritereturn_t const bwr = mongoc_bulkwrite_execute(bw, opts);

   ASSERT_NO_BULKWRITEEXCEPTION(bwr);

   mongoc_bulkwrite_check_acknowledged_t const check_acknowledged = mongoc_bulkwrite_check_acknowledged(bw, &error);
   ASSERT_OR_PRINT(check_acknowledged.is_ok, error);
   ASSERT_CMPBOOL(check_acknowledged.is_acknowledged, ==, is_acknowledged_expected);

   mongoc_bulkwriteexception_destroy(bwr.exc);
   mongoc_bulkwriteresult_destroy(bwr.res);
   mongoc_bulkwrite_destroy(bw);
}

static void
test_bulkwrite_check_acknowledged(void *unused)
{
   BSON_UNUSED(unused);

   mongoc_client_t *client = test_framework_new_default_client();

   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new();
   // To prevent bulk write exception "Cannot request unacknowledged write concern and ordered writes."
   mongoc_bulkwriteopts_set_ordered(opts, false);

   mongoc_write_concern_t *wc_acknowledged = mongoc_write_concern_new();
   mongoc_write_concern_set_w(wc_acknowledged, 1);

   mongoc_write_concern_t *wc_unacknowledged = mongoc_write_concern_new();
   mongoc_write_concern_set_w(wc_unacknowledged, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);

   // Client w/ unacknowledged write concern, no opts override
   {
      mongoc_client_set_write_concern(client, wc_unacknowledged);
      mongoc_bulkwriteopts_set_writeconcern(opts, NULL);

      _do_bulkwrite_check_acknowledged_test_case(client, opts, false /* is_acknowledged_expected */);
   }

   // Client w/ acknowledged write concern, no opts override
   {
      mongoc_client_set_write_concern(client, wc_acknowledged);
      mongoc_bulkwriteopts_set_writeconcern(opts, NULL);

      _do_bulkwrite_check_acknowledged_test_case(client, opts, true /* is_acknowledged_expected */);
   }

   // Client w/ unacknowledged write concern, opts override w/ acknowledged write concern
   {
      mongoc_client_set_write_concern(client, wc_unacknowledged);
      mongoc_bulkwriteopts_set_writeconcern(opts, wc_acknowledged);

      _do_bulkwrite_check_acknowledged_test_case(client, opts, true /* is_acknowledged_expected */);
   }

   // Client w/ acknowledged write concern, opts override w/ unacknowledged write concern
   {
      mongoc_client_set_write_concern(client, wc_acknowledged);
      mongoc_bulkwriteopts_set_writeconcern(opts, wc_unacknowledged);

      _do_bulkwrite_check_acknowledged_test_case(client, opts, false /* is_acknowledged_expected */);
   }

   // Calling `mongoc_bulkwrite_check_acknowledged` before `mongoc_bulkwrite_execute` is an error
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);

      bson_error_t error;
      mongoc_bulkwrite_check_acknowledged_t const check_acknowledged = mongoc_bulkwrite_check_acknowledged(bw, &error);
      ASSERT(!check_acknowledged.is_ok);
      ASSERT_ERROR_CONTAINS(error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_COMMAND_INVALID_ARG,
                            "bulk write has not been executed or execution failed");

      mongoc_bulkwrite_destroy(bw);
   }

   mongoc_write_concern_destroy(wc_unacknowledged);
   mongoc_write_concern_destroy(wc_acknowledged);
   mongoc_bulkwriteopts_destroy(opts);
   mongoc_client_destroy(client);
}

// test_bulkwrite_missing_nModified mocks a server reply missing "nModified" in a per-operation update result.
// The missing "nModified" is a bug: SERVER-113026. This tests how the driver handles the reply.
static void
test_bulkwrite_missing_nModified(void)
{
   mock_server_t *server = mock_server_with_auto_hello(WIRE_VERSION_8_0);
   mock_server_run(server);
   mongoc_client_t *client = mongoc_client_new_from_uri(mock_server_get_uri(server));
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);

   bson_error_t error;
   bool ok = mongoc_bulkwrite_append_updateone(
      bw, "db.coll", tmp_bson("{'_id': 1}"), tmp_bson("{'$set': {'x': 1}}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);

   mongoc_bulkwriteopts_t *bwo = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_verboseresults(bwo, true);

   future_t *fut = future_bulkwrite_execute(bw, bwo);
   request_t *req = mock_server_receives_msg(server,
                                             MONGOC_MSG_NONE,
                                             tmp_bson("{'bulkWrite': 1, 'errorsOnly': false}"),
                                             tmp_bson("{'ns': 'db.coll'}"), // "nsInfo"
                                             tmp_bson("{'update': 0}"));    // "ops"
   reply_to_request_simple(req, BSON_STR({
                              "ok" : 1,
                              "nInserted" : 0,
                              "nMatched" : 0,
                              "nModified" : 0,
                              "nDeleted" : 0,
                              "nUpserted" : 0,
                              "nErrors" : 0,
                              "cursor" : {
                                 "id" : 0,
                                 "firstBatch" : [ {
                                    "ok" : 1,
                                    "idx" : 0,
                                    "n" : 0
                                    // Omit "nModified" to reproduce SERVER-113026.
                                 } ],
                                 "ns" : "admin.$cmd.bulkWrite"
                              }
                           }));
   mongoc_bulkwritereturn_t bwr = future_get_mongoc_bulkwritereturn_t(fut);

   // Expect no error:
   ASSERT_NO_BULKWRITEEXCEPTION(bwr);

   // Expect per-operation result has 0 "modifiedCount":
   ASSERT(bwr.res);
   const bson_t *updateResults = mongoc_bulkwriteresult_updateresults(bwr.res);
   ASSERT(updateResults);
   ASSERT_MATCH(updateResults, BSON_STR({"0" : {"matchedCount" : 0, "modifiedCount" : 0, "upsertedId" : null}}));

   future_destroy(fut);
   request_destroy(req);
   mongoc_bulkwriteexception_destroy(bwr.exc);
   mongoc_bulkwriteresult_destroy(bwr.res);
   mongoc_bulkwriteopts_destroy(bwo);
   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
   mock_server_destroy(server);
}

// test_bulkwrite_unexpected_results mocks a server reply including per-operation results even with `errorsOnly: true`.
// The unexpected results are a bug: SERVER-113344. This tests how the driver handles the reply.
static void
test_bulkwrite_unexpected_results(void)
{
   mock_server_t *server = mock_server_with_auto_hello(WIRE_VERSION_8_0);
   mock_server_run(server);
   mongoc_client_t *client = mongoc_client_new_from_uri(mock_server_get_uri(server));
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new(client);

   bson_error_t error;
   bool ok = mongoc_bulkwrite_append_updateone(
      bw, "db.coll", tmp_bson("{'_id': 1}"), tmp_bson("{'$set': {'x': 1}}"), NULL, &error);
   ASSERT_OR_PRINT(ok, error);

   mongoc_bulkwriteopts_t *bwo = mongoc_bulkwriteopts_new();
   mongoc_bulkwriteopts_set_verboseresults(bwo, false);

   future_t *fut = future_bulkwrite_execute(bw, bwo);
   request_t *req = mock_server_receives_msg(server,
                                             MONGOC_MSG_NONE,
                                             tmp_bson("{'bulkWrite': 1, 'errorsOnly': true }"),
                                             tmp_bson("{'ns': 'db.coll'}"), // "nsInfo"
                                             tmp_bson("{'update': 0}"));    // "ops"
   reply_to_request_simple(req, BSON_STR({
                              "ok" : 1,
                              "nInserted" : 0,
                              "nMatched" : 0,
                              "nModified" : 0,
                              "nDeleted" : 0,
                              "nUpserted" : 0,
                              "nErrors" : 0,
                              "cursor" : {
                                 "id" : 0,
                                 // Unexpected per-operation result:
                                 "firstBatch" : [ {"ok" : 1, "idx" : 0, "n" : 0, "nModified" : 0} ],
                                 "ns" : "admin.$cmd.bulkWrite"
                              }
                           }));
   mongoc_bulkwritereturn_t bwr = future_get_mongoc_bulkwritereturn_t(fut);

   // Expect no error:
   ASSERT_NO_BULKWRITEEXCEPTION(bwr);

   // Extra per-operation result is not reported:
   ASSERT(bwr.res);
   const bson_t *updateResults = mongoc_bulkwriteresult_updateresults(bwr.res);
   ASSERT(!updateResults);

   future_destroy(fut);
   request_destroy(req);
   mongoc_bulkwriteexception_destroy(bwr.exc);
   mongoc_bulkwriteresult_destroy(bwr.res);
   mongoc_bulkwriteopts_destroy(bwo);
   mongoc_bulkwrite_destroy(bw);
   mongoc_client_destroy(client);
   mock_server_destroy(server);
}

void
test_bulkwrite_install(TestSuite *suite)
{
   TestSuite_AddFull(suite,
                     "/bulkwrite/insert [lock:live-server]",
                     test_bulkwrite_insert,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/upsert_with_null [lock:live-server]",
                     test_bulkwrite_upsert_with_null,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/writeError [lock:live-server]",
                     test_bulkwrite_writeError,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/session_with_unacknowledged [lock:live-server]",
                     test_bulkwrite_session_with_unacknowledged,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25, // require server 8.0
                     test_framework_skip_if_no_sessions);

   TestSuite_AddFull(suite,
                     "/bulkwrite/double_execute [lock:live-server]",
                     test_bulkwrite_double_execute,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/server_id [lock:live-server]",
                     test_bulkwrite_serverid,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/server_id/unacknowledged [lock:live-server]",
                     test_bulkwrite_serverid_unacknowledged,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/server_id/on_retry [lock:live-server]",
                     test_bulkwrite_serverid_on_retry,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25, // require server 8.0
                     test_framework_skip_if_not_mongos, // Requires multiple hosts that can accept writes.
                     test_framework_skip_if_no_crypto   // Require crypto for retryable writes.
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/extra [lock:live-server]",
                     test_bulkwrite_extra,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/no_verbose_results [lock:live-server]",
                     test_bulkwrite_no_verbose_results,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/many_namespaces [lock:live-server]",
                     test_bulkwrite_many_namespaces,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25, // require server 8.0
                     test_framework_skip_if_mongos // Creating 100k collections is very slow (~5 minutes) on mongos.
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/execute_requires_client [lock:live-server]",
                     test_bulkwrite_execute_requires_client,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/two_large_inserts [lock:live-server]",
                     test_bulkwrite_two_large_inserts,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/client_error_no_result [lock:live-server]",
                     test_bulkwrite_client_error_no_result,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull(suite,
                     "/bulkwrite/check_acknowledged [lock:live-server]",
                     test_bulkwrite_check_acknowledged,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddMockServerTest(suite, "/bulkwrite/missing_nModified", test_bulkwrite_missing_nModified);
   TestSuite_AddMockServerTest(suite, "/bulkwrite/unexpected_results", test_bulkwrite_unexpected_results);
}
