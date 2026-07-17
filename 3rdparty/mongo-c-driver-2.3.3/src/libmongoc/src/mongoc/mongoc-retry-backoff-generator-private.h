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

#ifndef MONGOC_RETRY_BACKOFF_GENERATOR_PRIVATE_H
#define MONGOC_RETRY_BACKOFF_GENERATOR_PRIVATE_H

#include <mongoc/mongoc-jitter-source-private.h>

#include <mlib/duration.h>

typedef struct {
   double growth_factor;
   mlib_duration backoff_initial;
   mlib_duration backoff_max;
} mongoc_retry_backoff_params_t;

typedef struct _mongoc_retry_backoff_generator_t mongoc_retry_backoff_generator_t;

mongoc_retry_backoff_generator_t *
_mongoc_retry_backoff_generator_new(mongoc_retry_backoff_params_t params, mongoc_jitter_source_t *jitter_source);

void
_mongoc_retry_backoff_generator_destroy(mongoc_retry_backoff_generator_t *generator);

mlib_duration
_mongoc_retry_backoff_generator_next(mongoc_retry_backoff_generator_t *generator);

void
_mongoc_retry_backoff_generator_skip(mongoc_retry_backoff_generator_t *generator);

#endif
