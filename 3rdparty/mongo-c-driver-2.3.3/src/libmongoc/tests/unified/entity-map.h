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

#ifndef UNIFIED_ENTITY_MAP_H
#define UNIFIED_ENTITY_MAP_H

#include "./test-diagnostics.h"

#include <common-thread-private.h>
#include <mongoc/mongoc-array-private.h>
#include <mongoc/mongoc-topology-description-private.h>

#include <mongoc/mongoc.h>

#include <bson/bson.h>
#include <bsonutil/bson-match.h>

typedef struct _event_t {
   struct _event_t *next;
   const char *type;      // Non-owning
   const char *eventType; // Non-owning
   bson_t *serialized;
   bool is_sensitive_command;
} event_t;

typedef bool(log_filter_func_t)(const mongoc_structured_log_entry_t *entry, void *user_data);

typedef struct _log_message_t {
   struct _log_message_t *next;
   mongoc_structured_log_component_t component;
   mongoc_structured_log_level_t level;
   bson_t *message;
} log_message_t;

typedef struct _log_filter_t {
   struct _log_filter_t *next;
   log_filter_func_t *func;
   void *user_data;
} log_filter_t;

typedef struct _observe_event_t {
   char *type; // Type of event to observe.
} observe_event_t;

typedef struct _store_event_t {
   char *entity_id; // Target entity to store event.
   char *type;      // Type of event to store.
} store_event_t;

typedef struct _entity_t {
   char *id;
   char *type;
   void *value;
   bson_t *ignore_command_monitoring_events;
   bool *observe_sensitive_commands;
   struct _entity_t *next;
   event_t *events;
   bson_mutex_t log_mutex;
   log_message_t *log_messages;
   log_filter_t *log_filters;
   struct _entity_map_t *entity_map; // Parent entity map.
   mongoc_array_t observe_events;    // observe_event_t [N].
   mongoc_array_t store_events;      // store_event_t [N].
   bson_t *lsid;
   char *session_client_id;
} entity_t;

struct _entity_findcursor_t;
typedef struct _entity_findcursor_t entity_findcursor_t;

/* Operations on the entity map enforce:
 * 1. Uniqueness. Attempting to create two entries with the same id is an error.
 * 2. Referential integrity. Attempting to get with an unknown id is an error.
 */
typedef struct _entity_map_t {
   entity_t *entities;
   bool reduced_heartbeat;
} entity_map_t;

entity_map_t *
entity_map_new(void);

void
entity_map_destroy(entity_map_t *em);

/* Creates an entry in the entity map based on what is specified in @bson.
 */
bool
entity_map_create(entity_map_t *em, bson_t *bson, const bson_t *cluster_time_after_initial_data, bson_error_t *error);

/* Steals ownership of changestream. */
bool
entity_map_add_changestream(entity_map_t *em,
                            const char *id,
                            mongoc_change_stream_t *changestream,
                            bson_error_t *error);

/* Steals ownership of cursor. */
bool
entity_map_add_findcursor(
   entity_map_t *em, const char *id, mongoc_cursor_t *cursor, const bson_t *first_result, bson_error_t *error);

/* Steals ownership of td. */
bool
entity_map_add_topology_description(entity_map_t *em,
                                    const char *id,
                                    mongoc_topology_description_t *td,
                                    bson_error_t *error);

/* Copies val */
bool
entity_map_add_bson(entity_map_t *em, const char *id, bson_val_t *val, bson_error_t *error);

bool
entity_map_add_bson_array(entity_map_t *em, const char *id, bson_error_t *error);

/* Steals ownership of value. */
bool
entity_map_add_size_t(entity_map_t *em, const char *id, size_t *value, bson_error_t *error);

/* Returns NULL and sets @error if @id does not map to an entry. */
entity_t *
entity_map_get(entity_map_t *em, const char *id, bson_error_t *error);

/* Implements the 'close' operation. Doesn't fully remove the entity.
 * Returns false and sets @error if @id does not map to an entry, or
 * if the entity type does not support 'close' operations. */
bool
entity_map_close(entity_map_t *em, const char *id, bson_error_t *error);

mongoc_client_t *
entity_map_get_client(entity_map_t *entity_map, const char *id, bson_error_t *error);

mongoc_client_encryption_t *
entity_map_get_client_encryption(entity_map_t *entity_map, const char *id, bson_error_t *error);

mongoc_database_t *
entity_map_get_database(entity_map_t *entity_map, const char *id, bson_error_t *error);

mongoc_collection_t *
entity_map_get_collection(entity_map_t *entity_map, const char *id, bson_error_t *error);

mongoc_change_stream_t *
entity_map_get_changestream(entity_map_t *entity_map, const char *id, bson_error_t *error);

entity_findcursor_t *
entity_map_get_findcursor(entity_map_t *entity_map, const char *id, bson_error_t *error);

mongoc_topology_description_t *
entity_map_get_topology_description(entity_map_t *entity_map, const char *id, bson_error_t *error);

void
entity_findcursor_iterate_until_document_or_error(entity_findcursor_t *cursor,
                                                  const bson_t **document,
                                                  bson_error_t *error,
                                                  const bson_t **error_document);

mongoc_client_session_t *
entity_map_get_session(entity_map_t *entity_map, const char *id, bson_error_t *error);

bson_val_t *
entity_map_get_bson(entity_map_t *entity_map, const char *id, bson_error_t *error);

mongoc_array_t *
entity_map_get_bson_array(entity_map_t *entity_map, const char *id, bson_error_t *error);

size_t *
entity_map_get_size_t(entity_map_t *entity_map, const char *id, bson_error_t *error);

mongoc_gridfs_bucket_t *
entity_map_get_bucket(entity_map_t *entity_map, const char *id, bson_error_t *error);

bool
entity_map_match(
   entity_map_t *em, const bson_val_t *expected, const bson_val_t *actual, bool allow_extra, bson_error_t *error);

char *
event_list_to_string(event_t *events);

bool
entity_map_end_session(entity_map_t *em, char *session_id, bson_error_t *error);

char *
entity_map_get_session_client_id(entity_map_t *em, char *session_id, bson_error_t *error);

void
entity_map_set_reduced_heartbeat(entity_map_t *em, bool val);

void
entity_map_disable_event_listeners(entity_map_t *em);

void
entity_log_filter_push(entity_t *entity, log_filter_func_t *func, void *user_data);

void
entity_log_filter_pop(entity_t *entity, log_filter_func_t *func, void *user_data);

void
entity_map_log_filter_push(entity_map_t *entity_map, const char *entity_id, log_filter_func_t *func, void *user_data);

void
entity_map_log_filter_pop(entity_map_t *entity_map, const char *entity_id, log_filter_func_t *func, void *user_data);

#endif /* UNIFIED_ENTITY_MAP_H */
