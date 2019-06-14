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

#include "mongoc/mongoc-prelude.h"

#ifndef MONGOC_UTIL_PRIVATE_H
#define MONGOC_UTIL_PRIVATE_H

#include <bson/bson.h>
#include "mongoc/mongoc.h"

#ifdef BSON_HAVE_STRINGS_H
#include <strings.h>
#endif

/* string comparison functions for Windows */
#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

#if BSON_GNUC_CHECK_VERSION(4, 6)
#define BEGIN_IGNORE_DEPRECATIONS  \
   _Pragma ("GCC diagnostic push") \
      _Pragma ("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#define END_IGNORE_DEPRECATIONS _Pragma ("GCC diagnostic pop")
#elif defined(__clang__)
#define BEGIN_IGNORE_DEPRECATIONS    \
   _Pragma ("clang diagnostic push") \
      _Pragma ("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define END_IGNORE_DEPRECATIONS _Pragma ("clang diagnostic pop")
#else
#define BEGIN_IGNORE_DEPRECATIONS
#define END_IGNORE_DEPRECATIONS
#endif

#ifndef _WIN32
#define MONGOC_PRINTF_FORMAT(a, b) __attribute__ ((format (__printf__, a, b)))
#else
#define MONGOC_PRINTF_FORMAT(a, b) /* no-op */
#endif

#define COALESCE(x, y) ((x == 0) ? (y) : (x))


/* Helper macros for stringifying things */
#define MONGOC_STR(s) #s
#define MONGOC_EVALUATE_STR(s) MONGOC_STR (s)

BSON_BEGIN_DECLS

extern const bson_validate_flags_t _mongoc_default_insert_vflags;
extern const bson_validate_flags_t _mongoc_default_replace_vflags;
extern const bson_validate_flags_t _mongoc_default_update_vflags;

int
_mongoc_rand_simple (unsigned int *seed);

char *
_mongoc_hex_md5 (const char *input);

void
_mongoc_usleep (int64_t usec);

const char *
_mongoc_get_command_name (const bson_t *command);

const char *
_mongoc_get_documents_field_name (const char *command_name);

bool
_mongoc_lookup_bool (const bson_t *bson, const char *key, bool default_value);

void
_mongoc_get_db_name (const char *ns, char *db /* OUT */);

void
_mongoc_bson_init_if_set (bson_t *bson);

const char *
_mongoc_bson_type_to_str (bson_type_t t);

bool
_mongoc_get_server_id_from_opts (const bson_t *opts,
                                 mongoc_error_domain_t domain,
                                 mongoc_error_code_t code,
                                 uint32_t *server_id,
                                 bson_error_t *error);

bool
_mongoc_validate_new_document (const bson_t *insert,
                               bson_validate_flags_t vflags,
                               bson_error_t *error);

bool
_mongoc_validate_replace (const bson_t *insert,
                          bson_validate_flags_t vflags,
                          bson_error_t *error);

bool
_mongoc_validate_update (const bson_t *update,
                         bson_validate_flags_t vflags,
                         bson_error_t *error);

void
mongoc_lowercase (const char *src, char *buf /* OUT */);

bool
mongoc_parse_port (uint16_t *port, const char *str);

void
_mongoc_bson_array_add_label (bson_t *bson, const char *label);

void
_mongoc_bson_array_copy_labels_to (const bson_t *reply, bson_t *dst);

void
_mongoc_bson_init_with_transient_txn_error (const mongoc_client_session_t *cs,
                                            bson_t *reply);

BSON_END_DECLS

#endif /* MONGOC_UTIL_PRIVATE_H */
