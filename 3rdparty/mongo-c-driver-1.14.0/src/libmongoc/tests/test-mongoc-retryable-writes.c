#include <mongoc/mongoc.h>

#include "mongoc/mongoc-collection-private.h"

#include "json-test.h"
#include "test-libmongoc.h"
#include "mock_server/mock-rs.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "json-test-operations.h"


static void
retryable_writes_test_run_operation (json_test_ctx_t *ctx,
                                     const bson_t *test,
                                     const bson_t *operation)
{
   bool *explicit_session = (bool *) ctx->config->ctx;

   json_test_operation (ctx,
                        test,
                        operation,
                        ctx->collection,
                        *explicit_session ? ctx->sessions[0] : NULL);
}


/* Callback for JSON tests from Retryable Writes Spec */
static void
test_retryable_writes_cb (bson_t *scenario)
{
   bool explicit_session;
   json_test_config_t config = JSON_TEST_CONFIG_INIT;

   /* use the context pointer to send "explicit_session" to the callback */
   config.ctx = &explicit_session;
   config.run_operation_cb = retryable_writes_test_run_operation;
   config.scenario = scenario;
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

   rs = mock_rs_with_autoismaster (WIRE_VERSION_OP_MSG,
                                   true /* has primary */,
                                   2 /* secondaries */,
                                   0 /* arbiters */);

   mock_rs_run (rs);
   uri = mongoc_uri_copy (mock_rs_get_uri (rs));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = mongoc_client_new_from_uri (uri);
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

   /* insert receives "not master" from old primary, reselects and retries */
   future = future_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);

   request =
      mock_rs_receives_msg (rs, 0, tmp_bson ("{'insert': 'collection'}"), b);
   BSON_ASSERT (mock_rs_request_is_to_secondary (rs, request));
   mock_server_replies_simple (request, "{'ok': 0, 'errmsg': 'not master'}");
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


/* Test code paths for _mongoc_client_command_with_opts */
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

   client = mongoc_client_new_from_uri (uri);
   test_framework_set_ssl_opts (client);
   mongoc_uri_destroy (uri);

   /* clean up in case a previous test aborted */
   server_id = mongoc_topology_select_server_id (
      client->topology, MONGOC_SS_WRITE, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);
   deactivate_fail_points (client, server_id);

   collection = get_test_collection (client, "retryable_writes");

   if (!mongoc_collection_drop (collection, &error)) {
      if (strcmp (error.message, "ns not found")) {
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
   client = mongoc_client_new_from_uri (uri);
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
   client = mongoc_client_new_from_uri (uri);
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
   client = mongoc_client_new_from_uri (uri);
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
   client = mongoc_client_new_from_uri (uri);
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
   client = mongoc_client_new_from_uri (uri);
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
   mongoc_client_t *client;

   capture_logs (true);
   client = mongoc_client_new ("mongodb://localhost/?retryWrites=true");
   BSON_ASSERT (client);

   ASSERT_CAPTURED_LOG (
      "retryWrites=true",
      MONGOC_LOG_LEVEL_WARNING,
      "retryWrites not supported without an SSL crypto library");

   mongoc_client_destroy (client);
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
   char resolved[PATH_MAX];

   test_framework_resolve_path (JSON_DIR "/retryable_writes", resolved);
   install_json_test_suite_with_check (suite,
                                       resolved,
                                       test_retryable_writes_cb,
                                       test_framework_skip_if_no_crypto,
                                       test_framework_skip_if_not_replset);
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
}
