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

#include "mongoc-prelude.h"

#ifndef MONGOC_CLIENT_SIDE_ENCRYPTION_H
#define MONGOC_CLIENT_SIDE_ENCRYPTION_H

#include <bson/bson.h>

/* Forward declare */
struct _mongoc_client_t;
struct _mongoc_client_pool_t;

#define MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_RANDOM \
   "AEAD_AES_256_CBC_HMAC_SHA_512-Random"
#define MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_DETERMINISTIC \
   "AEAD_AES_256_CBC_HMAC_SHA_512-Deterministic"

BSON_BEGIN_DECLS

typedef struct _mongoc_auto_encryption_opts_t mongoc_auto_encryption_opts_t;

MONGOC_EXPORT (mongoc_auto_encryption_opts_t *)
mongoc_auto_encryption_opts_new (void) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_destroy (mongoc_auto_encryption_opts_t *opts);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_keyvault_client (
   mongoc_auto_encryption_opts_t *opts, struct _mongoc_client_t *client);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_keyvault_client_pool (
   mongoc_auto_encryption_opts_t *opts, struct _mongoc_client_pool_t *pool);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_keyvault_namespace (
   mongoc_auto_encryption_opts_t *opts, const char *db, const char *coll);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_kms_providers (
   mongoc_auto_encryption_opts_t *opts, const bson_t *kms_providers);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_tls_opts (mongoc_auto_encryption_opts_t *opts,
                                          const bson_t *tls_opts);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_schema_map (mongoc_auto_encryption_opts_t *opts,
                                            const bson_t *schema_map);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_bypass_auto_encryption (
   mongoc_auto_encryption_opts_t *opts, bool bypass_auto_encryption);

MONGOC_EXPORT (void)
mongoc_auto_encryption_opts_set_extra (mongoc_auto_encryption_opts_t *opts,
                                       const bson_t *extra);

typedef struct _mongoc_client_encryption_opts_t mongoc_client_encryption_opts_t;
typedef struct _mongoc_client_encryption_t mongoc_client_encryption_t;
typedef struct _mongoc_client_encryption_encrypt_opts_t
   mongoc_client_encryption_encrypt_opts_t;
typedef struct _mongoc_client_encryption_datakey_opts_t
   mongoc_client_encryption_datakey_opts_t;

MONGOC_EXPORT (mongoc_client_encryption_opts_t *)
mongoc_client_encryption_opts_new (void) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_client_encryption_opts_destroy (mongoc_client_encryption_opts_t *opts);

MONGOC_EXPORT (void)
mongoc_client_encryption_opts_set_keyvault_client (
   mongoc_client_encryption_opts_t *opts,
   struct _mongoc_client_t *keyvault_client);

MONGOC_EXPORT (void)
mongoc_client_encryption_opts_set_keyvault_namespace (
   mongoc_client_encryption_opts_t *opts, const char *db, const char *coll);

MONGOC_EXPORT (void)
mongoc_client_encryption_opts_set_kms_providers (
   mongoc_client_encryption_opts_t *opts, const bson_t *kms_providers);

MONGOC_EXPORT (void)
mongoc_client_encryption_opts_set_tls_opts (
   mongoc_client_encryption_opts_t *opts, const bson_t *tls_opts);

MONGOC_EXPORT (mongoc_client_encryption_t *)
mongoc_client_encryption_new (mongoc_client_encryption_opts_t *opts,
                              bson_error_t *error) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_client_encryption_destroy (
   mongoc_client_encryption_t *client_encryption);

MONGOC_EXPORT (bool)
mongoc_client_encryption_create_datakey (
   mongoc_client_encryption_t *client_encryption,
   const char *kms_provider,
   mongoc_client_encryption_datakey_opts_t *opts,
   bson_value_t *keyid,
   bson_error_t *error);

MONGOC_EXPORT (bool)
mongoc_client_encryption_encrypt (mongoc_client_encryption_t *client_encryption,
                                  const bson_value_t *value,
                                  mongoc_client_encryption_encrypt_opts_t *opts,
                                  bson_value_t *ciphertext,
                                  bson_error_t *error);

MONGOC_EXPORT (bool)
mongoc_client_encryption_decrypt (mongoc_client_encryption_t *client_encryption,
                                  const bson_value_t *ciphertext,
                                  bson_value_t *value,
                                  bson_error_t *error);

MONGOC_EXPORT (mongoc_client_encryption_encrypt_opts_t *)
mongoc_client_encryption_encrypt_opts_new (void) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_client_encryption_encrypt_opts_destroy (
   mongoc_client_encryption_encrypt_opts_t *opts);

MONGOC_EXPORT (void)
mongoc_client_encryption_encrypt_opts_set_keyid (
   mongoc_client_encryption_encrypt_opts_t *opts, const bson_value_t *keyid);

MONGOC_EXPORT (void)
mongoc_client_encryption_encrypt_opts_set_keyaltname (
   mongoc_client_encryption_encrypt_opts_t *opts, const char *keyaltname);

MONGOC_EXPORT (void)
mongoc_client_encryption_encrypt_opts_set_algorithm (
   mongoc_client_encryption_encrypt_opts_t *opts, const char *algorithm);

MONGOC_EXPORT (mongoc_client_encryption_datakey_opts_t *)
mongoc_client_encryption_datakey_opts_new (void) BSON_GNUC_WARN_UNUSED_RESULT;

MONGOC_EXPORT (void)
mongoc_client_encryption_datakey_opts_destroy (
   mongoc_client_encryption_datakey_opts_t *opts);

MONGOC_EXPORT (void)
mongoc_client_encryption_datakey_opts_set_masterkey (
   mongoc_client_encryption_datakey_opts_t *opts, const bson_t *masterkey);

MONGOC_EXPORT (void)
mongoc_client_encryption_datakey_opts_set_keyaltnames (
   mongoc_client_encryption_datakey_opts_t *opts,
   char **keyaltnames,
   uint32_t keyaltnames_count);

BSON_END_DECLS

#endif /* MONGOC_CLIENT_SIDE_ENCRYPTION_H */
