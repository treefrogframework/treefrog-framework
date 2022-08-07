/*
 * Copyright 2020-present MongoDB, Inc.
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

#include "operation.h"

#include "mongoc-array-private.h"
#include "result.h"
#include "test-diagnostics.h"
#include "util.h"
#include "utlist.h"

typedef struct {
   char *name;
   char *object;
   bson_t *arguments;
   bson_t *expect_error;
   bson_val_t *expect_result;
   bool *ignore_result_and_error;
   char *save_result_as_entity;
   bson_parser_t *parser;
   char *session_id;
   mongoc_client_session_t *session;
} operation_t;

static void
operation_destroy (operation_t *op)
{
   if (!op) {
      return;
   }
   bson_parser_destroy_with_parsed_fields (op->parser);
   bson_free (op->session_id);
   bson_free (op);
}

static operation_t *
operation_new (bson_t *bson, bson_error_t *error)
{
   operation_t *op = bson_malloc0 (sizeof (operation_t));
   op->parser = bson_parser_new ();
   bson_parser_utf8 (op->parser, "name", &op->name);
   bson_parser_utf8 (op->parser, "object", &op->object);
   bson_parser_doc_optional (op->parser, "arguments", &op->arguments);
   bson_parser_doc_optional (op->parser, "expectError", &op->expect_error);
   bson_parser_any_optional (op->parser, "expectResult", &op->expect_result);
   bson_parser_bool_optional (
      op->parser, "ignoreResultAndError", &op->ignore_result_and_error);
   bson_parser_utf8_optional (
      op->parser, "saveResultAsEntity", &op->save_result_as_entity);
   if (!bson_parser_parse (op->parser, bson, error)) {
      operation_destroy (op);
      return NULL;
   }
   return op;
}

static bool
operation_create_change_stream (test_t *test,
                                operation_t *op,
                                result_t *result,
                                bson_error_t *error)
{
   bool ret = false;
   entity_t *entity = NULL;
   bson_parser_t *parser = NULL;
   mongoc_change_stream_t *changestream = NULL;
   bson_t *pipeline = NULL;
   const bson_t *op_reply = NULL;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   /* Capture options as all extra fields, and pass them directly as change
    * stream options. */
   bson_parser_allow_extra (parser, true);
   bson_parser_array (parser, "pipeline", &pipeline);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   entity = entity_map_get (test->entity_map, op->object, error);
   if (!entity) {
      goto done;
   }

   if (0 == strcmp (entity->type, "client")) {
      mongoc_client_t *client = (mongoc_client_t *) entity->value;
      changestream = mongoc_client_watch (client, pipeline, opts);
   } else if (0 == strcmp (entity->type, "database")) {
      mongoc_database_t *db = (mongoc_database_t *) entity->value;
      changestream = mongoc_database_watch (db, pipeline, opts);
   } else if (0 == strcmp (entity->type, "collection")) {
      mongoc_collection_t *coll = (mongoc_collection_t *) entity->value;
      changestream = mongoc_collection_watch (coll, pipeline, opts);
   }

   if (!op->save_result_as_entity) {
      test_set_error (error,
                      "unexpected createChangeStream does not save result");
      goto done;
   }

   if (!entity_map_add_changestream (
          test->entity_map, op->save_result_as_entity, changestream, error)) {
      goto done;
   }

   mongoc_change_stream_error_document (changestream, &op_error, &op_reply);
   result_from_val_and_reply (result, NULL, (bson_t *) op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (opts);
   return ret;
}

static bool
operation_list_databases (test_t *test,
                          operation_t *op,
                          result_t *result,
                          bson_error_t *error)
{
   bool ret = false;
   mongoc_client_t *client = NULL;
   mongoc_cursor_t *cursor = NULL;
   bson_t *opts = NULL;

   opts = bson_new ();
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }
   if (op->arguments) {
      bson_concat (opts, op->arguments);
   }

   client = entity_map_get_client (test->entity_map, op->object, error);
   if (!client) {
      goto done;
   }

   cursor = mongoc_client_find_databases_with_opts (client, opts);

   result_from_cursor (result, cursor);

   ret = true;
done:
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   return ret;
}

static bool
operation_create_collection (test_t *test,
                             operation_t *op,
                             result_t *result,
                             bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *parser = NULL;
   mongoc_database_t *db = NULL;
   char *collection = NULL;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;
   mongoc_collection_t *coll = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_utf8 (parser, "collection", &collection);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   db = entity_map_get_database (test->entity_map, op->object, error);
   if (!db) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = mongoc_database_create_collection (db, collection, opts, &op_error);

   result_from_val_and_reply (result, NULL, NULL, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (opts);
   mongoc_collection_destroy (coll);
   return ret;
}

static bool
operation_drop_collection (test_t *test,
                           operation_t *op,
                           result_t *result,
                           bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *parser = NULL;
   mongoc_database_t *db = NULL;
   mongoc_collection_t *coll = NULL;
   char *collection = NULL;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_utf8 (parser, "collection", &collection);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_new ();
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   db = entity_map_get_database (test->entity_map, op->object, error);
   if (!db) {
      goto done;
   }

   coll = mongoc_database_get_collection (db, collection);
   mongoc_collection_drop_with_opts (coll, opts, &op_error);

   result_from_val_and_reply (result, NULL, NULL, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   mongoc_collection_destroy (coll);
   bson_destroy (opts);
   return ret;
}

static bool
operation_list_collections (test_t *test,
                            operation_t *op,
                            result_t *result,
                            bson_error_t *error)
{
   bool ret = false;
   mongoc_database_t *db = NULL;
   mongoc_cursor_t *cursor = NULL;
   bson_t *opts = NULL;

   opts = bson_new ();
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }
   bson_concat (opts, op->arguments);

   db = entity_map_get_database (test->entity_map, op->object, error);
   if (!db) {
      goto done;
   }

   cursor = mongoc_database_find_collections_with_opts (db, opts);

   result_from_cursor (result, cursor);

   ret = true;
done:
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   return ret;
}

static bool
operation_list_collection_names (test_t *test,
                                 operation_t *op,
                                 result_t *result,
                                 bson_error_t *error)
{
   bool ret = false;
   mongoc_database_t *db = NULL;
   mongoc_cursor_t *cursor = NULL;
   char **op_ret = NULL;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   opts = bson_new ();
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }
   bson_concat (opts, op->arguments);

   db = entity_map_get_database (test->entity_map, op->object, error);
   if (!db) {
      goto done;
   }

   op_ret =
      mongoc_database_get_collection_names_with_opts (db, opts, &op_error);

   result_from_ok (result);

   ret = true;
done:
   mongoc_cursor_destroy (cursor);
   bson_strfreev (op_ret);
   bson_destroy (opts);
   return ret;
}

static bool
operation_list_indexes (test_t *test,
                        operation_t *op,
                        result_t *result,
                        bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   mongoc_cursor_t *cursor = NULL;
   bson_t *opts = NULL;

   opts = bson_new ();
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }
   bson_concat (opts, op->arguments);

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   cursor = mongoc_collection_find_indexes_with_opts (coll, opts);

   result_from_cursor (result, cursor);

   ret = true;
done:
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   return ret;
}

static bool
operation_run_command (test_t *test,
                       operation_t *op,
                       result_t *result,
                       bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *parser = NULL;
   bson_t *command = NULL;
   char *command_name = NULL;
   mongoc_database_t *db = NULL;
   mongoc_read_prefs_t *rp = NULL;
   mongoc_write_concern_t *wc = NULL;
   mongoc_read_concern_t *rc = NULL;
   bson_error_t op_error = {0};
   bson_t op_reply = BSON_INITIALIZER;
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_doc (parser, "command", &command);
   bson_parser_utf8 (parser, "commandName", &command_name);
   bson_parser_read_concern_optional (parser, &rc);
   bson_parser_write_concern_optional (parser, &wc);
   bson_parser_read_prefs_optional (parser, &rp);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_new ();
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   db = entity_map_get_database (test->entity_map, op->object, error);
   if (!db) {
      goto done;
   }

   if (rc) {
      mongoc_read_concern_append (rc, opts);
   }
   if (wc) {
      mongoc_write_concern_append (wc, opts);
   }

   bson_destroy (&op_reply);
   mongoc_database_command_with_opts (
      db, command, rp, opts, &op_reply, &op_error);

   result_from_val_and_reply (result, NULL, &op_reply, &op_error);

   ret = true;
done:
   bson_destroy (&op_reply);
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (opts);
   return ret;
}

static bool
operation_aggregate (test_t *test,
                     operation_t *op,
                     result_t *result,
                     bson_error_t *error)
{
   bool ret = false;
   entity_t *entity = NULL;
   bson_t *pipeline = NULL;
   bson_parser_t *parser = NULL;
   mongoc_cursor_t *cursor = NULL;
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_array (parser, "pipeline", &pipeline);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   entity = entity_map_get (test->entity_map, op->object, error);
   if (0 == strcmp (entity->type, "collection")) {
      mongoc_collection_t *coll = (mongoc_collection_t *) entity->value;
      cursor = mongoc_collection_aggregate (
         coll, 0 /* query flags */, pipeline, opts, NULL /* read prefs */);
   } else if (0 == strcmp (entity->type, "database")) {
      mongoc_database_t *db = (mongoc_database_t *) entity->value;
      cursor =
         mongoc_database_aggregate (db, pipeline, opts, NULL /* read prefs */);
   } else {
      goto done;
   }

   result_from_cursor (result, cursor);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   mongoc_cursor_destroy (cursor);
   bson_destroy (opts);
   return ret;
}

static bool
bulk_op_append (mongoc_bulk_operation_t *bulk,
                bson_t *request,
                bson_error_t *error)
{
   bool ret = false;
   bson_iter_t iter;
   const char *op_type;
   bson_parser_t *parser = NULL;
   bson_t *document = NULL, *filter = NULL, *update = NULL, *replacement = NULL;
   bson_t request_doc;

   bson_iter_init (&iter, request);
   bson_iter_next (&iter);
   if (!BSON_ITER_HOLDS_DOCUMENT (&iter)) {
      test_set_error (error,
                      "Unexpected non-document in bulk write model: %s",
                      bson_iter_key (&iter));
   }
   op_type = bson_iter_key (&iter);
   bson_iter_bson (&iter, &request_doc);

   parser = bson_parser_new ();
   /* Pass extra options to operation. Server errors on unrecognized options. */
   bson_parser_allow_extra (parser, true);

   if (0 == strcmp (op_type, "insertOne")) {
      bson_parser_doc (parser, "document", &document);
      if (!bson_parser_parse (parser, &request_doc, error)) {
         goto done;
      }

      mongoc_bulk_operation_insert_with_opts (
         bulk, document, bson_parser_get_extra (parser), error);
   } else if (0 == strcmp (op_type, "updateOne")) {
      bson_parser_doc (parser, "filter", &filter);
      bson_parser_array_or_doc (parser, "update", &update);
      if (!bson_parser_parse (parser, &request_doc, error)) {
         goto done;
      }

      mongoc_bulk_operation_update_one_with_opts (
         bulk, filter, update, bson_parser_get_extra (parser), error);
   } else if (0 == strcmp (op_type, "updateMany")) {
      bson_parser_doc (parser, "filter", &filter);
      bson_parser_array_or_doc (parser, "update", &update);
      if (!bson_parser_parse (parser, &request_doc, error)) {
         goto done;
      }

      mongoc_bulk_operation_update_many_with_opts (
         bulk, filter, update, bson_parser_get_extra (parser), error);
   } else if (0 == strcmp (op_type, "deleteOne")) {
      bson_parser_doc (parser, "filter", &filter);

      if (!bson_parser_parse (parser, &request_doc, error)) {
         goto done;
      }

      mongoc_bulk_operation_remove_one_with_opts (
         bulk, filter, bson_parser_get_extra (parser), error);
   } else if (0 == strcmp (op_type, "deleteMany")) {
      bson_parser_doc (parser, "filter", &filter);
      if (!bson_parser_parse (parser, &request_doc, error)) {
         goto done;
      }

      mongoc_bulk_operation_remove_many_with_opts (
         bulk, filter, bson_parser_get_extra (parser), error);
   } else if (0 == strcmp (op_type, "replaceOne")) {
      bson_parser_doc (parser, "filter", &filter);
      bson_parser_doc (parser, "replacement", &replacement);
      if (!bson_parser_parse (parser, &request_doc, error)) {
         goto done;
      }

      mongoc_bulk_operation_replace_one_with_opts (
         bulk, filter, replacement, bson_parser_get_extra (parser), error);
   }

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   return ret;
}

static bool
operation_bulk_write (test_t *test,
                      operation_t *op,
                      result_t *result,
                      bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bool *ordered = NULL;
   bson_t *requests = NULL;
   bson_t *opts = NULL;
   bson_iter_t iter;
   mongoc_bulk_operation_t *bulk_op = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};

   parser = bson_parser_new ();
   bson_parser_array (parser, "requests", &requests);
   bson_parser_bool_optional (parser, "ordered", &ordered);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   opts = bson_new ();
   if (ordered) {
      BSON_APPEND_BOOL (opts, "ordered", *ordered);
   }
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   bulk_op = mongoc_collection_create_bulk_operation_with_opts (coll, opts);

   BSON_FOREACH (requests, iter)
   {
      bson_t request;
      bson_iter_bson (&iter, &request);
      if (!bulk_op_append (bulk_op, &request, error)) {
         goto done;
      }
   }

   bson_destroy (&op_reply);
   mongoc_bulk_operation_execute (bulk_op, &op_reply, &op_error);
   result_from_bulk_write (result, &op_reply, &op_error);


   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_destroy (opts);
   mongoc_bulk_operation_destroy (bulk_op);
   return ret;
}

static bool
operation_count_documents (test_t *test,
                           operation_t *op,
                           result_t *result,
                           bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *filter = NULL;
   int64_t op_ret = -1;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_val_t *val = NULL;
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bson_destroy (&op_reply);
   op_ret = mongoc_collection_count_documents (
      coll, filter, opts, NULL /* read prefs */, &op_reply, &op_error);

   if (op_ret != -1) {
      val = bson_val_from_int64 (op_ret);
   }

   result_from_val_and_reply (result, val, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_val_destroy (val);
   bson_destroy (opts);
   return ret;
}

static bool
operation_create_find_cursor (test_t *test,
                             operation_t *op,
                             result_t *result,
                             bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   mongoc_cursor_t *cursor = NULL;
   bson_t *filter = NULL;
   bson_t *opts = NULL;
   const bson_t *op_reply = NULL;
   bson_error_t op_error = {0};
   const bson_t *first_result = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   cursor = mongoc_collection_find_with_opts (
      coll, filter, opts, NULL /* read prefs */);

   mongoc_cursor_next (cursor, &first_result);

   mongoc_cursor_error_document (cursor, &op_error, &op_reply);
   result_from_val_and_reply (result, NULL, (bson_t *) op_reply, &op_error);

   ret = true;

   if (!op->save_result_as_entity) {
      mongoc_cursor_destroy (cursor);
      goto done;
   }

   if (!entity_map_add_findcursor (test->entity_map,
                                   op->save_result_as_entity,
                                   cursor,
                                   first_result,
                                   error)) {
      goto done;
   }

done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (opts);
   return ret;
}

static bool
operation_create_index (test_t *test,
                        operation_t *op,
                        result_t *result,
                        bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *bp = NULL;
   char *name = NULL;
   bson_t *keys = NULL;
   bson_t *create_indexes = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *opts = bson_new ();

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bp = bson_parser_new ();
   bson_parser_utf8 (bp, "name", &name);
   bson_parser_doc (bp, "keys", &keys);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   /* libmongoc has no create index helper. Use runCommand. */
   create_indexes = BCON_NEW ("createIndexes",
                              mongoc_collection_get_name (coll),
                              "indexes",
                              "[",
                              "{",
                              "name",
                              name,
                              "key",
                              BCON_DOCUMENT (keys),
                              "}",
                              "]");
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   MONGOC_DEBUG ("running createIndexes: %s", tmp_json (create_indexes));

   bson_destroy (&op_reply);
   mongoc_collection_command_with_opts (
      coll, create_indexes, NULL /* read prefs */, opts, &op_reply, &op_error);

   result_from_val_and_reply (result, NULL, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   bson_destroy (&op_reply);
   bson_destroy (opts);
   bson_destroy (create_indexes);
   return ret;
}

static bool
operation_delete_one (test_t *test,
                      operation_t *op,
                      result_t *result,
                      bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *filter = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bson_destroy (&op_reply);
   mongoc_collection_delete_one (coll, filter, opts, &op_reply, &op_error);

   result_from_delete (result, &op_reply, &op_error);

   ret = true;
done:
   bson_destroy (&op_reply);
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (opts);
   return ret;
}

static bool
operation_delete_many (test_t *test,
                       operation_t *op,
                       result_t *result,
                       bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *filter = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bson_destroy (&op_reply);
   mongoc_collection_delete_many (coll, filter, opts, &op_reply, &op_error);

   result_from_delete (result, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_destroy (opts);
   return ret;
}

static bool
operation_distinct (test_t *test,
                    operation_t *op,
                    result_t *result,
                    bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *distinct = NULL;
   char *field_name = NULL;
   bson_t *filter = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_utf8 (parser, "fieldName", &field_name);
   bson_parser_doc (parser, "filter", &filter);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   distinct = BCON_NEW ("distinct",
                        mongoc_collection_get_name (coll),
                        "key",
                        BCON_UTF8 (field_name),
                        "query",
                        "{",
                        &filter,
                        "}");

   bson_destroy (&op_reply);
   mongoc_collection_read_command_with_opts (
      coll, distinct, NULL /* read prefs */, opts, &op_reply, &op_error);

   result_from_distinct (result, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_destroy (distinct);
   bson_destroy (opts);
   return ret;
}

static bool
operation_estimated_document_count (test_t *test,
                                    operation_t *op,
                                    result_t *result,
                                    bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   int64_t op_ret;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_val_t *val = NULL;
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bson_destroy (&op_reply);
   op_ret = mongoc_collection_estimated_document_count (
      coll, opts, NULL /* read prefs */, &op_reply, &op_error);

   if (op_ret != -1) {
      val = bson_val_from_int64 (op_ret);
   }

   result_from_val_and_reply (result, val, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_val_destroy (val);
   bson_destroy (opts);
   return ret;
}

static bool
operation_find (test_t *test,
                operation_t *op,
                result_t *result,
                bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   mongoc_cursor_t *cursor = NULL;
   bson_t *filter = NULL;
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   cursor = mongoc_collection_find_with_opts (
      coll, filter, opts, NULL /* read prefs */);

   result_from_cursor (result, cursor);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (opts);
   mongoc_cursor_destroy (cursor);
   return ret;
}

static bool
operation_find_one_and_update (test_t *test,
                               operation_t *op,
                               result_t *result,
                               bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *filter = NULL;
   char *return_document = NULL;
   mongoc_find_and_modify_opts_t *opts = NULL;
   mongoc_find_and_modify_flags_t flags = 0;
   bson_val_t *val = NULL;
   bson_iter_t iter;
   bson_t *session_opts = bson_new ();

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   bson_parser_utf8_optional (parser, "returnDocument", &return_document);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   opts = mongoc_find_and_modify_opts_new ();
   if (return_document && 0 == strcmp (return_document, "After")) {
      flags |= MONGOC_FIND_AND_MODIFY_RETURN_NEW;
   }
   mongoc_find_and_modify_opts_set_flags (opts, flags);
   mongoc_find_and_modify_opts_append (opts, bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, session_opts, error)) {
         goto done;
      }
   }
   mongoc_find_and_modify_opts_append (opts, session_opts);

   bson_destroy (&op_reply);
   mongoc_collection_find_and_modify_with_opts (
      coll, filter, opts, &op_reply, &op_error);

   if (bson_iter_init_find (&iter, &op_reply, "value")) {
      val = bson_val_from_iter (&iter);
   }
   result_from_val_and_reply (result, val, &op_reply, &op_error);

   ret = true;
done:
   bson_val_destroy (val);
   bson_parser_destroy_with_parsed_fields (parser);
   mongoc_find_and_modify_opts_destroy (opts);
   bson_destroy (&op_reply);
   bson_destroy (session_opts);
   return ret;
}

static bool
operation_find_one_and_replace (test_t *test,
                                operation_t *op,
                                result_t *result,
                                bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *filter = NULL;
   bson_t *replacement = NULL;
   char *return_document = NULL;
   mongoc_find_and_modify_opts_t *opts = NULL;
   mongoc_find_and_modify_flags_t flags = 0;
   bson_val_t *val = NULL;
   bson_iter_t iter;
   bson_t *session_opts = bson_new ();

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   bson_parser_doc (parser, "replacement", &replacement);
   bson_parser_utf8_optional (parser, "returnDocument", &return_document);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   opts = mongoc_find_and_modify_opts_new ();
   if (return_document && 0 == strcmp (return_document, "After")) {
      flags |= MONGOC_FIND_AND_MODIFY_RETURN_NEW;
   }
   mongoc_find_and_modify_opts_set_flags (opts, flags);
   mongoc_find_and_modify_opts_append (opts, bson_parser_get_extra (parser));
   mongoc_find_and_modify_opts_set_update (opts, replacement);
   if (op->session) {
      if (!mongoc_client_session_append (op->session, session_opts, error)) {
         goto done;
      }
   }
   mongoc_find_and_modify_opts_append (opts, session_opts);

   bson_destroy (&op_reply);
   mongoc_collection_find_and_modify_with_opts (
      coll, filter, opts, &op_reply, &op_error);

   if (bson_iter_init_find (&iter, &op_reply, "value")) {
      val = bson_val_from_iter (&iter);
   }
   result_from_val_and_reply (result, val, &op_reply, &op_error);

   ret = true;
done:
   bson_val_destroy (val);
   bson_parser_destroy_with_parsed_fields (parser);
   mongoc_find_and_modify_opts_destroy (opts);
   bson_destroy (&op_reply);
   bson_destroy (session_opts);
   return ret;
}

static bool
operation_find_one_and_delete (test_t *test,
                               operation_t *op,
                               result_t *result,
                               bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *filter = NULL;
   mongoc_find_and_modify_opts_t *opts = NULL;
   mongoc_find_and_modify_flags_t flags = 0;
   bson_val_t *val = NULL;
   bson_iter_t iter;
   bson_t *session_opts = bson_new ();

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   opts = mongoc_find_and_modify_opts_new ();
   flags |= MONGOC_FIND_AND_MODIFY_REMOVE;
   mongoc_find_and_modify_opts_set_flags (opts, flags);
   mongoc_find_and_modify_opts_append (opts, bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, session_opts, error)) {
         goto done;
      }
   }
   mongoc_find_and_modify_opts_append (opts, session_opts);

   bson_destroy (&op_reply);
   mongoc_collection_find_and_modify_with_opts (
      coll, filter, opts, &op_reply, &op_error);

   if (bson_iter_init_find (&iter, &op_reply, "value")) {
      val = bson_val_from_iter (&iter);
   }
   result_from_val_and_reply (result, val, &op_reply, &op_error);

   ret = true;
done:
   bson_val_destroy (val);
   bson_parser_destroy_with_parsed_fields (parser);
   mongoc_find_and_modify_opts_destroy (opts);
   bson_destroy (&op_reply);
   bson_destroy (session_opts);
   return ret;
}

static bool
operation_insert_many (test_t *test,
                       operation_t *op,
                       result_t *result,
                       bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *documents = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_iter_t iter;
   int n_docs = 0, i = 0;
   bson_t **docs = NULL;
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_array (parser, "documents", &documents);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   BSON_FOREACH (documents, iter)
   {
      n_docs++;
   }
   docs = bson_malloc0 (n_docs * sizeof (bson_t *));

   MONGOC_DEBUG ("ndocs=%d", n_docs);
   BSON_FOREACH (documents, iter)
   {
      bson_t doc;

      bson_iter_bson (&iter, &doc);
      docs[i] = bson_copy (&doc);
      i++;
   }

   bson_destroy (&op_reply);
   mongoc_collection_insert_many (
      coll, (const bson_t **) docs, n_docs, opts, &op_reply, &op_error);
   result_from_insert_many (result, &op_reply, &op_error);
   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   if (docs) {
      for (i = 0; i < n_docs; i++) {
         bson_destroy (docs[i]);
      }
   }
   bson_free (docs);
   bson_destroy (&op_reply);
   bson_destroy (opts);
   return ret;
}

static bool
operation_insert_one (test_t *test,
                      operation_t *op,
                      result_t *result,
                      bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_t *document = NULL;
   bson_parser_t *parser = NULL;
   bson_error_t op_error = {0};
   bson_t op_reply = BSON_INITIALIZER;
   bson_t *opts = bson_new ();

   parser = bson_parser_new ();
   bson_parser_doc (parser, "document", &document);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   bson_destroy (&op_reply);
   mongoc_collection_insert_one (coll, document, opts, &op_reply, &op_error);
   result_from_insert_one (result, &op_reply, &op_error);


   ret = true;
done:
   bson_destroy (&op_reply);
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (opts);
   return ret;
}

static bool
operation_replace_one (test_t *test,
                       operation_t *op,
                       result_t *result,
                       bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *filter = NULL;
   bson_t *replacement = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   bson_parser_doc (parser, "replacement", &replacement);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bson_destroy (&op_reply);
   mongoc_collection_replace_one (
      coll, filter, replacement, opts, &op_reply, &op_error);

   result_from_update_or_replace (result, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_destroy (opts);
   return ret;
}

static bool
operation_update_one (test_t *test,
                      operation_t *op,
                      result_t *result,
                      bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *filter = NULL;
   bson_t *update = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   bson_parser_array_or_doc (parser, "update", &update);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bson_destroy (&op_reply);
   mongoc_collection_update_one (
      coll, filter, update, opts, &op_reply, &op_error);

   result_from_update_or_replace (result, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_destroy (opts);
   return ret;
}

static bool
operation_update_many (test_t *test,
                       operation_t *op,
                       result_t *result,
                       bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *coll = NULL;
   bson_parser_t *parser = NULL;
   bson_t *filter = NULL;
   bson_t *update = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   bson_t *opts = NULL;

   parser = bson_parser_new ();
   bson_parser_allow_extra (parser, true);
   bson_parser_doc (parser, "filter", &filter);
   bson_parser_array_or_doc (parser, "update", &update);
   if (!bson_parser_parse (parser, op->arguments, error)) {
      goto done;
   }

   opts = bson_copy (bson_parser_get_extra (parser));
   if (op->session) {
      if (!mongoc_client_session_append (op->session, opts, error)) {
         goto done;
      }
   }

   coll = entity_map_get_collection (test->entity_map, op->object, error);
   if (!coll) {
      goto done;
   }

   bson_destroy (&op_reply);
   mongoc_collection_update_many (
      coll, filter, update, opts, &op_reply, &op_error);

   result_from_update_or_replace (result, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (parser);
   bson_destroy (&op_reply);
   bson_destroy (opts);
   return ret;
}

static bool
operation_iterate_until_document_or_error (test_t *test,
                                           operation_t *op,
                                           result_t *result,
                                           bson_error_t *error)
{
   bool ret = false;
   mongoc_change_stream_t *changestream = NULL;
   entity_findcursor_t *findcursor = NULL;
   const bson_t *doc = NULL;
   const bson_t *op_reply = NULL;
   bson_error_t op_error = {0};
   bson_val_t *val = NULL;
   entity_t *entity;

   entity = entity_map_get (test->entity_map, op->object, error);
   if (!entity) {
      goto done;
   }

   if (strcmp ("changestream", entity->type) == 0) {
      changestream =
         entity_map_get_changestream (test->entity_map, op->object, error);
      if (!changestream) {
         goto done;
      }

      /* Loop until error or document is returned. */
      while (!mongoc_change_stream_next (changestream, &doc)) {
         if (mongoc_change_stream_error_document (
                changestream, &op_error, &op_reply)) {
            break;
         }
      }
   } else {
      findcursor = entity_map_get_findcursor (test->entity_map, op->object, error);
      if (!findcursor) {
         goto done;
      }

      entity_findcursor_iterate_until_document_or_error (
         findcursor, &doc, &op_error, &op_reply);
   }

   if (NULL != doc) {
      val = bson_val_from_bson ((bson_t *) doc);
   }

   result_from_val_and_reply (result, val, (bson_t *) op_reply, &op_error);

   ret = true;
done:
   bson_val_destroy (val);
   return ret;
}

static bool
operation_close (test_t *test,
                 operation_t *op,
                 result_t *result,
                 bson_error_t *error)
{
   bool ret = false;
   entity_t *entity;
   entity = entity_map_get (test->entity_map, op->object, error);
   if (!entity) {
      goto done;
   }

   if (0 != strcmp (entity->type, "findcursor") &&
       0 != strcmp (entity->type, "changestream")) {
      test_set_error (
         error, "attempting to close an unsupported entity: %s", entity->type);
      goto done;
   }

   entity_map_delete (test->entity_map, op->object, error);

   result_from_ok (result);

   ret = true;
done:
   return ret;
}

static bool
operation_failpoint (test_t *test,
                     operation_t *op,
                     result_t *result,
                     bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *bp = NULL;
   char *client_id = NULL;
   mongoc_client_t *client = NULL;
   bson_t *failpoint = NULL;
   mongoc_read_prefs_t *rp = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};

   bp = bson_parser_new ();
   bson_parser_utf8 (bp, "client", &client_id);
   bson_parser_doc (bp, "failPoint", &failpoint);

   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   client = entity_map_get_client (test->entity_map, client_id, error);
   if (!client) {
      goto done;
   }

   rp = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   bson_destroy (&op_reply);
   mongoc_client_command_simple (
      client, "admin", failpoint, rp, &op_reply, &op_error);
   result_from_val_and_reply (result, NULL /* value */, &op_reply, &op_error);

   /* Add failpoint to list of test_t's known failpoints */
   register_failpoint (
      test,
      (char *) bson_lookup_utf8 (failpoint, "configureFailPoint"),
      client_id,
      0);

   ret = true;
done:
   mongoc_read_prefs_destroy (rp);
   bson_parser_destroy_with_parsed_fields (bp);
   bson_destroy (&op_reply);
   return ret;
}

static bool
operation_targeted_failpoint (test_t *test,
                              operation_t *op,
                              result_t *result,
                              bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *bp = NULL;
   char *client_id = NULL;
   mongoc_client_t *client = NULL;
   bson_t *failpoint = NULL;
   mongoc_read_prefs_t *rp = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   uint32_t server_id;

   bp = bson_parser_new ();
   bson_parser_doc (bp, "failPoint", &failpoint);

   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   if (!op->session) {
      test_set_error (error, "%s", "session unset");
      goto done;
   }

   client_id = entity_map_get_session_client_id (
      test->entity_map, op->session_id, error);
   if (!client_id) {
      goto done;
   }

   client = entity_map_get_client (test->entity_map, client_id, error);
   if (!client) {
      goto done;
   }

   server_id = mongoc_client_session_get_server_id (op->session);
   if (0 == server_id) {
      test_set_error (
         error, "expected session %s to be pinned but was not", op->session_id);
      goto done;
   }

   rp = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   bson_destroy (&op_reply);
   mongoc_client_command_simple_with_server_id (
      client, "admin", failpoint, rp, server_id, &op_reply, &op_error);
   result_from_val_and_reply (result, NULL /* value */, &op_reply, &op_error);

   /* Add failpoint to list of test_t's known failpoints */
   register_failpoint (
      test,
      (char *) bson_lookup_utf8 (failpoint, "configureFailPoint"),
      client_id,
      server_id);

   ret = true;
done:
   mongoc_read_prefs_destroy (rp);
   bson_parser_destroy_with_parsed_fields (bp);
   bson_destroy (&op_reply);
   return ret;
}

static bool
operation_delete (test_t *test,
                  operation_t *op,
                  result_t *result,
                  bson_error_t *error)
{
   bool ret = false;
   mongoc_gridfs_bucket_t *bucket = NULL;
   bson_parser_t *bp = NULL;
   bson_val_t *id = NULL;
   bson_error_t op_error = {0};

   bp = bson_parser_new ();
   bson_parser_any (bp, "id", &id);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   bucket = entity_map_get_bucket (test->entity_map, op->object, error);
   if (!bucket) {
      goto done;
   }

   mongoc_gridfs_bucket_delete_by_id (
      bucket, bson_val_to_value (id), &op_error);
   result_from_val_and_reply (result, NULL, NULL, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   return ret;
}

static bool
operation_download (test_t *test,
                    operation_t *op,
                    result_t *result,
                    bson_error_t *error)
{
   bool ret = false;
   mongoc_gridfs_bucket_t *bucket = NULL;
   bson_parser_t *bp = NULL;
   bson_val_t *id = NULL;
   bson_error_t op_error = {0};
   mongoc_stream_t *stream = NULL;
   uint8_t buf[256];
   ssize_t bytes_read = 0;
   mongoc_array_t all_bytes;
   bson_val_t *val = NULL;

   _mongoc_array_init (&all_bytes, sizeof (uint8_t));
   bp = bson_parser_new ();
   bson_parser_any (bp, "id", &id);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   bucket = entity_map_get_bucket (test->entity_map, op->object, error);
   if (!bucket) {
      goto done;
   }

   stream = mongoc_gridfs_bucket_open_download_stream (
      bucket, bson_val_to_value (id), &op_error);

   if (stream) {
      while ((bytes_read =
                 mongoc_stream_read (stream, buf, sizeof (buf), 1, 0)) > 0) {
         _mongoc_array_append_vals (&all_bytes, buf, bytes_read);
      }
      mongoc_gridfs_bucket_stream_error (stream, &op_error);
   }

   val = bson_val_from_bytes (all_bytes.data, all_bytes.len);
   result_from_val_and_reply (result, val, NULL, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   _mongoc_array_destroy (&all_bytes);
   bson_val_destroy (val);
   mongoc_stream_destroy (stream);
   return ret;
}

static bool
operation_upload (test_t *test,
                  operation_t *op,
                  result_t *result,
                  bson_error_t *error)
{
   bool ret = false;
   mongoc_gridfs_bucket_t *bucket = NULL;
   bson_parser_t *bp = NULL;
   bson_t *source = NULL;
   char *filename = NULL;
   mongoc_stream_t *stream = NULL;
   bson_value_t file_id;
   bson_error_t op_error = {0};
   bson_val_t *val = NULL;

   bp = bson_parser_new ();
   bson_parser_allow_extra (bp, true);
   bson_parser_doc (bp, "source", &source);
   bson_parser_utf8 (bp, "filename", &filename);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   bucket = entity_map_get_bucket (test->entity_map, op->object, error);
   if (!bucket) {
      goto done;
   }

   stream = mongoc_gridfs_bucket_open_upload_stream (
      bucket, filename, bson_parser_get_extra (bp), &file_id, &op_error);

   if (stream) {
      ssize_t total_written = 0;
      uint8_t *source_bytes;
      uint32_t source_bytes_len;
      bson_iter_t iter;

      if (!bson_iter_init_find (&iter, source, "$$hexBytes") ||
          !BSON_ITER_HOLDS_UTF8 (&iter)) {
         test_set_error (error, "$$hexBytes not found in source data");
         goto done;
      }

      source_bytes =
         hex_to_bin (bson_iter_utf8 (&iter, NULL), &source_bytes_len);
      while (total_written < source_bytes_len) {
         ssize_t bytes_written = 0;

         bytes_written =
            mongoc_stream_write (stream,
                                 source_bytes,
                                 (size_t) (source_bytes_len - total_written),
                                 0);
         if (bytes_written < 0) {
            break;
         }
         total_written += bytes_written;
      }
      mongoc_gridfs_bucket_stream_error (stream, &op_error);
      bson_free (source_bytes);
   }

   val = bson_val_from_value (&file_id);
   result_from_val_and_reply (result, val, NULL, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   mongoc_stream_destroy (stream);
   bson_val_destroy (val);
   return ret;
}

static bool
assert_session_dirty_helper (test_t *test,
                             operation_t *op,
                             result_t *result,
                             bool check_dirty,
                             bson_error_t *error)
{
   bool ret = false;

   if (!op->session) {
      test_set_error (error, "%s", "session unset");
      goto done;
   }

   if (check_dirty != mongoc_client_session_get_dirty (op->session)) {
      test_set_error (error,
                      "expected session to%s be dirty but was not",
                      check_dirty ? "" : " not");
      goto done;
   }

   result_from_ok (result);

   ret = true;
done:
   return ret;
}
static bool
operation_assert_session_not_dirty (test_t *test,
                                    operation_t *op,
                                    result_t *result,
                                    bson_error_t *error)
{
   return assert_session_dirty_helper (test, op, result, false, error);
}

static bool
operation_assert_session_dirty (test_t *test,
                                operation_t *op,
                                result_t *result,
                                bson_error_t *error)
{
   return assert_session_dirty_helper (test, op, result, true, error);
}

static event_t *
next_started_event (event_t *iter)
{
   if (!iter) {
      return NULL;
   }

   iter = iter->next;
   while (iter && 0 != strcmp (iter->type, "commandStartedEvent")) {
      iter = iter->next;
   }
   return iter;
}

static bool
assert_lsid_on_last_two_commands (test_t *test,
                                  operation_t *op,
                                  result_t *result,
                                  bool check_same,
                                  bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *bp = NULL;
   char *client_id = NULL;
   entity_t *entity = NULL;
   event_t *a, *b;
   bson_iter_t iter;
   bson_t a_lsid;
   bson_t b_lsid;

   bp = bson_parser_new ();
   bson_parser_utf8 (bp, "client", &client_id);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   entity = entity_map_get (test->entity_map, client_id, error);
   if (!entity) {
      goto done;
   }

   if (0 != strcmp (entity->type, "client")) {
      goto done;
   }

   a = NULL;
   b = entity->events;
   if (b && 0 != strcmp (b->type, "commandStartedEvent")) {
      b = next_started_event (b);
   }

   while (next_started_event (b) != NULL) {
      a = b;
      b = next_started_event (b);
   }

   if (NULL == a || NULL == b) {
      test_set_error (error,
                      "unable to find two commandStartedEvents on client: %s",
                      client_id);
      goto done;
   }

   if (!bson_iter_init_find (&iter, a->command, "lsid")) {
      test_set_error (
         error,
         "unable to find lsid in second to last commandStartedEvent: %s",
         tmp_json (a->command));
      goto done;
   }

   bson_iter_bson (&iter, &a_lsid);

   if (!bson_iter_init_find (&iter, b->command, "lsid")) {
      test_set_error (
         error,
         "unable to find lsid in second to last commandStartedEvent: %s",
         tmp_json (b->command));
      goto done;
   }

   bson_iter_bson (&iter, &b_lsid);

   if (check_same != bson_equal (&a_lsid, &b_lsid)) {
      test_set_error (error,
                      "expected $lsid's to be%s equal, but got: %s and %s",
                      check_same ? "" : " not",
                      tmp_json (&a_lsid),
                      tmp_json (&b_lsid));
      goto done;
   }

   result_from_ok (result);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   return ret;
}

static bool
operation_assert_same_lsid_on_last_two_commands (test_t *test,
                                                 operation_t *op,
                                                 result_t *result,
                                                 bson_error_t *error)
{
   return assert_lsid_on_last_two_commands (test, op, result, true, error);
}

static bool
operation_assert_different_lsid_on_last_two_commands (test_t *test,
                                                      operation_t *op,
                                                      result_t *result,
                                                      bson_error_t *error)
{
   return assert_lsid_on_last_two_commands (test, op, result, false, error);
}

static bool
operation_end_session (test_t *test,
                       operation_t *op,
                       result_t *result,
                       bson_error_t *error)
{
   bool ret = false;

   if (!entity_map_end_session (test->entity_map, op->object, error)) {
      goto done;
   }
   result_from_ok (result);

   ret = true;
done:
   return ret;
}

static bool
operation_start_transaction (test_t *test,
                             operation_t *op,
                             result_t *result,
                             bson_error_t *error)
{
   bool ret = false;
   mongoc_client_session_t *session = NULL;
   bson_error_t op_error = {0};
   mongoc_transaction_opt_t *opts = NULL;
   mongoc_read_concern_t *rc = NULL;
   mongoc_write_concern_t *wc = NULL;
   mongoc_read_prefs_t *rp = NULL;
   bson_parser_t *bp = NULL;

   opts = mongoc_transaction_opts_new ();
   bp = bson_parser_new ();
   bson_parser_read_concern_optional (bp, &rc);
   bson_parser_write_concern_optional (bp, &wc);
   bson_parser_read_prefs_optional (bp, &rp);
   if (rc) {
      mongoc_transaction_opts_set_read_concern (opts, rc);
   }

   if (wc) {
      mongoc_transaction_opts_set_write_concern (opts, wc);
   }

   if (rp) {
      mongoc_transaction_opts_set_read_prefs (opts, rp);
   }

   session = entity_map_get_session (test->entity_map, op->object, error);
   if (!session) {
      goto done;
   }

   mongoc_client_session_start_transaction (session, opts, &op_error);
   result_from_val_and_reply (result, NULL, NULL, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   mongoc_transaction_opts_destroy (opts);
   return ret;
}

const char *
transaction_state_to_string (mongoc_transaction_state_t tstate)
{
   switch (tstate) {
   case MONGOC_TRANSACTION_NONE:
      return "none";
   case MONGOC_TRANSACTION_STARTING:
      return "starting";
   case MONGOC_TRANSACTION_IN_PROGRESS:
      return "in_progress";
   case MONGOC_TRANSACTION_COMMITTED:
      return "committed";
   case MONGOC_TRANSACTION_ABORTED:
      return "aborted";
   default:
      return "invalid";
   }
}

static bool
operation_assert_session_transaction_state (test_t *test,
                                            operation_t *op,
                                            result_t *result,
                                            bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *bp = NULL;
   char *expected = NULL;
   const char *actual;
   mongoc_transaction_state_t state;

   bp = bson_parser_new ();
   bson_parser_utf8 (bp, "state", &expected);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   if (!op->session) {
      test_set_error (error, "expected session");
      goto done;
   }

   state = mongoc_client_session_get_transaction_state (op->session);
   actual = transaction_state_to_string (state);
   if (0 != strcmp (expected, actual)) {
      test_set_error (
         error, "expected state: %s, got state: %s", expected, actual);
      goto done;
   }

   result_from_ok (result);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   return ret;
}

static bool
assert_collection_exists (test_t *test,
                          operation_t *op,
                          result_t *result,
                          bool expect_exist,
                          bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *bp = NULL;
   char *collection_name = NULL;
   char *database_name = NULL;
   mongoc_database_t *db = NULL;
   bson_error_t op_error = {0};
   bool actual_exist = false;
   char **head = NULL, **iter = NULL;

   bp = bson_parser_new ();
   bson_parser_utf8 (bp, "collectionName", &collection_name);
   bson_parser_utf8 (bp, "databaseName", &database_name);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   db = mongoc_client_get_database (
      test->test_file->test_runner->internal_client, database_name);
   head = mongoc_database_get_collection_names_with_opts (db, NULL, &op_error);
   for (iter = head; *iter; iter++) {
      if (0 == strcmp (*iter, collection_name)) {
         actual_exist = true;
         break;
      }
   }

   if (expect_exist != actual_exist) {
      test_set_error (error,
                      "expected collection %s %s exist but %s",
                      collection_name,
                      expect_exist ? "to" : "to not",
                      expect_exist ? "did not" : "did");
      goto done;
   }

   result_from_val_and_reply (result, NULL, NULL, &op_error);

   ret = true;
done:
   bson_strfreev (head);
   mongoc_database_destroy (db);
   bson_parser_destroy_with_parsed_fields (bp);
   return ret;
}

static bool
operation_assert_collection_exists (test_t *test,
                                    operation_t *op,
                                    result_t *result,
                                    bson_error_t *error)
{
   return assert_collection_exists (test, op, result, true, error);
}

static bool
operation_assert_collection_not_exists (test_t *test,
                                        operation_t *op,
                                        result_t *result,
                                        bson_error_t *error)
{
   return assert_collection_exists (test, op, result, false, error);
}

static bool
operation_commit_transaction (test_t *test,
                              operation_t *op,
                              result_t *result,
                              bson_error_t *error)
{
   bool ret = false;
   mongoc_client_session_t *session = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};

   session = entity_map_get_session (test->entity_map, op->object, error);
   if (!session) {
      goto done;
   }

   bson_destroy (&op_reply);
   mongoc_client_session_commit_transaction (session, &op_reply, &op_error);
   result_from_val_and_reply (result, NULL, &op_reply, &op_error);

   ret = true;
done:
   bson_destroy (&op_reply);
   return ret;
}

static bool
operation_abort_transaction (test_t *test,
                             operation_t *op,
                             result_t *result,
                             bson_error_t *error)
{
   bool ret = false;
   mongoc_client_session_t *session = NULL;
   bson_error_t op_error = {0};

   session = entity_map_get_session (test->entity_map, op->object, error);
   if (!session) {
      goto done;
   }

   mongoc_client_session_abort_transaction (session, &op_error);
   result_from_val_and_reply (result, NULL, NULL, &op_error);

   ret = true;
done:
   return ret;
}

static bool
assert_index_exists (test_t *test,
                     operation_t *op,
                     result_t *result,
                     bool expect_exist,
                     bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *bp = NULL;
   char *collection_name = NULL;
   char *database_name = NULL;
   char *index_name = NULL;
   mongoc_collection_t *coll = NULL;
   bson_error_t op_error = {0};
   bool actual_exist = false;
   mongoc_cursor_t *cursor = NULL;
   const bson_t *index;
   const bson_t *error_doc = NULL;

   bp = bson_parser_new ();
   bson_parser_utf8 (bp, "collectionName", &collection_name);
   bson_parser_utf8 (bp, "databaseName", &database_name);
   bson_parser_utf8 (bp, "indexName", &index_name);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }

   coll = mongoc_client_get_collection (
      test->test_file->test_runner->internal_client,
      database_name,
      collection_name);
   cursor = mongoc_collection_find_indexes_with_opts (coll, NULL);
   while (mongoc_cursor_next (cursor, &index)) {
      bson_iter_t iter;

      if (!bson_iter_init_find (&iter, index, "name")) {
         continue;
      }

      if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
         continue;
      }

      if (0 != strcmp (bson_iter_utf8 (&iter, NULL), index_name)) {
         continue;
      }

      actual_exist = true;
      break;
   }

   mongoc_cursor_error_document (cursor, &op_error, &error_doc);

   if (expect_exist != actual_exist) {
      test_set_error (error,
                      "expected index %s %s exist but %s",
                      index_name,
                      expect_exist ? "to" : "to not",
                      expect_exist ? "did not" : "did");
      goto done;
   }

   result_from_val_and_reply (result, NULL, (bson_t *) error_doc, &op_error);

   ret = true;
done:
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (coll);
   bson_parser_destroy_with_parsed_fields (bp);
   return ret;
}

static bool
operation_assert_index_exists (test_t *test,
                               operation_t *op,
                               result_t *result,
                               bson_error_t *error)
{
   return assert_index_exists (test, op, result, true, error);
}

static bool
operation_assert_index_not_exists (test_t *test,
                                   operation_t *op,
                                   result_t *result,
                                   bson_error_t *error)
{
   return assert_index_exists (test, op, result, false, error);
}

typedef struct {
   test_t *test;
   bson_t *ops;
} txn_ctx_t;

static bool
with_transaction_cb (mongoc_client_session_t *session,
                     void *ctx,
                     bson_t **reply,
                     bson_error_t *error)
{
   bool ret = false;
   bson_iter_t iter;
   txn_ctx_t *tctx = NULL;

   tctx = (txn_ctx_t *) ctx;

   BSON_FOREACH (tctx->ops, iter)
   {
      bson_t op_bson;

      bson_iter_bson (&iter, &op_bson);
      MONGOC_DEBUG ("in with_transaction_cb running: %s", tmp_json (&op_bson));
      if (!operation_run (tctx->test, &op_bson, error)) {
         goto done;
      }
   }

   ret = true;
done:
   return ret;
}

static bool
operation_with_transaction (test_t *test,
                            operation_t *op,
                            result_t *result,
                            bson_error_t *error)
{
   bool ret = false;
   bson_parser_t *bp = NULL;
   mongoc_client_session_t *session = NULL;
   bson_t op_reply = BSON_INITIALIZER;
   bson_error_t op_error = {0};
   mongoc_read_concern_t *rc = NULL;
   mongoc_write_concern_t *wc = NULL;
   mongoc_read_prefs_t *rp = NULL;
   mongoc_transaction_opt_t *topts = NULL;
   txn_ctx_t tctx;

   session = entity_map_get_session (test->entity_map, op->object, error);
   if (!session) {
      goto done;
   }

   topts = mongoc_transaction_opts_new ();

   bp = bson_parser_new ();
   bson_parser_array (bp, "callback", &tctx.ops);
   bson_parser_read_concern_optional (bp, &rc);
   bson_parser_write_concern_optional (bp, &wc);
   bson_parser_read_prefs_optional (bp, &rp);
   if (!bson_parser_parse (bp, op->arguments, error)) {
      goto done;
   }
   if (rc) {
      mongoc_transaction_opts_set_read_concern (topts, rc);
   }

   if (wc) {
      mongoc_transaction_opts_set_write_concern (topts, wc);
   }

   if (rp) {
      mongoc_transaction_opts_set_read_prefs (topts, rp);
   }

   tctx.test = test;
   bson_destroy (&op_reply);
   mongoc_client_session_with_transaction (
      session, with_transaction_cb, topts, &tctx, &op_reply, &op_error);

   result_from_val_and_reply (result, NULL, &op_reply, &op_error);

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   mongoc_transaction_opts_destroy (topts);
   bson_destroy (&op_reply);
   return ret;
}

static bool
assert_session_pinned (test_t *test,
                       operation_t *op,
                       result_t *result,
                       bool expect_pinned,
                       bson_error_t *error)
{
   bool ret = false;
   bool actual_pinned = false;

   if (!op->session) {
      test_set_error (error, "%s", "expected session to be set");
      goto done;
   }

   if (0 != mongoc_client_session_get_server_id (op->session)) {
      actual_pinned = true;
   }

   if (actual_pinned != expect_pinned) {
      test_set_error (error,
                      "expected session to be %s but got %s",
                      expect_pinned ? "pinned" : "unpinned",
                      expect_pinned ? "unpinned" : "pinnned");
      goto done;
   }

   result_from_ok (result);
   ret = true;
done:
   return ret;
}

static bool
operation_assert_session_pinned (test_t *test,
                                 operation_t *op,
                                 result_t *result,
                                 bson_error_t *error)
{
   return assert_session_pinned (test, op, result, true, error);
}

static bool
operation_assert_session_unpinned (test_t *test,
                                   operation_t *op,
                                   result_t *result,
                                   bson_error_t *error)
{
   return assert_session_pinned (test, op, result, false, error);
}

static bool
operation_loop (test_t *test,
                operation_t *op,
                result_t *result,
                bson_error_t *error)
{
   /* TODO: CDRIVER-3867 Comprehensive Atlas Testing */
   test_set_error (error, "Loop operation not implemented");
   return false;
}

static bool
operation_assert_number_connections_checked_out (test_t *test,
                                                 operation_t *op,
                                                 result_t *result,
                                                 bson_error_t *error)
{
   /* "This operation only applies to drivers that implement connection pooling
    * and should be skipped for drivers that do not."
    * TODO: (CDRIVER-3525) add this assertion when CMAP is implemented. */
   result_from_ok (result);
   return true;
}

typedef struct {
   const char *op;
   bool (*fn) (test_t *, operation_t *, result_t *, bson_error_t *);
} op_to_fn_t;

bool
operation_run (test_t *test, bson_t *op_bson, bson_error_t *error)
{
   operation_t *op = NULL;
   result_t *result = NULL;
   int i, num_ops;
   bool check_result = true;
   op_to_fn_t op_to_fn_map[] = {
      /* Client operations */
      {"createChangeStream", operation_create_change_stream},
      {"listDatabases", operation_list_databases},

      /* Database operations */
      {"createCollection", operation_create_collection},
      {"dropCollection", operation_drop_collection},
      {"listCollections", operation_list_collections},
      {"listCollectionNames", operation_list_collection_names},
      {"listIndexes", operation_list_indexes},
      {"runCommand", operation_run_command},

      /* Collection operations */
      {"aggregate", operation_aggregate},
      {"bulkWrite", operation_bulk_write},
      {"countDocuments", operation_count_documents},
      {"createFindCursor", operation_create_find_cursor},
      {"createIndex", operation_create_index},
      {"deleteOne", operation_delete_one},
      {"deleteMany", operation_delete_many},
      {"distinct", operation_distinct},
      {"estimatedDocumentCount", operation_estimated_document_count},
      {"find", operation_find},
      {"findOneAndDelete", operation_find_one_and_delete},
      {"findOneAndReplace", operation_find_one_and_replace},
      {"findOneAndUpdate", operation_find_one_and_update},
      {"insertMany", operation_insert_many},
      {"insertOne", operation_insert_one},
      {"replaceOne", operation_replace_one},
      {"updateOne", operation_update_one},
      {"updateMany", operation_update_many},

      /* Change stream and cursor operations */
      {"iterateUntilDocumentOrError",
       operation_iterate_until_document_or_error},
      {"close", operation_close},

      /* Test runner operations */
      {"failPoint", operation_failpoint},
      {"targetedFailPoint", operation_targeted_failpoint},
      {"assertSessionDirty", operation_assert_session_dirty},
      {"assertSessionNotDirty", operation_assert_session_not_dirty},
      {"assertSameLsidOnLastTwoCommands",
       operation_assert_same_lsid_on_last_two_commands},
      {"assertDifferentLsidOnLastTwoCommands",
       operation_assert_different_lsid_on_last_two_commands},
      {"assertSessionTransactionState",
       operation_assert_session_transaction_state},
      {"assertCollectionNotExists", operation_assert_collection_not_exists},
      {"assertCollectionExists", operation_assert_collection_exists},
      {"assertIndexNotExists", operation_assert_index_not_exists},
      {"assertIndexExists", operation_assert_index_exists},
      {"assertSessionPinned", operation_assert_session_pinned},
      {"assertSessionUnpinned", operation_assert_session_unpinned},
      {"loop", operation_loop},
      {"assertNumberConnectionsCheckedOut",
       operation_assert_number_connections_checked_out},

      /* GridFS operations */
      {"delete", operation_delete},
      {"download", operation_download},
      {"upload", operation_upload},

      /* Session operations. */
      {"endSession", operation_end_session},
      {"startTransaction", operation_start_transaction},
      {"commitTransaction", operation_commit_transaction},
      {"withTransaction", operation_with_transaction},
      {"abortTransaction", operation_abort_transaction},

   };
   bool ret = false;

   op = operation_new (op_bson, error);
   if (!op) {
      goto done;
   }

   /* Check for a "session" argument in all operations, it can be
    * an argument for any operation. */
   if (op->arguments && bson_has_field (op->arguments, "session")) {
      bson_t copied;
      mongoc_client_session_t *session = NULL;

      op->session_id =
         bson_strdup (bson_lookup_utf8 (op->arguments, "session"));
      session =
         entity_map_get_session (test->entity_map, op->session_id, error);

      if (!session) {
         goto done;
      }

      bson_copy_to_excluding (op->arguments, &copied, "session", NULL);
      bson_destroy (op->arguments);
      op->arguments = bson_copy (&copied);
      bson_destroy (&copied);
      op->session = session;
   }

   MONGOC_DEBUG ("running operation: %s", tmp_json (op_bson));

   num_ops = sizeof (op_to_fn_map) / sizeof (op_to_fn_t);
   result = result_new ();

   if (op->ignore_result_and_error && *op->ignore_result_and_error) {
      check_result = false;
   }

   for (i = 0; i < num_ops; i++) {
      if (0 == strcmp (op->name, op_to_fn_map[i].op)) {
         if (!op_to_fn_map[i].fn (test, op, result, error)) {
            goto done;
         }
         if (check_result && !result_check (result,
                                            test->entity_map,
                                            op->expect_result,
                                            op->expect_error,
                                            error)) {
            test_diagnostics_error_info (
               "checking for result (%s) / error (%s)",
               bson_val_to_json (op->expect_result),
               tmp_json (op->expect_error));
            goto done;
         }
         if (result_get_val (result) != NULL && op->save_result_as_entity) {
            if (!entity_map_add_bson (test->entity_map,
                                      op->save_result_as_entity,
                                      result_get_val (result),
                                      error)) {
               goto done;
            }
         }
         break;
      }
   }

   if (i == num_ops) {
      test_set_error (error, "unrecognized operation: %s", op->name);
      goto done;
   }

   ret = true;
done:
   operation_destroy (op);
   result_destroy (result);
   return ret;
}
