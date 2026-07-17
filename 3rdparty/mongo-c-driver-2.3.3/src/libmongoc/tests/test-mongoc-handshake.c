/*
 * Copyright 2009-present MongoDB, Inc.
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

#include <bson/bson.h>

#include <mock_server/request.h>

#include <stdint.h>
#include <string.h>
#ifdef _POSIX_VERSION
#include <sys/utsname.h>
#endif

#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-config-private.h>
#include <mongoc/mongoc-handshake-private.h>

#include <mongoc/mongoc-handshake.h>

#include <mlib/config.h>

#include <TestSuite.h>
#include <mock_server/future-functions.h>
#include <mock_server/future.h>
#include <mock_server/mock-server.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

/*
 * Call this before any test which uses mongoc_handshake_data_append, to
 * reset the global state and unfreeze the handshake struct. Call it
 * after a test so later tests don't have a weird handshake document
 *
 * This is not safe to call while we have any clients or client pools running!
 */
static void
_reset_handshake(void)
{
   _mongoc_handshake_cleanup();
   _mongoc_handshake_init();
}

static const char *default_appname = "testapp";
static const char *default_driver_name = "php driver";
static const char *default_driver_version = "version abc";
static const char *default_platform = "./configure -nottoomanyflags";
static const int32_t default_timeout_sec = 60;
static const int32_t default_memory_mb = 1024;

static void
test_mongoc_handshake_appname_in_uri(void)
{
   char long_string[MONGOC_HANDSHAKE_APPNAME_MAX + 2];
   char *uri_str;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";
   mongoc_uri_t *uri;
   const char *appname = "mongodump";
   const char *value;

   memset(long_string, 'a', MONGOC_HANDSHAKE_APPNAME_MAX + 1);
   long_string[MONGOC_HANDSHAKE_APPNAME_MAX + 1] = '\0';

   /* Shouldn't be able to set with appname really long */
   capture_logs(true);
   uri_str = bson_strdup_printf("mongodb://a/?" MONGOC_URI_APPNAME "=%s", long_string);
   ASSERT(!test_framework_client_new(uri_str, NULL));
   ASSERT_CAPTURED_LOG("_mongoc_topology_scanner_set_appname", MONGOC_LOG_LEVEL_WARNING, "Unsupported value");
   capture_logs(false);

   uri = mongoc_uri_new(good_uri);
   ASSERT(uri);
   value = mongoc_uri_get_appname(uri);
   ASSERT(value);
   ASSERT_CMPSTR(appname, value);
   mongoc_uri_destroy(uri);

   uri = mongoc_uri_new(NULL);
   ASSERT(uri);
   ASSERT(!mongoc_uri_set_appname(uri, long_string));
   ASSERT(mongoc_uri_set_appname(uri, appname));
   value = mongoc_uri_get_appname(uri);
   ASSERT(value);
   ASSERT_CMPSTR(appname, value);
   mongoc_uri_destroy(uri);

   bson_free(uri_str);
}

static void
test_mongoc_handshake_appname_frozen_single(void)
{
   mongoc_client_t *client;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";

   client = test_framework_client_new(good_uri, NULL);

   /* Shouldn't be able to set appname again */
   capture_logs(true);
   ASSERT(!mongoc_client_set_appname(client, "a"));
   ASSERT_CAPTURED_LOG(
      "_mongoc_topology_scanner_set_appname", MONGOC_LOG_LEVEL_ERROR, "Cannot set appname more than once");
   capture_logs(false);

   mongoc_client_destroy(client);
}

static void
test_mongoc_handshake_appname_frozen_pooled(void)
{
   mongoc_client_pool_t *pool;
   const char *good_uri = "mongodb://host/?" MONGOC_URI_APPNAME "=mongodump";
   mongoc_uri_t *uri;

   uri = mongoc_uri_new(good_uri);

   pool = test_framework_client_pool_new_from_uri(uri, NULL);
   capture_logs(true);
   ASSERT(!mongoc_client_pool_set_appname(pool, "test"));
   ASSERT_CAPTURED_LOG(
      "_mongoc_topology_scanner_set_appname", MONGOC_LOG_LEVEL_ERROR, "Cannot set appname more than once");
   capture_logs(false);

   mongoc_client_pool_destroy(pool);
   mongoc_uri_destroy(uri);
}

static void
_check_arch_string_valid(const char *arch)
{
#ifdef _POSIX_VERSION
   struct utsname system_info;

   ASSERT(uname(&system_info) >= 0);
   ASSERT_CMPSTR(system_info.machine, arch);
#endif
   ASSERT(strlen(arch) > 0);
}

static void
_check_os_version_valid(const char *os_version)
{
#if defined(__linux__) || defined(_WIN32)
   /* On linux we search the filesystem for os version or use uname.
    * On windows we call GetSystemInfo(). */
   ASSERT(os_version);
   ASSERT(strlen(os_version) > 0);
#elif defined(_POSIX_VERSION)
   /* On a non linux posix systems, we just call uname() */
   struct utsname system_info;

   ASSERT(uname(&system_info) >= 0);
   ASSERT(os_version);
   ASSERT_CMPSTR(system_info.release, os_version);
#endif
}

static void
_handshake_check_application(bson_t *doc)
{
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *val;

   ASSERT(bson_iter_init_find(&md_iter, doc, "application"));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&md_iter));
   ASSERT(bson_iter_recurse(&md_iter, &inner_iter));
   ASSERT(bson_iter_find(&inner_iter, "name"));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT_CMPSTR(val, default_appname);
}

static void
_handshake_check_driver(bson_t *doc)
{
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *val;
   ASSERT(bson_iter_init_find(&md_iter, doc, "driver"));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&md_iter));
   ASSERT(bson_iter_recurse(&md_iter, &inner_iter));
   ASSERT(bson_iter_find(&inner_iter, "name"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT(strstr(val, default_driver_name) != NULL);
   ASSERT(bson_iter_find(&inner_iter, "version"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT(strstr(val, default_driver_version));
}

static void
_handshake_check_os(bson_t *doc)
{
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *val;

   /* Check os type not empty */
   ASSERT(bson_iter_init_find(&md_iter, doc, "os"));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&md_iter));
   ASSERT(bson_iter_recurse(&md_iter, &inner_iter));

   ASSERT(bson_iter_find(&inner_iter, "type"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT(strlen(val) > 0);

   /* Check os version valid */
   ASSERT(bson_iter_find(&inner_iter, "version"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   _check_os_version_valid(val);

   /* Check os arch is valid */
   ASSERT(bson_iter_find(&inner_iter, "architecture"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   _check_arch_string_valid(val);

   /* Not checking os_name, as the spec says it can be NULL. */
}

static void
_handshake_check_env(bson_t *doc, int expected_memory_mb, int expected_timeout_sec, const char *expected_region)
{
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *name;
   ASSERT(bson_iter_init_find(&md_iter, doc, "env"));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&md_iter));
   ASSERT(bson_iter_recurse(&md_iter, &inner_iter));

   ASSERT(bson_iter_find(&inner_iter, "name"));
   name = bson_iter_utf8(&inner_iter, NULL);

   bool is_aws = strstr(name, "aws.lambda") == name;
   bool is_azure = strstr(name, "azure.func") == name;
   bool is_gcp = strstr(name, "gcp.func") == name;
   bool is_vercel = strstr(name, "vercel") == name;
   ASSERT(is_aws || is_azure || is_gcp || is_vercel);

   if (expected_timeout_sec) {
      ASSERT(bson_iter_find(&inner_iter, "timeout_sec"));
      ASSERT(BSON_ITER_HOLDS_INT32(&inner_iter));
      ASSERT_CMPINT32(bson_iter_int32(&inner_iter), ==, expected_timeout_sec);
   }

   if (expected_memory_mb) {
      ASSERT(bson_iter_find(&inner_iter, "memory_mb"));
      ASSERT(BSON_ITER_HOLDS_INT32(&inner_iter));
      ASSERT_CMPINT32(bson_iter_int32(&inner_iter), ==, expected_memory_mb);
   }

   if (expected_region) {
      ASSERT(bson_iter_find(&inner_iter, "region"));
      ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
      ASSERT_CMPSTR(bson_iter_utf8(&inner_iter, NULL), expected_region);
   }
}

static void
_handshake_check_platform(bson_t *doc)
{
   bson_iter_t md_iter;
   const char *val;
   ASSERT(bson_iter_init_find(&md_iter, doc, "platform"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&md_iter));
   val = bson_iter_utf8(&md_iter, NULL);
   ASSERT(val);
   if (strlen(val) < 250) { /* standard val are < 100, may be truncated on some platform */
      ASSERT(strstr(val, default_platform) != NULL);
   }
}

static void
_handshake_check_env_name(bson_t *doc, const char *expected)
{
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   ASSERT(bson_iter_init_find(&md_iter, doc, "env"));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&md_iter));
   ASSERT(bson_iter_recurse(&md_iter, &inner_iter));
   const char *name;

   ASSERT(bson_iter_find(&inner_iter, "name"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   name = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(name);
   ASSERT_CMPSTR(name, expected);
}

static void
_handshake_check_required_fields(bson_t *doc)
{
   _handshake_check_application(doc);
   _handshake_check_driver(doc);
   _handshake_check_os(doc);
   _handshake_check_platform(doc);
}

// Start a mock server, get the driver's handshake doc, and clean up
static bson_t *
_get_handshake_document(bool default_append)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;
   request_t *request;
   const bson_t *request_doc;
   bson_iter_t iter;

   /* Make sure setting the handshake works */
   if (default_append) {
      ASSERT(mongoc_handshake_data_append(default_driver_name, default_driver_version, default_platform));
   }

   server = mock_server_new();
   mock_server_run(server);
   uri = mongoc_uri_copy(mock_server_get_uri(server));
   mongoc_uri_set_option_as_utf8(uri, MONGOC_URI_APPNAME, default_appname);
   pool = test_framework_client_pool_new_from_uri(uri, NULL);

   /* Force topology scanner to start */
   client = mongoc_client_pool_pop(pool);

   request = mock_server_receives_any_hello(server);
   ASSERT(request);
   request_doc = request_get_doc(request, 0);
   ASSERT(request_doc);
   ASSERT(bson_has_field(request_doc, HANDSHAKE_FIELD));

   ASSERT(bson_iter_init_find(&iter, request_doc, HANDSHAKE_FIELD));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&iter));

   uint32_t len;
   const uint8_t *data;
   bson_iter_document(&iter, &len, &data);
   bson_t *handshake_doc = bson_new_from_data(data, len);

   reply_to_request_simple(request, "{'ok': 1, 'isWritablePrimary': true}");
   request_destroy(request);

   /* Cleanup */
   mongoc_client_pool_push(pool, client);
   mongoc_client_pool_destroy(pool);
   mongoc_uri_destroy(uri);
   mock_server_destroy(server);

   return handshake_doc;
}

/* Override host info with plausible but short strings to avoid any
 * truncation */
static void
_override_host_platform_os(void)
{
   _reset_handshake();
   mongoc_handshake_t *md = _mongoc_handshake_get_unfrozen();
   bson_free(md->os_type);
   md->os_type = bson_strdup("Linux");
   bson_free(md->os_name);
   md->os_name = bson_strdup("mongoc");
   bson_free(md->driver_name);
   md->driver_name = bson_strdup("test_e");
   bson_free(md->driver_version);
   md->driver_version = bson_strdup("1.25.0");
   bson_free(md->platform);
   md->platform = bson_strdup("posix=1234");
   bson_free(md->compiler_info);
   md->compiler_info = bson_strdup("CC=GCC");
   bson_free(md->flags);
   md->flags = bson_strdup(" CFLAGS=\"-fPIE\"");
}

// erase all FaaS variables used in testing
static void
clear_faas_env(void)
{
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", ""));
   ASSERT(_mongoc_setenv("AWS_LAMBDA_RUNTIME_API", ""));
   ASSERT(_mongoc_setenv("AWS_REGION", ""));
   ASSERT(_mongoc_setenv("AWS_LAMBDA_FUNCTION_MEMORY_SIZE", ""));
   ASSERT(_mongoc_setenv("FUNCTIONS_WORKER_RUNTIME", ""));
   ASSERT(_mongoc_setenv("K_SERVICE", ""));
   ASSERT(_mongoc_setenv("KUBERNETES_SERVICE_HOST", ""));
   ASSERT(_mongoc_setenv("FUNCTION_MEMORY_MB", ""));
   ASSERT(_mongoc_setenv("FUNCTION_TIMEOUT_SEC", ""));
   ASSERT(_mongoc_setenv("FUNCTION_REGION", ""));
   ASSERT(_mongoc_setenv("VERCEL", ""));
   ASSERT(_mongoc_setenv("VERCEL_REGION", ""));
}

static void
test_mongoc_handshake_data_append_success(void)
{
   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   bson_iter_t md_iter;
   bson_iter_init(&md_iter, doc);
   _handshake_check_application(doc);
   _handshake_check_driver(doc);
   _handshake_check_os(doc);
   _handshake_check_platform(doc);

   bson_destroy(doc);
   _reset_handshake();
}

static void
test_valid_aws_lambda(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("AWS_LAMBDA_RUNTIME_API", "foo"));
   ASSERT(_mongoc_setenv("AWS_REGION", "us-east-2"));
   ASSERT(_mongoc_setenv("AWS_LAMBDA_FUNCTION_MEMORY_SIZE", "1024"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);
   _handshake_check_env(doc, default_memory_mb, 0, "us-east-2");
   _handshake_check_env_name(doc, "aws.lambda");

   bson_iter_t iter;
   ASSERT(bson_iter_init_find(&iter, doc, "env"));
   bson_iter_t inner_iter;
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&iter));
   bson_iter_recurse(&iter, &inner_iter);

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_valid_aws_and_vercel(void *test_ctx)
{
   // Test that Vercel takes precedence over AWS. From the specification:
   // > When variables for multiple ``client.env.name`` values are present,
   // > ``vercel`` takes precedence over ``aws.lambda``

   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", "AWS_Lambda_java8"));
   ASSERT(_mongoc_setenv("AWS_REGION", "us-east-2"));
   ASSERT(_mongoc_setenv("AWS_LAMBDA_FUNCTION_MEMORY_SIZE", "1024"));

   ASSERT(_mongoc_setenv("VERCEL", "1"));
   ASSERT(_mongoc_setenv("VERCEL_REGION", "cdg1"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);
   _handshake_check_env(doc, 0, 0, "cdg1");
   _handshake_check_env_name(doc, "vercel");

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_valid_aws(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", "AWS_Lambda_java8"));
   ASSERT(_mongoc_setenv("AWS_REGION", "us-east-2"));
   ASSERT(_mongoc_setenv("AWS_LAMBDA_FUNCTION_MEMORY_SIZE", "1024"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);

   _handshake_check_required_fields(doc);
   _handshake_check_env(doc, default_memory_mb, 0, "us-east-2");
   _handshake_check_env_name(doc, "aws.lambda");

   bson_iter_t iter;
   ASSERT(bson_iter_init_find(&iter, doc, "env"));
   bson_iter_t inner_iter;
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&iter));
   bson_iter_recurse(&iter, &inner_iter);

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_valid_azure(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("FUNCTIONS_WORKER_RUNTIME", "node"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);
   _handshake_check_env(doc, 0, 0, NULL);
   _handshake_check_env_name(doc, "azure.func");

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_valid_gcp(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("K_SERVICE", "servicename"));
   ASSERT(_mongoc_setenv("FUNCTION_MEMORY_MB", "1024"));
   ASSERT(_mongoc_setenv("FUNCTION_TIMEOUT_SEC", "60"));
   ASSERT(_mongoc_setenv("FUNCTION_REGION", "us-central1"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);
   _handshake_check_env(doc, default_memory_mb, default_timeout_sec, "us-central1");
   _handshake_check_env_name(doc, "gcp.func");

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_valid_vercel(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("VERCEL", "1"));
   ASSERT(_mongoc_setenv("VERCEL_REGION", "cdg1"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);
   _handshake_check_env(doc, 0, 0, "cdg1");
   _handshake_check_env_name(doc, "vercel");

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_multiple_faas(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   // Multiple FaaS variables must cause the entire env field to be omitted
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", "AWS_Lambda_java8"));
   ASSERT(_mongoc_setenv("FUNCTIONS_WORKER_RUNTIME", "node"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);

   ASSERT(!bson_has_field(doc, "env"));

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_truncate_region(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", "AWS_Lambda_java8"));
   const size_t region_len = 512;
   char long_region[512];
   memset(&long_region, 'a', region_len - 1);
   long_region[region_len - 1] = '\0';
   ASSERT(_mongoc_setenv("AWS_REGION", long_region));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);
   _handshake_check_env_name(doc, "aws.lambda");

   bson_iter_t iter;
   ASSERT(bson_iter_init_find(&iter, doc, "env"));
   bson_iter_t inner_iter;
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&iter));
   bson_iter_recurse(&iter, &inner_iter);
   ASSERT(!bson_iter_find(&inner_iter, "memory_mb"));
   ASSERT(!bson_iter_find(&inner_iter, "timeout_sec"));
   ASSERT(!bson_iter_find(&inner_iter, "region"));

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}


static void
test_wrong_types(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", "AWS_Lambda_java8"));
   ASSERT(_mongoc_setenv("AWS_LAMBDA_FUNCTION_MEMORY_SIZE", "big"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);
   _handshake_check_env_name(doc, "aws.lambda");

   bson_iter_t iter;
   ASSERT(bson_iter_init_find(&iter, doc, "env"));
   bson_iter_t inner_iter;
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&iter));
   bson_iter_recurse(&iter, &inner_iter);
   ASSERT(!bson_iter_find(&iter, "memory_mb"));

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_aws_not_lambda(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   // Entire env field must be omitted with non-lambda AWS
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", "EC2"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);

   ASSERT(!bson_has_field(doc, "env"));

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_aws_and_container(void *test_ctx)
{
   BSON_UNUSED(test_ctx);
   ASSERT(_mongoc_setenv("AWS_EXECUTION_ENV", "AWS_Lambda_java8"));
   ASSERT(_mongoc_setenv("AWS_REGION", "us-east-2"));
   ASSERT(_mongoc_setenv("AWS_LAMBDA_FUNCTION_MEMORY_SIZE", "1024"));
   ASSERT(_mongoc_setenv("KUBERNETES_SERVICE_HOST", "1"));

   _override_host_platform_os();
   bson_t *doc = _get_handshake_document(true);
   _handshake_check_required_fields(doc);

   ASSERT_CMPSTR(bson_lookup_utf8(doc, "container.orchestrator"), "kubernetes");
   ASSERT_CMPSTR(bson_lookup_utf8(doc, "env.name"), "aws.lambda");
   _handshake_check_env(doc, default_memory_mb, 0, "us-east-2");

   bson_destroy(doc);
   clear_faas_env();
   _reset_handshake();
}

static void
test_mongoc_handshake_data_append_null_args(void)
{
   bson_iter_t md_iter;
   bson_iter_t inner_iter;
   const char *val;

   /* Make sure setting the handshake works */
   ASSERT(mongoc_handshake_data_append(NULL, NULL, NULL));

   _override_host_platform_os();
   bson_t *handshake_doc = _get_handshake_document(false);
   bson_iter_init(&md_iter, handshake_doc);
   _handshake_check_application(handshake_doc);

   /* Make sure driver.name and driver.version and platform are all right */
   ASSERT(bson_iter_find(&md_iter, "driver"));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&md_iter));
   ASSERT(bson_iter_recurse(&md_iter, &inner_iter));
   ASSERT(bson_iter_find(&inner_iter, "name"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT(strstr(val, " / ") == NULL); /* No append delimiter */

   ASSERT(bson_iter_find(&inner_iter, "version"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT(strstr(val, " / ") == NULL); /* No append delimiter */

   /* Check os type not empty */
   ASSERT(bson_iter_find(&md_iter, "os"));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&md_iter));
   ASSERT(bson_iter_recurse(&md_iter, &inner_iter));

   ASSERT(bson_iter_find(&inner_iter, "type"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT(strlen(val) > 0);

   /* Check os version valid */
   ASSERT(bson_iter_find(&inner_iter, "version"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   ASSERT(strlen(val) > 0);

   /* Check os arch is valid */
   ASSERT(bson_iter_find(&inner_iter, "architecture"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&inner_iter));
   val = bson_iter_utf8(&inner_iter, NULL);
   ASSERT(val);
   _check_arch_string_valid(val);

   /* Not checking os_name, as the spec says it can be NULL. */

   /* Check platform field ok */
   ASSERT(bson_iter_find(&md_iter, "platform"));
   ASSERT(BSON_ITER_HOLDS_UTF8(&md_iter));
   val = bson_iter_utf8(&md_iter, NULL);
   ASSERT(val);
   /* standard val are < 100, may be truncated on some platform */
   if (strlen(val) < 250) {
      /* `printf("%s", NULL)` -> "(null)" with libstdc++, libc++, and STL */
      ASSERT(strstr(val, "null") == NULL);
   }

   bson_destroy(handshake_doc);
}


static void
_test_platform(bool platform_oversized)
{
   mongoc_handshake_t *md;
   size_t platform_len;
   const char *platform_suffix = "b";

   _reset_handshake();

   md = _mongoc_handshake_get_unfrozen();

   bson_free(md->os_type);
   md->os_type = bson_strdup("foo");
   bson_free(md->os_name);
   md->os_name = bson_strdup("foo");
   bson_free(md->os_version);
   md->os_version = bson_strdup("foo");
   bson_free(md->os_architecture);
   md->os_architecture = bson_strdup("foo");
   bson_free(md->driver_name);
   md->driver_name = bson_strdup("foo");
   bson_free(md->driver_version);
   md->driver_version = bson_strdup("foo");

   platform_len = HANDSHAKE_MAX_SIZE;
   if (platform_oversized) {
      platform_len += 100;
   }

   bson_free(md->platform);
   md->platform = bson_malloc(platform_len);
   memset(md->platform, 'a', platform_len - 1);
   md->platform[platform_len - 1] = '\0';

   /* returns true, but ignores the suffix; there's no room */
   ASSERT(mongoc_handshake_data_append(NULL, NULL, platform_suffix));
   ASSERT(!strstr(md->platform, "b"));

   bson_t *doc;
   ASSERT(doc = _mongoc_handshake_build_doc_with_application(md, "my app"));
   ASSERT_CMPUINT32(doc->len, ==, (uint32_t)HANDSHAKE_MAX_SIZE);

   bson_destroy(doc);
   _reset_handshake(); /* frees the strings created above */
}


static void
test_mongoc_handshake_big_platform(void)
{
   _test_platform(false);
}


static void
test_mongoc_handshake_oversized_platform(void)
{
   _test_platform(true);
}


static void
test_mongoc_handshake_data_append_after_cmd(void)
{
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   mongoc_uri_t *uri;

   _reset_handshake();

   uri = mongoc_uri_new("mongodb://127.0.0.1/?" MONGOC_URI_MAXPOOLSIZE "=1");

   /* Make sure that after we pop a client we can't set global handshake */
   pool = test_framework_client_pool_new_from_uri(uri, NULL);

   client = mongoc_client_pool_pop(pool);

   capture_logs(true);
   ASSERT(!mongoc_handshake_data_append("a", "a", "a"));
   capture_logs(false);

   mongoc_client_pool_push(pool, client);

   mongoc_uri_destroy(uri);
   mongoc_client_pool_destroy(pool);

   _reset_handshake();
}

/*
 * Append to the platform field a huge string
 * Make sure that it gets truncated
 */
static void
test_mongoc_handshake_too_big(void)
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

   server = mock_server_new();
   mock_server_run(server);

   _reset_handshake();

   memset(big_string, 'a', BUFFER_SIZE - 1);
   big_string[BUFFER_SIZE - 1] = '\0';
   ASSERT(mongoc_handshake_data_append(NULL, NULL, big_string));

   uri = mongoc_uri_copy(mock_server_get_uri(server));
   /* avoid rare test timeouts */
   mongoc_uri_set_option_as_int32(uri, MONGOC_URI_CONNECTTIMEOUTMS, 20000);

   client = test_framework_client_new_from_uri(uri, NULL);

   ASSERT(mongoc_client_set_appname(client, "my app"));

   /* Send a ping, mock server deals with it */
   future = future_client_command_simple(client, "admin", tmp_bson("{'ping': 1}"), NULL, NULL, NULL);
   request = mock_server_receives_any_hello(server);

   /* Make sure the hello request has a handshake field, and it's not huge */
   ASSERT(request);
   hello_doc = request_get_doc(request, 0);
   ASSERT(hello_doc);
   ASSERT(bson_has_field(hello_doc, HANDSHAKE_FIELD));

   /* hello with handshake isn't too big */
   bson_iter_init_find(&iter, hello_doc, HANDSHAKE_FIELD);
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&iter));
   bson_iter_document(&iter, &len, &dummy);

   /* Should have truncated the platform field so it fits exactly */
   ASSERT(len == HANDSHAKE_MAX_SIZE);

   reply_to_request_simple(
      request, tmp_str("{'ok': 1, 'minWireVersion': %d, 'maxWireVersion': %d}", WIRE_VERSION_MIN, WIRE_VERSION_MAX));
   request_destroy(request);

   request = mock_server_receives_msg(server, MONGOC_MSG_NONE, tmp_bson("{'$db': 'admin', 'ping': 1}"));

   reply_to_request_simple(request, "{'ok': 1}");
   ASSERT(future_get_bool(future));

   future_destroy(future);
   request_destroy(request);
   mongoc_client_destroy(client);
   mongoc_uri_destroy(uri);
   mock_server_destroy(server);

   /* So later tests don't have "aaaaa..." as the md platform string */
   _reset_handshake();
}

/*
 * Testing whether platform string data is truncated/dropped appropriately
 * drop specifies what case should be tested for
 */
static void
test_mongoc_platform_truncate(int drop)
{
   mongoc_handshake_t *md;
   bson_iter_t iter;

   char *undropped;
   char *expected;
   char big_string[HANDSHAKE_MAX_SIZE];
   memset(big_string, 'a', HANDSHAKE_MAX_SIZE - 1);
   big_string[HANDSHAKE_MAX_SIZE - 1] = '\0';

   /* Need to know how much space storing fields in our BSON will take
    * so that we can make our platform string the correct length here */
   _reset_handshake();

   /* we manually bypass the defaults of the handshake to ensure an exceedingly
    * long field does not cause our test to incorrectly fail */
   md = _mongoc_handshake_get_unfrozen();
   bson_free(md->os_type);
   md->os_type = bson_strdup("test_a");
   bson_free(md->os_name);
   md->os_name = bson_strdup("test_b");
   bson_free(md->os_architecture);
   md->os_architecture = bson_strdup("test_d");
   bson_free(md->driver_name);
   md->driver_name = bson_strdup("test_e");
   bson_free(md->driver_version);
   md->driver_version = bson_strdup("test_f");

   // Set all fields used to generate the platform string to empty
   bson_free(md->platform);
   md->platform = bson_strdup("");
   bson_free(md->compiler_info);
   md->compiler_info = bson_strdup("");
   bson_free(md->flags);
   md->flags = bson_strdup("");

   // Set os_version to a long string to force drop all os fields except name
   bson_free(md->os_version);
   md->os_version = big_string;

   bson_t *handshake_no_platform =
      _mongoc_handshake_build_doc_with_application(_mongoc_handshake_get_unfrozen(), default_appname);
   size_t handshake_remaining_space = HANDSHAKE_MAX_SIZE - handshake_no_platform->len;
   bson_destroy(handshake_no_platform);

   md->os_version = bson_strdup("test_c");
   bson_free(md->compiler_info);
   md->compiler_info = bson_strdup("test_g");
   bson_free(md->flags);
   md->flags = bson_strdup("test_h");

   /* adjust remaining space depending on which combination of
    * flags/compiler_info we want to test dropping */
   if (drop == 2) {
      undropped = bson_strdup("");
   } else if (drop == 1) {
      handshake_remaining_space -= strlen(" / ") + strlen(md->compiler_info);
      undropped = bson_strdup_printf(" / %s", md->compiler_info);
   } else {
      handshake_remaining_space -= strlen(" / ") + strlen(md->flags) + strlen(md->compiler_info);
      undropped = bson_strdup_printf(" / %s%s", md->compiler_info, md->flags);
   }

   big_string[handshake_remaining_space] = '\0';
   ASSERT(mongoc_handshake_data_append(NULL, NULL, big_string));

   bson_t *doc;
   ASSERT(doc = _mongoc_handshake_build_doc_with_application(_mongoc_handshake_get_unfrozen(), default_appname));

   /* doc.len being strictly less than HANDSHAKE_MAX_SIZE proves that we have
    * dropped the flags correctly, instead of truncating anything
    */

   ASSERT_CMPUINT32(doc->len, <=, (uint32_t)HANDSHAKE_MAX_SIZE);
   ASSERT(bson_iter_init_find(&iter, doc, "platform"));
   expected = bson_strdup_printf("%s%s", big_string, undropped);
   ASSERT_CMPSTR(bson_iter_utf8(&iter, NULL), expected);

   bson_free(expected);
   bson_free(undropped);
   bson_destroy(doc);
   /* So later tests don't have "aaaaa..." as the md platform string */
   _reset_handshake();
}

/*
 * Test dropping neither compiler_info/flags, dropping just flags, and dropping
 * both
 */
static void
test_mongoc_oversized_flags(void)
{
   test_mongoc_platform_truncate(0);
   test_mongoc_platform_truncate(1);
   test_mongoc_platform_truncate(2);
}

/* Test the case where we can't prevent the handshake doc being too big
 * and so we just don't send it */
static void
test_mongoc_handshake_cannot_send(void)
{
   mock_server_t *server;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_client_pool_t *pool;
   request_t *request;
   const char *const server_reply = "{'ok': 1, 'isWritablePrimary': true}";
   const bson_t *request_doc;
   char big_string[HANDSHAKE_MAX_SIZE];

   _reset_handshake();
   capture_logs(true);

   memset(big_string, 'a', HANDSHAKE_MAX_SIZE - 1);
   big_string[HANDSHAKE_MAX_SIZE - 1] = '\0';

   /* The handshake cannot be built if a field that cannot be dropped
    * (os.type) is set to a very long string */
   mongoc_handshake_t *md = _mongoc_handshake_get_unfrozen();
   bson_free(md->os_type);
   md->os_type = bson_strdup(big_string);

   server = mock_server_new();
   mock_server_run(server);
   uri = mongoc_uri_copy(mock_server_get_uri(server));
   mongoc_uri_set_option_as_int32(uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500);
   pool = test_framework_client_pool_new_from_uri(uri, NULL);

   /* Pop a client to trigger the topology scanner */
   client = mongoc_client_pool_pop(pool);
   request = mock_server_receives_any_hello(server);

   /* Make sure the hello request DOESN'T have a handshake field: */
   ASSERT(request);
   request_doc = request_get_doc(request, 0);
   ASSERT(request_doc);
   ASSERT(!bson_has_field(request_doc, HANDSHAKE_FIELD));

   reply_to_request_simple(request, server_reply);
   request_destroy(request);

   /* Cause failure on client side */
   request = mock_server_receives_any_hello(server);
   ASSERT(request);
   reply_to_request_with_hang_up(request);
   request_destroy(request);

   /* Make sure the hello request still DOESN'T have a handshake field
    * on subsequent heartbeats. */
   request = mock_server_receives_any_hello(server);
   ASSERT(request);
   request_doc = request_get_doc(request, 0);
   ASSERT(request_doc);
   ASSERT(!bson_has_field(request_doc, HANDSHAKE_FIELD));

   reply_to_request_simple(request, server_reply);
   request_destroy(request);

   /* cleanup */
   mongoc_client_pool_push(pool, client);

   mongoc_client_pool_destroy(pool);
   mongoc_uri_destroy(uri);
   mock_server_destroy(server);

   /* Reset again so the next tests don't have a handshake doc which
    * is too big */
   _reset_handshake();
}

/* Test the case where the driver does not raise an error if saslSupportedMechs attribute
of the initial handshake reply contains an unknown mechanism. */
static void
test_mongoc_handshake_no_validation_for_sasl_supported_mech(void)
{
   mongoc_client_t *client;
   mock_server_t *server;
   mongoc_uri_t *uri;
   future_t *future;
   request_t *request;
   const bson_t *doc;

   server = mock_server_new();
   mock_server_run(server);

   uri = mongoc_uri_copy(mock_server_get_uri(server));
   /* avoid rare test timeouts */
   mongoc_uri_set_option_as_int32(uri, MONGOC_URI_CONNECTTIMEOUTMS, 20000);

   client = test_framework_client_new_from_uri(uri, NULL);
   ASSERT(mongoc_client_set_appname(client, "my app"));

   /* Send a ping where 'saslSupportedMechs' contains an arbitrary string  */
   future = future_client_command_simple(
      client, "admin", tmp_bson("{'ping': 1, 'saslSupportedMechs': 'unknownMechanism'}"), NULL, NULL, NULL);
   ASSERT(future);
   request = mock_server_receives_any_hello(server);
   ASSERT(request);

   reply_to_request_simple(
      request, tmp_str("{'ok': 1, 'minWireVersion': %d, 'maxWireVersion': %d}", WIRE_VERSION_MIN, WIRE_VERSION_MAX));
   request_destroy(request);
   request = mock_server_receives_msg(server, MONGOC_MSG_NONE, tmp_bson("{'$db': 'admin', 'ping': 1}"));

   ASSERT(request);
   doc = request_get_doc(request, 0);
   ASSERT(doc);
   ASSERT(bson_has_field(doc, "saslSupportedMechs"));

   reply_to_request_simple(request, "{'ok': 1}");
   ASSERT(future_get_bool(future));

   future_destroy(future);
   request_destroy(request);
   mongoc_client_destroy(client);
   mongoc_uri_destroy(uri);
   mock_server_destroy(server);

   ASSERT_NO_CAPTURED_LOGS("mongoc_handshake_no_validation_for_sasl_supported_mechs");
}

extern char *
_mongoc_handshake_get_config_hex_string(void);

static bool
_get_bit(char *config_str, uint32_t bit)
{
   /* get the location of the two byte chars for this bit. */
   uint32_t byte = bit / 8;
   uint32_t bit_of_byte = bit % 8;
   uint32_t char_loc;
   uint32_t as_num;
   char byte_str[3];

   /* byte 0 is represented at the last two characters. */
   char_loc = (uint32_t)strlen(config_str) - 2 - (byte * 2);
   /* index should be past the prefixed "0x" */
   ASSERT_CMPINT32(char_loc, >, 1);
   /* get the number representation of the byte. */
   byte_str[0] = config_str[char_loc];
   byte_str[1] = config_str[char_loc + 1];
   byte_str[2] = '\0';
   as_num = (uint8_t)strtol(byte_str, NULL, 16);
   return (as_num & (1u << bit_of_byte)) > 0u;
}

void
test_handshake_platform_config(void)
{
   /* Parse the config string, and check that it matches the defined flags. */
   char *config_str = _mongoc_handshake_get_config_hex_string();
   uint32_t total_bytes = (LAST_MONGOC_MD_FLAG + 7) / 8;
   uint32_t total_bits = 8 * total_bytes;
   uint32_t i;
   /* config_str should have the form 0x?????. */
   ASSERT_CMPINT((int)strlen(config_str), ==, 2 + (2 * total_bytes));
   BSON_ASSERT(strncmp(config_str, "0x", 2) == 0);

/* go through all flags. */
#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
   BSON_ASSERT(_get_bit(config_str, MONGOC_ENABLE_SSL_SECURE_CHANNEL));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_CNG
   BSON_ASSERT(_get_bit(config_str, MONGOC_ENABLE_CRYPTO_CNG));
#endif

#ifdef MONGOC_HAVE_BCRYPT_PBKDF2
   BSON_ASSERT(_get_bit(config_str, MONGOC_HAVE_BCRYPT_PBKDF2));
#endif

#ifdef MONGOC_ENABLE_SSL_SECURE_TRANSPORT
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SSL_SECURE_TRANSPORT));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO_COMMON_CRYPTO));
#endif

#ifdef MONGOC_ENABLE_SSL_OPENSSL
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SSL_OPENSSL));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_LIBCRYPTO
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO_LIBCRYPTO));
#endif

#ifdef MONGOC_ENABLE_SSL
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SSL));
#endif

#ifdef MONGOC_ENABLE_CRYPTO
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO));
#endif

#ifdef MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_CRYPTO_SYSTEM_PROFILE));
#endif

#ifdef MONGOC_ENABLE_SASL
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SASL));
#endif

#ifdef MONGOC_HAVE_SASL_CLIENT_DONE
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_HAVE_SASL_CLIENT_DONE));
#endif

   BSON_ASSERT(!_get_bit(config_str, MONGOC_MD_FLAG_NO_AUTOMATIC_GLOBALS_UNUSED)); // Flag was removed.

#ifdef MONGOC_EXPERIMENTAL_FEATURES
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_EXPERIMENTAL_FEATURES));
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SASL_CYRUS));
#endif

#ifdef MONGOC_ENABLE_SASL_SSPI
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SASL_SSPI));
#endif

#ifdef MONGOC_HAVE_SOCKLEN
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_HAVE_SOCKLEN));
#endif

#ifdef MONGOC_ENABLE_COMPRESSION
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_COMPRESSION));
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_COMPRESSION_SNAPPY));
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_COMPRESSION_ZLIB));
#endif

#ifdef MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI));
#endif

#ifdef MONGOC_HAVE_RES_NSEARCH
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_RES_NSEARCH));
#endif

#ifdef MONGOC_HAVE_RES_NDESTROY
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_RES_NDESTROY));
#endif

#ifdef MONGOC_HAVE_RES_NCLOSE
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_RES_NCLOSE));
#endif

#ifdef MONGOC_HAVE_RES_SEARCH
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_RES_SEARCH));
#endif

#ifdef MONGOC_HAVE_DNSAPI
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_DNSAPI));
#endif

#ifdef MONGOC_HAVE_RDTSCP
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_RDTSCP));
#endif

#ifdef MONGOC_HAVE_SCHED_GETCPU
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_HAVE_SCHED_GETCPU));
#endif

   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SRV) == MONGOC_SRV_ENABLED);

#ifdef MONGOC_ENABLE_SHM_COUNTERS
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_SHM_COUNTERS));
#endif

   mlib_diagnostic_push();
   mlib_disable_constant_conditional_expression_warnings();
   if (MONGOC_TRACE_ENABLED) {
      BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_TRACE));
   }
   mlib_diagnostic_pop();

   // Check that `MONGOC_MD_FLAG_ENABLE_ICU` is always unset. libicu dependency
   // was removed in CDRIVER-4680.
   BSON_ASSERT(!_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_ICU_UNUSED));

#ifdef MONGOC_ENABLE_CLIENT_SIDE_ENCRYPTION
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_CLIENT_SIDE_ENCRYPTION));
#endif

#ifdef MONGOC_ENABLE_MONGODB_AWS_AUTH
   BSON_ASSERT(_get_bit(config_str, MONGOC_MD_FLAG_ENABLE_MONGODB_AWS_AUTH));
#endif

   /* any excess bits should all be zero. */
   for (i = LAST_MONGOC_MD_FLAG; i < total_bits; i++) {
      BSON_ASSERT(!_get_bit(config_str, i));
   }
   bson_free(config_str);
}

/* Called by a single thread in test_mongoc_handshake_race_condition */
static BSON_THREAD_FUN(handshake_append_worker, data)
{
   BSON_UNUSED(data);

   mongoc_handshake_data_append(default_driver_name, default_driver_version, default_platform);

   BSON_THREAD_RETURN;
}

/* Run 1000 iterations of mongoc_handshake_data_append() using 4 threads */
static void
test_mongoc_handshake_race_condition(void)
{
   unsigned i, j;
   bson_thread_t threads[4];

   for (i = 0; i < 1000; ++i) {
      _reset_handshake();

      for (j = 0; j < 4; ++j) {
         BSON_ASSERT(!mcommon_thread_create(&threads[j], &handshake_append_worker, NULL /* args */));
      }
      for (j = 0; j < 4; ++j) {
         mcommon_thread_join(threads[j]);
      }
   }

   _reset_handshake();
}

static void
test_mongoc_handshake_cpp(void)
{
   bson_t *handshake = _mongoc_handshake_build_doc_with_application(_mongoc_handshake_get_unfrozen(), "foo");
   const char *platform = bson_lookup_utf8(handshake, "platform");
   if (0 != strlen(MONGOC_CXX_COMPILER_VERSION)) {
      ASSERT_CONTAINS(platform, "CXX=" MONGOC_CXX_COMPILER_ID " " MONGOC_CXX_COMPILER_VERSION);
   } else {
      ASSERT_CONTAINS(platform, "CXX=" MONGOC_CXX_COMPILER_ID);
   }
   bson_destroy(handshake);
}

// Prose test 9 in the Handshake spec validates the presence of the backpressure flag, but we instead use a mock server
// test because the C Driver lacks a mechanism for inspecting handshake documents sent to the server.
static void
test_mongoc_handshake_includes_backpressure_flag(void)
{
   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   mongoc_client_pool_t *const pool = test_framework_client_pool_new_from_uri(mock_server_get_uri(server), NULL);

   mongoc_client_t *const client = mongoc_client_pool_pop(pool);

   request_t *const request = mock_server_receives_any_hello(server);
   ASSERT(request);

   const bson_t *const request_doc = request_get_doc(request, 0);
   ASSERT(request_doc);
   ASSERT(bson_has_field(request_doc, HANDSHAKE_BACKPRESSURE_FIELD));
   bson_iter_t iter;
   ASSERT(bson_iter_init_find(&iter, request_doc, HANDSHAKE_BACKPRESSURE_FIELD));
   ASSERT(BSON_ITER_HOLDS_BOOL(&iter));
   ASSERT(bson_iter_bool(&iter));

   reply_to_request_simple(request, "{'ok': 1, 'isWritablePrimary': true}");

   request_destroy(request);
   mongoc_client_pool_push(pool, client);
   mongoc_client_pool_destroy(pool);
   mock_server_destroy(server);
}

// `maxIdleTimeMS` is not supported. Use hangups to force subsequent handshakes instead.
// Override `minHeartbeatFrequencyMS` (500ms) to avoid slowdowns from hangups.
static mongoc_client_t *
_test_metadata_append_setup_client(mock_server_t *server)
{
   mongoc_client_t *const client = test_framework_client_new_from_uri(mock_server_get_uri(server), NULL);
   client->topology->min_heartbeat_frequency_msec = 0;
   return client;
}

// Return a copy of the "client" (metadata) field.
static bson_t *
_test_metadata_append_get_metadata(request_t *request)
{
   const bson_t *req_doc = request_get_doc(request, 0);
   ASSERT(req_doc);

   bson_iter_t iter;
   ASSERT(bson_iter_init_find(&iter, req_doc, HANDSHAKE_FIELD));
   ASSERT(BSON_ITER_HOLDS_DOCUMENT(&iter));

   uint32_t len;
   const uint8_t *data;
   bson_iter_document(&iter, &len, &data);
   return bson_new_from_data(data, len);
}

static void
_test_metadata_append_reply_hello_and_destroy(request_t *request)
{
   BSON_ASSERT_PARAM(request);

   reply_to_request_simple(request,
                           tmp_str("{'ok': 1, 'isWritablePrimary': true, 'minWireVersion': %d, 'maxWireVersion': %d}",
                                   WIRE_VERSION_MIN,
                                   WIRE_VERSION_MAX));
   request_destroy(request);
}

static void
_set_initial_metadata(const char *name, const char *version, const char *platform)
{
   ASSERT(!_mongoc_handshake_is_frozen());

   // Avoid noise in handshake platform string.
   {
      mongoc_handshake_t *const md = _mongoc_handshake_get_unfrozen();

      bson_free(md->compiler_info);
      bson_free(md->flags);

      md->compiler_info = NULL;
      md->flags = NULL;
   }

   ASSERT(mongoc_handshake_data_append(name, version, platform));
}

typedef struct driver_info_options {
   const char *name;
   const char *version;
   const char *platform;
} driver_info_options;

static bson_t *
_handshake_metadata_append_ping_capture(mock_server_t *server, mongoc_client_t *client)
{
   BSON_ASSERT_PARAM(server);
   BSON_ASSERT_PARAM(client);

   bson_t *metadata = NULL;

   // Setup Step 2: Send a `ping` command to the server and verify that the command succeeds.
   future_t *future = future_client_command_simple(client, "admin", tmp_bson("{'ping': 1}"), NULL, NULL, NULL);

   // Setup Step 3: Save intercepted `client` document as `initialClientMetadata`.
   {
      request_t *const request = mock_server_receives_any_hello(server);
      ASSERT(request);

      metadata = _test_metadata_append_get_metadata(request);
      ASSERT(metadata);

      _test_metadata_append_reply_hello_and_destroy(request);
   }

   // Setup Step 4: Wait 5ms for the connection to become idle.
   //
   // `maxIdleTimeMS` is not supported. Use hangups to force subsequent handshakes instead.
   {
      request_t *const request =
         mock_server_receives_msg(server, MONGOC_MSG_NONE, tmp_bson("{'$db': 'admin', 'ping': 1}"));
      reply_to_request_with_hang_up(request);
      request_destroy(request);
   }

   ASSERT(!future_get_bool(future)); // Hang up: network error.
   future_destroy(future);

   return metadata;
}

static bson_t *
_handshake_metadata_append_setup(mock_server_t *server, mongoc_client_t *client, const driver_info_options *options)
{
   BSON_ASSERT_PARAM(server);
   BSON_ASSERT_PARAM(client);

   // Setup Step 1: Create a MongoClient instance with the following:
   _set_initial_metadata(options->name, options->version, options->platform);

   return _handshake_metadata_append_ping_capture(server, client);
}

// All other subfields in the `client` document remain unchanged from `initialClientMetadata`.
static void
_test_metadata_append_check_other_subfields(const bson_t *initial_client_metadata,
                                            const bson_t *updated_client_metadata)
{
   // client.driver (other subfields)
   {
      bson_t initial = BSON_INITIALIZER;
      bson_t updated = BSON_INITIALIZER;

      bson_t *const initial_driver = bson_lookup_bson(initial_client_metadata, "driver");
      bson_t *const updated_driver = bson_lookup_bson(updated_client_metadata, "driver");

      ASSERT(initial_driver);
      ASSERT(updated_driver);

      bson_copy_to_excluding_noinit(initial_driver, &initial, "name", "version", NULL);
      bson_copy_to_excluding_noinit(updated_driver, &updated, "name", "version", NULL);

      ASSERT_EQUAL_BSON(&initial, &updated);

      bson_destroy(initial_driver);
      bson_destroy(updated_driver);

      bson_destroy(&initial);
      bson_destroy(&updated);
   }

   // client (other subfields)
   {
      bson_t initial = BSON_INITIALIZER;
      bson_t updated = BSON_INITIALIZER;

      bson_copy_to_excluding_noinit(initial_client_metadata, &initial, "driver", "platform", NULL);
      bson_copy_to_excluding_noinit(updated_client_metadata, &updated, "driver", "platform", NULL);

      ASSERT_EQUAL_BSON(&initial, &updated);

      bson_destroy(&initial);
      bson_destroy(&updated);
   }
}

static void
_test_metadata_append_ping_check(mongoc_client_t *client,
                                 mock_server_t *server,
                                 const bson_t *initial_client_metadata,
                                 const driver_info_options *options,
                                 void (*check_fn)(const bson_t *initial_client_metadata,
                                                  const bson_t *updated_client_metadata,
                                                  const driver_info_options *options))
{
   future_t *const future = future_client_command_simple(client, "admin", tmp_bson("{'ping': 1}"), NULL, NULL, NULL);
   ASSERT(future);

   {
      request_t *const request = mock_server_receives_any_hello(server);
      ASSERT(request);

      bson_t *const updated_client_metadata = _test_metadata_append_get_metadata(request);
      ASSERT(updated_client_metadata);

      check_fn(initial_client_metadata, updated_client_metadata, options);
      _test_metadata_append_reply_hello_and_destroy(request);

      bson_destroy(updated_client_metadata);
   }

   {
      request_t *const request =
         mock_server_receives_msg(server, MONGOC_MSG_NONE, tmp_bson("{'$db': 'admin', 'ping': 1}"));
      reply_to_request_simple(request, "{'ok': 1}");
      request_destroy(request);
   }

   // ... the command succeeds.
   ASSERT(future_get_bool(future)); // {"ok": 1}
   future_destroy(future);
}

static void
_test_metadata_append_unique_check(const bson_t *initial_client_metadata,
                                   const bson_t *updated_client_metadata,
                                   const driver_info_options *options)
{
   BSON_ASSERT_PARAM(initial_client_metadata);
   BSON_ASSERT_PARAM(updated_client_metadata);
   BSON_ASSERT_PARAM(options);

   // client.driver.name
   {
      const char *const initial_driver_name = bson_lookup_utf8(initial_client_metadata, "driver.name");
      const char *const updated_driver_name = bson_lookup_utf8(updated_client_metadata, "driver.name");

      ASSERT(initial_driver_name);
      ASSERT(updated_driver_name);

      if (options->name) {
         // - If test case's name is non-null: `library|<name>`.
         ASSERT_CMPSTR(updated_driver_name, tmp_str("%s / %s", initial_driver_name, options->name));
      } else {
         // - Otherwise, the field remains unchanged: `library`.
         ASSERT_CMPSTR(updated_driver_name, initial_driver_name);
      }
   }

   // client.driver.version
   {
      const char *const initial_driver_version = bson_lookup_utf8(initial_client_metadata, "driver.version");
      const char *const updated_driver_version = bson_lookup_utf8(updated_client_metadata, "driver.version");

      ASSERT(initial_driver_version);
      ASSERT(updated_driver_version);

      if (options->version) {
         // - If test case's version is non-null: `1.2|<version>`.
         char *expected_version = bson_strdup_printf("%s / %s", initial_driver_version, options->version);
         ASSERT_CMPSTR(updated_driver_version, expected_version);
         bson_free(expected_version);
      } else {
         // - Otherwise, the field remains unchanged: `1.2`.
         ASSERT_CMPSTR(updated_driver_version, initial_driver_version);
      }
   }

   // client.platform
   {
      const char *const initial_platform = bson_lookup_utf8(initial_client_metadata, "platform");
      const char *const updated_platform = bson_lookup_utf8(updated_client_metadata, "platform");

      ASSERT(initial_platform);
      ASSERT(updated_platform);

      if (options->platform) {
         // - If test case's platform is non-null: `Library Platform|<platform>`.
         ASSERT_CMPSTR(updated_platform, tmp_str("%s / %s", initial_platform, options->platform));
      } else {
         // - Otherwise, the field remains unchanged: `Library Platform`.
         ASSERT_CMPSTR(updated_platform, initial_platform);
      }
   }

   _test_metadata_append_check_other_subfields(initial_client_metadata, updated_client_metadata);
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 1: Drivers should verify that metadata provided
// after `MongoClient` initialization is appended, not replaced, and is visible in the `hello` command of new
// connections.
static void
test_handshake_metadata_append_single_impl(const driver_info_options *options)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_t *const initial_client_metadata = _handshake_metadata_append_setup(server,
                                                                            client,
                                                                            &(driver_info_options){
                                                                               .name = "library",
                                                                               .version = "1.2",
                                                                               .platform = "Library Platform",
                                                                            });

   // Step 1: Append the `DriverInfoOptions` from the selected test case to the `MongoClient` metadata.
   ASSERT(mongoc_client_append_metadata(client, options->name, options->version, options->platform));

   // Step 2: Send a `ping` command to the server and verify:
   _test_metadata_append_ping_check(
      client, server, initial_client_metadata, options, _test_metadata_append_unique_check);

   bson_destroy(initial_client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

static void
test_handshake_metadata_append_single_case_1(void)
{
   test_handshake_metadata_append_single_impl(&(driver_info_options){
      .name = "framework",
      .version = "2.0",
      .platform = "Framework Platform",
   });
}

static void
test_handshake_metadata_append_single_case_2(void)
{
   test_handshake_metadata_append_single_impl(&(driver_info_options){
      .name = "framework",
      .version = "2.0",
      .platform = NULL,
   });
}

static void
test_handshake_metadata_append_single_case_3(void)
{
   test_handshake_metadata_append_single_impl(&(driver_info_options){
      .name = "framework",
      .version = NULL,
      .platform = "Framework Platform",
   });
}

static void
test_handshake_metadata_append_single_case_4(void)
{
   test_handshake_metadata_append_single_impl(&(driver_info_options){
      .name = "framework",
      .version = NULL,
      .platform = NULL,
   });
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 2: Drivers should verify that after `MongoClient`
// initialization, metadata can be updated multiple times, not replaced, and is visible in the `hello` command of new
// connections.
static void
test_handshake_metadata_append_multiple_impl(const driver_info_options *options)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_destroy(_handshake_metadata_append_setup(server, client, &(driver_info_options){0}));

   // Setup Step 1: Create a `MongoClient` instance with: ...
   // Setup Step 2: Append the following `DriverInfoOptions` to the `MongoClient` metadata: ...
   ASSERT(mongoc_client_append_metadata(client, "library", "1.2", "Library Platform"));

   // Setup Step 3: Send a `ping` command to the server and verify that the command succeeds.
   // Setup Step 4: Save intercepted client document as `updatedClientMetadata`.
   // Setup Step 5: Wait 5ms for the connection to become idle (use hangups instead).
   bson_t *const updated_client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 1: Append the `DriverInfoOptions` from the selected test case to the `MongoClient` metadata.
   ASSERT(mongoc_client_append_metadata(client, options->name, options->version, options->platform));

   // Step 2: Send a `ping` command to the server and verify:
   _test_metadata_append_ping_check(
      client, server, updated_client_metadata, options, _test_metadata_append_unique_check);

   bson_destroy(updated_client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

static void
test_handshake_metadata_append_multiple_case_1(void)
{
   test_handshake_metadata_append_multiple_impl(&(driver_info_options){
      .name = "framework",
      .version = "2.0",
      .platform = "Framework Platform",
   });
}

static void
test_handshake_metadata_append_multiple_case_2(void)
{
   test_handshake_metadata_append_multiple_impl(&(driver_info_options){
      .name = "framework",
      .version = "2.0",
      .platform = NULL,
   });
}

static void
test_handshake_metadata_append_multiple_case_3(void)
{
   test_handshake_metadata_append_multiple_impl(&(driver_info_options){
      .name = "framework",
      .version = NULL,
      .platform = "Framework Platform",
   });
}

static void
test_handshake_metadata_append_multiple_case_4(void)
{
   test_handshake_metadata_append_multiple_impl(&(driver_info_options){
      .name = "framework",
      .version = NULL,
      .platform = NULL,
   });
}

static void
_test_metadata_append_duplicate_check(const bson_t *initial_client_metadata,
                                      const bson_t *updated_client_metadata,
                                      const driver_info_options *options)
{
   BSON_ASSERT_PARAM(initial_client_metadata);
   BSON_ASSERT_PARAM(updated_client_metadata);
   BSON_ASSERT_PARAM(options);

   ASSERT(options->name);
   ASSERT(options->version);
   ASSERT(options->platform);

   const char *const initial_driver_name = bson_lookup_utf8(initial_client_metadata, "driver.name");
   const char *const initial_driver_version = bson_lookup_utf8(initial_client_metadata, "driver.version");
   const char *const initial_driver_platform = bson_lookup_utf8(initial_client_metadata, "platform");

   ASSERT(initial_driver_name);
   ASSERT(initial_driver_version);
   ASSERT(initial_driver_platform);

   const char *const updated_driver_name = bson_lookup_utf8(updated_client_metadata, "driver.name");
   const char *const updated_driver_version = bson_lookup_utf8(updated_client_metadata, "driver.version");
   const char *const updated_driver_platform = bson_lookup_utf8(updated_client_metadata, "platform");

   ASSERT(updated_driver_name);
   ASSERT(updated_driver_version);
   ASSERT(updated_driver_platform);

   // If the test case's DriverInfo is identical to the driver info from setup step 2 (test case 1):
   if (strcmp(options->name, "library") == 0 && strcmp(options->version, "1.2") == 0 &&
       strcmp(options->platform, "Library Platform") == 0) {
      // - Assert `metadata.driver.name` is equal to `library`
      ASSERT_CMPSTR(updated_driver_name, initial_driver_name);

      // - Assert `metadata.driver.version` is equal to `1.2`
      ASSERT_CMPSTR(updated_driver_version, initial_driver_version);

      // - Assert `metadata.platform` is equal to `Library Platform`
      ASSERT_CMPSTR(updated_driver_platform, initial_driver_platform);
   }

   // Otherwise:
   else {
      // - Assert `metadata.driver.name` is equal to `library|<name>`
      ASSERT_CMPSTR(updated_driver_name, tmp_str("%s / %s", initial_driver_name, options->name));

      // - Assert `metadata.driver.version` is equal to `1.2|<version>`
      ASSERT_CMPSTR(updated_driver_version, tmp_str("%s / %s", initial_driver_version, options->version));

      // - Assert `metadata.platform` is equal to `Library Platform|<platform>`
      ASSERT_CMPSTR(updated_driver_platform, tmp_str("%s / %s", initial_driver_platform, options->platform));
   }

   _test_metadata_append_check_other_subfields(initial_client_metadata, updated_client_metadata);
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 3: Multiple Successive Metadata Updates with
// Duplicate Data
static void
test_handshake_metadata_append_duplicate_impl(const driver_info_options *options)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   // Setup Step 1: Create a `MongoClient` instance with: ...
   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_destroy(_handshake_metadata_append_setup(server, client, &(driver_info_options){0}));

   // Setup Step 2: Append the following `DriverInfoOptions` to the `MongoClient` metadata: ...
   ASSERT(mongoc_client_append_metadata(client, "library", "1.2", "Library Platform"));

   // Setup Step 3: Send a `ping` command to the server and verify that the command succeeds.
   // Setup Step 4: Save intercepted `client` document as `updatedClientMetadata`.
   // Setup Step 5: Wait 5ms for the connection to become idle (use hangups instead).
   bson_t *const initial_client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 1: Append the `DriverInfoOptions` from the selected test case to the `MongoClient` metadata.
   ASSERT(mongoc_client_append_metadata(client, options->name, options->version, options->platform));

   // Step 2: Send a `ping` command to the server and verify:
   _test_metadata_append_ping_check(
      client, server, initial_client_metadata, options, _test_metadata_append_duplicate_check);

   bson_destroy(initial_client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

static void
test_handshake_metadata_append_duplicate_case_1(void)
{
   test_handshake_metadata_append_duplicate_impl(&(driver_info_options){
      .name = "library",
      .version = "1.2",
      .platform = "Library Platform",
   });
}

static void
test_handshake_metadata_append_duplicate_case_2(void)
{
   test_handshake_metadata_append_duplicate_impl(&(driver_info_options){
      .name = "framework",
      .version = "1.2",
      .platform = "Library Platform",
   });
}

static void
test_handshake_metadata_append_duplicate_case_3(void)
{
   test_handshake_metadata_append_duplicate_impl(&(driver_info_options){
      .name = "library",
      .version = "2.0",
      .platform = "Library Platform",
   });
}

static void
test_handshake_metadata_append_duplicate_case_4(void)
{
   test_handshake_metadata_append_duplicate_impl(&(driver_info_options){
      .name = "library",
      .version = "1.2",
      .platform = "Framework Platform",
   });
}

static void
test_handshake_metadata_append_duplicate_case_5(void)
{
   test_handshake_metadata_append_duplicate_impl(&(driver_info_options){
      .name = "framework",
      .version = "2.0",
      .platform = "Library Platform",
   });
}

static void
test_handshake_metadata_append_duplicate_case_6(void)
{
   test_handshake_metadata_append_duplicate_impl(&(driver_info_options){
      .name = "framework",
      .version = "1.2",
      .platform = "Framework Platform",
   });
}

static void
test_handshake_metadata_append_duplicate_case_7(void)
{
   test_handshake_metadata_append_duplicate_impl(&(driver_info_options){
      .name = "library",
      .version = "2.0",
      .platform = "Framework Platform",
   });
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 4: Multiple Metadata Updates with Duplicate Data
static void
test_handshake_metadata_append_full_duplicate(void)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   // Step 1: Create a `MongoClient` instance with ...
   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_destroy(_handshake_metadata_append_setup(server, client, &(driver_info_options){0}));

   // Step 2: Append the following `DriverInfoOptions` to the `MongoClient` metadata:
   ASSERT(mongoc_client_append_metadata(client, "library", "1.2", "Library Platform"));

   // Step 3: Send a `ping` command to the server and verify that the command succeeds.
   // Step 4: Wait 5ms for the connection to become idle (use hangups instead).
   bson_destroy(_handshake_metadata_append_ping_capture(server, client));

   // Step 5: Append the following `DriverInfoOptions` to the `MongoClient` metadata:
   ASSERT(mongoc_client_append_metadata(client, "framework", "2.0", "Framework Platform"));

   // Step 6: Send a `ping` command to the server and verify that the command succeeds.
   // Step 7: Save intercepted `client` document as `clientMetadata`.
   // Step 8: Wait 5ms for the connection to become idle (use hangups instead).
   bson_t *const client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 9: Append the following `DriverInfoOptions` to the `MongoClient` metadata:
   ASSERT(mongoc_client_append_metadata(client, "library", "1.2", "Library Platform"));

   // Step 10: Send a `ping` command to the server and verify that the command succeeds.
   // Step 11: Save intercepted `client` document as `updatedClientMetadata`.
   bson_t *const updated_client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 12: Assert that `clientMetadata` is identical to `updatedClientMetadata`.
   ASSERT_EQUAL_BSON(client_metadata, updated_client_metadata);

   bson_destroy(updated_client_metadata);
   bson_destroy(client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 5: Metadata is not appended if identical to
// initial metadata
static void
test_handshake_metadata_append_identical(void)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   // Step 1: Create a `MongoClient` instance with ...
   // Step 2: Send a `ping` command to the server and verify that the command succeeds.
   // Step 3: Save intercepted `client` document as `clientMetadata`.
   // Step 4: Wait 5ms for the connection to become idle (use hangups instead).
   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_t *const client_metadata = _handshake_metadata_append_setup(server,
                                                                    client,
                                                                    &(driver_info_options){
                                                                       .name = "library",
                                                                       .version = "1.2",
                                                                       .platform = "Library Platform",
                                                                    });

   // Step 5: Append the following `DriverInfoOptions` to the `MongoClient` metadata:
   ASSERT(mongoc_client_append_metadata(client, "library", "1.2", "Library Platform"));

   // Step 6: Send a `ping` command to the server and verify that the command succeeds.
   // Step 7: Save intercepted `client` document as `updatedClientMetadata`.
   bson_t *const updated_client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 8: Assert that clientMetadata is identical to updatedClientMetadata.
   ASSERT_EQUAL_BSON(client_metadata, updated_client_metadata);

   bson_destroy(client_metadata);
   bson_destroy(updated_client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 6: Metadata is not appended if identical to
// initial metadata (separated by non-identical metadata)
static void
test_handshake_metadata_append_separated_identical(void)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   // Step 1: Create a `MongoClient` instance with ...
   // Step 2: Send a `ping` command to the server and verify that the command succeeds.
   // Step 3: Wait 5ms for the connection to become idle (use hangups instead).
   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_destroy(_handshake_metadata_append_setup(server,
                                                 client,
                                                 &(driver_info_options){
                                                    .name = "library",
                                                    .version = "1.2",
                                                    .platform = "Library Platform",
                                                 }));

   // Step 4: Append the following `DriverInfoOptions` to the `MongoClient` metadata:
   ASSERT(mongoc_client_append_metadata(client, "framework", "1.2", "Library Platform"));

   // Step 5: Send a `ping` command to the server and verify that the command succeeds.
   // Step 6: Save intercepted `client` document as `updatedClientMetadata`.
   // Step 7: Wait 5ms for the connection to become idle (use hangups instead).
   bson_t *const client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 8: Append the following `DriverInfoOptions` to the `MongoClient` metadata:
   ASSERT(mongoc_client_append_metadata(client, "library", "1.2", "Library Platform"));

   // Step 9: Send a `ping` command to the server and verify that the command succeeds.
   // Step 10: Save intercepted `client` document as `updatedClientMetadata`.
   bson_t *const updated_client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 11: Assert that clientMetadata is identical to updatedClientMetadata.
   ASSERT_EQUAL_BSON(client_metadata, updated_client_metadata);

   bson_destroy(client_metadata);
   bson_destroy(updated_client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 7: Empty strings are considered unset when
// appending duplicate metadata
static void
test_handshake_metadata_append_empty_duplicate_impl(const driver_info_options *appended,
                                                    const driver_info_options *duplicate)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   // Step 1: Create a `MongoClient` instance with ...
   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_destroy(_handshake_metadata_append_setup(server, client, &(driver_info_options){0}));

   // Step 2: Append the `DriverInfoOptions` from the selected test case from the appended metadata section.
   ASSERT(mongoc_client_append_metadata(client, appended->name, appended->version, appended->platform));

   // Step 3: Send a `ping` command to the server and verify that the command succeeds.
   // Step 4: Save intercepted `client` document as `clientMetadata`.
   // Step 5: Wait 5ms for the connection to become idle (use hangups instead).
   bson_t *const client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 6: Append the `DriverInfoOptions` from the selected test case from the duplicate metadata section.
   ASSERT(mongoc_client_append_metadata(client, duplicate->name, duplicate->version, duplicate->platform));

   // Step 7: Send a `ping` command to the server and verify that the command succeeds.
   // Step 8: Store the response as `updatedClientMetadata`.
   bson_t *const updated_client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 9: Assert that `clientMetadata` is identical to `updatedClientMetadata`.
   ASSERT_EQUAL_BSON(client_metadata, updated_client_metadata);

   bson_destroy(client_metadata);
   bson_destroy(updated_client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

static void
test_handshake_metadata_append_empty_duplicate_case_2(void)
{
   test_handshake_metadata_append_empty_duplicate_impl(
      &(driver_info_options){.name = "library", .version = NULL, .platform = "Library Platform"},
      &(driver_info_options){.name = "library", .version = "", .platform = "Library Platform"});
}

static void
test_handshake_metadata_append_empty_duplicate_case_3(void)
{
   test_handshake_metadata_append_empty_duplicate_impl(
      &(driver_info_options){.name = "library", .version = "1.2", .platform = NULL},
      &(driver_info_options){.name = "library", .version = "1.2", .platform = ""});
}

// MongoDB Handshake Tests: Client Metadata Update Prose Tests: Test 8: Empty strings are considered unset when
// appending metadata identical to initial metadata
static void
test_handshake_metadata_append_empty_identical_impl(const driver_info_options *initial,
                                                    const driver_info_options *appended)
{
   _reset_handshake();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   // Step 1: Create a `MongoClient` instance with ...
   // Step 2: Send a `ping` command to the server and verify that the command succeeds.
   // Step 3: Save intercepted `client` document as `initialClientMetadata`.
   // Step 4: Wait 5ms for the connection to become idle (use hangups instead).
   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   bson_t *const client_metadata = _handshake_metadata_append_setup(server, client, initial);

   // Step 5: Append the `DriverInfoOptions` from the selected test case from the appended metadata section.
   ASSERT(mongoc_client_append_metadata(client, appended->name, appended->version, appended->platform));

   // Step 6: Send a `ping` command to the server and verify that the command succeeds.
   // Step 7: Store the response as `updatedClientMetadata`.
   bson_t *const updated_client_metadata = _handshake_metadata_append_ping_capture(server, client);

   // Step 8: Assert that `initialClientMetadata` is identical to `updatedClientMetadata`.
   ASSERT_EQUAL_BSON(client_metadata, updated_client_metadata);

   bson_destroy(client_metadata);
   bson_destroy(updated_client_metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

static void
test_handshake_metadata_append_empty_identical_case_2(void)
{
   test_handshake_metadata_append_empty_identical_impl(
      &(driver_info_options){.name = "library", .version = NULL, .platform = "Library Platform"},
      &(driver_info_options){.name = "library", .version = "", .platform = "Library Platform"});
}

static void
test_handshake_metadata_append_empty_identical_case_3(void)
{
   test_handshake_metadata_append_empty_identical_impl(
      &(driver_info_options){.name = "library", .version = "1.2", .platform = NULL},
      &(driver_info_options){.name = "library", .version = "1.2", .platform = ""});
}

static void
test_handshake_metadata_mongoc_platform_reappends_impl(bool initialize_with)
{
   _override_host_platform_os();

   {
      mongoc_handshake_t *const md = _mongoc_handshake_get_unfrozen();

      // Start with an empty platform string.
      bson_free(md->platform);
      md->platform = bson_strdup("");
   }

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   mongoc_client_t *const client = _test_metadata_append_setup_client(server);

   if (initialize_with) {
      ASSERT(!_mongoc_handshake_is_frozen());
      ASSERT(mongoc_handshake_data_append("library", "1.2", "Library Platform"));
   } else {
      bson_t *const metadata = _handshake_metadata_append_ping_capture(server, client);

      // "<mongoc platform>" is always appended as the last element whenever able (no truncation).
      ASSERT_MATCH(metadata,
                   "{"
                   "  'driver': {"
                   "    'name': 'test_e',"
                   "    'version': '1.25.0'"
                   "  },"
                   "  'platform': 'CC=GCC CFLAGS=\\\"-fPIE\\\"'"
                   "}");

      bson_destroy(metadata);

      ASSERT(mongoc_client_append_metadata(client, "library", "1.2", "Library Platform"));
   }

   {
      bson_t *const metadata = _handshake_metadata_append_ping_capture(server, client);

      // User-provided platform metadata must always come before "<mongoc platform>".
      ASSERT_MATCH(metadata,
                   "{"
                   "  'driver': {"
                   "    'name': 'test_e / library',"
                   "    'version': '1.25.0 / 1.2'"
                   "  },"
                   "  'platform': 'Library Platform / CC=GCC CFLAGS=\\\"-fPIE\\\"'"
                   "}");

      bson_destroy(metadata);
   }

   ASSERT(mongoc_client_append_metadata(client, "framework", "2.0", "Framework Platform"));

   {
      bson_t *const metadata = _handshake_metadata_append_ping_capture(server, client);

      // "<mongoc platform>" must be reappended as the last element following an append operation.
      ASSERT_MATCH(metadata,
                   "{"
                   "  'driver': {"
                   "    'name': 'test_e / library / framework',"
                   "    'version': '1.25.0 / 1.2 / 2.0'"
                   "  },"
                   "  'platform': 'Library Platform / Framework Platform / CC=GCC CFLAGS=\\\"-fPIE\\\"'"
                   "}");

      bson_destroy(metadata);
   }

   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

static void
test_handshake_metadata_mongoc_platform_reappends(void)
{
   test_handshake_metadata_mongoc_platform_reappends_impl(true);
   test_handshake_metadata_mongoc_platform_reappends_impl(false);
}

static void
test_handshake_metadata_mongoc_platform_truncation(void)
{
   // Ensure determinism of the total handshake command length.
   _override_host_platform_os();

   {
      mongoc_handshake_t *const md = _mongoc_handshake_get_unfrozen();

      // Ensure determinism of the total handshake command length.
      md->docker = false;
      md->kubernetes = false;
   }

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   // This value is steadily increased until truncation occurs. The subtracted value must be large enough (the
   // `big_platform` string must start small enough) to allow for state 0 (no truncation) to occur at least once.
   size_t big_platform_len = (HANDSHAKE_MAX_SIZE - 147);
   int state = 0;

   int count_state_0 = 0; // No truncation.
   int count_state_1 = 0; // Only `flags` is omitted.
   int count_state_2 = 0; // Both `compiler_info` and `flags` are omitted.

   // Repeatedly increase the length of the appended platform value to trigger incremental truncation.
   while (state < 3) {
      mongoc_client_t *const client = _test_metadata_append_setup_client(server);

      char *const big_platform = bson_malloc(big_platform_len);
      memset(big_platform, 'a', big_platform_len - 1u);
      big_platform[big_platform_len - 1u] = '\0';

      ASSERT(mongoc_client_append_metadata(client, "library", "1.2", big_platform));

      {
         bson_t *const metadata = _handshake_metadata_append_ping_capture(server, client);
         const char *const platform = bson_lookup_utf8(metadata, "platform");

         // Initially, entire platform string should be present.
         if (state == 0) {
            char *const expected = bson_strdup_printf("posix=1234 / %s / CC=GCC CFLAGS=\"-fPIE\"", big_platform);

            if (strcmp(platform, expected) == 0) {
               ++count_state_0;
            } else {
               ++state; // Transition to state 1.
            }

            bson_free(expected);
         }

         // `flags` must be the first value to be omitted due to truncation.
         if (state == 1) {
            char *const expected = bson_strdup_printf("posix=1234 / %s / CC=GCC", big_platform);

            if (strcmp(platform, expected) == 0) {
               ++count_state_1;
            } else {
               ++state; // Transition to state 2.
            }

            bson_free(expected);
         }

         // `compiler_info` must be the next value omitted due to truncation.
         if (state == 2) {
            char *const expected = bson_strdup_printf("posix=1234 / %s", big_platform);

            if (strcmp(platform, expected) == 0) {
               ++count_state_2; // `big_platform` is not yet truncated.
            } else {
               // The truncated platform string is still a substring.
               ASSERT(strstr(expected, platform) == expected);

               // Truncation must have removed exactly one character due to `big_platform_len` being incremented by 1.
               ASSERT_CMPSIZE_T(strlen(platform), ==, strlen(expected) - 1u);

               ++state; // End iteration.
            }

            bson_free(expected);
         }

         bson_destroy(metadata);
      }

      bson_free(big_platform);
      mongoc_client_destroy(client);

      // Must be smaller than length of `compiler_info` or `flags` to ensure states 1 and 2 each occur at least once.
      big_platform_len += 1u;
   }

   // Unlikely: one or more of the string comparisons above failed to match the expected platform string for a reason
   // other than the expected truncation behavior. When this happens, one or more of these counters will not have been
   // incremented as expected.
   ASSERT_CMPINT(count_state_0, >, 0);
   ASSERT_CMPINT(count_state_1, >, 0);
   ASSERT_CMPINT(count_state_2, >, 0);

   mock_server_destroy(server);

   _reset_handshake();
}

static void
test_handshake_metadata_append_strip_delimiters(void)
{
   _override_host_platform_os();

   mock_server_t *const server = mock_server_new();
   mock_server_run(server);

   mongoc_client_t *const client = _test_metadata_append_setup_client(server);
   ASSERT(client);

   // For backward compatibility, permit string arguments to old metadata append API to contain trailing delimiters.
   ASSERT(mongoc_handshake_data_append("driver_name / ", "driver_version / ", "platform / "));

   bson_t *const metadata = _handshake_metadata_append_ping_capture(server, client);

   // Resulting handshake command must not contain trailing delimiters (e.g. "driver_name / ") or double-delimiters
   // (e.g. "posix=1234 / / CC=GCC CFLAGS=\"-fPIE\"").
   ASSERT_MATCH(metadata,
                "{"
                "  'driver': {"
                "    'name': 'test_e / driver_name',"
                "    'version': '1.25.0 / driver_version'"
                "  },"
                "  'platform': 'posix=1234 / platform / CC=GCC CFLAGS=\\\"-fPIE\\\"'"
                "}");

   bson_destroy(metadata);
   mongoc_client_destroy(client);
   mock_server_destroy(server);

   _reset_handshake();
}

void
test_handshake_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/MongoDB/handshake/appname_in_uri", test_mongoc_handshake_appname_in_uri);
   TestSuite_Add(suite, "/MongoDB/handshake/appname_frozen_single", test_mongoc_handshake_appname_frozen_single);
   TestSuite_Add(suite, "/MongoDB/handshake/appname_frozen_pooled", test_mongoc_handshake_appname_frozen_pooled);

   TestSuite_AddMockServerTest(suite, "/MongoDB/handshake/success", test_mongoc_handshake_data_append_success);
   TestSuite_AddMockServerTest(suite, "/MongoDB/handshake/null_args", test_mongoc_handshake_data_append_null_args);
   TestSuite_Add(suite, "/MongoDB/handshake/big_platform", test_mongoc_handshake_big_platform);
   TestSuite_Add(suite, "/MongoDB/handshake/oversized_platform", test_mongoc_handshake_oversized_platform);
   TestSuite_Add(suite, "/MongoDB/handshake/failure", test_mongoc_handshake_data_append_after_cmd);
   TestSuite_AddMockServerTest(suite, "/MongoDB/handshake/too_big", test_mongoc_handshake_too_big);
   TestSuite_Add(suite, "/MongoDB/handshake/oversized_flags", test_mongoc_oversized_flags);
   TestSuite_AddMockServerTest(suite, "/MongoDB/handshake/cannot_send", test_mongoc_handshake_cannot_send);
   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/no_validation_for_sasl_supported_mech",
                               test_mongoc_handshake_no_validation_for_sasl_supported_mech);
   TestSuite_Add(suite, "/MongoDB/handshake/platform_config", test_handshake_platform_config);
   TestSuite_Add(suite, "/MongoDB/handshake/race_condition", test_mongoc_handshake_race_condition);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/valid_aws",
                     test_valid_aws,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/valid_azure",
                     test_valid_azure,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/valid_gcp",
                     test_valid_gcp,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/valid_aws_lambda",
                     test_valid_aws_lambda,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/valid_aws_and_vercel",
                     test_valid_aws_and_vercel,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/valid_vercel",
                     test_valid_vercel,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/multiple",
                     test_multiple_faas,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/truncate_region",
                     test_truncate_region,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/wrong_types",
                     test_wrong_types,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/aws_not_lambda",
                     test_aws_not_lambda,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_AddFull(suite,
                     "/MongoDB/handshake/faas/aws_and_container",
                     test_aws_and_container,
                     NULL /* dtor */,
                     NULL /* ctx */,
                     test_framework_skip_if_no_setenv);
   TestSuite_Add(suite, "/MongoDB/handshake/includes_c++", test_mongoc_handshake_cpp);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/includes_backpressure_flag", test_mongoc_handshake_includes_backpressure_flag);

   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/single/case_1", test_handshake_metadata_append_single_case_1);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/single/case_2", test_handshake_metadata_append_single_case_2);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/single/case_3", test_handshake_metadata_append_single_case_3);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/single/case_4", test_handshake_metadata_append_single_case_4);

   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/multiple/case_1", test_handshake_metadata_append_multiple_case_1);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/multiple/case_2", test_handshake_metadata_append_multiple_case_2);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/multiple/case_3", test_handshake_metadata_append_multiple_case_3);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/multiple/case_4", test_handshake_metadata_append_multiple_case_4);

   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/duplicate/case_1", test_handshake_metadata_append_duplicate_case_1);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/duplicate/case_2", test_handshake_metadata_append_duplicate_case_2);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/duplicate/case_3", test_handshake_metadata_append_duplicate_case_3);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/duplicate/case_4", test_handshake_metadata_append_duplicate_case_4);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/duplicate/case_5", test_handshake_metadata_append_duplicate_case_5);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/duplicate/case_6", test_handshake_metadata_append_duplicate_case_6);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/duplicate/case_7", test_handshake_metadata_append_duplicate_case_7);

   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/full_duplicate", test_handshake_metadata_append_full_duplicate);

   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/identical", test_handshake_metadata_append_identical);

   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/metadata_append/separated_identical",
                               test_handshake_metadata_append_separated_identical);

   // Do not implement Case 1, which requires an empty/null "name" field.
   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/metadata_append/empty_duplicate/case_2",
                               test_handshake_metadata_append_empty_duplicate_case_2);
   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/metadata_append/empty_duplicate/case_3",
                               test_handshake_metadata_append_empty_duplicate_case_3);

   // Do not implement Case 1, which requires an empty/null "name" field.
   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/metadata_append/empty_identical/case_2",
                               test_handshake_metadata_append_empty_identical_case_2);
   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/metadata_append/empty_identical/case_3",
                               test_handshake_metadata_append_empty_identical_case_3);

   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/metadata_append/mongoc_platform_reappends",
                               test_handshake_metadata_mongoc_platform_reappends);
   TestSuite_AddMockServerTest(suite,
                               "/MongoDB/handshake/metadata_append/mongoc_platform_truncation",
                               test_handshake_metadata_mongoc_platform_truncation);
   TestSuite_AddMockServerTest(
      suite, "/MongoDB/handshake/metadata_append/strip_delimiters", test_handshake_metadata_append_strip_delimiters);
}
