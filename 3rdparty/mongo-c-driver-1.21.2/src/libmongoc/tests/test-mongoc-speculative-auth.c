/*
 * Copyright 2020-present MongoDB, Inc.
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

#include <mongoc/mongoc.h>
#ifdef _POSIX_VERSION
#include <sys/utsname.h>
#endif

#include "mongoc/mongoc-client-private.h"
#include "mongoc/mongoc-handshake.h"
#include "mongoc/mongoc-handshake-private.h"

#include "TestSuite.h"
#include "test-libmongoc.h"
#include "test-conveniences.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"

typedef void (*setup_uri_options_t) (mongoc_uri_t *uri);
typedef void (*compare_auth_command_t) (bson_t *auth_command);
typedef void (*post_handshake_commands_t) (mock_server_t *server);

#ifdef MONGOC_ENABLE_CRYPTO

/* For single threaded clients, we execute a command to cause a hello to be
 * sent */
static future_t *
_force_hello_with_ping (mongoc_client_t *client)
{
   future_t *future;

   /* Send a ping */
   future = future_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, NULL);

   ASSERT (future);
   return future;
}

/* Call after we've dealt with the hello sent by
 * _force_hello_with_ping */
static void
_respond_to_ping (future_t *future, mock_server_t *server, bool expect_ping)
{
   request_t *request;

   ASSERT (future);

   if (!expect_ping) {
      BSON_ASSERT (!future_get_bool (future));
      future_destroy (future);

      return;
   }

   request = mock_server_receives_msg (
      server, MONGOC_MSG_NONE, tmp_bson ("{'$db': 'admin', 'ping': 1}"));

   ASSERT (request);

   mock_server_replies_simple (request, "{'ok': 1}");

   ASSERT (future_get_bool (future));
   request_destroy (request);
   future_destroy (future);
}

static bool
_auto_hello_without_speculative_auth (request_t *request, void *data)
{
   const char *response_json = (const char *) data;
   char *quotes_replaced;

   if (!request->is_command) {
      return false;
   }

   if (strcasecmp (request->command_name, HANDSHAKE_CMD_LEGACY_HELLO) &&
       strcmp (request->command_name, "hello")) {
      return false;
   }

   if (bson_has_field (request_get_doc (request, 0),
                       "speculativeAuthenticate")) {
      return false;
   }

   quotes_replaced = single_quotes_to_double (response_json);

   if (mock_server_get_rand_delay (request->server)) {
      _mongoc_usleep ((int64_t) (rand () % 10) * 1000);
   }

   mock_server_replies (request, MONGOC_REPLY_NONE, 0, 0, 1, response_json);

   bson_free (quotes_replaced);
   request_destroy (request);
   return true;
}

static void
_test_mongoc_speculative_auth (bool pooled,
                               bool use_ssl,
                               setup_uri_options_t setup_uri_options,
                               bool includes_speculative_auth,
                               compare_auth_command_t compare_auth_command,
                               bson_t *speculative_auth_response,
                               post_handshake_commands_t post_hello_commands,
                               bool expect_successful_ping)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool = NULL;
   future_t *future;

   mongoc_ssl_opt_t client_ssl_opts = {0};
   mongoc_ssl_opt_t server_ssl_opts = {0};
   client_ssl_opts.ca_file = CERT_CA;
   client_ssl_opts.pem_file = CERT_CLIENT;
   server_ssl_opts.ca_file = CERT_CA;
   server_ssl_opts.pem_file = CERT_SERVER;

   server = mock_server_new ();

#ifdef MONGOC_ENABLE_SSL
   if (use_ssl) {
      mock_server_set_ssl_opts (server, &server_ssl_opts);
   }
#endif

   mock_server_autoresponds (server,
                             _auto_hello_without_speculative_auth,
                             (void *) tmp_str ("{'ok': 1,"
                                               " 'isWritablePrimary': true,"
                                               " 'minWireVersion': %d,"
                                               " 'maxWireVersion': %d}",
                                               WIRE_VERSION_MIN,
                                               WIRE_VERSION_MAX),
                             NULL);

   mock_server_run (server);

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 15000);

   if (setup_uri_options) {
      setup_uri_options (uri);
   }

   if (pooled) {
      pool = test_framework_client_pool_new_from_uri (uri, NULL);

#ifdef MONGOC_ENABLE_SSL
      if (use_ssl) {
         mongoc_client_pool_set_ssl_opts (pool, &client_ssl_opts);
      }
#endif

      /* Force topology scanner to start */
      client = mongoc_client_pool_pop (pool);
      /* suppress the auth failure logs from pooled clients. */
      capture_logs (true);
   } else {
      client = test_framework_client_new_from_uri (uri, NULL);

#ifdef MONGOC_ENABLE_SSL
      if (use_ssl) {
         mongoc_client_set_ssl_opts (client, &client_ssl_opts);
      }
#endif
   }

   future = _force_hello_with_ping (client);

   if (includes_speculative_auth) {
      request_t *request;
      const bson_t *request_doc;
      bson_t *response;
      char *str;

      request = mock_server_receives_legacy_hello (server, NULL);
      ASSERT (request);
      request_doc = request_get_doc (request, 0);
      ASSERT (request_doc);
      ASSERT (bson_has_field (request_doc, "speculativeAuthenticate"));

      if (compare_auth_command) {
         bson_t auth_cmd;

         bson_lookup_doc (request_doc, "speculativeAuthenticate", &auth_cmd);
         compare_auth_command (&auth_cmd);
      }

      /* Include authentication information in response */
      response = BCON_NEW ("ok",
                           BCON_INT32 (1),
                           "isWritablePrimary",
                           BCON_BOOL (true),
                           "minWireVersion",
                           BCON_INT32 (WIRE_VERSION_MIN),
                           "maxWireVersion",
                           BCON_INT32 (WIRE_VERSION_MAX));

      if (speculative_auth_response) {
         BSON_APPEND_DOCUMENT (
            response, "speculativeAuthenticate", speculative_auth_response);
      }

      str = bson_as_canonical_extended_json (response, NULL);
      mock_server_replies_simple (request, str);

      bson_free (str);
      bson_destroy (response);
      request_destroy (request);
   }

   if (post_hello_commands) {
      post_hello_commands (server);
   }

   _respond_to_ping (future, server, expect_successful_ping);

   /* Cleanup */
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
_setup_speculative_auth_x_509 (mongoc_uri_t *uri)
{
   mongoc_uri_set_auth_mechanism (uri, "MONGODB-X509");
   mongoc_uri_set_username (
      uri,
      "CN=myName,OU=myOrgUnit,O=myOrg,L=myLocality,ST=myState,C=myCountry");
}

static void
_compare_auth_cmd_x509 (bson_t *auth_cmd)
{
   bson_t *expected_auth_cmd = BCON_NEW (
      "authenticate",
      BCON_INT32 (1),
      "mechanism",
      BCON_UTF8 ("MONGODB-X509"),
      "user",
      BCON_UTF8 (
         "CN=myName,OU=myOrgUnit,O=myOrg,L=myLocality,ST=myState,C=myCountry"),
      "db",
      BCON_UTF8 ("$external"));

   char *auth_cmd_str = bson_as_canonical_extended_json (auth_cmd, NULL);
   char *expected_auth_cmd_str =
      bson_as_canonical_extended_json (expected_auth_cmd, NULL);

   ASSERT_CMPSTR (auth_cmd_str, expected_auth_cmd_str);

   bson_free (auth_cmd_str);
   bson_free (expected_auth_cmd_str);
   bson_destroy (expected_auth_cmd);
}

static void
_setup_speculative_auth_scram (mongoc_uri_t *uri)
{
   mongoc_uri_set_username (uri, "sasl");
   mongoc_uri_set_password (uri, "sasl");
}

static void
_compare_auth_cmd_scram (bson_t *auth_cmd)
{
   bson_iter_t iter;

   ASSERT (bson_has_field (auth_cmd, "saslStart"));
   ASSERT (bson_has_field (auth_cmd, "payload"));
   ASSERT (bson_iter_init_find (&iter, auth_cmd, "db"));
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "admin");
}

static void
_post_hello_scram_invalid_auth_response (mock_server_t *srv)
{
   request_t *request;
   const bson_t *request_doc;

   request = mock_server_receives_msg (
      srv, MONGOC_MSG_NONE, tmp_bson ("{'$db': 'admin'}"));
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);

   /* Speculative authentication was not successful because the mock server
    * can't respond with a valid scram payload. This results in a new
    * authentication attempt being started using an explicit saslStart command.
    */
   ASSERT_CMPSTR (request->command_name, "saslStart");

   /* Let authentication fail directly since we won't be able to continue the
    * scram conversation. */
   mock_server_replies_simple (
      request, "{ 'ok': 1, 'errmsg': 'Cannot mock scram auth conversation' }");

   request_destroy (request);
}

static void
test_mongoc_speculative_auth_request_none (void)
{
   _test_mongoc_speculative_auth (
      false, false, NULL, false, NULL, NULL, NULL, true);
}

static void
test_mongoc_speculative_auth_request_none_pool (void)
{
   _test_mongoc_speculative_auth (
      true, false, NULL, false, NULL, NULL, NULL, true);
}

static void
test_mongoc_speculative_auth_request_x509 (void)
{
   bson_t *response =
      BCON_NEW ("dbname",
                BCON_UTF8 ("$external"),
                "user",
                BCON_UTF8 ("CN=myName,OU=myOrgUnit,O=myOrg,L=myLocality,ST="
                           "myState,C=myCountry"));

   _test_mongoc_speculative_auth (false,
                                  true,
                                  _setup_speculative_auth_x_509,
                                  true,
                                  _compare_auth_cmd_x509,
                                  response,
                                  NULL,
                                  true);

   bson_destroy (response);
}

static void
test_mongoc_speculative_auth_request_x509_pool (void)
{
   bson_t *response =
      BCON_NEW ("dbname",
                BCON_UTF8 ("$external"),
                "user",
                BCON_UTF8 ("CN=myName,OU=myOrgUnit,O=myOrg,L=myLocality,ST="
                           "myState,C=myCountry"));

   _test_mongoc_speculative_auth (true,
                                  true,
                                  _setup_speculative_auth_x_509,
                                  true,
                                  _compare_auth_cmd_x509,
                                  response,
                                  NULL,
                                  true);

   bson_destroy (response);
}

static void
test_mongoc_speculative_auth_request_scram (void)
{
   bson_t *response = BCON_NEW (
      "conversationId",
      BCON_INT32 (15081984),
      "payload",
      BCON_BIN (BSON_SUBTYPE_BINARY, (const uint8_t *) "deadbeef", 8));

   _test_mongoc_speculative_auth (false,
                                  false,
                                  _setup_speculative_auth_scram,
                                  true,
                                  _compare_auth_cmd_scram,
                                  response,
                                  _post_hello_scram_invalid_auth_response,
                                  false);

   bson_destroy (response);
}

static void
test_mongoc_speculative_auth_request_scram_pool (void)
{
   bson_t *response = BCON_NEW (
      "conversationId",
      BCON_INT32 (15081984),
      "payload",
      BCON_BIN (BSON_SUBTYPE_BINARY, (const uint8_t *) "deadbeef", 8));

   _test_mongoc_speculative_auth (true,
                                  false,
                                  _setup_speculative_auth_scram,
                                  true,
                                  _compare_auth_cmd_scram,
                                  response,
                                  _post_hello_scram_invalid_auth_response,
                                  false);

   bson_destroy (response);
}
#endif /* MONGOC_ENABLE_CRYPTO */

void
test_speculative_auth_install (TestSuite *suite)
{
#ifdef MONGOC_ENABLE_CRYPTO
   TestSuite_AddMockServerTest (suite,
                                "/speculative_auth/request_none",
                                test_mongoc_speculative_auth_request_none);
#if defined(MONGOC_ENABLE_SSL_OPENSSL) || \
   defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
   TestSuite_AddMockServerTest (suite,
                                "/speculative_auth/request_x509",
                                test_mongoc_speculative_auth_request_x509);
#endif /* MONGOC_ENABLE_SSL_* */
   TestSuite_AddMockServerTest (suite,
                                "/speculative_auth/request_scram",
                                test_mongoc_speculative_auth_request_scram);
   TestSuite_AddMockServerTest (suite,
                                "/speculative_auth_pool/request_none",
                                test_mongoc_speculative_auth_request_none_pool);
#if defined(MONGOC_ENABLE_SSL_OPENSSL) || \
   defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
   TestSuite_AddMockServerTest (suite,
                                "/speculative_auth_pool/request_x509",
                                test_mongoc_speculative_auth_request_x509_pool);
#endif /* MONGOC_ENABLE_SSL_* */
   TestSuite_AddMockServerTest (
      suite,
      "/speculative_auth_pool/request_scram",
      test_mongoc_speculative_auth_request_scram_pool);
#endif /* MONGOC_ENABLE_CRYPTO */
}
