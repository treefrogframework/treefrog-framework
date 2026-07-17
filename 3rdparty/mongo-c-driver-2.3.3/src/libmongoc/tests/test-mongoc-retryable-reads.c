#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-collection-private.h>
#include <mongoc/mongoc-uri-private.h>

#include <mongoc/mongoc.h>

#include <json-test-operations.h>
#include <json-test.h>
#include <mock_server/future-functions.h>
#include <mock_server/future.h>
#include <mock_server/mock-rs.h>
#include <test-libmongoc.h>
#include <test-mongoc-retryability-helpers.h>

static bool
retryable_reads_test_run_operation(json_test_ctx_t *ctx, const bson_t *test, const bson_t *operation)
{
   bool *explicit_session = (bool *)ctx->config->ctx;
   bson_t reply;
   bson_iter_t iter;
   const char *op_name;
   uint32_t op_len;
   bool res;

   bson_iter_init_find(&iter, operation, "name");
   op_name = bson_iter_utf8(&iter, &op_len);
   if (strcmp(op_name, "estimatedDocumentCount") == 0 || strcmp(op_name, "count") == 0) {
      /* CDRIVER-3612: mongoc_collection_estimated_document_count does not
       * support explicit sessions */
      *explicit_session = false;
   }
   res =
      json_test_operation(ctx, test, operation, ctx->collection, *explicit_session ? ctx->sessions[0] : NULL, &reply);

   bson_destroy(&reply);

   return res;
}


/* Callback for JSON tests from Retryable Reads Spec */
static void
test_retryable_reads_cb(void *scenario)
{
   bool explicit_session;
   json_test_config_t config = JSON_TEST_CONFIG_INIT;

   /* use the context pointer to send "explicit_session" to the callback */
   config.ctx = &explicit_session;
   config.run_operation_cb = retryable_reads_test_run_operation;
   config.scenario = scenario;
   config.command_started_events_only = true;
   explicit_session = true;
   run_json_general_test(&config);
   explicit_session = false;
   run_json_general_test(&config);
}


static void
_set_failpoint(mongoc_client_t *client)
{
   bson_error_t error;
   bson_t *cmd = tmp_bson("{'configureFailPoint': 'failCommand',"
                          " 'mode': {'times': 1},"
                          " 'data': {'errorCode': 10107, 'failCommands': ['count']}}");

   ASSERT(client);

   ASSERT_OR_PRINT(mongoc_client_command_simple(client, "admin", cmd, NULL, NULL, &error), error);
}
/* Test code paths for all command helpers */
static void
test_cmd_helpers(void *ctx)
{
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   uint32_t server_id;
   mongoc_collection_t *collection;
   bson_t *cmd;
   bson_t reply;
   bson_error_t error;
   bson_iter_t iter;
   mongoc_database_t *database;

   BSON_UNUSED(ctx);

   uri = test_framework_get_uri();
   mongoc_uri_set_option_as_bool(uri, MONGOC_URI_RETRYREADS, true);

   client = test_framework_client_new_from_uri(uri, NULL);
   mongoc_client_set_error_api(client, MONGOC_ERROR_API_VERSION_2);
   test_framework_set_ssl_opts(client);
   mongoc_uri_destroy(uri);

   /* clean up in case a previous test aborted */
   const mongoc_ss_log_context_t ss_log_context = {.operation = "configureFailPoint"};
   server_id =
      mongoc_topology_select_server_id(client->topology, MONGOC_SS_WRITE, &ss_log_context, NULL, NULL, NULL, &error);
   ASSERT_OR_PRINT(server_id, error);
   deactivate_fail_points(client, server_id);

   collection = get_test_collection(client, "retryable_reads");
   database = mongoc_client_get_database(client, "test");

   if (!mongoc_collection_drop(collection, &error)) {
      if (NULL == strstr(error.message, "ns not found")) {
         /* an error besides ns not found */
         ASSERT_OR_PRINT(false, error);
      }
   }

   ASSERT_OR_PRINT(mongoc_collection_insert_one(collection, tmp_bson("{'_id': 0}"), NULL, NULL, &error), error);
   ASSERT_OR_PRINT(mongoc_collection_insert_one(collection, tmp_bson("{'_id': 1}"), NULL, NULL, &error), error);

   cmd = tmp_bson("{'count': '%s'}", collection->collection);

   /* read helpers must retry. */
   _set_failpoint(client);
   ASSERT_OR_PRINT(mongoc_client_read_command_with_opts(client, "test", cmd, NULL, NULL, &reply, &error), error);
   bson_iter_init_find(&iter, &reply, "n");
   ASSERT(bson_iter_as_int64(&iter) == 2);
   bson_destroy(&reply);

   _set_failpoint(client);
   ASSERT_OR_PRINT(mongoc_database_read_command_with_opts(database, cmd, NULL, NULL, &reply, &error), error);
   bson_iter_init_find(&iter, &reply, "n");
   ASSERT(bson_iter_as_int64(&iter) == 2);
   bson_destroy(&reply);

   _set_failpoint(client);
   ASSERT_OR_PRINT(mongoc_collection_read_command_with_opts(collection, cmd, NULL, NULL, &reply, &error), error);
   bson_iter_init_find(&iter, &reply, "n");
   ASSERT(bson_iter_as_int64(&iter) == 2);
   bson_destroy(&reply);

   /* TODO: once CDRIVER-3314 is resolved, test the read+write helpers. */

   /* read/write agnostic command_simple helpers must not retry. */
   _set_failpoint(client);
   ASSERT(!mongoc_client_command_simple(client, "test", cmd, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint(client);
   ASSERT(!mongoc_database_command_simple(database, cmd, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint(client);
   ASSERT(!mongoc_collection_command_simple(collection, cmd, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_SERVER, 10107, "Failing command");


   /* read/write agnostic command_with_opts helpers must not retry. */
   _set_failpoint(client);
   ASSERT(!mongoc_client_command_with_opts(client, "test", cmd, NULL, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint(client);
   ASSERT(!mongoc_database_command_with_opts(database, cmd, NULL, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   _set_failpoint(client);
   ASSERT(!mongoc_collection_command_with_opts(collection, cmd, NULL, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   /* read/write agnostic command_simple_with_server_id helper must not retry.
    */
   server_id =
      mongoc_topology_select_server_id(client->topology, MONGOC_SS_WRITE, &ss_log_context, NULL, NULL, NULL, &error);
   ASSERT_OR_PRINT(server_id, error);
   _set_failpoint(client);
   ASSERT(!mongoc_client_command_simple_with_server_id(client, "test", cmd, NULL, server_id, NULL, &error));
   ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_SERVER, 10107, "Failing command");

   ASSERT_OR_PRINT(mongoc_collection_drop(collection, &error), error);

   deactivate_fail_points(client, server_id);
   mongoc_collection_destroy(collection);
   mongoc_database_destroy(database);
   mongoc_client_destroy(client);
}

static void
test_retry_reads_off(void *ctx)
{
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   uint32_t server_id;
   bson_t *cmd;
   bson_error_t error;
   bool res;

   BSON_UNUSED(ctx);

   uri = test_framework_get_uri();
   mongoc_uri_set_option_as_bool(uri, "retryreads", false);
   client = test_framework_client_new_from_uri(uri, NULL);
   test_framework_set_ssl_opts(client);

   /* clean up in case a previous test aborted */
   const mongoc_ss_log_context_t ss_log_context = {.operation = "configureFailPoint"};
   server_id =
      mongoc_topology_select_server_id(client->topology, MONGOC_SS_WRITE, &ss_log_context, NULL, NULL, NULL, &error);
   ASSERT_OR_PRINT(server_id, error);
   deactivate_fail_points(client, server_id);

   collection = get_test_collection(client, "retryable_reads");

   cmd = tmp_bson("{'configureFailPoint': 'failCommand',"
                  " 'mode': {'times': 1},"
                  " 'data': {'errorCode': 10107, 'failCommands': ['count']}}");
   ASSERT_OR_PRINT(mongoc_client_command_simple_with_server_id(client, "admin", cmd, NULL, server_id, NULL, &error),
                   error);

   cmd = tmp_bson("{'count': 'coll'}", collection->collection);

   res = mongoc_collection_read_command_with_opts(collection, cmd, NULL, NULL, NULL, &error);
   ASSERT(!res);
   ASSERT_CONTAINS(error.message, "failpoint");

   deactivate_fail_points(client, server_id);

   mongoc_collection_destroy(collection);
   mongoc_uri_destroy(uri);
   mongoc_client_destroy(client);
}

typedef struct _test_retry_reads_sharded_on_other_mongos_ctx {
   int count;
   uint16_t ports[2];
} test_retry_reads_sharded_on_other_mongos_ctx;

static void
_test_retry_reads_sharded_on_other_mongos_cb(const mongoc_apm_command_failed_t *event)
{
   BSON_ASSERT_PARAM(event);

   test_retry_reads_sharded_on_other_mongos_ctx *const ctx =
      (test_retry_reads_sharded_on_other_mongos_ctx *)mongoc_apm_command_failed_get_context(event);
   BSON_ASSERT(ctx);

   ASSERT_WITH_MSG(ctx->count < 2, "expected at most two failpoints to trigger");

   const mongoc_host_list_t *const host = mongoc_apm_command_failed_get_host(event);
   BSON_ASSERT(host);
   BSON_ASSERT(!host->next);
   ctx->ports[ctx->count++] = host->port;
}

// Retryable Reads Are Retried on a Different mongos if One is Available
static void
test_retry_reads_sharded_on_other_mongos(void *_ctx)
{
   BSON_UNUSED(_ctx);

   bson_error_t error = {0};

   // Create two clients `s0` and `s1` that each connect to a single mongos from
   // the sharded cluster. They must not connect to the same mongos.
   const char *ports[] = {"27017", "27018"};
   const size_t num_ports = sizeof(ports) / sizeof(*ports);
   mongoc_array_t clients = _test_get_mongos_clients(ports, num_ports);
   BSON_ASSERT(clients.len == 2u);
   mongoc_client_t *const s0 = _mongoc_array_index(&clients, mongoc_client_t *, 0u);
   mongoc_client_t *const s1 = _mongoc_array_index(&clients, mongoc_client_t *, 1u);
   BSON_ASSERT(s0 && s1);

   // Deprioritization cannot be deterministically asserted by this test due to
   // randomized selection from suitable servers. Repeat the test a few times to
   // increase the likelihood of detecting incorrect deprioritization behavior.
   for (int i = 0; i < 10; ++i) {
      // Configure the following fail point for both s0 and s1:
      {
         bson_t *const command = tmp_bson("{"
                                          "  'configureFailPoint': 'failCommand',"
                                          "  'mode': { 'times': 1 },"
                                          "  'data': {"
                                          "    'failCommands': ['find'],"
                                          "    'errorCode': 6"
                                          "  }"
                                          "}");

         ASSERT_OR_PRINT(mongoc_client_command_simple(s0, "admin", command, NULL, NULL, &error), error);
         ASSERT_OR_PRINT(mongoc_client_command_simple(s1, "admin", command, NULL, NULL, &error), error);
      }

      // Create a client client with `retryReads=true` that connects to the
      // cluster with both mongoses used by `s0` and `s1` in the initial seed
      // list.
      mongoc_client_t *client = NULL;
      {
         const char *const host_and_port = "mongodb://localhost:27017,localhost:27018/?retryReads=true";
         char *const uri_str = test_framework_add_user_password_from_env(host_and_port);
         mongoc_uri_t *const uri = mongoc_uri_new(uri_str);

         client = mongoc_client_new_from_uri_with_error(uri, &error);
         ASSERT_OR_PRINT(client, error);
         test_framework_set_ssl_opts(client);

         mongoc_uri_destroy(uri);
         bson_free(uri_str);
      }
      BSON_ASSERT(client);

      {
         test_retry_reads_sharded_on_other_mongos_ctx ctx = {0};

         // Enable failed command event monitoring for client.
         {
            mongoc_apm_callbacks_t *const callbacks = mongoc_apm_callbacks_new();
            mongoc_apm_set_command_failed_cb(callbacks, _test_retry_reads_sharded_on_other_mongos_cb);
            mongoc_client_set_apm_callbacks(client, callbacks, &ctx);
            mongoc_apm_callbacks_destroy(callbacks);
         }

         // Execute a `find` command with `client`. Assert that the command
         // failed.
         {
            mongoc_database_t *const db = mongoc_client_get_database(client, "db");
            mongoc_collection_t *const coll = mongoc_database_get_collection(db, "test");
            mongoc_cursor_t *const cursor = mongoc_collection_find_with_opts(coll, tmp_bson("{}"), NULL, NULL);
            const bson_t *reply = NULL;
            ASSERT_WITH_MSG(!mongoc_cursor_next(cursor, &reply), "expected find command to fail");
            ASSERT_WITH_MSG(mongoc_cursor_error(cursor, &error), "expected find command to fail");
            mongoc_cursor_destroy(cursor);
            mongoc_collection_destroy(coll);
            mongoc_database_destroy(db);
         }

         // Assert that two failed command events occurred.
         ASSERT_WITH_MSG(ctx.count == 2,
                         "expected exactly 2 failpoints to trigger, but "
                         "observed %d with error: %s",
                         ctx.count,
                         error.message);

         // Assert that both events occurred on different mongoses.
         ASSERT_WITH_MSG((ctx.ports[0] == 27017 || ctx.ports[0] == 27018) &&
                            (ctx.ports[1] == 27017 || ctx.ports[1] == 27018) && (ctx.ports[0] != ctx.ports[1]),
                         "expected failpoints to trigger once on each mongos, "
                         "but observed failures on %d and %d",
                         ctx.ports[0],
                         ctx.ports[1]);

         mongoc_client_destroy(client);
      }

      // Disable the fail point on both s0 and s1.
      {
         bson_t *const command = tmp_bson("{"
                                          "  'configureFailPoint': 'failCommand',"
                                          "  'mode': 'off'"
                                          "}");

         ASSERT_OR_PRINT(mongoc_client_command_simple(s0, "admin", command, NULL, NULL, &error), error);
         ASSERT_OR_PRINT(mongoc_client_command_simple(s1, "admin", command, NULL, NULL, &error), error);
      }
   }

   mongoc_client_destroy(s0);
   mongoc_client_destroy(s1);
   _mongoc_array_destroy(&clients);
}

typedef struct _test_retry_reads_sharded_on_same_mongos_ctx {
   int failed_count;
   int succeeded_count;
   uint16_t failed_port;
   uint16_t succeeded_port;
} test_retry_reads_sharded_on_same_mongos_ctx;

static void
_test_retry_reads_sharded_on_same_mongos_cb(test_retry_reads_sharded_on_same_mongos_ctx *ctx,
                                            const mongoc_apm_command_failed_t *failed,
                                            const mongoc_apm_command_succeeded_t *succeeded)
{
   BSON_ASSERT_PARAM(ctx);
   BSON_OPTIONAL_PARAM(failed);
   BSON_OPTIONAL_PARAM(succeeded);

   ASSERT_WITH_MSG(ctx->failed_count + ctx->succeeded_count < 2,
                   "expected at most two events, but observed %d failed and %d succeeded",
                   ctx->failed_count,
                   ctx->succeeded_count);

   if (failed) {
      ctx->failed_count += 1;

      const mongoc_host_list_t *const host = mongoc_apm_command_failed_get_host(failed);
      BSON_ASSERT(host);
      BSON_ASSERT(!host->next);
      ctx->failed_port = host->port;
   }

   if (succeeded) {
      ctx->succeeded_count += 1;

      const mongoc_host_list_t *const host = mongoc_apm_command_succeeded_get_host(succeeded);
      BSON_ASSERT(host);
      BSON_ASSERT(!host->next);
      ctx->succeeded_port = host->port;
   }
}

static void
_test_retry_reads_sharded_on_same_mongos_failed_cb(const mongoc_apm_command_failed_t *event)
{
   _test_retry_reads_sharded_on_same_mongos_cb(mongoc_apm_command_failed_get_context(event), event, NULL);
}

static void
_test_retry_reads_sharded_on_same_mongos_succeeded_cb(const mongoc_apm_command_succeeded_t *event)
{
   _test_retry_reads_sharded_on_same_mongos_cb(mongoc_apm_command_succeeded_get_context(event), NULL, event);
}

// Retryable Reads Are Retried on the Same mongos if No Others are Available
static void
test_retry_reads_sharded_on_same_mongos(void *_ctx)
{
   BSON_UNUSED(_ctx);

   bson_error_t error = {0};

   // Create a client `s0` that connects to a single mongos from the cluster.
   const char *ports[] = {"27017"};
   const size_t num_ports = sizeof(ports) / sizeof(*ports);
   mongoc_array_t clients = _test_get_mongos_clients(ports, num_ports);
   BSON_ASSERT(clients.len == 1u);
   mongoc_client_t *const s0 = _mongoc_array_index(&clients, mongoc_client_t *, 0u);
   BSON_ASSERT(s0);

   // Configure the following fail point for `s0`:
   ASSERT_OR_PRINT(mongoc_client_command_simple(s0,
                                                "admin",
                                                tmp_bson("{"
                                                         "  'configureFailPoint': 'failCommand',"
                                                         "  'mode': { 'times': 1 },"
                                                         "  'data': {"
                                                         "    'failCommands': ['find'],"
                                                         "    'errorCode': 6"
                                                         "  }"
                                                         "}"),
                                                NULL,
                                                NULL,
                                                &error),
                   error);

   // Create a client client with `directConnection=false` (when not set by
   // default) and `retryReads=true` that connects to the cluster using the same
   // single mongos as `s0`.
   mongoc_client_t *client = NULL;
   {
      const char *const host_and_port = "mongodb://localhost:27017/?retryReads=true&directConnection=false";
      char *const uri_str = test_framework_add_user_password_from_env(host_and_port);
      mongoc_uri_t *const uri = mongoc_uri_new(uri_str);

      client = mongoc_client_new_from_uri_with_error(uri, &error);
      ASSERT_OR_PRINT(client, error);
      test_framework_set_ssl_opts(client);

      mongoc_uri_destroy(uri);
      bson_free(uri_str);
   }
   BSON_ASSERT(client);

   {
      test_retry_reads_sharded_on_same_mongos_ctx ctx = {
         .failed_count = 0,
         .succeeded_count = 0,
      };

      // Enable succeeded and failed command event monitoring for `client`.
      {
         mongoc_apm_callbacks_t *const callbacks = mongoc_apm_callbacks_new();
         mongoc_apm_set_command_failed_cb(callbacks, _test_retry_reads_sharded_on_same_mongos_failed_cb);
         mongoc_apm_set_command_succeeded_cb(callbacks, _test_retry_reads_sharded_on_same_mongos_succeeded_cb);
         mongoc_client_set_apm_callbacks(client, callbacks, &ctx);
         mongoc_apm_callbacks_destroy(callbacks);
      }

      // Execute a `find` command with `client`. Assert that the command
      // succeeded.
      {
         mongoc_database_t *const db = mongoc_client_get_database(client, "db");
         mongoc_collection_t *const coll = mongoc_database_get_collection(db, "test");
         bson_t opts = BSON_INITIALIZER;
         {
            // Ensure drop from earlier is observed.
            mongoc_read_concern_t *const rc = mongoc_read_concern_new();
            mongoc_read_concern_set_level(rc, MONGOC_READ_CONCERN_LEVEL_MAJORITY);
            mongoc_read_concern_append(rc, &opts);
            mongoc_read_concern_destroy(rc);
         }
         mongoc_cursor_t *const cursor = mongoc_collection_find_with_opts(coll, &opts, NULL, NULL);
         const bson_t *reply = NULL;
         (void)mongoc_cursor_next(cursor, &reply);
         ASSERT_WITH_MSG(
            !mongoc_cursor_error(cursor, &error), "expecting find to succeed, but observed error: %s", error.message);
         mongoc_cursor_destroy(cursor);
         bson_destroy(&opts);
         mongoc_collection_destroy(coll);
         mongoc_database_destroy(db);
      }

      // Avoid capturing additional events.
      mongoc_client_set_apm_callbacks(client, NULL, NULL);

      // Assert that exactly one failed command event and one succeeded command
      // event occurred.
      ASSERT_WITH_MSG(ctx.failed_count == 1 && ctx.succeeded_count == 1,
                      "expected exactly one failed event and one succeeded "
                      "event, but observed %d failures and %d successes with error: %s",
                      ctx.failed_count,
                      ctx.succeeded_count,
                      ctx.succeeded_count > 1 ? "none" : error.message);

      //  Assert that both events occurred on the same mongos.
      ASSERT_WITH_MSG(ctx.failed_port == ctx.succeeded_port,
                      "expected failed and succeeded events on the same mongos, but "
                      "instead observed port %d (failed) and port %d (succeeded)",
                      ctx.failed_port,
                      ctx.succeeded_port);

      mongoc_client_destroy(client);
   }

   // Disable the fail point on s0.
   ASSERT_OR_PRINT(mongoc_client_command_simple(s0,
                                                "admin",
                                                tmp_bson("{"
                                                         "  'configureFailPoint': 'failCommand',"
                                                         "  'mode': 'off'"
                                                         "}"),
                                                NULL,
                                                NULL,
                                                &error),
                   error);

   mongoc_client_destroy(s0);
   _mongoc_array_destroy(&clients);
}

static void
run_admin_command(const char *cmd_str)
{
   bson_t *const cmd_bson = tmp_bson(cmd_str);

   bson_error_t error;
   mongoc_client_t *const client = test_framework_new_default_client();
   ASSERT_OR_PRINT(mongoc_client_command_simple(client, "admin", cmd_bson, NULL, NULL, &error), error);

   mongoc_client_destroy(client);
}

typedef struct {
   bool is_set;
   uint32_t id;
} server_id_maybe;

typedef struct {
   server_id_maybe server_id_failed;
   server_id_maybe server_id_succeeded;
} prose_test_3_apm_ctx;

static void
prose_test_3_on_command_failed(const mongoc_apm_command_failed_t *event)
{
   prose_test_3_apm_ctx *const ctx = (prose_test_3_apm_ctx *)mongoc_apm_command_failed_get_context(event);

   if (0 != strcmp(mongoc_apm_command_failed_get_command_name(event), "find")) {
      return;
   }

   ASSERT(!ctx->server_id_failed.is_set);

   ctx->server_id_failed.id = mongoc_apm_command_failed_get_server_id(event);
   ctx->server_id_failed.is_set = true;
}

static void
prose_test_3_on_command_succeeded(const mongoc_apm_command_succeeded_t *event)
{
   prose_test_3_apm_ctx *const ctx = (prose_test_3_apm_ctx *)mongoc_apm_command_succeeded_get_context(event);

   if (0 != strcmp(mongoc_apm_command_succeeded_get_command_name(event), "find")) {
      return;
   }

   ASSERT(!ctx->server_id_succeeded.is_set);

   ctx->server_id_succeeded.id = mongoc_apm_command_succeeded_get_server_id(event);
   ctx->server_id_succeeded.is_set = true;
}

static mongoc_uri_t *
make_prose_test_3_uri(void)
{
   mongoc_uri_t *const uri = test_framework_get_uri();

   mongoc_uri_set_option_as_bool(uri, MONGOC_URI_RETRYREADS, true);

   {
      mongoc_read_prefs_t *const read_prefs = mongoc_read_prefs_new(MONGOC_READ_PRIMARY_PREFERRED);
      mongoc_uri_set_read_prefs_t(uri, read_prefs);
      mongoc_read_prefs_destroy(read_prefs);
   }

   return uri;
}

static void
test_retryable_reads_prose_3_steps_1_to_5(const char *fail_point_cmd_str,
                                          mongoc_uri_t *uri, // Owning.
                                          prose_test_3_apm_ctx *apm_ctx)
{
   BSON_ASSERT_PARAM(uri);

   // Step 1: Create a client with the provided URI and command event monitoring enabled.
   mongoc_client_t *const client = test_framework_client_new_from_uri(uri, NULL);
   mongoc_uri_destroy(uri);

   test_framework_set_ssl_opts(client);

   // Step 2: Configure the provided fail point for `client`:
   run_admin_command(fail_point_cmd_str);

   // Step 3: Reset the command event monitor to clear the failpoint command from its stored events.
   apm_ctx->server_id_failed.is_set = false;
   apm_ctx->server_id_succeeded.is_set = false;
   {
      mongoc_apm_callbacks_t *const callbacks = mongoc_apm_callbacks_new();
      mongoc_apm_set_command_failed_cb(callbacks, prose_test_3_on_command_failed);
      mongoc_apm_set_command_succeeded_cb(callbacks, prose_test_3_on_command_succeeded);

      mongoc_client_set_apm_callbacks(client, callbacks, apm_ctx);

      mongoc_apm_callbacks_destroy(callbacks);
   }

   // Step 4: Execute a `find` command with `client`.
   {
      mongoc_collection_t *const coll = get_test_collection(client, "retryable_reads");
      mongoc_cursor_t *const cursor = mongoc_collection_find_with_opts(coll, tmp_bson("{}"), NULL, NULL);

      const bson_t *doc;
      while (mongoc_cursor_next(cursor, &doc))
         ;

      bson_error_t error;
      ASSERT(!mongoc_cursor_error(cursor, &error));

      mongoc_cursor_destroy(cursor);
      mongoc_collection_destroy(coll);
   }

   // Step 5: Assert that one failed command event and one successful command event occurred.
   ASSERT(apm_ctx->server_id_failed.is_set);
   ASSERT(apm_ctx->server_id_succeeded.is_set);

   mongoc_client_destroy(client);
}

// Retryable Reads Caused by Overload Errors Are Retried on a Different Replicaset Server When One is Available and
// enableOverloadRetargeting is enabled.
static void
test_retryable_reads_prose_3_1(void *ctx)
{
   BSON_UNUSED(ctx);

   mongoc_uri_t *const uri = make_prose_test_3_uri();
   mongoc_uri_set_option_as_bool(uri, MONGOC_URI_ENABLEOVERLOADRETARGETING, true);

   prose_test_3_apm_ctx apm_ctx = {0};

   test_retryable_reads_prose_3_steps_1_to_5(
      BSON_STR({
         "configureFailPoint" : "failCommand",
         "mode" : {"times" : 1},
         "data" :
            {"failCommands" : ["find"], "errorLabels" : [ "RetryableError", "SystemOverloadedError" ], "errorCode" : 6}
      }),
      uri, // Ownership transfer.
      &apm_ctx);

   // Step 6: Assert that both events occurred on different servers.
   ASSERT(apm_ctx.server_id_failed.id != apm_ctx.server_id_succeeded.id);
}

// Retryable Reads Caused by Non-Overload Errors Are Retried on the Same Replicaset Server.
static void
test_retryable_reads_prose_3_2(void *ctx)
{
   BSON_UNUSED(ctx);

   prose_test_3_apm_ctx apm_ctx = {0};

   test_retryable_reads_prose_3_steps_1_to_5(
      BSON_STR({
         "configureFailPoint" : "failCommand",
         "mode" : {"times" : 1},
         "data" : {"failCommands" : ["find"], "errorLabels" : ["RetryableError"], "errorCode" : 6}
      }),
      make_prose_test_3_uri(), // Ownership transfer.
      &apm_ctx);

   // Step 6: Assert that both events occurred on the same server.
   ASSERT(apm_ctx.server_id_failed.id == apm_ctx.server_id_succeeded.id);
}

// Retryable Reads Caused by Overload Errors Are Retried on the Same Replicaset Server When enableOverloadRetargeting
// is disabled.
static void
test_retryable_reads_prose_3_3(void *ctx)
{
   BSON_UNUSED(ctx);

   prose_test_3_apm_ctx apm_ctx = {0};

   test_retryable_reads_prose_3_steps_1_to_5(
      BSON_STR({
         "configureFailPoint" : "failCommand",
         "mode" : {"times" : 1},
         "data" :
            {"failCommands" : ["find"], "errorLabels" : [ "RetryableError", "SystemOverloadedError" ], "errorCode" : 6}
      }),
      make_prose_test_3_uri(), // Ownership transfer
      &apm_ctx);

   // Step 6: Assert that both events occurred on the same server.
   ASSERT(apm_ctx.server_id_failed.id == apm_ctx.server_id_succeeded.id);
}

typedef struct {
   const char *second_failpoint_cmd;
   int find_commands_started_count;
} prose_test_4_ctx_t;

static void
prose_test_4_on_command_started(const mongoc_apm_command_started_t *event)
{
   prose_test_4_ctx_t *const ctx = (prose_test_4_ctx_t *)mongoc_apm_command_started_get_context(event);

   if (0 == strcmp(mongoc_apm_command_started_get_command_name(event), "find")) {
      ctx->find_commands_started_count++;
   }
}

static void
prose_test_4_on_command_failed(const mongoc_apm_command_failed_t *event)
{
   prose_test_4_ctx_t *const ctx = (prose_test_4_ctx_t *)mongoc_apm_command_failed_get_context(event);

   if (0 != strcmp(mongoc_apm_command_failed_get_command_name(event), "find")) {
      return;
   }

   // Configure the second fail point command only if the failed event is for the first error configured in step 2.
   if (ctx->second_failpoint_cmd) {
      run_admin_command(ctx->second_failpoint_cmd);
      ctx->second_failpoint_cmd = NULL;
   }
}

// Test that drivers set the maximum number of retries for all retryable read errors when an overload error is
// encountered.
static void
test_retryable_reads_prose_4(void *unused)
{
   BSON_UNUSED(unused);

   // Step 1: Create a client.
   mongoc_client_t *const client = test_framework_new_default_client();

   // Step 2: Configure a fail point with error code 91 (`ShutdownInProgress`) with the `RetryableError` and
   // `SystemOverloadedError` error labels.
   run_admin_command(BSON_STR({
      "configureFailPoint" : "failCommand",
      "mode" : {"times" : 1},
      "data" :
         {"failCommands" : ["find"], "errorLabels" : [ "RetryableError", "SystemOverloadedError" ], "errorCode" : 91}
   }));

   // Step 3: Via the command monitoring CommandFailedEvent, configure a fail point with error code 91
   // (`ShutdownInProgress`) and the `RetryableError` label. Configure the second fail point command only if the failed
   // event is for the first error configured in step 2.
   prose_test_4_ctx_t apm_ctx = {
      .second_failpoint_cmd = BSON_STR({
         "configureFailPoint" : "failCommand",
         "mode" : "alwaysOn",
         "data" : {"failCommands" : ["find"], "errorLabels" : ["RetryableError"], "errorCode" : 91}
      }),
      .find_commands_started_count = 0,
   };

   {
      mongoc_apm_callbacks_t *const callbacks = mongoc_apm_callbacks_new();
      mongoc_apm_set_command_started_cb(callbacks, prose_test_4_on_command_started);
      mongoc_apm_set_command_failed_cb(callbacks, prose_test_4_on_command_failed);
      mongoc_client_set_apm_callbacks(client, callbacks, &apm_ctx);
      mongoc_apm_callbacks_destroy(callbacks);
   }

   // Step 4: Attempt a `findOne` operation on any record for any database and collection. Expect the `findOne` to fail
   // with a server error.
   {
      mongoc_collection_t *const coll = get_test_collection(client, "retryable_reads");
      mongoc_cursor_t *const cursor = mongoc_collection_find_with_opts(coll, tmp_bson("{}"), NULL, NULL);

      const bson_t *doc;
      ASSERT(!mongoc_cursor_next(cursor, &doc));

      bson_error_t error;
      ASSERT(mongoc_cursor_error(cursor, &error));

      mongoc_cursor_destroy(cursor);
      mongoc_collection_destroy(coll);
   }

   // Assert that `MONGOC_DEFAULT_MAXADAPTIVERETRIES + 1` attempts were made.
   ASSERT_CMPINT(apm_ctx.find_commands_started_count, ==, MONGOC_DEFAULT_MAXADAPTIVERETRIES + 1);

   // Step 5: Disable the fail point.
   run_admin_command(BSON_STR({"configureFailPoint" : "failCommand", "mode" : "off"}));

   mongoc_client_destroy(client);
}

typedef struct {
   int count;
} backoff_counter_t;

static double
backoff_counting_jitter_source_generate(mongoc_jitter_source_t *source)
{
   backoff_counter_t *const counter = (backoff_counter_t *)_mongoc_jitter_source_get_context(source);
   ++counter->count;
   return 0.0;
}

typedef struct {
   const char *second_failpoint_cmd;
} prose_test_5_ctx_t;

static void
prose_test_5_on_command_failed(const mongoc_apm_command_failed_t *event)
{
   prose_test_5_ctx_t *const ctx = (prose_test_5_ctx_t *)mongoc_apm_command_failed_get_context(event);

   if (0 != strcmp(mongoc_apm_command_failed_get_command_name(event), "find")) {
      return;
   }

   // Configure the second fail point command only if the failed event is for the first error configured in step 2.
   if (ctx->second_failpoint_cmd) {
      run_admin_command(ctx->second_failpoint_cmd);
      ctx->second_failpoint_cmd = NULL;
   }
}

// Test that drivers do not apply backoff to non-overload errors.
static void
test_retryable_reads_prose_5(void *unused)
{
   BSON_UNUSED(unused);


   // Step 1: Create a client.
   mongoc_client_t *const client = test_framework_new_default_client();

   backoff_counter_t backoff_counter = {0};
   {
      mongoc_jitter_source_t *const jitter_source = _mongoc_jitter_source_new(backoff_counting_jitter_source_generate);
      _mongoc_jitter_source_set_context(jitter_source, &backoff_counter);
      _mongoc_client_set_jitter_source(client, jitter_source);
   }

   // Step 3: Via the command monitoring CommandFailedEvent, configure a fail point with error code 91
   // (`ShutdownInProgress`) and the `RetryableError` label. Configure the second fail point command only if the failed
   // event is for the first error configured in step 2.
   prose_test_5_ctx_t ctx = {
      .second_failpoint_cmd = BSON_STR({
         "configureFailPoint" : "failCommand",
         "mode" : "alwaysOn",
         "data" : {"failCommands" : ["find"], "errorLabels" : ["RetryableError"], "errorCode" : 91}
      }),
   };

   {
      mongoc_apm_callbacks_t *const callbacks = mongoc_apm_callbacks_new();
      mongoc_apm_set_command_failed_cb(callbacks, prose_test_5_on_command_failed);
      mongoc_client_set_apm_callbacks(client, callbacks, &ctx);
      mongoc_apm_callbacks_destroy(callbacks);
   }

   // Step 2: Configure a fail point with error code 91 (`ShutdownInProgress`) with the `RetryableError` and
   // `SystemOverloadedError` error labels.
   run_admin_command(BSON_STR({
      "configureFailPoint" : "failCommand",
      "mode" : {"times" : 1},
      "data" :
         {"failCommands" : ["find"], "errorLabels" : [ "RetryableError", "SystemOverloadedError" ], "errorCode" : 91}
   }));

   // Step 4: Attempt a findOne operation on any record for any database and collection. Expect the findOne to fail with
   // a server error.
   {
      mongoc_collection_t *const coll = get_test_collection(client, "retryable_reads");
      mongoc_cursor_t *const cursor = mongoc_collection_find_with_opts(coll, tmp_bson("{}"), NULL, NULL);

      const bson_t *doc;
      ASSERT(!mongoc_cursor_next(cursor, &doc));

      bson_error_t error;
      ASSERT(mongoc_cursor_error(cursor, &error));

      mongoc_cursor_destroy(cursor);
      mongoc_collection_destroy(coll);
   }
   // Assert that backoff was applied only once for the initial overload error and not for the subsequent non-overload
   // retryable errors. The jitter source is only invoked when backoff is actually applied (via
   // _mongoc_retry_backoff_generator_next), not when it is skipped (via _mongoc_retry_backoff_generator_skip).
   ASSERT_CMPINT(backoff_counter.count, ==, 1);

   // Step 5: Disable the fail point.
   run_admin_command(BSON_STR({"configureFailPoint" : "failCommand", "mode" : "off"}));

   mongoc_client_destroy(client);
}

/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for retryable reads.
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests(TestSuite *suite)
{
   install_json_test_suite_with_check(suite,
                                      JSON_DIR,
                                      "retryable_reads/legacy",
                                      test_retryable_reads_cb,
                                      TestSuite_CheckLive,
                                      test_framework_skip_if_no_failpoint,
                                      test_framework_skip_if_slow);
}

void
test_retryable_reads_install(TestSuite *suite)
{
   test_all_spec_tests(suite);
   TestSuite_AddFull(suite,
                     "/retryable_reads/cmd_helpers [lock:live-server]",
                     test_cmd_helpers,
                     NULL,
                     NULL,
                     test_framework_skip_if_mongos,
                     test_framework_skip_if_no_failpoint);
   TestSuite_AddFull(suite,
                     "/retryable_reads/retry_off [lock:live-server]",
                     test_retry_reads_off,
                     NULL,
                     NULL,
                     test_framework_skip_if_mongos,
                     test_framework_skip_if_no_failpoint);
   TestSuite_AddFull(suite,
                     "/retryable_reads/sharded/on_other_mongos [lock:live-server]",
                     test_retry_reads_sharded_on_other_mongos,
                     NULL,
                     NULL,
                     test_framework_skip_if_not_mongos,
                     test_framework_skip_if_no_failpoint);
   TestSuite_AddFull(suite,
                     "/retryable_reads/sharded/on_same_mongos [lock:live-server]",
                     test_retry_reads_sharded_on_same_mongos,
                     NULL,
                     NULL,
                     test_framework_skip_if_not_mongos,
                     test_framework_skip_if_no_failpoint);
   TestSuite_AddFull(suite,
                     "/retryable_reads/prose_test_3_1",
                     test_retryable_reads_prose_3_1,
                     NULL,
                     NULL,
                     test_framework_skip_if_not_replset_with_secondary,
                     test_framework_skip_if_max_wire_version_less_than_9 /* require 4.4+ */);
   TestSuite_AddFull(suite,
                     "/retryable_reads/prose_test_3_2",
                     test_retryable_reads_prose_3_2,
                     NULL,
                     NULL,
                     test_framework_skip_if_not_replset_with_secondary,
                     test_framework_skip_if_max_wire_version_less_than_9 /* require 4.4+ */);
   TestSuite_AddFull(suite,
                     "/retryable_reads/prose_test_3_3",
                     test_retryable_reads_prose_3_3,
                     NULL,
                     NULL,
                     test_framework_skip_if_not_replset_with_secondary,
                     test_framework_skip_if_max_wire_version_less_than_9 /* require 4.4+ */);
   TestSuite_AddFull(suite,
                     "/retryable_reads/prose_test_4",
                     test_retryable_reads_prose_4,
                     NULL,
                     NULL,
                     test_framework_skip_if_max_wire_version_less_than_9 /* require 4.4+ */);
   TestSuite_AddFull(suite,
                     "/retryable_reads/prose_test_5",
                     test_retryable_reads_prose_5,
                     NULL,
                     NULL,
                     test_framework_skip_if_max_wire_version_less_than_9 /* require 4.4+ */);
}
