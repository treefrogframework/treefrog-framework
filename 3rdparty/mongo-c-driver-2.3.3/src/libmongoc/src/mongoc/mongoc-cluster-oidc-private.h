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


#ifndef MONGOC_CLUSTER_OIDC_PRIVATE_H
#define MONGOC_CLUSTER_OIDC_PRIVATE_H

#include <mongoc/mongoc-oidc-cache-private.h>
#include <mongoc/mongoc-server-description-private.h>
#include <mongoc/mongoc-stream-private.h>

struct _mongoc_cluster_t; // Forward declare.

#include <bson/error.h>

bool
mongoc_oidc_append_speculative_auth(const char *access_token, uint32_t server_id, bson_t *cmd, bson_error_t *error);


bool
_mongoc_cluster_auth_node_oidc(struct _mongoc_cluster_t *cluster,
                               mongoc_stream_t *stream,
                               mongoc_oidc_connection_cache_t *conn_cache,
                               const mongoc_server_description_t *sd,
                               bson_error_t *error);

bool
_mongoc_cluster_reauth_node_oidc(struct _mongoc_cluster_t *cluster,
                                 mongoc_stream_t *stream,
                                 mongoc_oidc_connection_cache_t *conn_cache,
                                 const mongoc_server_description_t *sd,
                                 bson_error_t *error);

#endif /* MONGOC_CLUSTER_OIDC_PRIVATE_H */
