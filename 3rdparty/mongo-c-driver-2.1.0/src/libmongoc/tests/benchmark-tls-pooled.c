/*
 * Used as a benchmark for CDRIVER-4656 to test the performance effect of sharing the OpenSSL context among all clients
 * in a pool.
 *
 * TO BUILD: % cmake --build cmake-build --target benchmark-tls-pooled
 * TO RUN: % ./cmake-build/src/libmongoc/benchmark-tls-pooled  [number of clients to check out]
 * The integer argument is optional, if not provided 200 clients are created by default.
 */

#include <mongoc/mongoc.h>

#include <mlib/loop.h>

#include <pthread.h>

#include <stdio.h>


static void *
worker (void *data)
{
   mongoc_client_pool_t *pool = data;
   mongoc_client_t *client;
   mongoc_database_t *database;
   bson_t ping = BSON_INITIALIZER;
   bson_error_t error;
   bool r;

   bson_append_int32 (&ping, "ping", 4, 1);

   client = mongoc_client_pool_pop (pool);

   database = mongoc_client_get_database (client, "test");
   r = mongoc_database_command_with_opts (database, &ping, NULL, NULL, NULL, &error);

   if (!r) {
      fprintf (stderr, "Ping failure: %s\n", error.message);
   }

   mongoc_client_pool_push (pool, client);

   bson_destroy (&ping);
   return NULL;
}

int
main (int argc, char *argv[])
{
   int num_clients = 200;

   if (argc > 1) {
      num_clients = atoi (argv[1]);
   }

   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool;
   pthread_t threads[num_clients];
   void *ret;

   mongoc_init ();

   uri = mongoc_uri_new ("mongodb://localhost:27017/");

   const char *certificate_path;
   const char *ca_path;

   // Use built-in test CA and PEM files.
   certificate_path = "./src/libmongoc/tests/x509gen/client.pem";
   ca_path = "./src/libmongoc/tests/x509gen/ca.pem";

   mongoc_uri_set_option_as_bool (uri, MONGOC_URI_TLS, true);
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, certificate_path);
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, ca_path);

   // Always set maxPoolSize to match client count.
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_MAXPOOLSIZE, num_clients);

   pool = mongoc_client_pool_new (uri);
   mongoc_client_pool_set_error_api (pool, MONGOC_ERROR_API_VERSION_2);

   mlib_foreach_irange (i, num_clients) {
      pthread_create (&threads[i], NULL, worker, pool);
   }

   mlib_foreach_irange (i, num_clients) {
      pthread_join (threads[i], &ret);
   }

   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);

   mongoc_cleanup ();

   return EXIT_SUCCESS;
}
