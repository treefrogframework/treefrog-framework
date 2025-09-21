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

#ifndef MONGOC_STRUCTURED_LOG_H
#define MONGOC_STRUCTURED_LOG_H

#include <mongoc/mongoc-macros.h>

#include <bson/bson.h>

BSON_BEGIN_DECLS

typedef enum {
   MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY = 0,
   MONGOC_STRUCTURED_LOG_LEVEL_ALERT = 1,
   MONGOC_STRUCTURED_LOG_LEVEL_CRITICAL = 2,
   MONGOC_STRUCTURED_LOG_LEVEL_ERROR = 3,
   MONGOC_STRUCTURED_LOG_LEVEL_WARNING = 4,
   MONGOC_STRUCTURED_LOG_LEVEL_NOTICE = 5,
   MONGOC_STRUCTURED_LOG_LEVEL_INFO = 6,
   MONGOC_STRUCTURED_LOG_LEVEL_DEBUG = 7,
   MONGOC_STRUCTURED_LOG_LEVEL_TRACE = 8,
} mongoc_structured_log_level_t;

typedef enum {
   MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND = 0,
   MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY = 1,
   MONGOC_STRUCTURED_LOG_COMPONENT_SERVER_SELECTION = 2,
   MONGOC_STRUCTURED_LOG_COMPONENT_CONNECTION = 3,
} mongoc_structured_log_component_t;

typedef struct mongoc_structured_log_entry_t mongoc_structured_log_entry_t;

typedef struct mongoc_structured_log_opts_t mongoc_structured_log_opts_t;

typedef void (BSON_CALL *mongoc_structured_log_func_t) (const mongoc_structured_log_entry_t *entry, void *user_data);

MONGOC_EXPORT (mongoc_structured_log_opts_t *)
mongoc_structured_log_opts_new (void);

MONGOC_EXPORT (void)
mongoc_structured_log_opts_destroy (mongoc_structured_log_opts_t *opts);

MONGOC_EXPORT (void)
mongoc_structured_log_opts_set_handler (mongoc_structured_log_opts_t *opts,
                                        mongoc_structured_log_func_t log_func,
                                        void *user_data);

MONGOC_EXPORT (bool)
mongoc_structured_log_opts_set_max_level_for_component (mongoc_structured_log_opts_t *opts,
                                                        mongoc_structured_log_component_t component,
                                                        mongoc_structured_log_level_t level);

MONGOC_EXPORT (bool)
mongoc_structured_log_opts_set_max_level_for_all_components (mongoc_structured_log_opts_t *opts,
                                                             mongoc_structured_log_level_t level);

MONGOC_EXPORT (bool)
mongoc_structured_log_opts_set_max_levels_from_env (mongoc_structured_log_opts_t *opts);

MONGOC_EXPORT (mongoc_structured_log_level_t)
mongoc_structured_log_opts_get_max_level_for_component (const mongoc_structured_log_opts_t *opts,
                                                        mongoc_structured_log_component_t component);

MONGOC_EXPORT (size_t)
mongoc_structured_log_opts_get_max_document_length (const mongoc_structured_log_opts_t *opts);

MONGOC_EXPORT (bool)
mongoc_structured_log_opts_set_max_document_length_from_env (mongoc_structured_log_opts_t *opts);

MONGOC_EXPORT (bool)
mongoc_structured_log_opts_set_max_document_length (mongoc_structured_log_opts_t *opts, size_t max_document_length);

MONGOC_EXPORT (bson_t *)
mongoc_structured_log_entry_message_as_bson (const mongoc_structured_log_entry_t *entry);

MONGOC_EXPORT (mongoc_structured_log_level_t)
mongoc_structured_log_entry_get_level (const mongoc_structured_log_entry_t *entry);

MONGOC_EXPORT (mongoc_structured_log_component_t)
mongoc_structured_log_entry_get_component (const mongoc_structured_log_entry_t *entry);

MONGOC_EXPORT (const char *)
mongoc_structured_log_entry_get_message_string (const mongoc_structured_log_entry_t *entry);

MONGOC_EXPORT (const char *)
mongoc_structured_log_get_level_name (mongoc_structured_log_level_t level);

MONGOC_EXPORT (bool)
mongoc_structured_log_get_named_level (const char *name, mongoc_structured_log_level_t *out);

MONGOC_EXPORT (const char *)
mongoc_structured_log_get_component_name (mongoc_structured_log_component_t component);

MONGOC_EXPORT (bool)
mongoc_structured_log_get_named_component (const char *name, mongoc_structured_log_component_t *out);

BSON_END_DECLS

#endif /* MONGOC_STRUCTURED_LOG_H */
