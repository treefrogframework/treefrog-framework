/*
 * Copyright 2016 MongoDB, Inc.
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


#ifndef MONGOC_HANDSHAKE_PRIVATE_H
#define MONGOC_HANDSHAKE_PRIVATE_H

#include "mongoc.h"

BSON_BEGIN_DECLS

#define HANDSHAKE_FIELD "client"
#define HANDSHAKE_PLATFORM_FIELD "platform"

#define HANDSHAKE_MAX_SIZE 512

#define HANDSHAKE_OS_TYPE_MAX 32
#define HANDSHAKE_OS_NAME_MAX 32
#define HANDSHAKE_OS_VERSION_MAX 32
#define HANDSHAKE_OS_ARCHITECTURE_MAX 32
#define HANDSHAKE_DRIVER_NAME_MAX 64
#define HANDSHAKE_DRIVER_VERSION_MAX 32

#define HANDSHAKE_CMD_LEGACY_HELLO "isMaster"
#define HANDSHAKE_RESPONSE_LEGACY_HELLO "ismaster"
/* platform has no fixed max size. It can just occupy the remaining
 * available space in the document. */

/* When adding a new field to mongoc-config.h.in, update this! */
typedef enum {
   /* The bit position (from the RHS) of each config flag. Do not reorder. */
   MONGOC_MD_FLAG_ENABLE_CRYPTO = 0,
   MONGOC_MD_FLAG_ENABLE_CRYPTO_CNG,
   MONGOC_MD_FLAG_ENABLE_CRYPTO_COMMON_CRYPTO,
   MONGOC_MD_FLAG_ENABLE_CRYPTO_LIBCRYPTO,
   MONGOC_MD_FLAG_ENABLE_CRYPTO_SYSTEM_PROFILE,
   MONGOC_MD_FLAG_ENABLE_SASL,
   MONGOC_MD_FLAG_ENABLE_SSL,
   MONGOC_MD_FLAG_ENABLE_SSL_OPENSSL,
   MONGOC_MD_FLAG_ENABLE_SSL_SECURE_CHANNEL,
   MONGOC_MD_FLAG_ENABLE_SSL_SECURE_TRANSPORT,
   MONGOC_MD_FLAG_EXPERIMENTAL_FEATURES,
   MONGOC_MD_FLAG_HAVE_SASL_CLIENT_DONE,
   MONGOC_MD_FLAG_HAVE_WEAK_SYMBOLS,
   MONGOC_MD_FLAG_NO_AUTOMATIC_GLOBALS,
   MONGOC_MD_FLAG_ENABLE_SSL_LIBRESSL,
   MONGOC_MD_FLAG_ENABLE_SASL_CYRUS,
   MONGOC_MD_FLAG_ENABLE_SASL_SSPI,
   MONGOC_MD_FLAG_HAVE_SOCKLEN,
   MONGOC_MD_FLAG_ENABLE_COMPRESSION,
   MONGOC_MD_FLAG_ENABLE_COMPRESSION_SNAPPY,
   MONGOC_MD_FLAG_ENABLE_COMPRESSION_ZLIB,
   MONGOC_MD_FLAG_ENABLE_SASL_GSSAPI_UNUSED, /* CDRIVER-2654 removed this . */
   MONGOC_MD_FLAG_ENABLE_RES_NSEARCH,
   MONGOC_MD_FLAG_ENABLE_RES_NDESTROY,
   MONGOC_MD_FLAG_ENABLE_RES_NCLOSE,
   MONGOC_MD_FLAG_ENABLE_RES_SEARCH,
   MONGOC_MD_FLAG_ENABLE_DNSAPI,
   MONGOC_MD_FLAG_ENABLE_RDTSCP,
   MONGOC_MD_FLAG_HAVE_SCHED_GETCPU,
   MONGOC_MD_FLAG_ENABLE_SHM_COUNTERS,
   MONGOC_MD_FLAG_TRACE,
   MONGOC_MD_FLAG_ENABLE_ICU,
   MONGOC_MD_FLAG_ENABLE_CLIENT_SIDE_ENCRYPTION,
   MONGOC_MD_FLAG_ENABLE_MONGODB_AWS_AUTH,
   /* Add additional config flags here, above LAST_MONGOC_MD_FLAG. */
   LAST_MONGOC_MD_FLAG
} mongoc_handshake_config_flag_bit_t;


typedef struct _mongoc_handshake_t {
   char *os_type;
   char *os_name;
   char *os_version;
   char *os_architecture;

   char *driver_name;
   char *driver_version;
   char *platform;
   char *compiler_info;
   char *flags;

   bool frozen;
} mongoc_handshake_t;

void
_mongoc_handshake_init (void);

void
_mongoc_handshake_cleanup (void);

bool
_mongoc_handshake_build_doc_with_application (bson_t *doc,
                                              const char *application);

void
_mongoc_handshake_freeze (void);

mongoc_handshake_t *
_mongoc_handshake_get (void);

bool
_mongoc_handshake_appname_is_valid (const char *appname);

typedef struct {
   bool scram_sha_256;
   bool scram_sha_1;
} mongoc_handshake_sasl_supported_mechs_t;

void
_mongoc_handshake_append_sasl_supported_mechs (const mongoc_uri_t *uri,
                                               bson_t *hello);

void
_mongoc_handshake_parse_sasl_supported_mechs (
   const bson_t *hello,
   mongoc_handshake_sasl_supported_mechs_t *sasl_supported_mechs);

BSON_END_DECLS

#endif
