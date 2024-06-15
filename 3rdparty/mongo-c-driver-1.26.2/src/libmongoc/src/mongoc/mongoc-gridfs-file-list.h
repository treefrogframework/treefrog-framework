/*
 * Copyright 2013 MongoDB Inc.
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

#ifndef MONGOC_GRIDFS_FILE_LIST_H
#define MONGOC_GRIDFS_FILE_LIST_H

#include <bson/bson.h>

#include "mongoc-macros.h"
#include "mongoc-gridfs-file.h"


BSON_BEGIN_DECLS


typedef struct _mongoc_gridfs_file_list_t mongoc_gridfs_file_list_t;


MONGOC_EXPORT (mongoc_gridfs_file_t *)
mongoc_gridfs_file_list_next (mongoc_gridfs_file_list_t *list)
   BSON_GNUC_WARN_UNUSED_RESULT;
MONGOC_EXPORT (void)
mongoc_gridfs_file_list_destroy (mongoc_gridfs_file_list_t *list);
MONGOC_EXPORT (bool)
mongoc_gridfs_file_list_error (mongoc_gridfs_file_list_t *list,
                               bson_error_t *error);


BSON_END_DECLS


#endif /* MONGOC_GRIDFS_FILE_LIST_H */
