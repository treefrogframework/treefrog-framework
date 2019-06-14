/*
 * Copyright 2018-present MongoDB, Inc.
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

#ifndef JSON_TEST_OPERATIONS_H
#define JSON_TEST_OPERATIONS_H

struct _json_test_config_t;

typedef struct {
   const struct _json_test_config_t *config;
   uint32_t n_events;
   bson_t events;
   mongoc_uri_t *test_framework_uri;
   mongoc_client_session_t *sessions[2];
   bson_t lsids[2];
   bool acknowledged;
   bool verbose;
   bool has_sessions;
   mongoc_client_t *client;
   mongoc_database_t *db;
   mongoc_collection_t *collection;
} json_test_ctx_t;

mongoc_client_session_t *
session_from_name (json_test_ctx_t *ctx, const char *session_name);

void
json_test_ctx_init (json_test_ctx_t *ctx,
                    const bson_t *test,
                    mongoc_client_t *client,
                    mongoc_database_t *db,
                    mongoc_collection_t *collection,
                    const struct _json_test_config_t *config);

void
json_test_ctx_end_sessions (json_test_ctx_t *ctx);

void
json_test_ctx_cleanup (json_test_ctx_t *ctx);

typedef void (*json_test_operation_cb_t) (json_test_ctx_t *ctx,
                                          const bson_t *test,
                                          const bson_t *operation);

typedef void (*json_test_cb_t) (json_test_ctx_t *ctx, const bson_t *test);

void
json_test_operation (json_test_ctx_t *ctx,
                     const bson_t *test,
                     const bson_t *operation,
                     mongoc_collection_t *collection,
                     mongoc_client_session_t *session);

void
json_test_operations (json_test_ctx_t *ctx, const bson_t *test);

#endif
