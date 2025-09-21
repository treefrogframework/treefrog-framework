/* Copyright 2009-present MongoDB, Inc.
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

#include <mongoc/mongoc-crypto-private.h>

#include <mongoc/mongoc-config.h>

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO
#include <mongoc/mongoc-crypto-common-crypto-private.h>

#include <CommonCrypto/CommonCryptoError.h>
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#include <CommonCrypto/CommonKeyDerivation.h>

// Ensure lossless conversion between `uint32_t` and `uint` below.
BSON_STATIC_ASSERT2 (sizeof_uint_uint32_t, sizeof (uint) == sizeof (uint32_t));

bool
mongoc_crypto_common_crypto_pbkdf2_hmac_sha1 (mongoc_crypto_t *crypto,
                                              const char *password,
                                              size_t password_len,
                                              const uint8_t *salt,
                                              size_t salt_len,
                                              uint32_t iterations,
                                              size_t output_len,
                                              unsigned char *output)
{
   BSON_UNUSED (crypto);

   return kCCSuccess == CCKeyDerivationPBKDF (kCCPBKDF2,
                                              password,
                                              password_len,
                                              salt,
                                              salt_len,
                                              kCCPRFHmacAlgSHA1,
                                              (uint) iterations,
                                              output,
                                              output_len);
}

void
mongoc_crypto_common_crypto_hmac_sha1 (mongoc_crypto_t *crypto,
                                       const void *key,
                                       int key_len,
                                       const unsigned char *data,
                                       int data_len,
                                       unsigned char *hmac_out)
{
   /* U1 = HMAC(input, salt + 0001) */
   CCHmac (kCCHmacAlgSHA1, key, (size_t) key_len, data, (size_t) data_len, hmac_out);
}

bool
mongoc_crypto_common_crypto_sha1 (mongoc_crypto_t *crypto,
                                  const unsigned char *input,
                                  const size_t input_len,
                                  unsigned char *hash_out)
{
   if (CC_SHA1 (input, (CC_LONG) input_len, hash_out)) {
      return true;
   }
   return false;
}

bool
mongoc_crypto_common_crypto_pbkdf2_hmac_sha256 (mongoc_crypto_t *crypto,
                                                const char *password,
                                                size_t password_len,
                                                const uint8_t *salt,
                                                size_t salt_len,
                                                uint32_t iterations,
                                                size_t output_len,
                                                unsigned char *output)
{
   BSON_UNUSED (crypto);

   return kCCSuccess == CCKeyDerivationPBKDF (kCCPBKDF2,
                                              password,
                                              password_len,
                                              salt,
                                              salt_len,
                                              kCCPRFHmacAlgSHA256,
                                              (uint) iterations,
                                              output,
                                              output_len);
}

void
mongoc_crypto_common_crypto_hmac_sha256 (mongoc_crypto_t *crypto,
                                         const void *key,
                                         int key_len,
                                         const unsigned char *data,
                                         int data_len,
                                         unsigned char *hmac_out)
{
   CCHmac (kCCHmacAlgSHA256, key, (size_t) key_len, data, (size_t) data_len, hmac_out);
}

bool
mongoc_crypto_common_crypto_sha256 (mongoc_crypto_t *crypto,
                                    const unsigned char *input,
                                    const size_t input_len,
                                    unsigned char *hash_out)
{
   if (CC_SHA256 (input, (CC_LONG) input_len, hash_out)) {
      return true;
   }
   return false;
}
#endif
