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

#ifndef MONGOC_STREAM_PRIVATE_H
#define MONGOC_STREAM_PRIVATE_H

#include <mongoc/mongoc-stream.h> // IWYU pragma: export

//

#include <mongoc/mongoc-iovec.h>

#include <mlib/timer.h>


BSON_BEGIN_DECLS


#define MONGOC_STREAM_SOCKET 1
#define MONGOC_STREAM_FILE 2
#define MONGOC_STREAM_BUFFERED 3
#define MONGOC_STREAM_GRIDFS 4
#define MONGOC_STREAM_TLS 5
#define MONGOC_STREAM_GRIDFS_UPLOAD 6
#define MONGOC_STREAM_GRIDFS_DOWNLOAD 7

bool
mongoc_stream_wait(mongoc_stream_t *stream, int64_t expire_at);

bool
_mongoc_stream_writev_full(
   mongoc_stream_t *stream, mongoc_iovec_t *iov, size_t iovcnt, int64_t timeout_msec, bson_error_t *error);

mongoc_stream_t *
mongoc_stream_get_root_stream(mongoc_stream_t *stream);

/**
 * @brief Poll the given set of streams
 *
 * @param streams Pointer to an array of stream polling parameters
 * @param nstreams The number of streams in the array
 * @param until A timer that will wake up `poll()` from blocking
 */
ssize_t
_mongoc_stream_poll_internal(mongoc_stream_poll_t *streams, size_t nstreams, mlib_timer until);

BSON_END_DECLS


#endif /* MONGOC_STREAM_PRIVATE_H */
