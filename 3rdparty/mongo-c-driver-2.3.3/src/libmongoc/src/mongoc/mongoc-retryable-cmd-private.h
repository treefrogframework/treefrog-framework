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

#ifndef MONGOC_RETRYABLE_CMD_PRIVATE_H
#define MONGOC_RETRYABLE_CMD_PRIVATE_H

#include <mongoc/mongoc-jitter-source-private.h>
#include <mongoc/mongoc-server-stream-private.h>

#include <mlib/duration.h>

typedef enum {
   MONGOC_RETRY_ELIGIBILITY_NONE,
   MONGOC_RETRY_ELIGIBILITY_OVERLOAD_ONLY,
   MONGOC_RETRY_ELIGIBILITY_RETRYABLE_READ,  // Satisfies Retryable Read requirements
   MONGOC_RETRY_ELIGIBILITY_RETRYABLE_WRITE, // Satisfies Retryable Write requirements
} mongoc_retry_eligibility_t;

typedef bool (*mongoc_retryable_cmd_execute_cb_t)(void *user_data, bson_t *reply, bson_error_t *error);

typedef mongoc_server_description_t const *(*mongoc_retryable_cmd_select_retry_server_cb_t)(
   void *user_data, mongoc_deprioritized_servers_t *deprioritized_servers, bson_t *reply, bson_error_t *error);

typedef struct {
   mongoc_retryable_cmd_execute_cb_t execute;
   mongoc_retryable_cmd_select_retry_server_cb_t select_retry_server;
   void *user_data;
   mongoc_retry_eligibility_t retry_eligibility;
   mongoc_jitter_source_t *jitter_source;
   mongoc_server_description_t const *initial_server_description;
   int32_t max_adaptive_retries;
   bool enable_overload_retargeting;
} mongoc_retryable_cmd_t;

bool
_mongoc_retryable_cmd_run(const mongoc_retryable_cmd_t *cmd, bson_t *reply, bson_error_t *error);

#endif
