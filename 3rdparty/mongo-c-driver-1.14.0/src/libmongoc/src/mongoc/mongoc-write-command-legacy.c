/*
 * Copyright 2014-present MongoDB, Inc.
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

#include <bson/bson.h>

#include "mongoc/mongoc-write-command-legacy-private.h"
#include "mongoc/mongoc-trace-private.h"
#include "mongoc/mongoc-util-private.h"

static void
_mongoc_monitor_legacy_write (mongoc_client_t *client,
                              mongoc_write_command_t *command,
                              const char *db,
                              const char *collection,
                              mongoc_server_stream_t *stream,
                              int64_t request_id)
{
   bson_t doc;
   bson_t wc;
   mongoc_apm_command_started_t event;

   ENTRY;

   if (!client->apm_callbacks.started) {
      EXIT;
   }

   bson_init (&doc);
   _mongoc_write_command_init (&doc, command, collection);
   BSON_APPEND_DOCUMENT_BEGIN (&doc, "writeConcern", &wc);
   BSON_APPEND_INT32 (&wc, "w", 0);
   bson_append_document_end (&doc, &wc);

   _append_array_from_command (command, &doc);

   mongoc_apm_command_started_init (
      &event,
      &doc,
      db,
      _mongoc_command_type_to_name (command->type),
      request_id,
      command->operation_id,
      &stream->sd->host,
      stream->sd->id,
      client->apm_context);

   client->apm_callbacks.started (&event);

   mongoc_apm_command_started_cleanup (&event);
   bson_destroy (&doc);
}


/* fire command-succeeded event as if we'd used a modern write command.
 * note, cluster.request_id was incremented once for the write, again
 * for the getLastError, so cluster.request_id is no longer valid; used the
 * passed-in request_id instead.
 */
static void
_mongoc_monitor_legacy_write_succeeded (mongoc_client_t *client,
                                        int64_t duration,
                                        mongoc_write_command_t *command,
                                        mongoc_server_stream_t *stream,
                                        int64_t request_id)
{
   bson_t doc;

   mongoc_apm_command_succeeded_t event;

   ENTRY;

   if (!client->apm_callbacks.succeeded) {
      EXIT;
   }

   bson_init (&doc);
   /*
    * Unacknowledged writes must provide a CommandSucceededEvent with a { ok: 1
    * } reply.
    * https://github.com/mongodb/specifications/blob/master/source/command-monitoring/command-monitoring.rst#unacknowledged-acknowledged-writes
    */
   bson_append_int32 (&doc, "ok", 2, 1);
   bson_append_int32 (&doc, "n", 1, (int32_t) command->n_documents);

   mongoc_apm_command_succeeded_init (
      &event,
      duration,
      &doc,
      _mongoc_command_type_to_name (command->type),
      request_id,
      command->operation_id,
      &stream->sd->host,
      stream->sd->id,
      client->apm_context);

   client->apm_callbacks.succeeded (&event);

   mongoc_apm_command_succeeded_cleanup (&event);
   bson_destroy (&doc);

   EXIT;
}


void
_mongoc_write_command_delete_legacy (mongoc_write_command_t *command,
                                     mongoc_client_t *client,
                                     mongoc_server_stream_t *server_stream,
                                     const char *database,
                                     const char *collection,
                                     uint32_t offset,
                                     mongoc_write_result_t *result,
                                     bson_error_t *error)
{
   int64_t started;
   int32_t max_bson_obj_size;
   const uint8_t *data;
   mongoc_rpc_t rpc;
   uint32_t request_id;
   bson_iter_t q_iter;
   uint32_t len;
   int64_t limit = 0;
   char ns[MONGOC_NAMESPACE_MAX + 1];
   bool r;
   bson_reader_t *reader;
   const bson_t *bson;
   bool eof;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (database);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (collection);

   started = bson_get_monotonic_time ();

   max_bson_obj_size = mongoc_server_stream_max_bson_obj_size (server_stream);

   if (!command->n_documents) {
      bson_set_error (error,
                      MONGOC_ERROR_COLLECTION,
                      MONGOC_ERROR_COLLECTION_DELETE_FAILED,
                      "Cannot do an empty delete.");
      result->failed = true;
      EXIT;
   }

   bson_snprintf (ns, sizeof ns, "%s.%s", database, collection);

   reader =
      bson_reader_new_from_data (command->payload.data, command->payload.len);
   while ((bson = bson_reader_read (reader, &eof))) {
      /* the document is like { "q": { <selector> }, limit: <0 or 1> } */
      r = (bson_iter_init (&q_iter, bson) && bson_iter_find (&q_iter, "q") &&
           BSON_ITER_HOLDS_DOCUMENT (&q_iter));

      BSON_ASSERT (r);
      bson_iter_document (&q_iter, &len, &data);
      BSON_ASSERT (data);
      BSON_ASSERT (len >= 5);
      if (len > max_bson_obj_size) {
         _mongoc_write_command_too_large_error (
            error, 0, len, max_bson_obj_size);
         result->failed = true;
         bson_reader_destroy (reader);
         EXIT;
      }

      request_id = ++client->cluster.request_id;

      rpc.header.msg_len = 0;
      rpc.header.request_id = request_id;
      rpc.header.response_to = 0;
      rpc.header.opcode = MONGOC_OPCODE_DELETE;
      rpc.delete_.zero = 0;
      rpc.delete_.collection = ns;

      if (bson_iter_find (&q_iter, "limit") &&
          (BSON_ITER_HOLDS_INT (&q_iter))) {
         limit = bson_iter_as_int64 (&q_iter);
      }

      rpc.delete_.flags =
         limit ? MONGOC_DELETE_SINGLE_REMOVE : MONGOC_DELETE_NONE;
      rpc.delete_.selector = data;

      _mongoc_monitor_legacy_write (
         client, command, database, collection, server_stream, request_id);

      if (!mongoc_cluster_legacy_rpc_sendv_to_server (
             &client->cluster, &rpc, server_stream, error)) {
         result->failed = true;
         bson_reader_destroy (reader);
         EXIT;
      }

      _mongoc_monitor_legacy_write_succeeded (client,
                                              bson_get_monotonic_time () -
                                                 started,
                                              command,
                                              server_stream,
                                              request_id);

      started = bson_get_monotonic_time ();
   }
   bson_reader_destroy (reader);

   EXIT;
}


void
_mongoc_write_command_insert_legacy (mongoc_write_command_t *command,
                                     mongoc_client_t *client,
                                     mongoc_server_stream_t *server_stream,
                                     const char *database,
                                     const char *collection,
                                     uint32_t offset,
                                     mongoc_write_result_t *result,
                                     bson_error_t *error)
{
   int64_t started;
   mongoc_iovec_t *iov;
   mongoc_rpc_t rpc;
   uint32_t size = 0;
   bool has_more;
   char ns[MONGOC_NAMESPACE_MAX + 1];
   uint32_t n_docs_in_batch;
   uint32_t request_id = 0;
   uint32_t idx = 0;
   int32_t max_msg_size;
   int32_t max_bson_obj_size;
   bool singly;
   bson_reader_t *reader;
   const bson_t *bson;
   bool eof;
   int data_offset = 0;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (database);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (collection);
   BSON_ASSERT (command->type == MONGOC_WRITE_COMMAND_INSERT);

   started = bson_get_monotonic_time ();

   max_bson_obj_size = mongoc_server_stream_max_bson_obj_size (server_stream);
   max_msg_size = mongoc_server_stream_max_msg_size (server_stream);

   singly = !command->u.insert.allow_bulk_op_insert;

   if (!command->n_documents) {
      bson_set_error (error,
                      MONGOC_ERROR_COLLECTION,
                      MONGOC_ERROR_COLLECTION_INSERT_FAILED,
                      "Cannot do an empty insert.");
      result->failed = true;
      EXIT;
   }

   bson_snprintf (ns, sizeof ns, "%s.%s", database, collection);

   iov = (mongoc_iovec_t *) bson_malloc ((sizeof *iov) * command->n_documents);

again:
   has_more = false;
   n_docs_in_batch = 0;
   size = (uint32_t) (sizeof (mongoc_rpc_header_t) + 4 + strlen (database) + 1 +
                      strlen (collection) + 1);

   reader = bson_reader_new_from_data (command->payload.data + data_offset,
                                       command->payload.len - data_offset);
   while ((bson = bson_reader_read (reader, &eof))) {
      BSON_ASSERT (n_docs_in_batch <= idx);
      BSON_ASSERT (idx <= command->n_documents);

      if (bson->len > max_bson_obj_size) {
         /* document is too large */
         _mongoc_write_command_too_large_error (
            error, idx, bson->len, max_bson_obj_size);

         data_offset += bson->len;

         if (command->flags.ordered) {
            /* send the batch so far (if any) and return the error */
            break;
         }
      } else if ((n_docs_in_batch == 1 && singly) ||
                 size > (max_msg_size - bson->len)) {
         /* batch is full, send it and then start the next batch */
         has_more = true;
         break;
      } else {
         /* add document to batch and continue building the batch */
         iov[n_docs_in_batch].iov_base = (void *) bson_get_data (bson);
         iov[n_docs_in_batch].iov_len = bson->len;
         size += bson->len;
         n_docs_in_batch++;
         data_offset += bson->len;
      }

      idx++;
   }
   bson_reader_destroy (reader);

   if (n_docs_in_batch) {
      request_id = ++client->cluster.request_id;

      rpc.header.msg_len = 0;
      rpc.header.request_id = request_id;
      rpc.header.response_to = 0;
      rpc.header.opcode = MONGOC_OPCODE_INSERT;
      rpc.insert.flags =
         ((command->flags.ordered) ? MONGOC_INSERT_NONE
                                   : MONGOC_INSERT_CONTINUE_ON_ERROR);
      rpc.insert.collection = ns;
      rpc.insert.documents = iov;
      rpc.insert.n_documents = n_docs_in_batch;

      _mongoc_monitor_legacy_write (
         client, command, database, collection, server_stream, request_id);

      if (!mongoc_cluster_legacy_rpc_sendv_to_server (
             &client->cluster, &rpc, server_stream, error)) {
         result->failed = true;
         GOTO (cleanup);
      }

      _mongoc_monitor_legacy_write_succeeded (client,
                                              bson_get_monotonic_time () -
                                                 started,
                                              command,
                                              server_stream,
                                              request_id);

      started = bson_get_monotonic_time ();
   }

cleanup:

   if (has_more) {
      GOTO (again);
   }

   bson_free (iov);

   EXIT;
}


void
_mongoc_write_command_update_legacy (mongoc_write_command_t *command,
                                     mongoc_client_t *client,
                                     mongoc_server_stream_t *server_stream,
                                     const char *database,
                                     const char *collection,
                                     uint32_t offset,
                                     mongoc_write_result_t *result,
                                     bson_error_t *error)
{
   int64_t started;
   int32_t max_bson_obj_size;
   mongoc_rpc_t rpc;
   uint32_t request_id = 0;
   bson_iter_t subiter, subsubiter;
   bson_t doc;
   bson_t update, selector;
   const uint8_t *data = NULL;
   uint32_t len = 0;
   size_t err_offset;
   bool val = false;
   char ns[MONGOC_NAMESPACE_MAX + 1];
   int vflags = (BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL |
                 BSON_VALIDATE_DOLLAR_KEYS | BSON_VALIDATE_DOT_KEYS);
   bson_reader_t *reader;
   const bson_t *bson;
   bool eof;

   ENTRY;

   BSON_ASSERT (command);
   BSON_ASSERT (client);
   BSON_ASSERT (database);
   BSON_ASSERT (server_stream);
   BSON_ASSERT (collection);

   started = bson_get_monotonic_time ();

   max_bson_obj_size = mongoc_server_stream_max_bson_obj_size (server_stream);

   reader =
      bson_reader_new_from_data (command->payload.data, command->payload.len);
   while ((bson = bson_reader_read (reader, &eof))) {
      if (bson_iter_init (&subiter, bson) && bson_iter_find (&subiter, "u") &&
          BSON_ITER_HOLDS_DOCUMENT (&subiter)) {
         bson_iter_document (&subiter, &len, &data);
         BSON_ASSERT (bson_init_static (&doc, data, len));

         if (bson_iter_init (&subsubiter, &doc) &&
             bson_iter_next (&subsubiter) &&
             (bson_iter_key (&subsubiter)[0] != '$') &&
             !bson_validate (
                &doc, (bson_validate_flags_t) vflags, &err_offset)) {
            result->failed = true;
            bson_set_error (error,
                            MONGOC_ERROR_BSON,
                            MONGOC_ERROR_BSON_INVALID,
                            "update document is corrupt or contains "
                            "invalid keys including $ or .");
            bson_reader_destroy (reader);
            EXIT;
         }
      } else {
         result->failed = true;
         bson_set_error (error,
                         MONGOC_ERROR_BSON,
                         MONGOC_ERROR_BSON_INVALID,
                         "updates is malformed.");
         bson_reader_destroy (reader);
         EXIT;
      }
   }

   bson_snprintf (ns, sizeof ns, "%s.%s", database, collection);

   bson_reader_destroy (reader);
   reader =
      bson_reader_new_from_data (command->payload.data, command->payload.len);
   while ((bson = bson_reader_read (reader, &eof))) {
      request_id = ++client->cluster.request_id;

      rpc.header.msg_len = 0;
      rpc.header.request_id = request_id;
      rpc.header.response_to = 0;
      rpc.header.opcode = MONGOC_OPCODE_UPDATE;
      rpc.update.zero = 0;
      rpc.update.collection = ns;
      rpc.update.flags = MONGOC_UPDATE_NONE;

      BSON_ASSERT (bson_iter_init (&subiter, bson));
      while (bson_iter_next (&subiter)) {
         if (strcmp (bson_iter_key (&subiter), "u") == 0) {
            bson_iter_document (&subiter, &len, &data);
            if (len > max_bson_obj_size) {
               _mongoc_write_command_too_large_error (
                  error, 0, len, max_bson_obj_size);
               result->failed = true;
               bson_reader_destroy (reader);
               EXIT;
            }

            rpc.update.update = data;
            BSON_ASSERT (bson_init_static (&update, data, len));
         } else if (strcmp (bson_iter_key (&subiter), "q") == 0) {
            bson_iter_document (&subiter, &len, &data);
            if (len > max_bson_obj_size) {
               _mongoc_write_command_too_large_error (
                  error, 0, len, max_bson_obj_size);
               result->failed = true;
               bson_reader_destroy (reader);
               EXIT;
            }

            rpc.update.selector = data;
            BSON_ASSERT (bson_init_static (&selector, data, len));
         } else if (strcmp (bson_iter_key (&subiter), "multi") == 0) {
            val = bson_iter_bool (&subiter);
            if (val) {
               rpc.update.flags = (mongoc_update_flags_t) (
                  rpc.update.flags | MONGOC_UPDATE_MULTI_UPDATE);
            }
         } else if (strcmp (bson_iter_key (&subiter), "upsert") == 0) {
            val = bson_iter_bool (&subiter);
            if (val) {
               rpc.update.flags = (mongoc_update_flags_t) (
                  rpc.update.flags | MONGOC_UPDATE_UPSERT);
            }
         }
      }

      _mongoc_monitor_legacy_write (
         client, command, database, collection, server_stream, request_id);

      if (!mongoc_cluster_legacy_rpc_sendv_to_server (
             &client->cluster, &rpc, server_stream, error)) {
         result->failed = true;
         bson_reader_destroy (reader);
         EXIT;
      }

      _mongoc_monitor_legacy_write_succeeded (client,
                                              bson_get_monotonic_time () -
                                                 started,
                                              command,
                                              server_stream,
                                              request_id);

      started = bson_get_monotonic_time ();
   }
   bson_reader_destroy (reader);
}
