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


#ifndef MONGOC_ASYNC_PRIVATE_H
#define MONGOC_ASYNC_PRIVATE_H

#include <mongoc/mongoc-prelude.h>

#include <mongoc/mongoc-stream.h>

#include <bson/bson.h>

#include <mlib/duration.h>
#include <mlib/timer.h>

BSON_BEGIN_DECLS

struct _mongoc_async_cmd;

typedef struct _mongoc_async {
   struct _mongoc_async_cmd *cmds;
   size_t ncmds;
   uint32_t request_id;
} mongoc_async_t;

mongoc_async_t *
mongoc_async_new(void);

void
mongoc_async_destroy(mongoc_async_t *async);

void
mongoc_async_run(mongoc_async_t *async);

BSON_END_DECLS

#endif /* MONGOC_ASYNC_PRIVATE_H */
