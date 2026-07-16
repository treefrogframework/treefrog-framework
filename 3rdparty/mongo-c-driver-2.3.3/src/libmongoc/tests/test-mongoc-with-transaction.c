#include <mongoc/mongoc-client-session-private.h>

#include <mongoc/mongoc.h>

#include <mlib/time_point.h>

#include <json-test.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

/* Note, the with_transaction spec tests are in test-mongoc-transactions.c,
 * since it shares the same test runner with the transactions test runner. */

static bool
with_transaction_fail_transient_txn(mongoc_client_session_t *session, void *ctx, bson_t **reply, bson_error_t *error)
{
   BSON_UNUSED(ctx);
   BSON_UNUSED(error);

   mlib_sleep_for(session->with_txn_timeout_ms, ms);

   *reply = bson_new();
   _mongoc_add_error_label(*reply, MONGOC_ERROR_LABEL_TRANSIENTTRANSACTIONERROR);

   return false;
}

static bool
with_transaction_do_nothing(mongoc_client_session_t *session, void *ctx, bson_t **reply, bson_error_t *error)
{
   BSON_UNUSED(session);
   BSON_UNUSED(ctx);
   BSON_UNUSED(reply);
   BSON_UNUSED(error);
   return true;
}

static void
test_with_transaction_timeout(void *ctx)
{
   mongoc_client_t *client;
   mongoc_client_session_t *session;
   bson_error_t error;
   bool res;

   BSON_UNUSED(ctx);

   client = test_framework_new_default_client();

   session = mongoc_client_start_session(client, NULL, &error);
   ASSERT_OR_PRINT(session, error);

   session->with_txn_timeout_ms = 10;

   /* Test Case 1: Test that if the callback returns an
      error with the TransientTransactionError label and
      we have exceeded the timeout, withTransaction fails. */
   res = mongoc_client_session_with_transaction(session, with_transaction_fail_transient_txn, NULL, NULL, NULL, &error);
   ASSERT(!res);

   /* Test Case 2: If committing returns an error with the
      UnknownTransactionCommitResult label and we have exceeded
      the timeout, withTransaction fails. */
   session->fail_commit_label = MONGOC_ERROR_LABEL_UNKNOWNTRANSACTIONCOMMITRESULT;
   res = mongoc_client_session_with_transaction(session, with_transaction_do_nothing, NULL, NULL, NULL, &error);
   ASSERT(!res);

   /* Test Case 3: If committing returns an error with the
      TransientTransactionError label and we have exceeded the
      timeout, withTransaction fails. */
   session->fail_commit_label = MONGOC_ERROR_LABEL_TRANSIENTTRANSACTIONERROR;
   res = mongoc_client_session_with_transaction(session, with_transaction_do_nothing, NULL, NULL, NULL, &error);
   ASSERT(!res);

   mongoc_client_session_destroy(session);
   mongoc_client_destroy(client);
}


static double
always_0_jitter_source_generate(mongoc_jitter_source_t *source)
{
   BSON_UNUSED(source);
   return 0.0;
}

static double
always_1_jitter_source_generate(mongoc_jitter_source_t *source)
{
   BSON_UNUSED(source);
   return 1.0;
}

static void
retry_backoff_set_fail_point(mongoc_client_t *client)
{
   bson_error_t error;
   ASSERT_OR_PRINT(mongoc_client_command_simple(client,
                                                "admin",
                                                tmp_bson(BSON_STR({
                                                   "configureFailPoint" : "failCommand",
                                                   "mode" : {"times" : 13},
                                                   "data" : {
                                                      "failCommands" : ["commitTransaction"],
                                                      "errorCode" : 251 // NoSuchTransaction
                                                   }
                                                })),
                                                NULL,
                                                NULL,
                                                &error),
                   error);
}

static bool
retry_backoff_with_transaction_cb(mongoc_client_session_t *session, void *ctx, bson_t **reply, bson_error_t *error)
{
   BSON_UNUSED(ctx);
   BSON_UNUSED(reply);

   mongoc_client_t *const client = mongoc_client_session_get_client(session);
   mongoc_collection_t *const coll = mongoc_client_get_collection(client, "db", "coll");

   bson_t *const opts = bson_new();
   ASSERT_OR_PRINT(mongoc_client_session_append(session, opts, error), (*error));

   const bool ret = mongoc_collection_insert_one(coll, tmp_bson("{}"), opts, NULL, error);
   ASSERT_OR_PRINT(ret, (*error));

   bson_destroy(opts);
   mongoc_collection_destroy(coll);

   return ret;
}

static void
test_with_transaction_retry_backoff_is_enforced_prose(void *ctx)
{
   BSON_UNUSED(ctx);

   // Step 1
   mongoc_client_t *const client = test_framework_new_default_client();

   mongoc_database_t *const db = mongoc_client_get_database(client, "db");

   // Step 2
   bson_error_t error;
   mongoc_collection_t *const coll = mongoc_database_create_collection(db, "coll", NULL, &error);

   mongoc_client_session_t *const no_backoff_session = mongoc_client_start_session(client, NULL, &error);
   ASSERT_OR_PRINT(no_backoff_session, error);

   // Step 3.1
   _mongoc_client_session_set_jitter_source(no_backoff_session,
                                            _mongoc_jitter_source_new(always_0_jitter_source_generate));

   // Step 3.2
   retry_backoff_set_fail_point(client);

   // Step 3.3 (see `retry_backoff_with_transaction_cb`)
   // Step 3.4
   const mlib_time_point no_backoff_start = mlib_now();
   ASSERT_OR_PRINT(mongoc_client_session_with_transaction(
                      no_backoff_session, retry_backoff_with_transaction_cb, NULL, NULL, NULL, &error),
                   error);
   const mlib_duration no_backoff_time = mlib_elapsed_since(no_backoff_start);

   mongoc_client_session_destroy(no_backoff_session);

   mongoc_client_session_t *const with_backoff_session = mongoc_client_start_session(client, NULL, &error);
   ASSERT_OR_PRINT(with_backoff_session, error);

   // Step 4.1
   _mongoc_client_session_set_jitter_source(with_backoff_session,
                                            _mongoc_jitter_source_new(always_1_jitter_source_generate));

   // Step 4.2
   retry_backoff_set_fail_point(client);

   // Step 4.3 (see `retry_backoff_with_transaction_cb`)
   // Step 4.4
   const mlib_time_point with_backoff_start = mlib_now();
   ASSERT_OR_PRINT(mongoc_client_session_with_transaction(
                      with_backoff_session, retry_backoff_with_transaction_cb, NULL, NULL, NULL, &error),
                   error);
   const mlib_duration with_backoff_time = mlib_elapsed_since(with_backoff_start);

   mongoc_client_session_destroy(with_backoff_session);

   // Step 5
   const mlib_duration diff = mlib_duration(with_backoff_time, minus, (no_backoff_time, plus, (1800, ms)));
   ASSERT_CMPINT64(imaxabs(mlib_microseconds_count(diff)), <, mlib_microseconds_count(mlib_duration(500, ms)));

   mongoc_collection_destroy(coll);
   mongoc_database_destroy(db);
   mongoc_client_destroy(client);
}

void
test_with_transaction_install(TestSuite *suite)
{
   TestSuite_AddFull(suite,
                     "/with_transaction/timeout_tests [lock:live-server]",
                     test_with_transaction_timeout,
                     NULL,
                     NULL,
                     test_framework_skip_if_no_sessions,
                     test_framework_skip_if_no_crypto);
   TestSuite_AddFull(suite,
                     "/with_transaction/retry_backoff_is_enforced_prose",
                     test_with_transaction_retry_backoff_is_enforced_prose,
                     NULL,
                     NULL,
                     test_framework_skip_if_no_txns);
}
