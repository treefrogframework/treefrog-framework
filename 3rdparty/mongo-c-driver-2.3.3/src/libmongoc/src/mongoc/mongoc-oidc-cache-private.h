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

#ifndef MONGOC_OIDC_CACHE_PRIVATE_H
#define MONGOC_OIDC_CACHE_PRIVATE_H

#include <mongoc/mongoc-oidc-callback.h>
#include <mongoc/mongoc-sleep.h>

struct _mongoc_uri_t; // Forward declaration.

// mongoc_oidc_cache_t implements the OIDC spec "Client Cache".
// Stores the OIDC callback, cache, and lock.
// Expected to be shared among all clients in a pool.
typedef struct mongoc_oidc_cache_t mongoc_oidc_cache_t;

mongoc_oidc_cache_t *
mongoc_oidc_cache_new(void);

// mongoc_oidc_cache_apply_env_from_uri tries to set a callback if the URI includes an ENVIRONMENT.
// Assumes `uri` was already validated with a call to `mongoc_uri_finalize_auth`.
void
mongoc_oidc_cache_apply_env_from_uri(mongoc_oidc_cache_t *cache, const struct _mongoc_uri_t *uri);

// mongoc_oidc_cache_set_user_callback sets the token callback.
// Not thread safe. Call before any authentication can occur.
void
mongoc_oidc_cache_set_user_callback(mongoc_oidc_cache_t *cache, const char *username, const mongoc_oidc_callback_t *cb);

// mongoc_oidc_cache_has_user_callback returns true if a custom callback was set.
bool
mongoc_oidc_cache_has_user_callback(const mongoc_oidc_cache_t *cache);

// mongoc_oidc_cache_set_usleep_fn sets a custom sleep function.
// Not thread safe. Call before any authentication can occur.
void
mongoc_oidc_cache_set_usleep_fn(mongoc_oidc_cache_t *cache, mongoc_usleep_func_t usleep_fn, void *usleep_data);

// mongoc_oidc_cache_get_token returns a token or NULL on error. Thread safe.
// Sets *found_in_cache to indicate if the returned token came from the cache or callback.
// Calls sleep if needed to enforce 100ms delay between calls to the callback.
char *
mongoc_oidc_cache_get_token(mongoc_oidc_cache_t *cache, bool *found_in_cache, bson_error_t *error);

// mongoc_oidc_cache_get_cached_token returns a cached token or NULL if none is cached. Thread safe.
char *
mongoc_oidc_cache_get_cached_token(const mongoc_oidc_cache_t *cache);

// mongoc_oidc_cache_set_cached_token overwrites the cached token. Useful for tests. Thread safe.
void
mongoc_oidc_cache_set_cached_token(mongoc_oidc_cache_t *cache, const char *token);

// mongoc_oidc_cache_invalidate_token invalidates the cached token if it matches `token`. Thread safe.
void
mongoc_oidc_cache_invalidate_token(mongoc_oidc_cache_t *cache, const char *token);

void
mongoc_oidc_cache_destroy(mongoc_oidc_cache_t *);

// mongoc_oidc_connection_cache_t implements the OIDC spec "Connection Cache".
// Stores a possible OIDC access token used to authenticate one mongoc_stream_t.
typedef struct mongoc_oidc_connection_cache_t mongoc_oidc_connection_cache_t;

mongoc_oidc_connection_cache_t *
mongoc_oidc_connection_cache_new(void);

// mongoc_oidc_connection_cache_set overwrites the cached token. Pass a NULL token to clear cache.
void
mongoc_oidc_connection_cache_set(mongoc_oidc_connection_cache_t *cache, const char *token);

// mongoc_oidc_connection_cache_get returns the cached token or NULL.
char *
mongoc_oidc_connection_cache_get(const mongoc_oidc_connection_cache_t *cache);

void
mongoc_oidc_connection_cache_destroy(mongoc_oidc_connection_cache_t *cache);

#endif // MONGOC_OIDC_CACHE_PRIVATE_H
