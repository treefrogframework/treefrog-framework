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

#include <mongoc/mongoc-server-description-private.h>
#include <mongoc/mongoc-structured-log-private.h>
#include <mongoc/mongoc-topology-description-apm-private.h>

/* Application Performance Monitoring for topology events, complies with the
 * SDAM Monitoring Spec:

https://github.com/mongodb/specifications/blob/master/source/server-discovery-and-monitoring/server-discovery-and-monitoring.md

 */

/* ServerOpeningEvent */
void
_mongoc_topology_description_monitor_server_opening (const mongoc_topology_description_t *td,
                                                     const mongoc_log_and_monitor_instance_t *log_and_monitor,
                                                     mongoc_server_description_t *sd)
{
   /* Topology opening will be deferred until server selection, and
    * server opening must be deferred until topology opening. */
   if (td->opened && !sd->opened) {
      sd->opened = true;

      mongoc_structured_log (log_and_monitor->structured_log,
                             MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                             MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                             "Starting server monitoring",
                             oid ("topologyId", &td->topology_id),
                             server_description (sd, SERVER_HOST, SERVER_PORT));

      if (log_and_monitor->apm_callbacks.server_opening) {
         mongoc_apm_server_opening_t event;

         bson_oid_copy (&td->topology_id, &event.topology_id);
         event.host = &sd->host;
         event.context = log_and_monitor->apm_context;
         log_and_monitor->apm_callbacks.server_opening (&event);
      }
   }
}

/* ServerDescriptionChangedEvent */
void
_mongoc_topology_description_monitor_server_changed (const mongoc_topology_description_t *td,
                                                     const mongoc_log_and_monitor_instance_t *log_and_monitor,
                                                     const mongoc_server_description_t *prev_sd,
                                                     const mongoc_server_description_t *new_sd)
{
   if (log_and_monitor->apm_callbacks.server_changed) {
      mongoc_apm_server_changed_t event;

      /* address is same in previous and new sd */
      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.host = &new_sd->host;
      event.previous_description = prev_sd;
      event.new_description = new_sd;
      event.context = log_and_monitor->apm_context;
      log_and_monitor->apm_callbacks.server_changed (&event);
   }
}

/* ServerClosedEvent */
void
_mongoc_topology_description_monitor_server_closed (const mongoc_topology_description_t *td,
                                                    const mongoc_log_and_monitor_instance_t *log_and_monitor,
                                                    const mongoc_server_description_t *sd)
{
   if (!sd->opened) {
      return;
   }

   mongoc_structured_log (log_and_monitor->structured_log,
                          MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                          MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                          "Stopped server monitoring",
                          oid ("topologyId", &td->topology_id),
                          server_description (sd, SERVER_HOST, SERVER_PORT));

   if (log_and_monitor->apm_callbacks.server_closed) {
      mongoc_apm_server_closed_t event;

      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.host = &sd->host;
      event.context = log_and_monitor->apm_context;
      log_and_monitor->apm_callbacks.server_closed (&event);
   }
}


/* Send TopologyOpeningEvent when first called on this topology description.
 * td is not const: we mark it as "opened" by the current log-and-monitor instance. */
void
_mongoc_topology_description_monitor_opening (mongoc_topology_description_t *td,
                                              const mongoc_log_and_monitor_instance_t *log_and_monitor)
{
   if (td->opened) {
      return;
   }
   td->opened = true;

   // The initial 'previous' topology description, with Unknown type
   mongoc_topology_description_t *prev_td = BSON_ALIGNED_ALLOC0 (mongoc_topology_description_t);
   mongoc_topology_description_init (prev_td, td->heartbeat_msec);

   mongoc_structured_log (log_and_monitor->structured_log,
                          MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                          MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                          "Starting topology monitoring",
                          oid ("topologyId", &td->topology_id));

   if (log_and_monitor->apm_callbacks.topology_opening) {
      mongoc_apm_topology_opening_t event;

      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.context = log_and_monitor->apm_context;
      log_and_monitor->apm_callbacks.topology_opening (&event);
   }

   /* send initial description-changed event */
   _mongoc_topology_description_monitor_changed (prev_td, td, log_and_monitor);

   for (size_t i = 0u; i < mc_tpld_servers (td)->items_len; i++) {
      mongoc_server_description_t *sd = mongoc_set_get_item (mc_tpld_servers (td), i);
      _mongoc_topology_description_monitor_server_opening (td, log_and_monitor, sd);
   }

   /* If this is a load balanced topology:
    * - update the one server description to be LoadBalancer
    * - emit a server changed event Unknown => LoadBalancer
    * - emit a topology changed event
    */
   if (td->type == MONGOC_TOPOLOGY_LOAD_BALANCED) {
      mongoc_server_description_t *prev_sd;

      /* LoadBalanced deployments must have exactly one host listed. Otherwise,
       * an error would have occurred when constructing the topology. */
      BSON_ASSERT (mc_tpld_servers (td)->items_len == 1);
      mongoc_server_description_t *sd = mongoc_set_get_item (mc_tpld_servers (td), 0);
      prev_sd = mongoc_server_description_new_copy (sd);
      BSON_ASSERT (prev_sd);

      mongoc_topology_description_cleanup (prev_td);
      _mongoc_topology_description_copy_to (td, prev_td);

      sd->type = MONGOC_SERVER_LOAD_BALANCER;
      _mongoc_topology_description_monitor_server_changed (td, log_and_monitor, prev_sd, sd);
      mongoc_server_description_destroy (prev_sd);
      _mongoc_topology_description_monitor_changed (prev_td, td, log_and_monitor);
   }

   if (prev_td) {
      mongoc_topology_description_cleanup (prev_td);
      bson_free (prev_td);
   }
}

/* TopologyDescriptionChangedEvent */
void
_mongoc_topology_description_monitor_changed (const mongoc_topology_description_t *prev_td,
                                              const mongoc_topology_description_t *new_td,
                                              const mongoc_log_and_monitor_instance_t *log_and_monitor)
{
   mongoc_structured_log (log_and_monitor->structured_log,
                          MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                          MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                          "Topology description changed",
                          oid ("topologyId", &new_td->topology_id),
                          topology_description_as_json ("previousDescription", prev_td),
                          topology_description_as_json ("newDescription", new_td));

   if (log_and_monitor->apm_callbacks.topology_changed) {
      mongoc_apm_topology_changed_t event;

      /* callbacks, context, and id are the same in previous and new td */
      bson_oid_copy (&new_td->topology_id, &event.topology_id);
      event.context = log_and_monitor->apm_context;
      event.previous_description = prev_td;
      event.new_description = new_td;

      log_and_monitor->apm_callbacks.topology_changed (&event);
   }
}

/* TopologyClosedEvent */
void
_mongoc_topology_description_monitor_closed (const mongoc_topology_description_t *td,
                                             const mongoc_log_and_monitor_instance_t *log_and_monitor)
{
   // Expected preconditions for 'closed' events:
   // (mongoc_topology_destroy() carries out these transitions prior to close of monitoring.)
   BSON_ASSERT (td->type == MONGOC_TOPOLOGY_UNKNOWN);
   BSON_ASSERT (mc_tpld_servers_const (td)->items_len == 0);

   if (!td->opened) {
      return;
   }

   mongoc_structured_log (log_and_monitor->structured_log,
                          MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                          MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                          "Stopped topology monitoring",
                          oid ("topologyId", &td->topology_id));

   if (log_and_monitor->apm_callbacks.topology_closed) {
      mongoc_apm_topology_closed_t event;
      bson_oid_copy (&td->topology_id, &event.topology_id);
      event.context = log_and_monitor->apm_context;
      log_and_monitor->apm_callbacks.topology_closed (&event);
   }
}
