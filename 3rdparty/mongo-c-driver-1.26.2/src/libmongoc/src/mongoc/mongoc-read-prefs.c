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


#include "mongoc-error.h"
#include "mongoc-read-prefs-private.h"
#include "mongoc-trace-private.h"


mongoc_read_prefs_t *
mongoc_read_prefs_new (mongoc_read_mode_t mode)
{
   mongoc_read_prefs_t *read_prefs;

   read_prefs = BSON_ALIGNED_ALLOC0 (mongoc_read_prefs_t);
   read_prefs->mode = mode;
   bson_init (&read_prefs->tags);
   read_prefs->max_staleness_seconds = MONGOC_NO_MAX_STALENESS;
   bson_init (&read_prefs->hedge);

   return read_prefs;
}


mongoc_read_mode_t
mongoc_read_prefs_get_mode (const mongoc_read_prefs_t *read_prefs)
{
   return read_prefs ? read_prefs->mode : MONGOC_READ_PRIMARY;
}


void
mongoc_read_prefs_set_mode (mongoc_read_prefs_t *read_prefs,
                            mongoc_read_mode_t mode)
{
   BSON_ASSERT (read_prefs);
   BSON_ASSERT (mode <= MONGOC_READ_NEAREST);

   read_prefs->mode = mode;
}


const bson_t *
mongoc_read_prefs_get_tags (const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (read_prefs);
   return &read_prefs->tags;
}


void
mongoc_read_prefs_set_tags (mongoc_read_prefs_t *read_prefs, const bson_t *tags)
{
   BSON_ASSERT (read_prefs);

   bson_destroy (&read_prefs->tags);

   if (tags) {
      bson_copy_to (tags, &read_prefs->tags);
   } else {
      bson_init (&read_prefs->tags);
   }
}


void
mongoc_read_prefs_add_tag (mongoc_read_prefs_t *read_prefs, const bson_t *tag)
{
   bson_t empty = BSON_INITIALIZER;
   char str[16];
   int key;

   BSON_ASSERT (read_prefs);

   key = bson_count_keys (&read_prefs->tags);
   bson_snprintf (str, sizeof str, "%d", key);

   if (tag) {
      bson_append_document (&read_prefs->tags, str, -1, tag);
   } else {
      bson_append_document (&read_prefs->tags, str, -1, &empty);
   }

   bson_destroy (&empty);
}


int64_t
mongoc_read_prefs_get_max_staleness_seconds (
   const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (read_prefs);

   return read_prefs->max_staleness_seconds;
}


void
mongoc_read_prefs_set_max_staleness_seconds (mongoc_read_prefs_t *read_prefs,
                                             int64_t max_staleness_seconds)
{
   BSON_ASSERT (read_prefs);

   read_prefs->max_staleness_seconds = max_staleness_seconds;
}


const bson_t *
mongoc_read_prefs_get_hedge (const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (read_prefs);

   return &read_prefs->hedge;
}


void
mongoc_read_prefs_set_hedge (mongoc_read_prefs_t *read_prefs,
                             const bson_t *hedge)
{
   BSON_ASSERT (read_prefs);

   bson_destroy (&read_prefs->hedge);

   if (hedge) {
      bson_copy_to (hedge, &read_prefs->hedge);
   } else {
      bson_init (&read_prefs->hedge);
   }
}


bool
mongoc_read_prefs_is_valid (const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT (read_prefs);

   /*
    * Tags, maxStalenessSeconds, and hedge are not supported with PRIMARY mode.
    */
   if (read_prefs->mode == MONGOC_READ_PRIMARY) {
      if (!bson_empty (&read_prefs->tags) ||
          read_prefs->max_staleness_seconds != MONGOC_NO_MAX_STALENESS ||
          !bson_empty (&read_prefs->hedge)) {
         return false;
      }
   }

   if (read_prefs->max_staleness_seconds != MONGOC_NO_MAX_STALENESS &&
       read_prefs->max_staleness_seconds <= 0) {
      return false;
   }


   return true;
}


void
mongoc_read_prefs_destroy (mongoc_read_prefs_t *read_prefs)
{
   if (read_prefs) {
      bson_destroy (&read_prefs->tags);
      bson_destroy (&read_prefs->hedge);
      bson_free (read_prefs);
   }
}


mongoc_read_prefs_t *
mongoc_read_prefs_copy (const mongoc_read_prefs_t *read_prefs)
{
   mongoc_read_prefs_t *ret = NULL;

   if (read_prefs) {
      ret = mongoc_read_prefs_new (read_prefs->mode);
      bson_destroy (&ret->tags);
      bson_copy_to (&read_prefs->tags, &ret->tags);
      ret->max_staleness_seconds = read_prefs->max_staleness_seconds;
      bson_destroy (&ret->hedge);
      bson_copy_to (&read_prefs->hedge, &ret->hedge);
   }

   return ret;
}


const char *
_mongoc_read_mode_as_str (mongoc_read_mode_t mode)
{
   switch (mode) {
   case MONGOC_READ_PRIMARY:
      return "primary";
   case MONGOC_READ_PRIMARY_PREFERRED:
      return "primaryPreferred";
   case MONGOC_READ_SECONDARY:
      return "secondary";
   case MONGOC_READ_SECONDARY_PREFERRED:
      return "secondaryPreferred";
   case MONGOC_READ_NEAREST:
      return "nearest";
   default:
      return "";
   }
}


/* Update result with the read prefs, following Server Selection Spec.
 * The driver must have discovered the server is a mongos.
 */
static void
_apply_read_preferences_mongos (
   const mongoc_read_prefs_t *read_prefs,
   const bson_t *query_bson,
   mongoc_assemble_query_result_t *result /* OUT */)
{
   mongoc_read_mode_t mode;
   const bson_t *tags = NULL;
   bson_t child;
   const char *mode_str;
   int64_t max_staleness_seconds = MONGOC_NO_MAX_STALENESS;
   const bson_t *hedge = NULL;

   mode = mongoc_read_prefs_get_mode (read_prefs);
   if (read_prefs) {
      max_staleness_seconds =
         mongoc_read_prefs_get_max_staleness_seconds (read_prefs);

      tags = mongoc_read_prefs_get_tags (read_prefs);
      hedge = mongoc_read_prefs_get_hedge (read_prefs);
   }

   /* Server Selection Spec says:
    *
    * For mode 'primary', drivers MUST NOT set the secondaryOk wire protocol
    * flag and MUST NOT use $readPreference
    *
    * For mode 'secondary', drivers MUST set the secondaryOk wire protocol flag
    * and MUST also use $readPreference
    *
    * For mode 'primaryPreferred', drivers MUST set the secondaryOk wire
    * protocol flag and MUST also use $readPreference
    *
    * For mode 'secondaryPreferred', drivers MUST set the secondaryOk wire
    * protocol flag. If the read preference contains a non-empty tag_sets
    * parameter, maxStalenessSeconds is a positive integer, or the hedge
    * parameter is non-empty, drivers MUST use $readPreference; otherwise,
    * drivers MUST NOT use $readPreference
    *
    * For mode 'nearest', drivers MUST set the secondaryOk wire protocol flag
    * and MUST also use $readPreference
    */
   if (mode == MONGOC_READ_SECONDARY_PREFERRED &&
       (bson_empty0 (tags) && max_staleness_seconds <= 0 &&
        bson_empty0 (hedge))) {
      result->flags |= MONGOC_QUERY_SECONDARY_OK;

   } else if (mode != MONGOC_READ_PRIMARY) {
      result->flags |= MONGOC_QUERY_SECONDARY_OK;

      /* Server Selection Spec: "When any $ modifier is used, including the
       * $readPreference modifier, the query MUST be provided using the $query
       * modifier".
       *
       * This applies to commands, too.
       */
      result->assembled_query = bson_new ();
      result->query_owned = true;

      if (bson_has_field (query_bson, "$query")) {
         bson_concat (result->assembled_query, query_bson);
      } else {
         bson_append_document (
            result->assembled_query, "$query", 6, query_bson);
      }

      bson_append_document_begin (
         result->assembled_query, "$readPreference", 15, &child);
      mode_str = _mongoc_read_mode_as_str (mode);
      bson_append_utf8 (&child, "mode", 4, mode_str, -1);
      if (!bson_empty0 (tags)) {
         bson_append_array (&child, "tags", 4, tags);
      }

      if (max_staleness_seconds != MONGOC_NO_MAX_STALENESS) {
         bson_append_int64 (
            &child, "maxStalenessSeconds", 19, max_staleness_seconds);
      }

      if (!bson_empty0 (hedge)) {
         bson_append_document (&child, "hedge", 5, hedge);
      }

      bson_append_document_end (result->assembled_query, &child);
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * assemble_query --
 *
 *       Update @result based on @read_prefs, following the Server Selection
 *       Spec.
 *
 * Side effects:
 *       Sets @result->assembled_query and @result->flags.
 *
 *  Note:
 *       This function, the mongoc_assemble_query_result_t struct, and all
 *       related functions are only used for find operations with OP_QUERY.
 *       Remove them once we have implemented exhaust cursors with OP_MSG in
 *       the server, and all previous server versions are EOL.
 *
 *--------------------------------------------------------------------------
 */

void
assemble_query (const mongoc_read_prefs_t *read_prefs,
                const mongoc_server_stream_t *server_stream,
                const bson_t *query_bson,
                int32_t initial_flags,
                mongoc_assemble_query_result_t *result /* OUT */)
{
   mongoc_server_description_type_t server_type;

   ENTRY;

   BSON_ASSERT (server_stream);
   BSON_ASSERT (query_bson);
   BSON_ASSERT (result);

   /* default values */
   result->assembled_query = (bson_t *) query_bson;
   result->query_owned = false;
   result->flags = initial_flags;

   server_type = server_stream->sd->type;

   switch (server_stream->topology_type) {
   case MONGOC_TOPOLOGY_SINGLE:
      if (server_type == MONGOC_SERVER_MONGOS) {
         _apply_read_preferences_mongos (read_prefs, query_bson, result);
      } else {
         /* Server Selection Spec: for topology type single and server types
          * besides mongos, "clients MUST always set the secondaryOk wire
          * protocol flag on reads to ensure that any server type can handle the
          * request."
          */
         result->flags |= MONGOC_OP_QUERY_FLAG_SECONDARY_OK;
      }

      break;

   case MONGOC_TOPOLOGY_RS_NO_PRIMARY:
   case MONGOC_TOPOLOGY_RS_WITH_PRIMARY:
      /* Server Selection Spec: for RS topology types, "For all read
       * preferences modes except primary, clients MUST set the secondaryOk wire
       * protocol flag to ensure that any suitable server can handle the
       * request. Clients MUST  NOT set the secondaryOk wire protocol flag if
       * the read preference mode is primary.
       */
      if (read_prefs && read_prefs->mode != MONGOC_READ_PRIMARY) {
         result->flags |= MONGOC_OP_QUERY_FLAG_SECONDARY_OK;
      }

      break;

   case MONGOC_TOPOLOGY_SHARDED:
   case MONGOC_TOPOLOGY_LOAD_BALANCED:
      _apply_read_preferences_mongos (read_prefs, query_bson, result);
      break;

   case MONGOC_TOPOLOGY_UNKNOWN:
   case MONGOC_TOPOLOGY_DESCRIPTION_TYPES:
   default:
      /* must not call _apply_read_preferences with unknown topology type */
      BSON_ASSERT (false);
   }

   EXIT;
}


void
assemble_query_result_cleanup (mongoc_assemble_query_result_t *result)
{
   ENTRY;

   BSON_ASSERT (result);

   if (result->query_owned) {
      bson_destroy (result->assembled_query);
   }

   EXIT;
}

bool
_mongoc_read_prefs_validate (const mongoc_read_prefs_t *read_prefs,
                             bson_error_t *error)
{
   if (read_prefs && !mongoc_read_prefs_is_valid (read_prefs)) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Invalid mongoc_read_prefs_t");
      return false;
   }
   return true;
}
