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

#include <mongoc/mongoc-retry-backoff-generator-private.h>

#include <bson/macros.h>
#include <bson/memory.h>

#include <math.h>

struct _mongoc_retry_backoff_generator_t {
   int attempt;
   int max_attempt;
   mongoc_retry_backoff_params_t params;
   mongoc_jitter_source_t *jitter_source;
};

static int
_compute_max_attempt(mongoc_retry_backoff_params_t params)
{
   return (int)ceil(log((double)mlib_microseconds_count(params.backoff_max) /
                        (double)mlib_microseconds_count(params.backoff_initial)) /
                    log(params.growth_factor)) +
          1;
}

mongoc_retry_backoff_generator_t *
_mongoc_retry_backoff_generator_new(mongoc_retry_backoff_params_t params, mongoc_jitter_source_t *jitter_source)
{
   BSON_ASSERT_PARAM(jitter_source);

   mongoc_retry_backoff_generator_t *const generator =
      (mongoc_retry_backoff_generator_t *)bson_malloc(sizeof(mongoc_retry_backoff_generator_t));

   *generator = (mongoc_retry_backoff_generator_t){
      .attempt = 0,
      .max_attempt = _compute_max_attempt(params),
      .params = params,
      .jitter_source = jitter_source,
   };

   return generator;
}

void
_mongoc_retry_backoff_generator_destroy(mongoc_retry_backoff_generator_t *generator)
{
   bson_free(generator);
}

static mlib_duration
_duration_double_multiply(mlib_duration duration, double factor)
{
   return mlib_duration((mlib_duration_rep_t)round((double)mlib_microseconds_count(duration) * factor), us);
}

static void
_increment_attempt(mongoc_retry_backoff_generator_t *generator)
{
   generator->attempt = BSON_MIN(generator->attempt + 1, generator->max_attempt);
}

mlib_duration
_mongoc_retry_backoff_generator_next(mongoc_retry_backoff_generator_t *generator)
{
   BSON_ASSERT_PARAM(generator);

   _increment_attempt(generator);

   const double jitter = _mongoc_jitter_source_generate(generator->jitter_source);

   BSON_ASSERT(0.0 <= jitter && jitter <= 1.0);

   const mongoc_retry_backoff_params_t *const params = &generator->params;

   if (generator->attempt >= generator->max_attempt) {
      return _duration_double_multiply(params->backoff_max, jitter);
   }

   const double backoff_factor = pow(params->growth_factor, (double)generator->attempt - 1);

   return _duration_double_multiply(params->backoff_initial, jitter * backoff_factor);
}

void
_mongoc_retry_backoff_generator_skip(mongoc_retry_backoff_generator_t *generator)
{
   BSON_ASSERT_PARAM(generator);

   _increment_attempt(generator);
}
