#include <mongoc/mongoc.h>
#include <mock_server/mock-server.h>
#include <mock_server/future.h>
#include <mock_server/future-functions.h>

#include "mongoc/mongoc-crypto-private.h"
#include "mongoc/mongoc-scram-private.h"

#include "TestSuite.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"

#ifdef MONGOC_ENABLE_SSL
static void
test_mongoc_scram_step_username_not_set (void)
{
   mongoc_scram_t scram;
   bool success;
   uint8_t buf[4096] = {0};
   uint32_t buflen = 0;
   bson_error_t error;

   _mongoc_scram_init (&scram, MONGOC_CRYPTO_ALGORITHM_SHA_1);
   _mongoc_scram_set_pass (&scram, "password");

   success = _mongoc_scram_step (
      &scram, buf, buflen, buf, sizeof buf, &buflen, &error);

   ASSERT (!success);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_SCRAM,
                          MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                          "SCRAM Failure: username is not set");

   _mongoc_scram_destroy (&scram);
}

typedef struct {
   const char *original;
   const char *normalized;
   bool should_be_required;
   bool should_succeed;
} sasl_prep_testcase_t;


/* test that an error is reported if the server responds with an iteration
 * count that is less than 4096 */
static void
test_iteration_count (int count, bool should_succeed)
{
   mongoc_scram_t scram;
   uint8_t buf[4096] = {0};
   uint32_t buflen = 0;
   bson_error_t error;
   const char *client_nonce = "YWJjZA==";
   char *server_response;
   bool success;

   server_response = bson_strdup_printf (
      "r=YWJjZA==YWJjZA==,s=r6+P1iLmSJvhrRyuFi6Wsg==,i=%d", count);
   /* set up the scram state to immediately test step 2. */
   _mongoc_scram_init (&scram, MONGOC_CRYPTO_ALGORITHM_SHA_1);
   _mongoc_scram_set_pass (&scram, "password");
   bson_strncpy (
      scram.encoded_nonce, client_nonce, sizeof (scram.encoded_nonce));
   scram.encoded_nonce_len = (int32_t) strlen (client_nonce);
   scram.auth_message = bson_malloc0 (4096);
   scram.auth_messagemax = 4096;
   /* prepare the server's "response" from step 1 as the input for step 2. */
   memcpy (buf, server_response, strlen (server_response) + 1);
   buflen = (int32_t) strlen (server_response);
   scram.step = 1;
   success = _mongoc_scram_step (
      &scram, buf, buflen, buf, sizeof buf, &buflen, &error);
   if (should_succeed) {
      ASSERT_OR_PRINT (success, error);
   } else {
      BSON_ASSERT (!success);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_SCRAM,
                             MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                             "SCRAM Failure: iterations must be at least 4096");
   }
   bson_free (server_response);
   _mongoc_scram_destroy (&scram);
}

static void
test_mongoc_scram_iteration_count (void)
{
   test_iteration_count (1000, false);
   test_iteration_count (4095, false);
   test_iteration_count (4096, true);
   test_iteration_count (10000, true);
}

static void
test_mongoc_scram_sasl_prep (void)
{
#ifdef MONGOC_ENABLE_ICU
   int i, ntests;
   char *normalized;
   bson_error_t err;
   /* examples from RFC 4013 section 3. */
   sasl_prep_testcase_t tests[] = {{"\x65\xCC\x81", "\xC3\xA9", true, true},
                                   {"I\xC2\xADX", "IX", true, true},
                                   {"user", "user", false, true},
                                   {"USER", "USER", false, true},
                                   {"\xC2\xAA", "a", true, true},
                                   {"\xE2\x85\xA8", "IX", true, true},
                                   {"\x07", "(invalid)", true, false},
                                   {"\xD8\xA7\x31", "(invalid)", true, false}};
   ntests = sizeof (tests) / sizeof (sasl_prep_testcase_t);
   for (i = 0; i < ntests; i++) {
      ASSERT_CMPINT (tests[i].should_be_required,
                     ==,
                     _mongoc_sasl_prep_required (tests[i].original));
      memset (&err, 0, sizeof (err));
      normalized = _mongoc_sasl_prep (
         tests[i].original, strlen (tests[i].original), &err);
      if (tests[i].should_succeed) {
         ASSERT_CMPSTR (tests[i].normalized, normalized);
         ASSERT_CMPINT (err.code, ==, 0);
         bson_free (normalized);
      } else {
         ASSERT_CMPINT (err.code, ==, MONGOC_ERROR_SCRAM_PROTOCOL_ERROR);
         ASSERT_CMPINT (err.domain, ==, MONGOC_ERROR_SCRAM);
         BSON_ASSERT (normalized == NULL);
      }
   }
#endif
}
#endif

static void
_create_scram_users (void)
{
   mongoc_client_t *client;
   bool res;
   bson_error_t error;
   client = test_framework_new_default_client ();
   res = mongoc_client_command_simple (
      client,
      "admin",
      tmp_bson ("{'createUser': 'sha1', 'pwd': 'sha1', 'roles': ['root'], "
                "'mechanisms': ['SCRAM-SHA-1']}"),
      NULL /* read_prefs */,
      NULL /* reply */,
      &error);
   ASSERT_OR_PRINT (res, error);
   res = mongoc_client_command_simple (
      client,
      "admin",
      tmp_bson ("{'createUser': 'sha256', 'pwd': 'sha256', 'roles': ['root'], "
                "'mechanisms': ['SCRAM-SHA-256']}"),
      NULL /* read_prefs */,
      NULL /* reply */,
      &error);
   ASSERT_OR_PRINT (res, error);
   res = mongoc_client_command_simple (
      client,
      "admin",
      tmp_bson ("{'createUser': 'both', 'pwd': 'both', 'roles': ['root'], "
                "'mechanisms': ['SCRAM-SHA-1', 'SCRAM-SHA-256']}"),
      NULL /* read_prefs */,
      NULL /* reply */,
      &error);
   ASSERT_OR_PRINT (res, error);
   mongoc_client_destroy (client);
}

static void
_drop_scram_users (void)
{
   mongoc_client_t *client;
   mongoc_database_t *db;
   bool res;
   bson_error_t error;
   client = test_framework_new_default_client ();
   db = mongoc_client_get_database (client, "admin");
   res = mongoc_database_remove_user (db, "sha1", &error);
   ASSERT_OR_PRINT (res, error);
   res = mongoc_database_remove_user (db, "sha256", &error);
   ASSERT_OR_PRINT (res, error);
   res = mongoc_database_remove_user (db, "both", &error);
   ASSERT_OR_PRINT (res, error);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}

static void
_check_mechanism (bool pooled,
                  const char *client_mech,
                  const char *server_mechs,
                  const char *expected_used_mech)
{
   mock_server_t *server;
   mongoc_client_pool_t *client_pool = NULL;
   mongoc_client_t *client = NULL;
   mongoc_uri_t *uri;
   future_t *future;
   request_t *request;
   const bson_t *sasl_doc;
   const char *used_mech;

   server = mock_server_new ();
   mock_server_auto_hello (server,
                           "{'ok': 1,"
                           " 'minWireVersion': %d,"
                           " 'maxWireVersion': %d,"
                           " 'isWritablePrimary': true,"
                           " 'saslSupportedMechs': [%s]}",
                           WIRE_VERSION_MIN,
                           WIRE_VERSION_MAX,
                           server_mechs ? server_mechs : "");

   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_username (uri, "user");
   mongoc_uri_set_password (uri, "password");
   if (client_mech) {
      mongoc_uri_set_auth_mechanism (uri, client_mech);
   }

   if (pooled) {
      client_pool = test_framework_client_pool_new_from_uri (uri, NULL);
      client = mongoc_client_pool_pop (client_pool);
      /* suppress the auth failure logs from pooled clients. */
      capture_logs (true);
   } else {
      client = test_framework_client_new_from_uri (uri, NULL);
   }
   future = future_client_command_simple (client,
                                          "admin",
                                          tmp_bson ("{'dbstats': 1}"),
                                          NULL /* read_prefs. */,
                                          NULL /* reply. */,
                                          NULL /* error. */);
   request =
      mock_server_receives_msg (server, MONGOC_QUERY_NONE, tmp_bson ("{}"));
   sasl_doc = request_get_doc (request, 0);
   used_mech = bson_lookup_utf8 (sasl_doc, "mechanism");
   ASSERT_CMPSTR (used_mech, expected_used_mech);
   /* we're not actually going to auth, just hang up. */
   mock_server_hangs_up (request);
   future_wait (future);
   future_destroy (future);
   request_destroy (request);
   mongoc_uri_destroy (uri);
   if (pooled) {
      mongoc_client_pool_push (client_pool, client);
      mongoc_client_pool_destroy (client_pool);
      capture_logs (false);
   } else {
      mongoc_client_destroy (client);
   }
   mock_server_destroy (server);
}

typedef enum {
   MONGOC_TEST_NO_ERROR,
   MONGOC_TEST_USER_NOT_FOUND_ERROR,
   MONGOC_TEST_AUTH_ERROR,
   MONGOC_TEST_NO_ICU_ERROR
} test_error_t;

void
_check_error (const bson_error_t *error, test_error_t expected_error)
{
   int32_t domain = 0;
   int32_t code = 0;
   const char *message = "";

   switch (expected_error) {
   case MONGOC_TEST_AUTH_ERROR:
      domain = MONGOC_ERROR_CLIENT;
      code = MONGOC_ERROR_CLIENT_AUTHENTICATE;
      message = "Authentication failed";
      break;
   case MONGOC_TEST_USER_NOT_FOUND_ERROR:
      domain = MONGOC_ERROR_CLIENT;
      code = MONGOC_ERROR_CLIENT_AUTHENTICATE;
      message = "Could not find user";
      break;
   case MONGOC_TEST_NO_ICU_ERROR:
      domain = MONGOC_ERROR_SCRAM;
      code = MONGOC_ERROR_SCRAM_PROTOCOL_ERROR;
      message = "SCRAM Failure: ICU required to SASLPrep password";
      break;
   case MONGOC_TEST_NO_ERROR:
   default:
      return;
   }

   ASSERT_ERROR_CONTAINS ((*error), domain, code, message);
}

static void
_try_auth_from_uri (bool pooled, mongoc_uri_t *uri, test_error_t expected_error)
{
   mongoc_client_pool_t *client_pool = NULL;
   mongoc_client_t *client = NULL;
   mongoc_collection_t *coll;
   bson_error_t error;
   bson_t reply;
   bool res;

   if (pooled) {
      client_pool = test_framework_client_pool_new_from_uri (uri, NULL);
      test_framework_set_pool_ssl_opts (client_pool);
      mongoc_client_pool_set_error_api (client_pool, 2);
      client = mongoc_client_pool_pop (client_pool);
      /* suppress the auth failure logs from pooled clients. */
      capture_logs (true);
   } else {
      client = test_framework_client_new_from_uri (uri, NULL);
      mongoc_client_set_error_api (client, 2);
      test_framework_set_ssl_opts (client);
   }
   coll = get_test_collection (client, "try_auth");
   res = mongoc_collection_insert_one (
      coll, tmp_bson ("{'x': 1}"), NULL /* opts */, &reply, &error);

   if (expected_error == MONGOC_TEST_NO_ERROR) {
      ASSERT_OR_PRINT (res, error);
      ASSERT_MATCH (&reply, "{'insertedCount': 1 }");
   } else {
      ASSERT (!res);
      _check_error (&error, expected_error);
   }
   bson_destroy (&reply);
   mongoc_collection_destroy (coll);
   if (pooled) {
      mongoc_client_pool_push (client_pool, client);
      mongoc_client_pool_destroy (client_pool);
      capture_logs (false);
   } else {
      mongoc_client_destroy (client);
   }
}

/* if auth is expected to succeed, expected_error is zero'd out. */
static void
_try_auth (bool pooled,
           const char *user,
           const char *pwd,
           const char *mechanism,
           test_error_t expected_error)
{
   mongoc_uri_t *uri;

   uri = test_framework_get_uri ();
   mongoc_uri_set_username (uri, user);
   mongoc_uri_set_password (uri, pwd);
   if (mechanism) {
      mongoc_uri_set_auth_mechanism (uri, mechanism);
   }
   _try_auth_from_uri (pooled, uri, expected_error);
   mongoc_uri_destroy (uri);
}


static void
_test_mongoc_scram_auth (bool pooled)
{
   /* Auth spec: "For each test user, verify that you can connect and run a
   command requiring authentication for the following cases:
   - Explicitly specifying each mechanism the user supports.
   - Specifying no mechanism and relying on mechanism negotiation." */
   _try_auth (pooled, "sha1", "sha1", NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, "sha1", "sha1", "SCRAM-SHA-1", MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, "sha256", "sha256", NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (
      pooled, "sha256", "sha256", "SCRAM-SHA-256", MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, "both", "both", NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, "both", "both", "SCRAM-SHA-1", MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, "both", "both", "SCRAM-SHA-256", MONGOC_TEST_NO_ERROR);

   _check_mechanism (pooled, NULL, NULL, "SCRAM-SHA-1");
   _check_mechanism (pooled, NULL, "'SCRAM-SHA-1'", "SCRAM-SHA-1");
   _check_mechanism (pooled, NULL, "'SCRAM-SHA-256'", "SCRAM-SHA-256");
   _check_mechanism (
      pooled, NULL, "'SCRAM-SHA-1','SCRAM-SHA-256'", "SCRAM-SHA-256");

   _check_mechanism (pooled, "SCRAM-SHA-1", NULL, "SCRAM-SHA-1");
   _check_mechanism (pooled, "SCRAM-SHA-1", "'SCRAM-SHA-1'", "SCRAM-SHA-1");
   _check_mechanism (pooled, "SCRAM-SHA-1", "'SCRAM-SHA-256'", "SCRAM-SHA-1");
   _check_mechanism (
      pooled, "SCRAM-SHA-1", "'SCRAM-SHA-1','SCRAM-SHA-256'", "SCRAM-SHA-1");

   _check_mechanism (pooled, "SCRAM-SHA-256", NULL, "SCRAM-SHA-256");
   _check_mechanism (pooled, "SCRAM-SHA-256", "'SCRAM-SHA-1'", "SCRAM-SHA-256");
   _check_mechanism (
      pooled, "SCRAM-SHA-256", "'SCRAM-SHA-256'", "SCRAM-SHA-256");
   _check_mechanism (pooled,
                     "SCRAM-SHA-256",
                     "'SCRAM-SHA-1','SCRAM-SHA-256'",
                     "SCRAM-SHA-256");

   /* Test some failure auths. */
   _try_auth (pooled, "sha1", "bad", NULL, MONGOC_TEST_AUTH_ERROR);
   _try_auth (pooled, "sha256", "bad", NULL, MONGOC_TEST_AUTH_ERROR);
   _try_auth (pooled, "both", "bad", NULL, MONGOC_TEST_AUTH_ERROR);
   _try_auth (pooled, "sha1", "bad", "SCRAM-SHA-256", MONGOC_TEST_AUTH_ERROR);
   _try_auth (pooled, "sha256", "bad", "SCRAM-SHA-1", MONGOC_TEST_AUTH_ERROR);

   /* Auth spec: "For a non-existent username, verify that not specifying a
    * mechanism when connecting fails with the same error type that would occur
    * with a correct username but incorrect password or mechanism." */
   _try_auth (pooled, "unknown_user", "bad", NULL, MONGOC_TEST_AUTH_ERROR);
}

/* test the auth tests described in the auth spec. */
static void
test_mongoc_scram_auth (void *ctx)
{
   /* Auth spec: "Create three test users, one with only SHA-1, one with only
    * SHA-256 and one with both" */
   _create_scram_users ();
   _test_mongoc_scram_auth (false);
   _test_mongoc_scram_auth (true);
   _drop_scram_users ();
}

static int
_skip_if_no_sha256 ()
{
   mongoc_client_t *client;
   bool res;
   bson_error_t error;

   client = test_framework_new_default_client ();

   /* Check if SCRAM-SHA-256 is a supported auth mechanism by attempting to
    * create a new user with it. */
   res = mongoc_client_command_simple (
      client,
      "admin",
      tmp_bson ("{'createUser': 'temp', 'pwd': 'sha256', 'roles': ['root'], "
                "'mechanisms': ['SCRAM-SHA-256']}"),
      NULL /* read_prefs */,
      NULL /* reply */,
      &error);

   if (res) {
      mongoc_database_t *db;

      db = mongoc_client_get_database (client, "admin");
      ASSERT_OR_PRINT (mongoc_database_remove_user (db, "temp", &error), error);
      mongoc_database_destroy (db);
   }

   mongoc_client_destroy (client);
   return res ? 1 : 0;
}

#define ROMAN_NUMERAL_NINE "\xE2\x85\xA8"
#define ROMAN_NUMERAL_FOUR "\xE2\x85\xA3"

static int
skip_if_no_icu (void)
{
#ifdef MONGOC_ENABLE_ICU
   return true;
#else
   return false;
#endif
}

static int
skip_if_icu (void)
{
   return !skip_if_no_icu ();
}

static void
_create_saslprep_users ()
{
   mongoc_client_t *client;
   bool res;
   bson_error_t error;
   client = test_framework_new_default_client ();
   res = mongoc_client_command_simple (
      client,
      "admin",
      tmp_bson ("{'createUser': 'IX', 'pwd': 'IX', 'roles': ['root'], "
                "'mechanisms': ['SCRAM-SHA-256']}"),
      NULL /* read_prefs */,
      NULL /* reply */,
      &error);
   ASSERT_OR_PRINT (res, error);
   res = mongoc_client_command_simple (
      client,
      "admin",
      tmp_bson ("{'createUser': '" ROMAN_NUMERAL_NINE
                "', 'pwd': '" ROMAN_NUMERAL_FOUR
                "', 'roles': ['root'], 'mechanisms': ['SCRAM-SHA-256']}"),
      NULL /* read_prefs */,
      NULL /* reply */,
      &error);
   ASSERT_OR_PRINT (res, error);
   mongoc_client_destroy (client);
}

static void
_drop_saslprep_users ()
{
   mongoc_client_t *client;
   mongoc_database_t *db;
   bool res;
   bson_error_t error;
   client = test_framework_new_default_client ();
   db = mongoc_client_get_database (client, "admin");
   res = mongoc_database_remove_user (db, "IX", &error);
   ASSERT_OR_PRINT (res, error);
   res = mongoc_database_remove_user (db, ROMAN_NUMERAL_NINE, &error);
   ASSERT_OR_PRINT (res, error);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}

static void
_make_uri (const char *username, const char *password, mongoc_uri_t **out)
{
   char *uri_str;
   char *tmp;

   tmp = test_framework_get_uri_str_no_auth ("admin");
   uri_str = test_framework_add_user_password (tmp, username, password);
   mongoc_uri_destroy (*out);
   *out = mongoc_uri_new (uri_str);
   bson_free (tmp);
   bson_free (uri_str);
}

static void
_test_mongoc_scram_saslprep_auth (bool pooled)
{
   mongoc_uri_t *uri = NULL;

   /* Test URIs of the form in the auth spec test plan for SASLPrep.
      - mongodb://IX:IX@mongodb.example.com/admin
      - mongodb://IX:I%C2%ADX@mongodb.example.com/admin
      - mongodb://%E2%85%A8:IV@mongodb.example.com/admin
      - mongodb://%E2%85%A8:I%C2%ADV@mongodb.example.com/admin

      Test in three ways.
      1. By embedding the multi-byte UTF-8 characters directly into the
      connection string.
      2. By percent escaping the multi-byte UTF-8 characters.
      3. By using the setters, mongoc_uri_set_username/mongoc_uri_set_password
      and embedding the UTF-8 characters (percent unescaping does not occur for
      the setters)
   */

   /* Way 1: embedding multi-byte UTF-8 characters directly */
   _make_uri ("IX", "IX", &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);

   _make_uri ("IX", ROMAN_NUMERAL_NINE, &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);

   _make_uri (ROMAN_NUMERAL_NINE, "IV", &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);

   _make_uri (ROMAN_NUMERAL_NINE, ROMAN_NUMERAL_FOUR, &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);

   /* Way 2: Percent escaping */
   _make_uri ("IX", "IX", &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);

   _make_uri ("IX", "I%C2%ADX", &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);

   _make_uri ("%E2%85%A8", "IV", &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);

   _make_uri ("%E2%85%A8", "I%C2%ADV", &uri);
   _try_auth_from_uri (pooled, uri, MONGOC_TEST_NO_ERROR);
   mongoc_uri_destroy (uri);

   /* Way 3: with username/password setters. */
   _try_auth (pooled, "IX", "IX", NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, "IX", ROMAN_NUMERAL_NINE, NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, ROMAN_NUMERAL_NINE, "IV", NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (pooled,
              ROMAN_NUMERAL_NINE,
              ROMAN_NUMERAL_FOUR,
              NULL,
              MONGOC_TEST_NO_ERROR);
}


static void
test_mongoc_saslprep_auth (void *ctx)
{
   _create_saslprep_users ();
   _test_mongoc_scram_saslprep_auth (false);
   _test_mongoc_scram_saslprep_auth (true);
   _drop_saslprep_users ();
}


static void
_test_mongoc_scram_saslprep_auth_no_icu (bool pooled)
{
   _try_auth (pooled, "IX", "IX", NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (pooled, "IX", ROMAN_NUMERAL_NINE, NULL, MONGOC_TEST_NO_ICU_ERROR);
   _try_auth (pooled, ROMAN_NUMERAL_NINE, "IV", NULL, MONGOC_TEST_NO_ERROR);
   _try_auth (pooled,
              ROMAN_NUMERAL_NINE,
              ROMAN_NUMERAL_FOUR,
              NULL,
              MONGOC_TEST_NO_ICU_ERROR);
}

static void
test_mongoc_saslprep_auth_no_icu (void *ctx)
{
   _create_saslprep_users ();
   _test_mongoc_scram_saslprep_auth_no_icu (false);
   _test_mongoc_scram_saslprep_auth_no_icu (true);
   _drop_saslprep_users ();
}

void
test_scram_install (TestSuite *suite)
{
#ifdef MONGOC_ENABLE_SSL
   TestSuite_Add (suite,
                  "/scram/username_not_set",
                  test_mongoc_scram_step_username_not_set);
   TestSuite_Add (suite, "/scram/sasl_prep", test_mongoc_scram_sasl_prep);
   TestSuite_Add (
      suite, "/scram/iteration_count", test_mongoc_scram_iteration_count);
#endif
   TestSuite_AddFull (suite,
                      "/scram/auth_tests",
                      test_mongoc_scram_auth,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_no_auth,
                      _skip_if_no_sha256,
                      TestSuite_CheckLive);
   TestSuite_AddFull (suite,
                      "/scram/saslprep_auth",
                      test_mongoc_saslprep_auth,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_no_auth,
                      _skip_if_no_sha256,
                      skip_if_no_icu,
                      TestSuite_CheckLive);
   TestSuite_AddFull (suite,
                      "/scram/saslprep_auth_no_icu",
                      test_mongoc_saslprep_auth_no_icu,
                      NULL /* dtor */,
                      NULL /* ctx */,
                      test_framework_skip_if_no_auth,
                      _skip_if_no_sha256,
                      skip_if_icu,
                      TestSuite_CheckLive);
}
