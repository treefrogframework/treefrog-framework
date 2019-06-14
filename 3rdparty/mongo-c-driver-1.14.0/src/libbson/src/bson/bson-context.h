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

#include "bson/bson-prelude.h"


#ifndef BSON_CONTEXT_H
#define BSON_CONTEXT_H


#include "bson/bson-macros.h"
#include "bson/bson-types.h"


BSON_BEGIN_DECLS


BSON_EXPORT (bson_context_t *)
bson_context_new (bson_context_flags_t flags);
BSON_EXPORT (void)
bson_context_destroy (bson_context_t *context);
BSON_EXPORT (bson_context_t *)
bson_context_get_default (void);


BSON_END_DECLS


#endif /* BSON_CONTEXT_H */
