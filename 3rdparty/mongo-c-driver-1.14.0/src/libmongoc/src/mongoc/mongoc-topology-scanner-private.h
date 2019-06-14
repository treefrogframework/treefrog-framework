/*
 * Copyright 2014 MongoDB, Inc.
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

#ifndef MONGOC_TOPOLOGY_SCANNER_PRIVATE_H
#define MONGOC_TOPOLOGY_SCANNER_PRIVATE_H

/* TODO: rename to TOPOLOGY scanner */

#include <bson/bson.h>
#include "mongoc/mongoc-async-private.h"
#include "mongoc/mongoc-async-cmd-private.h"
#include "mongoc/mongoc-handshake-private.h"
#include "mongoc/mongoc-host-list.h"
#include "mongoc/mongoc-apm-private.h"

#ifdef MONGOC_ENABLE_SSL
#include "mongoc/mongoc-ssl.h"
#endif

BSON_BEGIN_DECLS

typedef void (*mongoc_topology_scanner_setup_err_cb_t) (
   uint32_t id, void *data, const bson_error_t *error /* IN */);

typedef void (*mongoc_topology_scanner_cb_t) (
   uint32_t id,
   const bson_t *bson,
   int64_t rtt,
   void *data,
   const bson_error_t *error /* IN */);

struct mongoc_topology_scanner;
struct mongoc_topology_scanner_node;

typedef struct mongoc_topology_scanner_node {
   uint32_t id;
   /* after scanning, this is set to the successful stream if one exists. */
   mongoc_stream_t *stream;

   int64_t timestamp;
   int64_t last_used;
   int64_t last_failed;
   bool has_auth;
   mongoc_host_list_t host;
   struct mongoc_topology_scanner *ts;

   struct mongoc_topology_scanner_node *next;
   struct mongoc_topology_scanner_node *prev;

   bool retired;
   bson_error_t last_error;

   /* the hostname for a node may resolve to multiple DNS results.
    * dns_results has the full list of DNS results, ordered by host preference.
    * successful_dns_result is the most recent successful DNS result.
    */
   struct addrinfo *dns_results;
   struct addrinfo *successful_dns_result;
   int64_t last_dns_cache;

   /* used by single-threaded clients to store negotiated sasl mechanisms on a
    * node. */
   mongoc_handshake_sasl_supported_mechs_t sasl_supported_mechs;
   bool negotiated_sasl_supported_mechs;
} mongoc_topology_scanner_node_t;

typedef struct mongoc_topology_scanner {
   mongoc_async_t *async;
   int64_t connect_timeout_msec;
   mongoc_topology_scanner_node_t *nodes;
   bson_t ismaster_cmd;
   bson_t ismaster_cmd_with_handshake;
   bson_t cluster_time;
   bool handshake_ok_to_send;
   const char *appname;

   mongoc_topology_scanner_setup_err_cb_t setup_err_cb;
   mongoc_topology_scanner_cb_t cb;
   void *cb_data;
   const mongoc_uri_t *uri;
   mongoc_async_cmd_setup_t setup;
   mongoc_stream_initiator_t initiator;
   void *initiator_context;
   bson_error_t error;

#ifdef MONGOC_ENABLE_SSL
   mongoc_ssl_opt_t *ssl_opts;
#endif

   mongoc_apm_callbacks_t apm_callbacks;
   void *apm_context;
   int64_t dns_cache_timeout_ms;
   /* only used by single-threaded clients to negotiate auth mechanisms. */
   bool negotiate_sasl_supported_mechs;
} mongoc_topology_scanner_t;

mongoc_topology_scanner_t *
mongoc_topology_scanner_new (
   const mongoc_uri_t *uri,
   mongoc_topology_scanner_setup_err_cb_t setup_err_cb,
   mongoc_topology_scanner_cb_t cb,
   void *data,
   int64_t connect_timeout_msec);

void
mongoc_topology_scanner_destroy (mongoc_topology_scanner_t *ts);

bool
mongoc_topology_scanner_valid (mongoc_topology_scanner_t *ts);

void
mongoc_topology_scanner_add (mongoc_topology_scanner_t *ts,
                             const mongoc_host_list_t *host,
                             uint32_t id);

void
mongoc_topology_scanner_scan (mongoc_topology_scanner_t *ts, uint32_t id);

void
mongoc_topology_scanner_disconnect (mongoc_topology_scanner_t *scanner);

void
mongoc_topology_scanner_node_retire (mongoc_topology_scanner_node_t *node);

void
mongoc_topology_scanner_node_disconnect (mongoc_topology_scanner_node_t *node,
                                         bool failed);

void
mongoc_topology_scanner_node_destroy (mongoc_topology_scanner_node_t *node,
                                      bool failed);

bool
mongoc_topology_scanner_in_cooldown (mongoc_topology_scanner_t *ts,
                                     int64_t when);

void
mongoc_topology_scanner_start (mongoc_topology_scanner_t *ts,
                               bool obey_cooldown);

void
mongoc_topology_scanner_work (mongoc_topology_scanner_t *ts);

void
_mongoc_topology_scanner_finish (mongoc_topology_scanner_t *ts);

void
mongoc_topology_scanner_get_error (mongoc_topology_scanner_t *ts,
                                   bson_error_t *error);

void
mongoc_topology_scanner_reset (mongoc_topology_scanner_t *ts);

void
mongoc_topology_scanner_node_setup (mongoc_topology_scanner_node_t *node,
                                    bson_error_t *error);

mongoc_topology_scanner_node_t *
mongoc_topology_scanner_get_node (mongoc_topology_scanner_t *ts, uint32_t id);

const bson_t *
_mongoc_topology_scanner_get_ismaster (mongoc_topology_scanner_t *ts);

bool
mongoc_topology_scanner_has_node_for_host (mongoc_topology_scanner_t *ts,
                                           mongoc_host_list_t *host);

void
mongoc_topology_scanner_set_stream_initiator (mongoc_topology_scanner_t *ts,
                                              mongoc_stream_initiator_t si,
                                              void *ctx);
bool
_mongoc_topology_scanner_set_appname (mongoc_topology_scanner_t *ts,
                                      const char *name);
void
_mongoc_topology_scanner_set_cluster_time (mongoc_topology_scanner_t *ts,
                                           const bson_t *cluster_time);

void
_mongoc_topology_scanner_set_dns_cache_timeout (mongoc_topology_scanner_t *ts,
                                                int64_t timeout_ms);

#ifdef MONGOC_ENABLE_SSL
void
mongoc_topology_scanner_set_ssl_opts (mongoc_topology_scanner_t *ts,
                                      mongoc_ssl_opt_t *opts);
#endif

bool
mongoc_topology_scanner_node_in_cooldown (mongoc_topology_scanner_node_t *node,
                                          int64_t when);

/* for testing. */
mongoc_stream_t *
_mongoc_topology_scanner_tcp_initiate (mongoc_async_cmd_t *acmd);

BSON_END_DECLS

#endif /* MONGOC_TOPOLOGY_SCANNER_PRIVATE_H */
