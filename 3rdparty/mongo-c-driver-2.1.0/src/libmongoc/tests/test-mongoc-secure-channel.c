#include "./TestSuite.h"
#include "./test-libmongoc.h"

#include <mongoc/mongoc.h>

#if defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
#include <mongoc/mongoc-client-private.h>     // mongoc_client_connect_tcp
#include <mongoc/mongoc-host-list-private.h>  // _mongoc_host_list_from_string_with_err
#include <mongoc/mongoc-log-private.h>        // _mongoc_log_get_handler
#include <mongoc/mongoc-stream-tls-private.h> // _mongoc_stream_tls_t
#include <mongoc/mongoc-stream-tls-secure-channel-private.h>

#include <mlib/test.h>

#include <test-conveniences.h>

static mongoc_stream_t *
connect_with_secure_channel_cred (const mongoc_ssl_opt_t *ssl_opt, mongoc_shared_ptr cred_ptr, bson_error_t *error)
{
   mongoc_host_list_t host;
   const int32_t connect_timout_ms = 10000;

   *error = (bson_error_t) {0};

   if (!_mongoc_host_list_from_string_with_err (&host, "localhost:27017", error)) {
      return false;
   }
   mongoc_stream_t *tcp_stream = mongoc_client_connect_tcp (connect_timout_ms, &host, error);
   if (!tcp_stream) {
      return false;
   }

   mongoc_stream_t *tls_stream = mongoc_stream_tls_secure_channel_new_with_creds (tcp_stream, ssl_opt, cred_ptr);
   if (!tls_stream) {
      mongoc_stream_destroy (tcp_stream);
      return false;
   }

   if (!mongoc_stream_tls_handshake_block (tls_stream, host.host, connect_timout_ms, error)) {
      mongoc_stream_destroy (tls_stream);
      return false;
   }

   return tls_stream;
}

// Test a TLS stream can be create with shared Secure Channel credentials.
static void
test_secure_channel_shared_creds_stream (void *unused)
{
   BSON_UNUSED (unused);

   bson_error_t error;
   const mongoc_ssl_opt_t ssl_opt = {.ca_file = CERT_TEST_DIR "/ca.pem", .pem_file = CERT_TEST_DIR "/client.pem"};
   // Test with no sharing:
   {
      mongoc_stream_t *stream = connect_with_secure_channel_cred (&ssl_opt, MONGOC_SHARED_PTR_NULL, &error);
      ASSERT_OR_PRINT (stream, error);
      mongoc_stream_destroy (stream);
   }

   // Test with sharing:
   {
      mongoc_shared_ptr cred_ptr =
         mongoc_shared_ptr_create (mongoc_secure_channel_cred_new (&ssl_opt), mongoc_secure_channel_cred_deleter);
      {
         mongoc_stream_t *stream = connect_with_secure_channel_cred (&ssl_opt, cred_ptr, &error);
         ASSERT_OR_PRINT (stream, error);
         // Check same credentials are stored on stream:
         {
            mongoc_stream_tls_t *tls_stream = (mongoc_stream_tls_t *) stream;
            mongoc_stream_tls_secure_channel_t *schannel = tls_stream->ctx;
            ASSERT_CMPVOID (schannel->cred_ptr.ptr, ==, cred_ptr.ptr);
         }

         ASSERT_CMPINT (mongoc_shared_ptr_use_count (cred_ptr), ==, 2);
         mongoc_stream_destroy (stream);
         ASSERT_CMPINT (mongoc_shared_ptr_use_count (cred_ptr), ==, 1);
      }

      // Use again:
      {
         mongoc_stream_t *stream = connect_with_secure_channel_cred (&ssl_opt, cred_ptr, &error);
         ASSERT_OR_PRINT (stream, error);
         // Check same credentials are stored on stream:
         {
            mongoc_stream_tls_t *tls_stream = (mongoc_stream_tls_t *) stream;
            mongoc_stream_tls_secure_channel_t *schannel = tls_stream->ctx;
            ASSERT_CMPVOID (schannel->cred_ptr.ptr, ==, cred_ptr.ptr);
         }

         ASSERT_CMPINT (mongoc_shared_ptr_use_count (cred_ptr), ==, 2);
         mongoc_stream_destroy (stream);
         ASSERT_CMPINT (mongoc_shared_ptr_use_count (cred_ptr), ==, 1);
      }
      mongoc_shared_ptr_reset_null (&cred_ptr);
   }

   // Test with bad SCHANNEL_CRED to exercise error path:
   {
      mongoc_secure_channel_cred *cred = mongoc_secure_channel_cred_new (&ssl_opt);
      mongoc_shared_ptr cred_ptr = mongoc_shared_ptr_create (cred, mongoc_secure_channel_cred_deleter);
      cred->cred.dwVersion = 0; // Invalid version.
      capture_logs (true);
      mongoc_stream_t *stream = connect_with_secure_channel_cred (&ssl_opt, cred_ptr, &error);
      ASSERT (!stream);
      ASSERT_CAPTURED_LOG ("schannel", MONGOC_LOG_LEVEL_ERROR, "Failed to initialize security context");
      mongoc_shared_ptr_reset_null (&cred_ptr);
   }
}

typedef struct {
   size_t failures;
   size_t failures2;
} cert_failures;

static void
count_cert_failures (mongoc_log_level_t log_level, const char *log_domain, const char *message, void *user_data)
{
   BSON_UNUSED (log_level);
   BSON_UNUSED (log_domain);
   cert_failures *cf = user_data;
   if (strstr (message, "Failed to open file: 'does-not-exist.pem'")) {
      cf->failures++;
   }
   if (strstr (message, "Failed to open file: 'does-not-exist-2.pem'")) {
      cf->failures2++;
   }
}

static bool
try_ping (mongoc_client_t *client, bson_error_t *error)
{
   return mongoc_client_command_simple (client, "admin", tmp_bson (BSON_STR ({"ping" : 1})), NULL, NULL, error);
}

static bool
try_ping_with_reconnect (mongoc_client_t *client, bson_error_t *error)
{
   // Force a connection error with a failpoint:
   if (!mongoc_client_command_simple (client,
                                      "admin",
                                      tmp_bson (BSON_STR ({
                                         "configureFailPoint" : "failCommand",
                                         "mode" : {"times" : 1},
                                         "data" : {"closeConnection" : true, "failCommands" : ["ping"]}
                                      })),
                                      NULL,
                                      NULL,
                                      error)) {
      return false;
   }

   // Expect first ping to fail:
   if (try_ping (client, error)) {
      bson_set_error (error, 0, 0, "unexpected: ping succeeded, but expected to fail");
      return false;
   }

   // Ping again:
   return try_ping (client, error);
}

static void
test_secure_channel_shared_creds_client (void *unused)
{
   BSON_UNUSED (unused);

   bson_error_t error;

   // Save log function:
   mongoc_log_func_t saved_log_func;
   void *saved_log_data;
   _mongoc_log_get_handler (&saved_log_func, &saved_log_data);

   // Set log function to count failed attempts to load client cert:
   cert_failures cf = {0};
   mongoc_log_set_handler (count_cert_failures, &cf);

   // Test client:
   {
      mongoc_client_t *client = test_framework_new_default_client ();

      // Set client cert to a bad path:
      {
         mongoc_ssl_opt_t ssl_opt = *test_framework_get_ssl_opts ();
         ssl_opt.pem_file = "does-not-exist.pem";
         mongoc_client_set_ssl_opts (client, &ssl_opt);
      }

      // Expect insert OK. Cert fails to load, but server configured with --tlsAllowConnectionsWithoutCertificates:
      {
         bool ok = try_ping (client, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      // Expect exactly one attempt to load the client cert:
      mlib_check (1, eq, cf.failures);
      mongoc_client_destroy (client);
   }

   cf = (cert_failures) {0};

   // Test pool:
   {
      mongoc_client_pool_t *pool = test_framework_new_default_client_pool ();

      // Set client cert to a bad path:
      {
         mongoc_ssl_opt_t ssl_opt = *test_framework_get_ssl_opts ();
         ssl_opt.pem_file = "does-not-exist.pem";
         mongoc_client_pool_set_ssl_opts (pool, &ssl_opt);
      }

      mongoc_client_t *client = mongoc_client_pool_pop (pool);

      // Expect insert OK. Cert fails to load, but server configured with --tlsAllowConnectionsWithoutCertificates:
      {
         bool ok = try_ping (client, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      mongoc_client_pool_push (pool, client);

      // Expect exactly one attempt to load the client cert:
      mlib_check (1, eq, cf.failures);

      mongoc_client_pool_destroy (pool);
   }

   cf = (cert_failures) {0};

   // Test client changing TLS options after connecting:
   {
      // Changing TLS options after connecting is prohibited on a client pool, but not on a single-threaded client.
      // It is not a documented feature, but is tested for OpenSSL.
      mongoc_client_t *client = test_framework_new_default_client ();

      // Set client cert to a bad path:
      {
         mongoc_ssl_opt_t ssl_opt = *test_framework_get_ssl_opts ();
         ssl_opt.pem_file = "does-not-exist.pem";
         mongoc_client_set_ssl_opts (client, &ssl_opt);
      }

      // Expect insert OK. Cert fails to load, but server configured with --tlsAllowConnectionsWithoutCertificates:
      {
         bool ok = try_ping (client, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      // Expect exactly one attempt to load the client cert:
      mlib_check (1, eq, cf.failures);
      mlib_check (0, eq, cf.failures2);

      // Change the client cert:
      {
         mongoc_ssl_opt_t ssl_opt = *test_framework_get_ssl_opts ();
         ssl_opt.pem_file = "does-not-exist-2.pem";
         mongoc_client_set_ssl_opts (client, &ssl_opt);
      }

      // Force a reconnect.
      {
         bool ok = try_ping_with_reconnect (client, &error);
         ASSERT_OR_PRINT (ok, error);
      }

      // Expect an attempt to load the new cert:
      mlib_check (1, eq, cf.failures); // Unchanged.
      mlib_check (1, eq, cf.failures2);

      mongoc_client_destroy (client);
   }

   // Restore log handler:
   mongoc_log_set_handler (saved_log_func, saved_log_data);
}

void
test_secure_channel_install (TestSuite *suite)
{
   TestSuite_AddFull (suite,
                      "/secure_channel/shared_creds/stream",
                      test_secure_channel_shared_creds_stream,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_server_ssl);

   TestSuite_AddFull (suite,
                      "/secure_channel/shared_creds/client",
                      test_secure_channel_shared_creds_client,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_server_ssl);
}

#else  // MONGOC_ENABLE_SSL_SECURE_CHANNEL
void
test_secure_channel_install (TestSuite *suite)
{
   BSON_UNUSED (suite);
}
#endif // MONGOC_ENABLE_SSL_SECURE_CHANNEL
