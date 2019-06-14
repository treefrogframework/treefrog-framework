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

#include "common-md5-private.h"
#include "mongoc/mongoc-util-private.h"
#include "mongoc/mongoc-client.h"
#include "mongoc/mongoc-client-session-private.h"
#include "mongoc/mongoc-trace-private.h"

const bson_validate_flags_t _mongoc_default_insert_vflags =
   BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL |
   BSON_VALIDATE_EMPTY_KEYS | BSON_VALIDATE_DOT_KEYS |
   BSON_VALIDATE_DOLLAR_KEYS;

const bson_validate_flags_t _mongoc_default_replace_vflags =
   BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL |
   BSON_VALIDATE_EMPTY_KEYS | BSON_VALIDATE_DOT_KEYS |
   BSON_VALIDATE_DOLLAR_KEYS;

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
      MONGOC_ERROR ("rand_s failed: %");
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

   _bson_md5_init (&md5);
   _bson_md5_append (&md5, (const uint8_t *) input, (uint32_t) strlen (input));
   _bson_md5_finish (&md5, digest);

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

void
_mongoc_get_db_name (const char *ns, char *db /* OUT */)
{
   size_t dblen;
   const char *dot;

   BSON_ASSERT (ns);

   dot = strstr (ns, ".");

   if (dot) {
      dblen = BSON_MIN (dot - ns + 1, MONGOC_NAMESPACE_MAX);
      bson_strncpy (db, ns, dblen);
   } else {
      bson_strncpy (db, ns, MONGOC_NAMESPACE_MAX);
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
                         "Invalid key '%s': update only works with $ operators",
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
