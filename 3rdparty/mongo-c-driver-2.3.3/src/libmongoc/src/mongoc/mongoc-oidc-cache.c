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

#include <common-bson-dsl-private.h>
#include <common-macros-private.h> // MC_DISABLE_CAST_QUAL_WARNING_BEGIN
#include <common-thread-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-oidc-cache-private.h>
#include <mongoc/mongoc-oidc-callback-private.h>
#include <mongoc/mongoc-oidc-env-private.h>

#include <mlib/duration.h>
#include <mlib/time_point.h>

#define SET_ERROR(...) _mongoc_set_error(error, MONGOC_ERROR_CLIENT, MONGOC_ERROR_CLIENT_AUTHENTICATE, __VA_ARGS__)

struct mongoc_oidc_cache_t {
   // username is copied from the URI.
   char *username;

   // user_callback is owned. NULL if unset. Not guarded by lock. Set before requesting tokens.
   // If both user_callback and env_callback are set, an error occurs when requesting a token.
   mongoc_oidc_callback_t *user_callback;

   // env_callback is owned. NULL if unset. Not guarded by lock. Set before requesting tokens.
   // If both user_callback and env_callback are set, an error occurs when requesting a token.
   mongoc_oidc_env_callback_t *env_callback;

   // usleep_fn is used to sleep between calls to the callback. Not guarded by lock. Set before requesting tokens.
   mongoc_usleep_func_t usleep_fn;
   void *usleep_data;

   // lock is used to prevent concurrent calls to callback. Guards access to token, last_called, and ever_called.
   bson_shared_mutex_t lock;

   // token is a cached OIDC access token.
   char *token;

   // last_call tracks the time just after the last call to the callback.
   mlib_time_point last_called;

   // ever_called is set to true after the first call to the callback.
   bool ever_called;
};

mongoc_oidc_cache_t *
mongoc_oidc_cache_new(void)
{
   mongoc_oidc_cache_t *oidc = bson_malloc0(sizeof(mongoc_oidc_cache_t));
   oidc->usleep_fn = mongoc_usleep_default_impl;
   bson_shared_mutex_init(&oidc->lock);
   return oidc;
}

void
mongoc_oidc_cache_apply_env_from_uri(mongoc_oidc_cache_t *cache, const mongoc_uri_t *uri)
{
   BSON_ASSERT_PARAM(cache);
   BSON_ASSERT_PARAM(uri);

   const char *mechanism = mongoc_uri_get_auth_mechanism(uri);
   if (!mechanism || 0 != strcmp(mechanism, "MONGODB-OIDC")) {
      // Not using OIDC.
      return;
   }

   const char *username = mongoc_uri_get_username(uri);
   bson_t mechanism_properties;
   if (!mongoc_uri_get_mechanism_properties(uri, &mechanism_properties)) {
      // Not configured with OIDC environment.
      return;
   }

   bson_iter_t iter;
   const char *environment = NULL;
   if (bson_iter_init_find(&iter, &mechanism_properties, "ENVIRONMENT") && BSON_ITER_HOLDS_UTF8(&iter)) {
      environment = bson_iter_utf8(&iter, NULL);
   }

   const mongoc_oidc_env_t *env = mongoc_oidc_env_find(environment);
   BSON_ASSERT(env);                                                    // Checked in mongoc_uri_finalize_auth.
   BSON_ASSERT(!(username && !mongoc_oidc_env_supports_username(env))); // Checked in mongoc_uri_finalize_auth.

   const char *token_resource = NULL;
   if (bson_iter_init_find(&iter, &mechanism_properties, "TOKEN_RESOURCE") && BSON_ITER_HOLDS_UTF8(&iter)) {
      BSON_ASSERT(BSON_ITER_HOLDS_UTF8(&iter));
      token_resource = bson_iter_utf8(&iter, NULL); // Checked in mongoc_uri_finalize_auth.
   }

   BSON_ASSERT((token_resource != NULL) ==
               mongoc_oidc_env_requires_token_resource(env)); // Checked in mongoc_uri_finalize_auth.

   BSON_ASSERT(!cache->env_callback); // Not set yet.
   cache->env_callback = mongoc_oidc_env_callback_new(env, token_resource);

   BSON_ASSERT(!cache->username); // Not set yet.
   cache->username = bson_strdup(username);
}

void
mongoc_oidc_cache_set_user_callback(mongoc_oidc_cache_t *cache, const char *username, const mongoc_oidc_callback_t *cb)
{
   BSON_ASSERT_PARAM(cache);
   BSON_OPTIONAL_PARAM(username);
   BSON_OPTIONAL_PARAM(cb);

   BSON_ASSERT(!cache->ever_called);

   mongoc_oidc_callback_destroy(cache->user_callback);
   cache->user_callback = cb ? mongoc_oidc_callback_copy(cb) : NULL;
   bson_free(cache->username);
   cache->username = bson_strdup(username);
}

bool
mongoc_oidc_cache_has_user_callback(const mongoc_oidc_cache_t *cache)
{
   BSON_ASSERT_PARAM(cache);

   return cache->user_callback;
}

void
mongoc_oidc_cache_set_usleep_fn(mongoc_oidc_cache_t *cache, mongoc_usleep_func_t usleep_fn, void *usleep_data)
{
   BSON_ASSERT_PARAM(cache);
   BSON_OPTIONAL_PARAM(usleep_fn);
   BSON_OPTIONAL_PARAM(usleep_data);

   BSON_ASSERT(!cache->ever_called);

   cache->usleep_fn = usleep_fn ? usleep_fn : mongoc_usleep_default_impl;
   cache->usleep_data = usleep_data;
}

void
mongoc_oidc_cache_destroy(mongoc_oidc_cache_t *cache)
{
   if (!cache) {
      return;
   }
   bson_free(cache->token);
   bson_shared_mutex_destroy(&cache->lock);
   mongoc_oidc_callback_destroy(cache->user_callback);
   mongoc_oidc_env_callback_destroy(cache->env_callback);
   bson_free(cache->username);
   bson_free(cache);
}

char *
mongoc_oidc_cache_get_cached_token(const mongoc_oidc_cache_t *cache)
{
   BSON_ASSERT_PARAM(cache);

   // Cast away const to lock. This function is logically const (read-only).
   MC_DISABLE_CAST_QUAL_WARNING_BEGIN
   bson_shared_mutex_lock_shared(&((mongoc_oidc_cache_t *)cache)->lock);
   char *token = bson_strdup(cache->token);
   bson_shared_mutex_unlock_shared(&((mongoc_oidc_cache_t *)cache)->lock);
   MC_DISABLE_CAST_QUAL_WARNING_END
   return token;
}

void
mongoc_oidc_cache_set_cached_token(mongoc_oidc_cache_t *cache, const char *token)
{
   BSON_ASSERT_PARAM(cache);
   BSON_OPTIONAL_PARAM(token);

   char *old_token;

   // Lock to update token:
   {
      bson_shared_mutex_lock(&cache->lock);
      old_token = cache->token;
      cache->token = bson_strdup(token);
      bson_shared_mutex_unlock(&cache->lock);
   }
   bson_free(old_token);
}

char *
mongoc_oidc_cache_get_token(mongoc_oidc_cache_t *cache, bool *found_in_cache, bson_error_t *error)
{
   BSON_ASSERT_PARAM(cache);
   BSON_ASSERT_PARAM(found_in_cache);
   BSON_OPTIONAL_PARAM(error);

   char *token = NULL;

   *found_in_cache = false;

   if (!cache->user_callback && !cache->env_callback) {
      SET_ERROR("MONGODB-OIDC requested, but no callback set");
      return NULL;
   }

   // From spec: "If both ENVIRONMENT and an OIDC Callback [...] are provided the driver MUST raise an error."
   if (cache->user_callback && cache->env_callback) {
      SET_ERROR("MONGODB-OIDC requested with both ENVIRONMENT and an OIDC Callback. Use one or the other.");
      return NULL;
   }

   const mongoc_oidc_callback_t *callback =
      cache->user_callback ? cache->user_callback : mongoc_oidc_env_callback_inner(cache->env_callback);

   token = mongoc_oidc_cache_get_cached_token(cache);
   if (NULL != token) {
      *found_in_cache = true;
      return token;
   }

   // Prepare to call callback outside of lock:
   mongoc_oidc_credential_t *cred = NULL;
   mongoc_oidc_callback_params_t *params = mongoc_oidc_callback_params_new();
   mongoc_oidc_callback_params_set_user_data(params, mongoc_oidc_callback_get_user_data(callback));
   // From spec: "If CSOT is not applied, then the driver MUST use 1 minute as the timeout."
   // The timeout parameter (when set) is meant to be directly compared against bson_get_monotonic_time(). It is a
   // time point, not a duration. bson_get_monotonic_time() calls mlib_now(). Use mlib_now() directly.
   mongoc_oidc_callback_params_set_timeout(
      params, mlib_microseconds_count(mlib_time_add(mlib_now(), mlib_duration(60, s)).time_since_monotonic_start));
   mongoc_oidc_callback_params_set_username(params, cache->username);

   // Obtain write-lock:
   {
      bson_shared_mutex_lock(&cache->lock);
      // Check if another thread populated cache between checking cached token and obtaining write lock:
      if (cache->token) {
         *found_in_cache = true;
         token = bson_strdup(cache->token);
         goto unlock_and_return;
      }

      // From spec: "Wait until it has been at least 100ms since the last callback invocation"
      if (cache->ever_called) {
         mlib_duration since_last_call = mlib_time_difference(mlib_now(), cache->last_called);
         if (mlib_duration_cmp(since_last_call, <, (100, ms))) {
            mlib_duration to_sleep = mlib_duration((100, ms), minus, since_last_call);
            cache->usleep_fn(mlib_microseconds_count(to_sleep), cache->usleep_data);
         }
      }

      // Call callback:
      cred = mongoc_oidc_callback_get_fn(callback)(params);

      cache->last_called = mlib_now();
      cache->ever_called = true;

      if (!cred) {
         if (mongoc_oidc_callback_params_get_cancelled_with_timeout(params)) {
            SET_ERROR("MONGODB-OIDC callback was cancelled due to timeout");
            goto unlock_and_return;
         }
         SET_ERROR("MONGODB-OIDC callback failed");
         goto unlock_and_return;
      }

      token = bson_strdup(mongoc_oidc_credential_get_access_token(cred));
      cache->token = bson_strdup(token); // Cache a copy.

   unlock_and_return:
      bson_shared_mutex_unlock(&cache->lock);
   }
   mongoc_oidc_callback_params_destroy(params);
   mongoc_oidc_credential_destroy(cred);
   return token;
}

void
mongoc_oidc_cache_invalidate_token(mongoc_oidc_cache_t *cache, const char *token)
{
   BSON_ASSERT_PARAM(cache);
   BSON_ASSERT_PARAM(token);

   char *old_token = NULL;

   // Lock to clear token
   {
      bson_shared_mutex_lock(&cache->lock);
      if (cache->token && 0 == strcmp(cache->token, token)) {
         old_token = cache->token;
         cache->token = NULL;
      }
      bson_shared_mutex_unlock(&cache->lock);
   }

   bson_free(old_token);
}

struct mongoc_oidc_connection_cache_t {
   char *token;
};

mongoc_oidc_connection_cache_t *
mongoc_oidc_connection_cache_new(void)
{
   mongoc_oidc_connection_cache_t *oidc = bson_malloc0(sizeof(mongoc_oidc_connection_cache_t));
   return oidc;
}

void
mongoc_oidc_connection_cache_set(mongoc_oidc_connection_cache_t *cache, const char *token)
{
   BSON_ASSERT_PARAM(cache);
   BSON_OPTIONAL_PARAM(token);
   bson_free(cache->token);
   cache->token = bson_strdup(token);
}

char *
mongoc_oidc_connection_cache_get(const mongoc_oidc_connection_cache_t *cache)
{
   BSON_ASSERT_PARAM(cache);
   return bson_strdup(cache->token);
}

void
mongoc_oidc_connection_cache_destroy(mongoc_oidc_connection_cache_t *cache)
{
   if (!cache) {
      return;
   }
   bson_free(cache->token);
   bson_free(cache);
}
