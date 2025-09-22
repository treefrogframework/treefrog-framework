#include <mongoc/mongoc-bulkwrite.h>
#include <mongoc/mongoc.h>

#include <mlib/cmp.h>
#include <mlib/loop.h>

#include <json-test-operations.h>
#include <json-test.h>
#include <test-libmongoc.h>

static bool
crud_test_operation_cb (json_test_ctx_t *ctx, const bson_t *test, const bson_t *operation)
{
   bson_t reply;
   bool res;

   res = json_test_operation (ctx, test, operation, ctx->collection, NULL, &reply);

   bson_destroy (&reply);

   return res;
}

static void
test_crud_cb (void *scenario)
{
   json_test_config_t config = JSON_TEST_CONFIG_INIT;
   config.run_operation_cb = crud_test_operation_cb;
   config.command_started_events_only = true;
   config.scenario = scenario;
   run_json_general_test (&config);
}

static void
test_all_spec_tests (TestSuite *suite)
{
   install_json_test_suite_with_check (
      suite, JSON_DIR, "crud/legacy", &test_crud_cb, test_framework_skip_if_no_crypto, TestSuite_CheckLive);

   /* Read/write concern spec tests use the same format. */
   install_json_test_suite_with_check (
      suite, JSON_DIR, "read_write_concern/operation", &test_crud_cb, TestSuite_CheckLive);
}

static void
prose_test_1 (void *ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   bool ret;
   bson_t reply;
   bson_error_t error;

   BSON_UNUSED (ctx);

   client = test_framework_new_default_client ();
   coll = get_test_collection (client, "coll");

   ret = mongoc_client_command_simple (client,
                                       "admin",
                                       tmp_bson ("{'configureFailPoint': 'failCommand', 'mode': {'times': 1}, "
                                                 " 'data': {'failCommands': ['insert'], 'writeConcernError': {"
                                                 "   'code': 100, 'codeName': 'UnsatisfiableWriteConcern', "
                                                 "   'errmsg': 'Not enough data-bearing nodes', "
                                                 "   'errInfo': {'writeConcern': {'w': 2, 'wtimeout': 0, "
                                                 "               'provenance': 'clientSupplied'}}}}}"),
                                       NULL,
                                       NULL,
                                       &error);
   ASSERT_OR_PRINT (ret, error);

   ret = mongoc_collection_insert_one (coll, tmp_bson ("{'x':1}"), NULL /* opts */, &reply, &error);
   ASSERT (!ret);

   /* libmongoc does not model WriteConcernError, so we only assert that the
    * "errInfo" field set in configureFailPoint matches that in the result */
   ASSERT_MATCH (&reply,
                 "{'writeConcernErrors': [{'errInfo': {'writeConcern': {"
                 "'w': 2, 'wtimeout': 0, 'provenance': 'clientSupplied'}}}]}");

   bson_destroy (&reply);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}

typedef struct {
   bool has_reply;
   bson_t reply;
} prose_test_2_apm_ctx_t;

static void
prose_test_2_command_succeeded (const mongoc_apm_command_succeeded_t *event)
{
   if (!strcmp (mongoc_apm_command_succeeded_get_command_name (event), "insert")) {
      prose_test_2_apm_ctx_t *ctx = mongoc_apm_command_succeeded_get_context (event);
      ASSERT (!ctx->has_reply);
      ctx->has_reply = true;
      bson_copy_to (mongoc_apm_command_succeeded_get_reply (event), &ctx->reply);
   }
}

static void
prose_test_2 (void *ctx)
{
   mongoc_client_t *client;
   mongoc_database_t *db;
   mongoc_collection_t *coll, *coll_created;
   mongoc_apm_callbacks_t *callbacks;
   prose_test_2_apm_ctx_t apm_ctx = {0};
   bool ret;
   bson_t reply, reply_errInfo, observed_errInfo;
   bson_error_t error = {0};

   BSON_UNUSED (ctx);

   client = test_framework_new_default_client ();
   db = get_test_database (client);
   coll = get_test_collection (client, "coll");

   /* don't care if ns not found. */
   (void) mongoc_collection_drop (coll, NULL);

   coll_created = mongoc_database_create_collection (
      db, mongoc_collection_get_name (coll), tmp_bson ("{'validator': {'x': {'$type': 'string'}}}"), &error);
   ASSERT_OR_PRINT (coll_created, error);
   mongoc_collection_destroy (coll_created);

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_succeeded_cb (callbacks, prose_test_2_command_succeeded);
   mongoc_client_set_apm_callbacks (client, callbacks, (void *) &apm_ctx);
   mongoc_apm_callbacks_destroy (callbacks);

   ret = mongoc_collection_insert_one (coll, tmp_bson ("{'x':1}"), NULL /* opts */, &reply, &error);
   ASSERT (!ret);

   /* Assert that the WriteError's code is DocumentValidationFailure */
   ASSERT_MATCH (&reply, "{'writeErrors': [{'code': 121}]}");

   /* libmongoc does not model WriteError, so we only assert that the observed
    * "errInfo" field matches that in the result */
   ASSERT (apm_ctx.has_reply);
   bson_lookup_doc (&apm_ctx.reply, "writeErrors.0.errInfo", &observed_errInfo);
   bson_lookup_doc (&reply, "writeErrors.0.errInfo", &reply_errInfo);
   ASSERT (bson_compare (&reply_errInfo, &observed_errInfo) == 0);

   bson_destroy (&apm_ctx.reply);
   bson_destroy (&reply);
   mongoc_collection_destroy (coll);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}

typedef struct {
   // `ops_counts` is a BSON document of this form:
   // { "0": <int64>, "1": <int64> ... }
   bson_t ops_counts;
   // `operation_ids` is a BSON document of this form:
   // { "0": <int64>, "1":  <int64> ... }
   bson_t operation_ids;
   // `write_concerns` is a BSON document of this form:
   // { "0": <document|null>, "1": <document|null> ... }
   bson_t write_concerns;
   int numGetMore;
   int numKillCursors;
} bulkWrite_ctx;

// `bulkWrite_cb` records the number of `ops` in each sent `bulkWrite` to a BSON
// document of this form:
// { "0": <int64>, "1": <int64> ... }
static void
bulkWrite_cb (const mongoc_apm_command_started_t *event)
{
   bulkWrite_ctx *ctx = mongoc_apm_command_started_get_context (event);
   const char *cmd_name = mongoc_apm_command_started_get_command_name (event);

   if (0 == strcmp (cmd_name, "bulkWrite")) {
      const bson_t *cmd = mongoc_apm_command_started_get_command (event);
      bson_iter_t ops_iter;
      // Count the number of `ops`.
      ASSERT (bson_iter_init_find (&ops_iter, cmd, "ops"));
      bson_t ops;
      bson_iter_bson (&ops_iter, &ops);
      uint32_t ops_count = bson_count_keys (&ops);
      // Record.
      char *key = bson_strdup_printf ("%" PRIu32, bson_count_keys (&ctx->ops_counts));
      BSON_APPEND_INT64 (&ctx->ops_counts, key, ops_count);
      BSON_APPEND_INT64 (&ctx->operation_ids, key, mongoc_apm_command_started_get_operation_id (event));
      // Record write concern (if present).
      bson_iter_t wc_iter;
      if (bson_iter_init_find (&wc_iter, cmd, "writeConcern")) {
         BSON_APPEND_ITER (&ctx->write_concerns, key, &wc_iter);
      } else {
         BSON_APPEND_NULL (&ctx->write_concerns, key);
      }
      bson_free (key);
   }

   if (0 == strcmp (cmd_name, "getMore")) {
      ctx->numGetMore++;
   }

   if (0 == strcmp (cmd_name, "killCursors")) {
      ctx->numKillCursors++;
   }
}

// `capture_bulkWrite_info` captures event data relevant to some bulk write prose tests.
static bulkWrite_ctx *
capture_bulkWrite_info (mongoc_client_t *client)
{
   bulkWrite_ctx *cb_ctx = bson_malloc0 (sizeof (*cb_ctx));
   bson_init (&cb_ctx->ops_counts);
   bson_init (&cb_ctx->operation_ids);
   bson_init (&cb_ctx->write_concerns);
   mongoc_apm_callbacks_t *cbs = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (cbs, bulkWrite_cb);
   mongoc_client_set_apm_callbacks (client, cbs, cb_ctx);
   mongoc_apm_callbacks_destroy (cbs);
   return cb_ctx;
}

static void
bulkWrite_ctx_reset (bulkWrite_ctx *cb_ctx)
{
   bson_reinit (&cb_ctx->ops_counts);
   bson_reinit (&cb_ctx->operation_ids);
   bson_reinit (&cb_ctx->write_concerns);
   cb_ctx->numGetMore = 0;
   cb_ctx->numKillCursors = 0;
}

static void
bulkWrite_ctx_destroy (bulkWrite_ctx *cb_ctx)
{
   if (!cb_ctx) {
      return;
   }
   bson_destroy (&cb_ctx->ops_counts);
   bson_destroy (&cb_ctx->operation_ids);
   bson_destroy (&cb_ctx->write_concerns);
   bson_free (cb_ctx);
}

static void
prose_test_3 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` batch splits a `writeModels` input with greater than `maxWriteBatchSize` operations
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();
   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   // Get `maxWriteBatchSize` from the server.
   int32_t maxWriteBatchSize;
   {
      bson_t reply;

      ok = mongoc_client_command_simple (client, "admin", tmp_bson ("{'hello': 1}"), NULL, &reply, &error);
      ASSERT_OR_PRINT (ok, error);

      maxWriteBatchSize = bson_lookup_int32 (&reply, "maxWriteBatchSize");
      bson_destroy (&reply);
   }

   bson_t *doc = tmp_bson ("{'a': 'b'}");
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
   for (int32_t i = 0; i < maxWriteBatchSize + 1; i++) {
      ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", doc, NULL, &error);
      ASSERT_OR_PRINT (ok, error);
   }

   mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, NULL /* options */);
   ASSERT (ret.res);
   ASSERT_CMPINT64 (mongoc_bulkwriteresult_insertedcount (ret.res), ==, maxWriteBatchSize + 1);
   mongoc_bulkwriteexception_destroy (ret.exc);
   mongoc_bulkwriteresult_destroy (ret.res);


   // Assert first `bulkWrite` sends `maxWriteBatchSize` ops.
   // Assert second `bulkWrite` sends 1 op.
   bson_t expect = BSON_INITIALIZER;
   BSON_APPEND_INT64 (&expect, "0", maxWriteBatchSize);
   BSON_APPEND_INT64 (&expect, "1", 1);
   ASSERT_EQUAL_BSON (&expect, &cb_ctx->ops_counts);
   bson_destroy (&expect);

   // Assert both have the same `operation_id`.
   int64_t operation_id_0 = bson_lookup_int64 (&cb_ctx->operation_ids, "0");
   int64_t operation_id_1 = bson_lookup_int64 (&cb_ctx->operation_ids, "1");
   ASSERT_CMPINT64 (operation_id_0, ==, operation_id_1);

   mongoc_bulkwrite_destroy (bw);
   bulkWrite_ctx_destroy (cb_ctx);
   mongoc_client_destroy (client);
}

static char *
repeat_char (char c, int32_t count)
{
   ASSERT (mlib_in_range (size_t, count));
   char *str = bson_malloc (count + 1);
   memset (str, c, count);
   str[count] = '\0';
   return str;
}

typedef struct {
   int32_t maxBsonObjectSize;
   int32_t maxMessageSizeBytes;
   int32_t maxWriteBatchSize;
} server_limits_t;

static server_limits_t
get_server_limits (mongoc_client_t *client)
{
   server_limits_t sl;
   bson_error_t error;
   bson_t reply;
   ASSERT_OR_PRINT (mongoc_client_command_simple (client, "admin", tmp_bson ("{'hello': 1}"), NULL, &reply, &error),
                    error);
   sl.maxBsonObjectSize = bson_lookup_int32 (&reply, "maxBsonObjectSize");
   sl.maxMessageSizeBytes = bson_lookup_int32 (&reply, "maxMessageSizeBytes");
   sl.maxWriteBatchSize = bson_lookup_int32 (&reply, "maxWriteBatchSize");
   bson_destroy (&reply);
   return sl;
}

static void
prose_test_4 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` batch splits when an `ops` payload exceeds `maxMessageSizeBytes`
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();
   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   // Get `maxWriteBatchSize` and `maxBsonObjectSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxMessageSizeBytes = sl.maxMessageSizeBytes;
   int32_t maxBsonObjectSize = sl.maxBsonObjectSize;


   bson_t doc = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('b', (size_t) maxBsonObjectSize - 500);
      BSON_APPEND_UTF8 (&doc, "a", large_str);
      bson_free (large_str);
   }

   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
   int32_t numModels = (maxMessageSizeBytes / maxBsonObjectSize) + 1;

   for (int32_t i = 0; i < numModels; i++) {
      ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", &doc, NULL, &error);
      ASSERT_OR_PRINT (ok, error);
   }

   mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, NULL /* options */);
   ASSERT (ret.res);
   ASSERT_NO_BULKWRITEEXCEPTION (ret);
   ASSERT_CMPINT64 (mongoc_bulkwriteresult_insertedcount (ret.res), ==, numModels);
   mongoc_bulkwriteexception_destroy (ret.exc);
   mongoc_bulkwriteresult_destroy (ret.res);


   // Assert two `bulkWrite`s were sent:
   ASSERT_CMPUINT32 (2, ==, bson_count_keys (&cb_ctx->ops_counts));

   // Assert first `bulkWrite` sends `numModels - 1` ops.
   int64_t ops_count_0 = bson_lookup_int64 (&cb_ctx->ops_counts, "0");
   ASSERT_CMPINT64 (ops_count_0, ==, numModels - 1);
   // Assert second `bulkWrite` sends 1 op.
   int64_t ops_count_1 = bson_lookup_int64 (&cb_ctx->ops_counts, "1");
   ASSERT_CMPINT64 (ops_count_1, ==, 1);

   // Assert both have the same `operation_id`.
   int64_t operation_id_0 = bson_lookup_int64 (&cb_ctx->operation_ids, "0");
   int64_t operation_id_1 = bson_lookup_int64 (&cb_ctx->operation_ids, "1");
   ASSERT_CMPINT64 (operation_id_0, ==, operation_id_1);

   mongoc_bulkwrite_destroy (bw);

   bulkWrite_ctx_destroy (cb_ctx);
   bson_destroy (&doc);
   mongoc_client_destroy (client);
}

static void
prose_test_5 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` collects `WriteConcernError`s across batches
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   // Construct client with retryable writes disabled.
   {
      mongoc_uri_t *uri = test_framework_get_uri ();
      mongoc_uri_set_option_as_bool (uri, MONGOC_URI_RETRYWRITES, false);
      client = mongoc_client_new_from_uri (uri);
      test_framework_set_ssl_opts (client);
      // Check if test runner is configured with a server API version:
      mongoc_server_api_t *api = test_framework_get_default_server_api ();
      if (api) {
         ASSERT_OR_PRINT (mongoc_client_set_server_api (client, api, &error), error);
      }
      mongoc_uri_destroy (uri);
   }

   // Get `maxWriteBatchSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxWriteBatchSize = sl.maxWriteBatchSize;

   // Drop collection to clear prior data.
   {
      mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
      mongoc_collection_drop (coll, NULL);
      mongoc_collection_destroy (coll);
   }

   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   // Set failpoint
   {
      ok = mongoc_client_command_simple (
         client,
         "admin",
         tmp_bson (BSON_STR ({
            "configureFailPoint" : "failCommand",
            "mode" : {"times" : 2},
            "data" : {
               "failCommands" : ["bulkWrite"],
               "writeConcernError" : {"code" : 91, "errmsg" : "Replication is being shut down"}
            }
         })),
         NULL,
         NULL,
         &error);
      ASSERT_OR_PRINT (ok, error);
   }

   // Construct models.
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
   {
      bson_t doc = BSON_INITIALIZER;
      BSON_APPEND_UTF8 (&doc, "a", "b");
      for (int32_t i = 0; i < maxWriteBatchSize + 1; i++) {
         ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", &doc, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }
   }

   mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, NULL /* options */);
   ASSERT (ret.res); // Has partial results.
   ASSERT (ret.exc);

   // Expect no top-level error.
   if (mongoc_bulkwriteexception_error (ret.exc, &error)) {
      test_error ("Expected no top-level error but got:\n%s", test_bulkwriteexception_str (ret.exc));
   }

   // Count write concern errors.
   {
      const bson_t *writeConcernErrors = mongoc_bulkwriteexception_writeconcernerrors (ret.exc);
      ASSERT_CMPUINT32 (bson_count_keys (writeConcernErrors), ==, 2);
   }

   // Assert partial results.
   ASSERT_CMPINT64 (mongoc_bulkwriteresult_insertedcount (ret.res), ==, maxWriteBatchSize + 1);

   // Assert two batches were sent.
   ASSERT_CMPUINT32 (bson_count_keys (&cb_ctx->ops_counts), ==, 2);

   bulkWrite_ctx_destroy (cb_ctx);
   mongoc_bulkwriteexception_destroy (ret.exc);
   mongoc_bulkwriteresult_destroy (ret.res);
   mongoc_bulkwrite_destroy (bw);
   mongoc_client_destroy (client);
}


static void
prose_test_6 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` handles individual `WriteError`s across batches
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();
   // Get `maxWriteBatchSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxWriteBatchSize = sl.maxWriteBatchSize;

   // Drop collection to clear prior data.
   mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
   mongoc_collection_drop (coll, NULL);


   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   bson_t document = BSON_INITIALIZER;
   BSON_APPEND_INT32 (&document, "_id", 1);
   ok = mongoc_collection_insert_one (coll, &document, NULL, NULL, &error);
   ASSERT_OR_PRINT (ok, error);


   // Test Unordered
   {
      // Construct models.
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);

      for (int32_t i = 0; i < maxWriteBatchSize + 1; i++) {
         ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", &document, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new ();
      mongoc_bulkwriteopts_set_ordered (opts, false);
      mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, opts);
      ASSERT (!ret.res); // No result due to no successful writes.
      ASSERT (ret.exc);

      if (mongoc_bulkwriteexception_error (ret.exc, &error)) {
         test_error ("Expected no top-level error but got:\n%s", test_bulkwriteexception_str (ret.exc));
      }

      // Count write errors.
      {
         const bson_t *writeErrors = mongoc_bulkwriteexception_writeerrors (ret.exc);
         ASSERT (mlib_in_range (uint32_t, maxWriteBatchSize + 1));
         ASSERT_CMPUINT32 (bson_count_keys (writeErrors), ==, (uint32_t) maxWriteBatchSize + 1);
      }

      // Assert two batches were sent.
      ASSERT_CMPUINT32 (bson_count_keys (&cb_ctx->ops_counts), ==, 2);

      mongoc_bulkwriteexception_destroy (ret.exc);
      mongoc_bulkwriteresult_destroy (ret.res);
      mongoc_bulkwriteopts_destroy (opts);
      mongoc_bulkwrite_destroy (bw);
   }

   // Reset state.
   bulkWrite_ctx_reset (cb_ctx);

   // Test Ordered
   {
      // Construct models.
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);

      for (int32_t i = 0; i < maxWriteBatchSize + 1; i++) {
         ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", &document, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }


      mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new ();
      mongoc_bulkwriteopts_set_ordered (opts, true);
      mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, opts);
      ASSERT (!ret.res); // No result due to no successful writes.
      ASSERT (ret.exc);

      if (mongoc_bulkwriteexception_error (ret.exc, &error)) {
         test_error ("Expected no top-level error but got:\n%s", test_bulkwriteexception_str (ret.exc));
      }

      // Count write errors.
      {
         const bson_t *writeErrors = mongoc_bulkwriteexception_writeerrors (ret.exc);
         ASSERT_CMPUINT32 (bson_count_keys (writeErrors), ==, 1);
      }

      // Assert one batch was sent.
      ASSERT_CMPUINT32 (bson_count_keys (&cb_ctx->ops_counts), ==, 1);

      mongoc_bulkwriteexception_destroy (ret.exc);
      mongoc_bulkwriteresult_destroy (ret.res);
      mongoc_bulkwriteopts_destroy (opts);
      mongoc_bulkwrite_destroy (bw);
   }

   bulkWrite_ctx_destroy (cb_ctx);
   bson_destroy (&document);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}

static void
prose_test_7 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` handles a cursor requiring a `getMore`
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();
   // Get `maxBsonObjectSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxBsonObjectSize = sl.maxBsonObjectSize;
   // Drop collection to clear prior data.
   mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
   mongoc_collection_drop (coll, NULL);

   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   // Construct models.
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
   size_t numModels = 0;

   mongoc_bulkwrite_updateoneopts_t *uo = mongoc_bulkwrite_updateoneopts_new ();
   mongoc_bulkwrite_updateoneopts_set_upsert (uo, true);
   bson_t *update = BCON_NEW ("$set", "{", "x", BCON_INT32 (1), "}");
   bson_t d1 = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('a', maxBsonObjectSize / 2);
      BSON_APPEND_UTF8 (&d1, "_id", large_str);
      bson_free (large_str);
   }

   ok = mongoc_bulkwrite_append_updateone (bw, "db.coll", &d1, update, uo, &error);
   ASSERT_OR_PRINT (ok, error);
   numModels++;

   bson_t d2 = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('b', maxBsonObjectSize / 2);
      BSON_APPEND_UTF8 (&d2, "_id", large_str);
      bson_free (large_str);
   }

   ok = mongoc_bulkwrite_append_updateone (bw, "db.coll", &d2, update, uo, &error);
   ASSERT_OR_PRINT (ok, error);
   numModels++;

   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new ();
   mongoc_bulkwriteopts_set_verboseresults (opts, true);
   mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, opts);
   ASSERT (ret.res);
   ASSERT_NO_BULKWRITEEXCEPTION (ret);

   ASSERT_CMPINT64 (mongoc_bulkwriteresult_upsertedcount (ret.res), ==, 2);

   // Check length of update results.
   {
      const bson_t *updateResults = mongoc_bulkwriteresult_updateresults (ret.res);
      ASSERT_CMPSIZE_T ((size_t) bson_count_keys (updateResults), ==, numModels);
   }

   ASSERT_CMPINT (cb_ctx->numGetMore, ==, 1);

   mongoc_bulkwriteopts_destroy (opts);
   bson_destroy (&d2);
   bson_destroy (&d1);
   bson_destroy (update);
   mongoc_bulkwriteexception_destroy (ret.exc);
   mongoc_bulkwriteresult_destroy (ret.res);
   mongoc_bulkwrite_updateoneopts_destroy (uo);
   mongoc_bulkwrite_destroy (bw);
   mongoc_collection_destroy (coll);
   bulkWrite_ctx_destroy (cb_ctx);
   mongoc_client_destroy (client);
}

static void
prose_test_8 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` handles a cursor requiring `getMore` within a transaction
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();
   // Get `maxBsonObjectSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxBsonObjectSize = sl.maxBsonObjectSize;

   // Drop collection to clear prior data.
   mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
   mongoc_collection_drop (coll, NULL);

   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   // Construct models.
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
   size_t numModels = 0;

   mongoc_bulkwrite_updateoneopts_t *uo = mongoc_bulkwrite_updateoneopts_new ();
   mongoc_bulkwrite_updateoneopts_set_upsert (uo, true);

   bson_t *update = BCON_NEW ("$set", "{", "x", BCON_INT32 (1), "}");
   mongoc_client_session_t *sess = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (sess, error);
   ASSERT_OR_PRINT (mongoc_client_session_start_transaction (sess, NULL, &error), error);

   bson_t d1 = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('a', maxBsonObjectSize / 2);
      BSON_APPEND_UTF8 (&d1, "_id", large_str);
      bson_free (large_str);
   }

   ok = mongoc_bulkwrite_append_updateone (bw, "db.coll", &d1, update, uo, &error);
   ASSERT_OR_PRINT (ok, error);
   numModels++;

   bson_t d2 = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('b', maxBsonObjectSize / 2);
      BSON_APPEND_UTF8 (&d2, "_id", large_str);
      bson_free (large_str);
   }

   ok = mongoc_bulkwrite_append_updateone (bw, "db.coll", &d2, update, uo, &error);
   ASSERT_OR_PRINT (ok, error);
   numModels++;

   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new ();
   mongoc_bulkwriteopts_set_verboseresults (opts, true);
   mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, opts);
   ASSERT (ret.res);
   ASSERT_NO_BULKWRITEEXCEPTION (ret);

   ASSERT_CMPINT64 (mongoc_bulkwriteresult_upsertedcount (ret.res), ==, 2);

   ASSERT_CMPINT (cb_ctx->numGetMore, ==, 1);

   // Check length of update results.
   {
      const bson_t *updateResults = mongoc_bulkwriteresult_updateresults (ret.res);
      ASSERT_CMPSIZE_T ((size_t) bson_count_keys (updateResults), ==, numModels);
   }

   mongoc_bulkwrite_updateoneopts_destroy (uo);
   mongoc_bulkwriteopts_destroy (opts);
   bson_destroy (&d2);
   bson_destroy (&d1);
   bson_destroy (update);
   mongoc_client_session_destroy (sess);
   mongoc_bulkwriteexception_destroy (ret.exc);
   mongoc_bulkwriteresult_destroy (ret.res);
   mongoc_bulkwrite_destroy (bw);
   mongoc_collection_destroy (coll);
   bulkWrite_ctx_destroy (cb_ctx);
   mongoc_client_destroy (client);
}

static void
prose_test_9 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` handles a `getMore` error
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();
   // Get `maxBsonObjectSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxBsonObjectSize = sl.maxBsonObjectSize;

   // Drop collection to clear prior data.
   mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
   mongoc_collection_drop (coll, NULL);

   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   // Configure failpoint on `getMore`.
   {
      {
         ok = mongoc_client_command_simple (client,
                                            "admin",
                                            tmp_bson (BSON_STR ({
                                               "configureFailPoint" : "failCommand",
                                               "mode" : {"times" : 1},
                                               "data" : {"failCommands" : ["getMore"], "errorCode" : 8}
                                            })),
                                            NULL,
                                            NULL,
                                            &error);
         ASSERT_OR_PRINT (ok, error);
      }
   }

   bson_t *update = BCON_NEW ("$set", "{", "x", BCON_INT32 (1), "}");

   // Construct models.
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
   size_t numModels = 0;

   mongoc_bulkwrite_updateoneopts_t *uo = mongoc_bulkwrite_updateoneopts_new ();
   mongoc_bulkwrite_updateoneopts_set_upsert (uo, true);

   bson_t d1 = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('a', maxBsonObjectSize / 2);
      BSON_APPEND_UTF8 (&d1, "_id", large_str);
      bson_free (large_str);
   }

   ok = mongoc_bulkwrite_append_updateone (bw, "db.coll", &d1, update, uo, &error);
   ASSERT_OR_PRINT (ok, error);
   numModels++;

   bson_t d2 = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('b', maxBsonObjectSize / 2);
      BSON_APPEND_UTF8 (&d2, "_id", large_str);
      bson_free (large_str);
   }

   ok = mongoc_bulkwrite_append_updateone (bw, "db.coll", &d2, update, uo, &error);
   ASSERT_OR_PRINT (ok, error);
   numModels++;

   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new ();
   mongoc_bulkwriteopts_set_verboseresults (opts, true);
   mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, opts);
   ASSERT (ret.res);
   ASSERT (ret.exc);

   if (!mongoc_bulkwriteexception_error (ret.exc, &error)) {
      test_error ("Expected top-level error but got:\n%s", test_bulkwriteexception_str (ret.exc));
   }
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_QUERY, 8, "Failing command via 'failCommand' failpoint");
   ASSERT_CMPSIZE_T ((size_t) mongoc_bulkwriteresult_upsertedcount (ret.res), ==, numModels);

   // Check length of update results.
   {
      const bson_t *updateResults = mongoc_bulkwriteresult_updateresults (ret.res);
      ASSERT_CMPSIZE_T ((size_t) bson_count_keys (updateResults), ==, 1);
   }
   ASSERT_CMPINT (cb_ctx->numGetMore, ==, 1);
   ASSERT_CMPINT (cb_ctx->numKillCursors, ==, 1);

   mongoc_bulkwrite_updateoneopts_destroy (uo);
   mongoc_bulkwriteopts_destroy (opts);
   bson_destroy (&d2);
   bson_destroy (&d1);
   bson_destroy (update);
   mongoc_bulkwriteexception_destroy (ret.exc);
   mongoc_bulkwriteresult_destroy (ret.res);
   mongoc_bulkwrite_destroy (bw);
   mongoc_collection_destroy (coll);
   bulkWrite_ctx_destroy (cb_ctx);
   mongoc_client_destroy (client);
}


static void
prose_test_10 (void *ctx)
{
   /*
   `MongoClient.bulkWrite` returns error for unacknowledged too-large insert
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;
   mongoc_write_concern_t *wc;

   client = test_framework_new_default_client ();
   // Get `maxBsonObjectSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxBsonObjectSize = sl.maxBsonObjectSize;

   bson_t doc = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('b', maxBsonObjectSize);
      BSON_APPEND_UTF8 (&doc, "a", large_str);
      bson_free (large_str);
   }


   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
   mongoc_bulkwriteopts_t *opts = mongoc_bulkwriteopts_new ();
   mongoc_bulkwriteopts_set_ordered (opts, false);
   mongoc_bulkwriteopts_set_writeconcern (opts, wc);

   // Test a large insert.
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
      ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", &doc, NULL, &error);
      ASSERT_OR_PRINT (ok, error);

      mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, opts);
      ASSERT (!ret.res); // No result due to unacknowledged write concern.
      ASSERT (ret.exc);
      if (!mongoc_bulkwriteexception_error (ret.exc, &error)) {
         test_error ("Expected top-level error but got:\n%s", test_bulkwriteexception_str (ret.exc));
      }
      ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "of size");
      mongoc_bulkwriteexception_destroy (ret.exc);
      mongoc_bulkwriteresult_destroy (ret.res);
      mongoc_bulkwrite_destroy (bw);
   }

   // Test a large replace.
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
      ok = mongoc_bulkwrite_append_replaceone (bw, "db.coll", tmp_bson ("{}"), &doc, NULL, &error);
      ASSERT_OR_PRINT (ok, error);

      mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, opts);
      ASSERT (!ret.res); // No result due to unacknowledged write concern.
      ASSERT (ret.exc);
      if (!mongoc_bulkwriteexception_error (ret.exc, &error)) {
         test_error ("Expected top-level error but got:\n%s", test_bulkwriteexception_str (ret.exc));
      }
      ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "of size");
      mongoc_bulkwriteexception_destroy (ret.exc);
      mongoc_bulkwriteresult_destroy (ret.res);
      mongoc_bulkwrite_destroy (bw);
   }

   mongoc_bulkwriteopts_destroy (opts);
   bson_destroy (&doc);
   mongoc_write_concern_destroy (wc);
   mongoc_client_destroy (client);
}

static void
capture_all_bulkWrite_commands (const mongoc_apm_command_started_t *event)
{
   if (0 == strcmp (mongoc_apm_command_started_get_command_name (event), "bulkWrite")) {
      mongoc_array_t *captured = mongoc_apm_command_started_get_context (event);
      bson_t *cmd = bson_copy (mongoc_apm_command_started_get_command (event));
      _mongoc_array_append_val (captured, cmd);
   }
}

typedef struct {
   mongoc_client_t *client;
   int32_t maxMessageSizeBytes;
   int32_t maxBsonObjectSize;
   int32_t numModels;
   mongoc_array_t captured;
   mongoc_bulkwrite_t *bw;
} prose_test_11_fixture_t;

static prose_test_11_fixture_t *
prose_test_11_fixture_new (void)
{
   bool ok;
   bson_error_t error;

   prose_test_11_fixture_t *tf = bson_malloc0 (sizeof (*tf));
   tf->client = test_framework_new_default_client ();
   // Get `maxMessageSizeBytes` and `maxBsonObjectSize` from the server.
   server_limits_t sl = get_server_limits (tf->client);
   tf->maxMessageSizeBytes = sl.maxMessageSizeBytes;
   tf->maxBsonObjectSize = sl.maxBsonObjectSize;

   // See CRUD prose test 12 description for the calculation of these values.
   const int32_t opsBytes = tf->maxMessageSizeBytes - 1122;
   tf->numModels = opsBytes / tf->maxBsonObjectSize;
   const int32_t remainderBytes = opsBytes % tf->maxBsonObjectSize;


   _mongoc_array_init (&tf->captured, sizeof (bson_t *));
   // Set callback to capture all `bulkWrite` commands.
   {
      mongoc_apm_callbacks_t *cbs = mongoc_apm_callbacks_new ();
      mongoc_apm_set_command_started_cb (cbs, capture_all_bulkWrite_commands);
      mongoc_client_set_apm_callbacks (tf->client, cbs, &tf->captured);
      mongoc_apm_callbacks_destroy (cbs);
   }

   tf->bw = mongoc_client_bulkwrite_new (tf->client);


   // Add initial list of documents.
   {
      // Create a document { 'a': 'b'.repeat(maxBsonObjectSize - 57) }
      bson_t *doc;
      {
         char *large_str = repeat_char ('b', tf->maxBsonObjectSize - 57);
         doc = BCON_NEW ("a", BCON_UTF8 (large_str));
         bson_free (large_str);
      }

      mlib_foreach_irange (i, tf->numModels) {
         (void) i;
         ok = mongoc_bulkwrite_append_insertone (tf->bw, "db.coll", doc, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      bson_destroy (doc);
   }

   if (remainderBytes >= 217) {
      // Create a document { 'a': 'b'.repeat(remainderBytes - 57) }
      bson_t *doc;
      {
         char *large_str = repeat_char ('b', remainderBytes - 57);
         doc = BCON_NEW ("a", BCON_UTF8 (large_str));
         bson_free (large_str);
      }

      ok = mongoc_bulkwrite_append_insertone (tf->bw, "db.coll", doc, NULL, &error);
      ASSERT_OR_PRINT (ok, error);
      tf->numModels++;
      bson_destroy (doc);
   }
   return tf;
}

static void
prose_test_11_fixture_destroy (prose_test_11_fixture_t *tf)
{
   if (!tf) {
      return;
   }
   for (size_t i = 0; i < tf->captured.len; i++) {
      bson_t *el = _mongoc_array_index (&tf->captured, bson_t *, i);
      bson_destroy (el);
   }

   _mongoc_array_destroy (&tf->captured);

   mongoc_bulkwrite_destroy (tf->bw);
   mongoc_client_destroy (tf->client);
   bson_free (tf);
}

static void
prose_test_11 (void *ctx)
{
   /*
   11. `MongoClient.bulkWrite` batch splits when the addition of a new namespace exceeds the maximum message size
   */
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   // Case 1: Does not split
   {
      prose_test_11_fixture_t *tf = prose_test_11_fixture_new ();

      // Add a document with the same namespace (expected to not result in a batch split).
      {
         bson_t *second_doc = BCON_NEW ("a", "b");
         ok = mongoc_bulkwrite_append_insertone (tf->bw, "db.coll", second_doc, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
         bson_destroy (second_doc);
      }

      // Execute.
      {
         mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute (tf->bw, NULL /* opts */);
         ASSERT (bwr.res);
         ASSERT_NO_BULKWRITEEXCEPTION (bwr);
         ASSERT (mlib_in_range (int64_t, tf->numModels));
         ASSERT_CMPINT64 (mongoc_bulkwriteresult_insertedcount (bwr.res), ==, (int64_t) tf->numModels + 1);
         mongoc_bulkwriteresult_destroy (bwr.res);
         mongoc_bulkwriteexception_destroy (bwr.exc);
      }

      // Expect one `bulkWrite` command is sent.
      ASSERT_CMPSIZE_T (tf->captured.len, ==, 1);
      // Expect the event contains the namespace for `db.coll`.
      bson_t *first = _mongoc_array_index (&tf->captured, bson_t *, 0);
      {
         bson_t *ops = bson_lookup_bson (first, "ops");
         ASSERT (mlib_in_range (uint32_t, tf->numModels));
         ASSERT_CMPUINT32 (bson_count_keys (ops), ==, (uint32_t) tf->numModels + 1);
         bson_destroy (ops);

         bson_t *nsInfo = bson_lookup_bson (first, "nsInfo");
         ASSERT_CMPUINT32 (bson_count_keys (nsInfo), ==, 1);
         bson_destroy (nsInfo);

         const char *ns = bson_lookup_utf8 (first, "nsInfo.0.ns");
         ASSERT_CMPSTR (ns, "db.coll");
      }
      prose_test_11_fixture_destroy (tf);
   }

   // Case 2: Splits with new namespace
   {
      prose_test_11_fixture_t *tf = prose_test_11_fixture_new ();

      // Create a large namespace.
      char *large_ns;
      {
         char *coll = repeat_char ('c', 200);
         large_ns = bson_strdup_printf ("db.%s", coll);
         bson_free (coll);
      }

      // Add a document that results in a batch split due to the namespace.
      {
         bson_t *second_doc = BCON_NEW ("a", "b");
         ok = mongoc_bulkwrite_append_insertone (tf->bw, large_ns, second_doc, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
         bson_destroy (second_doc);
      }

      // Execute.
      {
         mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute (tf->bw, NULL /* opts */);
         ASSERT (bwr.res);
         ASSERT_NO_BULKWRITEEXCEPTION (bwr);
         ASSERT (mlib_in_range (int64_t, tf->numModels));
         ASSERT_CMPINT64 (mongoc_bulkwriteresult_insertedcount (bwr.res), ==, (int64_t) tf->numModels + 1);
         mongoc_bulkwriteresult_destroy (bwr.res);
         mongoc_bulkwriteexception_destroy (bwr.exc);
      }

      // Expect two `bulkWrite` commands were sent.
      ASSERT_CMPSIZE_T (tf->captured.len, ==, 2);
      // Expect the first only contains the namespace for `db.coll`.
      bson_t *first = _mongoc_array_index (&tf->captured, bson_t *, 0);
      {
         bson_t *ops = bson_lookup_bson (first, "ops");
         ASSERT (mlib_in_range (uint32_t, tf->numModels));
         ASSERT_CMPUINT32 (bson_count_keys (ops), ==, (uint32_t) tf->numModels);
         bson_destroy (ops);

         bson_t *nsInfo = bson_lookup_bson (first, "nsInfo");
         ASSERT_CMPUINT32 (bson_count_keys (nsInfo), ==, 1);
         bson_destroy (nsInfo);

         const char *ns = bson_lookup_utf8 (first, "nsInfo.0.ns");
         ASSERT_CMPSTR (ns, "db.coll");
      }

      // Expect the second only contains the namespace for `large_ns`.
      bson_t *second = _mongoc_array_index (&tf->captured, bson_t *, 1);
      {
         bson_t *ops = bson_lookup_bson (second, "ops");
         ASSERT_CMPUINT32 (bson_count_keys (ops), ==, 1);
         bson_destroy (ops);

         bson_t *nsInfo = bson_lookup_bson (second, "nsInfo");
         ASSERT_CMPUINT32 (bson_count_keys (nsInfo), ==, 1);
         bson_destroy (nsInfo);

         const char *ns = bson_lookup_utf8 (second, "nsInfo.0.ns");
         ASSERT_CMPSTR (ns, large_ns);
      }

      bson_free (large_ns);
      prose_test_11_fixture_destroy (tf);
   }
}

static void
prose_test_12 (void *ctx)
{
   /*
   12. `MongoClient.bulkWrite` returns an error if no operations can be added to `ops`
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();

   // Get `maxMessageSizeBytes` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxMessageSizeBytes = sl.maxMessageSizeBytes;

   // Create a large string.
   char *large_str = repeat_char ('b', maxMessageSizeBytes);

   // Test too-big document.
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);

      bson_t *large_doc = BCON_NEW ("a", BCON_UTF8 (large_str));

      // Create bulk write.
      {
         ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", large_doc, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      // Execute.
      {
         mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute (bw, NULL);
         ASSERT (!bwr.res); // No result due to no successful writes.
         ASSERT (bwr.exc);
         if (!mongoc_bulkwriteexception_error (bwr.exc, &error)) {
            test_error ("Expected top-level error but got:\n%s", test_bulkwriteexception_str (bwr.exc));
         }
         ASSERT_ERROR_CONTAINS (
            error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "unable to send document");
         mongoc_bulkwriteresult_destroy (bwr.res);
         mongoc_bulkwriteexception_destroy (bwr.exc);
      }
      bson_destroy (large_doc);
      mongoc_bulkwrite_destroy (bw);
   }

   // Test too-big namespace.
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);

      char *large_namespace = bson_strdup_printf ("db.%s", large_str);

      // Create bulk write.
      {
         ok = mongoc_bulkwrite_append_insertone (bw, large_namespace, tmp_bson ("{'a': 'b'}"), NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      // Execute.
      {
         mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute (bw, NULL);
         ASSERT (!bwr.res); // No result due to no successful writes.
         ASSERT (bwr.exc);
         if (!mongoc_bulkwriteexception_error (bwr.exc, &error)) {
            test_error ("Expected top-level error but got:\n%s", test_bulkwriteexception_str (bwr.exc));
         }
         ASSERT_ERROR_CONTAINS (
            error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "unable to send document");
         mongoc_bulkwriteresult_destroy (bwr.res);
         mongoc_bulkwriteexception_destroy (bwr.exc);
      }
      bson_free (large_namespace);
      mongoc_bulkwrite_destroy (bw);
   }

   bson_free (large_str);
   mongoc_client_destroy (client);
}

static void
prose_test_13 (void *ctx)
{
   /*
   13. `MongoClient.bulkWrite` errors if configured with automatic encryption.
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();
   mongoc_auto_encryption_opts_t *aeo = mongoc_auto_encryption_opts_new ();
   mongoc_auto_encryption_opts_set_keyvault_namespace (aeo, "db", "coll");
   mongoc_auto_encryption_opts_set_kms_providers (
      aeo, tmp_bson (BSON_STR ({"aws" : {"accessKeyId" : "foo", "secretAccessKey" : "bar"}})));
   ok = mongoc_client_enable_auto_encryption (client, aeo, &error);
   ASSERT_OR_PRINT (ok, error);

   // Try to to a bulk write.
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);

      // Create bulk write.
      {
         ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", tmp_bson ("{'a': 'b'}"), NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      // Execute.
      {
         mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute (bw, NULL);
         ASSERT (!bwr.res); // No result due to no successful writes.
         ASSERT (bwr.exc);
         if (!mongoc_bulkwriteexception_error (bwr.exc, &error)) {
            test_error ("Expected top-level error but got:\n%s", test_bulkwriteexception_str (bwr.exc));
         }
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                "bulkWrite does not currently support automatic encryption");
         mongoc_bulkwriteresult_destroy (bwr.res);
         mongoc_bulkwriteexception_destroy (bwr.exc);
      }
      mongoc_bulkwrite_destroy (bw);
   }

   mongoc_auto_encryption_opts_destroy (aeo);
   mongoc_client_destroy (client);
}

static void
prose_test_15 (void *ctx)
{
   /*
   15. `MongoClient.bulkWrite` with unacknowledged write concern uses `w:0` for all batches
   */
   mongoc_client_t *client;
   BSON_UNUSED (ctx);
   bool ok;
   bson_error_t error;

   client = test_framework_new_default_client ();

   // Drop collection.
   {
      mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
      mongoc_collection_drop (coll, NULL); // Ignore error.
      mongoc_collection_destroy (coll);
   }

   // Create collection to workaround SERVER-95537.
   {
      mongoc_database_t *db = mongoc_client_get_database (client, "db");
      mongoc_collection_t *coll = mongoc_database_create_collection (db, "coll", NULL, &error);
      ASSERT_OR_PRINT (coll, error);
      mongoc_collection_destroy (coll);
      mongoc_database_destroy (db);
   }

   // Set callbacks to count the number of bulkWrite commands sent.
   bulkWrite_ctx *cb_ctx = capture_bulkWrite_info (client);

   // Get `maxWriteBatchSize` and `maxBsonObjectSize` from the server.
   server_limits_t sl = get_server_limits (client);
   int32_t maxMessageSizeBytes = sl.maxMessageSizeBytes;
   int32_t maxBsonObjectSize = sl.maxBsonObjectSize;


   // Make a large document.
   bson_t doc = BSON_INITIALIZER;
   {
      char *large_str = repeat_char ('b', (size_t) maxBsonObjectSize - 500);
      BSON_APPEND_UTF8 (&doc, "a", large_str);
      bson_free (large_str);
   }

   // Execute bulkWrite.
   {
      mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);
      for (int32_t i = 0; i < maxMessageSizeBytes / maxBsonObjectSize + 1; i++) {
         ok = mongoc_bulkwrite_append_insertone (bw, "db.coll", &doc, NULL, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      // Configure options with unacknowledge write concern and unordered writes.
      mongoc_bulkwriteopts_t *bwo;
      {
         mongoc_write_concern_t *wc = mongoc_write_concern_new ();
         mongoc_write_concern_set_w (wc, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
         bwo = mongoc_bulkwriteopts_new ();
         mongoc_bulkwriteopts_set_writeconcern (bwo, wc);
         mongoc_bulkwriteopts_set_ordered (bwo, false);
         mongoc_write_concern_destroy (wc);
      }

      mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute (bw, bwo);
      ASSERT (!ret.res); // No result due to unacknowledged.
      ASSERT_NO_BULKWRITEEXCEPTION (ret);
      mongoc_bulkwriteexception_destroy (ret.exc);
      mongoc_bulkwriteresult_destroy (ret.res);
      mongoc_bulkwriteopts_destroy (bwo);
      mongoc_bulkwrite_destroy (bw);
   }

   // Check command started events.
   {
      bson_t expect = BSON_INITIALIZER;
      // Assert first `bulkWrite` sends `maxWriteBatchSize` ops.
      BSON_APPEND_INT64 (&expect, "0", maxMessageSizeBytes / maxBsonObjectSize);
      // Assert second `bulkWrite` sends 1 op.
      BSON_APPEND_INT64 (&expect, "1", 1);
      ASSERT_EQUAL_BSON (&expect, &cb_ctx->ops_counts);
      bson_destroy (&expect);

      // Assert both have the same `operation_id`.
      int64_t operation_id_0 = bson_lookup_int64 (&cb_ctx->operation_ids, "0");
      int64_t operation_id_1 = bson_lookup_int64 (&cb_ctx->operation_ids, "1");
      ASSERT_CMPINT64 (operation_id_0, ==, operation_id_1);

      // Assert both use unacknowledged write concern.
      bson_init (&expect);
      BCON_APPEND (&expect, "0", "{", "w", BCON_INT32 (0), "}");
      BCON_APPEND (&expect, "1", "{", "w", BCON_INT32 (0), "}");
      ASSERT_EQUAL_BSON (&expect, &cb_ctx->write_concerns);
      bson_destroy (&expect);
   }

   // Count documents in collection.
   {
      mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
      int64_t expected = maxMessageSizeBytes / maxBsonObjectSize + 1;
      int64_t got = mongoc_collection_count_documents (coll, tmp_bson ("{}"), NULL, NULL, NULL, &error);
      ASSERT_CMPINT64 (got, ==, expected);
      mongoc_collection_destroy (coll);
   }

   bson_destroy (&doc);
   bulkWrite_ctx_destroy (cb_ctx);
   mongoc_client_destroy (client);
}


void
test_crud_install (TestSuite *suite)
{
   test_all_spec_tests (suite);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_1",
                      prose_test_1,
                      NULL, /* dtor */
                      NULL, /* ctx */
                      test_framework_skip_if_no_failpoint);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_2",
                      prose_test_2,
                      NULL, /* dtor */
                      NULL, /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_13);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_3",
                      prose_test_3,
                      NULL, /* dtor */
                      NULL, /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25 /* require 8.0+ server */);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_4",
                      prose_test_4,
                      NULL, /* dtor */
                      NULL, /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25 /* require 8.0+ server */);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_5",
                      prose_test_5,
                      NULL, /* dtor */
                      NULL, /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25 /* require 8.0+ server */);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_6",
                      prose_test_6,
                      NULL,                                                /* dtor */
                      NULL,                                                /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25 /* require 8.0+ server */
   );


   TestSuite_AddFull (suite,
                      "/crud/prose_test_7",
                      prose_test_7,
                      NULL,                                                /* dtor */
                      NULL,                                                /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25 /* require 8.0+ server */
   );


   TestSuite_AddFull (suite,
                      "/crud/prose_test_8",
                      prose_test_8,
                      NULL,                                                 /* dtor */
                      NULL,                                                 /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25, /* require 8.0+ server */
                      test_framework_skip_if_no_txns);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_9",
                      prose_test_9,
                      NULL,                                                /* dtor */
                      NULL,                                                /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25 /* require 8.0+ server */
   );

   TestSuite_AddFull (suite,
                      "/crud/prose_test_10",
                      prose_test_10,
                      NULL, /* dtor */
                      NULL, /* ctx */
                      test_framework_skip_if_max_wire_version_less_than_25 /* require 8.0+ server */);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_11",
                      prose_test_11,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull (suite,
                      "/crud/prose_test_12",
                      prose_test_12,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );

   TestSuite_AddFull (suite,
                      "/crud/prose_test_13",
                      prose_test_13,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_max_wire_version_less_than_25, // require server 8.0
                      test_framework_skip_if_no_client_side_encryption);

   TestSuite_AddFull (suite,
                      "/crud/prose_test_15",
                      prose_test_15,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_max_wire_version_less_than_25 // require server 8.0
   );
}
