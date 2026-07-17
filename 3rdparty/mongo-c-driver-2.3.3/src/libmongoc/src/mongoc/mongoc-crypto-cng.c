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

#include <mongoc/mongoc-config.h>

#ifdef MONGOC_ENABLE_CRYPTO_CNG
#include <mongoc/mongoc-crypto-cng-private.h>
#include <mongoc/mongoc-crypto-private.h>
#include <mongoc/mongoc-scram-private.h>
#include <mongoc/mongoc-thread-private.h>

#include <mongoc/mongoc-log.h>

#include <mlib/cmp.h>

#include <bcrypt.h>

#include <windows.h>

#include <stdio.h>
#include <string.h>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)

static BCRYPT_ALG_HANDLE _sha1_hash_algo;
static BCRYPT_ALG_HANDLE _sha1_hmac_algo;
static BCRYPT_ALG_HANDLE _sha256_hash_algo;
static BCRYPT_ALG_HANDLE _sha256_hmac_algo;

void
mongoc_crypto_cng_init(void)
{
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   _sha1_hash_algo = 0;
   status = BCryptOpenAlgorithmProvider(&_sha1_hash_algo, BCRYPT_SHA1_ALGORITHM, NULL, 0);
   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptOpenAlgorithmProvider(SHA1): %ld", status);
   }

   _sha1_hmac_algo = 0;
   status = BCryptOpenAlgorithmProvider(&_sha1_hmac_algo, BCRYPT_SHA1_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptOpenAlgorithmProvider(SHA1 HMAC): %ld", status);
   }

   _sha256_hash_algo = 0;
   status = BCryptOpenAlgorithmProvider(&_sha256_hash_algo, BCRYPT_SHA256_ALGORITHM, NULL, 0);
   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptOpenAlgorithmProvider(SHA256): %ld", status);
   }

   _sha256_hmac_algo = 0;
   status = BCryptOpenAlgorithmProvider(&_sha256_hmac_algo, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptOpenAlgorithmProvider(SHA256 HMAC): %ld", status);
   }
}

void
mongoc_crypto_cng_cleanup(void)
{
   if (_sha1_hash_algo) {
      BCryptCloseAlgorithmProvider(&_sha1_hash_algo, 0);
   }
   if (_sha1_hmac_algo) {
      BCryptCloseAlgorithmProvider(&_sha1_hmac_algo, 0);
   }
   if (_sha256_hash_algo) {
      BCryptCloseAlgorithmProvider(&_sha256_hash_algo, 0);
   }
   if (_sha256_hmac_algo) {
      BCryptCloseAlgorithmProvider(&_sha256_hmac_algo, 0);
   }
}

bool
_mongoc_crypto_cng_hmac_or_hash(
   BCRYPT_ALG_HANDLE algorithm, const void *key, size_t key_length, void *data, size_t data_length, void *output)
{
   unsigned char *hash_object_buffer = 0;
   ULONG hash_object_length = 0;
   BCRYPT_HASH_HANDLE hash = 0;
   ULONG mac_length = 0;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   bool retval = false;
   ULONG noop = 0;

   status = BCryptGetProperty(
      algorithm, BCRYPT_OBJECT_LENGTH, (unsigned char *)&hash_object_length, sizeof hash_object_length, &noop, 0);

   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptGetProperty(): OBJECT_LENGTH %ld", status);
      return false;
   }

   status = BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH, (unsigned char *)&mac_length, sizeof mac_length, &noop, 0);

   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptGetProperty(): HASH_LENGTH %ld", status);
      return false;
   }

   hash_object_buffer = bson_malloc(hash_object_length);

   status =
      BCryptCreateHash(algorithm, &hash, hash_object_buffer, hash_object_length, (PUCHAR)key, (ULONG)key_length, 0);

   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptCreateHash(): %ld", status);
      goto cleanup;
   }

   status = BCryptHashData(hash, data, (ULONG)data_length, 0);
   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptHashData(): %ld", status);
      goto cleanup;
   }

   status = BCryptFinishHash(hash, output, mac_length, 0);
   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("BCryptFinishHash(): %ld", status);
      goto cleanup;
   }

   retval = true;

cleanup:
   if (hash) {
      (void)BCryptDestroyHash(hash);
   }

   bson_free(hash_object_buffer);
   return retval;
}

#if defined(MONGOC_HAVE_BCRYPT_PBKDF2)
// Ensure lossless conversion between `uint64_t` and `ULONGLONG` below.
BSON_STATIC_ASSERT2(sizeof_ulonglong_uint64_t, sizeof(ULONGLONG) == sizeof(uint64_t));

/* Wrapper for BCryptDeriveKeyPBKDF2 */
static bool
_bcrypt_derive_key_pbkdf2(BCRYPT_ALG_HANDLE prf,
                          const char *password,
                          size_t password_len,
                          const uint8_t *salt,
                          size_t salt_len,
                          uint32_t iterations,
                          size_t output_len,
                          unsigned char *output)
{
   if (BSON_UNLIKELY(mlib_cmp(password_len, >, ULONG_MAX))) {
      MONGOC_ERROR("PBDKF2 HMAC password length exceeds ULONG_MAX");
      return false;
   }

   if (BSON_UNLIKELY(mlib_cmp(salt_len, >, ULONG_MAX))) {
      MONGOC_ERROR("PBDKF2 HMAC salt length exceeds ULONG_MAX");
      return false;
   }

   // `(ULONGLONG) iterations` is statically asserted above.

   if (BSON_UNLIKELY(mlib_cmp(output_len, >, ULONG_MAX))) {
      MONGOC_ERROR("PBDKF2 HMAC output length exceeds ULONG_MAX");
      return false;
   }

   // Make non-const versions of password and salt.
   unsigned char *password_copy = bson_malloc(password_len);
   memcpy(password_copy, password, password_len);
   unsigned char *salt_copy = bson_malloc(salt_len);
   memcpy(salt_copy, salt, salt_len);

   NTSTATUS status = BCryptDeriveKeyPBKDF2(prf,
                                           password_copy,
                                           (ULONG)password_len,
                                           salt_copy,
                                           (ULONG)salt_len,
                                           (ULONGLONG)iterations,
                                           output,
                                           (ULONG)output_len,
                                           0);
   bson_free(password_copy);
   bson_free(salt_copy);

   if (!NT_SUCCESS(status)) {
      MONGOC_ERROR("_bcrypt_derive_key_pbkdf2(): %ld", status);
      return false;
   }
   return true;
}

#else

static size_t
_crypto_hash_size(mongoc_crypto_t *crypto)
{
   if (crypto->algorithm == MONGOC_CRYPTO_ALGORITHM_SHA_1) {
      return MONGOC_SCRAM_SHA_1_HASH_SIZE;
   } else if (crypto->algorithm == MONGOC_CRYPTO_ALGORITHM_SHA_256) {
      return MONGOC_SCRAM_SHA_256_HASH_SIZE;
   } else {
      BSON_UNREACHABLE("Unexpected crypto algorithm");
   }
}

/* Manually salts password if BCryptDeriveKeyPBKDF2 is unavailable */
static bool
_bcrypt_derive_key_pbkdf2(BCRYPT_ALG_HANDLE algorithm,
                          const char *password,
                          size_t password_len,
                          const uint8_t *salt,
                          size_t salt_len,
                          uint32_t iterations,
                          size_t hash_size,
                          unsigned char *output)
{
   uint8_t intermediate_digest[MONGOC_SCRAM_HASH_MAX_SIZE];
   uint8_t start_key[MONGOC_SCRAM_HASH_MAX_SIZE];

   memcpy(start_key, salt, salt_len);
   start_key[salt_len] = 0;
   start_key[salt_len + 1] = 0;
   start_key[salt_len + 2] = 0;
   start_key[salt_len + 3] = 1;

   if (!_mongoc_crypto_cng_hmac_or_hash(algorithm, password, password_len, start_key, hash_size, output)) {
      return false;
   }
   memcpy(intermediate_digest, output, hash_size);

   for (uint32_t i = 2u; i <= iterations; i++) {
      if (!_mongoc_crypto_cng_hmac_or_hash(algorithm, password, password_len, intermediate_digest, hash_size, output)) {
         return false;
      }

      for (size_t k = 0; k < hash_size; k++) {
         output[k] ^= intermediate_digest[k];
      }
   }
   return true;
}
#endif

bool
mongoc_crypto_cng_pbkdf2_hmac_sha1(mongoc_crypto_t *crypto,
                                   const char *password,
                                   size_t password_len,
                                   const uint8_t *salt,
                                   size_t salt_len,
                                   uint32_t iterations,
                                   size_t output_len,
                                   unsigned char *output)
{
#if defined(MONGOC_HAVE_BCRYPT_PBKDF2)
   BSON_UNUSED(crypto);
   return _bcrypt_derive_key_pbkdf2(
      _sha1_hmac_algo, password, password_len, salt, salt_len, iterations, output_len, output);
#else
   return _bcrypt_derive_key_pbkdf2(
      _sha1_hmac_algo, password, password_len, salt, salt_len, iterations, _crypto_hash_size(crypto), output);
#endif
}

void
mongoc_crypto_cng_hmac_sha1(mongoc_crypto_t *crypto,
                            const void *key,
                            int key_len,
                            const unsigned char *data,
                            int data_len,
                            unsigned char *hmac_out)
{
   BSON_UNUSED(crypto);

   if (!_sha1_hmac_algo) {
      return;
   }

   _mongoc_crypto_cng_hmac_or_hash(_sha1_hmac_algo, key, key_len, (void *)data, data_len, hmac_out);
}

bool
mongoc_crypto_cng_sha1(mongoc_crypto_t *crypto,
                       const unsigned char *input,
                       const size_t input_len,
                       unsigned char *hash_out)
{
   BSON_UNUSED(crypto);

   bool res;

   if (!_sha1_hash_algo) {
      return false;
   }

   res = _mongoc_crypto_cng_hmac_or_hash(_sha1_hash_algo, NULL, 0, (void *)input, input_len, hash_out);
   return res;
}

bool
mongoc_crypto_cng_pbkdf2_hmac_sha256(mongoc_crypto_t *crypto,
                                     const char *password,
                                     size_t password_len,
                                     const uint8_t *salt,
                                     size_t salt_len,
                                     uint32_t iterations,
                                     size_t output_len,
                                     unsigned char *output)
{
#if defined(MONGOC_HAVE_BCRYPT_PBKDF2)
   BSON_UNUSED(crypto);
   return _bcrypt_derive_key_pbkdf2(
      _sha256_hmac_algo, password, password_len, salt, salt_len, iterations, output_len, output);
#else
   BSON_UNUSED(output_len);
   return _bcrypt_derive_key_pbkdf2(
      _sha256_hmac_algo, password, password_len, salt, salt_len, iterations, _crypto_hash_size(crypto), output);
#endif
}

void
mongoc_crypto_cng_hmac_sha256(mongoc_crypto_t *crypto,
                              const void *key,
                              int key_len,
                              const unsigned char *data,
                              int data_len,
                              unsigned char *hmac_out)
{
   BSON_UNUSED(crypto);

   if (!_sha256_hmac_algo) {
      return;
   }

   _mongoc_crypto_cng_hmac_or_hash(_sha256_hmac_algo, key, key_len, (void *)data, data_len, hmac_out);
}

bool
mongoc_crypto_cng_sha256(mongoc_crypto_t *crypto,
                         const unsigned char *input,
                         const size_t input_len,
                         unsigned char *hash_out)
{
   BSON_UNUSED(crypto);

   bool res;

   if (!_sha256_hash_algo) {
      return false;
   }

   res = _mongoc_crypto_cng_hmac_or_hash(_sha256_hash_algo, NULL, 0, (void *)input, input_len, hash_out);
   return res;
}
#endif
