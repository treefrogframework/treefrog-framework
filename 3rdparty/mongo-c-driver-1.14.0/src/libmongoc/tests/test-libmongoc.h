/*
 * Copyright 2013 MongoDB, Inc.
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


#ifndef TEST_LIBMONGOC_H
#define TEST_LIBMONGOC_H


mongoc_database_t *
get_test_database (mongoc_client_t *client);
char *
gen_collection_name (const char *prefix);
mongoc_collection_t *
get_test_collection (mongoc_client_t *client, const char *prefix);
void
capture_logs (bool capture);
void
clear_captured_logs (void);
bool
has_captured_log (mongoc_log_level_t level, const char *msg);
void
assert_all_captured_logs_have_prefix (const char *prefix);
bool
has_captured_logs (void);
void
print_captured_logs (const char *prefix);
int64_t
get_future_timeout_ms (void);
char *
test_framework_getenv (const char *name);
bool
test_framework_getenv_bool (const char *name);
int64_t
test_framework_getenv_int64 (const char *name, int64_t default_value);
char *
test_framework_get_host (void);
uint16_t
test_framework_get_port (void);
char *
test_framework_get_host_and_port (void);
char *
test_framework_get_admin_user (void);
char *
test_framework_get_admin_password (void);
bool
test_framework_get_ssl (void);
char *
test_framework_add_user_password (const char *uri_str,
                                  const char *user,
                                  const char *password);
char *
test_framework_add_user_password_from_env (const char *uri_str);
char *
test_framework_get_uri_str_no_auth (const char *database_name);
char *
test_framework_get_uri_str (void);
char *
test_framework_get_unix_domain_socket_uri_str (void);
char *
test_framework_get_unix_domain_socket_path_escaped (void);
mongoc_uri_t *
test_framework_get_uri (void);
size_t
test_framework_mongos_count (void);
char *
test_framework_replset_name (void);
size_t
test_framework_replset_member_count (void);
size_t
test_framework_data_nodes_count (void);
size_t
test_framework_server_count (void);

#ifdef MONGOC_ENABLE_SSL
const mongoc_ssl_opt_t *
test_framework_get_ssl_opts (void);
#endif
void
test_framework_set_ssl_opts (mongoc_client_t *client);
void
test_framework_set_pool_ssl_opts (mongoc_client_pool_t *pool);
mongoc_client_t *
test_framework_client_new (void);
mongoc_client_pool_t *
test_framework_client_pool_new (void);

bool
test_framework_is_mongos (void);
bool
test_framework_is_replset (void);
bool
test_framework_server_is_secondary (mongoc_client_t *client,
                                    uint32_t server_id);
int64_t
test_framework_session_timeout_minutes (void);
void
test_framework_get_max_wire_version (int64_t *max_version);
bool
test_framework_clustertime_supported (void);
bool
test_framework_max_wire_version_at_least (int version);
int64_t
test_framework_max_write_batch_size (void);

int
test_framework_skip_if_auth (void);
int
test_framework_skip_if_no_auth (void);
int
test_framework_skip_if_no_sessions (void);
int
test_framework_skip_if_no_cluster_time (void);
int
test_framework_skip_if_crypto (void);
int
test_framework_skip_if_no_crypto (void);
int
test_framework_skip_if_mongos (void);
int
test_framework_skip_if_replset (void);
int
test_framework_skip_if_single (void);
int
test_framework_skip_if_windows (void);
int
test_framework_skip_if_no_uds (void); /* skip if no Unix domain socket */
int
test_framework_skip_if_no_txns (void);
int
test_framework_skip_if_not_mongos (void);
int
test_framework_skip_if_not_replset (void);
int
test_framework_skip_if_not_single (void);
int
test_framework_skip_if_offline (void);
int
test_framework_skip_if_slow (void);
int
test_framework_skip_if_slow_or_live (void);
int
test_framework_skip_if_valgrind (void);

#define WIRE_VERSION_CHECK_DECLS(wv)                                  \
   int test_framework_skip_if_max_wire_version_less_than_##wv (void); \
   int test_framework_skip_if_max_wire_version_more_than_##wv (void); \
   int test_framework_skip_if_rs_version_##wv (void);                 \
   int test_framework_skip_if_not_rs_version_##wv (void);

WIRE_VERSION_CHECK_DECLS (3)
WIRE_VERSION_CHECK_DECLS (4)
WIRE_VERSION_CHECK_DECLS (5)
WIRE_VERSION_CHECK_DECLS (6)
WIRE_VERSION_CHECK_DECLS (7)

#undef WIRE_VERSION_CHECK_DECLS

typedef struct _debug_stream_stats_t {
   mongoc_client_t *client;
   int n_destroyed;
   int n_failed;
} debug_stream_stats_t;

void
test_framework_set_debug_stream (mongoc_client_t *client,
                                 debug_stream_stats_t *stats);

typedef int64_t server_version_t;

server_version_t
test_framework_get_server_version (void);
server_version_t
test_framework_str_to_version (const char *version_str);

int
test_framework_skip_if_no_dual_ip_hostname (void);

char *
test_framework_get_compressors (void);

int
test_framework_skip_if_no_compressors (void);

int
test_framework_skip_if_compressors (void);

void
test_framework_resolve_path (const char *path, char *resolved);

#endif
