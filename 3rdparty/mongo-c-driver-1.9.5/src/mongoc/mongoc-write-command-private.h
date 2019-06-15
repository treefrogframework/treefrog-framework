/*
 * Copyright 2014 MongoDB, Inc.
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

#ifndef MONGOC_WRITE_COMMAND_PRIVATE_H
#define MONGOC_WRITE_COMMAND_PRIVATE_H

#if !defined(MONGOC_COMPILATION)
#error "Only <mongoc.h> can be included directly."
#endif

#include <bson.h>

#include "mongoc-client.h"
#include "mongoc-error.h"
#include "mongoc-write-concern.h"
#include "mongoc-server-stream-private.h"
#include "mongoc-buffer-private.h"


BSON_BEGIN_DECLS


#define MONGOC_WRITE_COMMAND_DELETE 0
#define MONGOC_WRITE_COMMAND_INSERT 1
#define MONGOC_WRITE_COMMAND_UPDATE 2


typedef enum {
   MONGOC_BYPASS_DOCUMENT_VALIDATION_FALSE = 0,
   MONGOC_BYPASS_DOCUMENT_VALIDATION_TRUE = 1 << 0,
   MONGOC_BYPASS_DOCUMENT_VALIDATION_DEFAULT = 1 << 1,
} mongoc_write_bypass_document_validation_t;

struct _mongoc_bulk_write_flags_t {
   bool ordered;
   mongoc_write_bypass_document_validation_t bypass_document_validation;
   bool has_collation;
   bool has_multi_write;
};


typedef struct {
   int type;
   mongoc_buffer_t payload;
   uint32_t n_documents;
   mongoc_bulk_write_flags_t flags;
   int64_t operation_id;
   bson_t cmd_opts;
   union {
      struct {
         bool allow_bulk_op_insert;
      } insert;
   } u;
} mongoc_write_command_t;


typedef struct {
   uint32_t nInserted;
   uint32_t nMatched;
   uint32_t nModified;
   uint32_t nRemoved;
   uint32_t nUpserted;
   /* like [{"index": int, "code": int, "errmsg": str}, ...] */
   bson_t writeErrors;
   /* like [{"index": int, "_id": value}, ...] */
   bson_t upserted;
   uint32_t n_writeConcernErrors;
   /* like [{"code": 64, "errmsg": "duplicate"}, ...] */
   bson_t writeConcernErrors;
   bool failed;    /* The command failed */
   bool must_stop; /* The stream may have been disconnected */
   bson_error_t error;
   uint32_t upsert_append_count;
} mongoc_write_result_t;


const char *
_mongoc_command_type_to_field_name (int command_type);
const char *
_mongoc_command_type_to_name (int command_type);

void
_mongoc_write_command_destroy (mongoc_write_command_t *command);
void
_mongoc_write_command_init (bson_t *doc,
                            mongoc_write_command_t *command,
                            const char *collection,
                            const mongoc_write_concern_t *write_concern);
void
_mongoc_write_command_init_insert (mongoc_write_command_t *command,
                                   const bson_t *document,
                                   const bson_t *cmd_opts,
                                   mongoc_bulk_write_flags_t flags,
                                   int64_t operation_id,
                                   bool allow_bulk_op_insert);
void
_mongoc_write_command_init_delete (mongoc_write_command_t *command,
                                   const bson_t *selectors,
                                   const bson_t *cmd_opts,
                                   const bson_t *opts,
                                   mongoc_bulk_write_flags_t flags,
                                   int64_t operation_id);
void
_mongoc_write_command_init_update (mongoc_write_command_t *command,
                                   const bson_t *selector,
                                   const bson_t *update,
                                   const bson_t *opts,
                                   mongoc_bulk_write_flags_t flags,
                                   int64_t operation_id);
void
_mongoc_write_command_insert_append (mongoc_write_command_t *command,
                                     const bson_t *document);
void
_mongoc_write_command_update_append (mongoc_write_command_t *command,
                                     const bson_t *selector,
                                     const bson_t *update,
                                     const bson_t *opts);

void
_mongoc_write_command_delete_append (mongoc_write_command_t *command,
                                     const bson_t *selector,
                                     const bson_t *opts);

void
_mongoc_write_command_too_large_error (bson_error_t *error,
                                       int32_t idx,
                                       int32_t len,
                                       int32_t max_bson_size);
void
_mongoc_write_command_execute (mongoc_write_command_t *command,
                               mongoc_client_t *client,
                               mongoc_server_stream_t *server_stream,
                               const char *database,
                               const char *collection,
                               const mongoc_write_concern_t *write_concern,
                               uint32_t offset,
                               mongoc_client_session_t *cs,
                               mongoc_write_result_t *result);
void
_mongoc_write_result_init (mongoc_write_result_t *result);
void
_mongoc_write_result_append_upsert (mongoc_write_result_t *result,
                                    int32_t idx,
                                    const bson_value_t *value);
int32_t
_mongoc_write_result_merge_arrays (uint32_t offset,
                                   mongoc_write_result_t *result,
                                   bson_t *dest,
                                   bson_iter_t *iter);
void
_mongoc_write_result_merge (mongoc_write_result_t *result,
                            mongoc_write_command_t *command,
                            const bson_t *reply,
                            uint32_t offset);
#define MONGOC_WRITE_RESULT_COMPLETE(_result, ...) \
   _mongoc_write_result_complete (_result, __VA_ARGS__, NULL)
bool
_mongoc_write_result_complete (mongoc_write_result_t *result,
                               int32_t error_api_version,
                               const mongoc_write_concern_t *wc,
                               mongoc_error_domain_t err_domain_override,
                               bson_t *reply,
                               bson_error_t *error,
                               ...);
void
_mongoc_write_result_destroy (mongoc_write_result_t *result);

void
_append_array_from_command (mongoc_write_command_t *command, bson_t *bson);


BSON_END_DECLS


#endif /* MONGOC_WRITE_COMMAND_PRIVATE_H */
