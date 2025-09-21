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

#include <mongoc/mongoc-oidc-callback.h>

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
mongoc_oidc_env_fn_test (mongoc_oidc_callback_params_t *params)
{
   BSON_UNUSED (params);
   // TODO (CDRIVER-4489)
   return NULL;
}

static mongoc_oidc_credential_t *
mongoc_oidc_env_fn_azure (mongoc_oidc_callback_params_t *params)
{
   BSON_UNUSED (params);
   // TODO (CDRIVER-4489)
   return NULL;
}

static mongoc_oidc_credential_t *
mongoc_oidc_env_fn_gcp (mongoc_oidc_callback_params_t *params)
{
   BSON_UNUSED (params);
   // TODO (CDRIVER-4489)
   return NULL;
}

static mongoc_oidc_credential_t *
mongoc_oidc_env_fn_k8s (mongoc_oidc_callback_params_t *params)
{
   BSON_UNUSED (params);
   // TODO (CDRIVER-4489)
   return NULL;
}

const mongoc_oidc_env_t *
mongoc_oidc_env_find (const char *name)
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
         if (!strcmp (name, row->name)) {
            return row;
         }
      }
   }
   return NULL;
}

const char *
mongoc_oidc_env_name (const mongoc_oidc_env_t *env)
{
   BSON_ASSERT_PARAM (env);
   return env->name;
}

bool
mongoc_oidc_env_supports_username (const mongoc_oidc_env_t *env)
{
   BSON_ASSERT_PARAM (env);
   return env->supports_username;
}

bool
mongoc_oidc_env_requires_token_resource (const mongoc_oidc_env_t *env)
{
   BSON_ASSERT_PARAM (env);
   return env->requires_token_resource;
}

mongoc_oidc_env_callback_t *
mongoc_oidc_env_callback_new (const mongoc_oidc_env_t *env, const char *token_resource)
{
   BSON_ASSERT_PARAM (env);
   BSON_OPTIONAL_PARAM (token_resource);
   mongoc_oidc_env_callback_t *env_callback = bson_malloc (sizeof *env_callback);
   // Note that the callback's user_data points back to this containing mongoc_oidc_env_callback_t.
   // We expect that the inner callback can only be destroyed via mongoc_oidc_env_callback_destroy.
   *env_callback =
      (mongoc_oidc_env_callback_t) {.inner = mongoc_oidc_callback_new_with_user_data (env->callback_fn, env_callback),
                                    .token_resource = bson_strdup (token_resource)};
   return env_callback;
}

void
mongoc_oidc_env_callback_destroy (mongoc_oidc_env_callback_t *env_callback)
{
   if (env_callback) {
      BSON_ASSERT (mongoc_oidc_callback_get_user_data (env_callback->inner) == (void *) env_callback);
      mongoc_oidc_callback_destroy (env_callback->inner);
      bson_free (env_callback->token_resource);
      bson_free (env_callback);
   }
}

const mongoc_oidc_callback_t *
mongoc_oidc_env_callback_inner (const mongoc_oidc_env_callback_t *env_callback)
{
   BSON_ASSERT_PARAM (env_callback);
   return env_callback->inner;
}
