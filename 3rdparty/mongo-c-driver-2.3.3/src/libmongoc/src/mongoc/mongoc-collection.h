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

#ifndef MONGOC_COLLECTION_H
#define MONGOC_COLLECTION_H

#include <mongoc/mongoc-bulk-operation.h>
#include <mongoc/mongoc-change-stream.h>
#include <mongoc/mongoc-cursor.h>
#include <mongoc/mongoc-find-and-modify.h>
#include <mongoc/mongoc-flags.h>
#include <mongoc/mongoc-macros.h>
#include <mongoc/mongoc-read-concern.h>
#include <mongoc/mongoc-read-prefs.h>
#include <mongoc/mongoc-write-concern.h>

#include <bson/bson.h>

BSON_BEGIN_DECLS


typedef struct _mongoc_collection_t mongoc_collection_t;


MONGOC_EXPORT(mongoc_cursor_t *)
mongoc_collection_aggregate(mongoc_collection_t *collection,
                            mongoc_query_flags_t flags,
                            const bson_t *pipeline,
                            const bson_t *opts,
                            const mongoc_read_prefs_t *read_prefs) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT(void)
mongoc_collection_destroy(mongoc_collection_t *collection);

MONGOC_EXPORT(mongoc_collection_t *)
mongoc_collection_copy(mongoc_collection_t *collection) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT(bool)
mongoc_collection_read_command_with_opts(mongoc_collection_t *collection,
                                         const bson_t *command,
                                         const mongoc_read_prefs_t *read_prefs,
                                         const bson_t *opts,
                                         bson_t *reply,
                                         bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_write_command_with_opts(
   mongoc_collection_t *collection, const bson_t *command, const bson_t *opts, bson_t *reply, bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_read_write_command_with_opts(mongoc_collection_t *collection,
                                               const bson_t *command,
                                               const mongoc_read_prefs_t *read_prefs /* IGNORED */,
                                               const bson_t *opts,
                                               bson_t *reply,
                                               bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_command_with_opts(mongoc_collection_t *collection,
                                    const bson_t *command,
                                    const mongoc_read_prefs_t *read_prefs,
                                    const bson_t *opts,
                                    bson_t *reply,
                                    bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_command_simple(mongoc_collection_t *collection,
                                 const bson_t *command,
                                 const mongoc_read_prefs_t *read_prefs,
                                 bson_t *reply,
                                 bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_drop(mongoc_collection_t *collection, bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_drop_with_opts(mongoc_collection_t *collection, const bson_t *opts, bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_drop_index(mongoc_collection_t *collection, const char *index_name, bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_drop_index_with_opts(mongoc_collection_t *collection,
                                       const char *index_name,
                                       const bson_t *opts,
                                       bson_error_t *error);

MONGOC_EXPORT(mongoc_cursor_t *)
mongoc_collection_find_indexes_with_opts(mongoc_collection_t *collection, const bson_t *opts)
   BSON_GNUC_WARN_UNUSED_RESULT;

typedef struct _mongoc_index_model_t mongoc_index_model_t;

MONGOC_EXPORT(mongoc_index_model_t *)
mongoc_index_model_new(const bson_t *keys, const bson_t *opts);

MONGOC_EXPORT(void) mongoc_index_model_destroy(mongoc_index_model_t *model);

MONGOC_EXPORT(bool)
mongoc_collection_create_indexes_with_opts(mongoc_collection_t *collection,
                                           mongoc_index_model_t *const *models,
                                           size_t n_models,
                                           const bson_t *opts,
                                           bson_t *reply,
                                           bson_error_t *error);

MONGOC_EXPORT(mongoc_cursor_t *)
mongoc_collection_find_with_opts(mongoc_collection_t *collection,
                                 const bson_t *filter,
                                 const bson_t *opts,
                                 const mongoc_read_prefs_t *read_prefs) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT(bool)
mongoc_collection_insert(mongoc_collection_t *collection,
                         mongoc_insert_flags_t flags,
                         const bson_t *document,
                         const mongoc_write_concern_t *write_concern,
                         bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_insert_one(
   mongoc_collection_t *collection, const bson_t *document, const bson_t *opts, bson_t *reply, bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_insert_many(mongoc_collection_t *collection,
                              const bson_t **documents,
                              size_t n_documents,
                              const bson_t *opts,
                              bson_t *reply,
                              bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_update(mongoc_collection_t *collection,
                         mongoc_update_flags_t flags,
                         const bson_t *selector,
                         const bson_t *update,
                         const mongoc_write_concern_t *write_concern,
                         bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_update_one(mongoc_collection_t *collection,
                             const bson_t *selector,
                             const bson_t *update,
                             const bson_t *opts,
                             bson_t *reply,
                             bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_update_many(mongoc_collection_t *collection,
                              const bson_t *selector,
                              const bson_t *update,
                              const bson_t *opts,
                              bson_t *reply,
                              bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_replace_one(mongoc_collection_t *collection,
                              const bson_t *selector,
                              const bson_t *replacement,
                              const bson_t *opts,
                              bson_t *reply,
                              bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_remove(mongoc_collection_t *collection,
                         mongoc_remove_flags_t flags,
                         const bson_t *selector,
                         const mongoc_write_concern_t *write_concern,
                         bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_delete_one(
   mongoc_collection_t *collection, const bson_t *selector, const bson_t *opts, bson_t *reply, bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_delete_many(
   mongoc_collection_t *collection, const bson_t *selector, const bson_t *opts, bson_t *reply, bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_rename(mongoc_collection_t *collection,
                         const char *new_db,
                         const char *new_name,
                         bool drop_target_before_rename,
                         bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_rename_with_opts(mongoc_collection_t *collection,
                                   const char *new_db,
                                   const char *new_name,
                                   bool drop_target_before_rename,
                                   const bson_t *opts,
                                   bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_find_and_modify_with_opts(mongoc_collection_t *collection,
                                            const bson_t *query,
                                            const mongoc_find_and_modify_opts_t *opts,
                                            bson_t *reply,
                                            bson_error_t *error);

MONGOC_EXPORT(bool)
mongoc_collection_find_and_modify(mongoc_collection_t *collection,
                                  const bson_t *query,
                                  const bson_t *sort,
                                  const bson_t *update,
                                  const bson_t *fields,
                                  bool _remove,
                                  bool upsert,
                                  bool _new,
                                  bson_t *reply,
                                  bson_error_t *error);

MONGOC_EXPORT(mongoc_bulk_operation_t *)
mongoc_collection_create_bulk_operation_with_opts(mongoc_collection_t *collection, const bson_t *opts)
   BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT(const mongoc_read_prefs_t *)
mongoc_collection_get_read_prefs(const mongoc_collection_t *collection);

MONGOC_EXPORT(void)
mongoc_collection_set_read_prefs(mongoc_collection_t *collection, const mongoc_read_prefs_t *read_prefs);

MONGOC_EXPORT(const mongoc_read_concern_t *)
mongoc_collection_get_read_concern(const mongoc_collection_t *collection);

MONGOC_EXPORT(void)
mongoc_collection_set_read_concern(mongoc_collection_t *collection, const mongoc_read_concern_t *read_concern);

MONGOC_EXPORT(const mongoc_write_concern_t *)
mongoc_collection_get_write_concern(const mongoc_collection_t *collection);

MONGOC_EXPORT(void)
mongoc_collection_set_write_concern(mongoc_collection_t *collection, const mongoc_write_concern_t *write_concern);

MONGOC_EXPORT(const char *)
mongoc_collection_get_name(mongoc_collection_t *collection);

MONGOC_EXPORT(char *)
mongoc_collection_keys_to_index_string(const bson_t *keys) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT(mongoc_change_stream_t *)
mongoc_collection_watch(const mongoc_collection_t *coll, const bson_t *pipeline, const bson_t *opts)
   BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT(int64_t)
mongoc_collection_count_documents(mongoc_collection_t *coll,
                                  const bson_t *filter,
                                  const bson_t *opts,
                                  const mongoc_read_prefs_t *read_prefs,
                                  bson_t *reply,
                                  bson_error_t *error);

MONGOC_EXPORT(int64_t)
mongoc_collection_estimated_document_count(mongoc_collection_t *coll,
                                           const bson_t *opts,
                                           const mongoc_read_prefs_t *read_prefs,
                                           bson_t *reply,
                                           bson_error_t *error);

BSON_END_DECLS


#endif /* MONGOC_COLLECTION_H */
