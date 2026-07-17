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

#include <mongoc/mongoc-oidc-env-private.h>
#include <mongoc/mongoc-util-private.h>

#include <mongoc/mcd-azure.h>
#include <mongoc/mongoc-oidc-callback.h>
#include <mongoc/service-gcp.h>

#include <mlib/duration.h>
#include <mlib/time_point.h>
#include <mlib/timer.h>

struct _mongoc_oidc_env_t {
   const char *name;
   mongoc_oidc_callback_fn_t callback_fn;
   bool supports_username;
   bool requires_token_resource;
};

struct _mongoc_oidc_env_callback_t {
   mongoc_oidc_callback_t *inner; // Contains non-owning user_data pointer back to this mongoc_oidc_env_callback_t
   char *token_resource;
};

static mongoc_oidc_credential_t *
mongoc_oidc_env_fn_test(mongoc_oidc_callback_params_t *params)
{
   BSON_UNUSED(params);
   // TODO (CDRIVER-4489)
   return NULL;
}

static mongoc_oidc_credential_t *
mongoc_oidc_env_fn_azure(mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM(params);

   bson_error_t error;
   mcd_azure_access_token token = {0};
   mongoc_oidc_credential_t *ret = NULL;
   mongoc_oidc_env_callback_t *callback = mongoc_oidc_callback_params_get_user_data(params);
   BSON_ASSERT(callback);

   mlib_timer timer = {0};
   const int64_t *timeout_us = mongoc_oidc_callback_params_get_timeout(params);
   if (timeout_us) {
      timer = mlib_expires_at((mlib_time_point){.time_since_monotonic_start = mlib_duration(*timeout_us, us)});
      if (mlib_timer_is_expired(timer)) {
         // No time remaining. Immediately fail.
         mongoc_oidc_callback_params_cancel_with_timeout(params);
         goto fail;
      }
   }

   if (!mcd_azure_access_token_from_imds(&token,
                                         callback->token_resource,
                                         NULL, // Use the default host
                                         0,    // Default port as well
                                         NULL, // No extra headers
                                         timer,
                                         mongoc_oidc_callback_params_get_username(params), // Optional client id
                                         &error)) {
      MONGOC_ERROR("Failed to obtain Azure OIDC access token: %s", error.message);
      goto fail;
   }

   ret = mongoc_oidc_credential_new_with_expires_in(token.access_token, mlib_microseconds_count(token.expires_in));
   if (!ret) {
      MONGOC_ERROR("Failed to process Azure OIDC access token");

      if (!token.access_token) {
         MONGOC_ERROR("missing Azure OIDC access token string");
      }

      if (mlib_microseconds_count(token.expires_in) < 0) {
         MONGOC_ERROR("Azure OIDC access token expiration must not be a negative value");
      }

      goto fail;
   }

fail:
   mcd_azure_access_token_destroy(&token);
   return ret;
}

static mongoc_oidc_credential_t *
mongoc_oidc_env_fn_gcp(mongoc_oidc_callback_params_t *params)
{
   BSON_ASSERT_PARAM(params);

   bson_error_t error;
   gcp_service_account_token token = {0};
   mongoc_oidc_credential_t *ret = NULL;
   mongoc_oidc_env_callback_t *callback = mongoc_oidc_callback_params_get_user_data(params);
   BSON_ASSERT(callback);

   mlib_timer timer = {0};
   const int64_t *timeout_us = mongoc_oidc_callback_params_get_timeout(params);
   if (timeout_us) {
      timer = mlib_expires_at((mlib_time_point){.time_since_monotonic_start = mlib_duration(*timeout_us, us)});
      if (mlib_timer_is_expired(timer)) {
         // No time remaining. Immediately fail.
         mongoc_oidc_callback_params_cancel_with_timeout(params);
         goto fail;
      }
   }

   if (!gcp_identity_token_from_gcp_server(&token, callback->token_resource, timer, &error)) {
      MONGOC_ERROR("Failed to obtain GCP OIDC access token: %s", error.message);
      goto fail;
   }

   ret = mongoc_oidc_credential_new(token.access_token);
   if (!ret) {
      MONGOC_ERROR("Failed to process GCP OIDC access token");
      goto fail;
   }

fail:
   gcp_access_token_destroy(&token);
   return ret;
}

static mongoc_oidc_credential_t *
mongoc_oidc_env_fn_k8s(mongoc_oidc_callback_params_t *params)
{
   BSON_UNUSED(params);

   mongoc_oidc_credential_t *ret = NULL;
   mongoc_stream_t *fstream = NULL;

   const char *token_file_path = "/var/run/secrets/kubernetes.io/serviceaccount/token";
   char *AZURE_FEDERATED_TOKEN_FILE = _mongoc_getenv("AZURE_FEDERATED_TOKEN_FILE");
   if (AZURE_FEDERATED_TOKEN_FILE) {
      token_file_path = AZURE_FEDERATED_TOKEN_FILE;
   }
   char *AWS_WEB_IDENTITY_TOKEN_FILE = _mongoc_getenv("AWS_WEB_IDENTITY_TOKEN_FILE");
   if (AWS_WEB_IDENTITY_TOKEN_FILE) {
      token_file_path = AWS_WEB_IDENTITY_TOKEN_FILE;
   }

   // Read contents of token file.
   {
      fstream = mongoc_stream_file_new_for_path(token_file_path, O_RDONLY, 0);
      if (!fstream) {
         MONGOC_ERROR("failed to open K8s token file: %s. Reason: %s", token_file_path, strerror(errno));
         goto fail;
      }

      mcommon_string_append_t append;
      mcommon_string_new_as_append(&append);
      for (;;) {
         char buf[128];
         ssize_t got = mongoc_stream_read(
            fstream, buf, sizeof(buf), 0 /* min_bytes */, 0 /* timeout_msec. Unused for file stream. */);

         if (got > 0) {
            mcommon_string_append_bytes(&append, (const char *)buf, (uint32_t)got);
         } else if (got == 0) {
            // EOF.
            break;
         } else {
            mcommon_string_destroy(mcommon_string_from_append(&append));
            MONGOC_ERROR("failed to read K8s token file: %s. Reason: %s", token_file_path, strerror(errno));
            goto fail;
         }
      }
      mcommon_string_t *token_file_contents = mcommon_string_from_append(&append);
      ret = mongoc_oidc_credential_new(token_file_contents->str);
      mcommon_string_destroy(token_file_contents);
   }

fail:

   mongoc_stream_destroy(fstream);
   bson_free(AWS_WEB_IDENTITY_TOKEN_FILE);
   bson_free(AZURE_FEDERATED_TOKEN_FILE);
   return ret;
}

const mongoc_oidc_env_t *
mongoc_oidc_env_find(const char *name)
{
   static const mongoc_oidc_env_t oidc_env_table[] = {
      {.name = "test", .callback_fn = mongoc_oidc_env_fn_test},
      {.name = "azure",
       .supports_username = true,
       .requires_token_resource = true,
       .callback_fn = mongoc_oidc_env_fn_azure},
      {.name = "gcp", .requires_token_resource = true, .callback_fn = mongoc_oidc_env_fn_gcp},
      {.name = "k8s", .callback_fn = mongoc_oidc_env_fn_k8s},
      {0}};

   if (name) {
      for (const mongoc_oidc_env_t *row = oidc_env_table; row->name; ++row) {
         if (!strcmp(name, row->name)) {
            return row;
         }
      }
   }
   return NULL;
}

const char *
mongoc_oidc_env_name(const mongoc_oidc_env_t *env)
{
   BSON_ASSERT_PARAM(env);
   return env->name;
}

bool
mongoc_oidc_env_supports_username(const mongoc_oidc_env_t *env)
{
   BSON_ASSERT_PARAM(env);
   return env->supports_username;
}

bool
mongoc_oidc_env_requires_token_resource(const mongoc_oidc_env_t *env)
{
   BSON_ASSERT_PARAM(env);
   return env->requires_token_resource;
}

mongoc_oidc_env_callback_t *
mongoc_oidc_env_callback_new(const mongoc_oidc_env_t *env, const char *token_resource)
{
   BSON_ASSERT_PARAM(env);
   BSON_OPTIONAL_PARAM(token_resource);
   mongoc_oidc_env_callback_t *env_callback = bson_malloc(sizeof *env_callback);
   // Note that the callback's user_data points back to this containing mongoc_oidc_env_callback_t.
   // We expect that the inner callback can only be destroyed via mongoc_oidc_env_callback_destroy.
   *env_callback =
      (mongoc_oidc_env_callback_t){.inner = mongoc_oidc_callback_new_with_user_data(env->callback_fn, env_callback),
                                   .token_resource = bson_strdup(token_resource)};
   return env_callback;
}

void
mongoc_oidc_env_callback_destroy(mongoc_oidc_env_callback_t *env_callback)
{
   if (env_callback) {
      BSON_ASSERT(mongoc_oidc_callback_get_user_data(env_callback->inner) == (void *)env_callback);
      mongoc_oidc_callback_destroy(env_callback->inner);
      bson_free(env_callback->token_resource);
      bson_free(env_callback);
   }
}

const mongoc_oidc_callback_t *
mongoc_oidc_env_callback_inner(const mongoc_oidc_env_callback_t *env_callback)
{
   BSON_ASSERT_PARAM(env_callback);
   return env_callback->inner;
}
