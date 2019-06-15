#include <mongoc-util-private.h>
#include <mongoc.h>

#include "mongoc-client-private.h"
#include "mongoc-uri-private.h"

#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"


#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "cluster-test"


static uint32_t
server_id_for_reads (mongoc_cluster_t *cluster)
{
   bson_error_t error;
   mongoc_server_stream_t *server_stream;
   uint32_t id;

   server_stream = mongoc_cluster_stream_for_reads (cluster, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;

   mongoc_server_stream_cleanup (server_stream);

   return id;
}


static void
test_get_max_bson_obj_size (void)
{
   mongoc_server_description_t *sd;
   mongoc_cluster_node_t *node;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   int32_t max_bson_obj_size = 16;
   uint32_t id;

   /* single-threaded */
   client = test_framework_client_new ();
   BSON_ASSERT (client);

   id = server_id_for_reads (&client->cluster);
   sd = (mongoc_server_description_t *) mongoc_set_get (
      client->topology->description.servers, id);
   sd->max_bson_obj_size = max_bson_obj_size;
   BSON_ASSERT (max_bson_obj_size ==
                mongoc_cluster_get_max_bson_obj_size (&client->cluster));

   mongoc_client_destroy (client);

   /* multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   id = server_id_for_reads (&client->cluster);
   node = (mongoc_cluster_node_t *) mongoc_set_get (client->cluster.nodes, id);
   node->max_bson_obj_size = max_bson_obj_size;
   BSON_ASSERT (max_bson_obj_size ==
                mongoc_cluster_get_max_bson_obj_size (&client->cluster));

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}

static void
test_get_max_msg_size (void)
{
   mongoc_server_description_t *sd;
   mongoc_cluster_node_t *node;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   int32_t max_msg_size = 32;
   uint32_t id;

   /* single-threaded */
   client = test_framework_client_new ();
   id = server_id_for_reads (&client->cluster);

   sd = (mongoc_server_description_t *) mongoc_set_get (
      client->topology->description.servers, id);
   sd->max_msg_size = max_msg_size;
   BSON_ASSERT (max_msg_size ==
                mongoc_cluster_get_max_msg_size (&client->cluster));

   mongoc_client_destroy (client);

   /* multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   id = server_id_for_reads (&client->cluster);
   node = (mongoc_cluster_node_t *) mongoc_set_get (client->cluster.nodes, id);
   node->max_msg_size = max_msg_size;
   BSON_ASSERT (max_msg_size ==
                mongoc_cluster_get_max_msg_size (&client->cluster));

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}


#define ASSERT_CURSOR_ERR()                                  \
   do {                                                      \
      BSON_ASSERT (!future_get_bool (future));               \
      BSON_ASSERT (mongoc_cursor_error (cursor, &error));    \
      ASSERT_ERROR_CONTAINS (                                \
         error,                                              \
         MONGOC_ERROR_STREAM,                                \
         MONGOC_ERROR_STREAM_SOCKET,                         \
         "Failed to read 4 bytes: socket error or timeout"); \
   } while (0)


#define START_QUERY(client_port_variable)                               \
   do {                                                                 \
      cursor = mongoc_collection_find_with_opts (                       \
         collection, tmp_bson ("{}"), NULL, NULL);                      \
      future = future_cursor_next (cursor, &doc);                       \
      request = mock_server_receives_query (                            \
         server, "test.test", MONGOC_QUERY_SLAVE_OK, 0, 0, "{}", NULL); \
      client_port_variable = request_get_client_port (request);         \
   } while (0)


#define CLEANUP_QUERY()               \
   do {                               \
      request_destroy (request);      \
      future_destroy (future);        \
      mongoc_cursor_destroy (cursor); \
   } while (0)


/* test that we reconnect a cluster node after disconnect */
static void
_test_cluster_node_disconnect (bool pooled)
{
   mock_server_t *server;
   const int32_t socket_timeout_ms = 100;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   const bson_t *doc;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   uint16_t client_port_0, client_port_1;
   bson_error_t error;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   capture_logs (true);

   server = mock_server_with_autoismaster (WIRE_VERSION_MIN);
   mock_server_run (server);

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "socketTimeoutMS", socket_timeout_ms);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   collection = mongoc_client_get_collection (client, "test", "test");

   /* query 0 fails. set client_port_0 to the port used by the query. */
   START_QUERY (client_port_0);

   mock_server_resets (request);
   ASSERT_CURSOR_ERR ();
   CLEANUP_QUERY ();

   /* query 1 opens a new socket. set client_port_1 to the new port. */
   START_QUERY (client_port_1);
   ASSERT_CMPINT (client_port_1, !=, client_port_0);
   mock_server_replies_simple (request, "{'a': 1}");

   /* success! */
   BSON_ASSERT (future_get_bool (future));

   CLEANUP_QUERY ();
   mongoc_collection_destroy (collection);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
test_cluster_node_disconnect_single (void *ctx)
{
   _test_cluster_node_disconnect (false);
}


static void
test_cluster_node_disconnect_pooled (void *ctx)
{
   _test_cluster_node_disconnect (true);
}


static void
_test_cluster_command_timeout (bool pooled)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bson_error_t error;
   future_t *future;
   request_t *request;
   uint16_t client_port;
   mongoc_server_description_t *sd;
   bson_t reply;

   capture_logs (true);

   server = mock_server_with_autoismaster (WIRE_VERSION_MIN);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "socketTimeoutMS", 200);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   /* server doesn't respond in time */
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'foo': 1}"), NULL, NULL, &error);
   request =
      mock_server_receives_command (server, "db", MONGOC_QUERY_SLAVE_OK, NULL);
   client_port = request_get_client_port (request);

   ASSERT (!future_get_bool (future));
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_STREAM,
      MONGOC_ERROR_STREAM_SOCKET,
      "Failed to send \"foo\" command with database \"db\"");

   /* a network timeout does NOT invalidate the server description */
   sd = mongoc_topology_server_by_id (client->topology, 1, NULL);
   BSON_ASSERT (sd->type != MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_destroy (sd);

   /* late response */
   mock_server_replies_simple (request, "{'ok': 1, 'bar': 1}");
   request_destroy (request);
   future_destroy (future);

   future = future_client_command_simple (
      client, "db", tmp_bson ("{'baz': 1}"), NULL, &reply, &error);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'baz': 1}");
   ASSERT (request);
   /* new socket */
   ASSERT_CMPUINT16 (client_port, !=, request_get_client_port (request));
   mock_server_replies_simple (request, "{'ok': 1, 'quux': 1}");
   ASSERT (future_get_bool (future));

   /* got the proper response */
   ASSERT_HAS_FIELD (&reply, "quux");

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   bson_destroy (&reply);
   request_destroy (request);
   future_destroy (future);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
test_cluster_command_timeout_single (void)
{
   _test_cluster_command_timeout (false);
}


static void
test_cluster_command_timeout_pooled (void)
{
   _test_cluster_command_timeout (true);
}


static void
_test_write_disconnect (void)
{
   mock_server_t *server;
   char *ismaster_response;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;
   future_t *future;
   request_t *request;
   mongoc_topology_scanner_node_t *scanner_node;
   mongoc_server_description_t *sd;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_new ();
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));

   /*
    * establish connection with an "ismaster" and "ping"
    */
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);
   request = mock_server_receives_ismaster (server);
   ismaster_response = bson_strdup_printf ("{'ok': 1.0,"
                                           " 'ismaster': true,"
                                           " 'minWireVersion': 2,"
                                           " 'maxWireVersion': 3}");

   mock_server_replies_simple (request, ismaster_response);
   request_destroy (request);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'ping': 1}");
   mock_server_replies_simple (request, "{'ok': 1}");
   ASSERT_OR_PRINT (future_get_bool (future), error);

   /*
    * close the socket
    */
   mock_server_hangs_up (request);

   /*
    * next operation detects the hangup
    */
   collection = mongoc_client_get_collection (client, "db", "collection");
   future_destroy (future);
   future = future_collection_insert_one (
      collection, tmp_bson ("{'_id': 1}"), NULL, NULL, &error);

   ASSERT (!future_get_bool (future));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_STREAM);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_STREAM_SOCKET);

   scanner_node = mongoc_topology_scanner_get_node (client->topology->scanner,
                                                    1 /* server_id */);
   ASSERT (scanner_node && !scanner_node->stream);

   /* a hangup DOES invalidate the server description */
   sd = mongoc_topology_server_by_id (client->topology, 1, NULL);
   BSON_ASSERT (sd->type == MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_destroy (sd);

   mongoc_collection_destroy (collection);
   request_destroy (request);
   future_destroy (future);
   bson_free (ismaster_response);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_write_command_disconnect (void *ctx)
{
   _test_write_disconnect ();
}


typedef struct {
   int calls;
   bson_t *cluster_time;
   bson_t *command;
} cluster_time_test_t;


static void
test_cluster_time_cmd_started_cb (const mongoc_apm_command_started_t *event)
{
   const bson_t *cmd;
   cluster_time_test_t *test;
   bson_iter_t iter;
   bson_t client_cluster_time;

   cmd = mongoc_apm_command_started_get_command (event);
   if (!strcmp (_mongoc_get_command_name (cmd), "killCursors")) {
      /* ignore killCursors */
      return;
   }

   test =
      (cluster_time_test_t *) mongoc_apm_command_started_get_context (event);

   test->calls++;
   _mongoc_bson_destroy_if_set (test->command);
   test->command = bson_copy (cmd);

   /* Only a MongoDB 3.6+ server reports $clusterTime. If we've received a
    * $clusterTime, we send it to any server. In this case, we got a
    * $clusterTime during the initial handshake. */
   if (test_framework_clustertime_supported ()) {
      BSON_ASSERT (bson_iter_init_find (&iter, cmd, "$clusterTime"));
      BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));

      if (test->calls == 2) {
         /* previous call to cmd_succeeded_cb saved server's clusterTime */
         BSON_ASSERT (!bson_empty0 (test->cluster_time));
         bson_iter_bson (&iter, &client_cluster_time);
         if (!bson_equal (test->cluster_time, &client_cluster_time)) {
            fprintf (stderr,
                     "Unequal clusterTimes.\nServer sent %s\nClient sent %s\n",
                     bson_as_json (test->cluster_time, NULL),
                     bson_as_json (&client_cluster_time, NULL));

            abort ();
         }

         bson_destroy (&client_cluster_time);
      }
   } else {
      BSON_ASSERT (!bson_has_field (event->command, "$clusterTime"));
   }
}


static void
test_cluster_time_cmd_succeeded_cb (const mongoc_apm_command_succeeded_t *event)
{
   const bson_t *reply;
   cluster_time_test_t *test;
   bson_iter_t iter;
   uint32_t len;
   const uint8_t *data;

   reply = mongoc_apm_command_succeeded_get_reply (event);
   test =
      (cluster_time_test_t *) mongoc_apm_command_succeeded_get_context (event);

   /* Only a MongoDB 3.6+ server reports $clusterTime. Save it in "test". */
   if (test_framework_clustertime_supported ()) {
      BSON_ASSERT (bson_iter_init_find (&iter, reply, "$clusterTime"));
      BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
      bson_iter_document (&iter, &len, &data);
      _mongoc_bson_destroy_if_set (test->cluster_time);
      test->cluster_time = bson_new_from_data (data, len);
   }
}


typedef bool (*command_fn_t) (mongoc_client_t *, bson_error_t *);


/* test $clusterTime handling according to the test instructions in the
 * Driver Sessions Spec */
static void
_test_cluster_time (bool pooled, command_fn_t command)
{
   mongoc_apm_callbacks_t *callbacks;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bool r;
   bson_error_t error;
   cluster_time_test_t cluster_time_test;

   cluster_time_test.calls = 0;
   cluster_time_test.command = NULL;
   cluster_time_test.cluster_time = NULL;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks,
                                      test_cluster_time_cmd_started_cb);
   mongoc_apm_set_command_succeeded_cb (callbacks,
                                        test_cluster_time_cmd_succeeded_cb);

   if (pooled) {
      pool = test_framework_client_pool_new ();
      mongoc_client_pool_set_apm_callbacks (
         pool, callbacks, &cluster_time_test);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = test_framework_client_new ();
      mongoc_client_set_apm_callbacks (client, callbacks, &cluster_time_test);
   }

   r = command (client, &error);
   ASSERT_OR_PRINT (r, error);
   ASSERT_CMPINT (cluster_time_test.calls, ==, 1);

   /* repeat */
   r = command (client, &error);
   ASSERT_OR_PRINT (r, error);
   ASSERT_CMPINT (cluster_time_test.calls, ==, 2);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_apm_callbacks_destroy (callbacks);
   _mongoc_bson_destroy_if_set (cluster_time_test.command);
   _mongoc_bson_destroy_if_set (cluster_time_test.cluster_time);
}


static bool
command_simple (mongoc_client_t *client, bson_error_t *error)
{
   return mongoc_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, NULL, error);
}


static void
test_cluster_time_command_simple_single (void)
{
   _test_cluster_time (false, command_simple);
}


static void
test_cluster_time_command_simple_pooled (void)
{
   _test_cluster_time (true, command_simple);
}


/* test the deprecated mongoc_client_command function with $clusterTime */
static bool
client_command (mongoc_client_t *client, bson_error_t *error)
{
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bool r;

   cursor = mongoc_client_command (client,
                                   "test",
                                   MONGOC_QUERY_NONE,
                                   0,
                                   0,
                                   0,
                                   tmp_bson ("{'ping': 1}"),
                                   NULL,
                                   NULL);

   mongoc_cursor_next (cursor, &doc);
   r = !mongoc_cursor_error (cursor, error);
   mongoc_cursor_destroy (cursor);
   return r;
}


static void
test_cluster_time_command_single (void)
{
   _test_cluster_time (false, client_command);
}


static void
test_cluster_time_command_pooled (void)
{
   _test_cluster_time (true, client_command);
}


/* test modern mongoc_client_read_command_with_opts with $clusterTime */
static bool
client_command_with_opts (mongoc_client_t *client, bson_error_t *error)
{
   /* any of the with_opts command functions should work */
   return mongoc_client_read_command_with_opts (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, NULL, NULL, error);
}


static void
test_cluster_time_command_with_opts_single (void)
{
   _test_cluster_time (false, client_command_with_opts);
}


static void
test_cluster_time_command_with_opts_pooled (void)
{
   _test_cluster_time (true, client_command_with_opts);
}


/* test aggregate with $clusterTime */
static bool
aggregate (mongoc_client_t *client, bson_error_t *error)
{
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bool r;

   collection = mongoc_client_get_collection (client, "test", "collection");
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson ("{}"), NULL, NULL);

   mongoc_cursor_next (cursor, &doc);
   r = !mongoc_cursor_error (cursor, error);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);

   return r;
}


static void
test_cluster_time_aggregate_single (void)
{
   _test_cluster_time (false, aggregate);
}


static void
test_cluster_time_aggregate_pooled (void)
{
   _test_cluster_time (true, aggregate);
}


/* test queries with $clusterTime */
static bool
cursor_next (mongoc_client_t *client, bson_error_t *error)
{
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bool r;

   collection = get_test_collection (client, "test_cluster_time_cursor");
   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{'ping': 1}"), NULL, NULL);

   mongoc_cursor_next (cursor, &doc);
   r = !mongoc_cursor_error (cursor, error);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);

   return r;
}


static void
test_cluster_time_cursor_single (void)
{
   _test_cluster_time (false, cursor_next);
}


static void
test_cluster_time_cursor_pooled (void)
{
   _test_cluster_time (true, cursor_next);
}


/* test inserts with $clusterTime */
static bool
insert (mongoc_client_t *client, bson_error_t *error)
{
   mongoc_collection_t *collection;
   bool r;

   collection = get_test_collection (client, "test_cluster_time_cursor");
   r = mongoc_collection_insert_one (
      collection, tmp_bson ("{}"), NULL, NULL, error);

   mongoc_collection_destroy (collection);

   return r;
}


static void
test_cluster_time_insert_single (void)
{
   _test_cluster_time (false, insert);
}


static void
test_cluster_time_insert_pooled (void)
{
   _test_cluster_time (true, insert);
}


static void
replies_with_cluster_time (request_t *request,
                           int t,
                           int i,
                           const char *docs_json)
{
   char *quotes_replaced;
   bson_t doc;
   bson_error_t error;
   bool r;

   BSON_ASSERT (request);

   if (docs_json) {
      quotes_replaced = single_quotes_to_double (docs_json);
      r = bson_init_from_json (&doc, quotes_replaced, -1, &error);
      bson_free (quotes_replaced);
   } else {
      r = bson_init_from_json (&doc, "{}", -1, &error);
   }

   if (!r) {
      MONGOC_WARNING ("%s", error.message);
      return;
   }

   BSON_APPEND_DOCUMENT (
      &doc,
      "$clusterTime",
      tmp_bson ("{'clusterTime': {'$timestamp': {'t': %d, 'i': %d}}, 'x': 'y'}",
                t,
                i));

   mock_server_reply_multi (
      request, MONGOC_REPLY_NONE, &doc, 1, 0 /* cursor id */);

   bson_destroy (&doc);
   request_destroy (request);
}


static request_t *
receives_with_cluster_time (mock_server_t *server,
                            uint32_t timestamp,
                            uint32_t increment,
                            bson_t *command)
{
   request_t *request;
   const bson_t *doc;
   bson_iter_t cluster_time;
   uint32_t t;
   uint32_t i;

   request = mock_server_receives_msg (server, 0, command);
   BSON_ASSERT (request);
   doc = request_get_doc (request, 0);

   BSON_ASSERT (bson_iter_init_find (&cluster_time, doc, "$clusterTime"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&cluster_time));
   BSON_ASSERT (bson_iter_recurse (&cluster_time, &cluster_time));
   BSON_ASSERT (bson_iter_find (&cluster_time, "clusterTime"));
   BSON_ASSERT (BSON_ITER_HOLDS_TIMESTAMP (&cluster_time));
   bson_iter_timestamp (&cluster_time, &t, &i);
   if (t != timestamp || i != increment) {
      fprintf (stderr,
               "Expected Timestamp(%d, %d), got Timestamp(%d, %d)\n",
               timestamp,
               increment,
               t,
               i);
      abort ();
   }

   return request;
}


static void
assert_ok (future_t *future, const bson_error_t *error)
{
   bool r = future_get_bool (future);
   ASSERT_OR_PRINT (r, (*error));
   future_destroy (future);
}


static future_t *
future_ping (mongoc_client_t *client, bson_error_t *error)
{
   return future_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, NULL, error);
}


static void
_test_cluster_time_comparison (bool pooled)
{
   const char *ismaster =
      "{'ok': 1.0, 'ismaster': true, 'msg': 'isdbgrid', 'maxWireVersion': 6}";
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bson_error_t error;
   future_t *future;
   request_t *request;
   bson_t *ping = tmp_bson ("{'ping': 1}");

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", 500);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   future = future_ping (client, &error);

   /* timestamp is 1 */
   request = mock_server_receives_ismaster (server);
   replies_with_cluster_time (request, 1, 1, ismaster);

   if (pooled) {
      /* a pooled client handshakes its own connection */
      request =
         mock_server_receives_msg (server, 0, tmp_bson ("{'ismaster': 1}"));
      replies_with_cluster_time (request, 1, 1, ismaster);
   }

   request = receives_with_cluster_time (server, 1, 1, ping);

   /* timestamp is 2, increment is 2 */
   replies_with_cluster_time (request, 2, 2, "{'ok': 1.0}");
   assert_ok (future, &error);

   future = future_ping (client, &error);
   request = receives_with_cluster_time (server, 2, 2, ping);

   /* timestamp is 2, increment is only 1 */
   replies_with_cluster_time (request, 2, 1, "{'ok': 1.0}");
   assert_ok (future, &error);

   future = future_ping (client, &error);

   /* client doesn't update cluster time, since new value is less than old */
   request = receives_with_cluster_time (server, 2, 2, ping);
   mock_server_replies_ok_and_destroys (request);
   assert_ok (future, &error);

   if (pooled) {
      /* wait for next heartbeat, it should contain newest cluster time */
      request = mock_server_receives_command (server,
                                              "admin",
                                              MONGOC_QUERY_SLAVE_OK,
                                              "{'isMaster': 1, '$clusterTime': "
                                              "{'clusterTime': {'$timestamp': "
                                              "{'t': 2, 'i': 2}}}}");

      replies_with_cluster_time (request, 2, 1, ismaster);

      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      /* trigger next heartbeat, it should contain newest cluster time */
      _mongoc_usleep (750 * 1000); /* 750 ms */
      future = future_ping (client, &error);
      request = mock_server_receives_command (server,
                                              "admin",
                                              MONGOC_QUERY_SLAVE_OK,
                                              "{'isMaster': 1, '$clusterTime': "
                                              "{'clusterTime': {'$timestamp': "
                                              "{'t': 2, 'i': 2}}}}");

      replies_with_cluster_time (request, 2, 1, ismaster);
      request = receives_with_cluster_time (server, 2, 2, ping);
      mock_server_replies_ok_and_destroys (request);
      assert_ok (future, &error);

      mongoc_client_destroy (client);
   }

   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
test_cluster_time_comparison_single (void)
{
   _test_cluster_time_comparison (false);
}


static void
test_cluster_time_comparison_pooled (void)
{
   _test_cluster_time_comparison (true);
}


typedef future_t *(*run_command_fn_t) (mongoc_client_t *);


typedef struct {
   const char *errmsg;
   bool is_not_master_err;
} test_error_msg_t;


test_error_msg_t errors[] = {{"not master", true},
                             {"not master or secondary", true},
                             {"node is recovering", true},
                             {"foo", false},
                             {0}};

/* a "not master" or "node is recovering" error marks server Unknown */
static void
_test_not_master (bool pooled, bool use_op_msg, run_command_fn_t run_command)
{
   test_error_msg_t *test_error_msg;
   mock_server_t *server;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   const char *cmd = "{'cmd': 1}";
   bson_error_t error;
   future_t *future;
   request_t *request;
   mongoc_topology_description_t *td;
   mongoc_server_description_t *sd;
   char *reply;

   server = mock_server_with_autoismaster (use_op_msg ? 6 : 5);
   mock_server_run (server);

   if (pooled) {
      pool = mongoc_client_pool_new (mock_server_get_uri (server));
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   }

   td = &client->topology->description;
   sd = (mongoc_server_description_t *) mongoc_set_get (td->servers, 1);

   for (test_error_msg = errors; test_error_msg->errmsg; test_error_msg++) {
      /*
       * successful command results in known server type
       */
      future = future_client_command_simple (
         client, "test", tmp_bson (cmd), NULL, NULL, &error);

      request = mock_server_receives_request (server);
      mock_server_replies_ok_and_destroys (request);
      BSON_ASSERT (future_get_bool (future));
      future_destroy (future);

      BSON_ASSERT (sd->type == MONGOC_SERVER_STANDALONE);

      /*
       * command error marks server Unknown iff it's a "not master" error
       */
      future = run_command (client);
      request = mock_server_receives_request (server);

      reply = bson_strdup_printf ("{'ok': 0, 'errmsg': '%s'}",
                                  test_error_msg->errmsg);
      mock_server_replies_simple (request, reply);
      BSON_ASSERT (!future_get_bool (future));

      if (test_error_msg->is_not_master_err) {
         BSON_ASSERT (sd->type == MONGOC_SERVER_UNKNOWN);
      } else {
         BSON_ASSERT (sd->type == MONGOC_SERVER_STANDALONE);
      }

      bson_free (reply);
      request_destroy (request);
      future_destroy (future);
   }

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mock_server_destroy (server);
}


static future_t *
future_command_simple (mongoc_client_t *client)
{
   return future_client_command_simple (
      client, "test", tmp_bson ("{'cmd': 1}"), NULL, NULL, NULL);
}


static void
test_not_master_single_op_query (void)
{
   _test_not_master (false, false, future_command_simple);
}


static void
test_not_master_pooled_op_query (void)
{
   _test_not_master (true, false, future_command_simple);
}

static void
test_not_master_single_op_msg (void)
{
   _test_not_master (false, true, future_command_simple);
}


static void
test_not_master_pooled_op_msg (void)
{
   _test_not_master (true, true, future_command_simple);
}


/* parts must remain valid after future_command_private exits */
mongoc_cmd_parts_t parts;

static future_t *
future_command_private (mongoc_client_t *client)
{
   bson_error_t error;
   mongoc_server_stream_t *server_stream;

   server_stream = mongoc_cluster_stream_for_writes (&client->cluster, &error);
   ASSERT_OR_PRINT (server_stream, error);

   mongoc_cmd_parts_init (
      &parts, client, "test", MONGOC_QUERY_NONE, tmp_bson ("{'cmd': 1}"));

   /* mongoc_cluster_run_command_parts will call mongoc_cmd_parts_cleanup */
   return future_cluster_run_command_parts (
      &client->cluster, server_stream, &parts, NULL, NULL);
}


static void
test_not_master_auth_single_op_query (void)
{
   _test_not_master (false, false, future_command_private);
}


static void
test_not_master_auth_pooled_op_query (void)
{
   _test_not_master (true, false, future_command_private);
}

static void
test_not_master_auth_single_op_msg (void)
{
   _test_not_master (false, true, future_command_private);
}


static void
test_not_master_auth_pooled_op_msg (void)
{
   _test_not_master (true, true, future_command_private);
}


typedef struct {
   const char *name;
   const char *q;
   const char *e;
   bool secondary;
   bool cluster_time;
} dollar_query_test_t;


static bool
auto_ismaster (request_t *request, void *data)
{
   dollar_query_test_t *test;

   if (!request->is_command ||
       strcasecmp (request->command_name, "ismaster") != 0) {
      return false;
   }

   test = (dollar_query_test_t *) data;
   if (test->cluster_time) {
      mock_server_replies_simple (
         request,
         "{'ok': 1, "
         " 'minWireVersion': 0,"
         " 'maxWireVersion': 6,"
         " '$clusterTime': {'clusterTime': {'$timestamp': {'t': 1, 'i': 1}}}}");
   } else {
      mock_server_replies_simple (request,
                                  "{'ok': 1, "
                                  " 'minWireVersion': 0,"
                                  " 'maxWireVersion': 6}");
   }

   request_destroy (request);

   return true;
}


static void
_test_dollar_query (void *ctx)
{
   dollar_query_test_t *test;
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_read_prefs_t *read_prefs;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;
   bson_error_t error;

   test = (dollar_query_test_t *) ctx;

   server = mock_server_new ();
   mock_server_autoresponds (server, auto_ismaster, test, NULL);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");
   if (test->secondary) {
      read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   } else {
      read_prefs = NULL;
   }

   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    0,
                                    0,
                                    tmp_bson (test->q),
                                    NULL,
                                    read_prefs);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_msg (server, 0, tmp_bson (test->e));
   mock_server_replies_to_find (
      request, MONGOC_QUERY_NONE, 0, 0, "db.collection", "", true);

   BSON_ASSERT (!future_get_bool (future));
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   future_destroy (future);
   request_destroy (request);
   mongoc_read_prefs_destroy (read_prefs);
   mongoc_cursor_destroy (cursor);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


dollar_query_test_t tests[] = {
   {"/Cluster/cluster_time/query/",
    "{'a': 1}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   '$clusterTime': {'$exists': false}"
    "}",
    false,
    false},
   {"/Cluster/cluster_time/query/cluster_time",
    "{'a': 1}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   '$clusterTime': {'$exists': true}"
    "}",
    false,
    true},
   {"/Cluster/cluster_time/query/secondary",
    "{'a': 1}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1}, "
    "   '$clusterTime': {'$exists': false},"
    "   '$readPreference': {'mode': 'secondary'}"
    "}",
    true,
    false},
   {"/Cluster/cluster_time/query/cluster_time_secondary",
    "{'a': 1}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1}, "
    "   '$clusterTime': {'$exists': true},"
    "   '$readPreference': {'mode': 'secondary'}"
    "}",
    true,
    true},
   {"/Cluster/cluster_time/dollar_query/from_user",
    "{'$query': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   '$clusterTime': {'$exists': false}"
    "}",
    false,
    false},
   {"/Cluster/cluster_time/dollar_query/from_user/cluster_time",
    "{'$query': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   '$clusterTime': {'$exists': true}"
    "}",
    false,
    true},
   {"/Cluster/cluster_time/dollar_query/from_user/secondary",
    "{'$query': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   '$clusterTime': {'$exists': false},"
    "   '$readPreference': {'mode': 'secondary'}"
    "}",
    true,
    false},
   {"/Cluster/cluster_time/dollar_query/from_user/cluster_time_secondary",
    "{'$query': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   '$clusterTime': {'$exists': true},"
    "   '$readPreference': {'mode': 'secondary'}"
    "}",
    true,
    true},
   {"/Cluster/cluster_time/dollar_orderby",
    "{'$query': {'a': 1}, '$orderby': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   'sort': {'a': 1}"
    "}",
    false,
    false},
   {"/Cluster/cluster_time/dollar_orderby/secondary",
    "{'$query': {'a': 1}, '$orderby': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   'sort': {'a': 1},"
    "   '$clusterTime': {'$exists': false},"
    "   '$readPreference': {'mode': 'secondary'}"
    "}",
    true,
    false},
   {"/Cluster/cluster_time/dollar_orderby/cluster_time",
    "{'$query': {'a': 1}, '$orderby': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   'sort': {'a': 1},"
    "   '$clusterTime': {'$exists': true}"
    "}",
    false,
    true},
   {"/Cluster/cluster_time/dollar_orderby/cluster_time_secondary",
    "{'$query': {'a': 1}, '$orderby': {'a': 1}}",
    "{"
    "   'find': 'collection', 'filter': {'a': 1},"
    "   'sort': {'a': 1},"
    "   '$clusterTime': {'$exists': true},"
    "   '$readPreference': {'mode': 'secondary'}"
    "}",
    true,
    true},
   {NULL}};

static void
_test_cluster_ismaster_fails (bool hangup)
{
   mock_server_t *mock_server;
   mongoc_uri_t *uri;
   mongoc_server_description_t *sd;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   request_t *request;
   future_t *future;
   bson_error_t error;
   int autoresponder_id;

   mock_server = mock_server_new ();
   autoresponder_id =
      mock_server_auto_ismaster (mock_server, "{ 'isMaster': 1.0 }");
   mock_server_run (mock_server);
   uri = mongoc_uri_copy (mock_server_get_uri (mock_server));
   /* increase heartbeatFrequencyMS to prevent background server selection. */
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", 99999);
   pool = mongoc_client_pool_new (uri);
   mongoc_client_pool_set_error_api (pool, 2);
   mongoc_uri_destroy (uri);
   client = mongoc_client_pool_pop (pool);
   /* do server selection to add this server to the topology. this does not add
    * a cluster node for this server. */
   sd = mongoc_client_select_server (client, false, NULL, NULL);
   BSON_ASSERT (sd);
   mongoc_server_description_destroy (sd);
   mock_server_remove_autoresponder (mock_server, autoresponder_id);
   /* now create a cluster node by running a command. */
   future = future_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);
   /* the client adds a cluster node, creating a stream to the server, and then
    * sends an ismaster request. */
   request = mock_server_receives_ismaster (mock_server);
   /* CDRIVER-2576: the server replies with an error, so
    * _mongoc_stream_run_ismaster returns NULL, which
    * _mongoc_cluster_run_ismaster must check. */

   if (hangup) {
      capture_logs (true); /* suppress "failed to buffer" warning */
      mock_server_hangs_up (request);
      BSON_ASSERT (!future_get_bool (future));
      ASSERT_ERROR_CONTAINS (
         error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "socket err");
   } else {
      mock_server_replies_simple (request, "{'ok': 0, 'code': 123}");
      BSON_ASSERT (!future_get_bool (future));
      ASSERT_ERROR_CONTAINS (
         error, MONGOC_ERROR_SERVER, 123, "Unknown command error");
   }

   request_destroy (request);
   future_destroy (future);
   mock_server_destroy (mock_server);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}

static void
test_cluster_ismaster_fails (void)
{
   _test_cluster_ismaster_fails (false);
}


static void
test_cluster_ismaster_hangup (void)
{
   _test_cluster_ismaster_fails (true);
}

static void
_test_cluster_command_error (bool use_op_msg)
{
   mock_server_t *server;
   mongoc_client_t *client;
   bson_error_t err;
   request_t *request;
   future_t *future;

   if (use_op_msg) {
      server = mock_server_with_autoismaster (WIRE_VERSION_OP_MSG);
   } else {
      server = mock_server_with_autoismaster (WIRE_VERSION_OP_MSG - 1);
   }
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   future = future_client_command_simple (client,
                                          "db",
                                          tmp_bson ("{'ping': 1}"),
                                          NULL /* opts */,
                                          NULL /* read prefs */,
                                          &err);
   if (use_op_msg) {
      request = mock_server_receives_msg (
         server, MONGOC_QUERY_NONE, tmp_bson ("{'ping': 1}"));
   } else {
      request = mock_server_receives_command (
         server, "db", MONGOC_QUERY_SLAVE_OK, "{'ping': 1}");
   }
   mock_server_hangs_up (request);
   BSON_ASSERT (!future_get_bool (future));
   future_destroy (future);
   request_destroy (request);
   if (use_op_msg) {
      /* _mongoc_buffer_append_from_stream, used by opmsg gives more detail. */
      ASSERT_ERROR_CONTAINS (err,
                             MONGOC_ERROR_STREAM,
                             MONGOC_ERROR_STREAM_SOCKET,
                             "Failed to send \"ping\" command with database "
                             "\"db\": Failed to read 4 bytes: socket error or "
                             "timeout");
   } else {
      ASSERT_ERROR_CONTAINS (err,
                             MONGOC_ERROR_STREAM,
                             MONGOC_ERROR_STREAM_SOCKET,
                             "Failed to send \"ping\" command with database "
                             "\"db\": socket error or timeout");
   }
   mock_server_destroy (server);
   mongoc_client_destroy (client);
}

static void
test_cluster_command_error_op_msg ()
{
   _test_cluster_command_error (true);
}

static void
test_cluster_command_error_op_query ()
{
   _test_cluster_command_error (false);
}

void
test_cluster_install (TestSuite *suite)
{
   dollar_query_test_t *p = tests;

   while (p->name) {
      TestSuite_AddFull (suite,
                         p->name,
                         _test_dollar_query,
                         NULL,
                         p,
                         TestSuite_CheckMockServerAllowed);

      p++;
   }

   TestSuite_AddLive (
      suite, "/Cluster/test_get_max_bson_obj_size", test_get_max_bson_obj_size);
   TestSuite_AddLive (
      suite, "/Cluster/test_get_max_msg_size", test_get_max_msg_size);
   TestSuite_AddFull (suite,
                      "/Cluster/disconnect/single",
                      test_cluster_node_disconnect_single,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Cluster/disconnect/pooled",
                      test_cluster_node_disconnect_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/command/timeout/single",
                                test_cluster_command_timeout_single);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/command/timeout/pooled",
                                test_cluster_command_timeout_pooled);
   TestSuite_AddFull (suite,
                      "/Cluster/write_command/disconnect",
                      test_write_command_disconnect,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/command_simple/single",
                      test_cluster_time_command_simple_single);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/command_simple/pooled",
                      test_cluster_time_command_simple_pooled);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/command/single",
                      test_cluster_time_command_single);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/command/pooled",
                      test_cluster_time_command_pooled);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/command_with_opts/single",
                      test_cluster_time_command_with_opts_single);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/command_with_opts/pooled",
                      test_cluster_time_command_with_opts_pooled);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/aggregate/single",
                      test_cluster_time_aggregate_single);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/aggregate/pooled",
                      test_cluster_time_aggregate_pooled);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/cursor/single",
                      test_cluster_time_cursor_single);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/cursor/pooled",
                      test_cluster_time_cursor_pooled);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/insert/single",
                      test_cluster_time_insert_single);
   TestSuite_AddLive (suite,
                      "/Cluster/cluster_time/insert/pooled",
                      test_cluster_time_insert_pooled);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/cluster_time/comparison/single",
                                test_cluster_time_comparison_single,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/cluster_time/comparison/pooled",
                                test_cluster_time_comparison_pooled,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master/single/op_query",
                                test_not_master_single_op_query,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master/pooled/op_query",
                                test_not_master_pooled_op_query,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master/single/op_msg",
                                test_not_master_single_op_msg,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master/pooled/op_msg",
                                test_not_master_pooled_op_msg,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master_auth/single/op_query",
                                test_not_master_auth_single_op_query,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master_auth/pooled/op_query",
                                test_not_master_auth_pooled_op_query,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master_auth/single/op_msg",
                                test_not_master_auth_single_op_msg,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/not_master_auth/pooled/op_msg",
                                test_not_master_auth_pooled_op_msg,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (
      suite, "/Cluster/ismaster_fails", test_cluster_ismaster_fails);
   TestSuite_AddMockServerTest (
      suite, "/Cluster/ismaster_hangup", test_cluster_ismaster_hangup);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/command_error/op_msg",
                                test_cluster_command_error_op_msg);
   TestSuite_AddMockServerTest (suite,
                                "/Cluster/command_error/op_query",
                                test_cluster_command_error_op_query);
}
