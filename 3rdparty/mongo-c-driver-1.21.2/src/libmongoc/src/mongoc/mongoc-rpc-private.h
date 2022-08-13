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

#include "mongoc-prelude.h"

#ifndef MONGOC_RPC_PRIVATE_H
#define MONGOC_RPC_PRIVATE_H

#include <bson/bson.h>
#include <stddef.h>

#include "mongoc-array-private.h"
#include "mongoc-cmd-private.h"
#include "mongoc-iovec.h"
#include "mongoc-write-concern.h"
#include "mongoc-flags.h"
/* forward declaration */
struct _mongoc_cluster_t;

BSON_BEGIN_DECLS

typedef struct _mongoc_rpc_section_t {
   uint8_t payload_type;
   union {
      /* payload_type == 0 */
      const uint8_t *bson_document;
      /* payload_type == 1 */
      struct {
         int32_t size;
         uint32_t size_le;
         const char *identifier;
         const uint8_t *bson_documents;
      } sequence;
   } payload;
} mongoc_rpc_section_t;

#define RPC(_name, _code) \
   typedef struct {       \
      _code               \
   } mongoc_rpc_##_name##_t;
#define ENUM_FIELD(_name) uint32_t _name;
#define INT32_FIELD(_name) int32_t _name;
#define UINT8_FIELD(_name) uint8_t _name;
#define INT64_FIELD(_name) int64_t _name;
#define INT64_ARRAY_FIELD(_len, _name) \
   int32_t _len;                       \
   int64_t *_name;
#define CSTRING_FIELD(_name) const char *_name;
#define BSON_FIELD(_name) const uint8_t *_name;
#define BSON_ARRAY_FIELD(_name) \
   const uint8_t *_name;        \
   int32_t _name##_len;
#define IOVEC_ARRAY_FIELD(_name) \
   const mongoc_iovec_t *_name;  \
   int32_t n_##_name;            \
   mongoc_iovec_t _name##_recv;
#define SECTION_ARRAY_FIELD(_name) \
   mongoc_rpc_section_t _name[2];  \
   int32_t n_##_name;
#define RAW_BUFFER_FIELD(_name) \
   const uint8_t *_name;        \
   int32_t _name##_len;
#define BSON_OPTIONAL(_check, _code) _code


#pragma pack(1)
#include "op-delete.def"
#include "op-get-more.def"
#include "op-header.def"
#include "op-insert.def"
#include "op-kill-cursors.def"
#include "op-query.def"
#include "op-reply.def"
#include "op-reply-header.def"
#include "op-update.def"
#include "op-compressed.def"
/* restore default packing */
#pragma pack()

#include "op-msg.def"

typedef union {
   mongoc_rpc_delete_t delete_;
   mongoc_rpc_get_more_t get_more;
   mongoc_rpc_header_t header;
   mongoc_rpc_insert_t insert;
   mongoc_rpc_kill_cursors_t kill_cursors;
   mongoc_rpc_msg_t msg;
   mongoc_rpc_query_t query;
   mongoc_rpc_reply_t reply;
   mongoc_rpc_reply_header_t reply_header;
   mongoc_rpc_update_t update;
   mongoc_rpc_compressed_t compressed;
} mongoc_rpc_t;


BSON_STATIC_ASSERT2 (sizeof_rpc_header, sizeof (mongoc_rpc_header_t) == 16);
BSON_STATIC_ASSERT2 (offsetof_rpc_header,
                     offsetof (mongoc_rpc_header_t, opcode) ==
                        offsetof (mongoc_rpc_reply_t, opcode));
BSON_STATIC_ASSERT2 (sizeof_reply_header,
                     sizeof (mongoc_rpc_reply_header_t) == 36);


#undef RPC
#undef ENUM_FIELD
#undef UINT8_FIELD
#undef INT32_FIELD
#undef INT64_FIELD
#undef INT64_ARRAY_FIELD
#undef CSTRING_FIELD
#undef BSON_FIELD
#undef BSON_ARRAY_FIELD
#undef IOVEC_ARRAY_FIELD
#undef SECTION_ARRAY_FIELD
#undef BSON_OPTIONAL
#undef RAW_BUFFER_FIELD


void
_mongoc_rpc_gather (mongoc_rpc_t *rpc, mongoc_array_t *array);
void
_mongoc_rpc_swab_to_le (mongoc_rpc_t *rpc);
void
_mongoc_rpc_swab_from_le (mongoc_rpc_t *rpc);
void
_mongoc_rpc_printf (mongoc_rpc_t *rpc);
bool
_mongoc_rpc_scatter (mongoc_rpc_t *rpc, const uint8_t *buf, size_t buflen);
bool
_mongoc_rpc_scatter_reply_header_only (mongoc_rpc_t *rpc,
                                       const uint8_t *buf,
                                       size_t buflen);
bool
_mongoc_rpc_get_first_document (mongoc_rpc_t *rpc, bson_t *reply);
bool
_mongoc_rpc_reply_get_first (mongoc_rpc_reply_t *reply, bson_t *bson);
void
_mongoc_rpc_prep_command (mongoc_rpc_t *rpc,
                          const char *cmd_ns,
                          mongoc_cmd_t *cmd);
bool
_mongoc_rpc_check_ok (mongoc_rpc_t *rpc,
                      int32_t error_api_version,
                      bson_error_t *error /* OUT */,
                      bson_t *error_doc /* OUT */);
bool
_mongoc_cmd_check_ok (const bson_t *doc,
                      int32_t error_api_version,
                      bson_error_t *error);

bool
_mongoc_cmd_check_ok_no_wce (const bson_t *doc,
                             int32_t error_api_version,
                             bson_error_t *error);

bool
_mongoc_rpc_decompress (mongoc_rpc_t *rpc_le, uint8_t *buf, size_t buflen);

char *
_mongoc_rpc_compress (struct _mongoc_cluster_t *cluster,
                      int32_t compressor_id,
                      mongoc_rpc_t *rpc_le,
                      bson_error_t *error);

bool
_mongoc_rpc_decompress_if_necessary (mongoc_rpc_t *rpc,
                                     mongoc_buffer_t *buffer,
                                     bson_error_t *error);

BSON_END_DECLS


#endif /* MONGOC_RPC_PRIVATE_H */
