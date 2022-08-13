#include <mongoc/mongoc.h>

#include "mongoc/mongoc-collection-private.h"

#include "json-test.h"
#include "test-libmongoc.h"
#include "mock_server/mock-rs.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "json-test-operations.h"

static bool
retryable_reads_test_run_operation (json_test_ctx_t *ctx,
                                    const bson_t *test,
                                    const bson_t *operation)
{
   bool *explicit_session = (bool *) ctx->config->ctx;
   bson_t reply;
   bson_iter_t iter;
   const char *op_name;
   uint32_t op_len;
   bool res;

   bson_iter_init_find (&iter, operation, "name");
   op_name = bson_iter_utf8 (&iter, &op_len);
   if (strcmp (op_name, "estimatedDocumentCount") == 0 ||
       strcmp (op_name, "count") == 0) {
      /* CDRIVER-3612: mongoc_collection_estimated_document_count does not
       * support explicit sessions */
      *explicit_session = false;
   }
   res = json_test_operation (ctx,
                              test,
                              operation,
                              ctx->collection,
                              *explicit_session ? ctx->sessions[0] : NULL,
                              &reply);

   bson_destroy (&reply);

   return res;
}


/* Callback for JSON tests from Retryable Reads Spec */
static void
test_retryable_reads_cb (bson_t *scenario)
{
   bool explicit_session;
   json_test_config_t config = JSON_TEST_CONFIG_INIT;

   /* use the context pointer to send "explicit_session" to the callback */
   config.ctx = &explicit_session;
   config.run_operation_cb = retryable_reads_test_run_operation;
   config.scenario = scenario;
   config.command_started_events_only = true;
   explicit_session = true;
   run_json_general_test (&config);
   explicit_session = false;
   run_json_general_test (&config);
}


static void
_set_failpoint (mongoc_client_t *client)
{
   bson_error_t error;
   bson_t *cmd =
      tmp_bson ("{'configureFailPoint': 'failCommand',"
                " 'mode': {'times': 1},"
                " 'data': {'errorCode': 10107, 'failCommands': ['count']}}");

   ASSERT_OR_PRINT (
      mongoc_client_command_simple (client, "admin", cmd, NULL, NULL, &error),
      error);
}
/* Test code paths for all command helpers */
static void
test_cmd_helpers (void *ctx)
{
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   uint32_t server_id;
   mongoc_collection_t *collection;
   bson_t *cmd;
   bson_t reply;
   bson_error_t error;
   bson_iter_t iter;
   mongoc_cursor_t *cursor;
   mongoc_database_t *database;
   const bson_t *doc;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_bool (uri, "retryReads", true);

   client = test_framework_client_new_from_uri (uri, NULL);
   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);
   test_framework_set_ssl_opts (client);
   mongoc_uri_destroy (uri);

   /* clean up in case a previous test aborted */
   server_id = mongoc_topology_select_server_id (
      client->topology, MONGOC_SS_WRITE, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);
   deactivate_fail_points (client, server_id);

   collection = get_test_collection (client, "retryable_reads");
   database = mongoc_client_get_database (client, "test");

   if (!mongoc_collection_drop (collection, &error)) {
      if (NULL == strstr (error.message, "ns not found")) {
         /* an error besides ns not found */
         ASSERT_OR_PRINT (false, error);
      }
   }

   ASSERT_OR_PRINT (mongoc_collection_insert_one (
                       collection, tmp_bson ("{'_id': 0}"), NULL, NULL, &error),
                    error);
   ASSERT_OR_PRINT (mongoc_collection_insert_one (
                       collection, tmp_bson ("{'_id': 1}"), NULL, NULL, &error),
                    error);

   cmd = tmp_bson ("{'count': '%s'}", collection->collection);

   /* read helpers must retry. */
   _set_failpoint (client);
   ASSERT_OR_PRINT (mongoc_client_read_command_with_opts (
                       client, "test", cmd, NULL, NULL, &reply, &error),
                    error);
   bson_iter_init_find (&iter, &reply, "n");
   ASSERT (bson_iter_as_int64 (&iter) == 2);
   bson_destroy (&reply);

   _set_failpoint (client);
   ASSERT_OR_PRINT (mongoc_database_read_command_with_opts (
                       database, cmd, NULL, NULL, &reply, &error),
                    error);
   bson_iter_init_find (&iter, &reply, "n");
   ASSERT (bson_iter_as_int64 (&iter) == 2);
   bson_destroy (&reply);

   _set_failpoint (client);
   ASSERT_OR_PRINT (mongoc_collection_read_command_with_opts (
                       collection, cmd, NULL, NULL, &reply, &error),
                    error);
   bson_iter_init_find (&iter, &reply, "n");
   ASSERT (bson_iter_as_int64 (&iter) == 2);
   bson_destroy (&reply);

   /* TODO: once CDRIVER-3314 is resolved, test the read+write helpers. */

   /* read/write agnostic command_simple helpers must not retry. */
   _set_failpoint (client);
   ASSERT (
      !mongoc_client_command_simple (client, "test", cmd, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint (client);
   ASSERT (!mongoc_database_command_simple (database, cmd, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint (client);
   ASSERT (
      !mongoc_collection_command_simple (collection, cmd, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");


   /* read/write agnostic command_with_opts helpers must not retry. */
   _set_failpoint (client);
   ASSERT (!mongoc_client_command_with_opts (
      client, "test", cmd, NULL, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint (client);
   ASSERT (!mongoc_database_command_with_opts (
      database, cmd, NULL, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint (client);
   ASSERT (!mongoc_collection_command_with_opts (
      collection, cmd, NULL, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   /* read/write agnostic command_simple_with_server_id helper must not retry.
    */
   server_id = mongoc_topology_select_server_id (
      client->topology, MONGOC_SS_WRITE, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);
   _set_failpoint (client);
   ASSERT (!mongoc_client_command_simple_with_server_id (
      client, "test", cmd, NULL, server_id, NULL, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");


   /* deprecated command helpers (which goes through cursor logic) function must
    * not retry. */
   _set_failpoint (client);
   cursor = mongoc_client_command (
      client, "test", MONGOC_QUERY_NONE, 0, 1, 1, cmd, NULL, NULL);
   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");
   mongoc_cursor_destroy (cursor);

   _set_failpoint (client);
   cursor = mongoc_database_command (
      database, MONGOC_QUERY_NONE, 0, 1, 1, cmd, NULL, NULL);
   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");
   mongoc_cursor_destroy (cursor);

   _set_failpoint (client);
   cursor = mongoc_collection_command (
      collection, MONGOC_QUERY_NONE, 0, 1, 1, cmd, NULL, NULL);
   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 10107, "Failing command");
   mongoc_cursor_destroy (cursor);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   deactivate_fail_points (client, server_id);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_retry_reads_off (void *ctx)
{
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   uint32_t server_id;
   bson_t *cmd;
   bson_error_t error;
   bool res;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_bool (uri, "retryreads", false);
   client = test_framework_client_new_from_uri (uri, NULL);
   test_framework_set_ssl_opts (client);

   /* clean up in case a previous test aborted */
   server_id = mongoc_topology_select_server_id (
      client->topology, MONGOC_SS_WRITE, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);
   deactivate_fail_points (client, server_id);

   collection = get_test_collection (client, "retryable_reads");

   cmd = tmp_bson ("{'configureFailPoint': 'failCommand',"
                   " 'mode': {'times': 1},"
                   " 'data': {'errorCode': 10107, 'failCommands': ['count']}}");
   ASSERT_OR_PRINT (mongoc_client_command_simple_with_server_id (
                       client, "admin", cmd, NULL, server_id, NULL, &error),
                    error);

   cmd = tmp_bson ("{'count': 'coll'}", collection->collection);

   res = mongoc_collection_read_command_with_opts (
      collection, cmd, NULL, NULL, NULL, &error);
   ASSERT (!res);
   ASSERT_CONTAINS (error.message, "failpoint");

   deactivate_fail_points (client, server_id);

   mongoc_collection_destroy (collection);
   mongoc_uri_destroy (uri);
   mongoc_client_destroy (client);
}

/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for retryable reads.
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests (TestSuite *suite)
{
   install_json_test_suite_with_check (suite,
                                       JSON_DIR,
                                       "retryable_reads",
                                       test_retryable_reads_cb,
                                       TestSuite_CheckLive,
                                       test_framework_skip_if_no_failpoint,
                                       test_framework_skip_if_slow);
}

void
test_retryable_reads_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
   /* Since we need failpoints, require wire version 7 */
   TestSuite_AddFull (suite,
                      "/retryable_reads/cmd_helpers",
                      test_cmd_helpers,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_7,
                      test_framework_skip_if_mongos,
                      test_framework_skip_if_no_failpoint);
   TestSuite_AddFull (suite,
                      "/retryable_reads/retry_off",
                      test_retry_reads_off,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_7,
                      test_framework_skip_if_mongos,
                      test_framework_skip_if_no_failpoint);
}
