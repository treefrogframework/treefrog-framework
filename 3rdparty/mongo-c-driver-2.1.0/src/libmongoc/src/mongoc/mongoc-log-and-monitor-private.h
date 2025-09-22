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

#ifndef MONGOC_LOG_AND_MONITOR_PRIVATE_H
#define MONGOC_LOG_AND_MONITOR_PRIVATE_H

#include <mongoc/mongoc-apm-private.h>
#include <mongoc/mongoc-thread-private.h>

struct mongoc_structured_log_instance_t;
struct mongoc_structured_log_opts_t;

/*
 * @brief Logging and monitoring instance
 *
 * Includes APM callbacks, APM callback context, and the structured logging instance.
 *
 * It's owned by mongoc_topology_t on behalf of client/pool and borrowed by topology_description methods.
 */
typedef struct _mongoc_log_and_monitor_instance_t {
   bson_mutex_t apm_mutex;
   mongoc_apm_callbacks_t apm_callbacks;
   void *apm_context;
   struct mongoc_structured_log_instance_t *structured_log;
} mongoc_log_and_monitor_instance_t;

void
mongoc_log_and_monitor_instance_init (mongoc_log_and_monitor_instance_t *new_instance);

void
mongoc_log_and_monitor_instance_destroy_contents (mongoc_log_and_monitor_instance_t *instance);

void
mongoc_log_and_monitor_instance_set_apm_callbacks (mongoc_log_and_monitor_instance_t *instance,
                                                   const mongoc_apm_callbacks_t *callbacks,
                                                   void *context);

void
mongoc_log_and_monitor_instance_set_structured_log_opts (mongoc_log_and_monitor_instance_t *instance,
                                                         const struct mongoc_structured_log_opts_t *opts);


#endif /* MONGOC_LOG_AND_MONITOR_PRIVATE_H */
