/*
 * Copyright 2015 MongoDB, Inc.
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

#ifndef TEST_CONVENIENCES_H
#define TEST_CONVENIENCES_H

#include <bson/bson.h>

#include "mongoc/mongoc.h"
#include "mongoc/mongoc-read-prefs-private.h"
#include "mongoc/mongoc-client-private.h"

bson_t *
tmp_bson (const char *json, ...);

void
bson_iter_bson (const bson_iter_t *iter, bson_t *bson);

/* create a bson_t containing all types of values, and an empty key. The
 * returned bson_t does not need to be freed. This corresponds to the same
 * document in json_with_all_types. */
bson_t *
bson_with_all_types ();

/* returns a json string with all types of values, and an empty key. This
 * corresponds to the same document in bson_with_all_types. */
const char *
json_with_all_types ();


#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifdef _WIN32
#define realpath(path, expanded) \
   GetFullPathName (path, PATH_MAX, expanded, NULL)
#endif

const char *
bson_lookup_utf8 (const bson_t *b, const char *key);


void
bson_lookup_value (const bson_t *b, const char *key, bson_value_t *value);

void
bson_lookup_doc (const bson_t *b, const char *key, bson_t *doc);

void
bson_lookup_doc_null_ok (const bson_t *b, const char *key, bson_t *doc);

int32_t
bson_lookup_int32 (const bson_t *b, const char *key);

int64_t
bson_lookup_int64 (const bson_t *b, const char *key);

mongoc_read_concern_t *
bson_lookup_read_concern (const bson_t *b, const char *key);

mongoc_write_concern_t *
bson_lookup_write_concern (const bson_t *b, const char *key);

mongoc_read_prefs_t *
bson_lookup_read_prefs (const bson_t *b, const char *key);

void
bson_lookup_database_opts (const bson_t *b,
                           const char *key,
                           mongoc_database_t *database);

void
bson_lookup_collection_opts (const bson_t *b,
                             const char *key,
                             mongoc_collection_t *collection);

mongoc_transaction_opt_t *
bson_lookup_txn_opts (const bson_t *b, const char *key);

mongoc_session_opt_t *
bson_lookup_session_opts (const bson_t *b, const char *key);

mongoc_client_session_t *
bson_lookup_session (const bson_t *b, const char *key, mongoc_client_t *client);

bool
bson_init_from_value (bson_t *b, const bson_value_t *v);

char *
single_quotes_to_double (const char *str);

/* match_action_t determines if default check for a field is overridden. */
typedef enum {
   MATCH_ACTION_SKIP,    /* do not use the default check. */
   MATCH_ACTION_ABORT,   /* an error occurred, stop checking. */
   MATCH_ACTION_CONTINUE /* use the default check. */
} match_action_t;

struct _match_ctx_t;
/* doc_iter may be null if the pattern field is not found. */
typedef match_action_t (*match_visitor_fn) (struct _match_ctx_t *ctx,
                                            bson_iter_t *pattern_iter,
                                            bson_iter_t *doc_iter);

typedef struct _match_ctx_t {
   char errmsg[1000];
   bool strict_numeric_types;
   /* if retain_dots_in_keys is true, then don't consider a path with dots to
    * indicate recursing into a sub document. */
   bool retain_dots_in_keys;
   /* if allow_placeholders is true, treats 42 and "42" as placeholders. I.e.
    * comparing 42 to anything is ok. */
   bool allow_placeholders;
   /* path is the dot separated breadcrumb trail of keys. */
   char path[1000];
   /* if visitor_fn is not NULL, this is called on for every key in the pattern.
    * The returned match_action_t can override the default match behavior. */
   match_visitor_fn visitor_fn;
   void *visitor_ctx;
   /* if is_command is true, then compare the first key case insensitively. */
   bool is_command;
} match_ctx_t;

void
assert_match_bson (const bson_t *doc, const bson_t *pattern, bool is_command);

bool
match_bson (const bson_t *doc, const bson_t *pattern, bool is_command);

int64_t
bson_value_as_int64 (const bson_value_t *value);

bool
match_bson_value (const bson_value_t *doc,
                  const bson_value_t *pattern,
                  match_ctx_t *ctx);

bool
match_bson_with_ctx (const bson_t *doc,
                     const bson_t *pattern,
                     match_ctx_t *ctx);

bool
match_json (const bson_t *doc,
            bool is_command,
            const char *filename,
            int lineno,
            const char *funcname,
            const char *json_pattern,
            ...);

#define ASSERT_MATCH(doc, ...)                                                 \
   do {                                                                        \
      BSON_ASSERT (                                                            \
         match_json (doc, false, __FILE__, __LINE__, BSON_FUNC, __VA_ARGS__)); \
   } while (0)

bool
mongoc_write_concern_append_bad (mongoc_write_concern_t *write_concern,
                                 bson_t *command);

#define FOUR_MB 1024 * 1024 * 4

const char *
huge_string (mongoc_client_t *client);

size_t
huge_string_length (mongoc_client_t *client);

const char *
four_mb_string ();

void
assert_no_duplicate_keys (const bson_t *doc);

void
match_in_array (const bson_t *doc, const bson_t *array, match_ctx_t *ctx);

void
match_err (match_ctx_t *ctx, const char *fmt, ...);

#endif /* TEST_CONVENIENCES_H */
