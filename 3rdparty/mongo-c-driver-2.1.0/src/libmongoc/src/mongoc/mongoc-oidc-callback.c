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

#include <mongoc/mongoc-oidc-callback-private.h>

//

#include <bson/bson.h>

struct _mongoc_oidc_callback_t {
   mongoc_oidc_callback_fn_t fn;
   void *user_data;
};

struct _mongoc_oidc_callback_params_t {
   void *user_data;
   char *username;
   int64_t timeout; // Guarded by timeout_is_set.
   int32_t version;
   bool cancelled_with_timeout;
   bool timeout_is_set;
};

struct _mongoc_oidc_credential_t {
   char *access_token;
   int64_t expires_in; // Guarded by expires_in_set.
   bool expires_in_set;
};

mongoc_oidc_callback_t *
mongoc_oidc_callback_new (mongoc_oidc_callback_fn_t fn)
{
   if (!fn) {
      return NULL;
   }

   mongoc_oidc_callback_t *const ret = bson_malloc (sizeof (*ret));
   *ret = (mongoc_oidc_callback_t) {.fn = fn};
   return ret;
}

mongoc_oidc_callback_t *
mongoc_oidc_callback_new_with_user_data (mongoc_oidc_callback_fn_t fn, void *user_data)
{
   if (!fn) {
      return NULL;
   }

   mongoc_oidc_callback_t *const ret = bson_malloc (sizeof (*ret));
   *ret = (mongoc_oidc_callback_t) {.fn = fn, .user_data = user_data};
   return ret;
}

void
mongoc_oidc_callback_destroy (mongoc_oidc_callback_t *callback)
{
   if (callback) {
      bson_free (callback);
   }
}

mongoc_oidc_callback_fn_t
mongoc_oidc_callback_get_fn (const mongoc_oidc_callback_t *callback)
{
   BSON_ASSERT_PARAM (callback);
   return callback->fn;
}

void *
mongoc_oidc_callback_get_user_data (const mongoc_oidc_callback_t *callback)
{
   BSON_ASSERT_PARAM (callback);
   return callback->user_data;
}

void
mongoc_oidc_callback_set_user_data (mongoc_oidc_callback_t *callback, void *user_data)
{
   BSON_ASSERT_PARAM (callback);
   callback->user_data = user_data;
}

mongoc_oidc_callback_params_t *
mongoc_oidc_callback_params_new (void)
{
   mongoc_oidc_callback_params_t *const ret = bson_malloc (sizeof (*ret));
   *ret = (mongoc_oidc_callback_params_t) {
      .version = MONGOC_PRIVATE_OIDC_CALLBACK_API_VERSION,
   };
   return ret;
}

void
mongoc_oidc_callback_params_destroy (mongoc_oidc_callback_params_t *params)
{
   if (params) {
      bson_free (params->username);
      bson_free (params);
   }
}

int32_t
mongoc_oidc_callback_params_get_version (const mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM (params);
   return params->version;
}

void
mongoc_oidc_callback_params_set_version (mongoc_oidc_callback_params_t *params, int32_t version)
{
   BSON_ASSERT_PARAM (params);
   params->version = version;
}

void *
mongoc_oidc_callback_params_get_user_data (const mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM (params);
   return params->user_data;
}

void
mongoc_oidc_callback_params_set_user_data (mongoc_oidc_callback_params_t *params, void *user_data)
{
   BSON_ASSERT_PARAM (params);
   params->user_data = user_data;
}

const int64_t *
mongoc_oidc_callback_params_get_timeout (const mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM (params);
   return params->timeout_is_set ? &params->timeout : NULL;
}

void
mongoc_oidc_callback_params_set_timeout (mongoc_oidc_callback_params_t *params, int64_t timeout)
{
   BSON_ASSERT_PARAM (params);
   params->timeout = timeout;
   params->timeout_is_set = true;
}

void
mongoc_oidc_callback_params_unset_timeout (mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM (params);
   params->timeout_is_set = false;
}

const char *
mongoc_oidc_callback_params_get_username (const mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM (params);
   return params->username;
}

void
mongoc_oidc_callback_params_set_username (mongoc_oidc_callback_params_t *params, const char *username)
{
   BSON_ASSERT_PARAM (params);
   bson_free (params->username);
   params->username = bson_strdup (username);
}

mongoc_oidc_credential_t *
mongoc_oidc_callback_params_cancel_with_timeout (mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM (params);
   params->cancelled_with_timeout = true;
   return NULL;
}

bool
mongoc_oidc_callback_params_get_cancelled_with_timeout (const mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM (params);
   return params->cancelled_with_timeout;
}

void
mongoc_oidc_callback_params_set_cancelled_with_timeout (mongoc_oidc_callback_params_t *params, bool value)
{
   BSON_ASSERT_PARAM (params);
   params->cancelled_with_timeout = value;
}

mongoc_oidc_credential_t *
mongoc_oidc_credential_new (const char *access_token)
{
   if (!access_token) {
      return NULL;
   }

   mongoc_oidc_credential_t *const ret = bson_malloc (sizeof (*ret));
   *ret = (mongoc_oidc_credential_t) {
      .access_token = bson_strdup (access_token),
      .expires_in_set = false, // Infinite.
   };
   return ret;
}

mongoc_oidc_credential_t *
mongoc_oidc_credential_new_with_expires_in (const char *access_token, int64_t expires_in)
{
   if (!access_token) {
      return NULL;
   }

   if (expires_in < 0) {
      return NULL;
   }

   mongoc_oidc_credential_t *const ret = bson_malloc (sizeof (*ret));
   *ret = (mongoc_oidc_credential_t) {
      .access_token = bson_strdup (access_token),
      .expires_in_set = true,
      .expires_in = expires_in,
   };
   return ret;
}

void
mongoc_oidc_credential_destroy (mongoc_oidc_credential_t *cred)
{
   if (cred) {
      bson_free (cred->access_token);
      bson_free (cred);
   }
}

const char *
mongoc_oidc_credential_get_access_token (const mongoc_oidc_credential_t *cred)
{
   BSON_ASSERT_PARAM (cred);
   return cred->access_token;
}

const int64_t *
mongoc_oidc_credential_get_expires_in (const mongoc_oidc_credential_t *cred)
{
   BSON_ASSERT_PARAM (cred);
   return cred->expires_in_set ? &cred->expires_in : NULL;
}
