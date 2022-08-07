/*
 * Copyright 2014 MongoDB, Inc.
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


#ifndef MONGOC_SCRAM_PRIVATE_H
#define MONGOC_SCRAM_PRIVATE_H


#include <bson/bson.h>
#include "mongoc-crypto-private.h"


BSON_BEGIN_DECLS

#define MONGOC_SCRAM_SHA_1_HASH_SIZE 20
#define MONGOC_SCRAM_SHA_256_HASH_SIZE 32
/* SCRAM-SHA-1 uses a hash size of 20, and SCRAM-SHA-256 uses a hash size
 * of 32. Stack allocations should be large enough for either. */
#define MONGOC_SCRAM_HASH_MAX_SIZE MONGOC_SCRAM_SHA_256_HASH_SIZE

#define MONGOC_SCRAM_B64_ENCODED_SIZE(n) (2 * n)

#define MONGOC_SCRAM_B64_HASH_MAX_SIZE \
   MONGOC_SCRAM_B64_ENCODED_SIZE (MONGOC_SCRAM_HASH_MAX_SIZE)

typedef struct _mongoc_scram_cache_t {
   /* pre-secrets */
   char *hashed_password;
   uint8_t decoded_salt[MONGOC_SCRAM_B64_HASH_MAX_SIZE];
   uint32_t iterations;
   /* secrets */
   uint8_t client_key[MONGOC_SCRAM_HASH_MAX_SIZE];
   uint8_t server_key[MONGOC_SCRAM_HASH_MAX_SIZE];
   uint8_t salted_password[MONGOC_SCRAM_HASH_MAX_SIZE];
} mongoc_scram_cache_t;

typedef struct _mongoc_scram_t {
   int step;
   char *user;
   char *pass;
   char *hashed_password;
   uint8_t decoded_salt[MONGOC_SCRAM_B64_HASH_MAX_SIZE];
   uint32_t iterations;
   uint8_t client_key[MONGOC_SCRAM_HASH_MAX_SIZE];
   uint8_t server_key[MONGOC_SCRAM_HASH_MAX_SIZE];
   uint8_t salted_password[MONGOC_SCRAM_HASH_MAX_SIZE];
   char encoded_nonce[48];
   int32_t encoded_nonce_len;
   uint8_t *auth_message;
   uint32_t auth_messagemax;
   uint32_t auth_messagelen;
#ifdef MONGOC_ENABLE_CRYPTO
   mongoc_crypto_t crypto;
#endif
   mongoc_scram_cache_t *cache;
} mongoc_scram_t;

#ifdef MONGOC_ENABLE_CRYPTO
void
_mongoc_scram_init (mongoc_scram_t *scram, mongoc_crypto_hash_algorithm_t algo);
#endif

mongoc_scram_cache_t *
_mongoc_scram_get_cache (mongoc_scram_t *scram);

void
_mongoc_scram_set_cache (mongoc_scram_t *scram, mongoc_scram_cache_t *cache);

void
_mongoc_scram_set_pass (mongoc_scram_t *scram, const char *pass);

void
_mongoc_scram_set_user (mongoc_scram_t *scram, const char *user);

void
_mongoc_scram_set_server_key (mongoc_scram_t *scram,
                              const uint8_t *server_key,
                              size_t len);

void
_mongoc_scram_set_salted_password (mongoc_scram_t *scram,
                                   const uint8_t *salted_password,
                                   size_t len);

void
_mongoc_scram_destroy (mongoc_scram_t *scram);

bool
_mongoc_scram_step (mongoc_scram_t *scram,
                    const uint8_t *inbuf,
                    uint32_t inbuflen,
                    uint8_t *outbuf,
                    uint32_t outbufmax,
                    uint32_t *outbuflen,
                    bson_error_t *error);

void
_mongoc_scram_cache_destroy (mongoc_scram_cache_t *cache);

/* returns false if this string does not need SASLPrep. It returns true
 * conservatively, if str might need to be SASLPrep'ed. */
bool
_mongoc_sasl_prep_required (const char *str);

/* returns the output of SASLPrep as a new string which must be freed. Returns
 * null on error and sets err.
 * `name` should be "username" or "password". */
char *
_mongoc_sasl_prep (const char *in_utf8, int in_utf8_len, bson_error_t *err);

BSON_END_DECLS


#endif /* MONGOC_SCRAM_PRIVATE_H */
