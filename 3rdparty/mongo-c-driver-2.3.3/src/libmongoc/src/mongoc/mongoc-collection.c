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


#include <mongoc/mongoc-collection.h>

#include <common-bson-dsl-private.h>
#include <common-macros-private.h> // BEGIN_IGNORE_DEPRECATIONS
#include <common-string-private.h>
#include <mongoc/mongoc-aggregate-private.h>
#include <mongoc/mongoc-bulk-operation-private.h>
#include <mongoc/mongoc-change-stream-private.h>
#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-collection-private.h>
#include <mongoc/mongoc-cursor-private.h>
#include <mongoc/mongoc-database-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-find-and-modify-private.h>
#include <mongoc/mongoc-opts-private.h>
#include <mongoc/mongoc-read-concern-private.h>
#include <mongoc/mongoc-read-prefs-private.h>
#include <mongoc/mongoc-trace-private.h>
#include <mongoc/mongoc-util-private.h>
#include <mongoc/mongoc-write-command-private.h>
#include <mongoc/mongoc-write-concern-private.h>

#include <mongoc/mongoc-bulk-operation.h>
#include <mongoc/mongoc-find-and-modify.h>
#include <mongoc/mongoc-log.h>

#include <bson/bson.h>

#include <stdio.h>

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "collection"

static void
_mongoc_collection_write_command_execute(mongoc_write_command_t *command,
                                         const mongoc_collection_t *collection,
                                         const mongoc_write_concern_t *write_concern,
                                         mongoc_client_session_t *cs,
                                         mongoc_write_result_t *result)
{
   mongoc_server_stream_t *server_stream;

   ENTRY;

   const mongoc_ss_log_context_t ss_log_context = {.operation = _mongoc_write_command_get_name(command),
                                                   .has_operation_id = true,
                                                   .operation_id = command->operation_id};
   server_stream =
      mongoc_cluster_stream_for_writes(&collection->client->cluster, &ss_log_context, cs, NULL, NULL, &result->error);

   if (!server_stream) {
      /* result->error has been filled out */
      EXIT;
   }

   _mongoc_write_command_execute(command,
                                 collection->client,
                                 server_stream,
                                 collection->db,
                                 collection->collection,
                                 write_concern,
                                 0 /* offset */,
                                 cs,
                                 result);

   mongoc_server_stream_cleanup(server_stream);

   EXIT;
}


static void
_mongoc_collection_write_command_execute_idl(mongoc_write_command_t *command,
                                             const mongoc_collection_t *collection,
                                             mongoc_crud_opts_t *crud,
                                             mongoc_write_result_t *result)
{
   mongoc_server_stream_t *server_stream;
   bson_t reply;

   ENTRY;

   const mongoc_ss_log_context_t ss_log_context = {.operation = _mongoc_write_command_get_name(command),
                                                   .has_operation_id = true,
                                                   .operation_id = command->operation_id};
   server_stream = mongoc_cluster_stream_for_writes(
      &collection->client->cluster, &ss_log_context, crud->client_session, NULL, &reply, &result->error);

   if (!server_stream) {
      /* result->error and reply have been filled out */
      _mongoc_bson_array_copy_labels_to(&reply, &result->errorLabels);
      bson_destroy(&reply);
      EXIT;
   }

   if (_mongoc_client_session_in_txn(crud->client_session) && crud->writeConcern) {
      _mongoc_set_error(&result->error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "Cannot set write concern after starting transaction");
      mongoc_server_stream_cleanup(server_stream);
      EXIT;
   }

   if (!crud->writeConcern && !_mongoc_client_session_in_txn(crud->client_session)) {
      crud->writeConcern = collection->write_concern;
      crud->write_concern_owned = false;
   }

   _mongoc_write_command_execute_idl(
      command, collection->client, server_stream, collection->db, collection->collection, 0 /* offset */, crud, result);

   mongoc_server_stream_cleanup(server_stream);

   EXIT;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_collection_new --
 *
 *       INTERNAL API
 *
 *       Create a new mongoc_collection_t structure for the given client.
 *
 *       @client must remain valid during the lifetime of this structure.
 *       @db is the db name of the collection.
 *       @collection is the name of the collection.
 *       @read_prefs is the default read preferences to apply or NULL.
 *       @read_concern is the default read concern to apply or NULL.
 *       @write_concern is the default write concern to apply or NULL.
 *
 * Returns:
 *       A newly allocated mongoc_collection_t that should be freed with
 *       mongoc_collection_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_collection_t *
_mongoc_collection_new(mongoc_client_t *client,
                       const char *db,
                       const char *collection,
                       const mongoc_read_prefs_t *read_prefs,
                       const mongoc_read_concern_t *read_concern,
                       const mongoc_write_concern_t *write_concern)
{
   mongoc_collection_t *col;

   ENTRY;

   BSON_ASSERT_PARAM(client);
   BSON_ASSERT_PARAM(db);
   BSON_ASSERT_PARAM(collection);

   col = (mongoc_collection_t *)bson_malloc0(sizeof *col);
   col->client = client;
   col->write_concern = write_concern ? mongoc_write_concern_copy(write_concern) : mongoc_write_concern_new();
   col->read_concern = read_concern ? mongoc_read_concern_copy(read_concern) : mongoc_read_concern_new();
   col->read_prefs = read_prefs ? mongoc_read_prefs_copy(read_prefs) : mongoc_read_prefs_new(MONGOC_READ_PRIMARY);

   col->ns = bson_strdup_printf("%s.%s", db, collection);
   col->db = bson_strdup(db);
   col->collection = bson_strdup(collection);

   col->collectionlen = (uint32_t)strlen(col->collection);
   col->nslen = (uint32_t)strlen(col->ns);

   col->gle = NULL;

   RETURN(col);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_destroy --
 *
 *       Release resources associated with @collection and frees the
 *       structure.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_collection_destroy(mongoc_collection_t *collection) /* IN */
{
   ENTRY;

   if (!collection) {
      EXIT;
   }

   bson_clear(&collection->gle);

   if (collection->read_prefs) {
      mongoc_read_prefs_destroy(collection->read_prefs);
      collection->read_prefs = NULL;
   }

   if (collection->read_concern) {
      mongoc_read_concern_destroy(collection->read_concern);
      collection->read_concern = NULL;
   }

   if (collection->write_concern) {
      mongoc_write_concern_destroy(collection->write_concern);
      collection->write_concern = NULL;
   }

   bson_free(collection->collection);
   bson_free(collection->db);
   bson_free(collection->ns);
   bson_free(collection);

   EXIT;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_copy --
 *
 *       Returns a copy of @collection that needs to be freed by calling
 *       mongoc_collection_destroy.
 *
 * Returns:
 *       A copy of this collection.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_collection_t *
mongoc_collection_copy(mongoc_collection_t *collection) /* IN */
{
   ENTRY;

   BSON_ASSERT_PARAM(collection);

   RETURN(_mongoc_collection_new(collection->client,
                                 collection->db,
                                 collection->collection,
                                 collection->read_prefs,
                                 collection->read_concern,
                                 collection->write_concern));
}


mongoc_cursor_t *
mongoc_collection_aggregate(mongoc_collection_t *collection,       /* IN */
                            mongoc_query_flags_t flags,            /* IN */
                            const bson_t *pipeline,                /* IN */
                            const bson_t *opts,                    /* IN */
                            const mongoc_read_prefs_t *read_prefs) /* IN */
{
   return _mongoc_aggregate(collection->client,
                            collection->ns,
                            flags,
                            pipeline,
                            opts,
                            read_prefs,
                            collection->read_prefs,
                            collection->read_concern,
                            collection->write_concern);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_find_with_opts --
 *
 *       Create a cursor with a query filter. All other options are
 *       specified in a free-form BSON document.
 *
 * Parameters:
 *       @collection: A mongoc_collection_t.
 *       @filter: The query to locate matching documents.
 *       @opts: Other options.
 *       @read_prefs: Optional read preferences to choose cluster node.
 *
 * Returns:
 *       A newly allocated mongoc_cursor_t that should be freed with
 *       mongoc_cursor_destroy().
 *
 *       The client used by mongoc_collection_t must be valid for the
 *       lifetime of the resulting mongoc_cursor_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

mongoc_cursor_t *
mongoc_collection_find_with_opts(mongoc_collection_t *collection,
                                 const bson_t *filter,
                                 const bson_t *opts,
                                 const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(filter);

   bson_clear(&collection->gle);

   return _mongoc_cursor_find_new(
      collection->client, collection->ns, filter, opts, read_prefs, collection->read_prefs, collection->read_concern);
}


bool
mongoc_collection_read_command_with_opts(mongoc_collection_t *collection,
                                         const bson_t *command,
                                         const mongoc_read_prefs_t *read_prefs,
                                         const bson_t *opts,
                                         bson_t *reply,
                                         bson_error_t *error)
{
   BSON_ASSERT_PARAM(collection);

   return _mongoc_client_command_with_opts(collection->client,
                                           collection->db,
                                           command,
                                           MONGOC_CMD_READ,
                                           opts,
                                           MONGOC_QUERY_NONE,
                                           read_prefs,
                                           collection->read_prefs,
                                           collection->read_concern,
                                           collection->write_concern,
                                           reply,
                                           error);
}


bool
mongoc_collection_write_command_with_opts(
   mongoc_collection_t *collection, const bson_t *command, const bson_t *opts, bson_t *reply, bson_error_t *error)
{
   BSON_ASSERT_PARAM(collection);

   return _mongoc_client_command_with_opts(collection->client,
                                           collection->db,
                                           command,
                                           MONGOC_CMD_WRITE,
                                           opts,
                                           MONGOC_QUERY_NONE,
                                           NULL,
                                           collection->read_prefs,
                                           collection->read_concern,
                                           collection->write_concern,
                                           reply,
                                           error);
}


bool
mongoc_collection_read_write_command_with_opts(mongoc_collection_t *collection,
                                               const bson_t *command,
                                               const mongoc_read_prefs_t *read_prefs /* IGNORED */,
                                               const bson_t *opts,
                                               bson_t *reply,
                                               bson_error_t *error)
{
   BSON_ASSERT_PARAM(collection);

   return _mongoc_client_command_with_opts(collection->client,
                                           collection->db,
                                           command,
                                           MONGOC_CMD_RW,
                                           opts,
                                           MONGOC_QUERY_NONE,
                                           read_prefs,
                                           collection->read_prefs,
                                           collection->read_concern,
                                           collection->write_concern,
                                           reply,
                                           error);
}


bool
mongoc_collection_command_with_opts(mongoc_collection_t *collection,
                                    const bson_t *command,
                                    const mongoc_read_prefs_t *read_prefs,
                                    const bson_t *opts,
                                    bson_t *reply,
                                    bson_error_t *error)
{
   BSON_ASSERT_PARAM(collection);

   /* Server Selection Spec: "The generic command method has a default read
    * preference of mode 'primary'. The generic command method MUST ignore any
    * default read preference from client, database or collection
    * configuration. The generic command method SHOULD allow an optional read
    * preference argument." */

   return _mongoc_client_command_with_opts(collection->client,
                                           collection->db,
                                           command,
                                           MONGOC_CMD_RAW,
                                           opts,
                                           MONGOC_QUERY_NONE,
                                           read_prefs,
                                           NULL /* default prefs */,
                                           collection->read_concern,
                                           collection->write_concern,
                                           reply,
                                           error);
}


bool
mongoc_collection_command_simple(mongoc_collection_t *collection,
                                 const bson_t *command,
                                 const mongoc_read_prefs_t *read_prefs,
                                 bson_t *reply,
                                 bson_error_t *error)
{
   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(command);

   bson_clear(&collection->gle);

   /* Server Selection Spec: "The generic command method has a default read
    * preference of mode 'primary'. The generic command method MUST ignore any
    * default read preference from client, database or collection
    * configuration. The generic command method SHOULD allow an optional read
    * preference argument."
    */

   return _mongoc_client_command_with_opts(collection->client,
                                           collection->db,
                                           command,
                                           MONGOC_CMD_RAW,
                                           NULL /* opts */,
                                           MONGOC_QUERY_NONE,
                                           read_prefs,
                                           NULL /* default prefs */,
                                           NULL /* read concern */,
                                           NULL /* write concern */,
                                           reply,
                                           error);
}


int64_t
mongoc_collection_estimated_document_count(mongoc_collection_t *coll,
                                           const bson_t *opts,
                                           const mongoc_read_prefs_t *read_prefs,
                                           bson_t *reply,
                                           bson_error_t *error)
{
   ENTRY;

   BSON_ASSERT_PARAM(coll);

   // No sessionId allowed
   if (opts && bson_has_field(opts, "sessionId")) {
      _mongoc_set_error(error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "Collection count must not specify explicit session");
      RETURN(-1);
   }

   // Storage for the reply if no storage was given by caller
   bson_t reply_local = BSON_INITIALIZER;
   // Write the reply to either the caller's storage or a local variable
   bson_t *const reply_ptr = reply ? reply : &reply_local;

   // Create and execute a "count" command
   bsonBuildDecl(cmd, kv("count", cstr(coll->collection)));
   const bool command_ok = _mongoc_client_command_with_opts(coll->client,
                                                            coll->db,
                                                            &cmd,
                                                            MONGOC_CMD_READ,
                                                            opts,
                                                            MONGOC_QUERY_NONE,
                                                            read_prefs,
                                                            coll->read_prefs,
                                                            coll->read_concern,
                                                            coll->write_concern,
                                                            reply_ptr,
                                                            error);
   bson_destroy(&cmd);

   // Extract the "n" field from the response
   int64_t ret_count = -1;
   if (command_ok) {
      bsonParse(*reply_ptr, find(key("n"), do(ret_count = bson_iter_as_int64(&bsonVisitIter))));
   }
   // Destroy the local storage. This is a no-op if we used the caller's storage.
   bson_destroy(&reply_local);

   RETURN(ret_count);
}


/* --------------------------------------------------------------------------
 *
 * _make_aggregate_for_count --
 *
 *       Construct an aggregate pipeline with the following form:
 *       { pipeline: [
 *           { $match: {...} },
 *           { $group: { _id: 1, n: { sum: 1 } } },
 *           { $skip: ... },
 *           { $limit: ... }
 *         ]
 *       }
 *
 *--------------------------------------------------------------------------
 */
static void
_make_aggregate_for_count(const mongoc_collection_t *coll,
                          const bson_t *filter,
                          mongoc_count_document_opts_t *opts,
                          bson_t *out)
{
   bson_array_builder_t *pipeline;
   bson_t match_stage;
   bson_t group_stage;
   bson_t group_stage_doc;
   bson_t sum;
   bson_t empty;

   bson_init(out);
   bson_append_utf8(out, "aggregate", 9, coll->collection, coll->collectionlen);
   bson_append_document_begin(out, "cursor", 6, &empty);
   bson_append_document_end(out, &empty);
   bson_append_array_builder_begin(out, "pipeline", 8, &pipeline);

   bson_array_builder_append_document_begin(pipeline, &match_stage);
   bson_append_document(&match_stage, "$match", 6, filter);
   bson_array_builder_append_document_end(pipeline, &match_stage);
   /* if @opts includes "skip", or "limit", append $skip and $limit stages to
    * the aggregate pipeline. */
   if (opts->skip.value_type != BSON_TYPE_EOD) {
      bson_t skip_stage;
      bson_array_builder_append_document_begin(pipeline, &skip_stage);
      bson_append_value(&skip_stage, "$skip", 5, &opts->skip);
      bson_array_builder_append_document_end(pipeline, &skip_stage);
   }
   if (opts->limit.value_type != BSON_TYPE_EOD) {
      bson_t limit_stage;
      bson_array_builder_append_document_begin(pipeline, &limit_stage);
      bson_append_value(&limit_stage, "$limit", 6, &opts->limit);
      bson_array_builder_append_document_end(pipeline, &limit_stage);
   }
   bson_array_builder_append_document_begin(pipeline, &group_stage);
   bson_append_document_begin(&group_stage, "$group", 6, &group_stage_doc);
   bson_append_int32(&group_stage_doc, "_id", 3, 1);
   bson_append_document_begin(&group_stage_doc, "n", 1, &sum);
   bson_append_int32(&sum, "$sum", 4, 1);
   bson_append_document_end(&group_stage_doc, &sum);
   bson_append_document_end(&group_stage, &group_stage_doc);
   bson_array_builder_append_document_end(pipeline, &group_stage);
   bson_append_array_builder_end(out, pipeline);
}


int64_t
mongoc_collection_count_documents(mongoc_collection_t *coll,
                                  const bson_t *filter,
                                  const bson_t *opts,
                                  const mongoc_read_prefs_t *read_prefs,
                                  bson_t *reply,
                                  bson_error_t *error)
{
   bson_t aggregate_cmd;
   bson_t aggregate_opts;
   bool ret;
   const bson_t *result;
   mongoc_cursor_t *cursor = NULL;
   int64_t count = -1;
   bson_t cmd_reply;
   bson_iter_t iter;

   ENTRY;

   BSON_ASSERT_PARAM(coll);
   BSON_ASSERT_PARAM(filter);

   // Parse options to validate.
   mongoc_count_document_opts_t cd_opts;
   if (!_mongoc_count_document_opts_parse(coll->client, opts, &cd_opts, error)) {
      GOTO(done);
   }

   _make_aggregate_for_count(coll, filter, &cd_opts, &aggregate_cmd);
   bson_init(&aggregate_opts);
   if (opts) {
      bsonBuildAppend(aggregate_opts, insert(*opts, not(key("skip", "limit"))));
   }

   ret = mongoc_collection_read_command_with_opts(coll, &aggregate_cmd, read_prefs, &aggregate_opts, &cmd_reply, error);
   bson_destroy(&aggregate_cmd);
   bson_destroy(&aggregate_opts);
   if (reply) {
      bson_copy_to(&cmd_reply, reply);
   }

   if (!ret) {
      bson_destroy(&cmd_reply);
      GOTO(done);
   }

   /* steals reply */
   cursor = mongoc_cursor_new_from_command_reply_with_opts(coll->client, &cmd_reply, NULL);
   BSON_ASSERT(mongoc_cursor_get_id(cursor) == 0);
   ret = mongoc_cursor_next(cursor, &result);
   if (!ret) {
      if (mongoc_cursor_error(cursor, error)) {
         GOTO(done);
      } else {
         count = 0;
         GOTO(done);
      }
   }

   if (bson_iter_init_find(&iter, result, "n") && BSON_ITER_HOLDS_INT(&iter)) {
      count = bson_iter_as_int64(&iter);
   }

done:
   _mongoc_count_document_opts_cleanup(&cd_opts);
   if (cursor) {
      mongoc_cursor_destroy(cursor);
   }
   RETURN(count);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_drop --
 *
 *       Request the MongoDB server drop the collection.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       @error is set upon failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_collection_drop(mongoc_collection_t *collection, /* IN */
                       bson_error_t *error)             /* OUT */
{
   return mongoc_collection_drop_with_opts(collection, NULL, error);
}


static bool
drop_with_opts(mongoc_collection_t *collection, const bson_t *opts, bson_error_t *error)
{
   bool ret;
   bson_t cmd;

   BSON_ASSERT_PARAM(collection);

   bson_init(&cmd);
   bson_append_utf8(&cmd, "drop", 4, collection->collection, collection->collectionlen);

   ret = _mongoc_client_command_with_opts(collection->client,
                                          collection->db,
                                          &cmd,
                                          MONGOC_CMD_WRITE,
                                          opts,
                                          MONGOC_QUERY_NONE,
                                          NULL, /* user prefs */
                                          collection->read_prefs,
                                          collection->read_concern,
                                          collection->write_concern,
                                          NULL, /* reply */
                                          error);
   bson_destroy(&cmd);

   return ret;
}

static bool
drop_with_opts_with_encryptedFields(mongoc_collection_t *collection,
                                    const bson_t *opts,
                                    const bson_t *encryptedFields,
                                    bson_error_t *error)
{
   char *escName = NULL;
   char *ecocName = NULL;
   mongoc_collection_t *escCollection = NULL;
   mongoc_collection_t *ecocCollection = NULL;
   bool ok = false;
   const char *name = mongoc_collection_get_name(collection);
   bson_error_reset(error);

   /* Drop ESC collection. */
   escName = _mongoc_get_encryptedField_state_collection(encryptedFields, name, "esc", error);
   if (!escName) {
      goto fail;
   }

   escCollection = mongoc_client_get_collection(collection->client, collection->db, escName);
   if (!drop_with_opts(escCollection, NULL /* opts */, error)) {
      if (error->code == MONGOC_SERVER_ERR_NS_NOT_FOUND) {
         memset(error, 0, sizeof(bson_error_t));
      } else {
         goto fail;
      }
   }

   /* Drop ECOC collection. */
   ecocName = _mongoc_get_encryptedField_state_collection(encryptedFields, name, "ecoc", error);
   if (!ecocName) {
      goto fail;
   }

   ecocCollection = mongoc_client_get_collection(collection->client, collection->db, ecocName);
   if (!drop_with_opts(ecocCollection, NULL /* opts */, error)) {
      if (error->code == MONGOC_SERVER_ERR_NS_NOT_FOUND) {
         memset(error, 0, sizeof(bson_error_t));
      } else {
         goto fail;
      }
   }

   /* Drop data collection. */
   if (!drop_with_opts(collection, opts, error)) {
      if (error->code == MONGOC_SERVER_ERR_NS_NOT_FOUND) {
         memset(error, 0, sizeof(bson_error_t));
      } else {
         goto fail;
      }
   }

   ok = true;
fail:
   mongoc_collection_destroy(ecocCollection);
   bson_free(ecocName);
   mongoc_collection_destroy(escCollection);
   bson_free(escName);
   return ok;
}

bool
mongoc_collection_drop_with_opts(mongoc_collection_t *collection, const bson_t *opts, bson_error_t *error)
{
   // The encryptedFields for the collection.
   bson_t encryptedFields = BSON_INITIALIZER;
   bson_t opts_without_encryptedFields = BSON_INITIALIZER;
   bool okay = false;

   // Try to find the encryptedFields from the collection options or from the
   // encryptedFieldsMap.
   if (!_mongoc_get_collection_encryptedFields(collection->client,
                                               collection->db,
                                               mongoc_collection_get_name(collection),
                                               opts,
                                               true /* checkEncryptedFieldsMap */,
                                               &encryptedFields,
                                               error)) {
      goto done;
   }

   if (bson_empty(&encryptedFields)) {
      // We didn't find the encryptedFields (yet)
      if (collection->client->topology->encrypted_fields_map != NULL) {
         // but we can ask the server for them:
         if (!_mongoc_get_encryptedFields_from_server(
                collection->client, collection->db, mongoc_collection_get_name(collection), &encryptedFields, error)) {
            goto done;
         }
      }
   }

   if (bson_empty(&encryptedFields)) {
      // There are no encryptedFields with this collection, so we can just do a
      // regular drop
      okay = drop_with_opts(collection, opts, error);
      goto done;
   }

   // We've found the encryptedFields, so we need to do something different
   // to drop this collection:
   bsonBuildAppend(opts_without_encryptedFields, if (opts, then(insert(*opts, not(key("encryptedFields"))))));
   if (bsonBuildError) {
      _mongoc_set_error(
         error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, "Error while updating drop options: %s", bsonBuildError);
      goto done;
   }

   okay = drop_with_opts_with_encryptedFields(collection, &opts_without_encryptedFields, &encryptedFields, error);

done:
   bson_destroy(&opts_without_encryptedFields);
   bson_destroy(&encryptedFields);
   return okay;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_drop_index --
 *
 *       Request the MongoDB server drop the named index.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 * Side effects:
 *       @error is setup upon failure if non-NULL.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_collection_drop_index(mongoc_collection_t *collection, /* IN */
                             const char *index_name,          /* IN */
                             bson_error_t *error)             /* OUT */
{
   return mongoc_collection_drop_index_with_opts(collection, index_name, NULL, error);
}


bool
mongoc_collection_drop_index_with_opts(mongoc_collection_t *collection,
                                       const char *index_name,
                                       const bson_t *opts,
                                       bson_error_t *error)
{
   bool ret;
   bson_t cmd;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(index_name);

   bson_init(&cmd);
   bson_append_utf8(&cmd, "dropIndexes", -1, collection->collection, collection->collectionlen);
   bson_append_utf8(&cmd, "index", -1, index_name, -1);

   ret = _mongoc_client_command_with_opts(collection->client,
                                          collection->db,
                                          &cmd,
                                          MONGOC_CMD_WRITE,
                                          opts,
                                          MONGOC_QUERY_NONE,
                                          NULL, /* user prefs */
                                          collection->read_prefs,
                                          collection->read_concern,
                                          collection->write_concern,
                                          NULL, /* reply */
                                          error);
   bson_destroy(&cmd);

   return ret;
}


char *
mongoc_collection_keys_to_index_string(const bson_t *keys)
{
   mcommon_string_append_t append;
   bson_iter_t iter;
   bson_type_t type;
   int i = 0;

   BSON_ASSERT_PARAM(keys);

   if (!bson_iter_init(&iter, keys)) {
      return NULL;
   }

   mcommon_string_new_as_append(&append);

   while (bson_iter_next(&iter)) {
      /* Index type can be specified as a string ("2d") or as an integer
       * representing direction */
      type = bson_iter_type(&iter);
      if (type == BSON_TYPE_UTF8) {
         mcommon_string_append_printf(
            &append, (i++ ? "_%s_%s" : "%s_%s"), bson_iter_key(&iter), bson_iter_utf8(&iter, NULL));
      } else if (type == BSON_TYPE_INT32) {
         mcommon_string_append_printf(
            &append, (i++ ? "_%s_%d" : "%s_%d"), bson_iter_key(&iter), bson_iter_int32(&iter));
      } else if (type == BSON_TYPE_INT64) {
         mcommon_string_append_printf(
            &append, (i++ ? "_%s_%" PRId64 : "%s_%" PRId64), bson_iter_key(&iter), bson_iter_int64(&iter));
      } else {
         mcommon_string_from_append_destroy(&append);
         return NULL;
      }
   }
   return mcommon_string_from_append_destroy_with_steal(&append);
}


static bool
_mongoc_collection_index_keys_equal(const bson_t *expected, const bson_t *actual)
{
   bson_iter_t iter_expected;
   bson_iter_t iter_actual;

   if (!bson_iter_init(&iter_expected, expected)) {
      return false;
   }
   if (!bson_iter_init(&iter_actual, actual)) {
      return false;
   }

   while (bson_iter_next(&iter_expected)) {
      /* If the key document has fewer items than expected, indexes are unequal
       */
      if (!bson_iter_next(&iter_actual)) {
         return false;
      }

      /* If key order does not match, indexes are unequal */
      if (strcmp(bson_iter_key(&iter_expected), bson_iter_key(&iter_actual)) != 0) {
         return false;
      }

      if (BSON_ITER_HOLDS_NUMBER(&iter_expected) && BSON_ITER_HOLDS_NUMBER(&iter_actual)) {
         if (bson_iter_as_int64(&iter_expected) != bson_iter_as_int64(&iter_actual)) {
            return false;
         }
      } else if (BSON_ITER_HOLDS_UTF8(&iter_expected) && BSON_ITER_HOLDS_UTF8(&iter_actual)) {
         if (strcmp(bson_iter_utf8(&iter_expected, NULL), bson_iter_utf8(&iter_actual, NULL)) != 0) {
            return false;
         }
      } else {
         return false;
      }
   }

   /* If our expected document is exhausted, make sure there are no extra keys
    * in the actual key document */
   if (bson_iter_next(&iter_actual)) {
      return false;
   }

   return true;
}

bool
_mongoc_collection_create_index_if_not_exists(mongoc_collection_t *collection,
                                              const bson_t *keys,
                                              const bson_t *opts,
                                              bson_error_t *error)
{
   mongoc_cursor_t *cursor;
   bool index_exists;
   bool r = false;
   const bson_t *doc;
   bson_iter_t iter;
   bson_t inner_doc;
   uint32_t data_len;
   const uint8_t *data;
   bson_t index;
   bson_t command;

   BSON_ASSERT(collection);
   BSON_ASSERT(keys);

   cursor = mongoc_collection_find_indexes_with_opts(collection, NULL);

   index_exists = false;

   while (mongoc_cursor_next(cursor, &doc) && !index_exists) {
      r = bson_iter_init_find(&iter, doc, "key");
      if (!r) {
         continue;
      }

      bson_iter_document(&iter, &data_len, &data);
      BSON_ASSERT(bson_init_static(&inner_doc, data, data_len));

      if (_mongoc_collection_index_keys_equal(keys, &inner_doc)) {
         index_exists = true;
      }

      bson_destroy(&inner_doc);
   }

   if (mongoc_cursor_error(cursor, error)) {
      mongoc_cursor_destroy(cursor);
      return false;
   }

   mongoc_cursor_destroy(cursor);

   if (index_exists) {
      return true;
   }

   if (opts) {
      bson_copy_to(opts, &index);
   } else {
      bson_init(&index);
   }

   BSON_APPEND_DOCUMENT(&index, "key", keys);

   if (!bson_has_field(&index, "name")) {
      char *alloc_name = mongoc_collection_keys_to_index_string(keys);

      if (!alloc_name) {
         _mongoc_set_error(error,
                           MONGOC_ERROR_BSON,
                           MONGOC_ERROR_BSON_INVALID,
                           "Cannot generate index name from invalid `keys` argument");
         GOTO(done);
      }

      BSON_APPEND_UTF8(&index, "name", alloc_name);

      bson_free(alloc_name);
   }

   bson_init(&command);
   BCON_APPEND(&command,
               "createIndexes",
               BCON_UTF8(mongoc_collection_get_name(collection)),
               "indexes",
               "[",
               BCON_DOCUMENT(&index),
               "]");

   r = mongoc_collection_write_command_with_opts(collection, &command, NULL, NULL, error);

done:
   bson_destroy(&index);
   bson_destroy(&command);

   return r;
}


mongoc_cursor_t *
mongoc_collection_find_indexes(mongoc_collection_t *collection, bson_error_t *error)
{
   mongoc_cursor_t *cursor;

   cursor = mongoc_collection_find_indexes_with_opts(collection, NULL);

   (void)mongoc_cursor_error(cursor, error);

   return cursor;
}


mongoc_cursor_t *
mongoc_collection_find_indexes_with_opts(mongoc_collection_t *collection, const bson_t *opts)
{
   mongoc_cursor_t *cursor;
   bson_t cmd = BSON_INITIALIZER;
   bson_t child;
   bson_error_t error;

   BSON_ASSERT_PARAM(collection);

   bson_append_utf8(&cmd, "listIndexes", -1, collection->collection, collection->collectionlen);

   BSON_APPEND_DOCUMENT_BEGIN(&cmd, "cursor", &child);
   bson_append_document_end(&cmd, &child);

   /* No read preference. Index Enumeration Spec: "run listIndexes on the
    * primary node in replicaSet mode". */
   cursor = _mongoc_cursor_cmd_new(collection->client, collection->ns, &cmd, opts, NULL, NULL, NULL);

   if (!mongoc_cursor_error(cursor, &error)) {
      _mongoc_cursor_prime(cursor);
   }

   if (mongoc_cursor_error(cursor, &error) && error.code == MONGOC_ERROR_COLLECTION_DOES_NOT_EXIST) {
      /* collection does not exist. from spec: return no documents but no err:
       * https://github.com/mongodb/specifications/blob/master/source/enumerate-collections/enumerate-collections.md#getting-full-collection-information
       */
      _mongoc_cursor_set_empty(cursor);
   }

   bson_destroy(&cmd);

   return cursor;
}

bool
mongoc_collection_insert(mongoc_collection_t *collection,
                         mongoc_insert_flags_t flags,
                         const bson_t *document,
                         const mongoc_write_concern_t *write_concern,
                         bson_error_t *error)
{
   bson_t opts = BSON_INITIALIZER;
   bson_t reply;
   bool r;

   bson_clear(&collection->gle);

   if (flags & MONGOC_INSERT_NO_VALIDATE) {
      bson_append_bool(&opts, "validate", 8, false);
   }

   if (write_concern) {
      mongoc_write_concern_append((mongoc_write_concern_t *)write_concern, &opts);
   }

   r = mongoc_collection_insert_one(collection, document, &opts, &reply, error);

   collection->gle = bson_copy(&reply);
   bson_destroy(&reply);
   bson_destroy(&opts);

   return r;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_insert_one --
 *
 *       Insert a document into a MongoDB collection.
 *
 * Parameters:
 *       @collection: A mongoc_collection_t.
 *       @document: The document to insert.
 *       @opts: Standard command options.
 *       @reply: Optional. Uninitialized doc to receive the update result.
 *       @error: A location for an error or NULL.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 *       If the write concern does not dictate checking the result of the
 *       insert, then true may be returned even though the document was
 *       not actually inserted on the MongoDB server or cluster.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_collection_insert_one(
   mongoc_collection_t *collection, const bson_t *document, const bson_t *opts, bson_t *reply, bson_error_t *error)
{
   mongoc_insert_one_opts_t insert_one_opts;
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   bson_t insert_id = BSON_INITIALIZER;
   bson_t cmd_opts = BSON_INITIALIZER;
   bool ret = false;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(document);

   _mongoc_bson_init_if_set(reply);

   if (!_mongoc_insert_one_opts_parse(collection->client, opts, &insert_one_opts, error)) {
      GOTO(done);
   }

   if (!bson_empty(&insert_one_opts.extra)) {
      bson_concat(&cmd_opts, &insert_one_opts.extra);
   }

   if (insert_one_opts.crud.comment.value_type != BSON_TYPE_EOD) {
      bson_append_value(&cmd_opts, "comment", 7, &insert_one_opts.crud.comment);
   }

   if (!_mongoc_validate_new_document(document, insert_one_opts.crud.validate, error)) {
      GOTO(done);
   }

   _mongoc_write_result_init(&result);
   _mongoc_write_command_init_insert_one_idl(
      &command, document, &cmd_opts, &insert_id, ++collection->client->cluster.operation_id);

   command.flags.bypass_document_validation = insert_one_opts.bypass;
   _mongoc_collection_write_command_execute_idl(&command, collection, &insert_one_opts.crud, &result);

   ret = MONGOC_WRITE_RESULT_COMPLETE(&result,
                                      collection->client->error_api_version,
                                      insert_one_opts.crud.writeConcern,
                                      /* no error domain override */
                                      (mongoc_error_domain_t)0,
                                      reply,
                                      error,
                                      "insertedCount");

   // Only record _id of document if it was actually inserted and reply is non-NULL.
   if (reply && result.nInserted > 0) {
      bson_concat(reply, &insert_id);
   }

   _mongoc_write_result_destroy(&result);
   _mongoc_write_command_destroy(&command);

done:
   _mongoc_insert_one_opts_cleanup(&insert_one_opts);
   bson_destroy(&insert_id);
   bson_destroy(&cmd_opts);

   RETURN(ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_insert_many --
 *
 *       Insert documents into a MongoDB collection.
 *
 * Parameters:
 *       @collection: A mongoc_collection_t.
 *       @documents: The documents to insert.
 *       @n_documents: Length of @documents array.
 *       @opts: Standard command options.
 *       @reply: Optional. Uninitialized doc to receive the update result.
 *       @error: A location for an error or NULL.
 *
 * Returns:
 *       true if successful; otherwise false and @error is set.
 *
 *       If the write concern does not dictate checking the result of the
 *       insert, then true may be returned even though the document was
 *       not actually inserted on the MongoDB server or cluster.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_collection_insert_many(mongoc_collection_t *collection,
                              const bson_t **documents,
                              size_t n_documents,
                              const bson_t *opts,
                              bson_t *reply,
                              bson_error_t *error)
{
   mongoc_insert_many_opts_t insert_many_opts;
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   bson_t cmd_opts = BSON_INITIALIZER;
   size_t i;
   bool ret;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(documents);

   _mongoc_bson_init_if_set(reply);

   if (!_mongoc_insert_many_opts_parse(collection->client, opts, &insert_many_opts, error)) {
      _mongoc_insert_many_opts_cleanup(&insert_many_opts);
      return false;
   }

   if (insert_many_opts.crud.comment.value_type != BSON_TYPE_EOD) {
      bson_append_value(&cmd_opts, "comment", 7, &insert_many_opts.crud.comment);
   }

   if (!bson_empty(&insert_many_opts.extra)) {
      bson_concat(&cmd_opts, &insert_many_opts.extra);
   }

   _mongoc_write_result_init(&result);
   _mongoc_write_command_init_insert_idl(&command, NULL, &cmd_opts, ++collection->client->cluster.operation_id);

   command.flags.ordered = insert_many_opts.ordered;
   command.flags.bypass_document_validation = insert_many_opts.bypass;

   for (i = 0; i < n_documents; i++) {
      if (!_mongoc_validate_new_document(documents[i], insert_many_opts.crud.validate, error)) {
         ret = false;
         GOTO(done);
      }

      _mongoc_write_command_insert_append(&command, documents[i]);
   }

   _mongoc_collection_write_command_execute_idl(&command, collection, &insert_many_opts.crud, &result);

   ret = MONGOC_WRITE_RESULT_COMPLETE(&result,
                                      collection->client->error_api_version,
                                      insert_many_opts.crud.writeConcern,
                                      /* no error domain override */
                                      (mongoc_error_domain_t)0,
                                      reply,
                                      error,
                                      "insertedCount");

done:
   _mongoc_write_result_destroy(&result);
   _mongoc_write_command_destroy(&command);
   _mongoc_insert_many_opts_cleanup(&insert_many_opts);
   bson_destroy(&cmd_opts);

   RETURN(ret);
}

bool
mongoc_collection_update(mongoc_collection_t *collection,
                         mongoc_update_flags_t uflags,
                         const bson_t *selector,
                         const bson_t *update,
                         const mongoc_write_concern_t *write_concern,
                         bson_error_t *error)
{
   mongoc_bulk_write_flags_t write_flags = MONGOC_BULK_WRITE_FLAGS_INIT;
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   bson_iter_t iter;
   bool ret;
   int flags = uflags;
   bson_t opts;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(selector);
   BSON_ASSERT_PARAM(update);

   bson_clear(&collection->gle);

   if (!write_concern) {
      write_concern = collection->write_concern;
   }

   if (!((uint32_t)flags & MONGOC_UPDATE_NO_VALIDATE) && bson_iter_init(&iter, update) && bson_iter_next(&iter)) {
      if (bson_iter_key(&iter)[0] == '$') {
         /* update document, all keys must be $-operators */
         if (!_mongoc_validate_update(update, _mongoc_default_update_vflags, error)) {
            return false;
         }
      } else {
         if (!_mongoc_validate_replace(update, _mongoc_default_replace_vflags, error)) {
            return false;
         }
      }
   }

   bson_init(&opts);
   BSON_APPEND_BOOL(&opts, "upsert", !!(flags & MONGOC_UPDATE_UPSERT));
   BSON_APPEND_BOOL(&opts, "multi", !!(flags & MONGOC_UPDATE_MULTI_UPDATE));

   _mongoc_write_result_init(&result);
   _mongoc_write_command_init_update(&command,
                                     selector,
                                     update,
                                     NULL, /* cmd_opts */
                                     &opts,
                                     write_flags,
                                     ++collection->client->cluster.operation_id);
   bson_destroy(&opts);

   command.flags.has_multi_write = !!(flags & MONGOC_UPDATE_MULTI_UPDATE);

   _mongoc_collection_write_command_execute(&command, collection, write_concern, NULL, &result);

   collection->gle = bson_new();
   ret = MONGOC_WRITE_RESULT_COMPLETE(&result,
                                      collection->client->error_api_version,
                                      write_concern,
                                      /* no error domain override */
                                      (mongoc_error_domain_t)0,
                                      collection->gle,
                                      error);

   _mongoc_write_result_destroy(&result);
   _mongoc_write_command_destroy(&command);

   RETURN(ret);
}

static bool
_mongoc_collection_update_or_replace(mongoc_collection_t *collection,
                                     const bson_t *selector,
                                     const bson_t *update,
                                     mongoc_update_opts_t *update_opts,
                                     bool multi,
                                     bool bypass,
                                     const bson_t *array_filters,
                                     const bson_t *sort,
                                     bson_t *extra,
                                     bson_t *reply,
                                     bson_error_t *error)
{
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   mongoc_server_stream_t *server_stream = NULL;
   bson_t cmd_opts = BSON_INITIALIZER;
   bool reply_initialized = false;
   bool ret = false;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(selector);
   BSON_ASSERT_PARAM(update);

   if (!bson_empty(&update_opts->let)) {
      bson_append_document(&cmd_opts, "let", 3, &update_opts->let);
   }

   if (update_opts->crud.comment.value_type != BSON_TYPE_EOD) {
      bson_append_value(&cmd_opts, "comment", 7, &update_opts->crud.comment);
   }

   if (update_opts->upsert) {
      bson_append_bool(extra, "upsert", 6, true);
   }

   if (!bson_empty(&update_opts->collation)) {
      bson_append_document(extra, "collation", 9, &update_opts->collation);
   }

   if (update_opts->hint.value_type) {
      bson_append_value(extra, "hint", 4, &update_opts->hint);
   }

   if (!bson_empty0(array_filters)) {
      bson_append_array(extra, "arrayFilters", 12, array_filters);
   }

   if (!bson_empty0(sort)) {
      bson_append_document(extra, "sort", 4, sort);
   }

   if (multi) {
      bson_append_bool(extra, "multi", 5, true);
   }

   _mongoc_write_result_init(&result);
   _mongoc_write_command_init_update_idl(
      &command, selector, update, &cmd_opts, extra, ++collection->client->cluster.operation_id);

   command.flags.has_multi_write = multi;
   command.flags.bypass_document_validation = bypass;
   if (!bson_empty(&update_opts->collation)) {
      command.flags.has_collation = true;
   }
   if (update_opts->hint.value_type) {
      command.flags.has_update_hint = true;
   }

   const mongoc_ss_log_context_t ss_log_context = {.operation = _mongoc_write_command_get_name(&command),
                                                   .has_operation_id = true,
                                                   .operation_id = command.operation_id};
   server_stream = mongoc_cluster_stream_for_writes(
      &collection->client->cluster, &ss_log_context, update_opts->crud.client_session, NULL, reply, error);

   if (!server_stream) {
      /* mongoc_cluster_stream_for_writes inits reply on error */
      reply_initialized = true;
      GOTO(done);
   }

   if (!bson_empty0(array_filters)) {
      if (!mongoc_write_concern_is_acknowledged(update_opts->crud.writeConcern)) {
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                           "Cannot use array filters with unacknowledged writes");
         GOTO(done);
      }
   }

   if (_mongoc_client_session_in_txn(update_opts->crud.client_session) && update_opts->crud.writeConcern) {
      _mongoc_set_error(error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "Cannot set write concern after starting transaction");
      GOTO(done);
   }

   if (!update_opts->crud.writeConcern && !_mongoc_client_session_in_txn(update_opts->crud.client_session)) {
      update_opts->crud.writeConcern = collection->write_concern;
      update_opts->crud.write_concern_owned = false;
   }

   _mongoc_write_command_execute_idl(&command,
                                     collection->client,
                                     server_stream,
                                     collection->db,
                                     collection->collection,
                                     0 /* offset */,
                                     &update_opts->crud,
                                     &result);

   _mongoc_bson_init_if_set(reply);
   reply_initialized = true;

   /* set fields described in CRUD spec for the UpdateResult */
   ret = MONGOC_WRITE_RESULT_COMPLETE(&result,
                                      collection->client->error_api_version,
                                      update_opts->crud.writeConcern,
                                      /* no error domain override */
                                      (mongoc_error_domain_t)0,
                                      reply,
                                      error,
                                      "modifiedCount",
                                      "matchedCount",
                                      "upsertedCount",
                                      "upsertedId");

done:
   _mongoc_write_result_destroy(&result);
   mongoc_server_stream_cleanup(server_stream);
   _mongoc_write_command_destroy(&command);
   bson_destroy(&cmd_opts);

   if (!reply_initialized) {
      _mongoc_bson_init_if_set(reply);
   }

   RETURN(ret);
}

bool
mongoc_collection_update_one(mongoc_collection_t *collection,
                             const bson_t *selector,
                             const bson_t *update,
                             const bson_t *opts,
                             bson_t *reply,
                             bson_error_t *error)
{
   mongoc_update_one_opts_t update_one_opts;
   bool ret;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(update);

   if (!_mongoc_update_one_opts_parse(collection->client, opts, &update_one_opts, error)) {
      _mongoc_update_one_opts_cleanup(&update_one_opts);
      _mongoc_bson_init_if_set(reply);
      return false;
   }

   if (!_mongoc_validate_update(update, update_one_opts.update.crud.validate, error)) {
      _mongoc_update_one_opts_cleanup(&update_one_opts);
      _mongoc_bson_init_if_set(reply);
      return false;
   }

   ret = _mongoc_collection_update_or_replace(collection,
                                              selector,
                                              update,
                                              &update_one_opts.update,
                                              false /* multi */,
                                              update_one_opts.update.bypass,
                                              &update_one_opts.arrayFilters,
                                              &update_one_opts.sort,
                                              &update_one_opts.extra,
                                              reply,
                                              error);

   _mongoc_update_one_opts_cleanup(&update_one_opts);

   RETURN(ret);
}

bool
mongoc_collection_update_many(mongoc_collection_t *collection,
                              const bson_t *selector,
                              const bson_t *update,
                              const bson_t *opts,
                              bson_t *reply,
                              bson_error_t *error)
{
   mongoc_update_many_opts_t update_many_opts;
   bool ret;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(update);

   if (!_mongoc_update_many_opts_parse(collection->client, opts, &update_many_opts, error)) {
      _mongoc_update_many_opts_cleanup(&update_many_opts);
      _mongoc_bson_init_if_set(reply);
      return false;
   }

   if (!_mongoc_validate_update(update, update_many_opts.update.crud.validate, error)) {
      _mongoc_update_many_opts_cleanup(&update_many_opts);
      _mongoc_bson_init_if_set(reply);
      return false;
   }

   ret = _mongoc_collection_update_or_replace(collection,
                                              selector,
                                              update,
                                              &update_many_opts.update,
                                              true /* multi */,
                                              update_many_opts.update.bypass,
                                              &update_many_opts.arrayFilters,
                                              NULL /* sort */,
                                              &update_many_opts.extra,
                                              reply,
                                              error);

   _mongoc_update_many_opts_cleanup(&update_many_opts);

   RETURN(ret);
}

bool
mongoc_collection_replace_one(mongoc_collection_t *collection,
                              const bson_t *selector,
                              const bson_t *replacement,
                              const bson_t *opts,
                              bson_t *reply,
                              bson_error_t *error)
{
   mongoc_replace_one_opts_t replace_one_opts;
   bool ret;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(replacement);

   if (!_mongoc_replace_one_opts_parse(collection->client, opts, &replace_one_opts, error)) {
      _mongoc_replace_one_opts_cleanup(&replace_one_opts);
      _mongoc_bson_init_if_set(reply);
      return false;
   }

   if (!_mongoc_validate_replace(replacement, replace_one_opts.update.crud.validate, error)) {
      _mongoc_replace_one_opts_cleanup(&replace_one_opts);
      _mongoc_bson_init_if_set(reply);
      return false;
   }

   ret = _mongoc_collection_update_or_replace(collection,
                                              selector,
                                              replacement,
                                              &replace_one_opts.update,
                                              false /* multi */,
                                              replace_one_opts.update.bypass,
                                              NULL,
                                              &replace_one_opts.sort,
                                              &replace_one_opts.extra,
                                              reply,
                                              error);

   _mongoc_replace_one_opts_cleanup(&replace_one_opts);

   RETURN(ret);
}


bool
mongoc_collection_remove(mongoc_collection_t *collection,
                         mongoc_remove_flags_t flags,
                         const bson_t *selector,
                         const mongoc_write_concern_t *write_concern,
                         bson_error_t *error)
{
   mongoc_bulk_write_flags_t write_flags = MONGOC_BULK_WRITE_FLAGS_INIT;
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   bson_t opts;
   bool ret;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(selector);

   bson_clear(&collection->gle);

   if (!write_concern) {
      write_concern = collection->write_concern;
   }

   bson_init(&opts);
   BSON_APPEND_INT32(&opts, "limit", flags & MONGOC_REMOVE_SINGLE_REMOVE ? 1 : 0);
   _mongoc_write_result_init(&result);
   ++collection->client->cluster.operation_id;
   _mongoc_write_command_init_delete(
      &command, selector, NULL, &opts, write_flags, collection->client->cluster.operation_id);
   bson_destroy(&opts);

   command.flags.has_multi_write = !(flags & MONGOC_REMOVE_SINGLE_REMOVE);

   _mongoc_collection_write_command_execute(&command, collection, write_concern, NULL, &result);

   collection->gle = bson_new();
   ret = MONGOC_WRITE_RESULT_COMPLETE(&result,
                                      collection->client->error_api_version,
                                      write_concern,
                                      0 /* no error domain override */,
                                      collection->gle,
                                      error);

   _mongoc_write_result_destroy(&result);
   _mongoc_write_command_destroy(&command);

   RETURN(ret);
}


static bool
_mongoc_delete_one_or_many(mongoc_collection_t *collection,
                           bool multi,
                           const bson_t *selector,
                           mongoc_delete_opts_t *delete_opts,
                           const bson_t *extra,
                           bson_t *reply,
                           bson_error_t *error)
{
   mongoc_write_command_t command;
   mongoc_write_result_t result;
   bson_t cmd_opts = BSON_INITIALIZER;
   bson_t opts = BSON_INITIALIZER;
   bool ret;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(selector);
   BSON_ASSERT(bson_empty0(reply));

   /* TODO: This function has historically used `extra` for top-level, command
    * options. That is inconsistent with the update function, which uses `extra`
    * for statement-level options. We will keep the original behavior for BC
    * reasons, but this should ultimately be addressed by CDRIVER-4306. */
   if (!bson_empty0(extra)) {
      bson_concat(&cmd_opts, extra);
   }

   if (!bson_empty(&delete_opts->let)) {
      bson_append_document(&cmd_opts, "let", 3, &delete_opts->let);
   }

   if (delete_opts->crud.comment.value_type != BSON_TYPE_EOD) {
      bson_append_value(&cmd_opts, "comment", 7, &delete_opts->crud.comment);
   }

   _mongoc_write_result_init(&result);
   bson_append_int32(&opts, "limit", 5, multi ? 0 : 1);

   if (!bson_empty(&delete_opts->collation)) {
      bson_append_document(&opts, "collation", 9, &delete_opts->collation);
   }

   if (delete_opts->hint.value_type) {
      bson_append_value(&opts, "hint", 4, &delete_opts->hint);
   }

   _mongoc_write_command_init_delete_idl(
      &command, selector, &cmd_opts, &opts, ++collection->client->cluster.operation_id);

   command.flags.has_multi_write = multi;
   if (!bson_empty(&delete_opts->collation)) {
      command.flags.has_collation = true;
   }
   if (delete_opts->hint.value_type) {
      command.flags.has_delete_hint = true;
   }

   _mongoc_collection_write_command_execute_idl(&command, collection, &delete_opts->crud, &result);

   /* set field described in CRUD spec for the DeleteResult */
   ret = MONGOC_WRITE_RESULT_COMPLETE(&result,
                                      collection->client->error_api_version,
                                      delete_opts->crud.writeConcern,
                                      /* no error domain override */
                                      (mongoc_error_domain_t)0,
                                      reply,
                                      error,
                                      "deletedCount");

   _mongoc_write_result_destroy(&result);
   _mongoc_write_command_destroy(&command);
   bson_destroy(&cmd_opts);
   bson_destroy(&opts);

   RETURN(ret);
}


bool
mongoc_collection_delete_one(
   mongoc_collection_t *collection, const bson_t *selector, const bson_t *opts, bson_t *reply, bson_error_t *error)
{
   mongoc_delete_one_opts_t delete_one_opts;
   bool ret = false;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(selector);

   _mongoc_bson_init_if_set(reply);
   if (!_mongoc_delete_one_opts_parse(collection->client, opts, &delete_one_opts, error)) {
      GOTO(done);
   }

   ret = _mongoc_delete_one_or_many(
      collection, false /* multi */, selector, &delete_one_opts.delete, &delete_one_opts.extra, reply, error);

done:
   _mongoc_delete_one_opts_cleanup(&delete_one_opts);

   RETURN(ret);
}

bool
mongoc_collection_delete_many(
   mongoc_collection_t *collection, const bson_t *selector, const bson_t *opts, bson_t *reply, bson_error_t *error)
{
   mongoc_delete_many_opts_t delete_many_opts;
   bool ret = false;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(selector);

   _mongoc_bson_init_if_set(reply);
   if (!_mongoc_delete_many_opts_parse(collection->client, opts, &delete_many_opts, error)) {
      GOTO(done);
   }

   ret = _mongoc_delete_one_or_many(
      collection, true /* multi */, selector, &delete_many_opts.delete, &delete_many_opts.extra, reply, error);

done:
   _mongoc_delete_many_opts_cleanup(&delete_many_opts);

   RETURN(ret);
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_get_read_prefs --
 *
 *       Fetch the default read preferences for the collection.
 *
 * Returns:
 *       A mongoc_read_prefs_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_read_prefs_t *
mongoc_collection_get_read_prefs(const mongoc_collection_t *collection)
{
   BSON_ASSERT_PARAM(collection);
   return collection->read_prefs;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_set_read_prefs --
 *
 *       Sets the default read preferences for the collection instance.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_collection_set_read_prefs(mongoc_collection_t *collection, const mongoc_read_prefs_t *read_prefs)
{
   BSON_ASSERT_PARAM(collection);

   if (collection->read_prefs) {
      mongoc_read_prefs_destroy(collection->read_prefs);
      collection->read_prefs = NULL;
   }

   if (read_prefs) {
      collection->read_prefs = mongoc_read_prefs_copy(read_prefs);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_get_read_concern --
 *
 *       Fetches the default read concern for the collection instance.
 *
 * Returns:
 *       A mongoc_read_concern_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_read_concern_t *
mongoc_collection_get_read_concern(const mongoc_collection_t *collection)
{
   BSON_ASSERT_PARAM(collection);

   return collection->read_concern;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_set_read_concern --
 *
 *       Sets the default read concern for the collection instance.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_collection_set_read_concern(mongoc_collection_t *collection, const mongoc_read_concern_t *read_concern)
{
   BSON_ASSERT_PARAM(collection);

   if (collection->read_concern) {
      mongoc_read_concern_destroy(collection->read_concern);
      collection->read_concern = NULL;
   }

   if (read_concern) {
      collection->read_concern = mongoc_read_concern_copy(read_concern);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_get_write_concern --
 *
 *       Fetches the default write concern for the collection instance.
 *
 * Returns:
 *       A mongoc_write_concern_t that should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const mongoc_write_concern_t *
mongoc_collection_get_write_concern(const mongoc_collection_t *collection)
{
   BSON_ASSERT_PARAM(collection);

   return collection->write_concern;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_set_write_concern --
 *
 *       Sets the default write concern for the collection instance.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_collection_set_write_concern(mongoc_collection_t *collection, const mongoc_write_concern_t *write_concern)
{
   BSON_ASSERT_PARAM(collection);

   if (collection->write_concern) {
      mongoc_write_concern_destroy(collection->write_concern);
      collection->write_concern = NULL;
   }

   if (write_concern) {
      collection->write_concern = mongoc_write_concern_copy(write_concern);
   }
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_get_name --
 *
 *       Returns the name of the collection, excluding the database name.
 *
 * Returns:
 *       A string which should not be modified or freed.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

const char *
mongoc_collection_get_name(mongoc_collection_t *collection)
{
   BSON_ASSERT_PARAM(collection);

   return collection->collection;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_rename --
 *
 *       Rename the collection to @new_name.
 *
 *       If @new_db is NULL, the same db will be used.
 *
 *       If @drop_target_before_rename is true, then a collection named
 *       @new_name will be dropped before renaming @collection to
 *       @new_name.
 *
 * Returns:
 *       true on success; false on failure and @error is set.
 *
 * Side effects:
 *       @error is set on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_collection_rename(mongoc_collection_t *collection,
                         const char *new_db,
                         const char *new_name,
                         bool drop_target_before_rename,
                         bson_error_t *error)
{
   return mongoc_collection_rename_with_opts(collection, new_db, new_name, drop_target_before_rename, NULL, error);
}


bool
mongoc_collection_rename_with_opts(mongoc_collection_t *collection,
                                   const char *new_db,
                                   const char *new_name,
                                   bool drop_target_before_rename,
                                   const bson_t *opts,
                                   bson_error_t *error)
{
   bson_t cmd = BSON_INITIALIZER;
   char *newns;
   bool ret;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(new_name);

   if (strchr(new_name, '$')) {
      _mongoc_set_error(error,
                        MONGOC_ERROR_NAMESPACE,
                        MONGOC_ERROR_NAMESPACE_INVALID,
                        "\"%s\" is an invalid collection name.",
                        new_name);
      return false;
   }

   newns = bson_strdup_printf("%s.%s", new_db ? new_db : collection->db, new_name);

   BSON_APPEND_UTF8(&cmd, "renameCollection", collection->ns);
   BSON_APPEND_UTF8(&cmd, "to", newns);

   if (drop_target_before_rename) {
      BSON_APPEND_BOOL(&cmd, "dropTarget", true);
   }

   ret = _mongoc_client_command_with_opts(collection->client,
                                          "admin",
                                          &cmd,
                                          MONGOC_CMD_WRITE,
                                          opts,
                                          MONGOC_QUERY_NONE,
                                          NULL, /* user prefs */
                                          collection->read_prefs,
                                          collection->read_concern,
                                          collection->write_concern,
                                          NULL, /* reply */
                                          error);

   if (ret) {
      if (new_db) {
         bson_free(collection->db);
         collection->db = bson_strdup(new_db);
      }

      bson_free(collection->collection);
      collection->collection = bson_strdup(new_name);
      collection->collectionlen = (int)strlen(collection->collection);

      bson_free(collection->ns);
      collection->ns = bson_strdup_printf("%s.%s", collection->db, new_name);
      collection->nslen = (int)strlen(collection->ns);
   }

   bson_free(newns);
   bson_destroy(&cmd);

   return ret;
}


mongoc_bulk_operation_t *
mongoc_collection_create_bulk_operation_with_opts(mongoc_collection_t *collection, const bson_t *opts)
{
   mongoc_bulk_opts_t bulk_opts;
   mongoc_bulk_write_flags_t write_flags = MONGOC_BULK_WRITE_FLAGS_INIT;
   mongoc_write_concern_t *wc = NULL;
   mongoc_bulk_operation_t *bulk;
   bson_error_t err = {0};

   BSON_ASSERT_PARAM(collection);

   (void)_mongoc_bulk_opts_parse(collection->client, opts, &bulk_opts, &err);
   if (!_mongoc_client_session_in_txn(bulk_opts.client_session)) {
      wc = COALESCE(bulk_opts.writeConcern, collection->write_concern);
   }
   write_flags.ordered = bulk_opts.ordered;
   bulk = _mongoc_bulk_operation_new(collection->client, collection->db, collection->collection, write_flags, wc);

   if (!bson_empty(&bulk_opts.let)) {
      mongoc_bulk_operation_set_let(bulk, &bulk_opts.let);
   }

   if (bulk_opts.comment.value_type != BSON_TYPE_EOD) {
      mongoc_bulk_operation_set_comment(bulk, &bulk_opts.comment);
   }

   bulk->session = bulk_opts.client_session;
   if (err.domain) {
      /* _mongoc_bulk_opts_parse failed, above */
      memcpy(&bulk->result.error, &err, sizeof(bson_error_t));
   } else if (_mongoc_client_session_in_txn(bulk_opts.client_session) &&
              !mongoc_write_concern_is_default(bulk_opts.writeConcern)) {
      _mongoc_set_error(&bulk->result.error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "Cannot set write concern after starting transaction");
   }

   _mongoc_bulk_opts_cleanup(&bulk_opts);

   return bulk;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_find_and_modify_with_opts --
 *
 *       Find a document in @collection matching @query, applying @opts.
 *
 *       If @reply is not NULL, then the result document will be placed
 *       in reply and should be released with bson_destroy().
 *
 *       See for more information:
 *       https://www.mongodb.com/docs/manual/reference/command/findAndModify/
 *
 * Returns:
 *       true on success; false on failure.
 *
 * Side effects:
 *       reply is initialized.
 *       error is set if false is returned.
 *
 *--------------------------------------------------------------------------
 */
bool
mongoc_collection_find_and_modify_with_opts(mongoc_collection_t *collection,
                                            const bson_t *query,
                                            const mongoc_find_and_modify_opts_t *opts,
                                            bson_t *reply,
                                            bson_error_t *error)
{
   mongoc_cluster_t *cluster;
   mongoc_cmd_parts_t parts;
   bson_iter_t iter;
   bson_iter_t inner;
   const char *name;
   bson_t ss_reply;
   bson_t reply_local = BSON_INITIALIZER;
   bool ret = false;
   bson_t command = BSON_INITIALIZER;
   mongoc_server_stream_t *server_stream = NULL;
   mongoc_server_stream_t *retry_server_stream = NULL;
   mongoc_find_and_modify_appended_opts_t appended_opts;
   mongoc_write_concern_t *write_concern = NULL;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(query);
   BSON_ASSERT_PARAM(opts);

   if (reply) {
      bson_init(reply);
   } else {
      // Caller did not pass an output `reply`. Use a local `reply` to determine if a server error is retryable.
      reply = &reply_local;
   }
   cluster = &collection->client->cluster;

   mongoc_cmd_parts_init(&parts, collection->client, collection->db, MONGOC_QUERY_NONE, &command);
   parts.is_read_command = true;
   parts.is_write_command = true;

   if (!_mongoc_find_and_modify_appended_opts_parse(cluster->client, &opts->extra, &appended_opts, error)) {
      GOTO(done);
   }

   const mongoc_ss_log_context_t ss_log_context = {.operation = "findAndModify"};
   server_stream =
      mongoc_cluster_stream_for_writes(cluster, &ss_log_context, appended_opts.client_session, NULL, &ss_reply, error);

   if (!server_stream) {
      bson_concat(reply, &ss_reply);
      bson_destroy(&ss_reply);
      GOTO(done);
   }

   name = mongoc_collection_get_name(collection);
   BSON_APPEND_UTF8(&command, "findAndModify", name);
   BSON_APPEND_DOCUMENT(&command, "query", query);

   if (opts->sort) {
      BSON_APPEND_DOCUMENT(&command, "sort", opts->sort);
   }

   if (opts->update) {
      if (_mongoc_document_is_pipeline(opts->update)) {
         BSON_APPEND_ARRAY(&command, "update", opts->update);
      } else {
         BSON_APPEND_DOCUMENT(&command, "update", opts->update);
      }
   }

   if (opts->fields) {
      BSON_APPEND_DOCUMENT(&command, "fields", opts->fields);
   }

   if (opts->flags & MONGOC_FIND_AND_MODIFY_REMOVE) {
      BSON_APPEND_BOOL(&command, "remove", true);
   }

   if (opts->flags & MONGOC_FIND_AND_MODIFY_UPSERT) {
      BSON_APPEND_BOOL(&command, "upsert", true);
   }

   if (opts->flags & MONGOC_FIND_AND_MODIFY_RETURN_NEW) {
      BSON_APPEND_BOOL(&command, "new", true);
   }

   if (opts->bypass_document_validation) {
      BSON_APPEND_BOOL(&command, "bypassDocumentValidation", opts->bypass_document_validation);
   }

   if (opts->max_time_ms > 0) {
      BSON_APPEND_INT32(&command, "maxTimeMS", opts->max_time_ms);
   }

   /* Some options set via mongoc_find_and_modify_opts_append were parsed. Set
    * them on the command parts. */
   if (appended_opts.client_session) {
      mongoc_cmd_parts_set_session(&parts, appended_opts.client_session);
   }

   if (appended_opts.writeConcern) {
      if (_mongoc_client_session_in_txn(parts.assembled.session)) {
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "Cannot set write concern after starting transaction");
         GOTO(done);
      }

      write_concern = appended_opts.writeConcern;
   }
   /* inherit write concern from collection if not in transaction */
   else if (!_mongoc_client_session_in_txn(parts.assembled.session)) {
      if (!mongoc_write_concern_is_valid(collection->write_concern)) {
         _mongoc_set_error(
            error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "The write concern is invalid.");
         GOTO(done);
      }

      write_concern = collection->write_concern;
   }

   if (appended_opts.hint.value_type) {
      int max_wire_version = mongoc_write_concern_is_acknowledged(write_concern)
                                ? WIRE_VERSION_FIND_AND_MODIFY_HINT_SERVER_SIDE_ERROR
                                : WIRE_VERSION_FIND_AND_MODIFY_HINT;

      if (server_stream->sd->max_wire_version < max_wire_version) {
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                           "The selected server does not support hint for findAndModify");
         GOTO(done);
      }

      bson_append_value(&parts.extra, "hint", 4, &appended_opts.hint);
   }

   if (!bson_empty(&appended_opts.let)) {
      bson_append_document(&parts.extra, "let", 3, &appended_opts.let);
   }

   if (appended_opts.comment.value_type != BSON_TYPE_EOD) {
      bson_append_value(&parts.extra, "comment", 7, &appended_opts.comment);
   }

   /* Append any remaining unparsed options set via
    * mongoc_find_and_modify_opts_append to the command part. */
   if (bson_iter_init(&iter, &appended_opts.extra)) {
      if (!mongoc_cmd_parts_append_opts(&parts, &iter, error)) {
         GOTO(done);
      }
   }

   /* An empty write concern amounts to a no-op, so there's no need to guard
    * against it. */
   if (!mongoc_cmd_parts_set_write_concern(&parts, write_concern, error)) {
      GOTO(done);
   }

   parts.assembled.operation_id = ++cluster->operation_id;
   if (!mongoc_cmd_parts_assemble(&parts, server_stream, error)) {
      GOTO(done);
   }

   bson_destroy(reply);
   ret = mongoc_cluster_run_retryable_write(
      cluster, &parts.assembled, parts.is_retryable_write, &retry_server_stream, reply, error);

   if (bson_iter_init_find(&iter, reply, "writeConcernError") && BSON_ITER_HOLDS_DOCUMENT(&iter)) {
      const char *errmsg = NULL;
      uint32_t code = 0;

      BSON_ASSERT(bson_iter_recurse(&iter, &inner));
      while (bson_iter_next(&inner)) {
         if (BSON_ITER_IS_KEY(&inner, "code")) {
            code = (uint32_t)bson_iter_as_int64(&inner);
         } else if (BSON_ITER_IS_KEY(&inner, "errmsg")) {
            errmsg = bson_iter_utf8(&inner, NULL);
         }
      }
      _mongoc_set_error_with_category(
         error, MONGOC_ERROR_CATEGORY_SERVER, MONGOC_ERROR_WRITE_CONCERN, code, "Write Concern error: %s", errmsg);
      ret = false;
   }


done:
   mongoc_server_stream_cleanup(server_stream);
   mongoc_server_stream_cleanup(retry_server_stream);

   if (ret && error) {
      /* if a retry succeeded, clear the initial error */
      memset(error, 0, sizeof(bson_error_t));
   }
   mongoc_cmd_parts_cleanup(&parts);
   bson_destroy(&command);
   bson_destroy(&reply_local);
   _mongoc_find_and_modify_appended_opts_cleanup(&appended_opts);
   RETURN(ret);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_collection_find_and_modify --
 *
 *       Find a document in @collection matching @query and update it with
 *       the update document @update.
 *
 *       If @reply is not NULL, then the result document will be placed
 *       in reply and should be released with bson_destroy().
 *
 *       If @remove is true, then the matching documents will be removed.
 *
 *       If @fields is not NULL, it will be used to select the desired
 *       resulting fields.
 *
 *       If @_new is true, then the new version of the document is returned
 *       instead of the old document.
 *
 *       See for more information:
 *       https://www.mongodb.com/docs/manual/reference/command/findAndModify/
 *
 * Returns:
 *       true on success; false on failure.
 *
 * Side effects:
 *       reply is initialized.
 *       error is set if false is returned.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_collection_find_and_modify(mongoc_collection_t *collection,
                                  const bson_t *query,
                                  const bson_t *sort,
                                  const bson_t *update,
                                  const bson_t *fields,
                                  bool _remove,
                                  bool upsert,
                                  bool _new,
                                  bson_t *reply,
                                  bson_error_t *error)
{
   mongoc_find_and_modify_opts_t *opts;
   int flags = 0;
   bool ret;

   ENTRY;

   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(query);
   BSON_ASSERT(update || _remove);


   if (_remove) {
      flags |= MONGOC_FIND_AND_MODIFY_REMOVE;
   }
   if (upsert) {
      flags |= MONGOC_FIND_AND_MODIFY_UPSERT;
   }
   if (_new) {
      flags |= MONGOC_FIND_AND_MODIFY_RETURN_NEW;
   }

   opts = mongoc_find_and_modify_opts_new();

   mongoc_find_and_modify_opts_set_sort(opts, sort);
   mongoc_find_and_modify_opts_set_update(opts, update);
   mongoc_find_and_modify_opts_set_fields(opts, fields);
   mongoc_find_and_modify_opts_set_flags(opts, flags);

   ret = mongoc_collection_find_and_modify_with_opts(collection, query, opts, reply, error);
   mongoc_find_and_modify_opts_destroy(opts);

   return ret;
}

mongoc_change_stream_t *
mongoc_collection_watch(const mongoc_collection_t *coll, const bson_t *pipeline, const bson_t *opts)
{
   return _mongoc_change_stream_new_from_collection(coll, pipeline, opts);
}

struct _mongoc_index_model_t {
   bson_t *keys;
   bson_t *opts;
};

mongoc_index_model_t *
mongoc_index_model_new(const bson_t *keys, const bson_t *opts)
{
   BSON_ASSERT_PARAM(keys);
   // `opts` may be NULL.

   mongoc_index_model_t *im = bson_malloc(sizeof(mongoc_index_model_t));
   im->keys = bson_copy(keys);
   im->opts = opts ? bson_copy(opts) : NULL;
   return im;
}

void
mongoc_index_model_destroy(mongoc_index_model_t *im)
{
   if (!im) {
      return;
   }
   bson_destroy(im->keys);
   bson_destroy(im->opts);
   bson_free(im);
}

bool
mongoc_collection_create_indexes_with_opts(mongoc_collection_t *collection,
                                           mongoc_index_model_t *const *models,
                                           size_t n_models,
                                           const bson_t *opts,
                                           bson_t *reply,
                                           bson_error_t *error)
{
   BSON_ASSERT_PARAM(collection);
   BSON_ASSERT_PARAM(models);
   // `opts` may be NULL.
   // `reply` may be NULL.
   // `error` may be NULL.
   bson_t reply_local = BSON_INITIALIZER;
   bson_t *reply_ptr;
   mongoc_server_stream_t *server_stream = NULL;
   bool ok = false;
   bson_t cmd = BSON_INITIALIZER;

   reply_ptr = reply ? reply : &reply_local;
   // Always initialize `reply` if set. Caller is expected to `bson_destroy
   // (reply)`.
   bson_init(reply_ptr);

   // Check for commitQuorum option.
   if (opts && bson_has_field(opts, "commitQuorum")) {
      const mongoc_ss_log_context_t ss_log_context = {.operation = "createIndexes"};
      server_stream = mongoc_cluster_stream_for_writes(&collection->client->cluster,
                                                       &ss_log_context,
                                                       NULL /* mongoc_client_session_t */,
                                                       NULL /* deprioritized servers */,
                                                       reply_ptr,
                                                       error);
      if (server_stream->sd->max_wire_version < WIRE_VERSION_4_4) {
         // Raise an error required by the specification:
         // "Drivers MUST manually raise an error if this option is specified
         // when creating an index on a pre 4.4 server."
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                           "The selected server does not support the commitQuorum option");
         GOTO(fail);
      }
   }

   // Build the createIndexes command.
   BSON_ASSERT(BSON_APPEND_UTF8(&cmd, "createIndexes", collection->collection));
   bson_array_builder_t *indexes;
   BSON_ASSERT(BSON_APPEND_ARRAY_BUILDER_BEGIN(&cmd, "indexes", &indexes));
   for (uint32_t idx = 0; idx < n_models; idx++) {
      /*
         Append a document of this form:
         <idx>: {
             key: {
                 <key-value_pair>,
                 <key-value_pair>,
                 ...
             },
             name: <index_name>,
             <option1>,
             <option2>,
             ...
         },
      */

      bson_t index;
      BSON_ASSERT(bson_array_builder_append_document_begin(indexes, &index));
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&index, "key", models[idx]->keys));
      bson_iter_t name_iter;
      if (models[idx]->opts && bson_iter_init_find(&name_iter, models[idx]->opts, "name")) {
         // `name` was specified as an index option.
      } else {
         // No `name` was specified. Create index `name` from keys.
         char *name = mongoc_collection_keys_to_index_string(models[idx]->keys);
         BSON_ASSERT(name);
         BSON_ASSERT(BSON_APPEND_UTF8(&index, "name", name));
         bson_free(name);
      }

      if (models[idx]->opts) {
         BSON_ASSERT(bson_concat(&index, models[idx]->opts));
      }
      BSON_ASSERT(bson_array_builder_append_document_end(indexes, &index));
   }
   BSON_ASSERT(bson_append_array_builder_end(&cmd, indexes));

   ok = mongoc_collection_write_command_with_opts(collection, &cmd, opts, reply_ptr, error);

fail:
   mongoc_server_stream_cleanup(server_stream);
   bson_destroy(&cmd);
   bson_destroy(&reply_local);
   return ok;
}
