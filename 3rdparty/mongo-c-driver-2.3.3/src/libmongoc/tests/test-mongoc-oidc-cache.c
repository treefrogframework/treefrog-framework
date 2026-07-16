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

#include <mongoc/mongoc-oidc-cache-private.h>

#include <TestSuite.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

typedef struct {
   int call_count;
   bool returns_null;
   const char *expect_username;
} callback_ctx_t;

#define PLACEHOLDER_TOKEN "PLACEHOLDER_TOKEN"

static mongoc_oidc_credential_t *
oidc_callback_fn(mongoc_oidc_callback_params_t *params)
{
   callback_ctx_t *ctx = mongoc_oidc_callback_params_get_user_data(params);
   ASSERT(ctx);
   ctx->call_count += 1;
   if (ctx->returns_null) {
      return NULL;
   }
   ASSERT_CMPSTR(mongoc_oidc_callback_params_get_username(params), ctx->expect_username);
   return mongoc_oidc_credential_new(PLACEHOLDER_TOKEN);
}

static void
test_oidc_cache_works(void)
{
   bool found_in_cache = false;
   bson_error_t error;

   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();
   callback_ctx_t ctx = {0};

   // Expect error if no callback set:
   {
      ASSERT(!mongoc_oidc_cache_get_token(cache, &found_in_cache, &error));
      ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_CLIENT, MONGOC_ERROR_CLIENT_AUTHENTICATE, "no callback set");
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
   }

   // Set a callback:
   {
      mongoc_oidc_callback_t *cb = mongoc_oidc_callback_new(oidc_callback_fn);
      mongoc_oidc_callback_set_user_data(cb, &ctx);
      mongoc_oidc_cache_set_user_callback(cache, NULL, cb);
      mongoc_oidc_callback_destroy(cb);
   }

   // Expect callback is called to fetch token:
   {
      char *token = mongoc_oidc_cache_get_token(cache, &found_in_cache, &error);
      ASSERT_OR_PRINT(token, error);
      ASSERT_CMPSTR(token, PLACEHOLDER_TOKEN);
      ASSERT_CMPINT(ctx.call_count, ==, 1);
      ASSERT(!found_in_cache);
      bson_free(token);
   }

   // Expect token is cached:
   {
      char *token = mongoc_oidc_cache_get_cached_token(cache);
      ASSERT(token);
      bson_free(token);
   }

   // Expect callback is not called if token is cached:
   {
      char *token = mongoc_oidc_cache_get_token(cache, &found_in_cache, &error);
      ASSERT_OR_PRINT(token, error);
      ASSERT_CMPSTR(token, PLACEHOLDER_TOKEN);
      ASSERT_CMPINT(ctx.call_count, ==, 1);
      ASSERT(found_in_cache);
      bson_free(token);
   }

   // Invalidating a different token has no effect:
   {
      mongoc_oidc_cache_invalidate_token(cache, "different-token");
      char *token = mongoc_oidc_cache_get_cached_token(cache);
      ASSERT(token);
      bson_free(token);
   }

   // Invalidating same token clears cache:
   {
      char *token = mongoc_oidc_cache_get_cached_token(cache);
      ASSERT(token);
      mongoc_oidc_cache_invalidate_token(cache, token);
      bson_free(token);
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
   }

   mongoc_oidc_cache_destroy(cache);
}

static void
test_oidc_cache_waits_between_calls(void)
{
   bool found_in_cache = false;
   bson_error_t error;
   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();
   callback_ctx_t ctx = {0};

   // Set a callback:
   {
      mongoc_oidc_callback_t *cb = mongoc_oidc_callback_new(oidc_callback_fn);
      mongoc_oidc_callback_set_user_data(cb, &ctx);
      mongoc_oidc_cache_set_user_callback(cache, NULL, cb);
      mongoc_oidc_callback_destroy(cb);
   }

   mlib_time_point const start = mlib_now();

   // Expect callback is called to fetch token:
   {
      char *token = mongoc_oidc_cache_get_token(cache, &found_in_cache, &error);
      ASSERT_OR_PRINT(token, error);
      ASSERT_CMPSTR(token, PLACEHOLDER_TOKEN);
      ASSERT_CMPINT(ctx.call_count, ==, 1);
      ASSERT(!found_in_cache);
      bson_free(token);
   }

   // Invalidate token to clear cache:
   {
      char *token = mongoc_oidc_cache_get_cached_token(cache);
      ASSERT(token);
      mongoc_oidc_cache_invalidate_token(cache, token);
      bson_free(token);
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
   }

   const int64_t expected_delay = 90; // Use shorter time. Windows appears to sleep slightly less.
   // Expect duration less than delay:
   {
      mlib_duration diff = mlib_time_difference(mlib_now(), start);
      ASSERT_CMPINT64(mlib_milliseconds_count(diff), <, expected_delay);
   }

   // Fetch token again:
   {
      char *token = mongoc_oidc_cache_get_token(cache, &found_in_cache, &error);
      ASSERT_OR_PRINT(token, error);
      ASSERT_CMPSTR(token, PLACEHOLDER_TOKEN);
      ASSERT_CMPINT(ctx.call_count, ==, 2);
      ASSERT(!found_in_cache);
      bson_free(token);
   }

   // Expect delay:
   {
      mlib_duration diff = mlib_time_difference(mlib_now(), start);
      ASSERT_CMPINT64(mlib_milliseconds_count(diff), >=, expected_delay);
   }

   mongoc_oidc_cache_destroy(cache);
}

static void
test_oidc_cache_set_callback(void)
{
   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();

   ASSERT(!mongoc_oidc_cache_has_user_callback(cache));

   // Can set a callback:
   {
      mongoc_oidc_callback_t *cb = mongoc_oidc_callback_new(oidc_callback_fn);
      mongoc_oidc_cache_set_user_callback(cache, NULL, cb);
      ASSERT(mongoc_oidc_cache_has_user_callback(cache));
      mongoc_oidc_callback_destroy(cb);
   }

   // Can clear a callback:
   {
      mongoc_oidc_cache_set_user_callback(cache, NULL, NULL);
      ASSERT(!mongoc_oidc_cache_has_user_callback(cache));
   }

   mongoc_oidc_cache_destroy(cache);
}

static void
test_oidc_cache_passes_username(void)
{
   bool found_in_cache = false;
   bson_error_t error;
   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();
   callback_ctx_t ctx = {.expect_username = "test_user"};

   // Set a callback:
   {
      mongoc_oidc_callback_t *cb = mongoc_oidc_callback_new(oidc_callback_fn);
      mongoc_oidc_callback_set_user_data(cb, &ctx);
      mongoc_oidc_cache_set_user_callback(cache, "test_user", cb);
      mongoc_oidc_callback_destroy(cb);
   }

   // Expect callback is called to fetch token:
   {
      char *token = mongoc_oidc_cache_get_token(cache, &found_in_cache, &error);
      ASSERT_OR_PRINT(token, error);
      ASSERT_CMPSTR(token, PLACEHOLDER_TOKEN);
      ASSERT_CMPINT(ctx.call_count, ==, 1);
      ASSERT(!found_in_cache);
      bson_free(token);
   }

   mongoc_oidc_cache_destroy(cache);
}

typedef struct {
   int call_count;
   int64_t last_arg;
} sleep_ctx_t;

static void
sleep_callback_fn(int64_t usec, void *user_data)
{
   ASSERT(user_data);
   sleep_ctx_t *ctx = (sleep_ctx_t *)user_data;
   ctx->call_count += 1;
   ctx->last_arg = usec;
}

static void
test_oidc_cache_set_sleep(void)
{
   callback_ctx_t ctx = {0};
   sleep_ctx_t sleep_ctx = {0};
   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();

   // Set a callback to test:
   {
      mongoc_oidc_callback_t *cb = mongoc_oidc_callback_new(oidc_callback_fn);
      mongoc_oidc_callback_set_user_data(cb, &ctx);
      mongoc_oidc_cache_set_user_callback(cache, NULL, cb);
      mongoc_oidc_callback_destroy(cb);
   }

   // Can use a custom sleep function:
   {
      bool found_in_cache = false;
      bson_error_t error;
      char *token;

      // Set a custom sleep function:
      mongoc_oidc_cache_set_usleep_fn(cache, sleep_callback_fn, &sleep_ctx);

      // First call to get_token does not sleep:
      token = mongoc_oidc_cache_get_token(cache, &found_in_cache, &error);
      ASSERT_OR_PRINT(token, error);
      ASSERT_CMPSTR(token, PLACEHOLDER_TOKEN);
      ASSERT_CMPINT(ctx.call_count, ==, 1);
      ASSERT_CMPINT(sleep_ctx.call_count, ==, 0);
      ASSERT(!found_in_cache);

      // Invalidate cache to trigger another call:
      mongoc_oidc_cache_invalidate_token(cache, token);
      bson_free(token);

      // Second call to get_token sleeps to ensure at least 100ms between calls:
      token = mongoc_oidc_cache_get_token(cache, &found_in_cache, &error);
      ASSERT_OR_PRINT(token, error);
      ASSERT_CMPSTR(token, PLACEHOLDER_TOKEN);
      ASSERT_CMPINT(ctx.call_count, ==, 2);
      ASSERT_CMPINT(sleep_ctx.call_count, ==, 1);
      ASSERT_CMPINT64(sleep_ctx.last_arg, >, 0);
      ASSERT_CMPINT64(sleep_ctx.last_arg, <=, 100 * 1000); // at most 100ms
      ASSERT(!found_in_cache);
      bson_free(token);
   }

   mongoc_oidc_cache_destroy(cache);
}

static void
test_oidc_cache_set_cached_token(void)
{
   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();

   ASSERT(!mongoc_oidc_cache_get_cached_token(cache));

   // Can set a cached token:
   {
      mongoc_oidc_cache_set_cached_token(cache, PLACEHOLDER_TOKEN);
      char *got = mongoc_oidc_cache_get_cached_token(cache);
      ASSERT_CMPSTR(got, PLACEHOLDER_TOKEN);
      bson_free(got);
   }

   // Can clear cached token:
   {
      mongoc_oidc_cache_set_cached_token(cache, NULL);
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
   }

   mongoc_oidc_cache_destroy(cache);
}

static void
test_oidc_cache_propagates_error(void)
{
   // Test a callback returning NULL.
   bool found_in_cache = false;
   bson_error_t error;

   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();
   callback_ctx_t ctx = {.returns_null = true};

   // Set a callback:
   {
      mongoc_oidc_callback_t *cb = mongoc_oidc_callback_new(oidc_callback_fn);
      mongoc_oidc_callback_set_user_data(cb, &ctx);
      mongoc_oidc_cache_set_user_callback(cache, NULL, cb);
      mongoc_oidc_callback_destroy(cb);
   }

   // Expect error:
   {
      ASSERT(!mongoc_oidc_cache_get_token(cache, &found_in_cache, &error));
      ASSERT_ERROR_CONTAINS(error, MONGOC_ERROR_CLIENT, MONGOC_ERROR_CLIENT_AUTHENTICATE, "callback failed");
      ASSERT(!found_in_cache);
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
   }

   mongoc_oidc_cache_destroy(cache);
}

static void
test_oidc_cache_invalidate(void)
{
   mongoc_oidc_cache_t *cache = mongoc_oidc_cache_new();

   // Can invalidate when nothing cached:
   {
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
      mongoc_oidc_cache_invalidate_token(cache, "foobar");
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
   }

   // Cache a token:
   {
      mongoc_oidc_cache_set_cached_token(cache, "foo");
      char *token = mongoc_oidc_cache_get_cached_token(cache);
      ASSERT_CMPSTR(token, "foo");
      bson_free(token);
   }

   // Invalidating a different token has no effect:
   {
      mongoc_oidc_cache_invalidate_token(cache, "bar");
      char *token = mongoc_oidc_cache_get_cached_token(cache);
      ASSERT_CMPSTR(token, "foo");
      bson_free(token);
   }

   // Invalidating same token clears cache:
   {
      mongoc_oidc_cache_invalidate_token(cache, "foo");
      ASSERT(!mongoc_oidc_cache_get_cached_token(cache));
   }

   mongoc_oidc_cache_destroy(cache);
}


// test_oidc_connection_cache tests the connection token cache.
static void
test_oidc_connection_cache(void)
{
   mongoc_oidc_connection_cache_t *cache = mongoc_oidc_connection_cache_new();

   ASSERT(!mongoc_oidc_connection_cache_get(cache));

   // Can set a cached token:
   {
      mongoc_oidc_connection_cache_set(cache, PLACEHOLDER_TOKEN);
      char *got = mongoc_oidc_connection_cache_get(cache);
      ASSERT_CMPSTR(got, PLACEHOLDER_TOKEN);
      bson_free(got);
   }

   // Can clear cached token:
   {
      mongoc_oidc_connection_cache_set(cache, NULL);
      ASSERT(!mongoc_oidc_connection_cache_get(cache));
   }

   mongoc_oidc_connection_cache_destroy(cache);
}

void
test_mongoc_oidc_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/oidc/cache/works", test_oidc_cache_works);
   TestSuite_Add(suite, "/oidc/cache/set_callback", test_oidc_cache_set_callback);
   TestSuite_Add(suite, "/oidc/cache/set_sleep", test_oidc_cache_set_sleep);
   TestSuite_Add(suite, "/oidc/cache/set_cached_token", test_oidc_cache_set_cached_token);
   TestSuite_Add(suite, "/oidc/cache/propagates_error", test_oidc_cache_propagates_error);
   TestSuite_Add(suite, "/oidc/cache/invalidate", test_oidc_cache_invalidate);
   TestSuite_Add(suite, "/oidc/cache/waits_between_calls", test_oidc_cache_waits_between_calls);
   TestSuite_Add(suite, "/oidc/cache/passes_username", test_oidc_cache_passes_username);
   TestSuite_Add(suite, "/oidc/connection_cache", test_oidc_connection_cache);
}
