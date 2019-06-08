/*
 * Copyright 2018-present MongoDB, Inc.
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

#include <bson/bson.h>

#include "mongoc/mongoc-error.h"

bool
mongoc_error_has_label (const bson_t *reply, const char *label)
{
   bson_iter_t iter;
   bson_iter_t error_labels;

   BSON_ASSERT (reply);
   BSON_ASSERT (label);

   if (bson_iter_init_find (&iter, reply, "errorLabels") &&
       bson_iter_recurse (&iter, &error_labels)) {
      while (bson_iter_next (&error_labels)) {
         if (BSON_ITER_HOLDS_UTF8 (&error_labels) &&
             !strcmp (bson_iter_utf8 (&error_labels, NULL), label)) {
            return true;
         }
      }
   }

   return false;
}
