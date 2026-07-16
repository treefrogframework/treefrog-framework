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

#ifndef MONGOC_JITTER_SOURCE_PRIVATE_H
#define MONGOC_JITTER_SOURCE_PRIVATE_H

#include <mlib/duration.h>

typedef struct _mongoc_jitter_source_t mongoc_jitter_source_t;

// A function that returns nearly-uniformly-distributed values in the range `[0.0, 1.0]`.
typedef double (*mongoc_jitter_source_generate_fn_t)(mongoc_jitter_source_t *);

mongoc_jitter_source_t *
_mongoc_jitter_source_new(mongoc_jitter_source_generate_fn_t generate);

void
_mongoc_jitter_source_destroy(mongoc_jitter_source_t *source);

double
_mongoc_jitter_source_generate(mongoc_jitter_source_t *source);

double
_mongoc_jitter_source_generate_default(mongoc_jitter_source_t *source);

void
_mongoc_jitter_source_set_context(mongoc_jitter_source_t *source, void *ctx);

void *
_mongoc_jitter_source_get_context(mongoc_jitter_source_t *source);

#endif
