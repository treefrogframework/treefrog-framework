#include <mongoc/mongoc.h>

#include "mongoc/mongoc-collection-private.h"

#include "json-test.h"
#include "test-libmongoc.h"
#include "mock_server/mock-rs.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "json-test-operations.h"


static void
transactions_test_run_operation (json_test_ctx_t *ctx,
                                 const bson_t *test,
                                 const bson_t *operation)
{
   mongoc_client_session_t *session = NULL;

   if (bson_has_field (operation, "arguments.session")) {
      session = session_from_name (
         ctx, bson_lookup_utf8 (operation, "arguments.session"));
   }

   /* expect some warnings from abortTransaction, but don't suppress others: we
    * want to know if any other tests log warnings */
   capture_logs (true);
   json_test_operation (ctx, test, operation, ctx->collection, session);
   assert_all_captured_logs_have_prefix ("Error in abortTransaction:");
   capture_logs (false);
}


static void
test_transactions_cb (bson_t *scenario)
{
   json_test_config_t config = JSON_TEST_CONFIG_INIT;
   config.run_operation_cb = transactions_test_run_operation;
   config.scenario = scenario;
   config.command_started_events_only = true;
   run_json_general_test (&config);
}


static void
test_transactions_supported (void *ctx)
{
   bool supported;
   mongoc_client_t *client;
   mongoc_client_session_t *session;
   mongoc_database_t *db;
   mongoc_collection_t *collection;
   bson_t *majority = tmp_bson ("{'writeConcern': {'w': 'majority'}}");
   bson_t opts = BSON_INITIALIZER;
   bson_error_t error;
   bool r;

   if (test_framework_is_mongos ()) {
      return;
   }

   supported = test_framework_max_wire_version_at_least (7) &&
               test_framework_is_replset ();
   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   db = mongoc_client_get_database (client, "transaction-tests");

   /* drop and create collection outside of transaction */
   mongoc_database_write_command_with_opts (
      db, tmp_bson ("{'drop': 'test'}"), majority, NULL, NULL);
   collection =
      mongoc_database_create_collection (db, "test", majority, &error);
   ASSERT_OR_PRINT (collection, error);

   session = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (session, error);

   /* Transactions Spec says "startTransaction SHOULD report an error if the
    * driver can detect that transactions are not supported by the deployment",
    * but we take advantage of the wiggle room and don't error here. */
   r = mongoc_client_session_start_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);

   r = mongoc_client_session_append (session, &opts, &error);
   ASSERT_OR_PRINT (r, error);
   r = mongoc_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);

   if (supported) {
      ASSERT_OR_PRINT (r, error);
   } else {
      BSON_ASSERT (!r);
      ASSERT_CMPINT32 (error.domain, ==, MONGOC_ERROR_SERVER);
      ASSERT_CONTAINS (error.message, "transaction");
   }

   bson_destroy (&opts);
   mongoc_collection_destroy (collection);

   if (!supported) {
      /* suppress "error in abortTransaction" warning from session_destroy */
      capture_logs (true);
   }

   mongoc_client_session_destroy (session);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}


static void
test_in_transaction (void *ctx)
{
   mongoc_client_t *client;
   mongoc_client_session_t *session;
   mongoc_database_t *db;
   mongoc_collection_t *collection;
   bson_t *majority = tmp_bson ("{'writeConcern': {'w': 'majority'}}");
   bson_t opts = BSON_INITIALIZER;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   db = mongoc_client_get_database (client, "transaction-tests");
   /* drop and create collection outside of transaction */
   mongoc_database_write_command_with_opts (
      db, tmp_bson ("{'drop': 'test'}"), majority, NULL, NULL);
   collection =
      mongoc_database_create_collection (db, "test", majority, &error);
   ASSERT_OR_PRINT (collection, error);

   session = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (session, error);
   r = mongoc_client_session_append (session, &opts, &error);
   ASSERT_OR_PRINT (r, error);
   BSON_ASSERT (!mongoc_client_session_in_transaction (session));

   /* commit an empty transaction */
   r = mongoc_client_session_start_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   BSON_ASSERT (mongoc_client_session_in_transaction (session));
   r = mongoc_client_session_commit_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   BSON_ASSERT (!mongoc_client_session_in_transaction (session));

   /* commit a transaction with an insert */
   r = mongoc_client_session_start_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   BSON_ASSERT (mongoc_client_session_in_transaction (session));
   r = mongoc_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   r = mongoc_client_session_commit_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   BSON_ASSERT (!mongoc_client_session_in_transaction (session));

   bson_destroy (&opts);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (db);
   mongoc_client_session_destroy (session);
   mongoc_client_destroy (client);
}


static bool
hangup_except_ismaster (request_t *request, void *data)
{
   if (!bson_strcasecmp (request->command_name, "ismaster")) {
      /* allow default response */
      return false;
   }

   mock_server_hangs_up (request);
   request_destroy (request);
   return true;
}


static void
_test_transient_txn_err (bool hangup)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_client_session_t *session;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   mongoc_bulk_operation_t *bulk;
   mongoc_find_and_modify_opts_t *fam;
   bson_t opts = BSON_INITIALIZER;
   bson_error_t error;
   bson_t *b;
   bson_t *u;
   const bson_t *doc_out;
   const bson_t *error_doc;
   bson_t reply;
   bool r;

   server = mock_server_new ();
   mock_server_run (server);
   rs_response_to_ismaster (
      server, 7, true /* primary */, false /* tags */, server, NULL);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   /* allow fast reconnect */
   client->topology->min_heartbeat_frequency_msec = 0;
   session = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (session, error);
   r = mongoc_client_session_start_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   r = mongoc_client_session_append (session, &opts, &error);
   ASSERT_OR_PRINT (r, error);
   collection = mongoc_client_get_collection (client, "db", "collection");

   if (hangup) {
      /* test that network errors have TransientTransactionError */
      mock_server_autoresponds (server, hangup_except_ismaster, NULL, NULL);
   } else {
      /* test server selection errors have TransientTransactionError */
      mock_server_destroy (server);
      server = NULL;
   }

   /* warnings when trying to abort the transaction and later, end sessions */
   capture_logs (true);

#define ASSERT_TRANSIENT_LABEL(_b, _expr)                                \
   do {                                                                  \
      if (!mongoc_error_has_label ((_b), "TransientTransactionError")) { \
         test_error ("Reply lacks TransientTransactionError label: %s\n" \
                     "Running %s",                                       \
                     bson_as_json ((_b), NULL),                          \
                     #_expr);                                            \
      }                                                                  \
   } while (0)

#define TEST_CMD_ERR(_expr)                   \
   do {                                       \
      r = (_expr);                            \
      BSON_ASSERT (!r);                       \
      ASSERT_TRANSIENT_LABEL (&reply, _expr); \
      bson_destroy (&reply);                  \
      /* clean slate for next test */         \
      memset (&reply, 0, sizeof (reply));     \
   } while (0)


#define TEST_WRITE_ERR(_expr)                 \
   do {                                       \
      r = (_expr);                            \
      ASSERT_TRANSIENT_LABEL (&reply, _expr); \
      bson_destroy (&reply);                  \
      /* clean slate for next test */         \
      memset (&reply, 0, sizeof (reply));     \
   } while (0)

#define TEST_CURSOR_ERR(_cursor_expr)                                 \
   do {                                                               \
      cursor = (_cursor_expr);                                        \
      r = mongoc_cursor_next (cursor, &doc_out);                      \
      BSON_ASSERT (!r);                                               \
      r = !mongoc_cursor_error_document (cursor, &error, &error_doc); \
      BSON_ASSERT (!r);                                               \
      BSON_ASSERT (error_doc);                                        \
      ASSERT_TRANSIENT_LABEL (error_doc, _cursor_expr);               \
      mongoc_cursor_destroy (cursor);                                 \
   } while (0)

   b = tmp_bson ("{'x': 1}");
   u = tmp_bson ("{'$inc': {'x': 1}}");

   TEST_CMD_ERR (mongoc_client_command_with_opts (
      client, "db", b, NULL, &opts, &reply, NULL));
   TEST_CMD_ERR (mongoc_client_read_command_with_opts (
      client, "db", b, NULL, &opts, &reply, NULL));
   TEST_CMD_ERR (mongoc_client_write_command_with_opts (
      client, "db", b, &opts, &reply, NULL));
   TEST_CMD_ERR (mongoc_client_read_write_command_with_opts (
      client, "db", b, NULL, &opts, &reply, NULL));
   TEST_CMD_ERR (0 < mongoc_collection_count_documents (
                        collection, b, &opts, NULL, &reply, NULL));

   BEGIN_IGNORE_DEPRECATIONS;
   TEST_CMD_ERR (mongoc_collection_create_index_with_opts (
      collection, b, NULL, &opts, &reply, NULL));
   END_IGNORE_DEPRECATIONS

   fam = mongoc_find_and_modify_opts_new ();
   mongoc_find_and_modify_opts_append (fam, &opts);
   TEST_CMD_ERR (mongoc_collection_find_and_modify_with_opts (
      collection, b, fam, &reply, NULL));

   TEST_WRITE_ERR (
      mongoc_collection_insert_one (collection, b, &opts, &reply, NULL));
   TEST_WRITE_ERR (mongoc_collection_insert_many (
      collection, (const bson_t **) &b, 1, &opts, &reply, NULL));
   TEST_WRITE_ERR (
      mongoc_collection_update_one (collection, b, u, &opts, &reply, NULL));
   TEST_WRITE_ERR (
      mongoc_collection_update_many (collection, b, u, &opts, &reply, NULL));
   TEST_WRITE_ERR (
      mongoc_collection_replace_one (collection, b, b, &opts, &reply, NULL));
   TEST_WRITE_ERR (
      mongoc_collection_delete_one (collection, b, &opts, &reply, NULL));
   TEST_WRITE_ERR (
      mongoc_collection_delete_many (collection, b, &opts, &reply, NULL));

   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, &opts);
   mongoc_bulk_operation_insert (bulk, b);
   TEST_WRITE_ERR (mongoc_bulk_operation_execute (bulk, &reply, NULL));

   TEST_CURSOR_ERR (mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson ("[{}]"), &opts, NULL));
   TEST_CURSOR_ERR (
      mongoc_collection_find_with_opts (collection, b, &opts, NULL));

   mongoc_find_and_modify_opts_destroy (fam);
   mongoc_bulk_operation_destroy (bulk);
   bson_destroy (&opts);
   mongoc_collection_destroy (collection);
   mongoc_client_session_destroy (session);
   mongoc_client_destroy (client);

   if (server) {
      mock_server_destroy (server);
   }
}


static void
test_server_selection_error (void)
{
   _test_transient_txn_err (false /* hangup */);
}


static void
test_network_error (void)
{
   _test_transient_txn_err (true /* hangup */);
}


/* Transactions Spec: Drivers add the "UnknownTransactionCommitResult" to a
 * server selection error from commitTransaction, even if this is the first
 * attempt to send commitTransaction. It is true in this case that the driver
 * knows the result: the transaction is definitely not committed. However, the
 * "UnknownTransactionCommitResult" label properly communicates to the
 * application that calling commitTransaction again may succeed.
 */
static void
test_unknown_commit_result (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_client_session_t *session;
   mongoc_collection_t *collection;
   future_t *future;
   request_t *request;
   bson_t opts = BSON_INITIALIZER;
   bson_error_t error;
   bson_t reply;
   bool r;

   server = mock_server_new ();
   mock_server_run (server);
   rs_response_to_ismaster (
      server, 7, true /* primary */, false /* tags */, server, NULL);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   /* allow fast reconnect */
   client->topology->min_heartbeat_frequency_msec = 0;
   session = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (session, error);
   r = mongoc_client_session_start_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   r = mongoc_client_session_append (session, &opts, &error);
   ASSERT_OR_PRINT (r, error);
   collection = mongoc_client_get_collection (client, "db", "collection");
   future = future_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);
   request = mock_server_receives_msg (
      server, 0, tmp_bson ("{'insert': 'collection'}"), tmp_bson ("{}"));
   mock_server_replies_ok_and_destroys (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);
   future_destroy (future);

   /* test server selection errors have UnknownTransactionCommitResult */
   mock_server_destroy (server);
   r = mongoc_client_session_commit_transaction (session, &reply, &error);
   BSON_ASSERT (!r);

   if (!mongoc_error_has_label (&reply, "UnknownTransactionCommitResult")) {
      test_error ("Reply lacks UnknownTransactionCommitResult label: %s",
                  bson_as_json (&reply, NULL));
   }

   if (mongoc_error_has_label (&reply, "TransientTransactionError")) {
      test_error ("Reply shouldn't have TransientTransactionError label: %s",
                  bson_as_json (&reply, NULL));
   }

   bson_destroy (&reply);
   bson_destroy (&opts);
   mongoc_collection_destroy (collection);

   /* warning when trying to end the session */
   capture_logs (true);
   mongoc_client_session_destroy (session);
   mongoc_client_destroy (client);
}


static void
test_cursor_primary_read_pref (void *ctx)
{
   mongoc_client_t *client;
   mongoc_client_session_t *session;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_t opts = BSON_INITIALIZER;
   mongoc_read_prefs_t *read_prefs;
   const bson_t *doc;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_cursor_primary_read_pref");

   session = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (session, error);

   r = mongoc_client_session_start_transaction (session, NULL, &error);
   ASSERT_OR_PRINT (r, error);

   r = mongoc_client_session_append (session, &opts, &error);
   ASSERT_OR_PRINT (r, error);

   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{}"), &opts, read_prefs);

   bson_destroy (&opts);
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_collection_destroy (collection);

   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   mongoc_cursor_destroy (cursor);
   mongoc_client_session_destroy (session);
   mongoc_client_destroy (client);
}


/* test the fix to CDRIVER-2815. */
void
test_inherit_from_client (void *ctx)
{
   mongoc_client_t *client;
   mongoc_client_session_t *session;
   bson_error_t error;
   mongoc_uri_t *uri;
   mongoc_read_concern_t *rc;
   const mongoc_read_concern_t *returned_rc;
   mongoc_read_prefs_t *rp;
   const mongoc_read_prefs_t *returned_rp;
   mongoc_write_concern_t *wc;
   const mongoc_write_concern_t *returned_wc;
   mongoc_session_opt_t *sopt;
   const mongoc_session_opt_t *returned_sopt;
   mongoc_transaction_opt_t *topt;
   const mongoc_transaction_opt_t *returned_topt;

   uri = test_framework_get_uri ();

   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   mongoc_uri_set_read_concern (uri, rc);

   rp = mongoc_read_prefs_new (MONGOC_READ_NEAREST);
   mongoc_uri_set_read_prefs_t (uri, rp);

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_uri_set_write_concern (uri, wc);

   client = mongoc_client_new_from_uri (uri);

   sopt = mongoc_session_opts_new ();
   topt = mongoc_transaction_opts_new ();

   mongoc_transaction_opts_set_read_concern (topt, rc);
   mongoc_transaction_opts_set_read_prefs (topt, rp);
   mongoc_transaction_opts_set_write_concern (topt, wc);

   mongoc_session_opts_set_default_transaction_opts (sopt, topt);

   session = mongoc_client_start_session (client, sopt, &error);
   ASSERT_OR_PRINT (session, error);

   /* test that unacknowledged write concern is actually used, since it should
    * result in an error. */
   ASSERT (!mongoc_client_session_start_transaction (session, NULL, &error));
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_TRANSACTION,
      MONGOC_ERROR_TRANSACTION_INVALID_STATE,
      "Transactions do not support unacknowledged write concern");

   returned_sopt = mongoc_client_session_get_opts (session);
   returned_topt =
      mongoc_session_opts_get_default_transaction_opts (returned_sopt);
   returned_rc = mongoc_transaction_opts_get_read_concern (returned_topt);
   returned_rp = mongoc_transaction_opts_get_read_prefs (returned_topt);
   returned_wc = mongoc_transaction_opts_get_write_concern (returned_topt);

   BSON_ASSERT (strcmp (mongoc_read_concern_get_level (returned_rc),
                        mongoc_read_concern_get_level (rc)) == 0);
   BSON_ASSERT (mongoc_write_concern_get_w (returned_wc) ==
                mongoc_write_concern_get_w (wc));
   BSON_ASSERT (mongoc_read_prefs_get_mode (returned_rp) ==
                mongoc_read_prefs_get_mode (rp));

   mongoc_read_concern_destroy (rc);
   mongoc_read_prefs_destroy (rp);
   mongoc_write_concern_destroy (wc);
   mongoc_transaction_opts_destroy (topt);
   mongoc_session_opts_destroy (sopt);
   mongoc_client_session_destroy (session);
   mongoc_uri_destroy (uri);
   mongoc_client_destroy (client);
}


void
test_transactions_install (TestSuite *suite)
{
   char resolved[PATH_MAX];

   ASSERT (realpath (JSON_DIR "/transactions", resolved));
   install_json_test_suite_with_check (
      suite, resolved, test_transactions_cb, test_framework_skip_if_no_txns);

   /* skip mongos for now - txn support coming in 4.1.0 */
   TestSuite_AddFull (suite,
                      "/transactions/supported",
                      test_transactions_supported,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_sessions,
                      test_framework_skip_if_no_crypto,
                      test_framework_skip_if_mongos);
   TestSuite_AddFull (suite,
                      "/transactions/in_transaction",
                      test_in_transaction,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_txns);
   TestSuite_AddMockServerTest (suite,
                                "/transactions/server_selection_err",
                                test_server_selection_error,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddMockServerTest (suite,
                                "/transactions/network_err",
                                test_network_error,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddMockServerTest (suite,
                                "/transactions/unknown_commit_result",
                                test_unknown_commit_result,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddFull (suite,
                      "/transactions/cursor_primary_read_pref",
                      test_cursor_primary_read_pref,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_txns);
   TestSuite_AddFull (suite,
                      "/transactions/inherit_from_client",
                      test_inherit_from_client,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_txns);
}
