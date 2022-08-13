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

#include "mongoc-prelude.h"


#ifndef MONGOC_WRITE_COMMAND_LEGACY_PRIVATE_H
#define MONGOC_WRITE_COMMAND_LEGACY_PRIVATE_H

#include <bson/bson.h>
#include "mongoc-client-private.h"
#include "mongoc-write-command-private.h"

BSON_BEGIN_DECLS

void
_mongoc_write_command_insert_legacy (mongoc_write_command_t *command,
                                     mongoc_client_t *client,
                                     mongoc_server_stream_t *server_stream,
                                     const char *database,
                                     const char *collection,
                                     uint32_t offset,
                                     mongoc_write_result_t *result,
                                     bson_error_t *error);
void
_mongoc_write_command_update_legacy (mongoc_write_command_t *command,
                                     mongoc_client_t *client,
                                     mongoc_server_stream_t *server_stream,
                                     const char *database,
                                     const char *collection,
                                     uint32_t offset,
                                     mongoc_write_result_t *result,
                                     bson_error_t *error);
void
_mongoc_write_command_delete_legacy (mongoc_write_command_t *command,
                                     mongoc_client_t *client,
                                     mongoc_server_stream_t *server_stream,
                                     const char *database,
                                     const char *collection,
                                     uint32_t offset,
                                     mongoc_write_result_t *result,
                                     bson_error_t *error);
BSON_END_DECLS


#endif /* MONGOC_WRITE_COMMAND_LEGACY_PRIVATE_H */
