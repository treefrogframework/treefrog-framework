#include <mongoc/mongoc-client-private.h>

#include <mongoc/mongoc.h>

#include <mlib/loop.h>

#include <TestSuite.h>
#include <mock_server/future-functions.h>
#include <mock_server/future.h>
#include <mock_server/mock-server.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

static void
_test_query_flag(mongoc_query_flags_t flag, bson_t *opt)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   future_t *future;
   request_t *request;
   const bson_t *doc;

   server = mock_server_with_auto_hello(WIRE_VERSION_MAX);
   mock_server_run(server);
   client = test_framework_client_new_from_uri(mock_server_get_uri(server), NULL);
   collection = mongoc_client_get_collection(client, "db", "collection");
   cursor = mongoc_collection_aggregate(collection, flag, tmp_bson("{'pipeline': []}"), opt, NULL);

   ASSERT_OR_PRINT(!mongoc_cursor_error(cursor, &error), error);

   /* "aggregate" command */
   future = future_cursor_next(cursor, &doc);
   request = mock_server_receives_msg(server,
                                      MONGOC_QUERY_NONE,
                                      tmp_bson("{'aggregate': 'collection',"
                                               " 'pipeline': [ ],"
                                               " 'tailable': {'$exists': false}}"));
   ASSERT(request);
   reply_to_request_simple(request,
                           "{'ok': 1,"
                           " 'cursor': {"
                           "    'id': {'$numberLong': '123'},"
                           "    'ns': 'db.collection',"
                           "    'nextBatch': [{}]}}");
   ASSERT(future_get_bool(future));
   request_destroy(request);
   future_destroy(future);

   /* "getMore" command */
   future = future_cursor_next(cursor, &doc);
   request = mock_server_receives_msg(server,
                                      MONGOC_QUERY_NONE,
                                      tmp_bson("{'getMore': {'$numberLong': '123'},"
                                               " 'collection': 'collection',"
                                               " 'tailable': {'$exists': false}}"));
   ASSERT(request);
   reply_to_request_simple(request,
                           "{'ok': 1,"
                           " 'cursor': {"
                           "    'id': {'$numberLong': '0'},"
                           "    'ns': 'db.collection',"
                           "    'nextBatch': [{}]}}");

   ASSERT(future_get_bool(future));

   request_destroy(request);
   future_destroy(future);
   mongoc_cursor_destroy(cursor);
   mongoc_collection_destroy(collection);
   mongoc_client_destroy(client);
   mock_server_destroy(server);
}

static void
test_query_flags(void)
{
   typedef struct {
      mongoc_query_flags_t flag;
      bson_t *opt;
   } flag_and_opt_t;

   flag_and_opt_t flags_and_opts[] = {
      {MONGOC_QUERY_TAILABLE_CURSOR, tmp_bson("{'tailable': true}")},
      {MONGOC_QUERY_TAILABLE_CURSOR | MONGOC_QUERY_AWAIT_DATA, tmp_bson("{'tailable': true, 'awaitData': true}")}};

   /* test with both flag and opt */
   mlib_foreach_arr (flag_and_opt_t, opts, flags_and_opts) {
      _test_query_flag(opts->flag, NULL);
      _test_query_flag(MONGOC_QUERY_NONE, opts->opt);
   }
}

typedef struct {
   bson_t *cmd;
   bson_mutex_t lock;
} last_captured_t;

static void
command_started(const mongoc_apm_command_started_t *event)
{
   const bson_t *cmd = mongoc_apm_command_started_get_command(event);
   last_captured_t *lc = mongoc_apm_command_started_get_context(event);
   bson_mutex_lock(&lc->lock);
   bson_destroy(lc->cmd);
   lc->cmd = bson_copy(cmd);
   bson_mutex_unlock(&lc->lock);
}

// `test_write_respects_read_prefs` tests that an aggregate with a write stage respects the original read preferences
// when talking to >= 5.0 servers. This is a regression test for CDRIVER-5707.
static void
test_write_respects_read_prefs(void *unused)
{
   BSON_UNUSED(unused);

   bson_error_t error;
   last_captured_t lc = {0};
   bson_mutex_init(&lc.lock);

   mongoc_client_pool_t *pool = test_framework_new_default_client_pool();
   // Capture the most recent command-started event.
   {
      mongoc_apm_callbacks_t *cbs = mongoc_apm_callbacks_new();
      mongoc_apm_set_command_started_cb(cbs, command_started);
      mongoc_client_pool_set_apm_callbacks(pool, cbs, &lc);
      mongoc_apm_callbacks_destroy(cbs);
   }

   // Create database 'db' on separate client to avoid "database 'db' not found" error.
   {
      mongoc_client_t *client = test_framework_new_default_client();
      mongoc_collection_t *coll = mongoc_client_get_collection(client, "db", "coll");
      ASSERT_OR_PRINT(mongoc_collection_insert_one(coll, tmp_bson(BSON_STR({"x" : 1})), NULL, NULL, &error), error);
      mongoc_collection_destroy(coll);
      mongoc_client_destroy(client);
   }

   // Do an 'aggregate' with '$out'.
   {
      bson_t *pipeline = tmp_bson(BSON_STR({"pipeline" : [ {"$out" : "foo"} ]}));
      mongoc_client_t *client = mongoc_client_pool_pop(pool);
      mongoc_collection_t *coll = mongoc_client_get_collection(client, "db", "coll");
      mongoc_read_prefs_t *rp = mongoc_read_prefs_new(MONGOC_READ_SECONDARY_PREFERRED);
      mongoc_cursor_t *cursor = mongoc_collection_aggregate(coll, MONGOC_QUERY_NONE, pipeline, NULL /* opts */, rp);
      // Iterate cursor to send `aggregate` command.
      const bson_t *ignored;
      ASSERT(!mongoc_cursor_next(cursor, &ignored));
      ASSERT_OR_PRINT(!mongoc_cursor_error(cursor, &error), error);
      mongoc_read_prefs_destroy(rp);
      mongoc_cursor_destroy(cursor);
      mongoc_collection_destroy(coll);
      mongoc_client_pool_push(pool, client);
   }

   // Check that `aggregate` command contains $readPreference.
   {
      bson_t *got;
      bson_mutex_lock(&lc.lock);
      got = bson_copy(lc.cmd);
      bson_mutex_unlock(&lc.lock);
      ASSERT_MATCH(got, BSON_STR({"$readPreference" : {"mode" : "secondaryPreferred"}}));
      bson_destroy(got);
   }

   mongoc_client_pool_destroy(pool);
   bson_destroy(lc.cmd);
   bson_mutex_destroy(&lc.lock);
}

void
test_aggregate_install(TestSuite *suite)
{
   TestSuite_AddMockServerTest(suite, "/Aggregate/query_flags", test_query_flags);
   TestSuite_AddFull(suite,
                     "/Aggregate/write_respects_read_prefs [lock:live-server]",
                     test_write_respects_read_prefs,
                     NULL,
                     NULL,
                     test_framework_skip_if_single /* $readPreference is not sent for single servers */,
                     test_framework_skip_if_max_wire_version_less_than_13 /* require server 5.0+ */);
}
