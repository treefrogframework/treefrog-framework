/*
 * Copyright 2017 MongoDB, Inc.
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

#ifndef MONGOC_CLIENT_SESSION_PRIVATE_H
#define MONGOC_CLIENT_SESSION_PRIVATE_H

#if !defined(MONGOC_INSIDE) && !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>
#include "mongoc-client-session.h"

typedef enum {
   MONGOC_SESSION_NO_OPTS = 0,
   MONGOC_SESSION_CAUSAL_CONSISTENCY = (1 << 0),
} mongoc_session_flag_t;

struct _mongoc_session_opt_t {
   mongoc_session_flag_t flags;
};

typedef struct _mongoc_server_session_t {
   struct _mongoc_server_session_t *prev, *next;
   int64_t last_used_usec;
   bson_t lsid; /* logical session id */
   int64_t txn_number; /* transaction number */
} mongoc_server_session_t;

struct _mongoc_client_session_t {
   mongoc_client_t *client;
   mongoc_session_opt_t opts;
   mongoc_server_session_t *server_session;
   uint32_t client_session_id;
   bson_t cluster_time;
   uint32_t operation_timestamp;
   uint32_t operation_increment;
};

bool
_mongoc_parse_cluster_time (const bson_t *cluster_time,
                            uint32_t *timestamp,
                            uint32_t *increment);

bool
_mongoc_cluster_time_greater (const bson_t *new, const bson_t *old);

void
_mongoc_client_session_handle_reply (mongoc_client_session_t *session,
                                     bool is_acknowledged,
                                     const bson_t *reply);

mongoc_server_session_t *
_mongoc_server_session_new (bson_error_t *error);

bool
_mongoc_server_session_timed_out (const mongoc_server_session_t *server_session,
                                  int64_t session_timeout_minutes);

void
_mongoc_server_session_destroy (mongoc_server_session_t *server_session);

mongoc_client_session_t *
_mongoc_client_session_new (mongoc_client_t *client,
                            mongoc_server_session_t *server_session,
                            const mongoc_session_opt_t *opts,
                            uint32_t client_session_id);

bool
_mongoc_client_session_from_iter (mongoc_client_t *client,
                                  bson_iter_t *iter,
                                  mongoc_client_session_t **cs,
                                  bson_error_t *error);

#endif /* MONGOC_CLIENT_SESSION_PRIVATE_H */
