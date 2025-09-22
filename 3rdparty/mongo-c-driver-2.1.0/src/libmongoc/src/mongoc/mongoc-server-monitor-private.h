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

#ifndef MONGOC_SERVER_MONITOR_PRIVATE_H
#define MONGOC_SERVER_MONITOR_PRIVATE_H

#include <mongoc/mongoc-server-description-private.h>
#include <mongoc/mongoc-topology-private.h>

#include <mongoc/mongoc.h>

/* For background monitoring of a single server. */
typedef enum {
   MONGOC_SERVER_MONITORING_AUTO = 0,
   MONGOC_SERVER_MONITORING_POLL,
   MONGOC_SERVER_MONITORING_STREAM
} mongoc_server_monitoring_mode_t;

typedef struct _mongoc_server_monitor_t mongoc_server_monitor_t;

mongoc_server_monitor_t *
mongoc_server_monitor_new (mongoc_topology_t *topology,
                           mongoc_topology_description_t *td,
                           mongoc_server_description_t *init_description);

void
mongoc_server_monitor_request_cancel (mongoc_server_monitor_t *server_monitor);

void
mongoc_server_monitor_request_scan (mongoc_server_monitor_t *server_monitor);

bool
mongoc_server_monitor_request_shutdown (mongoc_server_monitor_t *server_monitor);

void
mongoc_server_monitor_wait_for_shutdown (mongoc_server_monitor_t *server_monitor);

void
mongoc_server_monitor_destroy (mongoc_server_monitor_t *server_monitor);

void
mongoc_server_monitor_run (mongoc_server_monitor_t *server_monitor);

void
mongoc_server_monitor_run_as_rtt (mongoc_server_monitor_t *server_monitor);

#endif /* MONGOC_SERVER_MONITOR_PRIVATE_H */
