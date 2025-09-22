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

#ifndef MONGOC_CLIENT_POOL_H
#define MONGOC_CLIENT_POOL_H

#include <mongoc/mongoc-apm.h>
#include <mongoc/mongoc-client.h>
#include <mongoc/mongoc-config.h>
#include <mongoc/mongoc-macros.h>

#include <bson/bson.h>
#ifdef MONGOC_ENABLE_SSL
#include <mongoc/mongoc-ssl.h>
#endif
#include <mongoc/mongoc-structured-log.h>
#include <mongoc/mongoc-uri.h>


BSON_BEGIN_DECLS


typedef struct _mongoc_client_pool_t mongoc_client_pool_t;


MONGOC_EXPORT (mongoc_client_pool_t *)
mongoc_client_pool_new (const mongoc_uri_t *uri) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (mongoc_client_pool_t *)
mongoc_client_pool_new_with_error (const mongoc_uri_t *uri, bson_error_t *error) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_client_pool_destroy (mongoc_client_pool_t *pool);

MONGOC_EXPORT (mongoc_client_t *)
mongoc_client_pool_pop (mongoc_client_pool_t *pool) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_client_pool_push (mongoc_client_pool_t *pool, mongoc_client_t *client);

MONGOC_EXPORT (mongoc_client_t *)
mongoc_client_pool_try_pop (mongoc_client_pool_t *pool) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_client_pool_max_size (mongoc_client_pool_t *pool, uint32_t max_pool_size);

#ifdef MONGOC_ENABLE_SSL
MONGOC_EXPORT (void)
mongoc_client_pool_set_ssl_opts (mongoc_client_pool_t *pool, const mongoc_ssl_opt_t *opts);
#endif

MONGOC_EXPORT (bool)
mongoc_client_pool_set_apm_callbacks (mongoc_client_pool_t *pool, mongoc_apm_callbacks_t *callbacks, void *context);

MONGOC_EXPORT (bool)
mongoc_client_pool_set_error_api (mongoc_client_pool_t *pool, int32_t version);

MONGOC_EXPORT (bool)
mongoc_client_pool_set_appname (mongoc_client_pool_t *pool, const char *appname);

MONGOC_EXPORT (bool)
mongoc_client_pool_enable_auto_encryption (mongoc_client_pool_t *pool,
                                           mongoc_auto_encryption_opts_t *opts,
                                           bson_error_t *error);

MONGOC_EXPORT (bool)
mongoc_client_pool_set_server_api (mongoc_client_pool_t *pool, const mongoc_server_api_t *api, bson_error_t *error);

MONGOC_EXPORT (bool)
mongoc_client_pool_set_structured_log_opts (mongoc_client_pool_t *pool, const mongoc_structured_log_opts_t *opts);

BSON_END_DECLS


#endif /* MONGOC_CLIENT_POOL_H */
