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

#include <mongoc/mongoc-structured-log.h>

#include <common-atomic-private.h>
#include <common-json-private.h>
#include <common-oid-private.h>
#include <common-string-private.h>
#include <common-thread-private.h>
#include <mongoc/mongoc-apm-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-structured-log-private.h>
#include <mongoc/mongoc-topology-private.h>
#include <mongoc/mongoc-util-private.h>

#include <mlib/intencode.h>
#include <mlib/loop.h>

#define STRUCTURED_LOG_COMPONENT_TABLE_SIZE (1 + (size_t) MONGOC_STRUCTURED_LOG_COMPONENT_CONNECTION)

// Environment variables with default level for each log component
static const char *gStructuredLogComponentEnvVars[] = {
   "MONGODB_LOG_COMMAND", "MONGODB_LOG_TOPOLOGY", "MONGODB_LOG_SERVER_SELECTION", "MONGODB_LOG_CONNECTION"};

// Canonical names for log components
static const char *gStructuredLogComponentNames[] = {"command", "topology", "serverSelection", "connection"};

// Canonical names for log levels
static const char *gStructuredLogLevelNames[] = {
   "Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Informational", "Debug", "Trace"};

// Additional valid names for log levels
static const struct {
   const char *name;
   mongoc_structured_log_level_t level;
} gStructuredLogLevelAliases[] = {{.name = "off", .level = (mongoc_structured_log_level_t) 0},
                                  {.name = "warn", .level = MONGOC_STRUCTURED_LOG_LEVEL_WARNING},
                                  {.name = "info", .level = MONGOC_STRUCTURED_LOG_LEVEL_INFO}};

// Shared mutable data for the default handler
typedef struct mongoc_structured_log_default_handler_shared_t {
   bson_mutex_t mutex;
   FILE *stream;
   bool stream_fclose_on_destroy;
} mongoc_structured_log_default_handler_shared_t;

struct mongoc_structured_log_opts_t {
   mongoc_structured_log_func_t handler_func;
   void *handler_user_data;
   mongoc_structured_log_level_t max_level_per_component[STRUCTURED_LOG_COMPONENT_TABLE_SIZE];
   uint32_t max_document_length;
   char *default_handler_path;
};

struct mongoc_structured_log_instance_t {
   struct mongoc_structured_log_opts_t opts;                              // Immutable capture of log_opts, func != NULL
   mongoc_structured_log_default_handler_shared_t default_handler_shared; // Inner mutability
};

bson_t *
mongoc_structured_log_entry_message_as_bson (const mongoc_structured_log_entry_t *entry)
{
   BSON_ASSERT_PARAM (entry);
   bson_t *bson = bson_new ();
   BSON_APPEND_UTF8 (bson, "message", entry->envelope.message);
   const mongoc_structured_log_builder_stage_t *stage = entry->builder;
   const mongoc_structured_log_opts_t *opts = &entry->envelope.instance->opts;
   while (stage->func) {
      stage = stage->func (bson, stage, opts);
   }
   return bson;
}

mongoc_structured_log_level_t
mongoc_structured_log_entry_get_level (const mongoc_structured_log_entry_t *entry)
{
   BSON_ASSERT_PARAM (entry);
   return entry->envelope.level;
}

mongoc_structured_log_component_t
mongoc_structured_log_entry_get_component (const mongoc_structured_log_entry_t *entry)
{
   BSON_ASSERT_PARAM (entry);
   return entry->envelope.component;
}

const char *
mongoc_structured_log_entry_get_message_string (const mongoc_structured_log_entry_t *entry)
{
   // Note that 'message' happens to have static lifetime right now (all messages are literals)
   // but our API only guarantees a lifetime that matches 'entry'.
   BSON_ASSERT_PARAM (entry);
   return entry->envelope.message;
}

mongoc_structured_log_level_t
mongoc_structured_log_opts_get_max_level_for_component (const mongoc_structured_log_opts_t *opts,
                                                        mongoc_structured_log_component_t component)
{
   BSON_ASSERT_PARAM (opts);
   unsigned table_index = (unsigned) component;
   if (table_index < STRUCTURED_LOG_COMPONENT_TABLE_SIZE) {
      return opts->max_level_per_component[table_index];
   } else {
      // As documented, unknown component enums return the lowest possible log level.
      return MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY;
   }
}

void
mongoc_structured_log_opts_set_handler (mongoc_structured_log_opts_t *opts,
                                        mongoc_structured_log_func_t log_func,
                                        void *user_data)
{
   BSON_ASSERT_PARAM (opts);
   opts->handler_func = log_func;
   opts->handler_user_data = user_data;
}

void
mongoc_structured_log_get_handler (const mongoc_structured_log_opts_t *opts,
                                   mongoc_structured_log_func_t *log_func,
                                   void **user_data)
{
   BSON_ASSERT_PARAM (opts);
   *log_func = opts->handler_func;
   *user_data = opts->handler_user_data;
}

bool
mongoc_structured_log_opts_set_max_level_for_component (mongoc_structured_log_opts_t *opts,
                                                        mongoc_structured_log_component_t component,
                                                        mongoc_structured_log_level_t level)
{
   BSON_ASSERT_PARAM (opts);
   if (level >= MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY && level <= MONGOC_STRUCTURED_LOG_LEVEL_TRACE) {
      unsigned table_index = (unsigned) component;
      if (table_index < STRUCTURED_LOG_COMPONENT_TABLE_SIZE) {
         opts->max_level_per_component[table_index] = level;
         return true;
      }
   }
   return false;
}

bool
mongoc_structured_log_opts_set_max_level_for_all_components (mongoc_structured_log_opts_t *opts,
                                                             mongoc_structured_log_level_t level)
{
   BSON_ASSERT_PARAM (opts);
   mlib_foreach_urange (component, STRUCTURED_LOG_COMPONENT_TABLE_SIZE) {
      if (!mongoc_structured_log_opts_set_max_level_for_component (
             opts, (mongoc_structured_log_component_t) component, level)) {
         // Fine to stop on the first error, always means 'level' is wrong and none of these will succeed.
         return false;
      }
   }
   return true;
}

bool
_mongoc_structured_log_should_log (const mongoc_structured_log_envelope_t *envelope)
{
   // Note that the instance's max_level_per_component table will also be
   // set to zeroes if logging is disabled. See mongoc_structured_log_instance_new.
   unsigned table_index = (unsigned) envelope->component;
   BSON_ASSERT (table_index < STRUCTURED_LOG_COMPONENT_TABLE_SIZE);
   return envelope->level <= envelope->instance->opts.max_level_per_component[table_index];
}

void
_mongoc_structured_log_with_entry (const mongoc_structured_log_entry_t *entry)
{
   // By now, func is not allowed to be NULL. See mongoc_structured_log_instance_new.
   mongoc_structured_log_instance_t *instance = entry->envelope.instance;
   mongoc_structured_log_func_t func = instance->opts.handler_func;
   BSON_ASSERT (func);
   func (entry, instance->opts.handler_user_data);
}

static bool
_mongoc_structured_log_get_log_level_from_env (const char *variable,
                                               mongoc_structured_log_level_t *out,
                                               int volatile *err_flag_atomic)
{
   char *level = _mongoc_getenv (variable);
   if (!level) {
      return false;
   }
   bool result = mongoc_structured_log_get_named_level (level, out);
   if (!result) {
      // Only log the first instance of each error per process
      if (0 == mcommon_atomic_int_compare_exchange_strong (err_flag_atomic, 0, 1, mcommon_memory_order_seq_cst)) {
         MONGOC_WARNING ("Invalid log level '%s' read from environment variable %s. Ignoring it.", level, variable);
      }
   }
   bson_free (level);
   return result;
}

const char *
mongoc_structured_log_get_level_name (mongoc_structured_log_level_t level)
{
   unsigned table_index = (unsigned) level;
   const size_t table_size = sizeof gStructuredLogLevelNames / sizeof gStructuredLogLevelNames[0];
   return table_index < table_size ? gStructuredLogLevelNames[table_index] : NULL;
}

bool
mongoc_structured_log_get_named_level (const char *name, mongoc_structured_log_level_t *out)
{
   BSON_ASSERT_PARAM (name);
   BSON_ASSERT_PARAM (out);

   // First check canonical names
   {
      const size_t table_size = sizeof gStructuredLogLevelNames / sizeof gStructuredLogLevelNames[0];
      for (unsigned table_index = 0; table_index < table_size; table_index++) {
         if (!strcasecmp (name, gStructuredLogLevelNames[table_index])) {
            *out = (mongoc_structured_log_level_t) table_index;
            return true;
         }
      }
   }
   // Check additional acceptable names
   {
      const size_t table_size = sizeof gStructuredLogLevelAliases / sizeof gStructuredLogLevelAliases[0];
      for (unsigned table_index = 0; table_index < table_size; table_index++) {
         const char *alias = gStructuredLogLevelAliases[table_index].name;
         mongoc_structured_log_level_t level = gStructuredLogLevelAliases[table_index].level;
         if (!strcasecmp (name, alias)) {
            *out = level;
            return true;
         }
      }
   }
   return false;
}

const char *
mongoc_structured_log_get_component_name (mongoc_structured_log_component_t component)
{
   unsigned table_index = (unsigned) component;
   const size_t table_size = sizeof gStructuredLogComponentNames / sizeof gStructuredLogComponentNames[0];
   return table_index < table_size ? gStructuredLogComponentNames[table_index] : NULL;
}

bool
mongoc_structured_log_get_named_component (const char *name, mongoc_structured_log_component_t *out)
{
   BSON_ASSERT_PARAM (name);
   BSON_ASSERT_PARAM (out);

   const size_t table_size = sizeof gStructuredLogComponentNames / sizeof gStructuredLogComponentNames[0];
   for (unsigned table_index = 0; table_index < table_size; table_index++) {
      if (!strcasecmp (name, gStructuredLogComponentNames[table_index])) {
         *out = (mongoc_structured_log_component_t) table_index;
         return true;
      }
   }
   return false;
}

size_t
mongoc_structured_log_opts_get_max_document_length (const mongoc_structured_log_opts_t *opts)
{
   return (size_t) opts->max_document_length;
}

bool
mongoc_structured_log_opts_set_max_document_length (mongoc_structured_log_opts_t *opts, size_t max_document_length)
{
   if (max_document_length <= (size_t) MONGOC_STRUCTURED_LOG_MAXIMUM_MAX_DOCUMENT_LENGTH) {
      opts->max_document_length = (uint32_t) max_document_length;
      return true;
   } else {
      return false;
   }
}

bool
mongoc_structured_log_opts_set_max_document_length_from_env (mongoc_structured_log_opts_t *opts)
{
   BSON_ASSERT_PARAM (opts);

   const char *variable = "MONGODB_LOG_MAX_DOCUMENT_LENGTH";
   char *max_length_str = _mongoc_getenv (variable);

   if (!max_length_str) {
      return true;
   }

   bool result = false;

   if (!strcasecmp (max_length_str, "unlimited")) {
      BSON_ASSERT (
         mongoc_structured_log_opts_set_max_document_length (opts, MONGOC_STRUCTURED_LOG_MAXIMUM_MAX_DOCUMENT_LENGTH));
      result = true;
   } else {
      char *endptr;
      long int_value = strtol (max_length_str, &endptr, 10);
      if (int_value >= 0 && endptr != max_length_str && *endptr == '\0' && mlib_in_range (size_t, int_value) &&
          mongoc_structured_log_opts_set_max_document_length (opts, (size_t) int_value)) {
         result = true;
      }
   }

   if (!result) {
      // Only log the first instance of each error per process
      static int err_flag_atomic;
      if (0 == mcommon_atomic_int_compare_exchange_strong (&err_flag_atomic, 0, 1, mcommon_memory_order_seq_cst)) {
         MONGOC_WARNING (
            "Invalid length '%s' read from environment variable %s. Ignoring it.", max_length_str, variable);
      }
   }

   bson_free (max_length_str);
   return result;
}

bool
mongoc_structured_log_opts_set_max_levels_from_env (mongoc_structured_log_opts_t *opts)
{
   BSON_ASSERT_PARAM (opts);

   bool all_ok = true;
   mongoc_structured_log_level_t level;

   // Errors are not fatal by default; always reported by return value, and reported the first time only via a log
   // warning.
   static int err_flag_all_atomic;
   static int err_flag_per_component_atomic[STRUCTURED_LOG_COMPONENT_TABLE_SIZE];

   if (_mongoc_structured_log_get_log_level_from_env ("MONGODB_LOG_ALL", &level, &err_flag_all_atomic)) {
      BSON_ASSERT (mongoc_structured_log_opts_set_max_level_for_all_components (opts, level));
   } else {
      all_ok = false;
   }

   mlib_foreach_urange (component, STRUCTURED_LOG_COMPONENT_TABLE_SIZE) {
      if (_mongoc_structured_log_get_log_level_from_env (
             gStructuredLogComponentEnvVars[component], &level, &err_flag_per_component_atomic[component])) {
         BSON_ASSERT (mongoc_structured_log_opts_set_max_level_for_component (
            opts, (mongoc_structured_log_component_t) component, level));
      } else {
         all_ok = false;
      }
   }

   return all_ok;
}

static void
_mongoc_structured_log_default_handler_open_stream (mongoc_structured_log_default_handler_shared_t *shared,
                                                    const char *path)
{
   // shared->mutex must already be locked

   if (!path || !strcasecmp (path, "stderr")) {
      // Default or explicit stderr
      shared->stream = stderr;
      shared->stream_fclose_on_destroy = false;
   } else if (!strcasecmp (path, "stdout")) {
      shared->stream = stdout;
      shared->stream_fclose_on_destroy = false;
   } else {
      FILE *file = fopen (path, "a");
      if (file) {
         shared->stream = file;
         shared->stream_fclose_on_destroy = true;
      } else {
         char errmsg_buf[BSON_ERROR_BUFFER_SIZE];
         const char *errmsg = bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf);
         MONGOC_WARNING ("Failed to open log file '%s' with error: '%s'. Logging to stderr instead.", path, errmsg);
         shared->stream = stderr;
         shared->stream_fclose_on_destroy = false;
      }
   }
}

static FILE *
_mongoc_structured_log_default_handler_get_stream (mongoc_structured_log_instance_t *instance)
{
   // instance->default_handler_shared->mutex must already be locked
   {
      FILE *log_stream = instance->default_handler_shared.stream;
      if (log_stream) {
         return log_stream;
      }
   }
   _mongoc_structured_log_default_handler_open_stream (&instance->default_handler_shared,
                                                       instance->opts.default_handler_path);
   {
      FILE *log_stream = instance->default_handler_shared.stream;
      BSON_ASSERT (log_stream);
      return log_stream;
   }
}

static void
_mongoc_structured_log_default_handler (const mongoc_structured_log_entry_t *entry, void *user_data)
{
   BSON_UNUSED (user_data);
   mongoc_structured_log_instance_t *instance = entry->envelope.instance;

   // We can serialize the message before taking the default_handler_shared mutex
   bson_t *bson_message = mongoc_structured_log_entry_message_as_bson (entry);
   char *json_message = bson_as_relaxed_extended_json (bson_message, NULL);
   bson_destroy (bson_message);

   const char *level_name = mongoc_structured_log_get_level_name (mongoc_structured_log_entry_get_level (entry));
   const char *component_name =
      mongoc_structured_log_get_component_name (mongoc_structured_log_entry_get_component (entry));

   bson_mutex_lock (&instance->default_handler_shared.mutex);
   fprintf (_mongoc_structured_log_default_handler_get_stream (instance),
            "MONGODB_LOG %s %s %s\n",
            level_name,
            component_name,
            json_message);
   bson_mutex_unlock (&instance->default_handler_shared.mutex);

   bson_free (json_message);
}

static void
_mongoc_structured_log_no_handler (const mongoc_structured_log_entry_t *entry, void *user_data)
{
   // Stub, for when logging is disabled. Only possible to call at MONGOC_STRUCTURED_LOG_LEVEL_EMERGENCY.
   BSON_UNUSED (entry);
   BSON_UNUSED (user_data);
}

mongoc_structured_log_opts_t *
mongoc_structured_log_opts_new (void)
{
   mongoc_structured_log_opts_t *opts = (mongoc_structured_log_opts_t *) bson_malloc0 (sizeof *opts);

   /* Capture default state from the environment now.
    * Note that error return values from mongoc_structured_log_opts_set_* must be ignored here.
    * If environment variables can't be parsed, warnings will be logged once but we must, by specification,
    * continue to provide structured logging using whatever valid or default settings remain. */
   opts->default_handler_path = _mongoc_getenv ("MONGODB_LOG_PATH");
   opts->max_document_length = MONGOC_STRUCTURED_LOG_DEFAULT_MAX_DOCUMENT_LENGTH;
   (void) mongoc_structured_log_opts_set_max_document_length_from_env (opts);
   (void) mongoc_structured_log_opts_set_max_level_for_all_components (opts, MONGOC_STRUCTURED_LOG_DEFAULT_LEVEL);
   (void) mongoc_structured_log_opts_set_max_levels_from_env (opts);

   // Set default handler. Its shared state is allocated later, as part of instance_t.
   mongoc_structured_log_opts_set_handler (opts, _mongoc_structured_log_default_handler, NULL);

   return opts;
}

void
mongoc_structured_log_opts_destroy (mongoc_structured_log_opts_t *opts)
{
   if (opts) {
      bson_free (opts->default_handler_path);
      bson_free (opts);
   }
}

mongoc_structured_log_instance_t *
mongoc_structured_log_instance_new (const mongoc_structured_log_opts_t *opts)
{
   /* Creating the instance captures an immutable copy of the options.
    * We also make a transformation that simplifies the critical path in
    * _mongoc_structured_log_should_log so that it only needs to check the
    * per-component table: In the instance, NULL handlers are no longer
    * allowed. If structured logging is disabled, the per-component table
    * will be set to the lowest possible levels and a stub handler function
    * is set in case of 'emergency' logs.
    *
    * 'opts' is optional; if NULL, structured logging is disabled.
    * (To request default options, you still need to use
    * mongoc_structured_log_opts_new) */

   mongoc_structured_log_instance_t *instance = (mongoc_structured_log_instance_t *) bson_malloc0 (sizeof *instance);
   bson_mutex_init (&instance->default_handler_shared.mutex);

   if (opts) {
      instance->opts.default_handler_path = bson_strdup (opts->default_handler_path);
      instance->opts.max_document_length = opts->max_document_length;
      instance->opts.handler_func = opts->handler_func;
      instance->opts.handler_user_data = opts->handler_user_data;
   }
   if (instance->opts.handler_func) {
      if (opts) {
         memcpy (instance->opts.max_level_per_component,
                 opts->max_level_per_component,
                 sizeof instance->opts.max_level_per_component);
      }
   } else {
      // No handler; leave the max_level_per_component table zero'ed, and add a stub handler for emergency level only.
      instance->opts.handler_func = _mongoc_structured_log_no_handler;
   }

   return instance;
}

void
mongoc_structured_log_instance_destroy (mongoc_structured_log_instance_t *instance)
{
   if (instance) {
      bson_mutex_destroy (&instance->default_handler_shared.mutex);
      bson_free (instance->opts.default_handler_path);
      if (instance->default_handler_shared.stream_fclose_on_destroy) {
         fclose (instance->default_handler_shared.stream);
      }
      bson_free (instance);
   }
}

static mcommon_string_t *
_mongoc_structured_log_append_json_truncation_marker (mcommon_string_append_t *append)
{
   if (!mcommon_string_status_from_append (append)) {
      mcommon_string_append_t marker_append;
      mcommon_string_set_append (mcommon_string_from_append (append), &marker_append);
      mcommon_string_append (&marker_append, "...");
   }
   // Guaranteed due to choice of MONGOC_STRUCTURED_LOG_MAXIMUM_MAX_DOCUMENT_LENGTH
   BSON_ASSERT (mcommon_strlen_from_append (append) <= (uint32_t) BSON_MAX_SIZE);
   return mcommon_string_from_append (append);
}

// Generic bson-to-json for documents that appear within a structured log message as truncated JSON
static mcommon_string_t *
_mongoc_structured_log_document_as_truncated_json (const bson_t *document, const mongoc_structured_log_opts_t *opts)
{
   // Use the bson_t document length as an initial buffer capacity guess
   mcommon_string_append_t append;
   mcommon_string_set_append_with_limit (
      mcommon_string_new_with_capacity ("", 0, document->len), &append, opts->max_document_length);

   if (mcommon_json_append_bson_document (&append, document, BSON_JSON_MODE_RELAXED, BSON_MAX_RECURSION)) {
      return _mongoc_structured_log_append_json_truncation_marker (&append);
   } else {
      mcommon_string_from_append_destroy (&append);
      return NULL;
   }
}

/**
 * @brief Specialized bson-to-json conversion for mongoc_cmd_t
 * @returns A new allocated mcommon_string_t, limited to the maximum document length from 'opts' plus the space for a
 * possible truncation marker. Returns NULL if an invalid BSON document is encountered. This is equivalent to
 * _mongoc_cmd_append_payload_as_array() combined with _mongoc_structured_log_document_as_truncated_json(), but it
 * avoids ever assembling a BSON representation of the complete logged JSON document. Each payload is serialized
 * separately using the mcommon_json_* functions. If we reach the maximum document length, unused portions of the input
 * command will not be read.
 */
static mcommon_string_t *
_mongoc_structured_log_command_with_payloads_as_truncated_json (const mongoc_cmd_t *cmd,
                                                                const mongoc_structured_log_opts_t *opts)
{
   BSON_ASSERT_PARAM (cmd);
   BSON_ASSERT (!bson_empty0 (cmd->command));
   BSON_ASSERT (cmd->payloads_count <= MONGOC_CMD_PAYLOADS_COUNT_MAX);

   // Use the bson length of the command itself as an initial buffer capacity guess.
   bool invalid_document = false;
   mcommon_string_append_t append;
   mcommon_string_set_append_with_limit (
      mcommon_string_new_with_capacity ("", 0, cmd->command->len), &append, opts->max_document_length);

   if (!mcommon_string_append (&append, "{ ")) {
      goto done;
   }

   if (!mcommon_json_append_bson_values (
          &append, cmd->command, BSON_JSON_MODE_RELAXED, true, BSON_MAX_RECURSION - 1u)) {
      invalid_document = true;
      goto done;
   }

   for (size_t i = 0; i < cmd->payloads_count; i++) {
      const char *field_name = cmd->payloads[i].identifier;
      BSON_ASSERT (field_name);

      // Each payload is an appended key containing a non-empty sequence of documents
      if (!mcommon_json_append_separator (&append) ||
          !mcommon_json_append_key (&append, field_name, strlen (field_name)) ||
          !mcommon_string_append (&append, "[ ")) {
         goto done;
      }

      const uint8_t *doc_begin = cmd->payloads[i].documents;
      BSON_ASSERT (doc_begin);
      const uint8_t *doc_end = doc_begin + cmd->payloads[i].size;
      BSON_ASSERT (doc_begin != doc_end);

      const uint8_t *doc_ptr = doc_begin;

      while (doc_ptr + sizeof (int32_t) <= doc_end) {
         const int32_t doc_len = mlib_read_i32le (doc_ptr);

         bson_t doc;
         if (doc_len < 5 || (size_t) doc_len > (size_t) (doc_end - doc_ptr) ||
             !bson_init_static (&doc, doc_ptr, (size_t) doc_len)) {
            invalid_document = true;
            goto done;
         }

         if (doc_ptr != doc_begin) {
            mcommon_json_append_separator (&append);
         }
         if (!mcommon_json_append_bson_document (&append, &doc, BSON_JSON_MODE_RELAXED, BSON_MAX_RECURSION - 2u)) {
            invalid_document = true;
            goto done;
         }
         doc_ptr += doc_len;
      }
      if (doc_ptr != doc_end) {
         invalid_document = true;
         goto done;
      }

      if (!mcommon_string_append (&append, " ]")) {
         goto done;
      }
   }

   mcommon_string_append (&append, " }");

done:
   if (invalid_document) {
      mcommon_string_from_append_destroy (&append);
      return NULL;
   } else {
      return _mongoc_structured_log_append_json_truncation_marker (&append);
   }
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_utf8 (bson_t *bson,
                                    const mongoc_structured_log_builder_stage_t *stage,
                                    const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   if (key_or_null) {
      bson_append_utf8 (bson, key_or_null, -1, stage->arg2.utf8, -1);
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_utf8_n_stage0 (bson_t *bson,
                                             const mongoc_structured_log_builder_stage_t *stage,
                                             const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   BSON_ASSERT (stage[1].func == _mongoc_structured_log_append_utf8_n_stage1);
   const char *key_or_null = stage[0].arg1.utf8;
   int32_t key_len = stage[0].arg2.int32;
   const char *value = stage[1].arg1.utf8;
   int32_t value_len = stage[1].arg2.int32;
   if (key_or_null) {
      bson_append_utf8 (bson, key_or_null, key_len, value, value_len);
   }
   return stage + 2;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_utf8_n_stage1 (bson_t *bson,
                                             const mongoc_structured_log_builder_stage_t *stage,
                                             const mongoc_structured_log_opts_t *opts)
{
   // Never called, marks the second stage in a two-stage utf8_n
   BSON_UNUSED (bson);
   BSON_UNUSED (stage);
   BSON_UNUSED (opts);
   BSON_ASSERT (false);
   return NULL;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_int32 (bson_t *bson,
                                     const mongoc_structured_log_builder_stage_t *stage,
                                     const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   if (key_or_null) {
      bson_append_int32 (bson, key_or_null, -1, stage->arg2.int32);
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_int64 (bson_t *bson,
                                     const mongoc_structured_log_builder_stage_t *stage,
                                     const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   if (key_or_null) {
      bson_append_int64 (bson, key_or_null, -1, stage->arg2.int64);
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_double (bson_t *bson,
                                      const mongoc_structured_log_builder_stage_t *stage,
                                      const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   if (key_or_null) {
      bson_append_double (bson, key_or_null, -1, stage->arg2.double_value);
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_boolean (bson_t *bson,
                                       const mongoc_structured_log_builder_stage_t *stage,
                                       const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   if (key_or_null) {
      bson_append_bool (bson, key_or_null, -1, stage->arg2.boolean);
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_oid (bson_t *bson,
                                   const mongoc_structured_log_builder_stage_t *stage,
                                   const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   const bson_oid_t *oid_or_null = stage->arg2.oid;
   if (key_or_null) {
      if (oid_or_null) {
         bson_append_oid (bson, key_or_null, -1, oid_or_null);
      } else {
         bson_append_null (bson, key_or_null, -1);
      }
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_oid_as_hex (bson_t *bson,
                                          const mongoc_structured_log_builder_stage_t *stage,
                                          const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   const bson_oid_t *oid_or_null = stage->arg2.oid;
   if (key_or_null) {
      if (oid_or_null) {
         char str[25];
         bson_oid_to_string (oid_or_null, str);
         bson_append_utf8 (bson, key_or_null, -1, str, 24);
      } else {
         bson_append_null (bson, key_or_null, -1);
      }
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_bson_as_json (bson_t *bson,
                                            const mongoc_structured_log_builder_stage_t *stage,
                                            const mongoc_structured_log_opts_t *opts)
{
   const char *key_or_null = stage->arg1.utf8;
   const bson_t *bson_or_null = stage->arg2.bson;
   if (key_or_null) {
      if (bson_or_null) {
         mcommon_string_t *json = _mongoc_structured_log_document_as_truncated_json (bson_or_null, opts);
         if (json) {
            BSON_ASSERT (json->len <= (uint32_t) INT_MAX);
            bson_append_utf8 (bson, key_or_null, -1, json->str, (int) json->len);
            mcommon_string_destroy (json);
         }
         // If invalid BSON was found in the input, the key is not logged.
      } else {
         bson_append_null (bson, key_or_null, -1);
      }
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_cmd (bson_t *bson,
                                   const mongoc_structured_log_builder_stage_t *stage,
                                   const mongoc_structured_log_opts_t *opts)
{
   const mongoc_cmd_t *cmd = stage->arg1.cmd;
   const mongoc_structured_log_cmd_content_flags_t flags = stage->arg2.cmd_flags;
   BSON_ASSERT (cmd);

   if (flags & MONGOC_STRUCTURED_LOG_CMD_CONTENT_FLAG_DATABASE_NAME) {
      BSON_APPEND_UTF8 (bson, "databaseName", cmd->db_name);
   }
   if (flags & MONGOC_STRUCTURED_LOG_CMD_CONTENT_FLAG_COMMAND_NAME) {
      BSON_APPEND_UTF8 (bson, "commandName", cmd->command_name);
   }
   if (flags & MONGOC_STRUCTURED_LOG_CMD_CONTENT_FLAG_OPERATION_ID) {
      BSON_APPEND_INT64 (bson, "operationId", cmd->operation_id);
   }
   if (flags & MONGOC_STRUCTURED_LOG_CMD_CONTENT_FLAG_COMMAND) {
      if (mongoc_apm_is_sensitive_command_message (cmd->command_name, cmd->command)) {
         BSON_APPEND_UTF8 (bson, "command", "{}");
      } else {
         mcommon_string_t *json = _mongoc_structured_log_command_with_payloads_as_truncated_json (cmd, opts);
         if (json) {
            const char *key = "command";
            BSON_ASSERT (json->len <= (uint32_t) INT_MAX);
            bson_append_utf8 (bson, key, strlen (key), json->str, (int) json->len);
            mcommon_string_destroy (json);
         }
         // If invalid BSON was found in the input, the key is not logged.
      }
   }

   return stage + 1;
}

static void
_mongoc_structured_log_append_redacted_cmd_reply (bson_t *bson,
                                                  bool is_sensitive,
                                                  const bson_t *reply,
                                                  const mongoc_structured_log_opts_t *opts)
{
   if (is_sensitive) {
      BSON_APPEND_UTF8 (bson, "reply", "{}");
   } else {
      mcommon_string_t *json = _mongoc_structured_log_document_as_truncated_json (reply, opts);
      if (json) {
         const char *key = "reply";
         BSON_ASSERT (json->len <= (uint32_t) INT_MAX);
         bson_append_utf8 (bson, key, strlen (key), json->str, (int) json->len);
         mcommon_string_destroy (json);
      }
      // If invalid BSON was found in the input, the key is not logged.
   }
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_cmd_reply (bson_t *bson,
                                         const mongoc_structured_log_builder_stage_t *stage,
                                         const mongoc_structured_log_opts_t *opts)
{
   const mongoc_cmd_t *cmd = stage->arg1.cmd;
   const bson_t *reply = stage->arg2.bson;

   BSON_ASSERT (cmd);
   BSON_ASSERT (reply);

   bool is_sensitive = mongoc_apm_is_sensitive_command_message (cmd->command_name, cmd->command) ||
                       mongoc_apm_is_sensitive_command_message (cmd->command_name, reply);
   _mongoc_structured_log_append_redacted_cmd_reply (bson, is_sensitive, reply, opts);
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_cmd_name_reply (bson_t *bson,
                                              const mongoc_structured_log_builder_stage_t *stage,
                                              const mongoc_structured_log_opts_t *opts)
{
   const char *cmd_name = stage->arg1.utf8;
   const bson_t *reply = stage->arg2.bson;

   BSON_ASSERT (cmd_name);
   BSON_ASSERT (reply);

   bool is_sensitive = mongoc_apm_is_sensitive_command_message (cmd_name, reply);
   _mongoc_structured_log_append_redacted_cmd_reply (bson, is_sensitive, reply, opts);
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_error (bson_t *bson,
                                     const mongoc_structured_log_builder_stage_t *stage,
                                     const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   const bson_error_t *error_or_null = stage->arg2.error;
   if (key_or_null) {
      if (error_or_null) {
         bson_t child;
         if (BSON_APPEND_DOCUMENT_BEGIN (bson, key_or_null, &child)) {
            mongoc_error_append_contents_to_bson (error_or_null,
                                                  &child,
                                                  MONGOC_ERROR_CONTENT_FLAG_MESSAGE | MONGOC_ERROR_CONTENT_FLAG_CODE |
                                                     MONGOC_ERROR_CONTENT_FLAG_DOMAIN);
            bson_append_document_end (bson, &child);
         }
      } else {
         bson_append_null (bson, key_or_null, -1);
      }
   }
   return stage + 1;
}

static void
_mongoc_structured_log_append_redacted_cmd_failure (bson_t *bson,
                                                    bool is_sensitive,
                                                    const bson_t *reply,
                                                    const bson_error_t *error)
{
   bool is_server_side = error->domain == MONGOC_ERROR_SERVER || error->domain == MONGOC_ERROR_WRITE_CONCERN_ERROR;
   if (is_server_side) {
      if (is_sensitive) {
         // Redacted server-side message, must be a document with at most 'code', 'codeName', 'errorLabels'
         bson_t failure;
         bson_iter_t iter;
         if (BSON_APPEND_DOCUMENT_BEGIN (bson, "failure", &failure)) {
            bson_iter_init (&iter, reply);
            while (bson_iter_next (&iter)) {
               const char *key = bson_iter_key (&iter);
               if (!strcmp (key, "code") || !strcmp (key, "codeName") || !strcmp (key, "errorLabels")) {
                  bson_append_iter (&failure, key, bson_iter_key_len (&iter), &iter);
               }
            }
            bson_append_document_end (bson, &failure);
         }
      } else {
         // Non-redacted server side message, pass through
         BSON_APPEND_DOCUMENT (bson, "failure", reply);
      }
   } else {
      /* Client-side errors converted directly from bson_error_t, never redacted.
       * In addition to the bson_error_t fields, client side errors may include errorLabels:
       * https://mongoc.org/libmongoc/current/errors.html#error-labels */
      bson_t failure;
      if (BSON_APPEND_DOCUMENT_BEGIN (bson, "failure", &failure)) {
         mongoc_error_append_contents_to_bson (error,
                                               &failure,
                                               MONGOC_ERROR_CONTENT_FLAG_MESSAGE | MONGOC_ERROR_CONTENT_FLAG_CODE |
                                                  MONGOC_ERROR_CONTENT_FLAG_DOMAIN);
         bson_iter_t iter;
         if (bson_iter_init_find (&iter, reply, "errorLabels")) {
            bson_append_iter (&failure, bson_iter_key (&iter), bson_iter_key_len (&iter), &iter);
         }
         bson_append_document_end (bson, &failure);
      }
   }
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_cmd_failure_stage0 (bson_t *bson,
                                                  const mongoc_structured_log_builder_stage_t *stage,
                                                  const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   BSON_ASSERT (stage[1].func == _mongoc_structured_log_append_cmd_failure_stage1);
   const mongoc_cmd_t *cmd = stage[0].arg1.cmd;
   const bson_t *reply = stage[0].arg2.bson;
   const bson_error_t *error = stage[1].arg1.error;

   BSON_ASSERT (cmd);
   BSON_ASSERT (reply);
   BSON_ASSERT (error);

   bool is_sensitive = mongoc_apm_is_sensitive_command_message (cmd->command_name, cmd->command) ||
                       mongoc_apm_is_sensitive_command_message (cmd->command_name, reply);
   _mongoc_structured_log_append_redacted_cmd_failure (bson, is_sensitive, reply, error);
   return stage + 2;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_cmd_failure_stage1 (bson_t *bson,
                                                  const mongoc_structured_log_builder_stage_t *stage,
                                                  const mongoc_structured_log_opts_t *opts)
{
   // Never called, marks the second stage in a two-stage cmd_failure
   BSON_UNUSED (bson);
   BSON_UNUSED (stage);
   BSON_UNUSED (opts);
   BSON_ASSERT (false);
   return NULL;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_cmd_name_failure_stage0 (bson_t *bson,
                                                       const mongoc_structured_log_builder_stage_t *stage,
                                                       const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   BSON_ASSERT (stage[1].func == _mongoc_structured_log_append_cmd_name_failure_stage1);
   const char *cmd_name = stage[0].arg1.utf8;
   const bson_t *reply = stage[0].arg2.bson;
   const bson_error_t *error = stage[1].arg1.error;

   BSON_ASSERT (cmd_name);
   BSON_ASSERT (reply);
   BSON_ASSERT (error);

   bool is_sensitive = mongoc_apm_is_sensitive_command_message (cmd_name, reply);
   _mongoc_structured_log_append_redacted_cmd_failure (bson, is_sensitive, reply, error);
   return stage + 2;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_cmd_name_failure_stage1 (bson_t *bson,
                                                       const mongoc_structured_log_builder_stage_t *stage,
                                                       const mongoc_structured_log_opts_t *opts)
{
   // Never called, marks the second stage in a two-stage cmd_name_failure
   BSON_UNUSED (bson);
   BSON_UNUSED (stage);
   BSON_UNUSED (opts);
   BSON_ASSERT (false);
   return NULL;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_server_description (bson_t *bson,
                                                  const mongoc_structured_log_builder_stage_t *stage,
                                                  const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const mongoc_server_description_t *sd = stage->arg1.server_description;
   const mongoc_server_description_content_flags_t flags = stage->arg2.server_description_flags;
   mongoc_server_description_append_contents_to_bson (sd, bson, flags);
   return stage + 1;
}

static mcommon_string_t *
_mongoc_structured_log_topology_description_as_json (const mongoc_topology_description_t *td,
                                                     const mongoc_structured_log_opts_t *opts)
{
   const mongoc_topology_description_content_flags_t td_flags =
      MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_TYPE | MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_SET_NAME |
      MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_MAX_ELECTION_ID |
      MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_MAX_SET_VERSION | MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_SERVERS |
      MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_STALE | MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_COMPATIBLE |
      MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_COMPATIBILITY_ERROR |
      MONGOC_TOPOLOGY_DESCRIPTION_CONTENT_FLAG_LOGICAL_SESSION_TIMEOUT_MINUTES;

   const mongoc_server_description_content_flags_t server_flags =
      MONGOC_SERVER_DESCRIPTION_CONTENT_FLAG_TYPE | MONGOC_SERVER_DESCRIPTION_CONTENT_FLAG_ADDRESS;

   bson_t doc = BSON_INITIALIZER;
   mongoc_topology_description_append_contents_to_bson (td, &doc, td_flags, server_flags);
   mcommon_string_t *result = _mongoc_structured_log_document_as_truncated_json (&doc, opts);
   bson_destroy (&doc);
   return result;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_topology_as_description_json (bson_t *bson,
                                                            const mongoc_structured_log_builder_stage_t *stage,
                                                            const mongoc_structured_log_opts_t *opts)
{
   const char *key_or_null = stage->arg1.utf8;
   const mongoc_topology_t *topology_or_null = stage->arg2.topology;
   if (key_or_null) {
      if (topology_or_null) {
         mc_shared_tpld td = mc_tpld_take_ref (topology_or_null);
         mcommon_string_t *json = _mongoc_structured_log_topology_description_as_json (td.ptr, opts);
         if (json) {
            BSON_ASSERT (json->len <= (uint32_t) INT_MAX);
            bson_append_utf8 (bson, key_or_null, -1, json->str, (int) json->len);
            mcommon_string_destroy (json);
         }
         mc_tpld_drop_ref (&td);
      } else {
         bson_append_null (bson, key_or_null, -1);
      }
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_topology_description_as_json (bson_t *bson,
                                                            const mongoc_structured_log_builder_stage_t *stage,
                                                            const mongoc_structured_log_opts_t *opts)
{
   const char *key_or_null = stage->arg1.utf8;
   const mongoc_topology_description_t *topology_description_or_null = stage->arg2.topology_description;
   if (key_or_null) {
      if (topology_description_or_null) {
         mcommon_string_t *json =
            _mongoc_structured_log_topology_description_as_json (topology_description_or_null, opts);
         if (json) {
            BSON_ASSERT (json->len <= (uint32_t) INT_MAX);
            bson_append_utf8 (bson, key_or_null, -1, json->str, (int) json->len);
            mcommon_string_destroy (json);
         }
      } else {
         bson_append_null (bson, key_or_null, -1);
      }
   }
   return stage + 1;
}

const mongoc_structured_log_builder_stage_t *
_mongoc_structured_log_append_read_prefs (bson_t *bson,
                                          const mongoc_structured_log_builder_stage_t *stage,
                                          const mongoc_structured_log_opts_t *opts)
{
   BSON_UNUSED (opts);
   const char *key_or_null = stage->arg1.utf8;
   const mongoc_read_prefs_t *read_prefs = stage->arg2.read_prefs;
   if (key_or_null) {
      bson_t child;
      if (BSON_APPEND_DOCUMENT_BEGIN (bson, key_or_null, &child)) {
         mongoc_read_prefs_append_contents_to_bson (
            read_prefs,
            &child,
            MONGOC_READ_PREFS_CONTENT_FLAG_MODE | MONGOC_READ_PREFS_CONTENT_FLAG_TAGS |
               MONGOC_READ_PREFS_CONTENT_FLAG_MAX_STALENESS_SECONDS | MONGOC_READ_PREFS_CONTENT_FLAG_HEDGE);
         bson_append_document_end (bson, &child);
      }
   }
   return stage + 1;
}
