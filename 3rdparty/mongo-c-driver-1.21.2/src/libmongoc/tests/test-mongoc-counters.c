/*
 * Copyright 2018-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mongoc/mongoc-util-private.h>
#include "mongoc/mongoc-counters-private.h"
#include "mock_server/mock-server.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"
#include "TestSuite.h"
#include "mock_server/future-functions.h"

/* test statistics counters excluding OP_INSERT, OP_UPDATE, and OP_DELETE since
 * those were superseded by write commands in 2.6. */
#ifdef MONGOC_ENABLE_SHM_COUNTERS
/* define a count function for each counter. */
#define COUNTER(id, category, name, description)                               \
   int32_t prev_##id;                                                          \
   int32_t count_##id (void)                                                   \
   {                                                                           \
      int32_t _sum = 0;                                                        \
      uint32_t _i;                                                             \
      for (_i = 0; _i < _mongoc_get_cpu_count (); _i++) {                      \
         const int64_t *counter =                                              \
            &BSON_CONCAT (__mongoc_counter_, id)                               \
                .cpus[_i]                                                      \
                .slots[BSON_CONCAT (COUNTER_, id) % SLOTS_PER_CACHELINE];      \
         _sum += bson_atomic_int64_fetch (counter, bson_memory_order_seq_cst); \
      }                                                                        \
      return _sum;                                                             \
   }
#include "mongoc/mongoc-counters.defs"
#undef COUNTER

#define RESET(id)               \
   bson_atomic_int32_exchange ( \
      &prev_##id, count_##id (), bson_memory_order_seq_cst)

#define DIFF_AND_RESET(id, cmp, expected)     \
   do {                                       \
      int32_t old_count = prev_##id;          \
      int32_t new_count = count_##id ();      \
      int32_t _diff = new_count - old_count;  \
      ASSERT_CMPINT32 (_diff, cmp, expected); \
      RESET (id);                             \
   } while (0)

static void
reset_all_counters ()
{
#define COUNTER(id, category, name, description) RESET (id);
#include "mongoc/mongoc-counters.defs"
#undef COUNTER
}

/* create a client and disable server selection after performing it. */
static mongoc_client_t *
_client_new_disable_ss (bool use_compression)
{
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   mongoc_server_description_t *sd;
   bson_error_t err;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 99999);
   mongoc_uri_set_option_as_int32 (
      uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 99999);
   if (use_compression) {
      char *compressors = test_framework_get_compressors ();
      mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_COMPRESSORS, compressors);
      bson_free (compressors);
   }
   client = test_framework_client_new_from_uri (uri, NULL);
   test_framework_set_ssl_opts (client);
   sd = mongoc_client_select_server (client, true, NULL, &err);
   ASSERT_OR_PRINT (sd, err);
   mongoc_server_description_destroy (sd);
   mongoc_uri_destroy (uri);
   /* reset counters to exclude anything done in server selection. */
   reset_all_counters ();
   return client;
}


mongoc_collection_t *
_drop_and_populate_coll (mongoc_client_t *client)
{
   /* insert thrice. */
   mongoc_collection_t *coll;
   bool ret;
   bson_error_t err;
   int i;
   coll = mongoc_client_get_collection (client, "test", "test");
   mongoc_collection_drop (coll, NULL); /* don't care if ns not found. */
   for (i = 0; i < 3; i++) {
      ret =
         mongoc_collection_insert_one (coll, tmp_bson ("{}"), NULL, NULL, &err);
      ASSERT_OR_PRINT (ret, err);
   }
   return coll;
}


void
_ping (mongoc_client_t *client)
{
   bool ret;
   bson_error_t err;
   ret = mongoc_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, NULL, &err);
   ASSERT_OR_PRINT (ret, err);
}


static void
test_counters_op_msg (void *ctx)
{
   mongoc_collection_t *coll;
   mongoc_cursor_t *cursor;
   const bson_t *bson;
   mongoc_client_t *client;

   client = _client_new_disable_ss (false);
   _ping (client);
   DIFF_AND_RESET (op_egress_msg, ==, 1);
   DIFF_AND_RESET (op_egress_total, ==, 1);
   DIFF_AND_RESET (op_ingress_msg, ==, 1);
   DIFF_AND_RESET (op_ingress_total, ==, 1);
   coll = _drop_and_populate_coll (client);
   DIFF_AND_RESET (op_egress_msg, ==, 4);
   DIFF_AND_RESET (op_egress_total, ==, 4);
   DIFF_AND_RESET (op_ingress_msg, ==, 4);
   DIFF_AND_RESET (op_ingress_total, ==, 4);
   cursor =
      mongoc_collection_find_with_opts (coll, tmp_bson ("{}"), NULL, NULL);
   while (mongoc_cursor_next (cursor, &bson))
      ;
   mongoc_cursor_destroy (cursor);
   DIFF_AND_RESET (op_egress_msg, >, 0);
   DIFF_AND_RESET (op_ingress_msg, >, 0);
   DIFF_AND_RESET (op_egress_query, ==, 0);
   DIFF_AND_RESET (op_ingress_reply, ==, 0);
   DIFF_AND_RESET (op_egress_total, >, 0);
   DIFF_AND_RESET (op_ingress_total, >, 0);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}


static void
test_counters_op_compressed (void *ctx)
{
   mongoc_collection_t *coll;
   mongoc_client_t *client;

   client = _client_new_disable_ss (true);
   _ping (client);
   /* we count one OP_MSG and one OP_COMPRESSED for the same message. */
   DIFF_AND_RESET (op_egress_msg, ==, 1);
   DIFF_AND_RESET (op_egress_compressed, ==, 1);
   DIFF_AND_RESET (op_egress_total, ==, 2);
   DIFF_AND_RESET (op_ingress_msg, ==, 1);
   DIFF_AND_RESET (op_ingress_compressed, ==, 1);
   DIFF_AND_RESET (op_ingress_total, ==, 2);
   coll = _drop_and_populate_coll (client);
   DIFF_AND_RESET (op_egress_msg, ==, 4);
   DIFF_AND_RESET (op_egress_compressed, ==, 4);
   DIFF_AND_RESET (op_egress_total, ==, 8);
   DIFF_AND_RESET (op_ingress_msg, ==, 4);
   DIFF_AND_RESET (op_ingress_compressed, ==, 4);
   DIFF_AND_RESET (op_ingress_total, ==, 8);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}


static void
test_counters_cursors (void)
{
   mongoc_collection_t *coll;
   mongoc_cursor_t *cursor;
   const bson_t *bson;
   mongoc_client_t *client;

   client = _client_new_disable_ss (false);
   coll = _drop_and_populate_coll (client);
   cursor = mongoc_collection_find_with_opts (
      coll, tmp_bson ("{}"), tmp_bson ("{'batchSize': 1}"), NULL);
   DIFF_AND_RESET (cursors_active, ==, 1);
   while (mongoc_cursor_next (cursor, &bson))
      ;
   mongoc_cursor_destroy (cursor);
   DIFF_AND_RESET (cursors_active, ==, -1);
   DIFF_AND_RESET (cursors_disposed, ==, 1);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
}


static void
test_counters_clients (void)
{
   mongoc_client_pool_t *client_pool;
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   uri = test_framework_get_uri ();

   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 99999);
   mongoc_uri_set_option_as_int32 (
      uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 99999);
   reset_all_counters ();
   client = test_framework_client_new_from_uri (uri, NULL);
   BSON_ASSERT (client);
   DIFF_AND_RESET (clients_active, ==, 1);
   DIFF_AND_RESET (clients_disposed, ==, 0);
   DIFF_AND_RESET (client_pools_active, ==, 0);
   DIFF_AND_RESET (client_pools_disposed, ==, 0);
   mongoc_client_destroy (client);
   DIFF_AND_RESET (clients_active, ==, -1);
   DIFF_AND_RESET (clients_disposed, ==, 1);
   DIFF_AND_RESET (client_pools_active, ==, 0);
   DIFF_AND_RESET (client_pools_disposed, ==, 0);
   /* check client pools. */
   client_pool = test_framework_client_pool_new_from_uri (uri, NULL);
   BSON_ASSERT (client_pool);
   DIFF_AND_RESET (clients_active, ==, 0);
   DIFF_AND_RESET (clients_disposed, ==, 0);
   DIFF_AND_RESET (client_pools_active, ==, 1);
   DIFF_AND_RESET (client_pools_disposed, ==, 0);
   client = mongoc_client_pool_pop (client_pool);
   DIFF_AND_RESET (clients_active, ==, 1);
   DIFF_AND_RESET (clients_disposed, ==, 0);
   DIFF_AND_RESET (client_pools_active, ==, 0);
   DIFF_AND_RESET (client_pools_disposed, ==, 0);
   mongoc_client_destroy (client);
   DIFF_AND_RESET (clients_active, ==, -1);
   DIFF_AND_RESET (clients_disposed, ==, 1);
   DIFF_AND_RESET (client_pools_active, ==, 0);
   DIFF_AND_RESET (client_pools_disposed, ==, 0);
   mongoc_client_pool_destroy (client_pool);
   DIFF_AND_RESET (clients_active, ==, 0);
   DIFF_AND_RESET (clients_disposed, ==, 0);
   DIFF_AND_RESET (client_pools_active, ==, -1);
   DIFF_AND_RESET (client_pools_disposed, ==, 1);
   mongoc_uri_destroy (uri);
}


static void
test_counters_streams (void *ctx)
{
   mongoc_client_t *client = _client_new_disable_ss (false);
   mongoc_socket_t *sock;
   mongoc_stream_t *stream_sock;
   mongoc_stream_t *buffered_stream_sock;
   mongoc_stream_t *file_stream;
   mongoc_gridfs_t *gridfs;
   mongoc_gridfs_file_t *file;
   mongoc_stream_t *gridfs_stream;
   bool used_ssl = false;
   bson_error_t err;
   char buf[16] = {0};
   const int TIMEOUT = 500;
   mongoc_gridfs_file_opt_t gridfs_opts = {0};
   bool ret;

   /* test ingress and egress of a stream to a server. */
   _ping (client);
   DIFF_AND_RESET (streams_egress, >, 0);
   DIFF_AND_RESET (streams_ingress, >, 0);
   /* test that creating and destroying each type of stream changes the
    * streams active and not active. */
   sock = mongoc_socket_new (AF_INET, SOCK_STREAM, 0);
   DIFF_AND_RESET (streams_active, ==, 0);
   DIFF_AND_RESET (streams_disposed, ==, 0);
   stream_sock = mongoc_stream_socket_new (sock);
   DIFF_AND_RESET (streams_active, ==, 1);
   DIFF_AND_RESET (streams_disposed, ==, 0);
   buffered_stream_sock = mongoc_stream_buffered_new (stream_sock, 16);
   DIFF_AND_RESET (streams_active, ==, 1);
   DIFF_AND_RESET (streams_disposed, ==, 0);
#ifdef MONGOC_ENABLE_SSL
   do {
      const mongoc_ssl_opt_t *default_opts = mongoc_ssl_opt_get_default ();
      mongoc_ssl_opt_t opts = *default_opts;
      mongoc_stream_t *ssl_buffered_stream_socket;

      ssl_buffered_stream_socket = mongoc_stream_tls_new_with_hostname (
         buffered_stream_sock, NULL, &opts, 0);
      DIFF_AND_RESET (streams_active, ==, 1);
      DIFF_AND_RESET (streams_disposed, ==, 0);
      mongoc_stream_destroy (ssl_buffered_stream_socket);
      DIFF_AND_RESET (streams_active, ==, -3);
      DIFF_AND_RESET (streams_disposed, ==, 3);
      used_ssl = true;
   } while (0);
#endif
   if (!used_ssl) {
      mongoc_stream_destroy (buffered_stream_sock);
      DIFF_AND_RESET (streams_active, ==, -2);
      DIFF_AND_RESET (streams_disposed, ==, 2);
   }
/* check a file stream. */
#ifdef WIN32
   file_stream = mongoc_stream_file_new_for_path (
      BINARY_DIR "/temp.dat", O_CREAT | O_WRONLY | O_TRUNC, _S_IWRITE);
#else
   file_stream = mongoc_stream_file_new_for_path (
      BINARY_DIR "/temp.dat", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
#endif
   BSON_ASSERT (file_stream);
   DIFF_AND_RESET (streams_active, ==, 1);
   DIFF_AND_RESET (streams_disposed, ==, 0);
   mongoc_stream_write (file_stream, buf, 16, TIMEOUT);
   DIFF_AND_RESET (streams_egress, ==, 16);
   DIFF_AND_RESET (streams_ingress, ==, 0);
   mongoc_stream_destroy (file_stream);
   DIFF_AND_RESET (streams_active, ==, -1);
   DIFF_AND_RESET (streams_disposed, ==, 1);
   file_stream =
      mongoc_stream_file_new_for_path (BINARY_DIR "/temp.dat", O_RDONLY, 0);
   BSON_ASSERT (file_stream);
   DIFF_AND_RESET (streams_active, ==, 1);
   DIFF_AND_RESET (streams_disposed, ==, 0);
   mongoc_stream_read (file_stream, buf, 16, 0, TIMEOUT);
   DIFF_AND_RESET (streams_egress, ==, 0);
   DIFF_AND_RESET (streams_ingress, ==, 16);
   mongoc_stream_destroy (file_stream);
   DIFF_AND_RESET (streams_active, ==, -1);
   DIFF_AND_RESET (streams_disposed, ==, 1);
   unlink (BINARY_DIR "/temp.dat");
   /* check a gridfs stream. */
   gridfs = mongoc_client_get_gridfs (client, "test", "fs", &err);
   ASSERT_OR_PRINT (gridfs, err);
   ret = mongoc_gridfs_drop (gridfs, &err);
   ASSERT_OR_PRINT (ret, err);
   reset_all_counters ();
   gridfs_opts.filename = "example";
   file = mongoc_gridfs_create_file (gridfs, &gridfs_opts);
   ASSERT_OR_PRINT (file, err);
   gridfs_stream = mongoc_stream_gridfs_new (file);
   DIFF_AND_RESET (streams_active, ==, 1);
   DIFF_AND_RESET (streams_disposed, ==, 0);
   mongoc_stream_write (gridfs_stream, buf, 16, TIMEOUT);
   DIFF_AND_RESET (streams_egress, ==, 16);
   DIFF_AND_RESET (streams_ingress, ==, 0);
   mongoc_stream_destroy (gridfs_stream);
   DIFF_AND_RESET (streams_active, ==, -1);
   DIFF_AND_RESET (streams_disposed, ==, 1);
   DIFF_AND_RESET (streams_egress, >, 0);
   mongoc_gridfs_file_save (file);
   mongoc_gridfs_file_destroy (file);
   file = mongoc_gridfs_find_one_by_filename (gridfs, "example", &err);
   ASSERT_OR_PRINT (file, err);
   gridfs_stream = mongoc_stream_gridfs_new (file);
   DIFF_AND_RESET (streams_active, ==, 1);
   DIFF_AND_RESET (streams_disposed, ==, 0);
   RESET (streams_egress);
   mongoc_stream_read (gridfs_stream, buf, 16, 0, TIMEOUT);
   DIFF_AND_RESET (streams_egress, >, 0);
   DIFF_AND_RESET (streams_ingress, >, 16);
   mongoc_stream_destroy (gridfs_stream);
   DIFF_AND_RESET (streams_active, ==, -1);
   DIFF_AND_RESET (streams_disposed, ==, 1);
   mongoc_gridfs_file_destroy (file);
   mongoc_gridfs_destroy (gridfs);
   mongoc_client_destroy (client);
}


static void
test_counters_auth (void *ctx)
{
   char *host_and_port = test_framework_get_host_and_port ();
   char *uri_str = test_framework_get_uri_str ();
   char *uri_str_bad = bson_strdup_printf (
      "mongodb://%s:%s@%s/", "bad_user", "bad_pass", host_and_port);
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   bool ret;
   bson_error_t err;

   uri = mongoc_uri_new (uri_str);
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 99999);
   mongoc_uri_set_option_as_int32 (
      uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 99999);
   reset_all_counters ();
   client = test_framework_client_new_from_uri (uri, NULL);
   test_framework_set_ssl_opts (client);
   BSON_ASSERT (client);
   ret = mongoc_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, NULL, &err);
   ASSERT_OR_PRINT (ret, err);
   DIFF_AND_RESET (auth_success, ==, 1);
   DIFF_AND_RESET (auth_failure, ==, 0);
   mongoc_uri_destroy (uri);
   bson_free (uri_str);
   bson_free (uri_str_bad);
   bson_free (host_and_port);
   mongoc_client_destroy (client);
}


static void
test_counters_dns (void)
{
   mongoc_client_t *client;
   mongoc_server_description_t *sd;
   bson_error_t err;
   reset_all_counters ();
   client = test_framework_new_default_client ();
   sd = mongoc_client_select_server (client, false, NULL, &err);
   ASSERT_OR_PRINT (sd, err);
   DIFF_AND_RESET (dns_success, >, 0);
   DIFF_AND_RESET (dns_failure, ==, 0);
   mongoc_server_description_destroy (sd);
   mongoc_client_destroy (client);
   client = test_framework_client_new ("mongodb://invalidhostname/", NULL);
   test_framework_set_ssl_opts (client);
   sd = mongoc_client_select_server (client, false, NULL, &err);
   ASSERT (!sd);
   DIFF_AND_RESET (dns_success, ==, 0);
   DIFF_AND_RESET (dns_failure, ==, 1);
   mongoc_client_destroy (client);
}


static void
test_counters_streams_timeout ()
{
   mock_server_t *server;
   bson_error_t err = {0};
   bool ret;
   future_t *future;
   mongoc_client_t *client;
   request_t *request;
   mongoc_uri_t *uri;
   mongoc_server_description_t *sd;

   server = mock_server_with_auto_hello (WIRE_VERSION_MAX);
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 300);
   client = test_framework_client_new_from_uri (uri, NULL);
   mongoc_uri_destroy (uri);
   sd = mongoc_client_select_server (client, true, NULL, &err);
   mongoc_server_description_destroy (sd);
   reset_all_counters ();
   future = future_client_command_simple (
      client, "test", tmp_bson ("{'ping': 1}"), NULL, NULL, &err);
   request = mock_server_receives_msg (
      server, MONGOC_QUERY_NONE, tmp_bson ("{'ping': 1}"));
   _mongoc_usleep (350);
   request_destroy (request);
   ret = future_get_bool (future);
   BSON_ASSERT (!ret);
   future_destroy (future);
   /* can't ASSERT == because the mock server times out normally reading. */
   DIFF_AND_RESET (streams_timeout, >=, 1);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}
#endif

void
test_counters_install (TestSuite *suite)
{
#ifdef MONGOC_ENABLE_SHM_COUNTERS
   TestSuite_AddFull (suite,
                      "/counters/op_msg",
                      test_counters_op_msg,
                      NULL,
                      NULL,
                      test_framework_skip_if_auth,
                      test_framework_skip_if_compressors);
   TestSuite_AddFull (suite,
                      "/counters/op_compressed",
                      test_counters_op_compressed,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_compressors,
                      test_framework_skip_if_auth);
   TestSuite_AddLive (suite, "/counters/cursors", test_counters_cursors);
   TestSuite_AddLive (suite, "/counters/clients", test_counters_clients);
   TestSuite_AddFull (suite,
                      "/counters/streams",
                      test_counters_streams,
                      NULL,
                      NULL,
                      TestSuite_CheckLive);
   TestSuite_AddFull (suite,
                      "/counters/auth",
                      test_counters_auth,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_auth,
                      test_framework_skip_if_not_single);
   TestSuite_AddLive (suite, "/counters/dns", test_counters_dns);
   TestSuite_AddMockServerTest (
      suite, "/counters/streams_timeout", test_counters_streams_timeout);
#endif
}
