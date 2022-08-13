/*
 * Copyright 2020-present MongoDB, Inc.
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

#include "mongoc-prelude.h"

#ifndef MONGOC_CLUSTER_AWS_PRIVATE_H
#define MONGOC_CLUSTER_AWS_PRIVATE_H

#include "bson/bson.h"
#include "mongoc/mongoc-cluster-private.h"

bool
_mongoc_cluster_auth_node_aws (mongoc_cluster_t *cluster,
                               mongoc_stream_t *stream,
                               mongoc_server_description_t *sd,
                               bson_error_t *error);

/* The following are declared in the private header for testing. It is only used
 * in test-mongoc-aws.c and mongoc-cluster.aws.c */
typedef struct {
   char *access_key_id;
   char *secret_access_key;
   char *session_token;
} _mongoc_aws_credentials_t;

bool
_mongoc_aws_credentials_obtain (mongoc_uri_t *uri,
                                _mongoc_aws_credentials_t *creds,
                                bson_error_t *error);

void
_mongoc_aws_credentials_cleanup (_mongoc_aws_credentials_t *creds);

bool
_mongoc_validate_and_derive_region (char *sts_fqdn,
                                    uint32_t sts_fqdn_len,
                                    char **region,
                                    bson_error_t *error);

#endif /* MONGOC_CLUSTER_AWS_PRIVATE_H */
