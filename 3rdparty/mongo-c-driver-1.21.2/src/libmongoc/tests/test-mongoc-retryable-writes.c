#include <mongoc/mongoc.h>

#include "mongoc/mongoc-collection-private.h"

#include "json-test.h"
#include "test-libmongoc.h"
#include "mock_server/mock-rs.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "json-test-operations.h"


static bool
retryable_writes_test_run_operation (json_test_ctx_t *ctx,
                                     const bson_t *test,
                                     const bson_t *operation)
{
   bool *explicit_session = (bool *) ctx->config->ctx;
   bson_t reply;
   bool res;

   res = json_test_operation (ctx,
                              test,
                              operation,
                              ctx->collection,
                              *explicit_session ? ctx->sessions[0] : NULL,
                              &reply);

   bson_destroy (&reply);

   return res;
}


static test_skip_t skips[] = {
   {"InsertOne fails after multiple retryable writeConcernErrors",
    "Waiting on CDRIVER-3790"},
   {0}};

/* Callback for JSON tests from Retryable Writes Spec */
static void
test_retryable_writes_cb (bson_t *scenario)
{
   bool explicit_session;
   json_test_config_t config = JSON_TEST_CONFIG_INIT;

   config.skips = skips;

   /* use the context pointer to send "explicit_session" to the callback */
   config.ctx = &explicit_session;
   config.run_operation_cb = retryable_writes_test_run_operation;
   config.scenario = scenario;
   config.command_started_events_only = true;
   explicit_session = true;
   run_json_general_test (&config);
   explicit_session = false;
   run_json_general_test (&config);
}


/* "Replica Set Failover Test" from Retryable Writes Spec */
static void
test_rs_failover (void)
{
   mock_rs_t *rs;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_client_session_t *cs;
   bson_t opts = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   bson_error_t error;
   bson_t *b = tmp_bson ("{}");

   rs = mock_rs_with_auto_hello (WIRE_VERSION_OP_MSG,
                                 true /* has primary */,
                                 2 /* secondaries */,
                                 0 /* arbiters */);

   mock_rs_run (rs);
   uri = mongoc_uri_copy (mock_rs_get_uri (rs));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = test_framework_client_new_from_uri (uri, NULL);
   collection = mongoc_client_get_collection (client, "db", "collection");
   cs = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (cs, error);
   ASSERT_OR_PRINT (mongoc_client_session_append (cs, &opts, &error), error);

   /* initial insert triggers replica set discovery */
   future = future_collection_insert_one (collection, b, &opts, NULL, &error);
   request =
      mock_rs_receives_msg (rs, 0, tmp_bson ("{'insert': 'collection'}"), b);
   mock_server_replies_ok_and_destroys (request);
   BSON_ASSERT (future_get_bool (future));
   future_destroy (future);

   /* failover */
   mock_rs_stepdown (rs);
   mock_rs_elect (rs, 1 /* server id */);

   /* insert receives "not primary" from old primary, reselects and retries */
   future = future_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);

   request =
      mock_rs_receives_msg (rs, 0, tmp_bson ("{'insert': 'collection'}"), b);
   BSON_ASSERT (mock_rs_request_is_to_secondary (rs, request));
   mock_server_replies_simple (
      request, "{'ok': 0, 'code': 10107, 'errmsg': 'not primary'}");
   request_destroy (request);

   request =
      mock_rs_receives_msg (rs, 0, tmp_bson ("{'insert': 'collection'}"), b);
   BSON_ASSERT (mock_rs_request_is_to_primary (rs, request));
   mock_server_replies_ok_and_destroys (request);
   BSON_ASSERT (future_get_bool (future));
   future_destroy (future);

   bson_destroy (&opts);
   mongoc_client_session_destroy (cs);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_rs_destroy (rs);
}


/* Test code paths for _mongoc_client_command_with_opts.
 * This test requires a 3.6+ replica set to support the
 * onPrimaryTransactionalWrite failpoint. */
static void
test_command_with_opts (void *ctx)
{
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   uint32_t server_id;
   mongoc_collection_t *collection;
   bson_t *cmd;
   bson_t reply;
   bson_t reply_result;
   bson_error_t error;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);

   client = test_framework_client_new_from_uri (uri, NULL);
   test_framework_set_ssl_opts (client);
   mongoc_uri_destroy (uri);

   /* clean up in case a previous test aborted */
   server_id = mongoc_topology_select_server_id (
      client->topology, MONGOC_SS_WRITE, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);
   deactivate_fail_points (client, server_id);

   collection = get_test_collection (client, "retryable_writes");

   if (!mongoc_collection_drop (collection, &error)) {
      if (NULL == strstr (error.message, "ns not found")) {
         /* an error besides ns not found */
         ASSERT_OR_PRINT (false, error);
      }
   }

   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (
         collection, tmp_bson ("{'_id':1, 'x': 1}"), NULL, NULL, &error),
      error);

   cmd = tmp_bson ("{'configureFailPoint': 'onPrimaryTransactionalWrite',"
                   " 'mode': {'times': 1},"
                   " 'data': {'failBeforeCommitExceptionCode': 1}}");

   ASSERT_OR_PRINT (mongoc_client_command_simple_with_server_id (
                       client, "admin", cmd, NULL, server_id, NULL, &error),
                    error);

   cmd = tmp_bson ("{'findAndModify': '%s', 'query': {'_id': 1}, 'update': "
                   "{'$inc': {'x': 1}}, 'new': true}",
                   collection->collection);

   ASSERT_OR_PRINT (mongoc_collection_read_write_command_with_opts (
                       collection, cmd, NULL, NULL, &reply, &error),
                    error);

   bson_lookup_doc (&reply, "value", &reply_result);
   ASSERT (match_bson (&reply_result, tmp_bson ("{'_id': 1, 'x': 2}"), false));

   deactivate_fail_points (client, server_id);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&reply_result);
   bson_destroy (&reply);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_insert_one_unacknowledged (void)
{
   mongoc_uri_t *uri;
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_write_concern_t *wc;
   bson_t opts = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_mongos_new (WIRE_VERSION_RETRY_WRITES);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = test_framework_client_new_from_uri (uri, NULL);
   collection = mongoc_client_get_collection (client, "db", "collection");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_write_concern_set_journal (wc, false);
   mongoc_write_concern_append (wc, &opts);

   future = future_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);

   request = mock_server_receives_msg (
      server,
      2, /* set moreToCome bit in mongoc_op_msg_flags_t */
      tmp_bson (
         "{'txnNumber': {'$exists': false}, 'lsid': {'$exists': false}}"),
      tmp_bson ("{}"));
   ASSERT (future_get_bool (future));
   mock_server_auto_endsessions (server);

   mongoc_write_concern_destroy (wc);
   mongoc_uri_destroy (uri);
   request_destroy (request);
   bson_destroy (&opts);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_update_one_unacknowledged (void)
{
   mongoc_uri_t *uri;
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_write_concern_t *wc;
   bson_t opts = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_mongos_new (WIRE_VERSION_RETRY_WRITES);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = test_framework_client_new_from_uri (uri, NULL);
   collection = mongoc_client_get_collection (client, "db", "collection");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_write_concern_set_journal (wc, false);
   mongoc_write_concern_append (wc, &opts);

   future = future_collection_update_one (collection,
                                          tmp_bson ("{}"),
                                          tmp_bson ("{'$set': {'x': 1}}"),
                                          &opts,
                                          NULL,
                                          &error);

   request = mock_server_receives_msg (
      server,
      2, /* set moreToCome bit in mongoc_op_msg_flags_t */
      tmp_bson (
         "{'txnNumber': {'$exists': false}, 'lsid': {'$exists': false}}"),
      tmp_bson ("{'q': {}, 'u': {'$set': {'x': 1}}}"));
   ASSERT (future_get_bool (future));

   mongoc_write_concern_destroy (wc);
   mongoc_uri_destroy (uri);
   request_destroy (request);
   bson_destroy (&opts);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_delete_one_unacknowledged (void)
{
   mongoc_uri_t *uri;
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_write_concern_t *wc;
   bson_t opts = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_mongos_new (WIRE_VERSION_RETRY_WRITES);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = test_framework_client_new_from_uri (uri, NULL);
   collection = mongoc_client_get_collection (client, "db", "collection");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_write_concern_set_journal (wc, false);
   mongoc_write_concern_append (wc, &opts);

   future = future_collection_delete_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);

   request = mock_server_receives_msg (
      server,
      2, /* set moreToCome bit in mongoc_op_msg_flags_t */
      tmp_bson (
         "{'txnNumber': {'$exists': false}, 'lsid': {'$exists': false}}"),
      tmp_bson ("{'q': {}, 'limit': 1}"));
   ASSERT (future_get_bool (future));

   mongoc_write_concern_destroy (wc);
   mongoc_uri_destroy (uri);
   request_destroy (request);
   bson_destroy (&opts);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_bulk_operation_execute_unacknowledged (void)
{
   mongoc_uri_t *uri;
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_write_concern_t *wc;
   mongoc_bulk_operation_t *bulk;
   bson_t opts = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_mongos_new (WIRE_VERSION_RETRY_WRITES);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = test_framework_client_new_from_uri (uri, NULL);
   collection = mongoc_client_get_collection (client, "db", "collection");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_write_concern_set_journal (wc, false);
   mongoc_write_concern_append (wc, &opts);

   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, &opts);
   mongoc_bulk_operation_insert (bulk, tmp_bson ("{'_id': 1}"));
   future = future_bulk_operation_execute (bulk, NULL, &error);

   request = mock_server_receives_msg (
      server,
      2, /* set moreToCome bit in mongoc_op_msg_flags_t */
      tmp_bson (
         "{'txnNumber': {'$exists': false}, 'lsid': {'$exists': false}}"),
      tmp_bson ("{'_id': 1}"));
   ASSERT (future_get_uint32_t (future) == 1);

   mongoc_write_concern_destroy (wc);
   mongoc_uri_destroy (uri);
   mongoc_bulk_operation_destroy (bulk);
   request_destroy (request);
   bson_destroy (&opts);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_remove_unacknowledged (void)
{
   mongoc_uri_t *uri;
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_write_concern_t *wc;
   bson_t opts = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_mongos_new (WIRE_VERSION_RETRY_WRITES);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = test_framework_client_new_from_uri (uri, NULL);
   collection = mongoc_client_get_collection (client, "db", "collection");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_write_concern_set_journal (wc, false);

   future = future_collection_remove (
      collection, MONGOC_REMOVE_NONE, tmp_bson ("{'a': 1}"), wc, &error);

   request = mock_server_receives_msg (
      server,
      2, /* set moreToCome bit in mongoc_op_msg_flags_t */
      tmp_bson (
         "{'txnNumber': {'$exists': false}, 'lsid': {'$exists': false}}"),
      tmp_bson ("{'q': {'a': 1}, 'limit': 0}"));
   ASSERT (future_get_bool (future));

   mongoc_write_concern_destroy (wc);
   mongoc_uri_destroy (uri);
   request_destroy (request);
   bson_destroy (&opts);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_retry_no_crypto (void *ctx)
{
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;

   capture_logs (true);

   /* Test that no warning is logged if retryWrites is disabled. Warning logic
    * is implemented in mongoc_topology_new, but test all public APIs that use
    * the common code path. */
   client = test_framework_client_new ("mongodb://localhost/?retryWrites=false",
                                       NULL);
   BSON_ASSERT (client);
   ASSERT_NO_CAPTURED_LOGS ("test_framework_client_new and retryWrites=false");
   mongoc_client_destroy (client);

   uri = mongoc_uri_new ("mongodb://localhost/?retryWrites=false");
   BSON_ASSERT (uri);

   client = test_framework_client_new_from_uri (uri, NULL);
   BSON_ASSERT (client);
   ASSERT_NO_CAPTURED_LOGS (
      "test_framework_client_new_from_uri and retryWrites=false");
   mongoc_client_destroy (client);

   pool = test_framework_client_pool_new_from_uri (uri, NULL);
   BSON_ASSERT (pool);
   ASSERT_NO_CAPTURED_LOGS (
      "test_framework_client_pool_new_from_uri and retryWrites=false");
   mongoc_client_pool_destroy (pool);

   mongoc_uri_destroy (uri);

   /* Test that a warning is logged if retryWrites is enabled. */
   client =
      test_framework_client_new ("mongodb://localhost/?retryWrites=true", NULL);
   BSON_ASSERT (client);
   ASSERT_CAPTURED_LOG (
      "test_framework_client_new and retryWrites=true",
      MONGOC_LOG_LEVEL_WARNING,
      "retryWrites not supported without an SSL crypto library");
   mongoc_client_destroy (client);

   clear_captured_logs ();

   uri = mongoc_uri_new ("mongodb://localhost/?retryWrites=true");
   BSON_ASSERT (uri);

   client = test_framework_client_new_from_uri (uri, NULL);
   BSON_ASSERT (client);
   ASSERT_CAPTURED_LOG (
      "test_framework_client_new_from_uri and retryWrites=true",
      MONGOC_LOG_LEVEL_WARNING,
      "retryWrites not supported without an SSL crypto library");
   mongoc_client_destroy (client);

   clear_captured_logs ();

   pool = test_framework_client_pool_new_from_uri (uri, NULL);
   BSON_ASSERT (pool);
   ASSERT_CAPTURED_LOG (
      "test_framework_client_pool_new_from_uri and retryWrites=true",
      MONGOC_LOG_LEVEL_WARNING,
      "retryWrites not supported without an SSL crypto library");
   mongoc_client_pool_destroy (pool);

   mongoc_uri_destroy (uri);
}

static void
test_unsupported_storage_engine_error (void)
{
   mock_rs_t *rs;
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   bson_t reply;
   bson_error_t error;
   future_t *future;
   request_t *request;
   mongoc_client_session_t *session;
   bson_t opts;
   const char *expected_msg = "This MongoDB deployment does not support "
                              "retryable writes. Please add retryWrites=false "
                              "to your connection string.";

   rs = mock_rs_with_auto_hello (WIRE_VERSION_RETRY_WRITES, true, 0, 0);
   mock_rs_run (rs);
   client = test_framework_client_new_from_uri (mock_rs_get_uri (rs), NULL);
   session = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (session, error);
   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);
   coll = mongoc_client_get_collection (client, "test", "test");
   bson_init (&opts);
   ASSERT_OR_PRINT (mongoc_client_session_append (session, &opts, &error),
                    error);
   /* findandmodify is retryable through mongoc_client_write_command_with_opts.
    */
   future = future_client_write_command_with_opts (
      client,
      "test",
      tmp_bson ("{'findandmodify': 'coll' }"),
      &opts,
      &reply,
      &error);
   request = mock_rs_receives_request (rs);
   mock_server_replies_simple (
      request,
      "{'ok': 0, 'code': 20, 'errmsg': 'Transaction numbers are great'}");
   request_destroy (request);

   BSON_ASSERT (!future_get_bool (future));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 20, expected_msg);
   ASSERT_MATCH (&reply, "{'code': 20, 'errmsg': '%s'}", expected_msg);

   bson_destroy (&opts);
   mongoc_client_session_destroy (session);
   bson_destroy (&reply);
   future_destroy (future);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   mock_rs_destroy (rs);
}
/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for retryable writes.
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests (TestSuite *suite)
{
   install_json_test_suite_with_check (suite,
                                       JSON_DIR,
                                       "retryable_writes",
                                       test_retryable_writes_cb,
                                       test_framework_skip_if_no_crypto,
                                       test_framework_skip_if_slow);
}


typedef struct {
   int num_inserts;
   int num_updates;
} _tracks_new_server_counters_t;

static void
_tracks_new_server_cb (const mongoc_apm_command_started_t *event)
{
   const char *cmd_name;
   _tracks_new_server_counters_t *counters;

   cmd_name = mongoc_apm_command_started_get_command_name (event);
   counters =
      (_tracks_new_server_counters_t *) mongoc_apm_command_started_get_context (
         event);

   if (0 == strcmp (cmd_name, "insert")) {
      counters->num_inserts++;
   } else if (0 == strcmp (cmd_name, "update")) {
      counters->num_updates++;
   }
}

/* Tests that when a command within a bulk write succeeds after a retryable
 * error, and selects a new server, it continues to use that server in
 * subsequent commands.
 * This test requires running against a replica set with at least one secondary.
 */
static void
test_bulk_retry_tracks_new_server (void *unused)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_error_t error;
   mongoc_read_prefs_t *read_prefs;
   bool ret;
   mongoc_server_description_t *sd;
   mongoc_apm_callbacks_t *callbacks;
   _tracks_new_server_counters_t counters = {0};

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, _tracks_new_server_cb);

   client = test_framework_new_default_client ();
   mongoc_client_set_apm_callbacks (client, callbacks, &counters);
   collection = get_test_collection (client, "tracks_new_server");
   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, NULL);

   /* The bulk write contains two operations, an insert, followed by an update.
    */
   ret = mongoc_bulk_operation_insert_with_opts (
      bulk, tmp_bson ("{'x': 1}"), NULL /* opts */, &error);
   ASSERT_OR_PRINT (ret, error);
   mongoc_bulk_operation_update_one (bulk,
                                     tmp_bson ("{}"),
                                     tmp_bson ("{'$inc': {'x': 1}}"),
                                     false /* upsert */);

   /* Explicitly tell the bulk write to use a secondary. That will result in a
    * retryable error, causing the first command to be sent twice. */
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   sd = mongoc_client_select_server (
      client, false /* for_writes */, read_prefs, &error);
   ASSERT_OR_PRINT (sd, error);
   mongoc_bulk_operation_set_hint (bulk, mongoc_server_description_id (sd));
   ret = mongoc_bulk_operation_execute (bulk, NULL /* reply */, &error);
   ASSERT_OR_PRINT (ret, error);

   /* The first insert fails with a retryable write error since it is sent to a
    * secondary. The retry selects the primary and succeeds. The second command
    * should use the newly selected server, so the update succeeds on the first
    * try. */
   ASSERT_CMPINT (counters.num_inserts, ==, 2);
   ASSERT_CMPINT (counters.num_updates, ==, 1);
   ASSERT_CMPINT (mongoc_bulk_operation_get_hint (bulk),
                  !=,
                  mongoc_server_description_id (sd));

   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

void
test_retryable_writes_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
   TestSuite_AddMockServerTest (suite,
                                "/retryable_writes/failover",
                                test_rs_failover,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddFull (suite,
                      "/retryable_writes/command_with_opts",
                      test_command_with_opts,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);
   TestSuite_AddMockServerTest (suite,
                                "/retryable_writes/insert_one_unacknowledged",
                                test_insert_one_unacknowledged,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddMockServerTest (suite,
                                "/retryable_writes/update_one_unacknowledged",
                                test_update_one_unacknowledged,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddMockServerTest (suite,
                                "/retryable_writes/delete_one_unacknowledged",
                                test_delete_one_unacknowledged,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddMockServerTest (suite,
                                "/retryable_writes/remove_unacknowledged",
                                test_remove_unacknowledged,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddMockServerTest (
      suite,
      "/retryable_writes/bulk_operation_execute_unacknowledged",
      test_bulk_operation_execute_unacknowledged,
      test_framework_skip_if_no_crypto);
   TestSuite_AddFull (suite,
                      "/retryable_writes/no_crypto",
                      test_retry_no_crypto,
                      NULL,
                      NULL,
                      test_framework_skip_if_crypto);
   TestSuite_AddMockServerTest (
      suite,
      "/retryable_writes/unsupported_storage_engine_error",
      test_unsupported_storage_engine_error,
      test_framework_skip_if_no_crypto);
   TestSuite_AddFull (suite,
                      "/retryable_writes/bulk_tracks_new_server",
                      test_bulk_retry_tracks_new_server,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_not_rs_version_6,
                      test_framework_skip_if_no_crypto);
}
