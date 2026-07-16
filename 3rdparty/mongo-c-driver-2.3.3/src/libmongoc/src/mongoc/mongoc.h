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


#ifndef MONGOC_H
#define MONGOC_H


#include <bson/bson.h>

#define MONGOC_INSIDE
#include <mongoc/mongoc-apm.h>
#include <mongoc/mongoc-bulk-operation.h>
#include <mongoc/mongoc-bulkwrite.h>
#include <mongoc/mongoc-change-stream.h>
#include <mongoc/mongoc-client-pool.h>
#include <mongoc/mongoc-client-session.h>
#include <mongoc/mongoc-client-side-encryption.h>
#include <mongoc/mongoc-client.h>
#include <mongoc/mongoc-collection.h>
#include <mongoc/mongoc-config.h>
#include <mongoc/mongoc-cursor.h>
#include <mongoc/mongoc-database.h>
#include <mongoc/mongoc-error.h>
#include <mongoc/mongoc-flags.h>
#include <mongoc/mongoc-gridfs-bucket.h>
#include <mongoc/mongoc-gridfs-file-list.h>
#include <mongoc/mongoc-gridfs-file-page.h>
#include <mongoc/mongoc-gridfs-file.h>
#include <mongoc/mongoc-gridfs.h>
#include <mongoc/mongoc-handshake.h>
#include <mongoc/mongoc-host-list.h>
#include <mongoc/mongoc-init.h>
#include <mongoc/mongoc-log.h>
#include <mongoc/mongoc-macros.h>
#include <mongoc/mongoc-opcode.h>
#include <mongoc/mongoc-sleep.h>
#include <mongoc/mongoc-socket.h>
#include <mongoc/mongoc-stream-buffered.h>
#include <mongoc/mongoc-stream-file.h>
#include <mongoc/mongoc-stream-gridfs.h>
#include <mongoc/mongoc-stream-socket.h>
#include <mongoc/mongoc-stream.h>
#include <mongoc/mongoc-structured-log.h>
#include <mongoc/mongoc-uri.h>
#include <mongoc/mongoc-version-functions.h>
#include <mongoc/mongoc-version.h>
#include <mongoc/mongoc-write-concern.h>
#ifdef MONGOC_ENABLE_SSL
#include <mongoc/mongoc-rand.h>
#include <mongoc/mongoc-ssl.h>
#include <mongoc/mongoc-stream-tls.h>
#endif
#undef MONGOC_INSIDE


#endif /* MONGOC_H */
