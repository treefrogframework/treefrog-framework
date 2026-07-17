#include <mongoc/mongoc-uri.h>

//

#include <common-bson-dsl-private.h>
#include <mongoc/mongoc-read-concern-private.h>
#include <mongoc/mongoc-util-private.h>

#include <bson/bson.h>

#include <json-test.h>
#include <test-libmongoc.h>


/*
 * Asserts every key-value pair in 'needle' is in 'haystack', including those in
 * embedded documents.
 */
static void
bson_contains_iter(const bson_t *haystack, bson_iter_t *needle)
{
   bson_iter_t iter;

   if (!bson_iter_next(needle)) {
      return;
   }

   const char *const key = bson_iter_key(needle);

   ASSERT_WITH_MSG(bson_iter_init_find_case(&iter, haystack, key), "'%s' is not present", key);

   const uint32_t bson_type = bson_iter_type(needle);

   switch (bson_type) {
   case BSON_TYPE_ARRAY:
   case BSON_TYPE_DOCUMENT: {
      bson_t sub_bson;
      bson_iter_t sub_iter;

      ASSERT_WITH_MSG(BSON_ITER_HOLDS_DOCUMENT(&iter), "'%s' is not a document", key);
      bson_iter_bson(&iter, &sub_bson);

      bson_iter_recurse(needle, &sub_iter);
      bson_contains_iter(&sub_bson, &sub_iter);

      bson_destroy(&sub_bson);
      return;
   }
   case BSON_TYPE_BOOL:
      ASSERT_WITH_MSG(bson_iter_as_bool(needle) == bson_iter_as_bool(&iter), "'%s' is not the correct value", key);
      bson_contains_iter(haystack, needle);
      return;
   case BSON_TYPE_UTF8:
      ASSERT_CMPSTR(bson_iter_utf8(needle, 0), bson_iter_utf8(&iter, 0));
      bson_contains_iter(haystack, needle);
      return;
   case BSON_TYPE_DOUBLE:
      ASSERT_CMPDOUBLE(bson_iter_double(needle), ==, bson_iter_double(&iter));
      bson_contains_iter(haystack, needle);
      return;
   case BSON_TYPE_INT64:
   case BSON_TYPE_INT32:
      ASSERT_CMPINT64(bson_iter_as_int64(needle), ==, bson_iter_as_int64(&iter));
      bson_contains_iter(haystack, needle);
      return;
   default:
      ASSERT(false);
      return;
   }
}

static void
run_uri_test(const char *uri_string,
             bool valid,
             const bson_t *hosts,
             const bson_t *auth,
             const bson_t *options,
             const bson_t *credentials)
{
   bson_error_t error;

   mongoc_uri_t *const uri = mongoc_uri_new_with_error(uri_string, &error);

   /* BEGIN Exceptions to test suite */

   /* some spec tests assume we allow DB names like "auth.foo" */
   if (auth) {
      bson_iter_t iter;
      if ((bson_iter_init_find(&iter, auth, "db") || bson_iter_init_find(&iter, auth, "source")) &&
          BSON_ITER_HOLDS_UTF8(&iter)) {
         if (strchr(bson_iter_utf8(&iter, NULL), '.')) {
            BSON_ASSERT(!uri);
            ASSERT_ERROR_CONTAINS(
               error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid database specifier \"admin.");
            clear_captured_logs();
            return;
         }
      }
   }

   if (valid && !uri && error.domain) {
      /* Eager failures which the spec expects to be warnings. */
      /* CDRIVER-3167 */
      if (strstr(uri_string, "=invalid") || strstr(uri_string, "heartbeatFrequencyMS=-2") ||
          strstr(uri_string, "w=-2") || strstr(uri_string, "wTimeoutMS=-2") ||
          strstr(uri_string, "zlibCompressionLevel=-2") || strstr(uri_string, "zlibCompressionLevel=10") ||
          (!strstr(uri_string, "mongodb+srv") && strstr(uri_string, "srvServiceName=customname")) ||
          strstr(uri_string, "srvMaxHosts=-1") || strstr(uri_string, "srvMaxHosts=foo")) {
         MONGOC_WARNING("Error parsing URI: '%s'", error.message);
         return;
      }

      // CDRIVER-4128: only legacy boolean values are currently supported.
      if (strstr(uri_string, "CANONICALIZE_HOST_NAME:none") || strstr(uri_string, "CANONICALIZE_HOST_NAME:forward")) {
         return;
      }

      // CDRIVER-5580: commas in TOKEN_RESOURCE are interpreted as a key-value pair delimiter which produces an invalid
      // mechanism property that is diagnosed as a client error instead of a warning.
      if (strstr(uri_string, "TOKEN_RESOURCE:mongodb://host1%2Chost2")) {
         MONGOC_WARNING("percent-encoded commas in TOKEN_RESOURCE");
         return;
      }
   }

   if (uri) {
      /* mongoc does not warn on negative timeouts when it should. */
      /* CDRIVER-3167 */
      if ((mongoc_uri_get_option_as_int32(uri, MONGOC_URI_CONNECTTIMEOUTMS, 0) < 0) ||
          (mongoc_uri_get_option_as_int32(uri, MONGOC_URI_LOCALTHRESHOLDMS, 0) < 0) ||
          (mongoc_uri_get_option_as_int32(uri, MONGOC_URI_SERVERSELECTIONTIMEOUTMS, 0) < 0) ||
          (mongoc_uri_get_option_as_int32(uri, MONGOC_URI_SOCKETTIMEOUTMS, 0) < 0)) {
         MONGOC_WARNING("Invalid negative timeout");
      }

      /* mongoc does not store lists the way the spec test expects. */
      if (strstr(uri_string, "compressors=") || strstr(uri_string, "readPreferenceTags=")) {
         options = NULL;
      }

      /* mongoc eagerly warns about unsupported compressors. */
#ifndef MONGOC_ENABLE_COMPRESSION_SNAPPY
      if (strstr(uri_string, "compressors=snappy")) {
         clear_captured_logs();
      }
#endif

#ifndef MONGOC_ENABLE_COMPRESSION_ZLIB
      if (strstr(uri_string, "compressors=zlib") || strstr(uri_string, "compressors=snappy,zlib")) {
         clear_captured_logs();
      }
#endif
   }

   /* END Exceptions to test suite */

   if (valid) {
      ASSERT_OR_PRINT(uri, error);
   } else {
      ASSERT_WITH_MSG(!uri, "expected URI to be invalid: %s", uri_string);
      return;
   }

   if (!bson_empty0(hosts)) {
      bson_iter_t iter;
      bson_iter_t host_iter;

      for (bson_iter_init(&iter, hosts); bson_iter_next(&iter) && bson_iter_recurse(&iter, &host_iter);) {
         const char *host = "localhost";
         int64_t port = 27017;
         bool ok = false;

         if (bson_iter_find(&host_iter, "host") && BSON_ITER_HOLDS_UTF8(&host_iter)) {
            host = bson_iter_utf8(&host_iter, NULL);
         }
         if (bson_iter_find(&host_iter, "port") && BSON_ITER_HOLDS_INT(&host_iter)) {
            port = bson_iter_as_int64(&host_iter);
         }

         for (const mongoc_host_list_t *hl = mongoc_uri_get_hosts(uri); hl; hl = hl->next) {
            if (!strcmp(host, hl->host) && port == hl->port) {
               ok = true;
               break;
            }
         }

         if (!ok) {
            test_error("Could not find '%s':%" PRId64 " in uri '%s'\n", host, port, mongoc_uri_get_string(uri));
         }
      }
   }

   if (auth) {
      bson_iter_t iter;

      if (bson_iter_init_find(&iter, auth, "username") && BSON_ITER_HOLDS_UTF8(&iter)) {
         ASSERT_CMPSTR(mongoc_uri_get_username(uri), bson_iter_utf8(&iter, NULL));
      }

      if (bson_iter_init_find(&iter, auth, "password") && BSON_ITER_HOLDS_UTF8(&iter)) {
         ASSERT_CMPSTR(mongoc_uri_get_password(uri), bson_iter_utf8(&iter, NULL));
      }

      if ((bson_iter_init_find(&iter, auth, "db") || bson_iter_init_find(&iter, auth, "source")) &&
          BSON_ITER_HOLDS_UTF8(&iter)) {
         ASSERT_CMPSTR(mongoc_uri_get_auth_source(uri), bson_iter_utf8(&iter, NULL));
      }
   }

   if (options) {
      bson_t actual = BSON_INITIALIZER;
      bson_iter_t iter;

      // "options" includes both URI options and credentials.
      bson_concat(&actual, mongoc_uri_get_options(uri));
      bson_concat(&actual, mongoc_uri_get_credentials(uri));

      const mongoc_read_concern_t *const rc = mongoc_uri_get_read_concern(uri);
      if (!mongoc_read_concern_is_default(rc)) {
         BSON_APPEND_UTF8(&actual, "readconcernlevel", mongoc_read_concern_get_level(rc));
      }

      bson_t expected = BSON_INITIALIZER;
      bson_copy_to_excluding_noinit(options,
                                    &expected,

                                    // These 'auth' params may be included in 'options'
                                    "username",
                                    "password",
                                    "source",

                                    // Credentials fields.
                                    "authmechanism",
                                    "authmechanismproperties",

                                    // Rename for consistency.
                                    "mechanism",            // -> "authmechanism"
                                    "mechanism_properties", // -> "authmechanismproperties"
                                    NULL);

      if ((bson_iter_init_find(&iter, options, "mechanism") || bson_iter_init_find(&iter, options, "authmechanism")) &&
          BSON_ITER_HOLDS_UTF8(&iter)) {
         ASSERT(!bson_has_field(&expected, "authmechanism"));
         ASSERT(BSON_APPEND_UTF8(&expected, "authmechanism", bson_iter_utf8(&iter, NULL)));
      }

      if ((bson_iter_init_find(&iter, options, "mechanism_properties") ||
           bson_iter_init_find(&iter, options, "authmechanismproperties")) &&
          BSON_ITER_HOLDS_DOCUMENT(&iter)) {
         ASSERT(!bson_has_field(&expected, "authmechanismproperties"));
         ASSERT(BSON_APPEND_ITER(&expected, "authmechanismproperties", &iter));
      }

      bson_iter_init(&iter, &expected);
      bson_contains_iter(&actual, &iter);

      bson_destroy(&expected);
      bson_destroy(&actual);
   }

   if (credentials) {
      bson_iter_t iter;

      bson_t expected = BSON_INITIALIZER;

      // Rename keys for consistency across tests:
      //  - "mechanism" -> "authmechanism"
      //  - "mechanism_properties" -> "authmechanismproperties"
      {
         bson_copy_to_excluding_noinit(credentials,
                                       &expected,

                                       // Credentials fields.
                                       "authmechanism",
                                       "authmechanismproperties",

                                       // Rename for consistency.
                                       "mechanism",            // -> "authmechanism"
                                       "mechanism_properties", // -> "authmechanismproperties"
                                       NULL);

         if ((bson_iter_init_find(&iter, credentials, "mechanism") ||
              bson_iter_init_find(&iter, credentials, "authmechanism")) &&
             BSON_ITER_HOLDS_UTF8(&iter)) {
            ASSERT(!bson_has_field(&expected, "authmechanism"));
            ASSERT(BSON_APPEND_UTF8(&expected, "authmechanism", bson_iter_utf8(&iter, NULL)));
         }

         if ((bson_iter_init_find(&iter, credentials, "mechanism_properties") ||
              bson_iter_init_find(&iter, credentials, "authmechanismproperties")) &&
             BSON_ITER_HOLDS_DOCUMENT(&iter)) {
            ASSERT(!bson_has_field(&expected, "authmechanismproperties"));
            ASSERT(BSON_APPEND_ITER(&expected, "authmechanismproperties", &iter));
         }
      }

      bsonVisitEach(
         expected,
         case (
            when(iKeyWithType("username", utf8), do({ ASSERT_CMPSTR(mongoc_uri_get_username(uri), bsonAs(cstr)); })),
            when(iKeyWithType("password", utf8), do({ ASSERT_CMPSTR(mongoc_uri_get_password(uri), bsonAs(cstr)); })),
            when(iKeyWithType("source", utf8), do({ ASSERT_CMPSTR(mongoc_uri_get_auth_source(uri), bsonAs(cstr)); })),
            when(iKeyWithType("authmechanism", utf8),
                 do({ ASSERT_CMPSTR(mongoc_uri_get_auth_mechanism(uri), bsonAs(cstr)); })),
            when(iKeyWithType("authmechanismproperties", doc), do({
                    bson_t expected_props = BSON_INITIALIZER;
                    ASSERT_OR_PRINT(_mongoc_iter_document_as_bson(&bsonVisitIter, &expected_props, &error), error);

                    // CDRIVER-4128: CANONICALIZE_HOST_NAME is UTF-8 even when "false" or "true".
                    {
                       bson_t updated = BSON_INITIALIZER;
                       bson_copy_to_excluding_noinit(&expected_props, &updated, "CANONICALIZE_HOST_NAME", NULL);
                       if (bson_iter_init_find_case(&iter, &expected_props, "CANONICALIZE_HOST_NAME")) {
                          if (BSON_ITER_HOLDS_BOOL(&iter)) {
                             BSON_APPEND_UTF8(
                                &updated, "CANONICALIZE_HOST_NAME", bson_iter_bool(&iter) ? "true" : "false");
                          } else {
                             BSON_APPEND_VALUE(&updated, "CANONICALIZE_HOST_NAME", bson_iter_value(&iter));
                          }
                       }
                       bson_destroy(&expected_props);
                       expected_props = updated; // Ownership transfer.
                    }

                    bson_t actual;
                    ASSERT_WITH_MSG(mongoc_uri_get_mechanism_properties(uri, &actual),
                                    "expected authmechanismproperties to be provided");

                    bson_iter_init(&iter, &expected_props);
                    bson_contains_iter(&actual, &iter);

                    bson_destroy(&expected_props);
                    bson_destroy(&actual);
                 })),
            // Connection String spec: if a test case includes a null value for one of these keys (e.g. auth: ~,
            // port: ~), no assertion is necessary.
            when(iKeyWithType("username", null), nop),
            when(iKeyWithType("password", null), nop),
            when(iKeyWithType("source", null), nop),
            when(iKeyWithType("authmechanism", null), nop),
            when(iKeyWithType("authmechanismproperties", null), nop),
            else(do({
               test_error("unexpected credentials field '%s' with type '%s'",
                          bson_iter_key(&bsonVisitIter),
                          _mongoc_bson_type_to_str(bson_iter_type(&bsonVisitIter)));
            }))));

      bson_destroy(&expected);
   }

   if (uri) {
      mongoc_uri_destroy(uri);
   }
}

static bson_t *
bson_lookup_doc_null_ok(const bson_t *b, const char *key)
{
   bson_iter_t iter;

   if (!bson_iter_init_find(&iter, b, key)) {
      return NULL;
   }

   if (BSON_ITER_HOLDS_NULL(&iter)) {
      return NULL;
   }

   bson_t *const ret = bson_new();
   {
      bson_t doc;
      bson_iter_bson(&iter, &doc);
      bson_concat(ret, &doc);
   }
   return ret;
}

static void
test_connection_uri_cb(void *scenario_vp)
{
   BSON_ASSERT_PARAM(scenario_vp);

   const bson_t *const scenario = scenario_vp;

   static const test_skip_t skips[] = {
      {.description = "Valid connection pool options are parsed correctly",
       .reason = "libmongoc does not support maxIdleTimeMS"},
      {.description = "Valid connection and timeout options are parsed correctly",
       .reason = "libmongoc does not support maxIdleTimeMS"},
      {.description = "timeoutMS=0", .reason = "libmongoc does not support timeoutMS (CDRIVER-3786)"},
      {.description = "Non-numeric timeoutMS causes a warning",
       .reason = "libmongoc does not support timeoutMS (CDRIVER-3786)"},
      {.description = "Too low timeoutMS causes a warning",
       .reason = "libmongoc does not support timeoutMS (CDRIVER-3786)"},
      {.description = "proxyPort without proxyHost", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "proxyUsername without proxyHost", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "proxyPassword without proxyHost", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "all other proxy options without proxyHost",
       .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "proxyUsername without proxyPassword",
       .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "proxyPassword without proxyUsername",
       .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "multiple proxyHost parameters", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "multiple proxyPort parameters", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "multiple proxyUsername parameters",
       .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "multiple proxyPassword parameters",
       .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "only host present", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "host and default port present", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "host and non-default port present",
       .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "replicaset, host and non-default port present",
       .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "all options present", .reason = "libmongoc does not support proxies (CDRIVER-4187)"},
      {.description = "Valid connection pool options are parsed correctly",
       .reason = "libmongoc does not support minPoolSize (CDRIVER-2390)"},
      {.description = "minPoolSize=0 does not error",
       .reason = "libmongoc does not support minPoolSize (CDRIVER-2390)"},
      {.description = "should throw an exception if neither environment nor callbacks specified (MONGODB-OIDC)",
       .reason = "libmongoc OIDC callbacks attach to MongoClient, which is not involved by this test"},
      {.description = NULL},
   };

   bson_iter_t iter;
   bson_iter_t tests_iter;

   ASSERT(bson_iter_init_find(&iter, scenario, "tests"));
   ASSERT(BSON_ITER_HOLDS_ARRAY(&iter));
   ASSERT(bson_iter_recurse(&iter, &tests_iter));

   while (bson_iter_next(&tests_iter)) {
      bson_t test_case;

      bson_iter_bson(&tests_iter, &test_case);

      const char *description = bson_lookup_utf8(&test_case, "description");
      if (test_should_be_skipped(skips, description)) {
         continue;
      }

      const char *uri_string = bson_lookup_utf8(&test_case, "uri");
      if (test_suite_debug_output()) {
         printf("  - %s: '%s'\n", description, uri_string);
         fflush(stdout);
      }

      bson_t *const hosts = bson_lookup_doc_null_ok(&test_case, "hosts");
      bson_t *const auth = bson_lookup_doc_null_ok(&test_case, "auth");
      bson_t *const options = bson_lookup_doc_null_ok(&test_case, "options");
      bson_t *const credentials = bson_lookup_doc_null_ok(&test_case, "credential");

      const bool valid = _mongoc_lookup_bool(&test_case, "valid", true);
      capture_logs(true);
      run_uri_test(uri_string, valid, hosts, auth, options, credentials);

      bson_iter_t warning_iter;
      bson_iter_init(&warning_iter, &test_case);

      bson_iter_t descendent;
      if (bson_iter_find_descendant(&warning_iter, "warning", &descendent) && BSON_ITER_HOLDS_BOOL(&descendent)) {
         if (bson_iter_as_bool(&descendent)) {
            ASSERT_CAPTURED_LOG("mongoc_uri", MONGOC_LOG_LEVEL_WARNING, "");
         } else {
            ASSERT_NO_CAPTURED_LOGS("mongoc_uri");
         }
      }

      bson_destroy(hosts);
      bson_destroy(auth);
      bson_destroy(options);
      bson_destroy(credentials);
   }
}


static void
test_all_spec_tests(TestSuite *suite)
{
   install_json_test_suite(suite, JSON_DIR, "uri-options", &test_connection_uri_cb);
   install_json_test_suite(suite, JSON_DIR, "connection_uri", &test_connection_uri_cb);
   install_json_test_suite(suite, JSON_DIR, "auth/legacy", &test_connection_uri_cb);
}


void
test_connection_uri_install(TestSuite *suite)
{
   test_all_spec_tests(suite);
}
