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

#ifndef MONGOC_OIDC_CALLBACK_PRIVATE_H
#define MONGOC_OIDC_CALLBACK_PRIVATE_H

#include <mongoc/mongoc-oidc-callback.h>

//

#include <stdint.h>

// Authentication spec: the version number is used to communicate callback API changes that are not breaking but that
// users may want to know about and review their implementation. Drivers MUST pass 1 for the initial callback API
// version number and increment the version number anytime the API changes.
#define MONGOC_PRIVATE_OIDC_CALLBACK_API_VERSION 1

mongoc_oidc_callback_params_t *
mongoc_oidc_callback_params_new(void);

void
mongoc_oidc_callback_params_destroy(mongoc_oidc_callback_params_t *params);

void
mongoc_oidc_callback_params_set_version(mongoc_oidc_callback_params_t *params, int32_t version);

void
mongoc_oidc_callback_params_set_user_data(mongoc_oidc_callback_params_t *params, void *user_data);

void
mongoc_oidc_callback_params_set_timeout(mongoc_oidc_callback_params_t *params, int64_t timeout);

void
mongoc_oidc_callback_params_unset_timeout(mongoc_oidc_callback_params_t *params);

void
mongoc_oidc_callback_params_set_username(mongoc_oidc_callback_params_t *params, const char *username);

bool
mongoc_oidc_callback_params_get_cancelled_with_timeout(const mongoc_oidc_callback_params_t *params);

void
mongoc_oidc_callback_params_set_cancelled_with_timeout(mongoc_oidc_callback_params_t *params, bool value);

mongoc_oidc_callback_t *
mongoc_oidc_callback_copy(const mongoc_oidc_callback_t *callback);

#endif // MONGOC_OIDC_CALLBACK_PRIVATE_H
