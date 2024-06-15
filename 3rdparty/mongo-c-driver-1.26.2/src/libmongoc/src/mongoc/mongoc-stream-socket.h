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

#include "mongoc-prelude.h"

#ifndef MONGOC_STREAM_SOCKET_H
#define MONGOC_STREAM_SOCKET_H

#include "mongoc-macros.h"
#include "mongoc-socket.h"
#include "mongoc-stream.h"


BSON_BEGIN_DECLS


typedef struct _mongoc_stream_socket_t mongoc_stream_socket_t;


MONGOC_EXPORT (mongoc_stream_t *)
mongoc_stream_socket_new (mongoc_socket_t *socket) BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (mongoc_socket_t *)
mongoc_stream_socket_get_socket (mongoc_stream_socket_t *stream);


BSON_END_DECLS


#endif /* MONGOC_STREAM_SOCKET_H */
