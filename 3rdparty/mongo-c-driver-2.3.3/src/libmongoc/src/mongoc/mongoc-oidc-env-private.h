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

#include <mongoc/mongoc-prelude.h>

#ifndef MONGOC_OIDC_ENV_PRIVATE_H
#define MONGOC_OIDC_ENV_PRIVATE_H

#include <mongoc/mongoc-macros.h>
#include <mongoc/mongoc-oidc-callback.h>

BSON_BEGIN_DECLS

typedef struct _mongoc_oidc_env_t mongoc_oidc_env_t;
typedef struct _mongoc_oidc_env_callback_t mongoc_oidc_env_callback_t;

const mongoc_oidc_env_t *
mongoc_oidc_env_find(const char *name);

const char *
mongoc_oidc_env_name(const mongoc_oidc_env_t *env);

bool
mongoc_oidc_env_supports_username(const mongoc_oidc_env_t *env);

bool
mongoc_oidc_env_requires_token_resource(const mongoc_oidc_env_t *env);

mongoc_oidc_env_callback_t *
mongoc_oidc_env_callback_new(const mongoc_oidc_env_t *env, const char *token_resource);

void
mongoc_oidc_env_callback_destroy(mongoc_oidc_env_callback_t *env_callback);

const mongoc_oidc_callback_t *
mongoc_oidc_env_callback_inner(const mongoc_oidc_env_callback_t *env_callback);

BSON_END_DECLS

#endif // MONGOC_OIDC_ENV_PRIVATE_H
