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

#ifndef MONGOC_OIDC_CALLBACK_H
#define MONGOC_OIDC_CALLBACK_H

#include <mongoc/mongoc-macros.h>

#include <bson/bson.h>

#include <stdint.h>

BSON_BEGIN_DECLS

typedef struct _mongoc_oidc_callback_t mongoc_oidc_callback_t;
typedef struct _mongoc_oidc_callback_params_t mongoc_oidc_callback_params_t;
typedef struct _mongoc_oidc_credential_t mongoc_oidc_credential_t;

typedef mongoc_oidc_credential_t *(MONGOC_CALL *mongoc_oidc_callback_fn_t)(mongoc_oidc_callback_params_t *params);

MONGOC_EXPORT(mongoc_oidc_callback_t *)
mongoc_oidc_callback_new(mongoc_oidc_callback_fn_t fn);

MONGOC_EXPORT(mongoc_oidc_callback_t *)
mongoc_oidc_callback_new_with_user_data(mongoc_oidc_callback_fn_t fn, void *user_data);

MONGOC_EXPORT(void)
mongoc_oidc_callback_destroy(mongoc_oidc_callback_t *callback);

MONGOC_EXPORT(mongoc_oidc_callback_fn_t)
mongoc_oidc_callback_get_fn(const mongoc_oidc_callback_t *callback);

MONGOC_EXPORT(void *)
mongoc_oidc_callback_get_user_data(const mongoc_oidc_callback_t *callback);

MONGOC_EXPORT(void)
mongoc_oidc_callback_set_user_data(mongoc_oidc_callback_t *callback, void *user_data);

MONGOC_EXPORT(int32_t)
mongoc_oidc_callback_params_get_version(const mongoc_oidc_callback_params_t *params);

MONGOC_EXPORT(void *)
mongoc_oidc_callback_params_get_user_data(const mongoc_oidc_callback_params_t *params);

MONGOC_EXPORT(const int64_t *)
mongoc_oidc_callback_params_get_timeout(const mongoc_oidc_callback_params_t *params);

MONGOC_EXPORT(const char *)
mongoc_oidc_callback_params_get_username(const mongoc_oidc_callback_params_t *params);

MONGOC_EXPORT(mongoc_oidc_credential_t *)
mongoc_oidc_callback_params_cancel_with_timeout(mongoc_oidc_callback_params_t *params);

MONGOC_EXPORT(mongoc_oidc_credential_t *)
mongoc_oidc_credential_new(const char *access_token);

MONGOC_EXPORT(mongoc_oidc_credential_t *)
mongoc_oidc_credential_new_with_expires_in(const char *access_token, int64_t expires_in);

MONGOC_EXPORT(void)
mongoc_oidc_credential_destroy(mongoc_oidc_credential_t *cred);

MONGOC_EXPORT(const char *)
mongoc_oidc_credential_get_access_token(const mongoc_oidc_credential_t *cred);

MONGOC_EXPORT(const int64_t *)
mongoc_oidc_credential_get_expires_in(const mongoc_oidc_credential_t *cred);

BSON_END_DECLS

#endif // MONGOC_OIDC_CALLBACK_H
