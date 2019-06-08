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


#include "mongoc/mongoc-cmd-private.h"
#include "mongoc/mongoc-read-prefs-private.h"
#include "mongoc/mongoc-trace-private.h"
#include "mongoc/mongoc-client-private.h"
#include "mongoc/mongoc-read-concern-private.h"
#include "mongoc/mongoc-write-concern-private.h"
/* For strcasecmp on Windows */
#include "mongoc/mongoc-util-private.h"


void
mongoc_cmd_parts_init (mongoc_cmd_parts_t *parts,
                       mongoc_client_t *client,
                       const char *db_name,
                       mongoc_query_flags_t user_query_flags,
                       const bson_t *command_body)
{
   parts->body = command_body;
   parts->user_query_flags = user_query_flags;
   parts->read_prefs = NULL;
   parts->is_read_command = false;
   parts->is_write_command = false;
   parts->prohibit_lsid = false;
   parts->allow_txn_number = MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_UNKNOWN;
   parts->is_retryable_write = false;
   parts->has_temp_session = false;
   parts->client = client;
   bson_init (&parts->read_concern_document);
   bson_init (&parts->write_concern_document);
   bson_init (&parts->extra);
   bson_init (&parts->assembled_body);

   parts->assembled.db_name = db_name;
   parts->assembled.command = NULL;
   parts->assembled.query_flags = MONGOC_QUERY_NONE;
   parts->assembled.payload_identifier = NULL;
   parts->assembled.payload = NULL;
   parts->assembled.session = NULL;
   parts->assembled.is_acknowledged = true;
   parts->assembled.is_txn_finish = false;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_set_session --
 *
 *       Set the client session field.
 *
 * Side effects:
 *       Aborts if the command is assembled or if mongoc_cmd_parts_append_opts
 *       was called before.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cmd_parts_set_session (mongoc_cmd_parts_t *parts,
                              mongoc_client_session_t *cs)
{
   BSON_ASSERT (parts);
   BSON_ASSERT (!parts->assembled.command);
   BSON_ASSERT (!parts->assembled.session);

   parts->assembled.session = cs;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_append_opts --
 *
 *       Take an iterator over user-supplied options document and append the
 *       options to @parts->command_extra, taking the selected server's max
 *       wire version into account.
 *
 * Return:
 *       True if the options were successfully applied. If any options are
 *       invalid, returns false and fills out @error. In that case @parts is
 *       invalid and must not be used.
 *
 * Side effects:
 *       May partly apply options before returning an error.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cmd_parts_append_opts (mongoc_cmd_parts_t *parts,
                              bson_iter_t *iter,
                              int max_wire_version,
                              bson_error_t *error)
{
   mongoc_client_session_t *cs = NULL;
   mongoc_write_concern_t *wc;
   uint32_t len;
   const uint8_t *data;
   bson_t read_concern;

   ENTRY;

   /* not yet assembled */
   BSON_ASSERT (!parts->assembled.command);

   while (bson_iter_next (iter)) {
      if (BSON_ITER_IS_KEY (iter, "collation")) {
         if (max_wire_version < WIRE_VERSION_COLLATION) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                            "The selected server does not support collation");
            RETURN (false);
         }

      } else if (BSON_ITER_IS_KEY (iter, "writeConcern")) {
         wc = _mongoc_write_concern_new_from_iter (iter, error);
         if (!wc) {
            RETURN (false);
         }

         if (!mongoc_cmd_parts_set_write_concern (
                parts, wc, max_wire_version, error)) {
            mongoc_write_concern_destroy (wc);
            RETURN (false);
         }

         mongoc_write_concern_destroy (wc);
         continue;
      } else if (BSON_ITER_IS_KEY (iter, "readConcern")) {
         if (max_wire_version < WIRE_VERSION_READ_CONCERN) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                            "The selected server does not support readConcern");
            RETURN (false);
         }

         if (!BSON_ITER_HOLDS_DOCUMENT (iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_COMMAND,
                            MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                            "Invalid readConcern");
            RETURN (false);
         }

         /* add readConcern later, once we know about causal consistency */
         bson_iter_document (iter, &len, &data);
         BSON_ASSERT (bson_init_static (&read_concern, data, (size_t) len));
         bson_destroy (&parts->read_concern_document);
         bson_copy_to (&read_concern, &parts->read_concern_document);
         continue;
      } else if (BSON_ITER_IS_KEY (iter, "sessionId")) {
         BSON_ASSERT (!parts->assembled.session);

         if (!_mongoc_client_session_from_iter (
                parts->client, iter, &cs, error)) {
            RETURN (false);
         }

         parts->assembled.session = cs;
         continue;
      } else if (BSON_ITER_IS_KEY (iter, "serverId") ||
                 BSON_ITER_IS_KEY (iter, "maxAwaitTimeMS")) {
         continue;
      }

      if (!bson_append_iter (&parts->extra, bson_iter_key (iter), -1, iter)) {
         RETURN (false);
      }
   }

   RETURN (true);
}


#define OPTS_ERR(_code, ...)                                              \
   do {                                                                   \
      bson_set_error (                                                    \
         error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_##_code, __VA_ARGS__); \
      RETURN (false);                                                     \
   } while (0)


/* set readConcern if allowed, otherwise error */
bool
mongoc_cmd_parts_set_read_concern (mongoc_cmd_parts_t *parts,
                                   const mongoc_read_concern_t *rc,
                                   int max_wire_version,
                                   bson_error_t *error)
{
   const char *command_name;

   ENTRY;

   /* In a txn, set read concern in mongoc_cmd_parts_assemble, not here. *
    * Transactions Spec: "The readConcern MUST NOT be inherited from the
    * collection, database, or client associated with the driver method that
    * invokes the first command." */
   if (_mongoc_client_session_in_txn (parts->assembled.session)) {
      RETURN (true);
   }

   command_name = _mongoc_get_command_name (parts->body);

   if (!command_name) {
      OPTS_ERR (COMMAND_INVALID_ARG, "Empty command document");
   }

   if (mongoc_read_concern_is_default (rc)) {
      RETURN (true);
   }

   if (max_wire_version < WIRE_VERSION_READ_CONCERN) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                      "\"%s\" command does not support readConcern with "
                      "wire version %d, wire version %d is required",
                      command_name,
                      max_wire_version,
                      WIRE_VERSION_READ_CONCERN);
      RETURN (false);
   }

   bson_destroy (&parts->read_concern_document);
   bson_copy_to (_mongoc_read_concern_get_bson ((mongoc_read_concern_t *) rc),
                 &parts->read_concern_document);

   RETURN (true);
}


/* set writeConcern if allowed, otherwise ignore - unlike set_read_concern, it's
 * the caller's responsibility to check if writeConcern is supported */
bool
mongoc_cmd_parts_set_write_concern (mongoc_cmd_parts_t *parts,
                                    const mongoc_write_concern_t *wc,
                                    int max_wire_version,
                                    bson_error_t *error)
{
   const char *command_name;
   bool is_fam;
   bool wc_allowed;

   ENTRY;

   if (!wc) {
      RETURN (true);
   }

   command_name = _mongoc_get_command_name (parts->body);

   if (!command_name) {
      OPTS_ERR (COMMAND_INVALID_ARG, "Empty command document");
   }

   is_fam = !strcasecmp (command_name, "findandmodify");

   wc_allowed =
      parts->is_write_command ||
      (is_fam && max_wire_version >= WIRE_VERSION_FAM_WRITE_CONCERN) ||
      (!is_fam && max_wire_version >= WIRE_VERSION_CMD_WRITE_CONCERN);

   if (wc_allowed) {
      parts->assembled.is_acknowledged =
         mongoc_write_concern_is_acknowledged (wc);
      bson_destroy (&parts->write_concern_document);
      bson_copy_to (
         _mongoc_write_concern_get_bson ((mongoc_write_concern_t *) wc),
         &parts->write_concern_document);
   }

   RETURN (true);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_append_read_write --
 *
 *       Append user-supplied options to @parts->command_extra, taking the
 *       selected server's max wire version into account.
 *
 * Return:
 *       True if the options were successfully applied. If any options are
 *       invalid, returns false and fills out @error. In that case @parts is
 *       invalid and must not be used.
 *
 * Side effects:
 *       May partly apply options before returning an error.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cmd_parts_append_read_write (mongoc_cmd_parts_t *parts,
                                    mongoc_read_write_opts_t *rw_opts,
                                    int max_wire_version,
                                    bson_error_t *error)
{
   ENTRY;

   /* not yet assembled */
   BSON_ASSERT (!parts->assembled.command);

   if (!bson_empty (&rw_opts->collation)) {
      if (max_wire_version < WIRE_VERSION_COLLATION) {
         OPTS_ERR (PROTOCOL_BAD_WIRE_VERSION,
                   "The selected server does not support collation");
      }

      if (!bson_append_document (
             &parts->extra, "collation", 9, &rw_opts->collation)) {
         OPTS_ERR (COMMAND_INVALID_ARG, "'opts' with 'collation' is too large");
      }
   }

   if (!mongoc_cmd_parts_set_write_concern (
          parts, rw_opts->writeConcern, max_wire_version, error)) {
      RETURN (false);
   }

   /* process explicit read concern */
   if (!bson_empty (&rw_opts->readConcern)) {
      if (max_wire_version < WIRE_VERSION_READ_CONCERN) {
         OPTS_ERR (PROTOCOL_BAD_WIRE_VERSION,
                   "The selected server does not support readConcern");
      }

      /* save readConcern for later, once we know about causal consistency */
      bson_destroy (&parts->read_concern_document);
      bson_copy_to (&rw_opts->readConcern, &parts->read_concern_document);
   }

   if (rw_opts->client_session) {
      BSON_ASSERT (!parts->assembled.session);
      parts->assembled.session = rw_opts->client_session;
   }

   if (!bson_concat (&parts->extra, &rw_opts->extra)) {
      OPTS_ERR (COMMAND_INVALID_ARG, "'opts' with extra fields is too large");
   }

   RETURN (true);
}

#undef OPTS_ERR

static void
_mongoc_cmd_parts_ensure_copied (mongoc_cmd_parts_t *parts)
{
   if (parts->assembled.command == parts->body) {
      bson_concat (&parts->assembled_body, parts->body);
      bson_concat (&parts->assembled_body, &parts->extra);
      parts->assembled.command = &parts->assembled_body;
   }
}


static void
_mongoc_cmd_parts_add_write_concern (mongoc_cmd_parts_t *parts)
{
   if (!bson_empty (&parts->write_concern_document)) {
      _mongoc_cmd_parts_ensure_copied (parts);
      bson_append_document (&parts->assembled_body,
                            "writeConcern",
                            12,
                            &parts->write_concern_document);
   }
}


/* The server type must be mongos, or message must be OP_MSG. */
static void
_mongoc_cmd_parts_add_read_prefs (bson_t *query,
                                  const mongoc_read_prefs_t *prefs)
{
   bson_t child;
   const char *mode_str;
   const bson_t *tags;
   int64_t stale;

   mode_str = _mongoc_read_mode_as_str (mongoc_read_prefs_get_mode (prefs));
   tags = mongoc_read_prefs_get_tags (prefs);
   stale = mongoc_read_prefs_get_max_staleness_seconds (prefs);

   bson_append_document_begin (query, "$readPreference", 15, &child);
   bson_append_utf8 (&child, "mode", 4, mode_str, -1);
   if (!bson_empty0 (tags)) {
      bson_append_array (&child, "tags", 4, tags);
   }

   if (stale != MONGOC_NO_MAX_STALENESS) {
      bson_append_int64 (&child, "maxStalenessSeconds", 19, stale);
   }

   bson_append_document_end (query, &child);
}


static void
_iter_concat (bson_t *dst, bson_iter_t *iter)
{
   uint32_t len;
   const uint8_t *data;
   bson_t src;

   bson_iter_document (iter, &len, &data);
   BSON_ASSERT (bson_init_static (&src, data, len));
   BSON_ASSERT (bson_concat (dst, &src));
}


/* Update result with the read prefs. Server must be mongos.
 */
static void
_mongoc_cmd_parts_assemble_mongos (mongoc_cmd_parts_t *parts,
                                   const mongoc_server_stream_t *server_stream)
{
   mongoc_read_mode_t mode;
   const bson_t *tags = NULL;
   bool add_read_prefs = false;
   bson_t query;
   bson_iter_t dollar_query;
   bool has_dollar_query = false;
   bool requires_read_concern;
   bool requires_write_concern;

   ENTRY;

   mode = mongoc_read_prefs_get_mode (parts->read_prefs);
   if (parts->read_prefs) {
      tags = mongoc_read_prefs_get_tags (parts->read_prefs);
   }

   /* Server Selection Spec says:
    *
    * For mode 'primary', drivers MUST NOT set the slaveOK wire protocol flag
    *   and MUST NOT use $readPreference
    *
    * For mode 'secondary', drivers MUST set the slaveOK wire protocol flag and
    *   MUST also use $readPreference
    *
    * For mode 'primaryPreferred', drivers MUST set the slaveOK wire protocol
    *   flag and MUST also use $readPreference
    *
    * For mode 'secondaryPreferred', drivers MUST set the slaveOK wire protocol
    *   flag. If the read preference contains a non-empty tag_sets parameter,
    *   drivers MUST use $readPreference; otherwise, drivers MUST NOT use
    *   $readPreference
    *
    * For mode 'nearest', drivers MUST set the slaveOK wire protocol flag and
    *   MUST also use $readPreference
    */
   switch (mode) {
   case MONGOC_READ_PRIMARY:
      break;
   case MONGOC_READ_SECONDARY_PREFERRED:
      if (!bson_empty0 (tags)) {
         add_read_prefs = true;
      }
      parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;
      break;
   case MONGOC_READ_PRIMARY_PREFERRED:
   case MONGOC_READ_SECONDARY:
   case MONGOC_READ_NEAREST:
   default:
      parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;
      add_read_prefs = true;
   }

   requires_read_concern =
      !bson_empty (&parts->read_concern_document) &&
      strcmp (parts->assembled.command_name, "getMore") != 0;

   requires_write_concern = !bson_empty (&parts->write_concern_document);

   if (add_read_prefs) {
      /* produce {$query: {user query, readConcern}, $readPreference: ... } */
      bson_append_document_begin (&parts->assembled_body, "$query", 6, &query);

      if (bson_iter_init_find (&dollar_query, parts->body, "$query")) {
         /* user provided something like {$query: {key: "x"}} */
         has_dollar_query = true;
         _iter_concat (&query, &dollar_query);
      } else {
         bson_concat (&query, parts->body);
      }

      bson_concat (&query, &parts->extra);
      if (requires_read_concern) {
         bson_append_document (
            &query, "readConcern", 11, &parts->read_concern_document);
      }

      if (requires_write_concern) {
         bson_append_document (
            &query, "writeConcern", 12, &parts->write_concern_document);
      }

      bson_append_document_end (&parts->assembled_body, &query);
      _mongoc_cmd_parts_add_read_prefs (&parts->assembled_body,
                                        parts->read_prefs);

      if (has_dollar_query) {
         /* copy anything that isn't in user's $query */
         bson_copy_to_excluding_noinit (
            parts->body, &parts->assembled_body, "$query", NULL);
      }

      parts->assembled.command = &parts->assembled_body;
   } else if (bson_iter_init_find (&dollar_query, parts->body, "$query")) {
      /* user provided $query, we have no read prefs */
      bson_append_document_begin (&parts->assembled_body, "$query", 6, &query);
      _iter_concat (&query, &dollar_query);
      bson_concat (&query, &parts->extra);
      if (requires_read_concern) {
         bson_append_document (
            &query, "readConcern", 11, &parts->read_concern_document);
      }

      if (requires_write_concern) {
         bson_append_document (
            &query, "writeConcern", 12, &parts->write_concern_document);
      }

      bson_append_document_end (&parts->assembled_body, &query);
      /* copy anything that isn't in user's $query */
      bson_copy_to_excluding_noinit (
         parts->body, &parts->assembled_body, "$query", NULL);

      parts->assembled.command = &parts->assembled_body;
   } else {
      if (requires_read_concern) {
         _mongoc_cmd_parts_ensure_copied (parts);
         bson_append_document (&parts->assembled_body,
                               "readConcern",
                               11,
                               &parts->read_concern_document);
      }

      _mongoc_cmd_parts_add_write_concern (parts);
   }

   if (!bson_empty (&parts->extra)) {
      /* if none of the above logic has merged "extra", do it now */
      _mongoc_cmd_parts_ensure_copied (parts);
   }

   EXIT;
}


static void
_mongoc_cmd_parts_assemble_mongod (mongoc_cmd_parts_t *parts,
                                   const mongoc_server_stream_t *server_stream)
{
   ENTRY;

   if (!parts->is_write_command) {
      switch (server_stream->topology_type) {
      case MONGOC_TOPOLOGY_SINGLE:
         /* Server Selection Spec: for topology type single and server types
          * besides mongos, "clients MUST always set the slaveOK wire
          * protocol flag on reads to ensure that any server type can handle
          * the request."
          */
         parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;
         break;

      case MONGOC_TOPOLOGY_RS_NO_PRIMARY:
      case MONGOC_TOPOLOGY_RS_WITH_PRIMARY:
         /* Server Selection Spec: for RS topology types, "For all read
          * preferences modes except primary, clients MUST set the slaveOK wire
          * protocol flag to ensure that any suitable server can handle the
          * request. Clients MUST  NOT set the slaveOK wire protocol flag if the
          * read preference mode is primary.
          */
         if (parts->read_prefs &&
             parts->read_prefs->mode != MONGOC_READ_PRIMARY) {
            parts->assembled.query_flags |= MONGOC_QUERY_SLAVE_OK;
         }

         break;
      case MONGOC_TOPOLOGY_SHARDED:
      case MONGOC_TOPOLOGY_UNKNOWN:
      case MONGOC_TOPOLOGY_DESCRIPTION_TYPES:
      default:
         /* must not call this function w/ sharded or unknown topology type */
         BSON_ASSERT (false);
      }
   } /* if (!parts->is_write_command) */

   if (!bson_empty (&parts->extra)) {
      _mongoc_cmd_parts_ensure_copied (parts);
   }

   if (!bson_empty (&parts->read_concern_document) &&
       strcmp (parts->assembled.command_name, "getMore") != 0) {
      _mongoc_cmd_parts_ensure_copied (parts);
      bson_append_document (&parts->assembled_body,
                            "readConcern",
                            11,
                            &parts->read_concern_document);
   }

   _mongoc_cmd_parts_add_write_concern (parts);

   EXIT;
}


static const bson_t *
_largest_cluster_time (const bson_t *a, const bson_t *b)
{
   if (!a) {
      return b;
   }

   if (!b) {
      return a;
   }

   if (_mongoc_cluster_time_greater (a, b)) {
      return a;
   }

   return b;
}


/* Check if the command should allow a transaction number if that has not
 * already been determined.
 *
 * This should only return true for write commands that are always retryable for
 * the server stream's wire version.
 *
 * The basic write commands (i.e. insert, update, delete) are intentionally
 * excluded here. While insert is always retryable, update and delete are only
 * retryable if they include no multi-document writes. Since it would be costly
 * to inspect the command document here, the bulk operation API explicitly sets
 * allow_txn_number for us. This means that insert, update, and delete are not
 * retryable if executed via mongoc_client_write_command_with_opts(); however,
 * documentation already instructs users not to use that for basic writes.
 */
static bool
_allow_txn_number (const mongoc_cmd_parts_t *parts,
                   const mongoc_server_stream_t *server_stream)
{
   /* There is no reason to call this function if allow_txn_number is set */
   BSON_ASSERT (parts->allow_txn_number ==
                MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_UNKNOWN);

   if (!parts->is_write_command) {
      return false;
   }

   if (server_stream->sd->max_wire_version < WIRE_VERSION_RETRY_WRITES) {
      return false;
   }

   if (!parts->assembled.is_acknowledged) {
      return false;
   }

   if (!strcasecmp (parts->assembled.command_name, "findandmodify")) {
      return true;
   }

   return false;
}


/* Check if the write command should support retryable behavior. */
static bool
_is_retryable_write (const mongoc_cmd_parts_t *parts,
                     const mongoc_server_stream_t *server_stream)
{
   if (!parts->assembled.session) {
      return false;
   }

   if (!parts->is_write_command) {
      return false;
   }

   if (parts->allow_txn_number != MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_YES) {
      return false;
   }

   if (server_stream->sd->max_wire_version < WIRE_VERSION_RETRY_WRITES) {
      return false;
   }

   if (server_stream->sd->type == MONGOC_SERVER_STANDALONE) {
      return false;
   }

   if (_mongoc_client_session_in_txn (parts->assembled.session)) {
      return false;
   }

   if (!mongoc_uri_get_option_as_bool (
          parts->client->uri, MONGOC_URI_RETRYWRITES, false)) {
      return false;
   }

   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_assemble --
 *
 *       Assemble the command body, options, and read preference into one
 *       command.
 *
 * Return:
 *       True if the options were successfully applied. If any options are
 *       invalid, returns false and fills out @error. In that case @parts is
 *       invalid and must not be used.
 *
 * Side effects:
 *       May partly assemble before returning an error.
 *       mongoc_cmd_parts_cleanup should be called in all cases.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_cmd_parts_assemble (mongoc_cmd_parts_t *parts,
                           const mongoc_server_stream_t *server_stream,
                           bson_error_t *error)
{
   mongoc_server_description_type_t server_type;
   mongoc_client_session_t *cs;
   const bson_t *cluster_time = NULL;
   mongoc_read_prefs_t *prefs = NULL;
   const char *cmd_name;
   bool is_get_more;
   const mongoc_read_prefs_t *prefs_ptr;
   bool ret = false;

   ENTRY;

   BSON_ASSERT (parts);
   BSON_ASSERT (server_stream);

   server_type = server_stream->sd->type;
   cs = parts->prohibit_lsid ? NULL : parts->assembled.session;

   /* must not be assembled already */
   BSON_ASSERT (!parts->assembled.command);
   BSON_ASSERT (bson_empty (&parts->assembled_body));

   /* begin with raw flags/cmd as assembled flags/cmd, might change below */
   parts->assembled.command = parts->body;
   /* unused in OP_MSG: */
   parts->assembled.query_flags = parts->user_query_flags;
   parts->assembled.server_stream = server_stream;
   cmd_name = parts->assembled.command_name =
      _mongoc_get_command_name (parts->assembled.command);

   if (!parts->assembled.command_name) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "Empty command document");
      GOTO (done);
   }

   TRACE ("Preparing '%s'", cmd_name);

   is_get_more = !strcmp (cmd_name, "getMore");
   parts->assembled.is_txn_finish = !strcmp (cmd_name, "commitTransaction") ||
                                    !strcmp (cmd_name, "abortTransaction");

   if (!parts->is_write_command && IS_PREF_PRIMARY (parts->read_prefs) &&
       server_stream->topology_type == MONGOC_TOPOLOGY_SINGLE &&
       server_type != MONGOC_SERVER_MONGOS) {
      prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY_PREFERRED);
      prefs_ptr = prefs;
   } else {
      prefs_ptr = parts->read_prefs;
   }

   if (server_stream->sd->max_wire_version >= WIRE_VERSION_OP_MSG) {
      if (!bson_has_field (parts->body, "$db")) {
         BSON_APPEND_UTF8 (&parts->extra, "$db", parts->assembled.db_name);
      }

      if (_mongoc_client_session_in_txn (cs)) {
         if (!IS_PREF_PRIMARY (cs->txn.opts.read_prefs) &&
             !parts->is_write_command) {
            bson_set_error (error,
                            MONGOC_ERROR_TRANSACTION,
                            MONGOC_ERROR_TRANSACTION_INVALID_STATE,
                            "Read preference in a transaction must be primary");
            GOTO (done);
         }
      } else if (!IS_PREF_PRIMARY (prefs_ptr)) {
         _mongoc_cmd_parts_add_read_prefs (&parts->extra, prefs_ptr);
      }

      if (!bson_empty (&parts->extra)) {
         _mongoc_cmd_parts_ensure_copied (parts);
      }

      /* If an explicit session was not provided and lsid is not prohibited,
       * attempt to create an implicit session (ignoring any errors). */
      if (!cs && !parts->prohibit_lsid && parts->assembled.is_acknowledged) {
         cs = mongoc_client_start_session (parts->client, NULL, NULL);

         if (cs) {
            parts->assembled.session = cs;
            parts->has_temp_session = true;
         }
      }

      /* Driver Sessions Spec: "For unacknowledged writes with an explicit
       * session, drivers SHOULD raise an error.... Without an explicit
       * session, drivers SHOULD NOT use an implicit session." We intentionally
       * do not restrict this logic to parts->is_write_command, since
       * mongoc_client_command_with_opts() does not identify as a write
       * command but may still include a write concern.
       */
      if (cs) {
         if (!parts->assembled.is_acknowledged) {
            bson_set_error (
               error,
               MONGOC_ERROR_COMMAND,
               MONGOC_ERROR_COMMAND_INVALID_ARG,
               "Cannot use client session with unacknowledged command");
            GOTO (done);
         }

         _mongoc_cmd_parts_ensure_copied (parts);
         bson_append_document (&parts->assembled_body,
                               "lsid",
                               4,
                               mongoc_client_session_get_lsid (cs));

         cs->server_session->last_used_usec = bson_get_monotonic_time ();
         cluster_time = mongoc_client_session_get_cluster_time (cs);
      }

      /* Ensure we know if the write command allows a transaction number */
      if (!_mongoc_client_session_txn_in_progress (cs) &&
          parts->is_write_command &&
          parts->allow_txn_number ==
             MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_UNKNOWN) {
         parts->allow_txn_number = _allow_txn_number (parts, server_stream)
                                      ? MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_YES
                                      : MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_NO;
      }

      /* Determine if the command is retryable. If so, append txnNumber now
       * for future use and mark the command as such. */
      if (_is_retryable_write (parts, server_stream)) {
         _mongoc_cmd_parts_ensure_copied (parts);
         bson_append_int64 (&parts->assembled_body, "txnNumber", 9, 0);
         parts->is_retryable_write = true;
      }

      if (!bson_empty (&server_stream->cluster_time)) {
         cluster_time =
            _largest_cluster_time (&server_stream->cluster_time, cluster_time);
      }

      if (cluster_time && server_type != MONGOC_SERVER_STANDALONE) {
         _mongoc_cmd_parts_ensure_copied (parts);
         bson_append_document (
            &parts->assembled_body, "$clusterTime", 12, cluster_time);
      }

      if (!is_get_more) {
         if (cs) {
            _mongoc_client_session_append_read_concern (
               cs,
               &parts->read_concern_document,
               parts->is_read_command,
               &parts->assembled_body);
         } else if (!bson_empty (&parts->read_concern_document)) {
            bson_append_document (&parts->assembled_body,
                                  "readConcern",
                                  11,
                                  &parts->read_concern_document);
         }
      }

      if (parts->assembled.is_txn_finish ||
          !_mongoc_client_session_in_txn (cs)) {
         _mongoc_cmd_parts_add_write_concern (parts);
      }

      if (!_mongoc_client_session_append_txn (
             cs, &parts->assembled_body, error)) {
         GOTO (done);
      }

      ret = true;
   } else if (server_type == MONGOC_SERVER_MONGOS) {
      _mongoc_cmd_parts_assemble_mongos (parts, server_stream);
      ret = true;
   } else {
      _mongoc_cmd_parts_assemble_mongod (parts, server_stream);
      ret = true;
   }

done:
   mongoc_read_prefs_destroy (prefs);
   RETURN (ret);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_cmd_parts_cleanup --
 *
 *       Free memory associated with a stack-allocated mongoc_cmd_parts_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_cmd_parts_cleanup (mongoc_cmd_parts_t *parts)
{
   bson_destroy (&parts->read_concern_document);
   bson_destroy (&parts->write_concern_document);
   bson_destroy (&parts->extra);
   bson_destroy (&parts->assembled_body);

   if (parts->has_temp_session) {
      /* client session returns its server session to server session pool */
      mongoc_client_session_destroy (parts->assembled.session);
   }
}

bool
mongoc_cmd_is_compressible (mongoc_cmd_t *cmd)
{
   BSON_ASSERT (cmd);
   BSON_ASSERT (cmd->command_name);

   return !!strcasecmp (cmd->command_name, "ismaster") &&
          !!strcasecmp (cmd->command_name, "authenticate") &&
          !!strcasecmp (cmd->command_name, "getnonce") &&
          !!strcasecmp (cmd->command_name, "saslstart") &&
          !!strcasecmp (cmd->command_name, "saslcontinue") &&
          !!strcasecmp (cmd->command_name, "createuser") &&
          !!strcasecmp (cmd->command_name, "updateuser") &&
          !!strcasecmp (cmd->command_name, "copydb") &&
          !!strcasecmp (cmd->command_name, "copydbsaslstart") &&
          !!strcasecmp (cmd->command_name, "copydbgetnonce");
}
