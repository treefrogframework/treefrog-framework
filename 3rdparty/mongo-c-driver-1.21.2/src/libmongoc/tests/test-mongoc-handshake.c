/*
 * Copyright 2016 MongoDB, Inc.
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

/*
 * Call this before any test which uses mongoc_handshake_data_append, to
 * reset the global state and unfreeze the handshake struct. Call it
 * after a test so later tests don't have a weird handshake document
 *
 * This is not safe to call while we have any clients or client pools running!
 */
static void
_reset_handshake (void)
{
   _mongoc_handshake_cleanup ();
   _mongoc_handshake_init ();
}

static void
test_mongoc_handshake_appname_in_uri (void)
{
   char long_string[MONGOC_HANDSHAKE_APPNAME_MAX + 2];
   char *uri_str;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";
   mongoc_uri_t *uri;
   const char *appname = "mongodump";
   const char *value;

   memset (long_string, 'a', MONGOC_HANDSHAKE_APPNAME_MAX + 1);
   long_string[MONGOC_HANDSHAKE_APPNAME_MAX + 1] = '\0';

   /* Shouldn't be able to set with appname really long */
   capture_logs (true);
   uri_str = bson_strdup_printf ("mongodb://a/?" MONGOC_URI_APPNAME "=%s",
                                 long_string);
   ASSERT (!test_framework_client_new (uri_str, NULL));
   ASSERT_CAPTURED_LOG ("_mongoc_topology_scanner_set_appname",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported value");
   capture_logs (false);

   uri = mongoc_uri_new (good_uri);
   ASSERT (uri);
   value = mongoc_uri_get_appname (uri);
   ASSERT (value);
   ASSERT_CMPSTR (appname, value);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new (NULL);
   ASSERT (uri);
   ASSERT (!mongoc_uri_set_appname (uri, long_string));
   ASSERT (mongoc_uri_set_appname (uri, appname));
   value = mongoc_uri_get_appname (uri);
   ASSERT (value);
   ASSERT_CMPSTR (appname, value);
   mongoc_uri_destroy (uri);

   bson_free (uri_str);
}

static void
test_mongoc_handshake_appname_frozen_single (void)
{
   mongoc_client_t *client;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";

   client = test_framework_client_new (good_uri, NULL);

   /* Shouldn't be able to set appname again */
   capture_logs (true);
   ASSERT (!mongoc_client_set_appname (client, "a"));
   ASSERT_CAPTURED_LOG ("_mongoc_topology_scanner_set_appname",
                        MONGOC_LOG_LEVEL_ERROR,
                        "Cannot set appname more than once");
   capture_logs (false);

   mongoc_client_destroy (client);
}

static void
test_mongoc_handshake_appname_frozen_pooled (void)
{
   mongoc_client_pool_t *pool;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";
   mongoc_uri_t *uri;

   uri = mongoc_uri_new (good_uri);

   pool = test_framework_client_pool_new_from_uri (uri, NULL);
   capture_logs (true);
   ASSERT (!mongoc_client_pool_set_appname (pool, "test"));
   ASSERT_CAPTURED_LOG ("_mongoc_topology_scanner_set_appname",
                        MONGOC_LOG_LEVEL_ERROR,
                        "Cannot set appname more than once");
   capture_logs (false);

   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
}

static void
_check_arch_string_valid (const char *arch)
{
#ifdef _POSIX_VERSION
   struct utsname system_info;

   ASSERT (uname (&system_info) >= 0);
   ASSERT_CMPSTR (system_info.machine, arch);
#endif
   ASSERT (strlen (arch) > 0);
}

static void
_check_os_version_valid (const char *os_version)
{
#if defined(__linux__) || defined(_WIN32)
   /* On linux we search the filesystem for os version or use uname.
    * On windows we call GetSystemInfo(). */
   ASSERT (os_version);
   ASSERT (strlen (os_version) > 0);
#elif defined(_POSIX_VERSION)
   /* On a non linux posix systems, we just call uname() */
   struct utsname system_info;

   ASSERT (uname (&system_info) >= 0);
   ASSERT (os_version);
   ASSERT_CMPSTR (system_info.release, os_version);
#endif
}

static void
test_mongoc_handshake_data_append_success (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;
   request_t *request;
   const bson_t *request_doc;
   bson_iter_t iter;
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *val;

   const char *driver_name = "php driver";
   const char *driver_version = "version abc";
   const char *platform = "./configure -nottoomanyflags";

   _reset_handshake ();
   /* Make sure setting the handshake works */
   ASSERT (
      mongoc_handshake_data_append (driver_name, driver_version, platform));

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_APPNAME, "testapp");
   pool = test_framework_client_pool_new_from_uri (uri, NULL);

   /* Force topology scanner to start */
   client = mongoc_client_pool_pop (pool);

   request = mock_server_receives_legacy_hello (server, NULL);
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);
   ASSERT (bson_has_field (request_doc, HANDSHAKE_FIELD));

   ASSERT (bson_iter_init_find (&iter, request_doc, HANDSHAKE_FIELD));
   ASSERT (bson_iter_recurse (&iter, &md_iter));

   ASSERT (bson_iter_find (&md_iter, "application"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));
   ASSERT (bson_iter_find (&inner_iter, "name"));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT_CMPSTR (val, "testapp");

   /* Make sure driver.name and driver.version and platform are all right */
   ASSERT (bson_iter_find (&md_iter, "driver"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));
   ASSERT (bson_iter_find (&inner_iter, "name"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strstr (val, driver_name) != NULL);

   ASSERT (bson_iter_find (&inner_iter, "version"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strstr (val, driver_version));

   /* Check os type not empty */
   ASSERT (bson_iter_find (&md_iter, "os"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));

   ASSERT (bson_iter_find (&inner_iter, "type"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strlen (val) > 0);

   /* Check os version valid */
   ASSERT (bson_iter_find (&inner_iter, "version"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   _check_os_version_valid (val);

   /* Check os arch is valid */
   ASSERT (bson_iter_find (&inner_iter, "architecture"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   _check_arch_string_valid (val);

   /* Not checking os_name, as the spec says it can be NULL. */

   /* Check platform field ok */
   ASSERT (bson_iter_find (&md_iter, "platform"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&md_iter));
   val = bson_iter_utf8 (&md_iter, NULL);
   ASSERT (val);
   if (strlen (val) <
       250) { /* standard val are < 100, may be truncated on some platform */
      ASSERT (strstr (val, platform) != NULL);
   }

   mock_server_replies_simple (request, "{'ok': 1, 'isWritablePrimary': true}");
   request_destroy (request);

   /* Cleanup */
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);

   _reset_handshake ();
}


static void
test_mongoc_handshake_data_append_null_args (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;
   request_t *request;
   const bson_t *request_doc;
   bson_iter_t iter;
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *val;

   _reset_handshake ();
   /* Make sure setting the handshake works */
   ASSERT (mongoc_handshake_data_append (NULL, NULL, NULL));

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_APPNAME, "testapp");
   pool = test_framework_client_pool_new_from_uri (uri, NULL);

   /* Force topology scanner to start */
   client = mongoc_client_pool_pop (pool);

   request = mock_server_receives_legacy_hello (server, NULL);
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);
   ASSERT (bson_has_field (request_doc, HANDSHAKE_FIELD));

   ASSERT (bson_iter_init_find (&iter, request_doc, HANDSHAKE_FIELD));
   ASSERT (bson_iter_recurse (&iter, &md_iter));

   ASSERT (bson_iter_find (&md_iter, "application"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));
   ASSERT (bson_iter_find (&inner_iter, "name"));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT_CMPSTR (val, "testapp");

   /* Make sure driver.name and driver.version and platform are all right */
   ASSERT (bson_iter_find (&md_iter, "driver"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));
   ASSERT (bson_iter_find (&inner_iter, "name"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strstr (val, " / ") == NULL); /* No append delimiter */

   ASSERT (bson_iter_find (&inner_iter, "version"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strstr (val, " / ") == NULL); /* No append delimiter */

   /* Check os type not empty */
   ASSERT (bson_iter_find (&md_iter, "os"));
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&md_iter));
   ASSERT (bson_iter_recurse (&md_iter, &inner_iter));

   ASSERT (bson_iter_find (&inner_iter, "type"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   ASSERT (strlen (val) > 0);

   /* Check os version valid */
   ASSERT (bson_iter_find (&inner_iter, "version"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   _check_os_version_valid (val);

   /* Check os arch is valid */
   ASSERT (bson_iter_find (&inner_iter, "architecture"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&inner_iter));
   val = bson_iter_utf8 (&inner_iter, NULL);
   ASSERT (val);
   _check_arch_string_valid (val);

   /* Not checking os_name, as the spec says it can be NULL. */

   /* Check platform field ok */
   ASSERT (bson_iter_find (&md_iter, "platform"));
   ASSERT (BSON_ITER_HOLDS_UTF8 (&md_iter));
   val = bson_iter_utf8 (&md_iter, NULL);
   ASSERT (val);
   /* standard val are < 100, may be truncated on some platform */
   if (strlen (val) < 250) {
      /* `printf("%s", NULL)` -> "(null)" with libstdc++, libc++, and STL */
      ASSERT (strstr (val, "null") == NULL);
   }

   mock_server_replies_simple (request, "{'ok': 1, 'isWritablePrimary': true}");
   request_destroy (request);

   /* Cleanup */
   mongoc_client_pool_push (pool, client);
   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);

   _reset_handshake ();
}


static void
_test_platform (bool platform_oversized)
{
   mongoc_handshake_t *md;
   size_t platform_len;
   const char *platform_suffix = "b";
   bson_t doc = BSON_INITIALIZER;

   _mongoc_handshake_cleanup ();
   _mongoc_handshake_init ();

   md = _mongoc_handshake_get ();

   bson_free (md->os_type);
   md->os_type = bson_strdup ("foo");
   bson_free (md->os_name);
   md->os_name = bson_strdup ("foo");
   bson_free (md->os_version);
   md->os_version = bson_strdup ("foo");
   bson_free (md->os_architecture);
   md->os_architecture = bson_strdup ("foo");
   bson_free (md->driver_name);
   md->driver_name = bson_strdup ("foo");
   bson_free (md->driver_version);
   md->driver_version = bson_strdup ("foo");

   platform_len = HANDSHAKE_MAX_SIZE;
   if (platform_oversized) {
      platform_len += 100;
   }

   bson_free (md->platform);
   md->platform = bson_malloc (platform_len);
   memset (md->platform, 'a', platform_len - 1);
   md->platform[platform_len - 1] = '\0';

   /* returns true, but ignores the suffix; there's no room */
   ASSERT (mongoc_handshake_data_append (NULL, NULL, platform_suffix));
   ASSERT (!strstr (md->platform, "b"));
   ASSERT (_mongoc_handshake_build_doc_with_application (&doc, "my app"));
   ASSERT_CMPUINT32 (doc.len, ==, (uint32_t) HANDSHAKE_MAX_SIZE);

   bson_destroy (&doc);
   _reset_handshake (); /* frees the strings created above */
}


static void
test_mongoc_handshake_big_platform (void)
{
   _test_platform (false);
}


static void
test_mongoc_handshake_oversized_platform (void)
{
   _test_platform (true);
}


static void
test_mongoc_handshake_data_append_after_cmd (void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   mongoc_uri_t *uri;

   _reset_handshake ();

   uri = mongoc_uri_new ("mongodb://127.0.0.1/?" MONGOC_URI_MAXPOOLSIZE "=1");

   /* Make sure that after we pop a client we can't set global handshake */
   pool = test_framework_client_pool_new_from_uri (uri, NULL);

   client = mongoc_client_pool_pop (pool);

   capture_logs (true);
   ASSERT (!mongoc_handshake_data_append ("a", "a", "a"));
   capture_logs (false);

   mongoc_client_pool_push (pool, client);

   mongoc_uri_destroy (uri);
   mongoc_client_pool_destroy (pool);

   _reset_handshake ();
}

/*
 * Append to the platform field a huge string
 * Make sure that it gets truncated
 */
static void
test_mongoc_handshake_too_big (void)
{
   mongoc_client_t *client;
   mock_server_t *server;
   mongoc_uri_t *uri;
   future_t *future;
   request_t *request;
   const bson_t *hello_doc;
   bson_iter_t iter;

   enum { BUFFER_SIZE = HANDSHAKE_MAX_SIZE };
   char big_string[BUFFER_SIZE];
   uint32_t len;
   const uint8_t *dummy;

   server = mock_server_new ();
   mock_server_run (server);

   _reset_handshake ();

   memset (big_string, 'a', BUFFER_SIZE - 1);
   big_string[BUFFER_SIZE - 1] = '\0';
   ASSERT (mongoc_handshake_data_append (NULL, NULL, big_string));

   uri = mongoc_uri_copy (mock_server_get_uri (server));
   /* avoid rare test timeouts */
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 20000);
   client = test_framework_client_new_from_uri (uri, NULL);

   ASSERT (mongoc_client_set_appname (client, "my app"));

   /* Send a ping, mock server deals with it */
   future = future_client_command_simple (
      client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, NULL);
   request = mock_server_receives_legacy_hello (server, NULL);

   /* Make sure the hello request has a handshake field, and it's not huge */
   ASSERT (request);
   hello_doc = request_get_doc (request, 0);
   ASSERT (hello_doc);
   ASSERT (bson_has_field (hello_doc, HANDSHAKE_FIELD));

   /* hello with handshake isn't too big */
   bson_iter_init_find (&iter, hello_doc, HANDSHAKE_FIELD);
   ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
   bson_iter_document (&iter, &len, &dummy);

   /* Should have truncated the platform field so it fits exactly */
   ASSERT (len == HANDSHAKE_MAX_SIZE);

   mock_server_replies_simple (
      request,
      tmp_str ("{'ok': 1, 'minWireVersion': %d, 'maxWireVersion': %d}",
               WIRE_VERSION_MIN,
               WIRE_VERSION_MAX));
   request_destroy (request);

   request = mock_server_receives_msg (
      server, MONGOC_MSG_NONE, tmp_bson ("{'$db': 'admin', 'ping': 1}"));

   mock_server_replies_simple (request, "{'ok': 1}");
   ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);

   /* So later tests don't have "aaaaa..." as the md platform string */
   _reset_handshake ();
}

/*
 * Testing whether platform string data is truncated/dropped appropriately
 * drop specifies what case should be tested for
 */
static void
test_mongoc_platform_truncate (int drop)
{
   mongoc_handshake_t *md;
   bson_t doc = BSON_INITIALIZER;
   bson_iter_t iter;

   char *undropped;
   char *expected;
   char big_string[HANDSHAKE_MAX_SIZE];
   int handshake_remaining_space;

   /* Need to know how much space storing fields in our BSON will take
    * so that we can make our platform string the correct length here */
   int handshake_bson_size = 163;
   _reset_handshake ();

   md = _mongoc_handshake_get ();

   /* we manually bypass the defaults of the handshake to ensure an exceedingly
    * long field does not cause our test to incorrectly fail */
   bson_free (md->os_type);
   md->os_type = bson_strdup ("test_a");
   bson_free (md->os_name);
   md->os_name = bson_strdup ("test_b");
   bson_free (md->os_version);
   md->os_version = bson_strdup ("test_c");
   bson_free (md->os_architecture);
   md->os_architecture = bson_strdup ("test_d");
   bson_free (md->driver_name);
   md->driver_name = bson_strdup ("test_e");
   bson_free (md->driver_version);
   md->driver_version = bson_strdup ("test_f");
   bson_free (md->compiler_info);
   md->compiler_info = bson_strdup ("test_g");
   bson_free (md->flags);
   md->flags = bson_strdup ("test_h");

   handshake_remaining_space =
      HANDSHAKE_MAX_SIZE -
      (strlen (md->os_type) + strlen (md->os_name) + strlen (md->os_version) +
       strlen (md->os_architecture) + strlen (md->driver_name) +
       strlen (md->driver_version) + strlen (md->compiler_info) +
       strlen (md->flags) + handshake_bson_size);

   /* adjust remaining space depending on which combination of
    * flags/compiler_info we want to test dropping */
   if (drop == 2) {
      handshake_remaining_space +=
         strlen (md->flags) + strlen (md->compiler_info);
      undropped = bson_strdup_printf ("%s", "");
   } else if (drop == 1) {
      handshake_remaining_space += strlen (md->flags);
      undropped = bson_strdup_printf ("%s", md->compiler_info);
   } else {
      undropped = bson_strdup_printf ("%s%s", md->compiler_info, md->flags);
   }

   memset (big_string, 'a', handshake_remaining_space + 1);
   big_string[handshake_remaining_space + 1] = '\0';

   ASSERT (mongoc_handshake_data_append (NULL, NULL, big_string));
   ASSERT (_mongoc_handshake_build_doc_with_application (&doc, "my app"));

   /* doc.len being strictly less than HANDSHAKE_MAX_SIZE proves that we have
    * dropped the flags correctly, instead of truncating anything
    */
   ASSERT_CMPUINT32 (doc.len, <, (uint32_t) HANDSHAKE_MAX_SIZE);
   bson_iter_init_find (&iter, &doc, "platform");
   expected = bson_strdup_printf ("%s%s", big_string, undropped);
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), expected);

   bson_free (expected);
   bson_free (undropped);
   bson_destroy (&doc);
   /* So later tests don't have "aaaaa..." as the md platform string */
   _reset_handshake ();
}

/*
 * Test dropping neither compiler_info/flags, dropping just flags, and dropping
 * both
 */
static void
test_mongoc_oversized_flags (void)
{
   test_mongoc_platform_truncate (0);
   test_mongoc_platform_truncate (1);
   test_mongoc_platform_truncate (2);
}

/* Test the case where we can't prevent the handshake doc being too big
 * and so we just don't send it */
static void
test_mongoc_handshake_cannot_send (void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;
   request_t *request;
   const char *const server_reply = "{'ok': 1, 'isWritablePrimary': true}";
   const bson_t *request_doc;
   char big_string[HANDSHAKE_MAX_SIZE];
   mongoc_handshake_t *md;

   _reset_handshake ();
   capture_logs (true);

   /* Mess with our global handshake struct so the handshake doc will be
    * way too big */
   memset (big_string, 'a', HANDSHAKE_MAX_SIZE - 1);
   big_string[HANDSHAKE_MAX_SIZE - 1] = '\0';

   md = _mongoc_handshake_get ();
   bson_free (md->os_name);
   md->os_name = bson_strdup (big_string);

   server = mock_server_new ();
   mock_server_run (server);
   uri = mongoc_uri_copy (mock_server_get_uri (server));
   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   pool = test_framework_client_pool_new_from_uri (uri, NULL);

   /* Pop a client to trigger the topology scanner */
   client = mongoc_client_pool_pop (pool);
   request = mock_server_receives_legacy_hello (server, NULL);

   /* Make sure the hello request DOESN'T have a handshake field: */
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);
   ASSERT (!bson_has_field (request_doc, HANDSHAKE_FIELD));

   mock_server_replies_simple (request, server_reply);
   request_destroy (request);

   /* Cause failure on client side */
   request = mock_server_receives_legacy_hello (server, NULL);
   ASSERT (request);
   mock_server_hangs_up (request);
   request_destroy (request);

   /* Make sure the hello request still DOESN'T have a handshake field
    * on subsequent heartbeats. */
   request = mock_server_receives_legacy_hello (server, NULL);
   ASSERT (request);
   request_doc = request_get_doc (request, 0);
   ASSERT (request_doc);
   ASSERT (!bson_has_field (request_doc, HANDSHAKE_FIELD));

   mock_server_replies_simple (request, server_reply);
   request_destroy (request);

   /* cleanup */
   mongoc_client_pool_push (pool, client);

   mongoc_client_pool_destroy (pool);
   mongoc_uri_destroy (uri);
   mock_server_destroy (server);

   /* Reset again so the next tests don't have a handshake doc which
    * is too big */
   _reset_handshake ();
}

extern char *
_mongoc_handshake_get_config_hex_string (void);

static bool
_get_bit (char *config_str, uint32_t bit)
{
   /* get the location of the two byte chars for this bit. */
   uint32_t byte = bit / 8;
   uint32_t bit_of_byte = bit % 8;
   uint32_t char_loc;
   uint32_t as_num;
   char byte_str[3];

   /* byte 0 is represented at the last two characters. */
   char_loc = (uint32_t) strlen (config_str) - 2 - (byte * 2);
   /* index should be past the prefixed "0x" */
   ASSERT_CMPINT32 (char_loc, >, 1);
   /* get the number representation of the byte. */
   byte_str[0] = config_str[char_loc];
   byte_str[1] = config_str[char_loc + 1];
   byte_str[2] = '\0';
   as_num = (uint8_t) strtol (byte_str, NULL, 16);
   return (as_num & (1u << bit_of_byte)) > 0u;
}

void
test_handshake_platform_config (void)
{
   /* Parse the config string, and check that it matches the defined flags. */
   char *config_str = _mongoc_handshake_get_config_hex_string ();
   uint32_t total_bytes = (LAST_MONGOC_MD_FLAG + 7) / 8;
   uint32_t total_bits = 8 * total_bytes;
   uint32_t i;
   /* config_str should have the form 0x?????. */
   ASSERT_CMPINT ((int) strlen (config_str), ==, 2 + (2 * total_bytes));
   BSON_ASSERT (strncmp (config_str, "0x", 2) == 0);

/* go through all flags. */
#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
   BSON_ASSERT (_get_bit (config_str, MONGOC_ENABLE_SSL_SECURE_CHANNEL));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_CNG
   BSON_ASSERT (_get_bit (config_str, MONGOC_ENABLE_CRYPTO_CNG));
#endif

#ifdef MONGOC_ENABLE_SSL_SECURE_TRANSPORT
   BSON_ASSERT (
      _get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SSL_SECURE_TRANSPORT));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO
   BSON_ASSERT (
      _get_bit (config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO_COMMON_CRYPTO));
#endif

#ifdef MONGOC_ENABLE_SSL_OPENSSL
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SSL_OPENSSL));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_LIBCRYPTO
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO_LIBCRYPTO));
#endif

#ifdef MONGOC_ENABLE_SSL
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SSL));
#endif

#ifdef MONGOC_ENABLE_CRYPTO
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE
   BSON_ASSERT (
      _get_bit (config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO_SYSTEM_PROFILE));
#endif

#ifdef MONGOC_ENABLE_SASL
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SASL));
#endif

#ifdef MONGOC_HAVE_SASL_CLIENT_DONE
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_HAVE_SASL_CLIENT_DONE));
#endif

#ifdef MONGOC_NO_AUTOMATIC_GLOBALS
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_NO_AUTOMATIC_GLOBALS));
#endif

#ifdef MONGOC_EXPERIMENTAL_FEATURES
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_EXPERIMENTAL_FEATURES));
#endif

#ifdef MONGOC_ENABLE_SSL_LIBRESSL
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SSL_LIBRESSL));
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SASL_CYRUS));
#endif

#ifdef MONGOC_ENABLE_SASL_SSPI
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SASL_SSPI));
#endif

#ifdef MONGOC_HAVE_SOCKLEN
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_HAVE_SOCKLEN));
#endif

#ifdef MONGOC_ENABLE_COMPRESSION
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_COMPRESSION));
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   BSON_ASSERT (
      _get_bit (config_str, MONGOC_MD_FLAG_ENABLE_COMPRESSION_SNAPPY));
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_COMPRESSION_ZLIB));
#endif

#ifdef MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI));
#endif

#ifdef MONGOC_HAVE_RES_NSEARCH
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_RES_NSEARCH));
#endif

#ifdef MONGOC_HAVE_RES_NDESTROY
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_RES_NDESTROY));
#endif

#ifdef MONGOC_HAVE_RES_NCLOSE
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_RES_NCLOSE));
#endif

#ifdef MONGOC_HAVE_RES_SEARCH
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_RES_SEARCH));
#endif

#ifdef MONGOC_HAVE_DNSAPI
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_DNSAPI));
#endif

#ifdef MONGOC_HAVE_RDTSCP
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_RDTSCP));
#endif

#ifdef MONGOC_HAVE_SCHED_GETCPU
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_HAVE_SCHED_GETCPU));
#endif

#ifdef MONGOC_ENABLE_SHM_COUNTERS
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_SHM_COUNTERS));
#endif

   if (MONGOC_TRACE_ENABLED) {
      BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_TRACE));
   }

#ifdef MONGOC_ENABLE_ICU
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_ICU));
#endif

#ifdef MONGOC_ENABLE_CLIENT_SIDE_ENCRYPTION
   BSON_ASSERT (
      _get_bit (config_str, MONGOC_MD_FLAG_ENABLE_CLIENT_SIDE_ENCRYPTION));
#endif

#ifdef MONGOC_ENABLE_MONGODB_AWS_AUTH
   BSON_ASSERT (_get_bit (config_str, MONGOC_MD_FLAG_ENABLE_MONGODB_AWS_AUTH));
#endif

   /* any excess bits should all be zero. */
   for (i = LAST_MONGOC_MD_FLAG; i < total_bits; i++) {
      BSON_ASSERT (!_get_bit (config_str, i));
   }
   bson_free (config_str);
}

/* Called by a single thread in test_mongoc_handshake_race_condition */
static BSON_THREAD_FUN (handshake_append_worker, data)
{
   const char *driver_name = "php driver";
   const char *driver_version = "version abc";
   const char *platform = "./configure -nottoomanyflags";

   mongoc_handshake_data_append (driver_name, driver_version, platform);

   BSON_THREAD_RETURN;
}

/* Run 1000 iterations of mongoc_handshake_data_append() using 4 threads */
static void
test_mongoc_handshake_race_condition (void)
{
   unsigned i, j;
   bson_thread_t threads[4];

   for (i = 0; i < 1000; ++i) {
      _reset_handshake ();

      for (j = 0; j < 4; ++j) {
         BSON_ASSERT (!COMMON_PREFIX (thread_create) (
            &threads[j], &handshake_append_worker, NULL));
      }
      for (j = 0; j < 4; ++j) {
         COMMON_PREFIX (thread_join) (threads[j]);
      }
   }

   _reset_handshake ();
}

void
test_handshake_install (TestSuite *suite)
{
   TestSuite_Add (suite,
                  "/MongoDB/handshake/appname_in_uri",
                  test_mongoc_handshake_appname_in_uri);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/appname_frozen_single",
                  test_mongoc_handshake_appname_frozen_single);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/appname_frozen_pooled",
                  test_mongoc_handshake_appname_frozen_pooled);

   TestSuite_AddMockServerTest (suite,
                                "/MongoDB/handshake/success",
                                test_mongoc_handshake_data_append_success);
   TestSuite_AddMockServerTest (suite,
                                "/MongoDB/handshake/null_args",
                                test_mongoc_handshake_data_append_null_args);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/big_platform",
                  test_mongoc_handshake_big_platform);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/oversized_platform",
                  test_mongoc_handshake_oversized_platform);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/failure",
                  test_mongoc_handshake_data_append_after_cmd);
   TestSuite_AddMockServerTest (
      suite, "/MongoDB/handshake/too_big", test_mongoc_handshake_too_big);
   TestSuite_Add (
      suite, "/MongoDB/handshake/oversized_flags", test_mongoc_oversized_flags);
   TestSuite_AddMockServerTest (suite,
                                "/MongoDB/handshake/cannot_send",
                                test_mongoc_handshake_cannot_send);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/platform_config",
                  test_handshake_platform_config);
   TestSuite_Add (suite,
                  "/MongoDB/handshake/race_condition",
                  test_mongoc_handshake_race_condition);
}
