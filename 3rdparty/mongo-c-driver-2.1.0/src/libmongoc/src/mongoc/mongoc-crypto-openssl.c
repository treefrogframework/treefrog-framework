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

#include <mongoc/mongoc-config.h>

#include <bson/bson.h>

#ifdef MONGOC_ENABLE_CRYPTO_LIBCRYPTO
#include <mongoc/mongoc-crypto-openssl-private.h>
#include <mongoc/mongoc-crypto-private.h>

#include <mongoc/mongoc-log.h>

#include <mlib/cmp.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

bool
mongoc_crypto_openssl_pbkdf2_hmac_sha1 (mongoc_crypto_t *crypto,
                                        const char *password,
                                        size_t password_len,
                                        const uint8_t *salt,
                                        size_t salt_len,
                                        uint32_t iterations,
                                        size_t output_len,
                                        unsigned char *output)
{
   BSON_UNUSED (crypto);

   if (BSON_UNLIKELY (mlib_cmp (password_len, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC password length exceeds INT_MAX");
      return false;
   }

   if (BSON_UNLIKELY (mlib_cmp (salt_len, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC salt length exceeds INT_MAX");
      return false;
   }

   if (BSON_UNLIKELY (mlib_cmp (iterations, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC iteration count exceeds INT_MAX");
      return false;
   }

   if (BSON_UNLIKELY (mlib_cmp (iterations, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC output buffer length exceeds INT_MAX");
      return false;
   }

   return 0 != PKCS5_PBKDF2_HMAC (password,
                                  (int) password_len,
                                  salt,
                                  (int) salt_len,
                                  (int) iterations,
                                  EVP_sha1 (),
                                  (int) output_len,
                                  output);
}

void
mongoc_crypto_openssl_hmac_sha1 (mongoc_crypto_t *crypto,
                                 const void *key,
                                 int key_len,
                                 const unsigned char *data,
                                 int data_len,
                                 unsigned char *hmac_out)
{
   BSON_UNUSED (crypto);

   /* U1 = HMAC(input, salt + 0001) */
   HMAC (EVP_sha1 (), key, key_len, data, data_len, hmac_out, NULL);
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
static EVP_MD_CTX *
EVP_MD_CTX_new (void)
{
   return bson_malloc0 (sizeof (EVP_MD_CTX));
}

static void
EVP_MD_CTX_free (EVP_MD_CTX *ctx)
{
   EVP_MD_CTX_cleanup (ctx);
   bson_free (ctx);
}
#endif

bool
mongoc_crypto_openssl_sha1 (mongoc_crypto_t *crypto,
                            const unsigned char *input,
                            const size_t input_len,
                            unsigned char *hash_out)
{
   EVP_MD_CTX *digest_ctxp = EVP_MD_CTX_new ();
   bool rval = false;

   BSON_UNUSED (crypto);

   if (1 != EVP_DigestInit_ex (digest_ctxp, EVP_sha1 (), NULL)) {
      goto cleanup;
   }

   if (1 != EVP_DigestUpdate (digest_ctxp, input, input_len)) {
      goto cleanup;
   }

   rval = (1 == EVP_DigestFinal_ex (digest_ctxp, hash_out, NULL));

cleanup:
   EVP_MD_CTX_free (digest_ctxp);

   return rval;
}

bool
mongoc_crypto_openssl_pbkdf2_hmac_sha256 (mongoc_crypto_t *crypto,
                                          const char *password,
                                          size_t password_len,
                                          const uint8_t *salt,
                                          size_t salt_len,
                                          uint32_t iterations,
                                          size_t output_len,
                                          unsigned char *output)
{
   BSON_UNUSED (crypto);

   if (BSON_UNLIKELY (mlib_cmp (password_len, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC password length exceeds INT_MAX");
      return false;
   }

   if (BSON_UNLIKELY (mlib_cmp (salt_len, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC salt length exceeds INT_MAX");
      return false;
   }

   if (BSON_UNLIKELY (mlib_cmp (iterations, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC iteration count exceeds INT_MAX");
      return false;
   }

   if (BSON_UNLIKELY (mlib_cmp (iterations, >, INT_MAX))) {
      MONGOC_ERROR ("PBKDF2 HMAC output buffer length exceeds INT_MAX");
      return false;
   }

   return 0 != PKCS5_PBKDF2_HMAC (password,
                                  (int) password_len,
                                  salt,
                                  (int) salt_len,
                                  (int) iterations,
                                  EVP_sha256 (),
                                  (int) output_len,
                                  output);
}

void
mongoc_crypto_openssl_hmac_sha256 (mongoc_crypto_t *crypto,
                                   const void *key,
                                   int key_len,
                                   const unsigned char *data,
                                   int data_len,
                                   unsigned char *hmac_out)
{
   BSON_UNUSED (crypto);

   /* U1 = HMAC(input, salt + 0001) */
   HMAC (EVP_sha256 (), key, key_len, data, data_len, hmac_out, NULL);
}

bool
mongoc_crypto_openssl_sha256 (mongoc_crypto_t *crypto,
                              const unsigned char *input,
                              const size_t input_len,
                              unsigned char *hash_out)
{
   EVP_MD_CTX *digest_ctxp = EVP_MD_CTX_new ();
   bool rval = false;

   BSON_UNUSED (crypto);

   if (1 != EVP_DigestInit_ex (digest_ctxp, EVP_sha256 (), NULL)) {
      goto cleanup;
   }

   if (1 != EVP_DigestUpdate (digest_ctxp, input, input_len)) {
      goto cleanup;
   }

   rval = (1 == EVP_DigestFinal_ex (digest_ctxp, hash_out, NULL));

cleanup:
   EVP_MD_CTX_free (digest_ctxp);

   return rval;
}
#endif
