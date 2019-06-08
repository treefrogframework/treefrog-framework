#include <mongoc/mongoc.h>
#include <mongoc/mongoc-uri-private.h>
#include <mongoc/mongoc-client-pool-private.h>

#include "mongoc/mongoc-client-private.h"
#include "mongoc/mongoc-util-private.h"
#include "TestSuite.h"

#include "test-libmongoc.h"
#include "mock_server/mock-server.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "test-conveniences.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "topology-test"


static void
test_topology_client_creation (void)
{
   mongoc_uri_t *uri;
   mongoc_topology_scanner_node_t *node;
   mongoc_topology_t *topology_a;
   mongoc_topology_t *topology_b;
   mongoc_client_t *client_a;
   mongoc_client_t *client_b;
   mongoc_stream_t *topology_stream;
   mongoc_server_stream_t *server_stream;
   bson_error_t error;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_int32 (uri, "localThresholdMS", 42);
   mongoc_uri_set_option_as_int32 (uri, "connectTimeoutMS", 12345);
   mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 54321);

   /* create two clients directly */
   client_a = mongoc_client_new_from_uri (uri);
   client_b = mongoc_client_new_from_uri (uri);
   BSON_ASSERT (client_a);
   BSON_ASSERT (client_b);

#ifdef MONGOC_ENABLE_SSL
   test_framework_set_ssl_opts (client_a);
   test_framework_set_ssl_opts (client_b);
#endif

   /* ensure that they are using different topologies */
   topology_a = client_a->topology;
   topology_b = client_b->topology;
   BSON_ASSERT (topology_a);
   BSON_ASSERT (topology_b);
   BSON_ASSERT (topology_a != topology_b);

   BSON_ASSERT (topology_a->local_threshold_msec == 42);
   BSON_ASSERT (topology_a->connect_timeout_msec == 12345);
   BSON_ASSERT (topology_a->server_selection_timeout_msec == 54321);

   /* ensure that their topologies are running in single-threaded mode */
   BSON_ASSERT (topology_a->single_threaded);
   BSON_ASSERT (topology_a->scanner_state == MONGOC_TOPOLOGY_SCANNER_OFF);

   /* ensure that we are sharing streams with the client */
   server_stream = mongoc_cluster_stream_for_reads (
      &client_a->cluster, NULL, NULL, NULL, &error);

   ASSERT_OR_PRINT (server_stream, error);
   node = mongoc_topology_scanner_get_node (client_a->topology->scanner,
                                            server_stream->sd->id);
   BSON_ASSERT (node);
   topology_stream = node->stream;
   BSON_ASSERT (topology_stream);
   BSON_ASSERT (topology_stream == server_stream->stream);

   mongoc_server_stream_cleanup (server_stream);
   mongoc_client_destroy (client_a);
   mongoc_client_destroy (client_b);
   mongoc_uri_destroy (uri);
}

static void
assert_topology_state (mongoc_topology_t *topology,
                       mongoc_topology_scanner_state_t state)
{
   ASSERT (topology);

   bson_mutex_lock (&topology->mutex);
   ASSERT (topology->scanner_state == state);
   bson_mutex_unlock (&topology->mutex);
}

static void
test_topology_thread_start_stop (void)
{
   mongoc_client_pool_t *pool;
   mongoc_topology_t *topology;

   pool = test_framework_client_pool_new ();
   topology = _mongoc_client_pool_get_topology (pool);

   /* Test starting up the scanner */
   ASSERT (_mongoc_topology_start_background_scanner (topology));
   assert_topology_state (topology, MONGOC_TOPOLOGY_SCANNER_BG_RUNNING);

   /* Test that starting the topology while it is already
      running is ok to do. */
   ASSERT (_mongoc_topology_start_background_scanner (topology));
   assert_topology_state (topology, MONGOC_TOPOLOGY_SCANNER_BG_RUNNING);

   /* Test that we can stop the topology */
   _mongoc_topology_background_thread_stop (topology);
   assert_topology_state (topology, MONGOC_TOPOLOGY_SCANNER_OFF);

   /* Test that stopping the topology when it is already
      stopped is ok to do. */
   _mongoc_topology_background_thread_stop (topology);
   assert_topology_state (topology, MONGOC_TOPOLOGY_SCANNER_OFF);

   /* Test that we can start the topology again after stopping it */
   ASSERT (_mongoc_topology_start_background_scanner (topology));
   assert_topology_state (topology, MONGOC_TOPOLOGY_SCANNER_BG_RUNNING);

   mongoc_client_pool_destroy (pool);
}

static void
test_topology_client_pool_creation (void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client_a;
   mongoc_client_t *client_b;
   mongoc_topology_t *topology_a;
   mongoc_topology_t *topology_b;

   /* create two clients through a client pool */
   pool = test_framework_client_pool_new ();
   client_a = mongoc_client_pool_pop (pool);
   client_b = mongoc_client_pool_pop (pool);
   BSON_ASSERT (client_a);
   BSON_ASSERT (client_b);

   /* ensure that they are using the same topology */
   topology_a = client_a->topology;
   topology_b = client_b->topology;
   BSON_ASSERT (topology_a);
   BSON_ASSERT (topology_a == topology_b);

   /* ensure that this topology is running in a background thread */
   BSON_ASSERT (!topology_a->single_threaded);
   BSON_ASSERT (topology_a->scanner_state != MONGOC_TOPOLOGY_SCANNER_OFF);

   mongoc_client_pool_push (pool, client_a);
   mongoc_client_pool_push (pool, client_b);
   mongoc_client_pool_destroy (pool);
}

static void
test_server_selection_try_once_option (void *ctx)
{
   const char *uri_strings[3] = {"mongodb://a",
                                 "mongodb://a/?serverSelectionTryOnce=true",
                                 "mongodb://a/?serverSelectionTryOnce=false"};

   unsigned long i;
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool;

   /* try_once is on by default for non-pooled, can be turned off */
   client = mongoc_client_new (uri_strings[0]);
   BSON_ASSERT (client->topology->server_selection_try_once);
   mongoc_client_destroy (client);

   client = mongoc_client_new (uri_strings[1]);
   BSON_ASSERT (client->topology->server_selection_try_once);
   mongoc_client_destroy (client);

   client = mongoc_client_new (uri_strings[2]);
   BSON_ASSERT (!client->topology->server_selection_try_once);
   mongoc_client_destroy (client);

   /* off for pooled clients, can't be enabled */
   for (i = 0; i < sizeof (uri_strings) / sizeof (char *); i++) {
      uri = mongoc_uri_new ("mongodb://a");
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
      BSON_ASSERT (!client->topology->server_selection_try_once);
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
      mongoc_uri_destroy (uri);
   }
}

static void
_test_server_selection (bool try_once)
{
   mock_server_t *server;
   char *secondary_response;
   char *primary_response;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_read_prefs_t *primary_pref;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_server_description_t *sd;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_new ();
   mock_server_run (server);

   secondary_response =
      bson_strdup_printf ("{'ok': 1, "
                          " 'ismaster': false,"
                          " 'secondary': true,"
                          " 'setName': 'rs',"
                          " 'minWireVersion': 2,"
                          " 'maxWireVersion': 5,"
                          " 'hosts': ['%s']}",
                          mock_server_get_host_and_port (server));

   primary_response =
      bson_strdup_printf ("{'ok': 1, "
                          " 'ismaster': true,"
                          " 'setName': 'rs',"
                          " 'minWireVersion': 2,"
                          " 'maxWireVersion': 5,"
                          " 'hosts': ['%s']}",
                          mock_server_get_host_and_port (server));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", 500);
   mongoc_uri_set_option_as_int32 (uri, "serverSelectionTimeoutMS", 100);
   if (!try_once) {
      /* serverSelectionTryOnce is on by default */
      mongoc_uri_set_option_as_bool (uri, "serverSelectionTryOnce", false);
   }

   client = mongoc_client_new_from_uri (uri);
   primary_pref = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   /* no primary, selection fails after one try */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, secondary_response);
   request_destroy (request);

   /* the selection timeout is 100 ms, and we can't rescan until a half second
    * passes, so selection fails without another ismaster call */
   mock_server_set_request_timeout_msec (server, 600);
   BSON_ASSERT (!mock_server_receives_ismaster (server));
   mock_server_set_request_timeout_msec (server, get_future_timeout_ms ());

   /* selection fails */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER_SELECTION);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_SERVER_SELECTION_FAILURE);
   ASSERT_STARTSWITH (error.message, "No suitable servers found");

   if (try_once) {
      ASSERT_CONTAINS (error.message, "serverSelectionTryOnce");
   } else {
      ASSERT_CONTAINS (error.message, "serverselectiontimeoutms");
   }

   BSON_ASSERT (client->topology->stale);
   future_destroy (future);

   _mongoc_usleep (510 * 1000); /* one heartbeat, plus a few milliseconds */

   /* second selection, now we try ismaster again */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);

   /* the secondary is now primary, selection succeeds */
   mock_server_replies_simple (request, primary_response);
   sd = future_get_mongoc_server_description_ptr (future);
   BSON_ASSERT (sd);
   BSON_ASSERT (!client->topology->stale);
   request_destroy (request);
   future_destroy (future);

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (primary_pref);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   bson_free (secondary_response);
   bson_free (primary_response);
   mock_server_destroy (server);
}

static void
test_server_selection_try_once (void *ctx)
{
   _test_server_selection (true);
}

static void
test_server_selection_try_once_false (void *ctx)
{
   _test_server_selection (false);
}

static void
host_list_init (mongoc_host_list_t *host_list,
                int family,
                const char *host,
                uint16_t port)
{
   memset (host_list, 0, sizeof *host_list);
   host_list->family = family;
   bson_snprintf (host_list->host, sizeof host_list->host, "%s", host);
   bson_snprintf (host_list->host_and_port,
                  sizeof host_list->host_and_port,
                  "%s:%hu",
                  host,
                  port);
}

static void
_test_topology_invalidate_server (bool pooled)
{
   mongoc_server_description_t *fake_sd;
   mongoc_server_description_t *sd;
   mongoc_topology_description_t *td;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   bson_error_t error;
   mongoc_host_list_t fake_host_list;
   uint32_t fake_id = 42;
   uint32_t id;
   mongoc_server_stream_t *server_stream;

   uri = test_framework_get_uri ();
   /* no auto heartbeat */
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", INT32_MAX);
   mongoc_uri_set_option_as_int32 (uri, "connectTimeoutMS", 2000);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      test_framework_set_pool_ssl_opts (pool);
      client = mongoc_client_pool_pop (pool);

      /* background scanner complains about failed connection */
      capture_logs (true);
   } else {
      client = mongoc_client_new_from_uri (uri);
      test_framework_set_ssl_opts (client);
   }

   td = &client->topology->description;

   /* call explicitly */
   server_stream = mongoc_cluster_stream_for_reads (
      &client->cluster, NULL, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   sd = server_stream->sd;
   id = server_stream->sd->id;
   BSON_ASSERT (sd->type == MONGOC_SERVER_STANDALONE ||
                sd->type == MONGOC_SERVER_RS_PRIMARY ||
                sd->type == MONGOC_SERVER_MONGOS);

   ASSERT_CMPINT64 (sd->round_trip_time_msec, !=, (int64_t) -1);

   bson_set_error (
      &error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "error");
   mongoc_topology_invalidate_server (client->topology, id, &error);
   sd = (mongoc_server_description_t *) mongoc_set_get (td->servers, id);
   BSON_ASSERT (sd);
   BSON_ASSERT (sd->type == MONGOC_SERVER_UNKNOWN);
   ASSERT_CMPINT64 (sd->round_trip_time_msec, ==, (int64_t) -1);

   fake_sd = (mongoc_server_description_t *) bson_malloc0 (sizeof (*fake_sd));

   /* insert a 'fake' server description and ensure that it is invalidated by
    * driver */
   host_list_init (&fake_host_list, AF_INET, "fakeaddress", 27033);
   mongoc_server_description_init (
      fake_sd, fake_host_list.host_and_port, fake_id);

   fake_sd->type = MONGOC_SERVER_STANDALONE;
   mongoc_set_add (td->servers, fake_id, fake_sd);
   mongoc_topology_scanner_add (
      client->topology->scanner, &fake_host_list, fake_id);
   BSON_ASSERT (!mongoc_cluster_stream_for_server (
      &client->cluster, fake_id, true, NULL, NULL, &error));
   bson_mutex_lock (&client->topology->mutex);
   sd = (mongoc_server_description_t *) mongoc_set_get (td->servers, fake_id);
   if (!pooled && test_framework_is_replset ()) {
      BSON_ASSERT (!sd);
   } else {
      BSON_ASSERT (sd);
      BSON_ASSERT (sd->type == MONGOC_SERVER_UNKNOWN);
      BSON_ASSERT (sd->error.domain != 0);
      ASSERT_CMPINT64 (sd->round_trip_time_msec, ==, (int64_t) -1);
      BSON_ASSERT (bson_empty (&sd->last_is_master));
      BSON_ASSERT (bson_empty (&sd->hosts));
      BSON_ASSERT (bson_empty (&sd->passives));
      BSON_ASSERT (bson_empty (&sd->arbiters));
      BSON_ASSERT (bson_empty (&sd->compressors));
   }
   bson_mutex_unlock (&client->topology->mutex);

   mongoc_server_stream_cleanup (server_stream);
   mongoc_uri_destroy (uri);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }
}

static void
test_topology_invalidate_server_single (void *ctx)
{
   _test_topology_invalidate_server (false);
}

static void
test_topology_invalidate_server_pooled (void *ctx)
{
   _test_topology_invalidate_server (true);
}

static void
test_invalid_cluster_node (void *ctx)
{
   mongoc_client_pool_t *pool;
   mongoc_cluster_node_t *cluster_node;
   mongoc_topology_scanner_node_t *scanner_node;
   bson_error_t error;
   mongoc_client_t *client;
   mongoc_cluster_t *cluster;
   mongoc_server_stream_t *server_stream;
   int64_t scanner_node_ts;
   uint32_t id;

   /* use client pool, this test is only valid when multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);
   cluster = &client->cluster;

   _mongoc_usleep (100 * 1000);

   /* load stream into cluster */
   server_stream = mongoc_cluster_stream_for_reads (
      &client->cluster, NULL, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;
   mongoc_server_stream_cleanup (server_stream);

   cluster_node = (mongoc_cluster_node_t *) mongoc_set_get (cluster->nodes, id);
   BSON_ASSERT (cluster_node);
   BSON_ASSERT (cluster_node->stream);

   bson_mutex_lock (&client->topology->mutex);
   scanner_node =
      mongoc_topology_scanner_get_node (client->topology->scanner, id);
   BSON_ASSERT (scanner_node);
   ASSERT_CMPINT64 (cluster_node->timestamp, >, scanner_node->timestamp);

   /* update the scanner node's timestamp */
   _mongoc_usleep (1000 * 1000);
   scanner_node_ts = scanner_node->timestamp = bson_get_monotonic_time ();
   ASSERT_CMPINT64 (cluster_node->timestamp, <, scanner_node_ts);
   _mongoc_usleep (1000 * 1000);
   bson_mutex_unlock (&client->topology->mutex);

   /* cluster discards node and creates new one */
   server_stream = mongoc_cluster_stream_for_server (
      &client->cluster, id, true, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   cluster_node = (mongoc_cluster_node_t *) mongoc_set_get (cluster->nodes, id);
   ASSERT_CMPINT64 (cluster_node->timestamp, >, scanner_node_ts);

   mongoc_server_stream_cleanup (server_stream);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}

static void
test_max_wire_version_race_condition (void *ctx)
{
   mongoc_topology_scanner_node_t *scanner_node;
   mongoc_server_description_t *sd;
   mongoc_database_t *database;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   bson_error_t error;
   mongoc_server_stream_t *server_stream;
   uint32_t id;
   bool r;

   /* connect directly and add our user, test is only valid with auth */
   client = test_framework_client_new ();
   database = mongoc_client_get_database (client, "test");
   (void) mongoc_database_remove_user (database, "pink", &error);

   r = mongoc_database_add_user (database,
                                 "pink",
                                 "panther",
                                 tmp_bson ("[{'role': 'read', 'db': 'test'}]"),
                                 NULL,
                                 &error);

   ASSERT_OR_PRINT (r, error);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);

   /* use client pool, test is only valid when multi-threaded */
   pool = test_framework_client_pool_new ();
   client = mongoc_client_pool_pop (pool);

   /* load stream into cluster */
   server_stream = mongoc_cluster_stream_for_reads (
      &client->cluster, NULL, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   id = server_stream->sd->id;
   mongoc_server_stream_cleanup (server_stream);

   /* "disconnect": invalidate timestamp and reset server description */
   scanner_node =
      mongoc_topology_scanner_get_node (client->topology->scanner, id);
   BSON_ASSERT (scanner_node);
   scanner_node->timestamp = bson_get_monotonic_time ();
   sd = (mongoc_server_description_t *) mongoc_set_get (
      client->topology->description.servers, id);
   BSON_ASSERT (sd);
   mongoc_server_description_reset (sd);

   /* new stream, ensure that we can still auth with cached wire version */
   server_stream = mongoc_cluster_stream_for_server (
      &client->cluster, id, true, NULL, NULL, &error);
   ASSERT_OR_PRINT (server_stream, error);
   BSON_ASSERT (server_stream);

   mongoc_server_stream_cleanup (server_stream);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
}


static void
test_cooldown_standalone (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_read_prefs_t *primary_pref;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_server_description_t *sd;
   int64_t start;

   server = mock_server_new ();
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   primary_pref = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   /* first ismaster fails, selection fails */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);
   mock_server_hangs_up (request);
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   request_destroy (request);
   future_destroy (future);

   /* second selection doesn't try to call ismaster: we're in cooldown */
   start = bson_get_monotonic_time ();
   sd = mongoc_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   BSON_ASSERT (!sd);
   /* waited less than 500ms (minHeartbeatFrequencyMS), in fact
    * didn't wait at all since all nodes are in cooldown */
   ASSERT_CMPINT64 (bson_get_monotonic_time () - start, <, (int64_t) 500000);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_SERVER_SELECTION,
                          MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                          "No servers yet eligible for rescan");

   _mongoc_usleep (1000 * 1000); /* 1 second */

   /* third selection doesn't try to call ismaster: we're still in cooldown */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   mock_server_set_request_timeout_msec (server, 100);
   BSON_ASSERT (!mock_server_receives_ismaster (server)); /* no ismaster call */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_SERVER_SELECTION,
                          MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                          "No suitable servers");

   future_destroy (future);
   mock_server_set_request_timeout_msec (server, get_future_timeout_ms ());

   _mongoc_usleep (5100 * 1000); /* 5.1 seconds */

   /* cooldown ends, now we try ismaster again, this time succeeding */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);
   request = mock_server_receives_ismaster (server); /* not in cooldown now */
   BSON_ASSERT (request);
   mock_server_replies_simple (
      request,
      "{'ok': 1, 'ismaster': true, 'minWireVersion': 2, 'maxWireVersion': 5 }");
   sd = future_get_mongoc_server_description_ptr (future);
   BSON_ASSERT (sd);
   request_destroy (request);
   future_destroy (future);

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (primary_pref);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_cooldown_rs (void)
{
   mock_server_t *servers[2]; /* two secondaries, no primary */
   char *uri_str;
   int i;
   mongoc_client_t *client;
   mongoc_read_prefs_t *primary_pref;
   char *secondary_response;
   char *primary_response;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_server_description_t *sd;

   for (i = 0; i < 2; i++) {
      servers[i] = mock_server_new ();
      mock_server_run (servers[i]);
   }

   uri_str = bson_strdup_printf ("mongodb://localhost:%hu/?replicaSet=rs"
                                 "&serverSelectionTimeoutMS=100"
                                 "&connectTimeoutMS=100",
                                 mock_server_get_port (servers[0]));

   client = mongoc_client_new (uri_str);
   primary_pref = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   secondary_response = bson_strdup_printf (
      "{'ok': 1, 'ismaster': false, 'minWireVersion': 2, 'maxWireVersion': 5 , "
      "'secondary': true, 'setName': 'rs',"
      " 'hosts': ['localhost:%hu', 'localhost:%hu']}",
      mock_server_get_port (servers[0]),
      mock_server_get_port (servers[1]));

   primary_response =
      bson_strdup_printf ("{'ok': 1, 'ismaster': true, 'minWireVersion': 2, "
                          "'maxWireVersion': 5 , 'setName': 'rs',"
                          " 'hosts': ['localhost:%hu']}",
                          mock_server_get_port (servers[1]));

   /* server 0 is a secondary. */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);

   request = mock_server_receives_ismaster (servers[0]);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, secondary_response);
   request_destroy (request);

   /* server 0 told us about server 1. we check it immediately but it's down. */
   request = mock_server_receives_ismaster (servers[1]);
   BSON_ASSERT (request);
   mock_server_hangs_up (request);
   request_destroy (request);

   /* selection fails. */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   future_destroy (future);

   _mongoc_usleep (1000 * 1000); /* 1 second */

   /* second selection doesn't try ismaster on server 1: it's in cooldown */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);

   request = mock_server_receives_ismaster (servers[0]);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, secondary_response);
   request_destroy (request);

   mock_server_set_request_timeout_msec (servers[1], 100);
   BSON_ASSERT (
      !mock_server_receives_ismaster (servers[1])); /* no ismaster call */
   mock_server_set_request_timeout_msec (servers[1], get_future_timeout_ms ());

   /* still no primary */
   BSON_ASSERT (!future_get_mongoc_server_description_ptr (future));
   future_destroy (future);

   _mongoc_usleep (5100 * 1000); /* 5.1 seconds. longer than 5 sec cooldown. */

   /* cooldown ends, now we try ismaster on server 1, this time succeeding */
   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);

   request = mock_server_receives_ismaster (servers[1]);
   BSON_ASSERT (request);
   mock_server_replies_simple (request, primary_response);
   request_destroy (request);

   /* server 0 doesn't need to respond */
   sd = future_get_mongoc_server_description_ptr (future);
   BSON_ASSERT (sd);
   future_destroy (future);

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (primary_pref);
   mongoc_client_destroy (client);
   bson_free (secondary_response);
   bson_free (primary_response);
   bson_free (uri_str);
   mock_server_destroy (servers[0]);
   mock_server_destroy (servers[1]);
}


/* test single-threaded client's cooldown with serverSelectionTryOnce false */
static void
test_cooldown_retry (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_read_prefs_t *primary_pref;
   future_t *future;
   bson_error_t error;
   request_t *request;
   mongoc_server_description_t *sd;
   int64_t start;
   int64_t duration;

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_bool (uri, "serverSelectionTryOnce", false);
   client = mongoc_client_new_from_uri (uri);
   primary_pref = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   future = future_topology_select (
      client->topology, MONGOC_SS_READ, primary_pref, &error);

   /* first ismaster fails */
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);
   mock_server_hangs_up (request);
   request_destroy (request);

   /* after cooldown passes, driver sends another ismaster */
   start = bson_get_monotonic_time ();
   request = mock_server_receives_ismaster (server);
   BSON_ASSERT (request);
   duration = bson_get_monotonic_time () - start;
   /* waited at least cooldownMS, but not unreasonably longer than that */
   ASSERT_CMPINT64 (duration, >, (int64_t) 5 * 1000 * 1000);
   ASSERT_CMPINT64 (duration, <, (int64_t) 10 * 1000 * 1000);

   mock_server_replies_simple (
      request,
      "{'ok': 1, 'ismaster': true, 'minWireVersion': 2, 'maxWireVersion': 5 }");
   sd = future_get_mongoc_server_description_ptr (future);
   ASSERT_OR_PRINT (sd, error);
   request_destroy (request);
   future_destroy (future);

   mongoc_server_description_destroy (sd);
   mongoc_read_prefs_destroy (primary_pref);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


static void
_test_select_succeed (bool try_once)
{
   const int32_t connect_timeout_ms = 200;

   mock_server_t *primary;
   mock_server_t *secondary;
   mongoc_server_description_t *sd;
   char *uri_str;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   future_t *future;
   int64_t start;
   bson_error_t error;
   int64_t duration_usec;

   primary = mock_server_new ();
   mock_server_run (primary);

   secondary = mock_server_new ();
   mock_server_run (secondary);

   /* Note: do not use localhost here. If localhost has both A and AAAA records,
    * an attempt to connect to IPv6 occurs first. Most platforms refuse the IPv6
    * attempt immediately, so IPv4 succeeds immediately. Windows is an
    * exception, and waits 1 second before refusing:
    * https://support.microsoft.com/en-us/help/175523/info-winsock-tcp-connection-performance-to-unused-ports
    */
   /* primary auto-responds, secondary never responds */
   mock_server_auto_ismaster (primary,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'rs',"
                              "  'minWireVersion': 2,"
                              "  'maxWireVersion': 5,"
                              " 'hosts': ['127.0.0.1:%hu', '127.0.0.1:%hu']}",
                              mock_server_get_port (primary),
                              mock_server_get_port (secondary));

   uri_str = bson_strdup_printf ("mongodb://127.0.0.1:%hu,127.0.0.1:%hu/"
                                 "?replicaSet=rs&connectTimeoutMS=%d",
                                 mock_server_get_port (primary),
                                 mock_server_get_port (secondary),
                                 connect_timeout_ms);

   uri = mongoc_uri_new (uri_str);
   BSON_ASSERT (uri);
   if (!try_once) {
      /* override default */
      mongoc_uri_set_option_as_bool (uri, "serverSelectionTryOnce", false);
   }

   client = mongoc_client_new_from_uri (uri);

   /* start waiting for a primary (NULL read pref) */
   start = bson_get_monotonic_time ();
   future =
      future_topology_select (client->topology, MONGOC_SS_READ, NULL, &error);

   /* selection succeeds */
   sd = future_get_mongoc_server_description_ptr (future);
   ASSERT_OR_PRINT (sd, error);
   future_destroy (future);

   duration_usec = bson_get_monotonic_time () - start;

   if (!test_suite_valgrind ()) {
      ASSERT_ALMOST_EQUAL (duration_usec / 1000, connect_timeout_ms);
   }

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   bson_free (uri_str);
   mongoc_server_description_destroy (sd);
   mock_server_destroy (primary);
   mock_server_destroy (secondary);
}


/* CDRIVER-1219: a secondary is unavailable, scan should take connectTimeoutMS,
 * then we select primary */
static void
test_select_after_timeout (void)
{
   _test_select_succeed (false);
}


/* CDRIVER-1219: a secondary is unavailable, scan should try it once,
 * then we select primary */
static void
test_select_after_try_once (void)
{
   _test_select_succeed (true);
}


static void
test_multiple_selection_errors (void *context)
{
   const char *uri = "mongodb://doesntexist,example.com:2/?replicaSet=rs"
                     "&connectTimeoutMS=100";
   mongoc_client_t *client;
   bson_t reply;
   bson_error_t error;

   client = mongoc_client_new (uri);
   mongoc_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, &reply, &error);

   ASSERT_CMPINT (MONGOC_ERROR_SERVER_SELECTION, ==, error.domain);
   ASSERT_CMPINT (MONGOC_ERROR_SERVER_SELECTION_FAILURE, ==, error.code);

   /* Like:
    * "No suitable servers found (`serverselectiontryonce` set):
    *  [Failed to resolve 'doesntexist']
    *  [connection error calling ismaster on 'example.com:2']"
    */
   ASSERT_CONTAINS (error.message, "No suitable servers found");
   /* either "connection error" or "connection timeout" calling ismaster */
   ASSERT_CONTAINS (error.message, "calling ismaster on 'example.com:2'");
   ASSERT_CONTAINS (error.message, "[Failed to resolve 'doesntexist']");

   bson_destroy (&reply);
   mongoc_client_destroy (client);
}


static void
test_invalid_server_id (void)
{
   mongoc_client_t *client;
   bson_error_t error;

   client = test_framework_client_new ();

   BSON_ASSERT (
      !mongoc_topology_server_by_id (client->topology, 99999, &error));
   ASSERT_STARTSWITH (error.message, "Could not find description for node");

   mongoc_client_destroy (client);
}


static bool
auto_ping (request_t *request, void *data)
{
   if (!request->is_command || strcasecmp (request->command_name, "ping")) {
      return false;
   }

   mock_server_replies_ok_and_destroys (request);

   return true;
}


/* Tests CDRIVER-562: after calling ismaster to handshake a new connection we
 * must update topology description with the server response.
 */
static void
_test_server_removed_during_handshake (bool pooled)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bool r;
   bson_error_t error;
   mongoc_server_description_t *sd;
   mongoc_server_description_t **sds;
   size_t n;

   server = mock_server_new ();
   mock_server_run (server);
   mock_server_autoresponds (server, auto_ping, NULL, NULL);
   mock_server_auto_ismaster (server,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'rs',"
                              "  'minWireVersion': 2,"
                              "  'maxWireVersion': 5,"
                              " 'hosts': ['%s']}",
                              mock_server_get_host_and_port (server));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   /* no auto heartbeat */
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", INT32_MAX);
   mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");

   if (pooled) {
      pool = mongoc_client_pool_new (uri);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
   }

   /* initial connection, discover one-node replica set */
   r = mongoc_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT_OR_PRINT (r, error);

   ASSERT_CMPINT (_mongoc_topology_get_type (client->topology),
                  ==,
                  MONGOC_TOPOLOGY_RS_WITH_PRIMARY);
   sd = mongoc_client_get_server_description (client, 1);
   ASSERT_CMPINT ((int) MONGOC_SERVER_RS_PRIMARY, ==, sd->type);
   mongoc_server_description_destroy (sd);

   /* primary changes setName */
   mock_server_auto_ismaster (server,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'BAD NAME',"
                              "  'minWireVersion': 2,"
                              "  'maxWireVersion': 5,"
                              " 'hosts': ['%s']}",
                              mock_server_get_host_and_port (server));

   /* pretend to close a connection. does NOT affect server description yet */
   mongoc_cluster_disconnect_node (
      &client->cluster, 1, false /* invalidate */, NULL);
   sd = mongoc_client_get_server_description (client, 1);
   /* still primary */
   ASSERT_CMPINT ((int) MONGOC_SERVER_RS_PRIMARY, ==, sd->type);
   mongoc_server_description_destroy (sd);

   /* opens new stream and runs ismaster again, discovers bad setName. */
   r = mongoc_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT (!r);
   if (!pooled) {
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_STREAM,
                             MONGOC_ERROR_STREAM_NOT_ESTABLISHED,
                             "Could not find stream for node");
   } else {
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_STREAM,
                             MONGOC_ERROR_STREAM_NOT_ESTABLISHED,
                             "removed from topology");
   }

   sds = mongoc_client_get_server_descriptions (client, &n);
   ASSERT_CMPSIZE_T (n, ==, (size_t) 0);
   ASSERT_CMPINT (_mongoc_topology_get_type (client->topology),
                  ==,
                  MONGOC_TOPOLOGY_RS_NO_PRIMARY);

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_server_descriptions_destroy_all (sds, n);
   mock_server_destroy (server);
   mongoc_uri_destroy (uri);
}


static void
test_server_removed_during_handshake_single (void)
{
   _test_server_removed_during_handshake (false);
}


static void
test_server_removed_during_handshake_pooled (void)
{
   _test_server_removed_during_handshake (true);
}


static void
test_rtt (void *ctx)
{
   mock_server_t *server;
   mongoc_client_t *client;
   future_t *future;
   request_t *request;
   bson_error_t error;
   mongoc_server_description_t *sd;
   int64_t rtt_msec;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_new ();
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   request = mock_server_receives_ismaster (server);
   _mongoc_usleep (1000 * 1000); /* one second */
   mock_server_replies (request,
                        MONGOC_REPLY_NONE,
                        0,
                        0,
                        1,
                        "{'ok': 1, 'minWireVersion': 2, 'maxWireVersion': 5}");
   request_destroy (request);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'ping': 1}");
   mock_server_replies (request,
                        MONGOC_REPLY_NONE,
                        0,
                        0,
                        1,
                        "{'ok': 1, 'minWireVersion': 2, 'maxWireVersion': 5}");
   request_destroy (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);

   sd = mongoc_topology_server_by_id (client->topology, 1, NULL);
   ASSERT (sd);

   /* assert, with plenty of slack, that rtt was calculated in ms, not usec */
   rtt_msec = mongoc_server_description_round_trip_time (sd);
   ASSERT_CMPINT64 (rtt_msec, >, (int64_t) 900);  /* 900 ms */
   ASSERT_CMPINT64 (rtt_msec, <, (int64_t) 9000); /* 9 seconds */

   mongoc_server_description_destroy (sd);
   future_destroy (future);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


/* mongoc_topology_scanner_add and mongoc_topology_scan are called within the
 * topology mutex to add a discovered node and call getaddrinfo on its host
 * immediately - test that this doesn't cause a recursive acquire on the
 * topology mutex */
static void
test_add_and_scan_failure (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   future_t *future;
   request_t *request;
   bson_error_t error;
   mongoc_server_description_t *sd;

   server = mock_server_new ();
   mock_server_run (server);
   /* client will discover "fake" host and fail to connect */
   mock_server_auto_ismaster (server,
                              "{'ok': 1,"
                              " 'ismaster': true,"
                              " 'setName': 'rs',"
                              "  'minWireVersion': 2,"
                              "  'maxWireVersion': 5,"
                              " 'hosts': ['%s', 'fake:1']}",
                              mock_server_get_host_and_port (server));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");
   pool = mongoc_client_pool_new (uri);
   client = mongoc_client_pool_pop (pool);
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_NONE, "{'ping': 1}");
   mock_server_replies_ok_and_destroys (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);

   sd = mongoc_topology_server_by_id (client->topology, 1, NULL);
   ASSERT (sd);
   ASSERT_CMPSTR (mongoc_server_description_type (sd), "RSPrimary");
   mongoc_server_description_destroy (sd);

   sd = mongoc_topology_server_by_id (client->topology, 2, NULL);
   ASSERT (sd);
   ASSERT_CMPSTR (mongoc_server_description_type (sd), "Unknown");
   mongoc_server_description_destroy (sd);

   future_destroy (future);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


typedef struct {
   int n_started;
   int n_succeeded;
   int n_failed;
} checks_t;


static void
check_started (const mongoc_apm_server_heartbeat_started_t *event)
{
   checks_t *c;

   c = (checks_t *) mongoc_apm_server_heartbeat_started_get_context (event);
   c->n_started++;
}


static void
check_succeeded (const mongoc_apm_server_heartbeat_succeeded_t *event)
{
   checks_t *c;

   c = (checks_t *) mongoc_apm_server_heartbeat_succeeded_get_context (event);
   c->n_succeeded++;
}


static void
check_failed (const mongoc_apm_server_heartbeat_failed_t *event)
{
   checks_t *c;

   c = (checks_t *) mongoc_apm_server_heartbeat_failed_get_context (event);
   c->n_failed++;
}


static mongoc_apm_callbacks_t *
heartbeat_callbacks (void)
{
   mongoc_apm_callbacks_t *callbacks;

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_server_heartbeat_started_cb (callbacks, check_started);
   mongoc_apm_set_server_heartbeat_succeeded_cb (callbacks, check_succeeded);
   mongoc_apm_set_server_heartbeat_failed_cb (callbacks, check_failed);

   return callbacks;
}


static future_t *
future_command (mongoc_client_t *client, bson_error_t *error)
{
   return future_client_command_simple (
      client, "admin", tmp_bson ("{'foo': 1}"), NULL, NULL, error);
}


static void
receives_command (mock_server_t *server, future_t *future)
{
   request_t *request;
   bson_error_t error;

   request = mock_server_receives_command (
      server, "admin", MONGOC_QUERY_NONE, "{'foo': 1}");
   mock_server_replies_ok_and_destroys (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);
   future_destroy (future);
}


static bool
has_known_server (mongoc_client_t *client)
{
   mongoc_server_description_t *sd;
   bool r;

   /* in this test we know the server id is always 1 */
   sd = mongoc_client_get_server_description (client, 1);
   r = (sd->type != MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_destroy (sd);
   return r;
}


static void
_test_ismaster_retry_single (bool hangup, int n_failures)
{
   checks_t checks = {0};
   mongoc_apm_callbacks_t *callbacks;
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   char *ismaster;
   future_t *future;
   request_t *request;
   bson_error_t error;
   int64_t t;

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_REPLICASET, "rs");
   if (!hangup) {
      mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 100);
   }

   client = mongoc_client_new_from_uri (uri);
   callbacks = heartbeat_callbacks ();
   mongoc_client_set_apm_callbacks (client, callbacks, &checks);

   ismaster = bson_strdup_printf ("{'ok': 1,"
                                  " 'ismaster': true,"
                                  " 'setName': 'rs',"
                                  "  'minWireVersion': 2,"
                                  "  'maxWireVersion': 5,"
                                  " 'hosts': ['%s']}",
                                  mock_server_get_host_and_port (server));

   /* start a {foo: 1} command, handshake normally */
   future = future_command (client, &error);
   request = mock_server_receives_ismaster (server);
   mock_server_replies_simple (request, ismaster);
   request_destroy (request);
   receives_command (server, future);

   /* wait for the next server check */
   _mongoc_usleep (600 * 1000);

   /* start a {foo: 1} command, server check fails and retries immediately */
   future = future_command (client, &error);
   request = mock_server_receives_ismaster (server);
   t = bson_get_monotonic_time ();
   if (hangup) {
      mock_server_hangs_up (request);
   }

   request_destroy (request);

   /* retry immediately (for testing, "immediately" means less than 250ms */
   request = mock_server_receives_ismaster (server);
   ASSERT_CMPINT64 (bson_get_monotonic_time () - t, <, (int64_t) 250 * 1000);

   if (n_failures == 2) {
      if (hangup) {
         mock_server_hangs_up (request);
      }

      BSON_ASSERT (!future_get_bool (future));
      future_destroy (future);
   } else {
      mock_server_replies_simple (request, ismaster);
      /* the {foo: 1} command finishes */
      receives_command (server, future);
   }

   request_destroy (request);

   ASSERT_CMPINT (checks.n_started, ==, 3);
   WAIT_UNTIL (checks.n_succeeded == 3 - n_failures);
   WAIT_UNTIL (checks.n_failed == n_failures);

   if (n_failures == 2) {
      BSON_ASSERT (!has_known_server (client));
   } else {
      BSON_ASSERT (has_known_server (client));
   }

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
   bson_free (ismaster);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
_test_ismaster_retry_pooled (bool hangup, int n_failures)
{
   checks_t checks = {0};
   mongoc_apm_callbacks_t *callbacks;
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   char *ismaster;
   future_t *future;
   request_t *request;
   bson_error_t error;
   int i;
   int64_t t;

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_REPLICASET, "rs");
   if (!hangup) {
      mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 100);
   }

   pool = mongoc_client_pool_new (uri);
   callbacks = heartbeat_callbacks ();
   mongoc_client_pool_set_apm_callbacks (pool, callbacks, &checks);
   client = mongoc_client_pool_pop (pool);

   ismaster = bson_strdup_printf ("{'ok': 1,"
                                  " 'ismaster': true,"
                                  " 'setName': 'rs',"
                                  "  'minWireVersion': 2,"
                                  "  'maxWireVersion': 5,"
                                  " 'hosts': ['%s']}",
                                  mock_server_get_host_and_port (server));

   /* start a {foo: 1} command, handshake normally */
   future = future_command (client, &error);

   /* one ismaster from the scanner, another to handshake the connection */
   for (i = 0; i < 2; i++) {
      request = mock_server_receives_ismaster (server);
      mock_server_replies_simple (request, ismaster);
      request_destroy (request);
   }

   /* the {foo: 1} command finishes */
   receives_command (server, future);

   /* wait for the next server check */
   request = mock_server_receives_ismaster (server);
   t = bson_get_monotonic_time ();
   if (hangup) {
      mock_server_hangs_up (request);
   }

   request_destroy (request);

   /* retry immediately (for testing, "immediately" means less than 250ms */
   request = mock_server_receives_ismaster (server);
   ASSERT_CMPINT64 (bson_get_monotonic_time () - t, <, (int64_t) 250 * 1000);
   if (n_failures == 2) {
      if (hangup) {
         mock_server_hangs_up (request);
      }
      BSON_ASSERT (!has_known_server (client));
   } else {
      mock_server_replies_simple (request, ismaster);
      WAIT_UNTIL (has_known_server (client));
   }

   request_destroy (request);

   WAIT_UNTIL (checks.n_succeeded == 3 - n_failures);
   WAIT_UNTIL (checks.n_failed == n_failures);
   ASSERT_CMPINT (checks.n_started, ==, 3);

   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
   bson_free (ismaster);
   mongoc_apm_callbacks_destroy (callbacks);
}


static void
test_ismaster_retry_single_hangup (void)
{
   _test_ismaster_retry_single (true, 1);
}


static void
test_ismaster_retry_single_timeout (void)
{
   _test_ismaster_retry_single (false, 1);
}

static void
test_ismaster_retry_single_hangup_fail (void)
{
   _test_ismaster_retry_single (true, 2);
}


static void
test_ismaster_retry_single_timeout_fail (void)
{
   _test_ismaster_retry_single (false, 2);
}


static void
test_ismaster_retry_pooled_hangup (void)
{
   _test_ismaster_retry_pooled (true, 1);
}


static void
test_ismaster_retry_pooled_timeout (void)
{
   _test_ismaster_retry_pooled (false, 1);
}


static void
test_ismaster_retry_pooled_hangup_fail (void)
{
   _test_ismaster_retry_pooled (true, 2);
}


static void
test_ismaster_retry_pooled_timeout_fail (void)
{
   _test_ismaster_retry_pooled (false, 2);
}


static void
test_incompatible_error (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   bson_error_t error;
   char *msg;

   /* incompatible */
   server = mock_server_with_autoismaster (WIRE_VERSION_MIN - 1);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", 500);
   client = mongoc_client_new_from_uri (uri);

   /* trigger connection, fails due to incompatibility */
   ASSERT (!mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ismaster': 1}"), NULL, NULL, &error));

   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_PROTOCOL,
                          MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                          "reports wire version 2, but this version of"
                          " libmongoc requires at least 3 (MongoDB 3.0)");

   mock_server_auto_ismaster (server,
                              "{'ok': 1.0,"
                              " 'ismaster': true,"
                              " 'minWireVersion': 10,"
                              " 'maxWireVersion': 11}");

   /* wait until it's time for next heartbeat */
   _mongoc_usleep (600 * 1000);
   ASSERT (!mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ismaster': 1}"), NULL, NULL, &error));

   msg = bson_strdup_printf ("requires wire version 10, but this version"
                             " of libmongoc only supports up to %d",
                             WIRE_VERSION_MAX);

   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_PROTOCOL,
                          MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                          msg);

   bson_free (msg);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);
}


/* ensure there's no invalid access if a null bson_error_t pointer is passed
 * to mongoc_topology_compatible () */
static void
test_compatible_null_error_pointer (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_topology_description_t *td;
   bson_error_t error;

   /* incompatible */
   server = mock_server_with_autoismaster (WIRE_VERSION_MIN - 1);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   td = &client->topology->description;

   /* trigger connection, fails due to incompatibility */
   ASSERT (!mongoc_client_command_simple (
      client, "admin", tmp_bson ("{'ismaster': 1}"), NULL, NULL, &error));

   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_PROTOCOL, MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION, "");

   /* null error pointer is ok */
   ASSERT (!mongoc_topology_compatible (
      td, NULL /* read prefs */, NULL /* error */));

   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

static char *
cluster_time_fmt (int t)
{
   return bson_strdup_printf (
      "{"
      "  'clusterTime': {'$timestamp': {'t': %d, 'i': 1}},"
      "  'signature': {"
      "    'hash': {'$binary': {'subType': '0', 'base64': 'Yw=='}},"
      "    'keyId': {'$numberLong': '6446735049323708417'}"
      "   },"
      "  'operationTime': {'$timestamp': {'t': 1, 'i': 1}}"
      "}",
      t);
}

static void
test_cluster_time_updated_during_handshake ()
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   bool r;
   bson_error_t error;
   char *cluster_time;
   mongoc_server_description_t *sd;

   server = mock_server_new ();
   mock_server_run (server);
   mock_server_autoresponds (server, auto_ping, NULL, NULL);
   cluster_time = cluster_time_fmt (1);
   mock_server_auto_ismaster (server,
                              "{'ok': 1, 'ismaster': true, 'setName': 'rs', "
                              "'minWireVersion': 2, 'maxWireVersion': 7, "
                              "'hosts': ['%s'], '$clusterTime': %s}",
                              mock_server_get_host_and_port (server),
                              cluster_time);

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   /* set a large heartbeatFrequencyMS so we don't do a background scan in
    * between the first scan and handshake. */
   mongoc_uri_set_option_as_int32 (uri, "heartbeatFrequencyMS", 99999);
   mongoc_uri_set_option_as_utf8 (uri, "replicaSet", "rs");

   pool = mongoc_client_pool_new (uri);
   client = mongoc_client_pool_pop (pool);

   /* ensure a topology scan has run, populating the topology description
    * cluster time. */
   sd = mongoc_client_select_server (client, false, NULL, &error);
   ASSERT_OR_PRINT (sd, error);
   mongoc_server_description_destroy (sd);

   /* check the cluster time stored on the topology description. */
   bson_mutex_lock (&client->topology->mutex);
   ASSERT_MATCH (&client->topology->description.cluster_time, cluster_time);
   bson_mutex_unlock (&client->topology->mutex);
   bson_free (cluster_time);
   cluster_time = cluster_time_fmt (2);

   /* primary changes clusterTime */
   mock_server_auto_ismaster (server,
                              "{'ok': 1, 'ismaster': true, 'setName': 'rs', "
                              "'minWireVersion': 2, 'maxWireVersion': 7, "
                              "'hosts': ['%s'], '$clusterTime': %s}",
                              mock_server_get_host_and_port (server),
                              cluster_time);

   /* remove the node from the cluster to trigger an ismaster handshake. */
   mongoc_cluster_disconnect_node (
      &client->cluster, 1, false /* invalidate */, NULL);

   /* opens new stream and does an ismaster handshake (in pooled mode only). */
   r = mongoc_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);

   ASSERT_OR_PRINT (r, error);
   bson_mutex_lock (&client->topology->mutex);
   ASSERT_MATCH (&client->topology->description.cluster_time, cluster_time);
   bson_mutex_unlock (&client->topology->mutex);
   bson_free (cluster_time);
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mock_server_destroy (server);
   mongoc_uri_destroy (uri);
}

/* returns the last time the topology completed a full scan. */
static int64_t
_get_last_scan (mongoc_client_t *client)
{
   int64_t last_scan;
   mongoc_topology_t *topology = client->topology;
   bson_mutex_lock (&topology->mutex);
   last_scan = topology->last_scan;
   bson_mutex_unlock (&topology->mutex);
   return last_scan;
}

typedef struct {
   int64_t when_transitioned_to_unknown;
   int64_t server_id;
} request_scan_error_ctx_t;

static void
_server_changed (const mongoc_apm_server_changed_t *event)
{
   const mongoc_server_description_t *sd;
   request_scan_error_ctx_t *ctx;

   ctx = (request_scan_error_ctx_t *) mongoc_apm_server_changed_get_context (
      event);
   sd = mongoc_apm_server_changed_get_new_description (event);
   if (sd->type == MONGOC_SERVER_UNKNOWN) {
      ctx->when_transitioned_to_unknown = bson_get_monotonic_time ();
      ctx->server_id = sd->id;
   }
}

/* test that when a command receives a "not master" or "node is recovering"
 * error that the client takes the appropriate action:
 * - a pooled client should mark the server as unknown and request a full scan
 *   of the topology
 * - a single-threaded client should mark the server as unknown and mark the
 *   topology as stale.
 */
static void
_test_request_scan_on_error (bool pooled,
                             const char *err_response,
                             bool should_scan,
                             bool should_mark_unknown,
                             const char *server_err)
{
   mock_server_t *primary, *secondary;
   char *uri_str;
   mongoc_uri_t *uri;
   mongoc_client_pool_t *client_pool = NULL;
   mongoc_client_t *client = NULL;
   bson_t reply;
   bson_error_t error = {0};
   future_t *future = NULL;
   request_t *request;
   const int64_t minHBMS = 50;
   int64_t last_scan = 0;
   mongoc_read_prefs_t *read_prefs;
   mongoc_apm_callbacks_t *callbacks;
   request_scan_error_ctx_t ctx = {0};

   primary = mock_server_new ();
   secondary = mock_server_new ();
   mock_server_run (primary);
   mock_server_run (secondary);
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY_PREFERRED);

   RS_RESPONSE_TO_ISMASTER (primary, 6, true, false, primary, secondary);
   RS_RESPONSE_TO_ISMASTER (secondary, 6, false, false, primary, secondary);

   /* set a high heartbeatFrequency. Only the first and requested scans run. */
   uri_str = bson_strdup_printf (
      "mongodb://%s,%s/?replicaSet=rs&heartbeatFrequencyMS=999999",
      mock_server_get_host_and_port (primary),
      mock_server_get_host_and_port (secondary));
   uri = mongoc_uri_new (uri_str);
   bson_free (uri_str);

   if (pooled) {
      mongoc_topology_t *topology;
      client_pool = mongoc_client_pool_new (uri);
      topology = _mongoc_client_pool_get_topology (client_pool);
      /* set a small minHeartbeatFrequency, so scans don't block for 500ms. */
      topology->min_heartbeat_frequency_msec = minHBMS;
      client = mongoc_client_pool_pop (client_pool);
      /* upon popping a client, the background monitoring thread is started. */
      /* wait for the initial server selection to finish. */
      WAIT_UNTIL (_get_last_scan (client) > last_scan);
   } else {
      mongoc_server_description_t *sd;
      client = mongoc_client_new_from_uri (uri);
      /* set a small minHeartbeatFrequency, so scans don't block for 500ms. */
      client->topology->min_heartbeat_frequency_msec = minHBMS;
      sd = mongoc_client_select_server (client, false, NULL, &error);
      ASSERT_OR_PRINT (sd, error);
      mongoc_server_description_destroy (sd);
   }
   mongoc_uri_destroy (uri);
   /* now that the initial server selection is completed, record the time. */
   last_scan = _get_last_scan (client);
   /* listen for transition to UNKNOWN */
   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_server_changed_cb (callbacks, _server_changed);
   if (pooled) {
      mongoc_client_pool_set_apm_callbacks (client_pool, callbacks, &ctx);
   } else {
      mongoc_client_set_apm_callbacks (client, callbacks, &ctx);
   }
   mongoc_apm_callbacks_destroy (callbacks);
   /* run a ping command on the primary. */
   future = future_client_command_simple (
      client, "db", tmp_bson ("{'ping': 1}"), read_prefs, &reply, &error);
   request = mock_server_receives_msg (
      primary, MONGOC_QUERY_NONE, tmp_bson ("{'ping': 1}"));
   mock_server_replies_simple (request, err_response);
   request_destroy (request);
   /* don't check the return value of future. write concern errors are still
    * considered successful results. */
   future_wait (future);
   future_destroy (future);
   bson_destroy (&reply);

   if (should_mark_unknown) {
      mongoc_server_description_t *sd;
      /* between sending the 'ping' command and returning, the server should
       * have been marked as unknown. */
      ASSERT_CMPINT64 (last_scan, <=, ctx.when_transitioned_to_unknown);
      ASSERT_CMPINT64 (
         ctx.when_transitioned_to_unknown, <=, bson_get_monotonic_time ());
      sd = mongoc_client_get_server_description (client,
                                                 (uint32_t) ctx.server_id);
      /* check that the error on the server description matches the error
       * message in the response. */
      if (server_err) {
         ASSERT_CMPSTR (server_err, sd->error.message);
      }
      mongoc_server_description_destroy (sd);
   } else {
      ASSERT_CMPINT64 (ctx.when_transitioned_to_unknown, ==, (int64_t) 0);
   }

   if (pooled) {
      if (should_scan) {
         /* a scan is requested immediately. wait for the scan to finish. */
         WAIT_UNTIL (_get_last_scan (client) > last_scan);
      } else {
         /* wait a short while to make sure no scan occurs. */
         _mongoc_usleep (10 * 1000);
      }
   } else {
      /* a single threaded client may mark the topology as stale. if a scan
       * should occur, it won't be triggered until the next command. */
      future = future_client_command_simple (
         client, "db", tmp_bson ("{'ping': 1}"), read_prefs, &reply, &error);
      if (should_scan || !should_mark_unknown) {
         request = mock_server_receives_msg (
            primary, MONGOC_QUERY_NONE, tmp_bson ("{'ping': 1}"));
      } else {
         /* if the primary was marked as UNKNOWN, and no scan occurred, the ping
          * goes to the secondary. */
         request = mock_server_receives_msg (
            secondary, MONGOC_QUERY_NONE, tmp_bson ("{'ping': 1}"));
      }
      mock_server_replies_simple (request, "{'ok': 1}");
      request_destroy (request);
      BSON_ASSERT (future_get_bool (future));
      future_destroy (future);
      bson_destroy (&reply);
   }

   if (should_scan) {
      ASSERT_CMPINT64 (last_scan, <, _get_last_scan (client));
   } else {
      ASSERT_CMPINT64 (last_scan, ==, _get_last_scan (client));
   }

   mongoc_read_prefs_destroy (read_prefs);
   if (pooled) {
      mongoc_client_pool_push (client_pool, client);
      mongoc_client_pool_destroy (client_pool);
   } else {
      mongoc_client_destroy (client);
   }
   mock_server_destroy (primary);
   mock_server_destroy (secondary);
}


static void
test_request_scan_on_error ()
{
#define TEST_POOLED(msg, should_scan, should_mark_unknown, server_err) \
   _test_request_scan_on_error (                                       \
      true, msg, should_scan, should_mark_unknown, server_err)
#define TEST_SINGLE(msg, should_scan, should_mark_unknown, server_err) \
   _test_request_scan_on_error (                                       \
      false, msg, should_scan, should_mark_unknown, server_err)
#define TEST_BOTH(msg, should_scan, should_mark_unknown, server_err) \
   TEST_POOLED (msg, should_scan, should_mark_unknown, server_err);  \
   TEST_SINGLE (msg, should_scan, should_mark_unknown, server_err)

   TEST_BOTH ("{'ok': 0, 'errmsg': 'not master'}",
              true /* should_scan */,
              true /* should_mark_unknown */,
              "not master");
   /* "node is recovering" behaves differently for single and pooled clients. */
   TEST_SINGLE ("{'ok': 0, 'errmsg': 'node is recovering'}",
                false /* should_scan */,
                true /* should_mark_unknown */,
                "node is recovering");
   TEST_POOLED ("{'ok': 0, 'errmsg': 'node is recovering'}",
                true /* should_scan */,
                true /* should_mark_unknown */,
                "node is recovering");
   TEST_BOTH ("{'ok': 0, 'errmsg': 'random error'}",
              false /* should_scan */,
              false /* should_mark_unknown */,
              "random error");
   /* check the error code for NotMaster, which should be considered a "not
    * master" error. */
   TEST_BOTH ("{'ok': 0, 'code': 10107 }",
              true /* should_scan */,
              true /* should_mark_unknown */,
              NULL /* server_err */);
   /* for an unknown code, the message should still be checked. */
   TEST_BOTH ("{'ok': 0, 'code': 12345, 'errmsg': 'not master'}",
              true /* should_scan */,
              true /* should_mark_unknown */,
              "not master");
   /* check the error code for InterruptedAtShutdown, which should be considered
    * a "node is recovering" error. */
   TEST_SINGLE ("{'ok': 0, 'code': 11600 }",
                false /* should_scan */,
                true /* should_mark_unknown */,
                NULL /* server_err */);
   TEST_POOLED ("{'ok': 0, 'code': 11600 }",
                true /* should_scan */,
                true /* should_mark_unknown */,
                NULL /* server_err */);
   /* with a "not master" error code but a "node is recovery" message, the error
    * code takes precedence */
   TEST_BOTH ("{'ok': 0, 'code': 10107, 'errmsg': 'node is recovering'}",
              true /* should_scan */,
              true /* should_mark_unknown */,
              "node is recovering");
   /* write concern errors are also checked. */
   TEST_BOTH ("{'ok': 1, 'writeConcernError': { 'errmsg': 'not master' }}",
              true, /* should_scan */
              true /* should_mark_unknown */,
              "not master");
   TEST_BOTH ("{'ok': 1, 'writeConcernError': { 'code': 10107 }}",
              true, /* should_scan */
              true /* should_mark_unknown */,
              NULL /* server_err */);

#undef TEST_BOTH
#undef TEST_POOLED
#undef TEST_SINGLE
}


void
test_topology_install (TestSuite *suite)
{
   TestSuite_AddLive (
      suite, "/Topology/client_creation", test_topology_client_creation);
   TestSuite_AddLive (suite,
                      "/Topology/client_pool_creation",
                      test_topology_client_pool_creation);
   TestSuite_AddLive (
      suite, "/Topology/start_stop", test_topology_thread_start_stop);
   TestSuite_AddFull (suite,
                      "/Topology/server_selection_try_once_option",
                      test_server_selection_try_once_option,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/server_selection_try_once",
                      test_server_selection_try_once,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/server_selection_try_once_false",
                      test_server_selection_try_once_false,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/invalidate_server/single",
                      test_topology_invalidate_server_single,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live,
                      test_framework_skip_if_valgrind);
   TestSuite_AddFull (suite,
                      "/Topology/invalidate_server/pooled",
                      test_topology_invalidate_server_pooled,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live,
                      test_framework_skip_if_valgrind);
   TestSuite_AddFull (suite,
                      "/Topology/invalid_cluster_node",
                      test_invalid_cluster_node,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddFull (suite,
                      "/Topology/max_wire_version_race_condition",
                      test_max_wire_version_race_condition,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_auth);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/cooldown/standalone",
                                test_cooldown_standalone,
                                NULL,
                                NULL,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/cooldown/rs",
                                test_cooldown_rs,
                                NULL,
                                NULL,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/cooldown/retry",
                                test_cooldown_retry,
                                NULL,
                                NULL,
                                test_framework_skip_if_slow);
   TestSuite_AddFull (suite,
                      "/Topology/multiple_selection_errors",
                      test_multiple_selection_errors,
                      NULL,
                      NULL,
                      test_framework_skip_if_offline);
   TestSuite_AddMockServerTest (
      suite, "/Topology/connect_timeout/succeed", test_select_after_timeout);
   TestSuite_AddMockServerTest (
      suite, "/Topology/try_once/succeed", test_select_after_try_once);
   TestSuite_AddLive (
      suite, "/Topology/invalid_server_id", test_invalid_server_id);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/server_removed/single",
                                test_server_removed_during_handshake_single);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/server_removed/pooled",
                                test_server_removed_during_handshake_pooled);
   TestSuite_AddFull (suite,
                      "/Topology/rtt",
                      test_rtt,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (
      suite, "/Topology/add_and_scan_failure", test_add_and_scan_failure);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/single/hangup",
                                test_ismaster_retry_single_hangup,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/single/timeout",
                                test_ismaster_retry_single_timeout,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/single/hangup/fail",
                                test_ismaster_retry_single_hangup_fail,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/single/timeout/fail",
                                test_ismaster_retry_single_timeout_fail,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/pooled/hangup",
                                test_ismaster_retry_pooled_hangup,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/pooled/timeout",
                                test_ismaster_retry_pooled_timeout,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/pooled/hangup/fail",
                                test_ismaster_retry_pooled_hangup_fail,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/ismaster_retry/pooled/timeout/fail",
                                test_ismaster_retry_pooled_timeout_fail,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/incompatible_error",
                                test_incompatible_error,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/compatible_null_error_pointer",
                                test_compatible_null_error_pointer,
                                test_framework_skip_if_slow);
   TestSuite_AddMockServerTest (suite,
                                "/Topology/handshake/updates_clustertime",
                                test_cluster_time_updated_during_handshake);
   TestSuite_AddMockServerTest (
      suite, "/Topology/request_scan_on_error", test_request_scan_on_error);
}
