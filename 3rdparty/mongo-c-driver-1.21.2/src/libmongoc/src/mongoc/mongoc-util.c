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

#ifdef _WIN32
#define _CRT_RAND_S
#endif

#include <string.h>

#include "bson/bson.h"

#include "common-md5-private.h"
#include "common-thread-private.h"
#include "mongoc-rand-private.h"
#include "mongoc-util-private.h"
#include "mongoc-client.h"
#include "mongoc-client-session-private.h"
#include "mongoc-trace-private.h"

const bson_validate_flags_t _mongoc_default_insert_vflags =
   BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL |
   BSON_VALIDATE_EMPTY_KEYS;

const bson_validate_flags_t _mongoc_default_replace_vflags =
   BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL |
   BSON_VALIDATE_EMPTY_KEYS;

const bson_validate_flags_t _mongoc_default_update_vflags =
   BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL |
   BSON_VALIDATE_EMPTY_KEYS;

int
_mongoc_rand_simple (unsigned int *seed)
{
#ifdef _WIN32
   /* ignore the seed */
   unsigned int ret = 0;
   errno_t err;

   err = rand_s (&ret);
   if (0 != err) {
      MONGOC_ERROR ("rand_s failed: %s", strerror (err));
   }

   return (int) ret;
#else
   return rand_r (seed);
#endif
}


char *
_mongoc_hex_md5 (const char *input)
{
   uint8_t digest[16];
   bson_md5_t md5;
   char digest_str[33];
   int i;

   COMMON_PREFIX (_bson_md5_init (&md5));
   COMMON_PREFIX (_bson_md5_append (
      &md5, (const uint8_t *) input, (uint32_t) strlen (input)));
   COMMON_PREFIX (_bson_md5_finish (&md5, digest));

   for (i = 0; i < sizeof digest; i++) {
      bson_snprintf (&digest_str[i * 2], 3, "%02x", digest[i]);
   }
   digest_str[sizeof digest_str - 1] = '\0';

   return bson_strdup (digest_str);
}


void
_mongoc_usleep (int64_t usec)
{
#ifdef _WIN32
   LARGE_INTEGER ft;
   HANDLE timer;

   BSON_ASSERT (usec >= 0);

   ft.QuadPart = -(10 * usec);
   timer = CreateWaitableTimer (NULL, true, NULL);
   SetWaitableTimer (timer, &ft, 0, NULL, NULL, 0);
   WaitForSingleObject (timer, INFINITE);
   CloseHandle (timer);
#else
   BSON_ASSERT (usec >= 0);
   usleep ((useconds_t) usec);
#endif
}

int64_t
_mongoc_get_real_time_ms (void)
{
   struct timeval tv;
   const bool rc = bson_gettimeofday (&tv);
   if (rc != 0) {
      return -1;
   }
   return tv.tv_sec * (int64_t) 1000 + tv.tv_usec / (int64_t) 1000;
}


const char *
_mongoc_get_command_name (const bson_t *command)
{
   bson_iter_t iter;
   const char *name;
   bson_iter_t child;
   const char *wrapper_name = NULL;

   BSON_ASSERT (command);

   if (!bson_iter_init (&iter, command) || !bson_iter_next (&iter)) {
      return NULL;
   }

   name = bson_iter_key (&iter);

   /* wrapped in "$query" or "query"?
    *
    *   {$query: {count: "collection"}, $readPreference: {...}}
    */
   if (name[0] == '$') {
      wrapper_name = "$query";
   } else if (!strcmp (name, "query")) {
      wrapper_name = "query";
   }

   if (wrapper_name && bson_iter_init_find (&iter, command, wrapper_name) &&
       BSON_ITER_HOLDS_DOCUMENT (&iter) && bson_iter_recurse (&iter, &child) &&
       bson_iter_next (&child)) {
      name = bson_iter_key (&child);
   }

   return name;
}


const char *
_mongoc_get_documents_field_name (const char *command_name)
{
   if (!strcmp (command_name, "insert")) {
      return "documents";
   }

   if (!strcmp (command_name, "update")) {
      return "updates";
   }

   if (!strcmp (command_name, "delete")) {
      return "deletes";
   }

   return NULL;
}

bool
_mongoc_lookup_bool (const bson_t *bson, const char *key, bool default_value)
{
   bson_iter_t iter;
   bson_iter_t child;

   if (!bson) {
      return default_value;
   }

   BSON_ASSERT (bson_iter_init (&iter, bson));
   if (!bson_iter_find_descendant (&iter, key, &child)) {
      return default_value;
   }

   return bson_iter_as_bool (&child);
}

char *
_mongoc_get_db_name (const char *ns)
{
   size_t dblen;
   const char *dot;

   BSON_ASSERT (ns);

   dot = strstr (ns, ".");

   if (dot) {
      dblen = dot - ns;
      return bson_strndup (ns, dblen);
   } else {
      return bson_strdup (ns);
   }
}

void
_mongoc_bson_init_if_set (bson_t *bson)
{
   if (bson) {
      bson_init (bson);
   }
}

const char *
_mongoc_bson_type_to_str (bson_type_t t)
{
   switch (t) {
   case BSON_TYPE_EOD:
      return "EOD";
   case BSON_TYPE_DOUBLE:
      return "DOUBLE";
   case BSON_TYPE_UTF8:
      return "UTF8";
   case BSON_TYPE_DOCUMENT:
      return "DOCUMENT";
   case BSON_TYPE_ARRAY:
      return "ARRAY";
   case BSON_TYPE_BINARY:
      return "BINARY";
   case BSON_TYPE_UNDEFINED:
      return "UNDEFINED";
   case BSON_TYPE_OID:
      return "OID";
   case BSON_TYPE_BOOL:
      return "BOOL";
   case BSON_TYPE_DATE_TIME:
      return "DATE_TIME";
   case BSON_TYPE_NULL:
      return "NULL";
   case BSON_TYPE_REGEX:
      return "REGEX";
   case BSON_TYPE_DBPOINTER:
      return "DBPOINTER";
   case BSON_TYPE_CODE:
      return "CODE";
   case BSON_TYPE_SYMBOL:
      return "SYMBOL";
   case BSON_TYPE_CODEWSCOPE:
      return "CODEWSCOPE";
   case BSON_TYPE_INT32:
      return "INT32";
   case BSON_TYPE_TIMESTAMP:
      return "TIMESTAMP";
   case BSON_TYPE_INT64:
      return "INT64";
   case BSON_TYPE_MAXKEY:
      return "MAXKEY";
   case BSON_TYPE_MINKEY:
      return "MINKEY";
   case BSON_TYPE_DECIMAL128:
      return "DECIMAL128";
   default:
      return "Unknown";
   }
}


/* Refer to:
 * https://github.com/mongodb/specifications/blob/master/source/wireversion-featurelist.rst
 * and:
 * https://github.com/mongodb/mongo/blob/master/src/mongo/db/wire_version.h#L57
 */
const char *
_mongoc_wire_version_to_server_version (int32_t version)
{
   switch (version) {
   case 1:
   case 2:
      return "2.6";
   case 3:
      return "3.0";
   case 4:
      return "3.2";
   case 5:
      return "3.4";
   case 6:
      return "3.6";
   case 7:
      return "4.0";
   case 8:
      return "4.2";
   case 9:
      return "4.4";
   case 10:
      return "4.7";
   case 11:
      return "4.8";
   case 12:
      return "4.9";
   case 13:
      return "5.0";
   case 14:
      return "5.1";
   case 15:
      return "5.2";
   default:
      return "Unknown";
   }
}


/* Get "serverId" from opts. Sets *server_id to the serverId from "opts" or 0
 * if absent. On error, fills out *error with domain and code and return false.
 */
bool
_mongoc_get_server_id_from_opts (const bson_t *opts,
                                 mongoc_error_domain_t domain,
                                 mongoc_error_code_t code,
                                 uint32_t *server_id,
                                 bson_error_t *error)
{
   bson_iter_t iter;

   ENTRY;

   BSON_ASSERT (server_id);

   *server_id = 0;

   if (!opts || !bson_iter_init_find (&iter, opts, "serverId")) {
      RETURN (true);
   }

   if (!BSON_ITER_HOLDS_INT (&iter)) {
      bson_set_error (
         error, domain, code, "The serverId option must be an integer");
      RETURN (false);
   }

   if (bson_iter_as_int64 (&iter) <= 0) {
      bson_set_error (error, domain, code, "The serverId option must be >= 1");
      RETURN (false);
   }

   *server_id = (uint32_t) bson_iter_as_int64 (&iter);

   RETURN (true);
}


bool
_mongoc_validate_new_document (const bson_t *doc,
                               bson_validate_flags_t vflags,
                               bson_error_t *error)
{
   bson_error_t validate_err;

   if (vflags == BSON_VALIDATE_NONE) {
      return true;
   }

   if (!bson_validate_with_error (doc, vflags, &validate_err)) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "invalid document for insert: %s",
                      validate_err.message);
      return false;
   }

   return true;
}


bool
_mongoc_validate_replace (const bson_t *doc,
                          bson_validate_flags_t vflags,
                          bson_error_t *error)
{
   bson_error_t validate_err;
   bson_iter_t iter;
   const char *key;

   if (vflags == BSON_VALIDATE_NONE) {
      return true;
   }

   if (!bson_validate_with_error (doc, vflags, &validate_err)) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "invalid argument for replace: %s",
                      validate_err.message);
      return false;
   }

   if (!bson_iter_init (&iter, doc)) {
      bson_set_error (error,
                      MONGOC_ERROR_BSON,
                      MONGOC_ERROR_BSON_INVALID,
                      "replace document is corrupt");
      return false;
   }

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);
      if (key[0] == '$') {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "Invalid key '%s': replace prohibits $ operators",
                         key);

         return false;
      }
   }

   return true;
}


bool
_mongoc_validate_update (const bson_t *update,
                         bson_validate_flags_t vflags,
                         bson_error_t *error)
{
   bson_error_t validate_err;
   bson_iter_t iter;
   const char *key;

   if (vflags == BSON_VALIDATE_NONE) {
      return true;
   }

   if (!bson_validate_with_error (update, vflags, &validate_err)) {
      bson_set_error (error,
                      MONGOC_ERROR_COMMAND,
                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                      "invalid argument for update: %s",
                      validate_err.message);
      return false;
   }

   if (_mongoc_document_is_pipeline (update)) {
      return true;
   }

   if (!bson_iter_init (&iter, update)) {
      bson_set_error (error,
                      MONGOC_ERROR_BSON,
                      MONGOC_ERROR_BSON_INVALID,
                      "update document is corrupt");
      return false;
   }

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);
      if (key[0] != '$') {
         bson_set_error (error,
                         MONGOC_ERROR_COMMAND,
                         MONGOC_ERROR_COMMAND_INVALID_ARG,
                         "Invalid key '%s': update only works with $ operators"
                         " and pipelines",
                         key);

         return false;
      }
   }

   return true;
}

void
mongoc_lowercase (const char *src, char *buf /* OUT */)
{
   for (; *src; ++src, ++buf) {
      /* UTF8 non-ascii characters have a 1 at the leftmost bit. If this is the
       * case, just copy */
      if ((*src & (0x1 << 7)) == 0) {
         *buf = (char) tolower (*src);
      } else {
         *buf = *src;
      }
   }
}

bool
mongoc_parse_port (uint16_t *port, const char *str)
{
   unsigned long ul_port;

   ul_port = strtoul (str, NULL, 10);

   if (ul_port == 0 || ul_port > UINT16_MAX) {
      /* Parse error or port number out of range. mongod prohibits port 0. */
      return false;
   }

   *port = (uint16_t) ul_port;
   return true;
}


/*--------------------------------------------------------------------------
 *
 * _mongoc_bson_array_add_label --
 *
 *       Append an error label like "TransientTransactionError" to a BSON
 *       array iff the array does not already contain it.
 *
 * Side effects:
 *       Aborts if the array is invalid or contains non-string elements.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_bson_array_add_label (bson_t *bson, const char *label)
{
   bson_iter_t iter;
   char buf[16];
   uint32_t i = 0;
   const char *key;

   BSON_ASSERT (bson_iter_init (&iter, bson));
   while (bson_iter_next (&iter)) {
      if (!strcmp (bson_iter_utf8 (&iter, NULL), label)) {
         /* already included once */
         return;
      }

      i++;
   }

   bson_uint32_to_string (i, &key, buf, sizeof buf);
   BSON_APPEND_UTF8 (bson, key, label);
}


/*--------------------------------------------------------------------------
 *
 * _mongoc_bson_array_copy_labels_to --
 *
 *       Copy error labels like "TransientTransactionError" from a server
 *       reply to a BSON array iff the array does not already contain it.
 *
 * Side effects:
 *       Aborts if @dst is invalid or contains non-string elements.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_bson_array_copy_labels_to (const bson_t *reply, bson_t *dst)
{
   bson_iter_t iter;
   bson_iter_t label;

   if (bson_iter_init_find (&iter, reply, "errorLabels")) {
      BSON_ASSERT (bson_iter_recurse (&iter, &label));
      while (bson_iter_next (&label)) {
         if (BSON_ITER_HOLDS_UTF8 (&label)) {
            _mongoc_bson_array_add_label (dst, bson_iter_utf8 (&label, NULL));
         }
      }
   }
}


/*--------------------------------------------------------------------------
 *
 * _mongoc_bson_init_with_transient_txn_error --
 *
 *       If @reply is not NULL, initialize it. If @cs is not NULL and in a
 *       transaction, add errorLabels: ["TransientTransactionError"] to @cs.
 *
 *       Transactions Spec: TransientTransactionError includes "server
 *       selection error encountered running any command besides
 *       commitTransaction in a transaction. ...in the case of network errors
 *       or server selection errors where the client receives no server reply,
 *       the client adds the label."
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_bson_init_with_transient_txn_error (const mongoc_client_session_t *cs,
                                            bson_t *reply)
{
   bson_t labels;

   if (!reply) {
      return;
   }

   bson_init (reply);

   if (_mongoc_client_session_in_txn (cs)) {
      BSON_APPEND_ARRAY_BEGIN (reply, "errorLabels", &labels);
      BSON_APPEND_UTF8 (&labels, "0", TRANSIENT_TXN_ERR);
      bson_append_array_end (reply, &labels);
   }
}

bool
_mongoc_document_is_pipeline (const bson_t *document)
{
   bson_iter_t iter;
   bson_iter_t child;
   const char *key;
   int i = 0;
   char *i_str;

   if (!bson_iter_init (&iter, document)) {
      return false;
   }

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);
      i_str = bson_strdup_printf ("%d", i++);

      if (strcmp (key, i_str)) {
         bson_free (i_str);
         return false;
      }

      bson_free (i_str);

      if (BSON_ITER_HOLDS_DOCUMENT (&iter)) {
         if (!bson_iter_recurse (&iter, &child)) {
            return false;
         }
         if (!bson_iter_next (&child)) {
            return false;
         }
         key = bson_iter_key (&child);
         if (key[0] != '$') {
            return false;
         }
      } else {
         return false;
      }
   }

   /* should return false when the document is empty */
   return i != 0;
}

char *
_mongoc_getenv (const char *name)
{
#ifdef _MSC_VER
   char buf[2048];
   size_t buflen;

   if ((0 == getenv_s (&buflen, buf, sizeof buf, name)) && buflen) {
      return bson_strdup (buf);
   } else {
      return NULL;
   }
#else

   if (getenv (name) && strlen (getenv (name))) {
      return bson_strdup (getenv (name));
   } else {
      return NULL;
   }

#endif
}

/* Nearly Divisionless (Algorithm 5): https://arxiv.org/abs/1805.10941 */
static uint32_t
_mongoc_rand_nduid32 (uint32_t s, uint32_t (*rand32) (void))
{
   const uint64_t limit = UINT32_MAX; /* 2^L */
   uint64_t x, m, l;

   x = rand32 ();
   m = x * s;
   l = m % limit;

   if (l < s) {
      const uint64_t t = (limit - s) % s;

      while (l < t) {
         x = rand32 ();
         m = x * s;
         l = m % limit;
      }
   }

   return (uint32_t) (m / limit);
}

/* Java Algorithm (Algorithm 4): https://arxiv.org/abs/1805.10941
 * The 64-bit version of the nearly divisionless algorithm requires 128-bit
 * integer arithmetic. Instead of trying to deal with cross-platform support for
 * `__int128`, fallback to using the Java algorithm for 64-bit instead. */
static uint64_t
_mongoc_rand_java64 (uint64_t s, uint64_t (*rand64) (void))
{
   const uint64_t limit = UINT64_MAX; /* 2^L */
   uint64_t x, r;

   x = rand64 ();
   r = x % s;

   while ((x - r) > (limit - s)) {
      x = rand64 ();
      r = x % s;
   }

   return r;
}

#if defined(MONGOC_ENABLE_CRYPTO)

uint32_t
_mongoc_crypto_rand_uint32_t (void)
{
   uint32_t res;

   (void) _mongoc_rand_bytes ((uint8_t *) &res, sizeof (res));

   return res;
}

uint64_t
_mongoc_crypto_rand_uint64_t (void)
{
   uint64_t res;

   (void) _mongoc_rand_bytes ((uint8_t *) &res, sizeof (res));

   return res;
}

size_t
_mongoc_crypto_rand_size_t (void)
{
   size_t res;

   (void) _mongoc_rand_bytes ((uint8_t *) &res, sizeof (res));

   return res;
}

#endif /* defined(MONGOC_ENABLE_CRYPTO) */

static BSON_ONCE_FUN (_mongoc_simple_rand_init)
{
   struct timeval tv;
   unsigned int seed = 0;

   bson_gettimeofday (&tv);

   seed ^= (unsigned int) tv.tv_sec;
   seed ^= (unsigned int) tv.tv_usec;

   srand (seed);

   BSON_ONCE_RETURN;
}

static bson_once_t _mongoc_simple_rand_init_once = BSON_ONCE_INIT;

uint32_t
_mongoc_simple_rand_uint32_t (void)
{
   bson_once (&_mongoc_simple_rand_init_once, _mongoc_simple_rand_init);

   /* Ensure *all* bits are random, as RAND_MAX is only required to be at least
    * 32767 (2^15). */
   return (((uint32_t) rand () & 0x7FFFu) << 0u) |
          (((uint32_t) rand () & 0x7FFFu) << 15u) |
          (((uint32_t) rand () & 0x0003u) << 30u);
}

uint64_t
_mongoc_simple_rand_uint64_t (void)
{
   bson_once (&_mongoc_simple_rand_init_once, _mongoc_simple_rand_init);

   /* Ensure *all* bits are random, as RAND_MAX is only required to be at least
    * 32767 (2^15). */
   return (((uint64_t) rand () & 0x7FFFu) << 0u) |
          (((uint64_t) rand () & 0x7FFFu) << 15u) |
          (((uint64_t) rand () & 0x7FFFu) << 30u) |
          (((uint64_t) rand () & 0x7FFFu) << 45u) |
          (((uint64_t) rand () & 0x0003u) << 60u);
}

uint32_t
_mongoc_rand_uint32_t (uint32_t min, uint32_t max, uint32_t (*rand) (void))
{
   BSON_ASSERT (min <= max);
   BSON_ASSERT (min != 0u || max != UINT32_MAX);

   return _mongoc_rand_nduid32 (max - min + 1u, rand) + min;
}

uint64_t
_mongoc_rand_uint64_t (uint64_t min, uint64_t max, uint64_t (*rand) (void))
{
   BSON_ASSERT (min <= max);
   BSON_ASSERT (min != 0u || max != UINT64_MAX);

   return _mongoc_rand_java64 (max - min + 1u, rand) + min;
}

#if SIZE_MAX == UINT64_MAX

BSON_STATIC_ASSERT2 (_mongoc_simple_rand_size_t,
                     sizeof (size_t) == sizeof (uint64_t));

size_t
_mongoc_simple_rand_size_t (void)
{
   return (size_t) _mongoc_simple_rand_uint64_t ();
}

size_t
_mongoc_rand_size_t (size_t min, size_t max, size_t (*rand) (void))
{
   BSON_ASSERT (min <= max);
   BSON_ASSERT (min != 0u || max != UINT64_MAX);

   return _mongoc_rand_java64 (max - min + 1u, (uint64_t (*) (void)) rand) +
          min;
}

#elif SIZE_MAX == UINT32_MAX

BSON_STATIC_ASSERT2 (_mongoc_simple_rand_size_t,
                     sizeof (size_t) == sizeof (uint32_t));

size_t
_mongoc_simple_rand_size_t (void)
{
   return (size_t) _mongoc_simple_rand_uint32_t ();
}

size_t
_mongoc_rand_size_t (size_t min, size_t max, size_t (*rand) (void))
{
   BSON_ASSERT (min <= max);
   BSON_ASSERT (min != 0u || max != UINT32_MAX);

   return _mongoc_rand_nduid32 (max - min + 1u, (uint32_t (*) (void)) rand) +
          min;
}

#else

#error \
   "Implementation of _mongoc_simple_rand_size_t() requires size_t be exactly 32-bit or 64-bit"

#endif
