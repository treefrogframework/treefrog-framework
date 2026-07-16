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

#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-retry-backoff-generator-private.h>
#include <mongoc/mongoc-retryable-cmd-private.h>
#include <mongoc/mongoc-trace-private.h>
#include <mongoc/mongoc-write-command-private.h>

#include <mlib/time_point.h>

#define MONGOC_RETRYABLE_CMD_BACKOFF_GROWTH_FACTOR 2.0
#define MONGOC_RETRYABLE_CMD_BACKOFF_INITIAL mlib_duration(100, ms)
#define MONGOC_RETRYABLE_CMD_BACKOFF_MAX mlib_duration(10, s)

bool
_mongoc_retryable_cmd_run(const mongoc_retryable_cmd_t *cmd, bson_t *reply, bson_error_t *error)
{
   BSON_ASSERT_PARAM(cmd);
   BSON_ASSERT(cmd->execute);
   BSON_ASSERT(cmd->select_retry_server);
   BSON_ASSERT(cmd->jitter_source);
   BSON_ASSERT(cmd->initial_server_description);

   bool ret = false;

   int attempt = 0;
   int allowed_retries = cmd->retry_eligibility == MONGOC_RETRY_ELIGIBILITY_OVERLOAD_ONLY ? 0 : 1;

   mongoc_server_description_t const *server_description = cmd->initial_server_description;

   mongoc_deprioritized_servers_t *const deprioritized_servers = mongoc_deprioritized_servers_new();

   const mongoc_retry_backoff_params_t retry_backoff_params = {
      .growth_factor = MONGOC_RETRYABLE_CMD_BACKOFF_GROWTH_FACTOR,
      .backoff_initial = MONGOC_RETRYABLE_CMD_BACKOFF_INITIAL,
      .backoff_max = MONGOC_RETRYABLE_CMD_BACKOFF_MAX,
   };

   mongoc_retry_backoff_generator_t *const retry_backoff_generator =
      _mongoc_retry_backoff_generator_new(retry_backoff_params, cmd->jitter_source);

   while (true) {
      ret = cmd->execute(cmd->user_data, reply, error);

      const bool is_retryable_read = cmd->retry_eligibility == MONGOC_RETRY_ELIGIBILITY_RETRYABLE_READ &&
                                     _mongoc_read_error_get_type(ret, error, reply) == MONGOC_READ_ERR_RETRY;
      const bool is_retryable_write = cmd->retry_eligibility == MONGOC_RETRY_ELIGIBILITY_RETRYABLE_WRITE &&
                                      _mongoc_write_error_get_type(reply) == MONGOC_WRITE_ERR_RETRY;

      const bool is_overload = mongoc_error_has_label(reply, MONGOC_ERROR_LABEL_SYSTEMOVERLOADEDERROR);
      const bool is_overload_retryable = is_overload &&
                                         mongoc_error_has_label(reply, MONGOC_ERROR_LABEL_RETRYABLEERROR) &&
                                         cmd->retry_eligibility != MONGOC_RETRY_ELIGIBILITY_NONE;

      const bool is_retryable = is_retryable_read || is_retryable_write || is_overload_retryable;

      if (ret && !is_retryable) {
         break;
      }

      // Propagate the error if it is non-retryable.
      if (!is_retryable) {
         break;
      }

      ++attempt;

      if (is_overload) {
         allowed_retries = cmd->max_adaptive_retries;
      }

      if (attempt > allowed_retries) {
         break;
      }

      if (server_description->type == MONGOC_SERVER_MONGOS || (is_overload && cmd->enable_overload_retargeting)) {
         TRACE("deprioritization: add to list: %s (id: %" PRIu32 ")",
               server_description->host.host_and_port,
               server_description->id);
         mongoc_deprioritized_servers_add(deprioritized_servers, server_description);
      }

      if (is_overload) {
         const mlib_duration backoff_duration = _mongoc_retry_backoff_generator_next(retry_backoff_generator);
         mlib_sleep_for(backoff_duration);
      } else {
         _mongoc_retry_backoff_generator_skip(retry_backoff_generator);
      }

      server_description = cmd->select_retry_server(cmd->user_data, deprioritized_servers, reply, error);

      if (!server_description) {
         break;
      }

      bson_destroy(reply);
   }

   _mongoc_retry_backoff_generator_destroy(retry_backoff_generator);
   mongoc_deprioritized_servers_destroy(deprioritized_servers);

   RETURN(ret);
}
