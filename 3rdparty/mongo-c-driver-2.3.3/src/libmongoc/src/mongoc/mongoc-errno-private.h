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

#ifndef MONGOC_ERRNO_PRIVATE_H
#define MONGOC_ERRNO_PRIVATE_H

#include <bson/bson.h>

#include <errno.h> // IWYU pragma: export

#ifdef _WIN32
#include <winerror.h>
#include <winsock2.h>
#endif


BSON_BEGIN_DECLS


#if defined(_WIN32)
#define MONGOC_ERRNO_IS_AGAIN(errno) ((errno == EAGAIN) || (errno == WSAEWOULDBLOCK) || (errno == WSAEINPROGRESS))
#define MONGOC_ERRNO_IS_TIMEDOUT(errno) (errno == WSAETIMEDOUT)
#else
#define MONGOC_ERRNO_IS_AGAIN(errno) \
   ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))
#define MONGOC_ERRNO_IS_TIMEDOUT(errno) (errno == ETIMEDOUT)
#endif


BSON_END_DECLS


#endif /* MONGOC_ERRNO_PRIVATE_H */
