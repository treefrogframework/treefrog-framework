/*
 * Copyright 2019-present MongoDB, Inc.
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

#ifndef _WIN32
#include <sys/wait.h>
#include <signal.h>
#endif

#include "mongoc.h"
#include "mongoc-client-private.h"
#include "mongoc-client-side-encryption-private.h"
#include "mongoc-host-list-private.h"
#include "mongoc-stream-private.h"
#include "mongoc-topology-private.h"
#include "mongoc-trace-private.h"
#include "mongoc-util-private.h"

/*--------------------------------------------------------------------------
 * Auto Encryption options.
 *--------------------------------------------------------------------------
 */
struct _mongoc_auto_encryption_opts_t {
   /* keyvault_client and keyvault_client_pool are not owned and must outlive
    * auto encrypted client/pool. */
   mongoc_client_t *keyvault_client;
   mongoc_client_pool_t *keyvault_client_pool;
   char *keyvault_db;
   char *keyvault_coll;
   bson_t *kms_providers;
   bson_t *tls_opts;
   bson_t *schema_map;
   bool bypass_auto_encryption;
   bson_t *extra;
};

mongoc_auto_encryption_opts_t *
mongoc_auto_encryption_opts_new (void)
{
   return bson_malloc0 (sizeof (mongoc_auto_encryption_opts_t));
}

void
mongoc_auto_encryption_opts_destroy (mongoc_auto_encryption_opts_t *opts)
{
   if (!opts) {
      return;
   }
   bson_destroy (opts->extra);
   bson_destroy (opts->kms_providers);
   bson_destroy (opts->schema_map);
   bson_free (opts->keyvault_db);
   bson_free (opts->keyvault_coll);
   bson_destroy (opts->tls_opts);
   bson_free (opts);
}

void
mongoc_auto_encryption_opts_set_keyvault_client (
   mongoc_auto_encryption_opts_t *opts, mongoc_client_t *client)
{
   if (!opts) {
      return;
   }
   /* Does not own. */
   opts->keyvault_client = client;
}

void
mongoc_auto_encryption_opts_set_keyvault_client_pool (
   mongoc_auto_encryption_opts_t *opts, mongoc_client_pool_t *pool)
{
   if (!opts) {
      return;
   }
   /* Does not own. */
   opts->keyvault_client_pool = pool;
}

void
mongoc_auto_encryption_opts_set_keyvault_namespace (
   mongoc_auto_encryption_opts_t *opts, const char *db, const char *coll)
{
   if (!opts) {
      return;
   }
   bson_free (opts->keyvault_db);
   opts->keyvault_db = NULL;
   opts->keyvault_db = bson_strdup (db);
   bson_free (opts->keyvault_coll);
   opts->keyvault_coll = NULL;
   opts->keyvault_coll = bson_strdup (coll);
}

void
mongoc_auto_encryption_opts_set_kms_providers (
   mongoc_auto_encryption_opts_t *opts, const bson_t *providers)
{
   if (!opts) {
      return;
   }

   bson_destroy (opts->kms_providers);
   opts->kms_providers = NULL;
   if (providers) {
      opts->kms_providers = bson_copy (providers);
   }
}

/* _bson_copy_or_null returns a copy of @bson or NULL if @bson is NULL */
static bson_t *
_bson_copy_or_null (const bson_t *bson)
{
   if (bson) {
      return bson_copy (bson);
   }
   return NULL;
}

void
mongoc_auto_encryption_opts_set_tls_opts (mongoc_auto_encryption_opts_t *opts,
                                          const bson_t *tls_opts)
{
   if (!opts) {
      return;
   }
   bson_destroy (opts->tls_opts);
   opts->tls_opts = _bson_copy_or_null (tls_opts);
}

void
mongoc_auto_encryption_opts_set_schema_map (mongoc_auto_encryption_opts_t *opts,
                                            const bson_t *schema_map)
{
   if (!opts) {
      return;
   }

   bson_destroy (opts->schema_map);
   opts->schema_map = NULL;
   if (schema_map) {
      opts->schema_map = bson_copy (schema_map);
   }
}

void
mongoc_auto_encryption_opts_set_bypass_auto_encryption (
   mongoc_auto_encryption_opts_t *opts, bool bypass_auto_encryption)
{
   if (!opts) {
      return;
   }
   opts->bypass_auto_encryption = bypass_auto_encryption;
}

void
mongoc_auto_encryption_opts_set_extra (mongoc_auto_encryption_opts_t *opts,
                                       const bson_t *extra)
{
   if (!opts) {
      return;
   }
   bson_destroy (opts->extra);
   opts->extra = NULL;
   if (extra) {
      opts->extra = bson_copy (extra);
   }
}

/*--------------------------------------------------------------------------
 * Client Encryption options.
 *--------------------------------------------------------------------------
 */
struct _mongoc_client_encryption_opts_t {
   mongoc_client_t *keyvault_client;
   char *keyvault_db;
   char *keyvault_coll;
   bson_t *kms_providers;
   bson_t *tls_opts;
};

mongoc_client_encryption_opts_t *
mongoc_client_encryption_opts_new (void)
{
   return bson_malloc0 (sizeof (mongoc_client_encryption_opts_t));
}

void
mongoc_client_encryption_opts_destroy (mongoc_client_encryption_opts_t *opts)
{
   if (!opts) {
      return;
   }
   bson_free (opts->keyvault_db);
   bson_free (opts->keyvault_coll);
   bson_destroy (opts->kms_providers);
   bson_destroy (opts->tls_opts);
   bson_free (opts);
}

void
mongoc_client_encryption_opts_set_keyvault_client (
   mongoc_client_encryption_opts_t *opts, mongoc_client_t *keyvault_client)
{
   if (!opts) {
      return;
   }
   opts->keyvault_client = keyvault_client;
}

void
mongoc_client_encryption_opts_set_keyvault_namespace (
   mongoc_client_encryption_opts_t *opts, const char *db, const char *coll)
{
   if (!opts) {
      return;
   }
   bson_free (opts->keyvault_db);
   opts->keyvault_db = NULL;
   opts->keyvault_db = bson_strdup (db);
   bson_free (opts->keyvault_coll);
   opts->keyvault_coll = NULL;
   opts->keyvault_coll = bson_strdup (coll);
}

void
mongoc_client_encryption_opts_set_kms_providers (
   mongoc_client_encryption_opts_t *opts, const bson_t *kms_providers)
{
   if (!opts) {
      return;
   }
   bson_destroy (opts->kms_providers);
   opts->kms_providers = NULL;
   if (kms_providers) {
      opts->kms_providers = bson_copy (kms_providers);
   }
}

void
mongoc_client_encryption_opts_set_tls_opts (
   mongoc_client_encryption_opts_t *opts, const bson_t *tls_opts)
{
   if (!opts) {
      return;
   }
   bson_destroy (opts->tls_opts);
   opts->tls_opts = _bson_copy_or_null (tls_opts);
}

/*--------------------------------------------------------------------------
 * Data key options.
 *--------------------------------------------------------------------------
 */
struct _mongoc_client_encryption_datakey_opts_t {
   bson_t *masterkey;
   char **keyaltnames;
   uint32_t keyaltnames_count;
};

mongoc_client_encryption_datakey_opts_t *
mongoc_client_encryption_datakey_opts_new (void)
{
   return bson_malloc0 (sizeof (mongoc_client_encryption_datakey_opts_t));
}

static void
_clear_datakey_keyaltnames (mongoc_client_encryption_datakey_opts_t *opts)
{
   if (opts->keyaltnames) {
      int i;

      for (i = 0; i < opts->keyaltnames_count; i++) {
         bson_free (opts->keyaltnames[i]);
      }
      bson_free (opts->keyaltnames);
      opts->keyaltnames = NULL;
      opts->keyaltnames_count = 0;
   }
}

void
mongoc_client_encryption_datakey_opts_destroy (
   mongoc_client_encryption_datakey_opts_t *opts)
{
   if (!opts) {
      return;
   }

   bson_destroy (opts->masterkey);
   _clear_datakey_keyaltnames (opts);

   bson_free (opts);
}

void
mongoc_client_encryption_datakey_opts_set_masterkey (
   mongoc_client_encryption_datakey_opts_t *opts, const bson_t *masterkey)
{
   if (!opts) {
      return;
   }
   bson_destroy (opts->masterkey);
   opts->masterkey = NULL;
   if (masterkey) {
      opts->masterkey = bson_copy (masterkey);
   }
}

void
mongoc_client_encryption_datakey_opts_set_keyaltnames (
   mongoc_client_encryption_datakey_opts_t *opts,
   char **keyaltnames,
   uint32_t keyaltnames_count)
{
   int i;

   if (!opts) {
      return;
   }

   /* Free all first (if any have been set before). */
   _clear_datakey_keyaltnames (opts);
   BSON_ASSERT (!opts->keyaltnames);

   if (keyaltnames_count) {
      opts->keyaltnames = bson_malloc (sizeof (char *) * keyaltnames_count);
      for (i = 0; i < keyaltnames_count; i++) {
         opts->keyaltnames[i] = bson_strdup (keyaltnames[i]);
      }
      opts->keyaltnames_count = keyaltnames_count;
   }
}

/*--------------------------------------------------------------------------
 * Explicit Encryption options.
 *--------------------------------------------------------------------------
 */
struct _mongoc_client_encryption_encrypt_opts_t {
   bson_value_t keyid;
   char *algorithm;
   char *keyaltname;
};

mongoc_client_encryption_encrypt_opts_t *
mongoc_client_encryption_encrypt_opts_new (void)
{
   return bson_malloc0 (sizeof (mongoc_client_encryption_encrypt_opts_t));
}

void
mongoc_client_encryption_encrypt_opts_destroy (
   mongoc_client_encryption_encrypt_opts_t *opts)
{
   if (!opts) {
      return;
   }
   bson_value_destroy (&opts->keyid);
   bson_free (opts->algorithm);
   bson_free (opts->keyaltname);
   bson_free (opts);
}

void
mongoc_client_encryption_encrypt_opts_set_keyid (
   mongoc_client_encryption_encrypt_opts_t *opts, const bson_value_t *keyid)
{
   if (!opts) {
      return;
   }
   bson_value_destroy (&opts->keyid);
   memset (&opts->keyid, 0, sizeof (opts->keyid));
   if (keyid) {
      bson_value_copy (keyid, &opts->keyid);
   }
}

void
mongoc_client_encryption_encrypt_opts_set_keyaltname (
   mongoc_client_encryption_encrypt_opts_t *opts, const char *keyaltname)
{
   if (!opts) {
      return;
   }
   bson_free (opts->keyaltname);
   opts->keyaltname = NULL;
   opts->keyaltname = bson_strdup (keyaltname);
}

void
mongoc_client_encryption_encrypt_opts_set_algorithm (
   mongoc_client_encryption_encrypt_opts_t *opts, const char *algorithm)
{
   if (!opts) {
      return;
   }
   bson_free (opts->algorithm);
   opts->algorithm = NULL;
   opts->algorithm = bson_strdup (algorithm);
}

#ifndef MONGOC_ENABLE_CLIENT_SIDE_ENCRYPTION

static bool
_disabled_error (bson_error_t *error)
{
   bson_set_error (error,
                   MONGOC_ERROR_CLIENT,
                   MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                   "libmongoc is not built with support for Client-Side Field "
                   "Level Encryption. Configure with "
                   "ENABLE_CLIENT_SIDE_ENCRYPTION=ON.");
   return false;
}

bool
_mongoc_cse_auto_encrypt (mongoc_client_t *client,
                          const mongoc_cmd_t *cmd,
                          mongoc_cmd_t *encrypted_cmd,
                          bson_t *encrypted,
                          bson_error_t *error)
{
   bson_init (encrypted);

   return _disabled_error (error);
}

bool
_mongoc_cse_auto_decrypt (mongoc_client_t *client,
                          const char *db_name,
                          const bson_t *reply,
                          bson_t *decrypted,
                          bson_error_t *error)
{
   bson_init (decrypted);
   return _disabled_error (error);
}

bool
_mongoc_cse_client_enable_auto_encryption (
   mongoc_client_t *client,
   mongoc_auto_encryption_opts_t *opts /* may be NULL */,
   bson_error_t *error)
{
   return _disabled_error (error);
}

bool
_mongoc_cse_client_pool_enable_auto_encryption (
   mongoc_topology_t *topology,
   mongoc_auto_encryption_opts_t *opts /* may be NULL */,
   bson_error_t *error)
{
   return _disabled_error (error);
}


bool
mongoc_client_encryption_create_datakey (
   mongoc_client_encryption_t *client_encryption,
   const char *kms_provider,
   mongoc_client_encryption_datakey_opts_t *opts,
   bson_value_t *keyid,
   bson_error_t *error)
{
   if (keyid) {
      memset (keyid, 0, sizeof (*keyid));
   }
   return _disabled_error (error);
}


MONGOC_EXPORT (mongoc_client_encryption_t *)
mongoc_client_encryption_new (mongoc_client_encryption_opts_t *opts,
                              bson_error_t *error)
{
   _disabled_error (error);
   return NULL;
}

void
mongoc_client_encryption_destroy (mongoc_client_encryption_t *client_encryption)
{
}

bool
mongoc_client_encryption_encrypt (mongoc_client_encryption_t *client_encryption,
                                  const bson_value_t *value,
                                  mongoc_client_encryption_encrypt_opts_t *opts,
                                  bson_value_t *ciphertext,
                                  bson_error_t *error)
{
   if (ciphertext) {
      memset (ciphertext, 0, sizeof (*ciphertext));
   }
   return _disabled_error (error);
}

bool
mongoc_client_encryption_decrypt (mongoc_client_encryption_t *client_encryption,
                                  const bson_value_t *ciphertext,
                                  bson_value_t *value,
                                  bson_error_t *error)
{
   if (value) {
      memset (value, 0, sizeof (*value));
   }
   return _disabled_error (error);
}

bool
_mongoc_cse_is_enabled (mongoc_client_t *client)
{
   return false;
}

#else

/*--------------------------------------------------------------------------
 *
 * _prep_for_auto_encryption --
 *    If @cmd contains a type=1 payload (document sequence), convert it into
 *    a type=0 payload (array payload). See OP_MSG spec for details.
 *    Place the command BSON that should be encrypted into @out.
 *
 * Post-conditions:
 *    @out is initialized and set to the full payload. If @cmd did not include
 *    a type=1 payload, @out is statically initialized. Caller must not modify
 *    @out after, but must call bson_destroy.
 *
 * --------------------------------------------------------------------------
 */
static void
_prep_for_auto_encryption (const mongoc_cmd_t *cmd, bson_t *out)
{
   /* If there is no type=1 payload, return the command unchanged. */
   if (!cmd->payload || !cmd->payload_size) {
      bson_init_static (out, bson_get_data (cmd->command), cmd->command->len);
      return;
   }

   /* Otherwise, append the type=1 payload as an array. */
   bson_copy_to (cmd->command, out);
   _mongoc_cmd_append_payload_as_array (cmd, out);
}

/* Return the mongocryptd client to use on a client with automatic encryption
 * enabled.
 * If @client_encrypted is single-threaded, use the client to mongocryptd.
 * If @client_encrypted is multi-threaded, use the client pool to mongocryptd.
 */
mongoc_client_t *
_get_mongocryptd_client (mongoc_client_t *client_encrypted)
{
   if (client_encrypted->topology->single_threaded) {
      return client_encrypted->topology->mongocryptd_client;
   }
   return mongoc_client_pool_pop (
      client_encrypted->topology->mongocryptd_client_pool);
}

void
_release_mongocryptd_client (mongoc_client_t *client_encrypted,
                             mongoc_client_t *mongocryptd_client)
{
   if (!mongocryptd_client) {
      return;
   }
   if (!client_encrypted->topology->single_threaded) {
      mongoc_client_pool_push (
         client_encrypted->topology->mongocryptd_client_pool,
         mongocryptd_client);
   }
}

/* Return the key vault collection to use on a client with automatic encryption
 * enabled.
 * If no custom key vault client/pool is set, create a collection from the
 * @client_encrypted itself.
 * If @client_encrypted is single-threaded, use the client to mongocryptd to
 * create the collection.
 * If @client_encrypted is multi-threaded, use the client pool to mongocryptd
 * to create the collection.
 */
mongoc_collection_t *
_get_keyvault_coll (mongoc_client_t *client_encrypted)
{
   mongoc_client_t *keyvault_client;
   const char *db;
   const char *coll;

   db = client_encrypted->topology->keyvault_db;
   coll = client_encrypted->topology->keyvault_coll;

   if (client_encrypted->topology->single_threaded) {
      if (client_encrypted->topology->keyvault_client) {
         keyvault_client = client_encrypted->topology->keyvault_client;
      } else {
         keyvault_client = client_encrypted;
      }
   } else {
      if (client_encrypted->topology->keyvault_client_pool) {
         keyvault_client = mongoc_client_pool_pop (
            client_encrypted->topology->keyvault_client_pool);
      } else {
         keyvault_client = client_encrypted;
      }
   }
   return mongoc_client_get_collection (keyvault_client, db, coll);
}

void
_release_keyvault_coll (mongoc_client_t *client_encrypted,
                        mongoc_collection_t *keyvault_coll)
{
   mongoc_client_t *keyvault_client;

   if (!keyvault_coll) {
      return;
   }

   keyvault_client = keyvault_coll->client;
   mongoc_collection_destroy (keyvault_coll);
   if (!client_encrypted->topology->single_threaded &&
       client_encrypted->topology->keyvault_client_pool) {
      mongoc_client_pool_push (client_encrypted->topology->keyvault_client_pool,
                               keyvault_client);
   }
}

static bool
_spawn_mongocryptd (const char *mongocryptd_spawn_path,
                    const bson_t *mongocryptd_spawn_args,
                    bson_error_t *error);

/*--------------------------------------------------------------------------
 *
 * _mongoc_cse_auto_encrypt --
 *
 *       Perform automatic encryption if enabled.
 *
 * Return:
 *       True on success, false on error.
 *
 * Pre-conditions:
 *       CSE is enabled on client or its associated client pool.
 *
 * Post-conditions:
 *       If return false, @error is set. @encrypted is always initialized.
 *       @encrypted_cmd is set to the mongoc_cmd_t to send, which may refer
 *       to @encrypted.
 *       If automatic encryption was bypassed, @encrypted is set to an empty
 *       document, but @encrypted_cmd is a copy of @cmd. Caller must always
 *       bson_destroy @encrypted.
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_cse_auto_encrypt (mongoc_client_t *client_encrypted,
                          const mongoc_cmd_t *cmd,
                          mongoc_cmd_t *encrypted_cmd,
                          bson_t *encrypted,
                          bson_error_t *error)
{
   bool ret = false;
   bson_t cmd_bson = BSON_INITIALIZER;
   bson_t *result = NULL;
   bson_iter_t iter;
   mongoc_client_t *mongocryptd_client = NULL;
   mongoc_collection_t *keyvault_coll = NULL;
   bool retried = false;

   ENTRY;

   bson_init (encrypted);

   if (client_encrypted->topology->bypass_auto_encryption) {
      memcpy (encrypted_cmd, cmd, sizeof (mongoc_cmd_t));
      bson_destroy (&cmd_bson);
      RETURN (true);
   }

   if (cmd->server_stream->sd->max_wire_version < WIRE_VERSION_CSE) {
      bson_set_error (
         error,
         MONGOC_ERROR_PROTOCOL,
         MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
         "%s",
         "Auto-encryption requires a minimum MongoDB version of 4.2");
      GOTO (fail);
   }

   /* Construct the command we're sending to libmongocrypt. If cmd includes a
    * type 1 payload, convert it to a type 0 payload. */
   bson_destroy (&cmd_bson);
   _prep_for_auto_encryption (cmd, &cmd_bson);
   keyvault_coll = _get_keyvault_coll (client_encrypted);
   mongocryptd_client = _get_mongocryptd_client (client_encrypted);

retry:
   bson_destroy (encrypted);
   if (!_mongoc_crypt_auto_encrypt (client_encrypted->topology->crypt,
                                    keyvault_coll,
                                    mongocryptd_client,
                                    client_encrypted,
                                    cmd->db_name,
                                    &cmd_bson,
                                    encrypted,
                                    error)) {
      /* From the Client-Side Encryption spec: If spawning is necessary, the
       * driver MUST spawn mongocryptd whenever server selection on the
       * MongoClient to mongocryptd fails. If the MongoClient fails to connect
       * after spawning, the server selection error is propagated to the user.
       */
      if (!client_encrypted->topology->mongocryptd_bypass_spawn &&
          error->domain == MONGOC_ERROR_SERVER_SELECTION && !retried) {
         if (!_spawn_mongocryptd (
                client_encrypted->topology->mongocryptd_spawn_path,
                client_encrypted->topology->mongocryptd_spawn_args,
                error)) {
            GOTO (fail);
         }
         /* Respawn and retry. */
         memset (error, 0, sizeof (*error));
         retried = true;
         GOTO (retry);
      }
      GOTO (fail);
   }


   /* Re-append $db if encryption stripped it. */
   if (!bson_iter_init_find (&iter, encrypted, "$db")) {
      BSON_APPEND_UTF8 (encrypted, "$db", cmd->db_name);
   }

   /* Create the modified cmd_t. */
   memcpy (encrypted_cmd, cmd, sizeof (mongoc_cmd_t));
   /* Modify the mongoc_cmd_t and clear the payload, since
    * _mongoc_cse_auto_encrypt converted the payload into an embedded array. */
   encrypted_cmd->payload = NULL;
   encrypted_cmd->payload_size = 0;
   encrypted_cmd->command = encrypted;

   ret = true;

fail:
   bson_destroy (result);
   bson_destroy (&cmd_bson);
   _release_mongocryptd_client (client_encrypted, mongocryptd_client);
   _release_keyvault_coll (client_encrypted, keyvault_coll);
   RETURN (ret);
}

/*--------------------------------------------------------------------------
 *
 * _mongoc_cse_auto_decrypt --
 *
 *       Perform automatic decryption.
 *
 * Return:
 *       True on success, false on error.
 *
 * Pre-conditions:
 *       FLE is enabled on client or its associated client pool.
 *
 * Post-conditions:
 *       If return false, @error is set. @decrypted is always initialized.
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_cse_auto_decrypt (mongoc_client_t *client_encrypted,
                          const char *db_name,
                          const bson_t *reply,
                          bson_t *decrypted,
                          bson_error_t *error)
{
   bool ret = false;
   mongoc_collection_t *keyvault_coll = NULL;

   ENTRY;

   keyvault_coll = _get_keyvault_coll (client_encrypted);
   if (!_mongoc_crypt_auto_decrypt (client_encrypted->topology->crypt,
                                    keyvault_coll,
                                    reply,
                                    decrypted,
                                    error)) {
      GOTO (fail);
   }

   ret = true;

fail:
   _release_keyvault_coll (client_encrypted, keyvault_coll);
   RETURN (ret);
}

static void
_uri_construction_error (bson_error_t *error)
{
   bson_set_error (error,
                   MONGOC_ERROR_CLIENT,
                   MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                   "Error constructing URI to mongocryptd");
}


#ifdef _WIN32
static bool
_do_spawn (const char *path, char **args, bson_error_t *error)
{
   bson_string_t *command;
   char **arg;
   PROCESS_INFORMATION process_information;
   STARTUPINFO startup_info;

   /* Construct the full command, quote path and arguments. */
   command = bson_string_new ("");
   bson_string_append (command, "\"");
   if (path) {
      bson_string_append (command, path);
   }
   bson_string_append (command, "mongocryptd.exe");
   bson_string_append (command, "\"");
   /* skip the "mongocryptd" first arg. */
   arg = args + 1;
   while (*arg) {
      bson_string_append (command, " \"");
      bson_string_append (command, *arg);
      bson_string_append (command, "\"");
      arg++;
   }

   ZeroMemory (&process_information, sizeof (process_information));
   ZeroMemory (&startup_info, sizeof (startup_info));

   startup_info.cb = sizeof (startup_info);

   if (!CreateProcessA (NULL,
                        command->str,
                        NULL,
                        NULL,
                        false /* inherit descriptors */,
                        DETACHED_PROCESS /* FLAGS */,
                        NULL /* environment */,
                        NULL /* current directory */,
                        &startup_info,
                        &process_information)) {
      long lastError = GetLastError ();
      LPSTR message = NULL;

      FormatMessageA (
         FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY |
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL,
         lastError,
         0,
         (LPSTR) &message,
         0,
         NULL);

      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                      "failed to spawn mongocryptd: %s",
                      message);
      LocalFree (message);
      bson_string_free (command, true);
      return false;
   }

   bson_string_free (command, true);
   return true;
}
#else

/*--------------------------------------------------------------------------
 *
 * _do_spawn --
 *
 *   Spawn process defined by arg[0] on POSIX systems.
 *
 *   Note, if mongocryptd fails to spawn (due to not being found on the path),
 *   an error is not reported and true is returned. Users will get an error
 *   later, upon first attempt to use mongocryptd.
 *
 *   These comments refer to three distinct processes: parent, child, and
 *   mongocryptd.
 *   - parent is initial calling process
 *   - child is the first forked child. It fork-execs mongocryptd then
 *     terminates. This makes mongocryptd an orphan, making it immediately
 *     adopted by the init process.
 *   - mongocryptd is the final background daemon (grandchild process).
 *
 * Return:
 *   False if an error definitely occurred. Returns true if no reportable
 *   error occurred (though an error may have occurred in starting
 *   mongocryptd, resulting in the process not running).
 *
 * Arguments:
 *    args - A NULL terminated list of arguments. The first argument MUST
 *    be the name of the process to execute, and the last argument MUST be
 *    NULL.
 *
 * Post-conditions:
 *    If return false, @error is set.
 *
 *--------------------------------------------------------------------------
 */
static bool
_do_spawn (const char *path, char **args, bson_error_t *error)
{
   pid_t pid;
   int fd;
   char *to_exec;

   /* Fork. The child will terminate immediately (after fork-exec'ing
    * mongocryptd). This orphans mongocryptd, and allows parent to wait on
    * child. */
   pid = fork ();
   if (pid < 0) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                      "failed to fork (errno=%d) '%s'",
                      errno,
                      strerror (errno));
      return false;
   } else if (pid > 0) {
      int child_status;

      /* Child will spawn mongocryptd and immediately terminate to turn
       * mongocryptd into an orphan. */
      if (waitpid (pid, &child_status, 0 /* options */) < 0) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                         "failed to wait for child (errno=%d) '%s'",
                         errno,
                         strerror (errno));
         return false;
      }
      /* parent is done at this point, return. */
      return true;
   }

   /* We're no longer in the parent process. Errors encountered result in an
    * exit.
    * Note, we're not logging here, because that would require the user's log
    * callback to be fork-safe.
    */

   /* Start a new session for the child, so it is not bound to the current
    * session (e.g. terminal session). */
   if (setsid () < 0) {
      exit (EXIT_FAILURE);
   }

   /* Fork again. Child terminates so mongocryptd gets orphaned and immedately
    * adopted by init. */
   signal (SIGHUP, SIG_IGN);
   pid = fork ();
   if (pid < 0) {
      exit (EXIT_FAILURE);
   } else if (pid > 0) {
      /* Child terminates immediately. */
      exit (EXIT_SUCCESS);
   }

   /* If we later decide to change the working directory for the pid file path,
    * possibly change the process's working directory with chdir like: `chdir
    * (default_pid_path)`. Currently pid file ends up in application's working
    * directory. */

   /* Set the user file creation mask to zero. */
   umask (0);

   /* Close and reopen stdin. */
   fd = open ("/dev/null", O_RDONLY);
   if (fd < 0) {
      exit (EXIT_FAILURE);
   }
   dup2 (fd, STDIN_FILENO);
   close (fd);

   /* Close and reopen stdout. */
   fd = open ("/dev/null", O_WRONLY);
   if (fd < 0) {
      exit (EXIT_FAILURE);
   }
   if (dup2 (fd, STDOUT_FILENO) < 0 || close (fd) < 0) {
      exit (EXIT_FAILURE);
   }

   /* Close and reopen stderr. */
   fd = open ("/dev/null", O_RDWR);
   if (fd < 0) {
      exit (EXIT_FAILURE);
   }
   if (dup2 (fd, STDERR_FILENO) < 0 || close (fd) < 0) {
      exit (EXIT_FAILURE);
   }

   if (path) {
      to_exec = bson_strdup_printf ("%s%s", path, args[0]);
   } else {
      to_exec = bson_strdup (args[0]);
   }
   if (execvp (to_exec, args) < 0) {
      /* Need to exit. */
      exit (EXIT_FAILURE);
   }

   /* Will never execute. */
   return false;
}
#endif

/*--------------------------------------------------------------------------
 *
 * _spawn_mongocryptd --
 *
 *   Attempt to spawn mongocryptd as a background process.
 *
 * Return:
 *   False if an error definitely occurred. Returns true if no reportable
 *   error occurred (though an error may have occurred in starting
 *   mongocryptd, resulting in the process not running).
 *
 * Arguments:
 *    mongocryptd_spawn_path May be NULL, otherwise the path to mongocryptd.
 *    mongocryptd_spawn_args May be NULL, otherwise a bson_iter_t to the
 *    value "mongocryptdSpawnArgs" in AutoEncryptionOpts.extraOptions
 *    (see spec).
 *
 * Post-conditions:
 *    If return false, @error is set.
 *
 *--------------------------------------------------------------------------
 */
static bool
_spawn_mongocryptd (const char *mongocryptd_spawn_path,
                    const bson_t *mongocryptd_spawn_args,
                    bson_error_t *error)
{
   char **args = NULL;
   bson_iter_t iter;
   bool passed_idle_shutdown_timeout_secs = false;
   int num_args = 2; /* for leading "mongocrypt" and trailing NULL */
   int i;
   bool ret;

   /* iterate once to get length and validate all are strings */
   if (mongocryptd_spawn_args) {
      bson_iter_init (&iter, mongocryptd_spawn_args);
      while (bson_iter_next (&iter)) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_CLIENT,
                            MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                            "invalid argument for mongocryptd, must be string");
            return false;
         }
         /* Check if the arg starts with --idleShutdownTimeoutSecs= or is equal
          * to --idleShutdownTimeoutSecs */
         if (0 == strncmp ("--idleShutdownTimeoutSecs=",
                           bson_iter_utf8 (&iter, NULL),
                           26) ||
             0 == strcmp ("--idleShutdownTimeoutSecs",
                          bson_iter_utf8 (&iter, NULL))) {
            passed_idle_shutdown_timeout_secs = true;
         }
         num_args++;
      }
   }

   if (!passed_idle_shutdown_timeout_secs) {
      /* add one more */
      num_args++;
   }

   args = (char **) bson_malloc (sizeof (char *) * num_args);
   i = 0;
   args[i++] = "mongocryptd";

   if (mongocryptd_spawn_args) {
      bson_iter_init (&iter, mongocryptd_spawn_args);
      while (bson_iter_next (&iter)) {
         args[i++] = (char *) bson_iter_utf8 (&iter, NULL);
      }
   }

   if (!passed_idle_shutdown_timeout_secs) {
      args[i++] = "--idleShutdownTimeoutSecs=60";
   }

   BSON_ASSERT (i == num_args - 1);
   args[i++] = NULL;

   ret = _do_spawn (mongocryptd_spawn_path, args, error);
   bson_free (args);
   return ret;
}

static bool
_parse_extra (const bson_t *extra,
              mongoc_topology_t *topology,
              mongoc_uri_t **uri,
              bson_error_t *error)
{
   bson_iter_t iter;
   bool ret = false;

   ENTRY;

   *uri = NULL;
   if (extra) {
      if (bson_iter_init_find (&iter, extra, "mongocryptdBypassSpawn")) {
         if (!BSON_ITER_HOLDS_BOOL (&iter)) {
            bson_set_error (
               error,
               MONGOC_ERROR_CLIENT,
               MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
               "Expected bool for option 'mongocryptdBypassSpawn'");
            GOTO (fail);
         }
         topology->mongocryptd_bypass_spawn = bson_iter_bool (&iter);
      }
      if (bson_iter_init_find (&iter, extra, "mongocryptdSpawnPath")) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            bson_set_error (
               error,
               MONGOC_ERROR_CLIENT,
               MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
               "Expected string for option 'mongocryptdSpawnPath'");
            GOTO (fail);
         }
         topology->mongocryptd_spawn_path =
            bson_strdup (bson_iter_utf8 (&iter, NULL));
      }
      if (bson_iter_init_find (&iter, extra, "mongocryptdSpawnArgs")) {
         uint32_t array_len;
         const uint8_t *array_data;

         if (!BSON_ITER_HOLDS_ARRAY (&iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_CLIENT,
                            MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                            "Expected array for option 'mongocryptdSpawnArgs'");
            GOTO (fail);
         }
         bson_iter_array (&iter, &array_len, &array_data);
         topology->mongocryptd_spawn_args =
            bson_new_from_data (array_data, array_len);
      }

      if (bson_iter_init_find (&iter, extra, "mongocryptdURI")) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            bson_set_error (error,
                            MONGOC_ERROR_CLIENT,
                            MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                            "Expected string for option 'mongocryptdURI'");
            GOTO (fail);
         }
         *uri = mongoc_uri_new_with_error (bson_iter_utf8 (&iter, NULL), error);
         if (!*uri) {
            GOTO (fail);
         }
      }
   }


   if (!*uri) {
      *uri = mongoc_uri_new_with_error ("mongodb://localhost:27020", error);

      if (!*uri) {
         GOTO (fail);
      }

      if (!mongoc_uri_set_option_as_int32 (
             *uri, MONGOC_URI_SERVERSELECTIONTIMEOUTMS, 10000)) {
         _uri_construction_error (error);
         GOTO (fail);
      }
   }

   ret = true;
fail:
   RETURN (ret);
}

bool
_mongoc_cse_client_enable_auto_encryption (mongoc_client_t *client,
                                           mongoc_auto_encryption_opts_t *opts,
                                           bson_error_t *error)
{
   bool ret = false;
   mongoc_uri_t *mongocryptd_uri = NULL;

   ENTRY;

   BSON_ASSERT (client);
   if (!client->topology->single_threaded) {
      bson_set_error (
         error,
         MONGOC_ERROR_CLIENT,
         MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
         "Automatic encryption on pooled clients must be set on the pool");
      GOTO (fail);
   }

   if (!opts) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "Auto encryption options required");
      GOTO (fail);
   }

   if (opts->keyvault_client_pool) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "The key vault client pool only applies to a client "
                      "pool, not a single threaded client");
      GOTO (fail);
   }

   if (opts->keyvault_client &&
       !opts->keyvault_client->topology->single_threaded) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "The key vault client must be single threaded, not be "
                      "from a client pool");
      GOTO (fail);
   }

   /* Check for required options */
   if (!opts->keyvault_db || !opts->keyvault_coll) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "Key vault namespace option required");
      GOTO (fail);
   }

   if (!opts->kms_providers) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "KMS providers option required");
      GOTO (fail);
   }

   if (client->topology->cse_state != MONGOC_CSE_DISABLED) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                      "Automatic encryption already set");
      GOTO (fail);
   } else {
      client->topology->cse_state = MONGOC_CSE_ENABLED;
   }

   if (!_parse_extra (opts->extra, client->topology, &mongocryptd_uri, error)) {
      GOTO (fail);
   }

   client->topology->crypt = _mongoc_crypt_new (
      opts->kms_providers, opts->schema_map, opts->tls_opts, error);
   if (!client->topology->crypt) {
      GOTO (fail);
   }

   client->topology->bypass_auto_encryption = opts->bypass_auto_encryption;

   if (!client->topology->bypass_auto_encryption) {
      if (!client->topology->mongocryptd_bypass_spawn) {
         if (!_spawn_mongocryptd (client->topology->mongocryptd_spawn_path,
                                  client->topology->mongocryptd_spawn_args,
                                  error)) {
            GOTO (fail);
         }
      }

      /* By default, single threaded clients set serverSelectionTryOnce to
       * true, which means server selection fails if a topology scan fails
       * the first time (i.e. it will not make repeat attempts until
       * serverSelectionTimeoutMS expires). Override this, since the first
       * attempt to connect to mongocryptd may fail when spawning, as it
       * takes some time for mongocryptd to listen on sockets. */
      if (!mongoc_uri_set_option_as_bool (
             mongocryptd_uri, MONGOC_URI_SERVERSELECTIONTRYONCE, false)) {
         _uri_construction_error (error);
         GOTO (fail);
      }

      client->topology->mongocryptd_client =
         mongoc_client_new_from_uri (mongocryptd_uri);

      if (!client->topology->mongocryptd_client) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                         "Unable to create client to mongocryptd");
         GOTO (fail);
      }
      /* Similarly, single threaded clients will by default wait for 5 second
       * cooldown period after failing to connect to a server before making
       * another attempt. Meaning if the first attempt to mongocryptd fails
       * to connect, then the user observes a 5 second delay. This is not
       * configurable in the URI, so override. */
      _mongoc_topology_bypass_cooldown (
         client->topology->mongocryptd_client->topology);

      /* Also, since single threaded server selection can foreseeably take
       * connectTimeoutMS (which by default is longer than 10 seconds), reduce
       * this as well. */
      if (!mongoc_uri_set_option_as_int32 (
             mongocryptd_uri, MONGOC_URI_CONNECTTIMEOUTMS, 10000)) {
         _uri_construction_error (error);
         GOTO (fail);
      }
   }

   client->topology->keyvault_db = bson_strdup (opts->keyvault_db);
   client->topology->keyvault_coll = bson_strdup (opts->keyvault_coll);
   if (opts->keyvault_client) {
      client->topology->keyvault_client = opts->keyvault_client;
   }

   ret = true;
fail:
   mongoc_uri_destroy (mongocryptd_uri);
   RETURN (ret);
}

bool
_mongoc_cse_client_pool_enable_auto_encryption (
   mongoc_topology_t *topology,
   mongoc_auto_encryption_opts_t *opts,
   bson_error_t *error)
{
   bool setup_okay = false;
   mongoc_uri_t *mongocryptd_uri = NULL;
   mongoc_topology_cse_state_t prev_cse_state = MONGOC_CSE_STARTING;

   BSON_ASSERT (topology);
   if (!opts) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "Auto encryption options required");
      GOTO (fail);
   }

   if (opts->keyvault_client) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "The key vault client only applies to a single threaded "
                      "client not a client pool. Set a key vault client pool");
      GOTO (fail);
   }

   /* Check for required options */
   if (!opts->keyvault_db || !opts->keyvault_coll) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "Key vault namespace option required");
      GOTO (fail);
   }

   if (!opts->kms_providers) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "KMS providers option required");
      GOTO (fail);
   }

   prev_cse_state =
      bson_atomic_int_compare_exchange_strong ((int *) &topology->cse_state,
                                               MONGOC_CSE_DISABLED,
                                               MONGOC_CSE_STARTING,
                                               bson_memory_order_acquire);
   while (prev_cse_state == MONGOC_CSE_STARTING) {
      /* Another thread is starting client-side encryption. It may take some
       * time to start, but don't continue until it is finished. */
      bson_thrd_yield ();
      prev_cse_state =
         bson_atomic_int_compare_exchange_strong ((int *) &topology->cse_state,
                                                  MONGOC_CSE_DISABLED,
                                                  MONGOC_CSE_STARTING,
                                                  bson_memory_order_acquire);
   }

   if (prev_cse_state == MONGOC_CSE_ENABLED) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                      "Automatic encryption already set");
      GOTO (fail);
   }

   /* We just set the CSE state from DISABLED to STARTING. Start it up now. */

   if (!_parse_extra (opts->extra, topology, &mongocryptd_uri, error)) {
      GOTO (fail);
   }

   topology->crypt = _mongoc_crypt_new (
      opts->kms_providers, opts->schema_map, opts->tls_opts, error);
   if (!topology->crypt) {
      GOTO (fail);
   }

   topology->bypass_auto_encryption = opts->bypass_auto_encryption;

   if (!topology->bypass_auto_encryption) {
      if (!topology->mongocryptd_bypass_spawn) {
         if (!_spawn_mongocryptd (topology->mongocryptd_spawn_path,
                                  topology->mongocryptd_spawn_args,
                                  error)) {
            GOTO (fail);
         }
      }

      topology->mongocryptd_client_pool =
         mongoc_client_pool_new (mongocryptd_uri);

      if (!topology->mongocryptd_client_pool) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                         "Unable to create client pool to mongocryptd");
         GOTO (fail);
      }
   }

   topology->keyvault_db = bson_strdup (opts->keyvault_db);
   topology->keyvault_coll = bson_strdup (opts->keyvault_coll);
   if (opts->keyvault_client_pool) {
      topology->keyvault_client_pool = opts->keyvault_client_pool;
   }

   setup_okay = true;
   BSON_ASSERT (prev_cse_state == MONGOC_CSE_DISABLED);
fail:
   if (prev_cse_state == MONGOC_CSE_DISABLED) {
      /* We need to set the new CSE state. */
      mongoc_topology_cse_state_t new_state =
         setup_okay ? MONGOC_CSE_ENABLED : MONGOC_CSE_DISABLED;
      bson_atomic_int_exchange (
         (int *) &topology->cse_state, new_state, bson_memory_order_release);
   }
   mongoc_uri_destroy (mongocryptd_uri);
   RETURN (setup_okay);
}

struct _mongoc_client_encryption_t {
   _mongoc_crypt_t *crypt;
   mongoc_collection_t *keyvault_coll;
   bson_t *kms_providers;
};

mongoc_client_encryption_t *
mongoc_client_encryption_new (mongoc_client_encryption_opts_t *opts,
                              bson_error_t *error)
{
   mongoc_client_encryption_t *client_encryption = NULL;
   bool success = false;
   mongoc_write_concern_t *wc = NULL;

   /* Check for required options */
   if (!opts || !opts->keyvault_client || !opts->keyvault_db ||
       !opts->keyvault_coll) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "Key vault client and namespace option required");
      goto fail;
   }

   if (!opts->kms_providers) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "KMS providers option required");
      goto fail;
   }

   client_encryption = bson_malloc0 (sizeof (*client_encryption));
   client_encryption->keyvault_coll = mongoc_client_get_collection (
      opts->keyvault_client, opts->keyvault_db, opts->keyvault_coll);
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_wmajority (wc, 1000);
   mongoc_collection_set_write_concern (client_encryption->keyvault_coll, wc);

   client_encryption->kms_providers = bson_copy (opts->kms_providers);
   client_encryption->crypt = _mongoc_crypt_new (
      opts->kms_providers, NULL /* schema_map */, opts->tls_opts, error);
   if (!client_encryption->crypt) {
      goto fail;
   }
   success = true;

fail:
   mongoc_write_concern_destroy (wc);
   if (!success) {
      mongoc_client_encryption_destroy (client_encryption);
      return NULL;
   }
   return client_encryption;
}

void
mongoc_client_encryption_destroy (mongoc_client_encryption_t *client_encryption)
{
   if (!client_encryption) {
      return;
   }
   _mongoc_crypt_destroy (client_encryption->crypt);
   mongoc_collection_destroy (client_encryption->keyvault_coll);
   bson_destroy (client_encryption->kms_providers);
   bson_free (client_encryption);
}

bool
mongoc_client_encryption_create_datakey (
   mongoc_client_encryption_t *client_encryption,
   const char *kms_provider,
   mongoc_client_encryption_datakey_opts_t *opts,
   bson_value_t *keyid,
   bson_error_t *error)
{
   bool ret = false;
   bson_t datakey = BSON_INITIALIZER;
   bson_t insert_opts = BSON_INITIALIZER;

   ENTRY;

   if (!opts) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "required 'opts' unset");
      GOTO (fail);
   }

   /* reset, so it is safe for caller to call bson_value_destroy on error or
    * success. */
   if (keyid) {
      keyid->value_type = BSON_TYPE_EOD;
   }

   bson_destroy (&datakey);
   if (!_mongoc_crypt_create_datakey (client_encryption->crypt,
                                      kms_provider,
                                      opts->masterkey,
                                      opts->keyaltnames,
                                      opts->keyaltnames_count,
                                      &datakey,
                                      error)) {
      GOTO (fail);
   }

   /* Insert the data key with write concern majority */
   if (!mongoc_collection_insert_one (client_encryption->keyvault_coll,
                                      &datakey,
                                      NULL /* opts */,
                                      NULL /* reply */,
                                      error)) {
      GOTO (fail);
   }

   if (keyid) {
      bson_iter_t iter;
      const bson_value_t *id_value;

      if (!bson_iter_init_find (&iter, &datakey, "_id")) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                         "data key not did not contain _id");
         GOTO (fail);
      } else if (!BSON_ITER_HOLDS_BINARY (&iter)) {
         bson_set_error (error,
                         MONGOC_ERROR_CLIENT,
                         MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                         "data key _id does not contain binary");
         GOTO (fail);
      } else {
         id_value = bson_iter_value (&iter);
         bson_value_copy (id_value, keyid);
      }
   }

   ret = true;

fail:
   bson_destroy (&insert_opts);
   bson_destroy (&datakey);

   RETURN (ret);
}

bool
mongoc_client_encryption_encrypt (mongoc_client_encryption_t *client_encryption,
                                  const bson_value_t *value,
                                  mongoc_client_encryption_encrypt_opts_t *opts,
                                  bson_value_t *ciphertext,
                                  bson_error_t *error)
{
   bool ret = false;

   ENTRY;

   BSON_ASSERT (client_encryption);

   if (!ciphertext) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "required 'ciphertext' unset");
      GOTO (fail);
   }
   /* reset, so it is safe for caller to call bson_value_destroy on error or
    * success. */
   ciphertext->value_type = BSON_TYPE_EOD;

   if (!opts) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "required 'opts' unset");
      GOTO (fail);
   }

   if (!_mongoc_crypt_explicit_encrypt (client_encryption->crypt,
                                        client_encryption->keyvault_coll,
                                        opts->algorithm,
                                        &opts->keyid,
                                        opts->keyaltname,
                                        value,
                                        ciphertext,
                                        error)) {
      GOTO (fail);
   }

   ret = true;
fail:
   RETURN (ret);
}

bool
mongoc_client_encryption_decrypt (mongoc_client_encryption_t *client_encryption,
                                  const bson_value_t *ciphertext,
                                  bson_value_t *value,
                                  bson_error_t *error)
{
   bool ret = false;

   ENTRY;

   BSON_ASSERT (client_encryption);

   if (!value) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "required 'value' unset");
      GOTO (fail);
   }

   /* reset, so it is safe for caller to call bson_value_destroy on error or
    * success. */
   value->value_type = BSON_TYPE_EOD;

   if (ciphertext->value_type != BSON_TYPE_BINARY ||
       ciphertext->value.v_binary.subtype != BSON_SUBTYPE_ENCRYPTED) {
      bson_set_error (error,
                      MONGOC_ERROR_CLIENT,
                      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                      "ciphertext must be BSON binary subtype 6");
      GOTO (fail);
   }

   if (!_mongoc_crypt_explicit_decrypt (client_encryption->crypt,
                                        client_encryption->keyvault_coll,
                                        ciphertext,
                                        value,
                                        error)) {
      GOTO (fail);
   }

   ret = true;
fail:
   RETURN (ret);
}

bool
_mongoc_cse_is_enabled (mongoc_client_t *client)
{
   while (1) {
      mongoc_topology_cse_state_t state = bson_atomic_int_fetch (
         (int *) &client->topology->cse_state, bson_memory_order_relaxed);
      if (state != MONGOC_CSE_STARTING) {
         return state == MONGOC_CSE_ENABLED;
      }
      /* CSE is starting up. Wait until that succeeds or fails. */
      bson_thrd_yield ();
   }
}

#endif /* MONGOC_ENABLE_CLIENT_SIDE_ENCRYPTION */
