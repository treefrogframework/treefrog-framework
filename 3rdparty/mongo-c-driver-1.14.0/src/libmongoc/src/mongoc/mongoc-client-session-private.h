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

#include "mongoc/mongoc-prelude.h"

#ifndef MONGOC_CLIENT_SESSION_PRIVATE_H
#define MONGOC_CLIENT_SESSION_PRIVATE_H

#include <bson/bson.h>
#include "mongoc/mongoc-client-session.h"

/* error labels: see Transactions Spec */
#define TRANSIENT_TXN_ERR "TransientTransactionError"
#define UNKNOWN_COMMIT_RESULT "UnknownTransactionCommitResult"

#define MONGOC_DEFAULT_WTIMEOUT_FOR_COMMIT_RETRY 10000

struct _mongoc_transaction_opt_t {
   mongoc_read_concern_t *read_concern;
   mongoc_write_concern_t *write_concern;
   mongoc_read_prefs_t *read_prefs;
};

typedef enum {
   MONGOC_SESSION_NO_OPTS = 0,
   MONGOC_SESSION_CAUSAL_CONSISTENCY = (1 << 0),
} mongoc_session_flag_t;

struct _mongoc_session_opt_t {
   mongoc_session_flag_t flags;
   mongoc_transaction_opt_t default_txn_opts;
};

typedef struct _mongoc_server_session_t {
   struct _mongoc_server_session_t *prev, *next;
   int64_t last_used_usec;
   bson_t lsid;        /* logical session id */
   int64_t txn_number; /* transaction number */
} mongoc_server_session_t;

typedef enum {
   MONGOC_TRANSACTION_NONE,
   MONGOC_TRANSACTION_STARTING,
   MONGOC_TRANSACTION_IN_PROGRESS,
   MONGOC_TRANSACTION_ENDING,
   MONGOC_TRANSACTION_COMMITTED,
   MONGOC_TRANSACTION_COMMITTED_EMPTY,
   MONGOC_TRANSACTION_ABORTED,
} mongoc_transaction_state_t;

typedef struct _mongoc_transaction_t {
   mongoc_transaction_state_t state;
   mongoc_transaction_opt_t opts;
} mongoc_transaction_t;

struct _mongoc_client_session_t {
   mongoc_client_t *client;
   mongoc_session_opt_t opts;
   mongoc_server_session_t *server_session;
   mongoc_transaction_t txn;
   uint32_t client_session_id;
   bson_t cluster_time;
   uint32_t operation_timestamp;
   uint32_t operation_increment;
   uint32_t client_generation;
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
                                  const bson_iter_t *iter,
                                  mongoc_client_session_t **cs,
                                  bson_error_t *error);

bool
_mongoc_client_session_in_txn (const mongoc_client_session_t *session);

bool
_mongoc_client_session_txn_in_progress (const mongoc_client_session_t *session);

bool
_mongoc_client_session_append_txn (mongoc_client_session_t *session,
                                   bson_t *cmd,
                                   bson_error_t *error);

void
_mongoc_client_session_append_read_concern (const mongoc_client_session_t *cs,
                                            const bson_t *user_read_concern,
                                            bool is_read_command,
                                            bson_t *cmd);

#endif /* MONGOC_CLIENT_SESSION_PRIVATE_H */
