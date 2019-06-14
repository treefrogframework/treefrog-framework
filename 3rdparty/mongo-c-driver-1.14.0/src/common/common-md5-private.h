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

#include "common-prelude.h"

#ifndef COMMON_MD5_PRIVATE_H
#define COMMON_MD5_PRIVATE_H

#include "bson/bson.h"

BSON_BEGIN_DECLS

void
_bson_md5_init (bson_md5_t *pms);
void
_bson_md5_append (bson_md5_t *pms, const uint8_t *data, uint32_t nbytes);
void
_bson_md5_finish (bson_md5_t *pms, uint8_t digest[16]);

BSON_END_DECLS

#endif /* COMMON_MD5_PRIVATE_H */
