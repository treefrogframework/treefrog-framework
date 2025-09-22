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

#ifndef MONGOC_TOPOLOGY_DESCRIPTION_PRIVATE_H
#define MONGOC_TOPOLOGY_DESCRIPTION_PRIVATE_H

#include <mongoc/mongoc-apm-private.h>
#include <mongoc/mongoc-array-private.h>
#include <mongoc/mongoc-deprioritized-servers-private.h>
#include <mongoc/mongoc-log-and-monitor-private.h>
#include <mongoc/mongoc-server-description-private.h>
#include <mongoc/mongoc-set-private.h>

#include <mongoc/mongoc-server-description.h>
#include <mongoc/mongoc-topology-description.h>


typedef enum {
   MONGOC_TOPOLOGY_UNKNOWN,
   MONGOC_TOPOLOGY_SHARDED,
   MONGOC_TOPOLOGY_RS_NO_PRIMARY,
   MONGOC_TOPOLOGY_RS_WITH_PRIMARY,
   MONGOC_TOPOLOGY_SINGLE,
   MONGOC_TOPOLOGY_LOAD_BALANCED,
   MONGOC_TOPOLOGY_DESCRIPTION_TYPES
} mongoc_topology_description_type_t;

struct _mongoc_topology_description_t {
   bson_oid_t topology_id;
   mongoc_topology_description_type_t type;
   int64_t heartbeat_msec;
   mongoc_set_t *_servers_;
   char *set_name;
   int64_t max_set_version;
   bson_oid_t max_election_id;
   bson_error_t compatibility_error;
   uint32_t max_server_id;
   int32_t max_hosts; /* srvMaxHosts */
   bool stale;
   bool opened;
   unsigned int rand_seed;

   /* the greatest seen cluster time, for a MongoDB 3.6+ sharded cluster.
    * see Driver Sessions Spec. */
   bson_t cluster_time;

   /* smallest seen logicalSessionTimeoutMinutes, or -1 if any server has no
    * logicalSessionTimeoutMinutes. see Server Discovery and Monitoring Spec */
   int64_t session_timeout_minutes;
};

typedef enum { MONGOC_SS_READ, MONGOC_SS_WRITE, MONGOC_SS_AGGREGATE_WITH_WRITE } mongoc_ss_optype_t;

/**
 * @brief Contextual information for logging during server selection
 *
 * Required to support the "common fields" defined in the Server Selection Logging specification.
 * The 'operation' string is borrowed for the lifetime of the mongoc_ss_log_context_t.
 */
typedef struct _mongoc_ss_log_context_t {
   const char *operation; // Required
   int64_t operation_id;
   bool has_operation_id;
} mongoc_ss_log_context_t;

void
mongoc_topology_description_init (mongoc_topology_description_t *description, int64_t heartbeat_msec);


/**
 * @brief Get a pointer to the set of server descriptions in the topology
 * description.
 */
static BSON_INLINE mongoc_set_t *
mc_tpld_servers (mongoc_topology_description_t *tpld)
{
   BSON_ASSERT_PARAM (tpld);
   return tpld->_servers_;
}

static BSON_INLINE const mongoc_set_t *
mc_tpld_servers_const (const mongoc_topology_description_t *tpld)
{
   BSON_ASSERT_PARAM (tpld);
   return tpld->_servers_;
}

void
_mongoc_topology_description_copy_to (const mongoc_topology_description_t *src, mongoc_topology_description_t *dst);

void
mongoc_topology_description_cleanup (mongoc_topology_description_t *description);

void
mongoc_topology_description_handle_hello (mongoc_topology_description_t *topology,
                                          const mongoc_log_and_monitor_instance_t *log_and_monitor,
                                          uint32_t server_id,
                                          const bson_t *hello_response,
                                          int64_t rtt_msec,
                                          const bson_error_t *error /* IN */);

mongoc_server_description_t const *
mongoc_topology_description_select (const mongoc_topology_description_t *description,
                                    mongoc_ss_optype_t optype,
                                    const mongoc_read_prefs_t *read_pref,
                                    bool *must_use_primary,
                                    const mongoc_deprioritized_servers_t *ds,
                                    int64_t local_threshold_ms);

mongoc_server_description_t *
mongoc_topology_description_server_by_id (mongoc_topology_description_t *description, uint32_t id, bson_error_t *error);

const mongoc_server_description_t *
mongoc_topology_description_server_by_id_const (const mongoc_topology_description_t *description,
                                                uint32_t id,
                                                bson_error_t *error);

int32_t
mongoc_topology_description_lowest_max_wire_version (const mongoc_topology_description_t *td);

bool
mongoc_topology_description_all_sds_have_write_date (const mongoc_topology_description_t *td);

bool
_mongoc_topology_description_validate_max_staleness (const mongoc_topology_description_t *td,
                                                     int64_t max_staleness_seconds,
                                                     bson_error_t *error);

const mongoc_server_description_t *
_mongoc_topology_description_has_primary (const mongoc_topology_description_t *description);

void
mongoc_topology_description_suitable_servers (mongoc_array_t *set, /* OUT */
                                              mongoc_ss_optype_t optype,
                                              const mongoc_topology_description_t *topology,
                                              const mongoc_read_prefs_t *read_pref,
                                              bool *must_use_primary,
                                              const mongoc_deprioritized_servers_t *ds,
                                              int64_t local_threshold_ms);

bool
mongoc_topology_description_has_data_node (const mongoc_topology_description_t *td);

void
mongoc_topology_description_invalidate_server (mongoc_topology_description_t *topology,
                                               const mongoc_log_and_monitor_instance_t *log_and_monitor,
                                               uint32_t id,
                                               const bson_error_t *error /* IN */);

bool
mongoc_topology_description_add_server (mongoc_topology_description_t *topology,
                                        const mongoc_log_and_monitor_instance_t *log_and_monitor,
                                        const char *server,
                                        uint32_t *id /* OUT */);

void
mongoc_topology_description_update_cluster_time (mongoc_topology_description_t *td, const bson_t *reply);

void
mongoc_topology_description_reconcile (mongoc_topology_description_t *td,
                                       const mongoc_log_and_monitor_instance_t *log_and_monitor,
                                       mongoc_host_list_t *host_list);

/**
 * @brief Invalidate open connnections to a server.
 *
 * Pooled clients with open connections will discover the invalidation
 * the next time they fetch a stream to the server.
 *
 * @param td The topology description that will be updated.
 * @param server_id The ID of the server to invalidate.
 * @param service_id A service ID for load-balanced deployments. Use
 * kZeroObjectId if not applicable.
 *
 * @note Not applicable to single-threaded clients, which only maintain a
 * single connection per server and therefore have no connection pool.
 */
void
_mongoc_topology_description_clear_connection_pool (mongoc_topology_description_t *td,
                                                    uint32_t server_id,
                                                    const bson_oid_t *service_id);

void
mongoc_deprioritized_servers_add_if_sharded (mongoc_deprioritized_servers_t *ds,
                                             mongoc_topology_description_type_t topology_type,
                                             const mongoc_server_description_t *sd);

typedef enum {
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_TYPE = (1 << 0),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_SET_NAME = (1 << 1),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_MAX_ELECTION_ID = (1 << 2),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_MAX_SET_VERSION = (1 << 3),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_SERVERS = (1 << 4),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_STALE = (1 << 5),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_COMPATIBLE = (1 << 6),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_COMPATIBILITY_ERROR = (1 << 7),
   MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_LOGICAL_SESSION_TIMEOUT_MINUTES = (1 << 8),
} mongoc_topology_description_content_flags_t;

bool
mongoc_topology_description_append_contents_to_bson (const mongoc_topology_description_t *td,
                                                     bson_t *bson,
                                                     mongoc_topology_description_content_flags_t flags,
                                                     mongoc_server_description_content_flags_t servers_flags);

#endif /* MONGOC_TOPOLOGY_DESCRIPTION_PRIVATE_H */
