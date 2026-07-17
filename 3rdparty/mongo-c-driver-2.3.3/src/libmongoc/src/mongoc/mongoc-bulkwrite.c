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

#include <mongoc/mongoc-bulkwrite.h>

#include <common-macros-private.h> // MC_ENABLE_CONVERSION_WARNING_BEGIN
#include <mongoc/mongoc-array-private.h>
#include <mongoc/mongoc-buffer-private.h>
#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-client-side-encryption-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-server-stream-private.h>
#include <mongoc/mongoc-util-private.h> // _mongoc_iter_document_as_bson

#include <mongoc/mcd-nsinfo.h>
#include <mongoc/mongoc-optional.h>

#include <bson/bson.h>

#include <mlib/cmp.h>
#include <mlib/intencode.h>

MC_ENABLE_CONVERSION_WARNING_BEGIN

struct _mongoc_bulkwriteopts_t {
   mongoc_optional_t ordered;
   mongoc_optional_t bypassdocumentvalidation;
   bson_t *let;
   mongoc_write_concern_t *writeconcern;
   mongoc_optional_t verboseresults;
   bson_value_t comment;
   bson_t *extra;
   uint32_t serverid;
};

// `set_bson_opt` sets `*dst` by copying `src`. If `src` is NULL, `dst` is cleared.
static void
set_bson_opt(bson_t **dst, const bson_t *src)
{
   BSON_ASSERT_PARAM(dst);
   bson_destroy(*dst);
   *dst = NULL;
   if (src) {
      *dst = bson_copy(src);
   }
}

static void
set_bson_value_opt(bson_value_t *dst, const bson_value_t *src)
{
   BSON_ASSERT_PARAM(dst);
   bson_value_destroy(dst);
   *dst = (bson_value_t){0};
   if (src) {
      bson_value_copy(src, dst);
   }
}

mongoc_bulkwriteopts_t *
mongoc_bulkwriteopts_new(void)
{
   return bson_malloc0(sizeof(mongoc_bulkwriteopts_t));
}
void
mongoc_bulkwriteopts_set_ordered(mongoc_bulkwriteopts_t *self, bool ordered)
{
   BSON_ASSERT_PARAM(self);
   mongoc_optional_set_value(&self->ordered, ordered);
}
void
mongoc_bulkwriteopts_set_bypassdocumentvalidation(mongoc_bulkwriteopts_t *self, bool bypassdocumentvalidation)
{
   BSON_ASSERT_PARAM(self);
   mongoc_optional_set_value(&self->bypassdocumentvalidation, bypassdocumentvalidation);
}
void
mongoc_bulkwriteopts_set_let(mongoc_bulkwriteopts_t *self, const bson_t *let)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(let);
   set_bson_opt(&self->let, let);
}
void
mongoc_bulkwriteopts_set_writeconcern(mongoc_bulkwriteopts_t *self, const mongoc_write_concern_t *writeconcern)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(writeconcern);
   mongoc_write_concern_destroy(self->writeconcern);
   self->writeconcern = mongoc_write_concern_copy(writeconcern);
}
void
mongoc_bulkwriteopts_set_verboseresults(mongoc_bulkwriteopts_t *self, bool verboseresults)
{
   BSON_ASSERT_PARAM(self);
   mongoc_optional_set_value(&self->verboseresults, verboseresults);
}
void
mongoc_bulkwriteopts_set_comment(mongoc_bulkwriteopts_t *self, const bson_value_t *comment)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(comment);
   set_bson_value_opt(&self->comment, comment);
}
void
mongoc_bulkwriteopts_set_extra(mongoc_bulkwriteopts_t *self, const bson_t *extra)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(extra);
   set_bson_opt(&self->extra, extra);
}
void
mongoc_bulkwriteopts_set_serverid(mongoc_bulkwriteopts_t *self, uint32_t serverid)
{
   BSON_ASSERT_PARAM(self);
   self->serverid = serverid;
}
void
mongoc_bulkwriteopts_destroy(mongoc_bulkwriteopts_t *self)
{
   if (!self) {
      return;
   }
   bson_destroy(self->extra);
   bson_value_destroy(&self->comment);
   mongoc_write_concern_destroy(self->writeconcern);
   bson_destroy(self->let);
   bson_free(self);
}

typedef enum { MODEL_OP_INSERT, MODEL_OP_UPDATE, MODEL_OP_DELETE } model_op_t;
typedef struct {
   model_op_t op;
   // `id_loc` locates the "_id" field of an insert document.
   struct {
      size_t op_start;    // Offset in `mongoc_bulkwrite_t::ops` to the BSON for the insert op: { "document": ... }
      size_t op_len;      // Length of insert op.
      uint32_t id_offset; // Offset in the insert op to the "_id" field.
   } id_loc;
   char *ns;
} modeldata_t;

struct _mongoc_bulkwrite_t {
   mongoc_client_t *client;
   // `executed` is set to true once `mongoc_bulkwrite_execute` is called.
   // `mongoc_bulkwrite_t` may not be executed more than once.
   bool executed;
   // `serverid` is set in `mongoc_bulkwrite_execute` to identify the last used serverid. For acknowledged writes, this
   // will be the same as `mongoc_bulkwriteresult_serverid`.
   struct {
      bool is_set;
      uint32_t value;
   } serverid;
   // `is_acknowledged` is set in `mongoc_bulkwrite_execute` based on the chosen write concern.
   mongoc_optional_t is_acknowledged;
   // `ops` is a document sequence.
   mongoc_buffer_t ops;
   size_t n_ops;
   // `arrayof_modeldata` is an array of `modeldata_t` sized to the number of models. It stores per-model data.
   mongoc_array_t arrayof_modeldata;
   // `max_insert_len` tracks the maximum length of any document to-be inserted.
   uint32_t max_insert_len;
   // `has_multi_write` is true if there are any multi-document update or delete operations. Multi-document
   // writes are ineligible for retryable writes.
   bool has_multi_write;
   int64_t operation_id;
   mongoc_client_session_t *session;
};


// `mongoc_client_bulkwrite_new` creates a new bulk write operation.
mongoc_bulkwrite_t *
mongoc_client_bulkwrite_new(mongoc_client_t *self)
{
   BSON_ASSERT_PARAM(self);
   mongoc_bulkwrite_t *bw = mongoc_bulkwrite_new();
   bw->client = self;
   bw->operation_id = ++self->cluster.operation_id;
   return bw;
}

mongoc_bulkwrite_t *
mongoc_bulkwrite_new(void)
{
   mongoc_bulkwrite_t *bw = bson_malloc0(sizeof(mongoc_bulkwrite_t));
   mongoc_optional_init(&bw->is_acknowledged);
   _mongoc_buffer_init(&bw->ops, NULL, 0, NULL, NULL);
   _mongoc_array_init(&bw->arrayof_modeldata, sizeof(modeldata_t));
   return bw;
}

void
mongoc_bulkwrite_destroy(mongoc_bulkwrite_t *self)
{
   if (!self) {
      return;
   }
   for (size_t i = 0; i < self->arrayof_modeldata.len; i++) {
      modeldata_t md = _mongoc_array_index(&self->arrayof_modeldata, modeldata_t, i);
      bson_free(md.ns);
   }
   _mongoc_array_destroy(&self->arrayof_modeldata);
   _mongoc_buffer_destroy(&self->ops);
   bson_free(self);
}

struct _mongoc_bulkwrite_insertoneopts_t {
   // No fields yet. Include an unused placeholder to prevent compile errors due to an empty struct.
   int unused;
};

mongoc_bulkwrite_insertoneopts_t *
mongoc_bulkwrite_insertoneopts_new(void)
{
   return bson_malloc0(sizeof(mongoc_bulkwrite_insertoneopts_t));
}

void
mongoc_bulkwrite_insertoneopts_destroy(mongoc_bulkwrite_insertoneopts_t *self)
{
   if (!self) {
      return;
   }
   bson_free(self);
}

#define ERROR_IF_EXECUTED                                                                                              \
   if (self->executed) {                                                                                               \
      _mongoc_set_error(error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed"); \
      return false;                                                                                                    \
   } else                                                                                                              \
      (void)0

bool
mongoc_bulkwrite_append_insertone(mongoc_bulkwrite_t *self,
                                  const char *ns,
                                  const bson_t *document,
                                  const mongoc_bulkwrite_insertoneopts_t *opts, // may be NULL
                                  bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(ns);
   BSON_ASSERT_PARAM(document);
   BSON_ASSERT(document->len >= 5);
   BSON_OPTIONAL_PARAM(opts);
   BSON_OPTIONAL_PARAM(error);

   ERROR_IF_EXECUTED;

   bson_t op = BSON_INITIALIZER;
   BSON_ASSERT(BSON_APPEND_INT32(&op, "insert", -1)); // Append -1 as a placeholder. Will be overwritten later.

   // `persisted_id_offset` is the byte offset the `_id` in `op`.
   uint32_t persisted_id_offset = 0;
   {
      // Refer: bsonspec.org for BSON format.
      persisted_id_offset += 4;                                 // Document length.
      persisted_id_offset += 1;                                 // BSON type for int32.
      persisted_id_offset += (uint32_t)strlen("insert") + 1u;   // Key + 1 for NULL byte.
      persisted_id_offset += 4;                                 // int32 value.
      persisted_id_offset += 1;                                 // BSON type for document.
      persisted_id_offset += (uint32_t)strlen("document") + 1u; // Key + 1 for NULL byte.
   }

   // If `document` does not contain `_id`, add one in the beginning.
   bson_iter_t existing_id_iter;
   if (!bson_iter_init_find(&existing_id_iter, document, "_id")) {
      bson_t tmp = BSON_INITIALIZER;
      bson_oid_t oid;
      bson_oid_init(&oid, NULL);
      BSON_ASSERT(BSON_APPEND_OID(&tmp, "_id", &oid));
      BSON_ASSERT(bson_concat(&tmp, document));
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "document", &tmp));
      self->max_insert_len = BSON_MAX(self->max_insert_len, tmp.len);
      bson_destroy(&tmp);
      persisted_id_offset += 4; // Document length.
   } else {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "document", document));
      self->max_insert_len = BSON_MAX(self->max_insert_len, document->len);
      // `existing_id_offset` is offset of `_id` in the input `document`.
      const uint32_t existing_id_offset = bson_iter_offset(&existing_id_iter);
      BSON_ASSERT(persisted_id_offset <= UINT32_MAX - existing_id_offset);
      persisted_id_offset += existing_id_offset;
   }

   size_t op_start = self->ops.len; // Save location of `op` to retrieve `_id` later.
   BSON_ASSERT(mlib_in_range(size_t, op.len));
   BSON_ASSERT(_mongoc_buffer_append(&self->ops, bson_get_data(&op), (size_t)op.len));

   self->n_ops++;
   modeldata_t md = {.op = MODEL_OP_INSERT,
                     .id_loc = {.op_start = op_start, .op_len = (size_t)op.len, .id_offset = persisted_id_offset},
                     .ns = bson_strdup(ns)};
   _mongoc_array_append_val(&self->arrayof_modeldata, md);
   bson_destroy(&op);
   return true;
}


static bool
validate_update(const bson_t *update, bool *is_pipeline, bson_error_t *error)
{
   BSON_ASSERT_PARAM(update);
   BSON_ASSERT_PARAM(is_pipeline);
   BSON_OPTIONAL_PARAM(error);

   bson_iter_t iter;
   *is_pipeline = _mongoc_document_is_pipeline(update);
   if (*is_pipeline) {
      return true;
   }

   BSON_ASSERT(bson_iter_init(&iter, update));

   if (bson_iter_next(&iter)) {
      const char *key = bson_iter_key(&iter);
      if (key[0] != '$') {
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "Invalid key '%s': update only works with $ operators"
                           " and pipelines",
                           key);

         return false;
      }
   }
   return true;
}

struct _mongoc_bulkwrite_updateoneopts_t {
   bson_t *arrayfilters;
   bson_t *collation;
   bson_value_t hint;
   mongoc_optional_t upsert;
   bson_t *sort;
};

mongoc_bulkwrite_updateoneopts_t *
mongoc_bulkwrite_updateoneopts_new(void)
{
   return bson_malloc0(sizeof(mongoc_bulkwrite_updateoneopts_t));
}
void
mongoc_bulkwrite_updateoneopts_set_arrayfilters(mongoc_bulkwrite_updateoneopts_t *self, const bson_t *arrayfilters)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(arrayfilters);
   set_bson_opt(&self->arrayfilters, arrayfilters);
}
void
mongoc_bulkwrite_updateoneopts_set_collation(mongoc_bulkwrite_updateoneopts_t *self, const bson_t *collation)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(collation);
   set_bson_opt(&self->collation, collation);
}
void
mongoc_bulkwrite_updateoneopts_set_hint(mongoc_bulkwrite_updateoneopts_t *self, const bson_value_t *hint)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(hint);
   set_bson_value_opt(&self->hint, hint);
}
void
mongoc_bulkwrite_updateoneopts_set_upsert(mongoc_bulkwrite_updateoneopts_t *self, bool upsert)
{
   BSON_ASSERT_PARAM(self);
   mongoc_optional_set_value(&self->upsert, upsert);
}
void
mongoc_bulkwrite_updateoneopts_set_sort(mongoc_bulkwrite_updateoneopts_t *self, const bson_t *sort)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(sort);
   set_bson_opt(&self->sort, sort);
}
void
mongoc_bulkwrite_updateoneopts_destroy(mongoc_bulkwrite_updateoneopts_t *self)
{
   if (!self) {
      return;
   }
   bson_destroy(self->arrayfilters);
   bson_destroy(self->collation);
   bson_value_destroy(&self->hint);
   bson_destroy(self->sort);
   bson_free(self);
}

bool
mongoc_bulkwrite_append_updateone(mongoc_bulkwrite_t *self,
                                  const char *ns,
                                  const bson_t *filter,
                                  const bson_t *update,
                                  const mongoc_bulkwrite_updateoneopts_t *opts /* May be NULL */,
                                  bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(ns);
   BSON_ASSERT_PARAM(filter);
   BSON_ASSERT(filter->len >= 5);
   BSON_ASSERT_PARAM(update);
   BSON_ASSERT(update->len >= 5);
   BSON_OPTIONAL_PARAM(opts);
   BSON_OPTIONAL_PARAM(error);

   ERROR_IF_EXECUTED;

   mongoc_bulkwrite_updateoneopts_t defaults = {0};
   if (!opts) {
      opts = &defaults;
   }

   bool is_pipeline = false;
   if (!validate_update(update, &is_pipeline, error)) {
      return false;
   }

   bson_t op = BSON_INITIALIZER;
   BSON_ASSERT(BSON_APPEND_INT32(&op, "update", -1)); // Append -1 as a placeholder. Will be overwritten later.
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "filter", filter));
   if (is_pipeline) {
      BSON_ASSERT(BSON_APPEND_ARRAY(&op, "updateMods", update));
   } else {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "updateMods", update));
   }
   BSON_ASSERT(BSON_APPEND_BOOL(&op, "multi", false));
   if (opts->arrayfilters) {
      BSON_ASSERT(BSON_APPEND_ARRAY(&op, "arrayFilters", opts->arrayfilters));
   }
   if (opts->collation) {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "collation", opts->collation));
   }
   if (opts->hint.value_type != BSON_TYPE_EOD) {
      BSON_ASSERT(BSON_APPEND_VALUE(&op, "hint", &opts->hint));
   }
   if (mongoc_optional_is_set(&opts->upsert)) {
      BSON_ASSERT(BSON_APPEND_BOOL(&op, "upsert", mongoc_optional_value(&opts->upsert)));
   }
   if (opts->sort) {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "sort", opts->sort));
   }

   BSON_ASSERT(_mongoc_buffer_append(&self->ops, bson_get_data(&op), op.len));

   self->n_ops++;
   modeldata_t md = {.op = MODEL_OP_UPDATE, .ns = bson_strdup(ns)};
   _mongoc_array_append_val(&self->arrayof_modeldata, md);
   bson_destroy(&op);
   return true;
}

struct _mongoc_bulkwrite_replaceoneopts_t {
   bson_t *collation;
   bson_value_t hint;
   mongoc_optional_t upsert;
   bson_t *sort;
};

mongoc_bulkwrite_replaceoneopts_t *
mongoc_bulkwrite_replaceoneopts_new(void)
{
   return bson_malloc0(sizeof(mongoc_bulkwrite_replaceoneopts_t));
}
void
mongoc_bulkwrite_replaceoneopts_set_collation(mongoc_bulkwrite_replaceoneopts_t *self, const bson_t *collation)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(collation);
   set_bson_opt(&self->collation, collation);
}
void
mongoc_bulkwrite_replaceoneopts_set_hint(mongoc_bulkwrite_replaceoneopts_t *self, const bson_value_t *hint)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(hint);
   set_bson_value_opt(&self->hint, hint);
}
void
mongoc_bulkwrite_replaceoneopts_set_upsert(mongoc_bulkwrite_replaceoneopts_t *self, bool upsert)
{
   BSON_ASSERT_PARAM(self);
   mongoc_optional_set_value(&self->upsert, upsert);
}
void
mongoc_bulkwrite_replaceoneopts_set_sort(mongoc_bulkwrite_replaceoneopts_t *self, const bson_t *sort)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(sort);
   set_bson_opt(&self->sort, sort);
}
void
mongoc_bulkwrite_replaceoneopts_destroy(mongoc_bulkwrite_replaceoneopts_t *self)
{
   if (!self) {
      return;
   }
   bson_destroy(self->collation);
   bson_value_destroy(&self->hint);
   bson_destroy(self->sort);
   bson_free(self);
}

bool
validate_replace(const bson_t *doc, bson_error_t *error)
{
   BSON_OPTIONAL_PARAM(doc);
   BSON_OPTIONAL_PARAM(error);

   bson_iter_t iter;

   BSON_ASSERT(bson_iter_init(&iter, doc));

   if (bson_iter_next(&iter)) {
      const char *key = bson_iter_key(&iter);
      if (key[0] == '$') {
         _mongoc_set_error(error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "Invalid key '%s': replace prohibits $ operators",
                           key);

         return false;
      }
   }

   return true;
}

bool
mongoc_bulkwrite_append_replaceone(mongoc_bulkwrite_t *self,
                                   const char *ns,
                                   const bson_t *filter,
                                   const bson_t *replacement,
                                   const mongoc_bulkwrite_replaceoneopts_t *opts /* May be NULL */,
                                   bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(ns);
   BSON_ASSERT_PARAM(filter);
   BSON_ASSERT(filter->len >= 5);
   BSON_ASSERT_PARAM(replacement);
   BSON_ASSERT(replacement->len >= 5);
   BSON_OPTIONAL_PARAM(opts);
   BSON_OPTIONAL_PARAM(error);

   ERROR_IF_EXECUTED;

   mongoc_bulkwrite_replaceoneopts_t defaults = {0};
   if (!opts) {
      opts = &defaults;
   }

   if (!validate_replace(replacement, error)) {
      return false;
   }

   bson_t op = BSON_INITIALIZER;
   BSON_ASSERT(BSON_APPEND_INT32(&op, "update", -1)); // Append -1 as a placeholder. Will be overwritten later.
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "filter", filter));
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "updateMods", replacement));
   BSON_ASSERT(BSON_APPEND_BOOL(&op, "multi", false));
   if (opts->collation) {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "collation", opts->collation));
   }
   if (opts->hint.value_type != BSON_TYPE_EOD) {
      BSON_ASSERT(BSON_APPEND_VALUE(&op, "hint", &opts->hint));
   }
   if (mongoc_optional_is_set(&opts->upsert)) {
      BSON_ASSERT(BSON_APPEND_BOOL(&op, "upsert", mongoc_optional_value(&opts->upsert)));
   }
   if (opts->sort) {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "sort", opts->sort));
   }

   BSON_ASSERT(_mongoc_buffer_append(&self->ops, bson_get_data(&op), op.len));

   self->n_ops++;
   self->max_insert_len = BSON_MAX(self->max_insert_len, replacement->len);
   modeldata_t md = {.op = MODEL_OP_UPDATE, .ns = bson_strdup(ns)};
   _mongoc_array_append_val(&self->arrayof_modeldata, md);
   bson_destroy(&op);
   return true;
}

struct _mongoc_bulkwrite_updatemanyopts_t {
   bson_t *arrayfilters;
   bson_t *collation;
   bson_value_t hint;
   mongoc_optional_t upsert;
};

mongoc_bulkwrite_updatemanyopts_t *
mongoc_bulkwrite_updatemanyopts_new(void)
{
   return bson_malloc0(sizeof(mongoc_bulkwrite_updatemanyopts_t));
}
void
mongoc_bulkwrite_updatemanyopts_set_arrayfilters(mongoc_bulkwrite_updatemanyopts_t *self, const bson_t *arrayfilters)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(arrayfilters);
   set_bson_opt(&self->arrayfilters, arrayfilters);
}
void
mongoc_bulkwrite_updatemanyopts_set_collation(mongoc_bulkwrite_updatemanyopts_t *self, const bson_t *collation)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(collation);
   set_bson_opt(&self->collation, collation);
}
void
mongoc_bulkwrite_updatemanyopts_set_hint(mongoc_bulkwrite_updatemanyopts_t *self, const bson_value_t *hint)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(hint);
   set_bson_value_opt(&self->hint, hint);
}
void
mongoc_bulkwrite_updatemanyopts_set_upsert(mongoc_bulkwrite_updatemanyopts_t *self, bool upsert)
{
   BSON_ASSERT_PARAM(self);
   mongoc_optional_set_value(&self->upsert, upsert);
}
void
mongoc_bulkwrite_updatemanyopts_destroy(mongoc_bulkwrite_updatemanyopts_t *self)
{
   if (!self) {
      return;
   }
   bson_destroy(self->arrayfilters);
   bson_destroy(self->collation);
   bson_value_destroy(&self->hint);
   bson_free(self);
}

bool
mongoc_bulkwrite_append_updatemany(mongoc_bulkwrite_t *self,
                                   const char *ns,
                                   const bson_t *filter,
                                   const bson_t *update,
                                   const mongoc_bulkwrite_updatemanyopts_t *opts /* May be NULL */,
                                   bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(ns);
   BSON_ASSERT_PARAM(filter);
   BSON_ASSERT(filter->len >= 5);
   BSON_ASSERT_PARAM(update);
   BSON_ASSERT(update->len >= 5);
   BSON_OPTIONAL_PARAM(opts);
   BSON_OPTIONAL_PARAM(error);

   ERROR_IF_EXECUTED;

   mongoc_bulkwrite_updatemanyopts_t defaults = {0};
   if (!opts) {
      opts = &defaults;
   }

   bool is_pipeline = false;
   if (!validate_update(update, &is_pipeline, error)) {
      return false;
   }

   bson_t op = BSON_INITIALIZER;
   BSON_ASSERT(BSON_APPEND_INT32(&op, "update", -1)); // Append -1 as a placeholder. Will be overwritten later.
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "filter", filter));
   if (is_pipeline) {
      BSON_ASSERT(BSON_APPEND_ARRAY(&op, "updateMods", update));
   } else {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "updateMods", update));
   }
   BSON_ASSERT(BSON_APPEND_BOOL(&op, "multi", true));
   if (opts->arrayfilters) {
      BSON_ASSERT(BSON_APPEND_ARRAY(&op, "arrayFilters", opts->arrayfilters));
   }
   if (opts->collation) {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "collation", opts->collation));
   }
   if (opts->hint.value_type != BSON_TYPE_EOD) {
      BSON_ASSERT(BSON_APPEND_VALUE(&op, "hint", &opts->hint));
   }
   if (mongoc_optional_is_set(&opts->upsert)) {
      BSON_ASSERT(BSON_APPEND_BOOL(&op, "upsert", mongoc_optional_value(&opts->upsert)));
   }

   BSON_ASSERT(_mongoc_buffer_append(&self->ops, bson_get_data(&op), op.len));

   self->has_multi_write = true;
   self->n_ops++;
   modeldata_t md = {.op = MODEL_OP_UPDATE, .ns = bson_strdup(ns)};
   _mongoc_array_append_val(&self->arrayof_modeldata, md);
   bson_destroy(&op);
   return true;
}

struct _mongoc_bulkwrite_deleteoneopts_t {
   bson_t *collation;
   bson_value_t hint;
};

mongoc_bulkwrite_deleteoneopts_t *
mongoc_bulkwrite_deleteoneopts_new(void)
{
   return bson_malloc0(sizeof(mongoc_bulkwrite_deleteoneopts_t));
}
void
mongoc_bulkwrite_deleteoneopts_set_collation(mongoc_bulkwrite_deleteoneopts_t *self, const bson_t *collation)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(collation);
   set_bson_opt(&self->collation, collation);
}
void
mongoc_bulkwrite_deleteoneopts_set_hint(mongoc_bulkwrite_deleteoneopts_t *self, const bson_value_t *hint)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(hint);
   set_bson_value_opt(&self->hint, hint);
}
void
mongoc_bulkwrite_deleteoneopts_destroy(mongoc_bulkwrite_deleteoneopts_t *self)
{
   if (!self) {
      return;
   }
   bson_value_destroy(&self->hint);
   bson_destroy(self->collation);
   bson_free(self);
}

bool
mongoc_bulkwrite_append_deleteone(mongoc_bulkwrite_t *self,
                                  const char *ns,
                                  const bson_t *filter,
                                  const mongoc_bulkwrite_deleteoneopts_t *opts /* May be NULL */,
                                  bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(ns);
   BSON_ASSERT_PARAM(filter);
   BSON_ASSERT(filter->len >= 5);
   BSON_OPTIONAL_PARAM(opts);
   BSON_OPTIONAL_PARAM(error);

   ERROR_IF_EXECUTED;

   mongoc_bulkwrite_deleteoneopts_t defaults = {0};
   if (!opts) {
      opts = &defaults;
   }

   bson_t op = BSON_INITIALIZER;
   BSON_ASSERT(BSON_APPEND_INT32(&op, "delete", -1)); // Append -1 as a placeholder. Will be overwritten later.
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "filter", filter));
   BSON_ASSERT(BSON_APPEND_BOOL(&op, "multi", false));
   if (opts->collation) {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "collation", opts->collation));
   }
   if (opts->hint.value_type != BSON_TYPE_EOD) {
      BSON_ASSERT(BSON_APPEND_VALUE(&op, "hint", &opts->hint));
   }

   BSON_ASSERT(_mongoc_buffer_append(&self->ops, bson_get_data(&op), op.len));

   self->n_ops++;
   modeldata_t md = {.op = MODEL_OP_DELETE, .ns = bson_strdup(ns)};
   _mongoc_array_append_val(&self->arrayof_modeldata, md);
   bson_destroy(&op);
   return true;
}

struct _mongoc_bulkwrite_deletemanyopts_t {
   bson_t *collation;
   bson_value_t hint;
};

mongoc_bulkwrite_deletemanyopts_t *
mongoc_bulkwrite_deletemanyopts_new(void)
{
   return bson_malloc0(sizeof(mongoc_bulkwrite_deletemanyopts_t));
}
void
mongoc_bulkwrite_deletemanyopts_set_collation(mongoc_bulkwrite_deletemanyopts_t *self, const bson_t *collation)
{
   BSON_ASSERT_PARAM(self);
   set_bson_opt(&self->collation, collation);
}
void
mongoc_bulkwrite_deletemanyopts_set_hint(mongoc_bulkwrite_deletemanyopts_t *self, const bson_value_t *hint)
{
   BSON_ASSERT_PARAM(self);
   set_bson_value_opt(&self->hint, hint);
}
void
mongoc_bulkwrite_deletemanyopts_destroy(mongoc_bulkwrite_deletemanyopts_t *self)
{
   if (!self) {
      return;
   }
   bson_value_destroy(&self->hint);
   bson_destroy(self->collation);
   bson_free(self);
}

bool
mongoc_bulkwrite_append_deletemany(mongoc_bulkwrite_t *self,
                                   const char *ns,
                                   const bson_t *filter,
                                   const mongoc_bulkwrite_deletemanyopts_t *opts /* May be NULL */,
                                   bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(ns);
   BSON_ASSERT_PARAM(filter);
   BSON_ASSERT(filter->len >= 5);
   BSON_OPTIONAL_PARAM(opts);
   BSON_OPTIONAL_PARAM(error);

   ERROR_IF_EXECUTED;

   mongoc_bulkwrite_deletemanyopts_t defaults = {0};
   if (!opts) {
      opts = &defaults;
   }

   bson_t op = BSON_INITIALIZER;
   BSON_ASSERT(BSON_APPEND_INT32(&op, "delete", -1)); // Append -1 as a placeholder. Will be overwritten later.
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "filter", filter));
   BSON_ASSERT(BSON_APPEND_BOOL(&op, "multi", true));
   if (opts->collation) {
      BSON_ASSERT(BSON_APPEND_DOCUMENT(&op, "collation", opts->collation));
   }
   if (opts->hint.value_type != BSON_TYPE_EOD) {
      BSON_ASSERT(BSON_APPEND_VALUE(&op, "hint", &opts->hint));
   }

   BSON_ASSERT(_mongoc_buffer_append(&self->ops, bson_get_data(&op), op.len));

   self->has_multi_write = true;
   self->n_ops++;
   modeldata_t md = {.op = MODEL_OP_DELETE, .ns = bson_strdup(ns)};
   _mongoc_array_append_val(&self->arrayof_modeldata, md);
   bson_destroy(&op);
   return true;
}


struct _mongoc_bulkwriteresult_t {
   int64_t insertedcount;
   int64_t upsertedcount;
   int64_t matchedcount;
   int64_t modifiedcount;
   int64_t deletedcount;
   int64_t errorscount; // sum of all `nErrors`.
   struct {
      bool isset;
      int64_t index;
   } first_error_index;
   uint32_t serverid;
   bson_t insertresults;
   bson_t updateresults;
   bson_t deleteresults;
   bool verboseresults;
   // `parsed_some_results` becomes true if an ok:1 reply to `bulkWrite` is successfully parsed.
   // Used to determine whether some writes were successful.
   bool parsed_some_results;
};

int64_t
mongoc_bulkwriteresult_insertedcount(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   return self->insertedcount;
}

int64_t
mongoc_bulkwriteresult_upsertedcount(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   return self->upsertedcount;
}

int64_t
mongoc_bulkwriteresult_matchedcount(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   return self->matchedcount;
}

int64_t
mongoc_bulkwriteresult_modifiedcount(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   return self->modifiedcount;
}

int64_t
mongoc_bulkwriteresult_deletedcount(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   return self->deletedcount;
}

const bson_t *
mongoc_bulkwriteresult_insertresults(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   if (!self->verboseresults) {
      return NULL;
   }
   return &self->insertresults;
}

const bson_t *
mongoc_bulkwriteresult_updateresults(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   if (!self->verboseresults) {
      return NULL;
   }
   return &self->updateresults;
}

const bson_t *
mongoc_bulkwriteresult_deleteresults(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   if (!self->verboseresults) {
      return NULL;
   }
   return &self->deleteresults;
}

uint32_t
mongoc_bulkwriteresult_serverid(const mongoc_bulkwriteresult_t *self)
{
   BSON_ASSERT_PARAM(self);
   return self->serverid;
}

void
mongoc_bulkwriteresult_destroy(mongoc_bulkwriteresult_t *self)
{
   if (!self) {
      return;
   }
   bson_destroy(&self->deleteresults);
   bson_destroy(&self->updateresults);
   bson_destroy(&self->insertresults);
   bson_free(self);
}

static mongoc_bulkwriteresult_t *
_bulkwriteresult_new(void)
{
   mongoc_bulkwriteresult_t *self = bson_malloc0(sizeof(*self));
   bson_init(&self->insertresults);
   bson_init(&self->updateresults);
   bson_init(&self->deleteresults);
   return self;
}

static void
_bulkwriteresult_set_updateresult(
   mongoc_bulkwriteresult_t *self, int64_t n, int64_t nModified, const bson_value_t *upserted_id, size_t models_idx)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(upserted_id);

   bson_t updateresult;
   {
      char *key = bson_strdup_printf("%zu", models_idx);
      BSON_APPEND_DOCUMENT_BEGIN(&self->updateresults, key, &updateresult);
      bson_free(key);
   }

   BSON_ASSERT(BSON_APPEND_INT64(&updateresult, "matchedCount", n));
   BSON_ASSERT(BSON_APPEND_INT64(&updateresult, "modifiedCount", nModified));
   if (upserted_id) {
      BSON_ASSERT(BSON_APPEND_VALUE(&updateresult, "upsertedId", upserted_id));
   }
   BSON_ASSERT(bson_append_document_end(&self->updateresults, &updateresult));
}

static void
_bulkwriteresult_set_deleteresult(mongoc_bulkwriteresult_t *self, int64_t n, size_t models_idx)
{
   BSON_ASSERT_PARAM(self);

   bson_t deleteresult;
   {
      char *key = bson_strdup_printf("%zu", models_idx);
      BSON_APPEND_DOCUMENT_BEGIN(&self->deleteresults, key, &deleteresult);
      bson_free(key);
   }

   BSON_ASSERT(BSON_APPEND_INT64(&deleteresult, "deletedCount", n));
   BSON_ASSERT(bson_append_document_end(&self->deleteresults, &deleteresult));
}

static void
_bulkwriteresult_set_insertresult(mongoc_bulkwriteresult_t *self, const bson_iter_t *id_iter, size_t models_idx)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(id_iter);

   bson_t insertresult;
   {
      char *key = bson_strdup_printf("%zu", models_idx);
      BSON_APPEND_DOCUMENT_BEGIN(&self->insertresults, key, &insertresult);
      bson_free(key);
   }

   BSON_ASSERT(BSON_APPEND_ITER(&insertresult, "insertedId", id_iter));
   BSON_ASSERT(bson_append_document_end(&self->insertresults, &insertresult));
}

struct _mongoc_bulkwriteexception_t {
   bson_error_t error;
   bson_t error_reply;
   bson_t write_concern_errors;
   size_t write_concern_errors_len;
   bson_t write_errors;
   // If `has_any_error` is false, the bulk write exception is not returned.
   bool has_any_error;
};

static mongoc_bulkwriteexception_t *
_bulkwriteexception_new(void)
{
   mongoc_bulkwriteexception_t *self = bson_malloc0(sizeof(*self));
   bson_init(&self->write_concern_errors);
   bson_init(&self->write_errors);
   bson_init(&self->error_reply);
   return self;
}

// Returns true if there was a top-level error.
bool
mongoc_bulkwriteexception_error(const mongoc_bulkwriteexception_t *self, bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(error);

   if (self->error.code != 0) {
      memcpy(error, &self->error, sizeof(*error));
      return true;
   }
   return false; // No top-level error.
}

const bson_t *
mongoc_bulkwriteexception_writeerrors(const mongoc_bulkwriteexception_t *self)
{
   BSON_ASSERT_PARAM(self);
   return &self->write_errors;
}

const bson_t *
mongoc_bulkwriteexception_writeconcernerrors(const mongoc_bulkwriteexception_t *self)
{
   BSON_ASSERT_PARAM(self);
   return &self->write_concern_errors;
}

const bson_t *
mongoc_bulkwriteexception_errorreply(const mongoc_bulkwriteexception_t *self)
{
   BSON_ASSERT_PARAM(self);
   return &self->error_reply;
}

void
mongoc_bulkwriteexception_destroy(mongoc_bulkwriteexception_t *self)
{
   if (!self) {
      return;
   }
   bson_destroy(&self->write_errors);
   bson_destroy(&self->write_concern_errors);
   bson_destroy(&self->error_reply);
   bson_free(self);
}

static void
_bulkwriteexception_set_error(mongoc_bulkwriteexception_t *self, bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(error);

   BSON_ASSERT(error->code != 0);
   memcpy(&self->error, error, sizeof(*error));
   self->has_any_error = true;
}

static void
_bulkwriteexception_set_error_reply(mongoc_bulkwriteexception_t *self, const bson_t *error_reply)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(error_reply);

   bson_copy_to(error_reply, &self->error_reply);
   self->has_any_error = true;
}

static void
_bulkwriteexception_append_writeconcernerror(mongoc_bulkwriteexception_t *self,
                                             int32_t code,
                                             const char *errmsg,
                                             const bson_t *errInfo)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(errmsg);
   BSON_ASSERT_PARAM(errInfo);

   char *key = bson_strdup_printf("%zu", self->write_concern_errors_len);
   self->write_concern_errors_len++;

   bson_t write_concern_error;
   BSON_ASSERT(BSON_APPEND_DOCUMENT_BEGIN(&self->write_concern_errors, key, &write_concern_error));
   BSON_ASSERT(BSON_APPEND_INT32(&write_concern_error, "code", code));
   BSON_ASSERT(BSON_APPEND_UTF8(&write_concern_error, "message", errmsg));
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&write_concern_error, "details", errInfo));
   BSON_ASSERT(bson_append_document_end(&self->write_concern_errors, &write_concern_error));
   self->has_any_error = true;
   bson_free(key);
}

static void
_bulkwriteexception_set_writeerror(
   mongoc_bulkwriteexception_t *self, int32_t code, const char *errmsg, const bson_t *errInfo, size_t models_idx)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(errmsg);
   BSON_ASSERT_PARAM(errInfo);

   bson_t write_error;
   {
      char *key = bson_strdup_printf("%zu", models_idx);
      BSON_APPEND_DOCUMENT_BEGIN(&self->write_errors, key, &write_error);
      bson_free(key);
   }

   BSON_ASSERT(BSON_APPEND_INT32(&write_error, "code", code));
   BSON_ASSERT(BSON_APPEND_UTF8(&write_error, "message", errmsg));
   BSON_ASSERT(BSON_APPEND_DOCUMENT(&write_error, "details", errInfo));
   BSON_ASSERT(bson_append_document_end(&self->write_errors, &write_error));
   self->has_any_error = true;
}

static bool
lookup_int32(const bson_t *bson, const char *key, int32_t *out, const char *source, mongoc_bulkwriteexception_t *exc)
{
   BSON_ASSERT_PARAM(bson);
   BSON_ASSERT_PARAM(key);
   BSON_ASSERT_PARAM(out);
   BSON_OPTIONAL_PARAM(source);
   BSON_ASSERT_PARAM(exc);

   bson_iter_t iter;
   if (bson_iter_init_find(&iter, bson, key) && BSON_ITER_HOLDS_INT32(&iter)) {
      *out = bson_iter_int32(&iter);
      return true;
   }
   bson_error_t error;
   if (source) {
      _mongoc_set_error(&error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "expected to find int32 `%s` in %s, but did not",
                        key,
                        source);
   } else {
      _mongoc_set_error(&error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "expected to find int32 `%s`, but did not",
                        key);
   }
   _bulkwriteexception_set_error(exc, &error);
   return false;
}

// `lookup_as_int64` looks for `key` as a BSON int32, int64, or double and returns as an int64_t. Doubles are truncated.
static int64_t
lookup_as_int64(const bson_t *bson, const char *key, int64_t *out, const char *source, mongoc_bulkwriteexception_t *exc)
{
   BSON_ASSERT_PARAM(bson);
   BSON_ASSERT_PARAM(key);
   BSON_ASSERT_PARAM(out);
   BSON_OPTIONAL_PARAM(source);
   BSON_ASSERT_PARAM(exc);

   bson_iter_t iter;
   if (bson_iter_init_find(&iter, bson, key) && BSON_ITER_HOLDS_NUMBER(&iter)) {
      *out = bson_iter_as_int64(&iter);
      return true;
   }
   bson_error_t error;
   if (source) {
      _mongoc_set_error(&error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "expected to find int32, int64, or double `%s` in %s, but did not",
                        key,
                        source);
   } else {
      _mongoc_set_error(&error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "expected to find int32, int64, or double `%s`, but did not",
                        key);
   }
   _bulkwriteexception_set_error(exc, &error);
   return false;
}

static bool
lookup_string(
   const bson_t *bson, const char *key, const char **out, const char *source, mongoc_bulkwriteexception_t *exc)
{
   BSON_ASSERT_PARAM(bson);
   BSON_ASSERT_PARAM(key);
   BSON_ASSERT_PARAM(out);
   BSON_OPTIONAL_PARAM(source);
   BSON_ASSERT_PARAM(exc);

   bson_iter_t iter;
   if (bson_iter_init_find(&iter, bson, key) && BSON_ITER_HOLDS_UTF8(&iter)) {
      *out = bson_iter_utf8(&iter, NULL);
      return true;
   }
   bson_error_t error;
   if (source) {
      _mongoc_set_error(&error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "expected to find string `%s` in %s, but did not",
                        key,
                        source);
   } else {
      _mongoc_set_error(&error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "expected to find string `%s`, but did not",
                        key);
   }
   _bulkwriteexception_set_error(exc, &error);
   return false;
}

// `_bulkwritereturn_apply_reply` applies the top-level fields of a server reply to the returned results.
static bool
_bulkwritereturn_apply_reply(mongoc_bulkwritereturn_t *self, const bson_t *cmd_reply)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(cmd_reply);

   // Parse top-level fields.
   // These fields are expected to be int32 as of server 8.0. However, drivers return the values as int64.
   // Use `lookup_as_int64` to support other numeric types to future-proof.
   int64_t nInserted;
   if (!lookup_as_int64(cmd_reply, "nInserted", &nInserted, NULL, self->exc)) {
      return false;
   }
   self->res->insertedcount += nInserted;

   int64_t nMatched;
   if (!lookup_as_int64(cmd_reply, "nMatched", &nMatched, NULL, self->exc)) {
      return false;
   }
   self->res->matchedcount += nMatched;

   int64_t nModified;
   if (!lookup_as_int64(cmd_reply, "nModified", &nModified, NULL, self->exc)) {
      return false;
   }
   self->res->modifiedcount += nModified;

   int64_t nDeleted;
   if (!lookup_as_int64(cmd_reply, "nDeleted", &nDeleted, NULL, self->exc)) {
      return false;
   }
   self->res->deletedcount += nDeleted;

   int64_t nUpserted;
   if (!lookup_as_int64(cmd_reply, "nUpserted", &nUpserted, NULL, self->exc)) {
      return false;
   }
   self->res->upsertedcount += nUpserted;

   int64_t nErrors;
   if (!lookup_as_int64(cmd_reply, "nErrors", &nErrors, NULL, self->exc)) {
      return false;
   }
   self->res->errorscount += nErrors;

   bson_error_t error;
   bson_iter_t iter;
   if (bson_iter_init_find(&iter, cmd_reply, "writeConcernError")) {
      bson_iter_t wce_iter;
      bson_t wce_bson;

      if (!_mongoc_iter_document_as_bson(&iter, &wce_bson, &error)) {
         _bulkwriteexception_set_error(self->exc, &error);
         _bulkwriteexception_set_error_reply(self->exc, cmd_reply);
         return false;
      }

      // Parse `code`.
      int32_t code;
      if (!lookup_int32(&wce_bson, "code", &code, "writeConcernError", self->exc)) {
         return false;
      }

      // Parse `errmsg`.
      const char *errmsg;
      if (!lookup_string(&wce_bson, "errmsg", &errmsg, "writeConcernError", self->exc)) {
         return false;
      }

      // Parse optional `errInfo`.
      bson_t errInfo = BSON_INITIALIZER;
      if (bson_iter_init_find(&wce_iter, &wce_bson, "errInfo")) {
         if (!_mongoc_iter_document_as_bson(&wce_iter, &errInfo, &error)) {
            _bulkwriteexception_set_error(self->exc, &error);
            _bulkwriteexception_set_error_reply(self->exc, cmd_reply);
            return false;
         }
      }

      _bulkwriteexception_append_writeconcernerror(self->exc, code, errmsg, &errInfo);
   }

   self->res->parsed_some_results = true;

   return true;
}

// `_bulkwritereturn_apply_result` applies an individual cursor result to the returned results.
static bool
_bulkwritereturn_apply_result(mongoc_bulkwritereturn_t *self,
                              const bson_t *result,
                              size_t ops_doc_offset,
                              const mongoc_array_t *arrayof_modeldata,
                              const mongoc_buffer_t *ops)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(result);
   BSON_ASSERT_PARAM(arrayof_modeldata);

   bson_error_t error;

   // Parse for `ok`.
   int64_t ok;
   if (!lookup_as_int64(result, "ok", &ok, "result", self->exc)) {
      return false;
   }

   // Parse `idx`.
   int64_t idx;
   {
      if (!lookup_as_int64(result, "idx", &idx, "result", self->exc)) {
         return false;
      }
      if (idx < 0) {
         _mongoc_set_error(&error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "expected to find non-negative int64 `idx` in "
                           "result, but did not");
         _bulkwriteexception_set_error(self->exc, &error);
         return false;
      }
   }

   BSON_ASSERT(mlib_in_range(size_t, idx));
   // `models_idx` is the index of the model that produced this result.
   size_t models_idx = (size_t)idx + ops_doc_offset;
   if (ok == 0) {
      if (!self->res->first_error_index.isset) {
         self->res->first_error_index.isset = true;
         self->res->first_error_index.index = idx;
      }
      bson_iter_t result_iter;

      // Parse `code`.
      int32_t code;
      if (!lookup_int32(result, "code", &code, "result", self->exc)) {
         return false;
      }

      // Parse `errmsg`.
      const char *errmsg;
      if (!lookup_string(result, "errmsg", &errmsg, "result", self->exc)) {
         return false;
      }

      // Parse optional `errInfo`.
      bson_t errInfo = BSON_INITIALIZER;
      if (bson_iter_init_find(&result_iter, result, "errInfo")) {
         if (!_mongoc_iter_document_as_bson(&result_iter, &errInfo, &error)) {
            _bulkwriteexception_set_error(self->exc, &error);
            return false;
         }
      }

      // Store a copy of the write error.
      _bulkwriteexception_set_writeerror(self->exc, code, errmsg, &errInfo, models_idx);
   } else {
      // This is a successful result of an individual operation.
      // Server only reports successful results of individual
      // operations when verbose results are requested
      // (`errorsOnly: false` is sent).

      modeldata_t *md = &_mongoc_array_index(arrayof_modeldata, modeldata_t, models_idx);
      // Check if model is an update.
      switch (md->op) {
      case MODEL_OP_UPDATE: {
         bson_iter_t result_iter;
         // Parse `n`.
         int64_t n;
         if (!lookup_as_int64(result, "n", &n, "result", self->exc)) {
            return false;
         }

         // Parse `nModified`.
         int64_t nModified;
         bson_iter_t nModified_iter;
         if (!bson_iter_init_find(&nModified_iter, result, "nModified")) {
            // `nModified` is expected for update results, but may be missing due to SERVER-113026. Default to 0.
            nModified = 0;
         } else if (!lookup_as_int64(result, "nModified", &nModified, "result", self->exc)) {
            return false;
         }

         // Check for an optional `upserted._id`.
         const bson_value_t *upserted_id = NULL;
         bson_iter_t id_iter;
         if (bson_iter_init_find(&result_iter, result, "upserted")) {
            BSON_ASSERT(bson_iter_init(&result_iter, result));
            if (!bson_iter_find_descendant(&result_iter, "upserted._id", &id_iter)) {
               _mongoc_set_error(&error,
                                 MONGOC_ERROR_COMMAND,
                                 MONGOC_ERROR_COMMAND_INVALID_ARG,
                                 "expected `upserted` to be a document "
                                 "containing `_id`, but did not find `_id`");
               _bulkwriteexception_set_error(self->exc, &error);
               return false;
            }
            upserted_id = bson_iter_value(&id_iter);
         }

         _bulkwriteresult_set_updateresult(self->res, n, nModified, upserted_id, models_idx);
         break;
      }
      case MODEL_OP_DELETE: {
         // Parse `n`.
         int64_t n;
         if (!lookup_as_int64(result, "n", &n, "result", self->exc)) {
            return false;
         }

         _bulkwriteresult_set_deleteresult(self->res, n, models_idx);
         break;
      }
      case MODEL_OP_INSERT: {
         bson_iter_t id_iter;
         BSON_ASSERT(bson_iter_init_from_data_at_offset(
            &id_iter, ops->data + md->id_loc.op_start, md->id_loc.op_len, md->id_loc.id_offset, strlen("_id")));
         _bulkwriteresult_set_insertresult(self->res, &id_iter, models_idx);
         break;
      }
      default:
         // Add an unreachable default case to silence `switch-default` warnings.
         BSON_UNREACHABLE("unexpected default");
      }
   }
   return true;
}

void
mongoc_bulkwrite_set_client(mongoc_bulkwrite_t *self, mongoc_client_t *client)
{
   BSON_ASSERT_PARAM(self);
   BSON_ASSERT_PARAM(client);

   if (self->session) {
      BSON_ASSERT(self->session->client == client);
   }

   /* NOP if the client is not changing; otherwise, assign it and increment and
    * fetch its operation_id. */
   if (self->client == client) {
      return;
   }

   self->client = client;
   self->operation_id = ++client->cluster.operation_id;
}

void
mongoc_bulkwrite_set_session(mongoc_bulkwrite_t *self, mongoc_client_session_t *session)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(session);

   if (self->client && session) {
      BSON_ASSERT(self->client == session->client);
   }

   self->session = session;
}

mongoc_bulkwritereturn_t
mongoc_bulkwrite_execute(mongoc_bulkwrite_t *self, const mongoc_bulkwriteopts_t *opts)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(opts);

   // `has_successful_results` is set to true if any `bulkWrite` reply indicates some writes succeeded.
   bool has_successful_results = false;
   mongoc_bulkwritereturn_t ret = {0};
   bson_error_t error = {0};
   mongoc_server_stream_t *ss = NULL;
   bson_t cmd = BSON_INITIALIZER;
   mongoc_cmd_parts_t parts = {{0}};
   mongoc_bulkwriteopts_t defaults = {{0}};

   if (!opts) {
      opts = &defaults;
   }
   bool is_ordered = mongoc_optional_is_set(&opts->ordered) ? mongoc_optional_value(&opts->ordered) : true; // default.
   // Create empty result and exception to collect results/errors from batches.
   ret.res = _bulkwriteresult_new();
   ret.exc = _bulkwriteexception_new();

   if (!self->client) {
      _mongoc_set_error(&error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "bulk write requires a client and one has not been set");
      _bulkwriteexception_set_error(ret.exc, &error);
      goto fail;
   }

   if (self->executed) {
      _mongoc_set_error(&error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "bulk write already executed");
      _bulkwriteexception_set_error(ret.exc, &error);
      goto fail;
   }
   self->executed = true;

   if (self->n_ops == 0) {
      _mongoc_set_error(
         &error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "cannot do `bulkWrite` with no models");
      _bulkwriteexception_set_error(ret.exc, &error);
      goto fail;
   }

   const mongoc_ss_log_context_t ss_log_context = {
      .operation = "bulkWrite", .has_operation_id = true, .operation_id = self->operation_id};

   // Select a stream.
   {
      bson_t reply;

      if (opts->serverid) {
         ss = mongoc_cluster_stream_for_server(
            &self->client->cluster, opts->serverid, true /* reconnect_ok */, self->session, &reply, &error);
      } else {
         ss = mongoc_cluster_stream_for_writes(
            &self->client->cluster, &ss_log_context, self->session, NULL /* deprioritized servers */, &reply, &error);
      }

      if (!ss) {
         _bulkwriteexception_set_error(ret.exc, &error);
         _bulkwriteexception_set_error_reply(ret.exc, &reply);
         bson_destroy(&reply);
         goto fail;
      }
   }

   bool verboseresults =
      mongoc_optional_is_set(&opts->verboseresults) ? mongoc_optional_value(&opts->verboseresults) : false;
   ret.res->verboseresults = verboseresults;

   int32_t maxBsonObjectSize = mongoc_server_stream_max_bson_obj_size(ss);
   // Create the payload 0.
   {
      BSON_ASSERT(BSON_APPEND_INT32(&cmd, "bulkWrite", 1));
      // errorsOnly is default true. Set to false if verboseResults requested.
      BSON_ASSERT(BSON_APPEND_BOOL(&cmd, "errorsOnly", !verboseresults));
      // ordered is default true.
      BSON_ASSERT(BSON_APPEND_BOOL(&cmd, "ordered", is_ordered));

      if (opts->comment.value_type != BSON_TYPE_EOD) {
         BSON_ASSERT(BSON_APPEND_VALUE(&cmd, "comment", &opts->comment));
      }

      if (mongoc_optional_is_set(&opts->bypassdocumentvalidation)) {
         BSON_ASSERT(
            BSON_APPEND_BOOL(&cmd, "bypassDocumentValidation", mongoc_optional_value(&opts->bypassdocumentvalidation)));
      }

      if (opts->let) {
         BSON_ASSERT(BSON_APPEND_DOCUMENT(&cmd, "let", opts->let));
      }

      // Add optional extra fields.
      if (opts->extra) {
         BSON_ASSERT(bson_concat(&cmd, opts->extra));
      }

      mongoc_cmd_parts_init(&parts, self->client, "admin", MONGOC_QUERY_NONE, &cmd);
      parts.assembled.operation_id = self->operation_id;

      parts.allow_txn_number = MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_YES; // To append `lsid`.
      if (self->has_multi_write) {
         // Write commands that include multi-document operations are not
         // retryable.
         parts.allow_txn_number = MONGOC_CMD_PARTS_ALLOW_TXN_NUMBER_NO;
      }
      parts.is_write_command = true; // To append `txnNumber`.

      if (self->session) {
         mongoc_cmd_parts_set_session(&parts, self->session);
      }

      // Apply write concern:
      {
         const mongoc_write_concern_t *wc = self->client->write_concern; // Default to client.
         if (opts->writeconcern) {
            if (_mongoc_client_session_in_txn(self->session)) {
               _mongoc_set_error(&error,
                                 MONGOC_ERROR_COMMAND,
                                 MONGOC_ERROR_COMMAND_INVALID_ARG,
                                 "Cannot set write concern after starting a transaction.");
               _bulkwriteexception_set_error(ret.exc, &error);
               goto fail;
            }
            wc = opts->writeconcern;
         }
         if (!mongoc_cmd_parts_set_write_concern(&parts, wc, &error)) {
            _bulkwriteexception_set_error(ret.exc, &error);
            goto fail;
         }
         if (!mongoc_write_concern_is_acknowledged(wc) && mlib_cmp(self->max_insert_len, >, maxBsonObjectSize)) {
            _mongoc_set_error(&error,
                              MONGOC_ERROR_COMMAND,
                              MONGOC_ERROR_COMMAND_INVALID_ARG,
                              "Unacknowledged `bulkWrite` includes insert of size: %" PRIu32
                              ", exceeding maxBsonObjectSize: %" PRId32,
                              self->max_insert_len,
                              maxBsonObjectSize);
            _bulkwriteexception_set_error(ret.exc, &error);
            goto fail;
         }
         mongoc_optional_set_value(&self->is_acknowledged, mongoc_write_concern_is_acknowledged(wc));
      }

      if (verboseresults && !mongoc_optional_value(&self->is_acknowledged)) {
         _mongoc_set_error(&error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "Cannot request unacknowledged write concern and verbose results.");
         _bulkwriteexception_set_error(ret.exc, &error);
         goto fail;
      }

      if (is_ordered && !mongoc_optional_value(&self->is_acknowledged)) {
         _mongoc_set_error(&error,
                           MONGOC_ERROR_COMMAND,
                           MONGOC_ERROR_COMMAND_INVALID_ARG,
                           "Cannot request unacknowledged write concern and ordered writes.");
         _bulkwriteexception_set_error(ret.exc, &error);
         goto fail;
      }

      if (!mongoc_cmd_parts_assemble(&parts, ss, &error)) {
         _bulkwriteexception_set_error(ret.exc, &error);
         goto fail;
      }
   }

   const int32_t maxWriteBatchSize = mongoc_server_stream_max_write_batch_size(ss);
   int32_t maxMessageSizeBytes = mongoc_server_stream_max_msg_size(ss);
   if (_mongoc_cse_is_enabled(self->client)) {
      maxMessageSizeBytes = MONGOC_REDUCED_MAX_MSG_SIZE_FOR_FLE;
   }
   // `ops_doc_offset` is an offset into the `ops` document sequence. Counts the number of documents sent.
   size_t ops_doc_offset = 0;
   // `ops_byte_offset` is an offset into the `ops` document sequence. Counts the number of bytes sent.
   size_t ops_byte_offset = 0;
   // Calculate overhead of OP_MSG and the `bulkWrite` command. See bulk write specification for explanation.
   size_t opmsg_overhead = 0;
   {
      opmsg_overhead += 1000;
      // Add size of `bulkWrite` command. Exclude command-agnostic fields added in `mongoc_cmd_parts_assemble` (e.g.
      // `txnNumber` and `lsid`).
      opmsg_overhead += cmd.len;
   }

   // Send one or more `bulkWrite` commands. Split input payload if necessary to satisfy server size limits.
   while (true) {
      bool has_write_errors = false;
      bool batch_ok = false;
      bson_t cmd_reply = BSON_INITIALIZER;
      mongoc_cursor_t *reply_cursor = NULL;
      // `ops_byte_len` is the number of bytes from `ops` to send in this batch.
      size_t ops_byte_len = 0;
      // `ops_doc_len` is the number of documents from `ops` to send in this batch.
      size_t ops_doc_len = 0;

      if (ops_byte_offset == self->ops.len) {
         // All write models were sent.
         break;
      }

      // Track the nsInfo entries to include in this batch.
      mcd_nsinfo_t *nsinfo = mcd_nsinfo_new();

      // Read as many documents from payload as possible.
      while (true) {
         if (ops_byte_offset + ops_byte_len >= self->ops.len) {
            // All remaining ops are readied.
            break;
         }

         if (mlib_cmp(ops_doc_len, >=, maxWriteBatchSize)) {
            // Maximum number of operations are readied.
            break;
         }

         // Read length of next document.
         const uint32_t doc_len = mlib_read_u32le(self->ops.data + ops_byte_offset + ops_byte_len);

         // Check if adding this operation requires adding an `nsInfo` entry.
         // `models_idx` is the index of the model that produced this result.
         size_t models_idx = ops_doc_len + ops_doc_offset;
         modeldata_t *md = &_mongoc_array_index(&self->arrayof_modeldata, modeldata_t, models_idx);
         uint32_t nsinfo_bson_size = 0;
         int32_t ns_index = mcd_nsinfo_find(nsinfo, md->ns);
         if (ns_index == -1) {
            // Need to append `nsInfo` entry. Append after checking that both the document and the `nsInfo` entry fit.
            nsinfo_bson_size = mcd_nsinfo_get_bson_size(md->ns);
         }

         if (mlib_cmp(opmsg_overhead + ops_byte_len + doc_len + nsinfo_bson_size, >, maxMessageSizeBytes)) {
            if (ops_byte_len == 0) {
               // Could not even fit one document within an OP_MSG.
               _mongoc_set_error(&error,
                                 MONGOC_ERROR_COMMAND,
                                 MONGOC_ERROR_COMMAND_INVALID_ARG,
                                 "unable to send document at index %zu. Sending "
                                 "would exceed maxMessageSizeBytes=%" PRId32,
                                 ops_doc_len,
                                 maxMessageSizeBytes);
               _bulkwriteexception_set_error(ret.exc, &error);
               goto batch_fail;
            }
            break;
         }

         // Check if a new `nsInfo` entry is needed.
         if (ns_index == -1) {
            ns_index = mcd_nsinfo_append(nsinfo, md->ns, &error);
            if (ns_index == -1) {
               _bulkwriteexception_set_error(ret.exc, &error);
               goto batch_fail;
            }
         }

         // Overwrite the placeholder to the index of the `nsInfo` entry.
         {
            bson_iter_t nsinfo_iter;
            bson_t doc;
            BSON_ASSERT(bson_init_static(&doc, self->ops.data + ops_byte_offset + ops_byte_len, doc_len));
            // Find the index.
            BSON_ASSERT(bson_iter_init(&nsinfo_iter, &doc));
            BSON_ASSERT(bson_iter_next(&nsinfo_iter));
            bson_iter_overwrite_int32(&nsinfo_iter, ns_index);
         }

         // Include document.
         {
            ops_byte_len += doc_len;
            ops_doc_len += 1;
         }
      }

      // Send batch.
      {
         parts.assembled.payloads_count = 2;

         // Create the `nsInfo` payload.
         {
            mongoc_cmd_payload_t *payload = &parts.assembled.payloads[0];
            const mongoc_buffer_t *nsinfo_docseq = mcd_nsinfo_as_document_sequence(nsinfo);
            payload->documents = nsinfo_docseq->data;
            BSON_ASSERT(mlib_in_range(int32_t, nsinfo_docseq->len));
            payload->size = (int32_t)nsinfo_docseq->len;
            payload->identifier = "nsInfo";
         }

         // Create the `ops` payload.
         {
            mongoc_cmd_payload_t *payload = &parts.assembled.payloads[1];
            payload->identifier = "ops";
            payload->documents = self->ops.data + ops_byte_offset;
            BSON_ASSERT(mlib_in_range(int32_t, ops_byte_len));
            payload->size = (int32_t)ops_byte_len;
         }

         // Check if stream is valid. A previous call to `mongoc_cluster_run_retryable_write` may have invalidated
         // stream (e.g. due to processing an error). If invalid, select a new stream before processing more batches.
         if (!mongoc_cluster_stream_valid(&self->client->cluster, parts.assembled.server_stream)) {
            bson_t reply;
            // Select a server and create a stream again.
            mongoc_server_stream_cleanup(ss);
            ss = mongoc_cluster_stream_for_writes(&self->client->cluster,
                                                  &ss_log_context,
                                                  NULL /* session */,
                                                  NULL /* deprioritized servers */,
                                                  &reply,
                                                  &error);

            if (ss) {
               parts.assembled.server_stream = ss;
            } else {
               _bulkwriteexception_set_error(ret.exc, &error);
               _bulkwriteexception_set_error_reply(ret.exc, &reply);
               bson_destroy(&reply);
               goto batch_fail;
            }
         }

         // Send command.
         {
            mongoc_server_stream_t *new_ss = NULL;
            bool ok = mongoc_cluster_run_retryable_write(
               &self->client->cluster, &parts.assembled, parts.is_retryable_write, &new_ss, &cmd_reply, &error);
            if (new_ss) {
               // A retry occurred. Save the newly created stream to use for subsequent commands.
               mongoc_server_stream_cleanup(ss);
               ss = new_ss;
               parts.assembled.server_stream = ss;
            }

            // Check for a command ('ok': 0) error.
            if (!ok) {
               if (error.code != 0) {
                  // The original error was a command ('ok': 0) error.
                  _bulkwriteexception_set_error(ret.exc, &error);
               }
               _bulkwriteexception_set_error_reply(ret.exc, &cmd_reply);
               goto batch_fail;
            }
         }

         // Add to result and/or exception.
         if (mongoc_optional_value(&self->is_acknowledged)) {
            // Parse top-level fields.
            if (!_bulkwritereturn_apply_reply(&ret, &cmd_reply)) {
               goto batch_fail;
            }

            // Construct reply cursor and read individual results.
            {
               bson_t cursor_opts = BSON_INITIALIZER;
               {
                  uint32_t serverid = parts.assembled.server_stream->sd->id;
                  BSON_ASSERT(mlib_in_range(int32_t, serverid));
                  int32_t serverid_i32 = (int32_t)serverid;
                  BSON_ASSERT(BSON_APPEND_INT32(&cursor_opts, "serverId", serverid_i32));
                  // Use same session if one was applied.
                  if (parts.assembled.session &&
                      !mongoc_client_session_append(parts.assembled.session, &cursor_opts, &error)) {
                     _bulkwriteexception_set_error(ret.exc, &error);
                     _bulkwriteexception_set_error_reply(ret.exc, &cmd_reply);
                     goto batch_fail;
                  }
               }

               // Construct the reply cursor.
               reply_cursor = mongoc_cursor_new_from_command_reply_with_opts(self->client, &cmd_reply, &cursor_opts);
               bson_destroy(&cursor_opts);
               // `cmd_reply` is stolen. Clear it.
               bson_init(&cmd_reply);

               // Ensure constructing cursor did not error.
               {
                  const bson_t *error_document;
                  if (mongoc_cursor_error_document(reply_cursor, &error, &error_document)) {
                     _bulkwriteexception_set_error(ret.exc, &error);
                     if (error_document) {
                        _bulkwriteexception_set_error_reply(ret.exc, error_document);
                     }
                     goto batch_fail;
                  }
               }

               // Iterate over cursor results.
               const bson_t *result;
               while (mongoc_cursor_next(reply_cursor, &result)) {
                  if (!_bulkwritereturn_apply_result(
                         &ret, result, ops_doc_offset, &self->arrayof_modeldata, &self->ops)) {
                     goto batch_fail;
                  }
               }
               has_write_errors = !bson_empty(&ret.exc->write_errors);
               // Ensure iterating cursor did not error.
               {
                  const bson_t *error_document;
                  if (mongoc_cursor_error_document(reply_cursor, &error, &error_document)) {
                     _bulkwriteexception_set_error(ret.exc, &error);
                     if (error_document) {
                        _bulkwriteexception_set_error_reply(ret.exc, error_document);
                     }
                     goto batch_fail;
                  }
               }
            }
         }
      }

      ops_doc_offset += ops_doc_len;
      ops_byte_offset += ops_byte_len;
      batch_ok = true;
   batch_fail:
      mcd_nsinfo_destroy(nsinfo);
      mongoc_cursor_destroy(reply_cursor);
      bson_destroy(&cmd_reply);
      if (!batch_ok) {
         goto fail;
      }
      if (has_write_errors && is_ordered) {
         // Ordered writes must not continue to send batches once an error is
         // occurred. An individual write error is not a top-level error.
         break;
      }
   }

fail:
   if (ret.res->parsed_some_results) {
      if (is_ordered) {
         // Ordered writes stop on first error. If the error reported is for an index > 0, assume some writes suceeded.
         if (ret.res->errorscount == 0 || (ret.res->first_error_index.isset && ret.res->first_error_index.index > 0)) {
            has_successful_results = true;
         }
      } else {
         BSON_ASSERT(mlib_in_range(size_t, ret.res->errorscount));
         size_t errorscount_sz = (size_t)ret.res->errorscount;
         if (errorscount_sz < self->n_ops) {
            has_successful_results = true;
         }
      }
   }
   if (!(mongoc_optional_is_set(&self->is_acknowledged) && mongoc_optional_value(&self->is_acknowledged) &&
         has_successful_results)) {
      mongoc_bulkwriteresult_destroy(ret.res);
      ret.res = NULL;
   }
   if (parts.body) {
      // Only clean-up if initialized.
      mongoc_cmd_parts_cleanup(&parts);
   }
   bson_destroy(&cmd);
   if (ss) {
      self->serverid.value = ss->sd->id;
      self->serverid.is_set = true;

      if (ret.res) {
         ret.res->serverid = self->serverid.value;
      }
   }
   mongoc_server_stream_cleanup(ss);
   if (!ret.exc->has_any_error) {
      mongoc_bulkwriteexception_destroy(ret.exc);
      ret.exc = NULL;
   }
   return ret;
}

MONGOC_EXPORT(mongoc_bulkwrite_check_acknowledged_t)
mongoc_bulkwrite_check_acknowledged(mongoc_bulkwrite_t const *self, bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(error);

   mongoc_bulkwrite_check_acknowledged_t result = {.is_ok = mongoc_optional_is_set(&self->is_acknowledged),
                                                   .is_acknowledged = false};

   if (result.is_ok) {
      result.is_acknowledged = mongoc_optional_value(&self->is_acknowledged);
   } else {
      _mongoc_set_error(error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "bulk write has not been executed or execution failed");
   }

   return result;
}

mongoc_bulkwrite_serverid_t
mongoc_bulkwrite_serverid(mongoc_bulkwrite_t const *self, bson_error_t *error)
{
   BSON_ASSERT_PARAM(self);
   BSON_OPTIONAL_PARAM(error);

   mongoc_bulkwrite_serverid_t const result = {.is_ok = self->serverid.is_set, .serverid = self->serverid.value};

   if (!result.is_ok) {
      _mongoc_set_error(error,
                        MONGOC_ERROR_COMMAND,
                        MONGOC_ERROR_COMMAND_INVALID_ARG,
                        "bulk write has not been executed or execution failed");
   }

   return result;
}

MC_ENABLE_CONVERSION_WARNING_END
