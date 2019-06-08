/* Copyright 2014 MongoDB, Inc.
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

#include "mongoc/mongoc-config.h"

#ifdef MONGOC_ENABLE_CRYPTO

#include <string.h>

#include "mongoc/mongoc-error.h"
#include "mongoc/mongoc-scram-private.h"
#include "mongoc/mongoc-rand-private.h"
#include "mongoc/mongoc-util-private.h"
#include "mongoc/mongoc-trace-private.h"

#include "mongoc/mongoc-crypto-private.h"
#include "common-b64-private.h"

#include "mongoc/mongoc-memcmp-private.h"

#define MONGOC_SCRAM_SERVER_KEY "Server Key"
#define MONGOC_SCRAM_CLIENT_KEY "Client Key"

static int
_scram_hash_size (mongoc_scram_t *scram)
{
   if (scram->crypto.algorithm == MONGOC_CRYPTO_ALGORITHM_SHA_1) {
      return MONGOC_SCRAM_SHA_1_HASH_SIZE;
   } else if (scram->crypto.algorithm == MONGOC_CRYPTO_ALGORITHM_SHA_256) {
      return MONGOC_SCRAM_SHA_256_HASH_SIZE;
   }
   return 0;
}

/* Copies the cache's secrets to scram */
static void
_mongoc_scram_cache_apply_secrets (mongoc_scram_cache_t *cache,
                                   mongoc_scram_t *scram)
{
   BSON_ASSERT (cache);
   BSON_ASSERT (scram);

   memcpy (scram->client_key, cache->client_key, sizeof (scram->client_key));
   memcpy (scram->server_key, cache->server_key, sizeof (scram->server_key));
   memcpy (scram->salted_password,
           cache->salted_password,
           sizeof (scram->salted_password));
}


static mongoc_scram_cache_t *
_mongoc_scram_cache_copy (const mongoc_scram_cache_t *cache)
{
   mongoc_scram_cache_t *ret = NULL;

   if (cache) {
      ret = (mongoc_scram_cache_t *) bson_malloc0 (sizeof (*ret));
      ret->hashed_password = bson_strdup (cache->hashed_password);
      memcpy (
         ret->decoded_salt, cache->decoded_salt, sizeof (ret->decoded_salt));
      ret->iterations = cache->iterations;
      memcpy (ret->client_key, cache->client_key, sizeof (ret->client_key));
      memcpy (ret->server_key, cache->server_key, sizeof (ret->server_key));
      memcpy (ret->salted_password,
              cache->salted_password,
              sizeof (ret->salted_password));
   }

   return ret;
}

#ifdef MONGOC_ENABLE_ICU
#include <unicode/usprep.h>
#include <unicode/ustring.h>
#endif


void
_mongoc_scram_cache_destroy (mongoc_scram_cache_t *cache)
{
   BSON_ASSERT (cache);

   if (cache->hashed_password) {
      bson_zero_free (cache->hashed_password, strlen (cache->hashed_password));
   }

   bson_free (cache);
}


/* Checks whether the cache contains scram's pre-secrets */
static bool
_mongoc_scram_cache_has_presecrets (mongoc_scram_cache_t *cache,
                                    mongoc_scram_t *scram)
{
   BSON_ASSERT (cache);
   BSON_ASSERT (scram);

   return cache->hashed_password && scram->hashed_password &&
          !strcmp (cache->hashed_password, scram->hashed_password) &&
          cache->iterations == scram->iterations &&
          !memcmp (cache->decoded_salt,
                   scram->decoded_salt,
                   sizeof (cache->decoded_salt));
}


mongoc_scram_cache_t *
_mongoc_scram_get_cache (mongoc_scram_t *scram)
{
   BSON_ASSERT (scram);

   return _mongoc_scram_cache_copy (scram->cache);
}


void
_mongoc_scram_set_cache (mongoc_scram_t *scram, mongoc_scram_cache_t *cache)
{
   BSON_ASSERT (scram);

   if (scram->cache) {
      _mongoc_scram_cache_destroy (scram->cache);
   }

   scram->cache = _mongoc_scram_cache_copy (cache);
}


void
_mongoc_scram_set_pass (mongoc_scram_t *scram, const char *pass)
{
   BSON_ASSERT (scram);

   if (scram->pass) {
      bson_zero_free (scram->pass, strlen (scram->pass));
   }

   scram->pass = pass ? bson_strdup (pass) : NULL;
}


void
_mongoc_scram_set_user (mongoc_scram_t *scram, const char *user)
{
   BSON_ASSERT (scram);

   bson_free (scram->user);
   scram->user = user ? bson_strdup (user) : NULL;
}


void
_mongoc_scram_init (mongoc_scram_t *scram, mongoc_crypto_hash_algorithm_t algo)
{
   BSON_ASSERT (scram);

   memset (scram, 0, sizeof *scram);

   mongoc_crypto_init (&scram->crypto, algo);
}


void
_mongoc_scram_destroy (mongoc_scram_t *scram)
{
   BSON_ASSERT (scram);

   bson_free (scram->user);

   if (scram->pass) {
      bson_zero_free (scram->pass, strlen (scram->pass));
   }

   if (scram->hashed_password) {
      bson_zero_free (scram->hashed_password, strlen (scram->hashed_password));
   }

   bson_free (scram->auth_message);

   if (scram->cache) {
      _mongoc_scram_cache_destroy (scram->cache);
   }
}


/* Updates the cache with scram's last-used pre-secrets and secrets */
static void
_mongoc_scram_update_cache (mongoc_scram_t *scram)
{
   mongoc_scram_cache_t *cache;

   BSON_ASSERT (scram);

   if (scram->cache) {
      _mongoc_scram_cache_destroy (scram->cache);
   }

   cache = (mongoc_scram_cache_t *) bson_malloc0 (sizeof (*cache));
   cache->hashed_password = bson_strdup (scram->hashed_password);
   memcpy (
      cache->decoded_salt, scram->decoded_salt, sizeof (cache->decoded_salt));
   cache->iterations = scram->iterations;
   memcpy (cache->client_key, scram->client_key, sizeof (cache->client_key));
   memcpy (cache->server_key, scram->server_key, sizeof (cache->server_key));
   memcpy (cache->salted_password,
           scram->salted_password,
           sizeof (cache->salted_password));

   scram->cache = cache;
}


static bool
_mongoc_scram_buf_write (const char *src,
                         int32_t src_len,
                         uint8_t *outbuf,
                         uint32_t outbufmax,
                         uint32_t *outbuflen)
{
   if (src_len < 0) {
      src_len = (int32_t) strlen (src);
   }

   if (*outbuflen + src_len >= outbufmax) {
      return false;
   }

   memcpy (outbuf + *outbuflen, src, src_len);

   *outbuflen += src_len;

   return true;
}


/* generate client-first-message:
 * n,a=authzid,n=encoded-username,r=client-nonce
 *
 * note that a= is optional, so we aren't dealing with that here
 */
static bool
_mongoc_scram_start (mongoc_scram_t *scram,
                     uint8_t *outbuf,
                     uint32_t outbufmax,
                     uint32_t *outbuflen,
                     bson_error_t *error)
{
   uint8_t nonce[24];
   const char *ptr;
   bool rval = true;

   BSON_ASSERT (scram);
   BSON_ASSERT (outbuf);
   BSON_ASSERT (outbufmax);
   BSON_ASSERT (outbuflen);

   if (!scram->user) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: username is not set");
      goto FAIL;
   }

   /* auth message is as big as the outbuf just because */
   scram->auth_message = (uint8_t *) bson_malloc (outbufmax);
   scram->auth_messagemax = outbufmax;

   /* the server uses a 24 byte random nonce.  so we do as well */
   if (1 != _mongoc_rand_bytes (nonce, sizeof (nonce))) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: could not generate a cryptographically "
                      "secure nonce in sasl step 1");
      goto FAIL;
   }

   scram->encoded_nonce_len = bson_b64_ntop (nonce,
                                             sizeof (nonce),
                                             scram->encoded_nonce,
                                             sizeof (scram->encoded_nonce));

   if (-1 == scram->encoded_nonce_len) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: could not encode nonce");
      goto FAIL;
   }

   if (!_mongoc_scram_buf_write ("n,,n=", -1, outbuf, outbufmax, outbuflen)) {
      goto BUFFER;
   }

   for (ptr = scram->user; *ptr; ptr++) {
      /* RFC 5802 specifies that ',' and '=' and encoded as '=2C' and '=3D'
       * respectively in the user name */
      switch (*ptr) {
      case ',':

         if (!_mongoc_scram_buf_write (
                "=2C", -1, outbuf, outbufmax, outbuflen)) {
            goto BUFFER;
         }

         break;
      case '=':

         if (!_mongoc_scram_buf_write (
                "=3D", -1, outbuf, outbufmax, outbuflen)) {
            goto BUFFER;
         }

         break;
      default:

         if (!_mongoc_scram_buf_write (ptr, 1, outbuf, outbufmax, outbuflen)) {
            goto BUFFER;
         }

         break;
      }
   }

   if (!_mongoc_scram_buf_write (",r=", -1, outbuf, outbufmax, outbuflen)) {
      goto BUFFER;
   }

   if (!_mongoc_scram_buf_write (scram->encoded_nonce,
                                 scram->encoded_nonce_len,
                                 outbuf,
                                 outbufmax,
                                 outbuflen)) {
      goto BUFFER;
   }

   /* we have to keep track of the conversation to create a client proof later
    * on.  This copies the message we're crafting from the 'n=' portion onwards
    * into a buffer we're managing */
   if (!_mongoc_scram_buf_write ((char *) outbuf + 3,
                                 *outbuflen - 3,
                                 scram->auth_message,
                                 scram->auth_messagemax,
                                 &scram->auth_messagelen)) {
      goto BUFFER_AUTH;
   }

   if (!_mongoc_scram_buf_write (",",
                                 -1,
                                 scram->auth_message,
                                 scram->auth_messagemax,
                                 &scram->auth_messagelen)) {
      goto BUFFER_AUTH;
   }

   goto CLEANUP;

BUFFER_AUTH:
   bson_set_error (
      error,
      MONGOC_ERROR_SCRAM,
      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
      "SCRAM Failure: could not buffer auth message in sasl step1");

   goto FAIL;

BUFFER:
   bson_set_error (error,
                   MONGOC_ERROR_SCRAM,
                   MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                   "SCRAM Failure: could not buffer sasl step1");

   goto FAIL;

FAIL:
   rval = false;

CLEANUP:

   return rval;
}


/* Compute the SCRAM step Hi() as defined in RFC5802 */
static void
_mongoc_scram_salt_password (mongoc_scram_t *scram,
                             const char *password,
                             uint32_t password_len,
                             const uint8_t *salt,
                             uint32_t salt_len,
                             uint32_t iterations)
{
   uint8_t intermediate_digest[MONGOC_SCRAM_HASH_MAX_SIZE];
   uint8_t start_key[MONGOC_SCRAM_HASH_MAX_SIZE];

   int i;
   int k;
   uint8_t *output = scram->salted_password;

   memcpy (start_key, salt, salt_len);

   start_key[salt_len] = 0;
   start_key[salt_len + 1] = 0;
   start_key[salt_len + 2] = 0;
   start_key[salt_len + 3] = 1;

   mongoc_crypto_hmac (&scram->crypto,
                       password,
                       password_len,
                       start_key,
                       _scram_hash_size (scram),
                       output);

   memcpy (intermediate_digest, output, _scram_hash_size (scram));

   /* intermediateDigest contains Ui and output contains the accumulated XOR:ed
    * result */
   for (i = 2; i <= iterations; i++) {
      mongoc_crypto_hmac (&scram->crypto,
                          password,
                          password_len,
                          intermediate_digest,
                          _scram_hash_size (scram),
                          intermediate_digest);

      for (k = 0; k < _scram_hash_size (scram); k++) {
         output[k] ^= intermediate_digest[k];
      }
   }
}


static bool
_mongoc_scram_generate_client_proof (mongoc_scram_t *scram,
                                     uint8_t *outbuf,
                                     uint32_t outbufmax,
                                     uint32_t *outbuflen)
{
   uint8_t stored_key[MONGOC_SCRAM_HASH_MAX_SIZE];
   uint8_t client_signature[MONGOC_SCRAM_HASH_MAX_SIZE];
   unsigned char client_proof[MONGOC_SCRAM_HASH_MAX_SIZE];
   int i;
   int r = 0;

   if (!*scram->client_key) {
      /* ClientKey := HMAC(saltedPassword, "Client Key") */
      mongoc_crypto_hmac (&scram->crypto,
                          scram->salted_password,
                          _scram_hash_size (scram),
                          (uint8_t *) MONGOC_SCRAM_CLIENT_KEY,
                          (int) strlen (MONGOC_SCRAM_CLIENT_KEY),
                          scram->client_key);
   }

   /* StoredKey := H(client_key) */
   mongoc_crypto_hash (&scram->crypto,
                       scram->client_key,
                       (size_t) _scram_hash_size (scram),
                       stored_key);

   /* ClientSignature := HMAC(StoredKey, AuthMessage) */
   mongoc_crypto_hmac (&scram->crypto,
                       stored_key,
                       _scram_hash_size (scram),
                       scram->auth_message,
                       scram->auth_messagelen,
                       client_signature);

   /* ClientProof := ClientKey XOR ClientSignature */

   for (i = 0; i < _scram_hash_size (scram); i++) {
      client_proof[i] = scram->client_key[i] ^ client_signature[i];
   }

   r = bson_b64_ntop (client_proof,
                      _scram_hash_size (scram),
                      (char *) outbuf + *outbuflen,
                      outbufmax - *outbuflen);

   if (-1 == r) {
      return false;
   }

   *outbuflen += r;

   return true;
}


/* Parse server-first-message of the form:
 * r=client-nonce|server-nonce,s=user-salt,i=iteration-count
 *
 * Generate client-final-message of the form:
 * c=channel-binding(base64),r=client-nonce|server-nonce,p=client-proof
 */
static bool
_mongoc_scram_step2 (mongoc_scram_t *scram,
                     const uint8_t *inbuf,
                     uint32_t inbuflen,
                     uint8_t *outbuf,
                     uint32_t outbufmax,
                     uint32_t *outbuflen,
                     bson_error_t *error)
{
   uint8_t *val_r = NULL;
   uint32_t val_r_len;
   uint8_t *val_s = NULL;
   uint32_t val_s_len;
   uint8_t *val_i = NULL;
   uint32_t val_i_len;

   uint8_t **current_val;
   uint32_t *current_val_len;

   const uint8_t *ptr;
   const uint8_t *next_comma;

   char *tmp;
   char *hashed_password;

   uint8_t decoded_salt[MONGOC_SCRAM_B64_HASH_MAX_SIZE] = {0};
   int32_t decoded_salt_len;
   /* the decoded salt leaves four trailing bytes to add the int32 0x00000001 */
   const int32_t expected_salt_length = _scram_hash_size (scram) - 4;
   bool rval = true;

   int iterations;


   BSON_ASSERT (scram);
   BSON_ASSERT (outbuf);
   BSON_ASSERT (outbufmax);
   BSON_ASSERT (outbuflen);

   if (scram->crypto.algorithm == MONGOC_CRYPTO_ALGORITHM_SHA_1) {
      /* Auth spec for SCRAM-SHA-1: "The password variable MUST be the mongodb
       * hashed variant. The mongo hashed variant is computed as hash = HEX(
       * MD5( UTF8( username + ':mongo:' + plain_text_password )))" */
      tmp = bson_strdup_printf ("%s:mongo:%s", scram->user, scram->pass);
      hashed_password = _mongoc_hex_md5 (tmp);
      bson_zero_free (tmp, strlen (tmp));
   } else if (scram->crypto.algorithm == MONGOC_CRYPTO_ALGORITHM_SHA_256) {
      /* Auth spec for SCRAM-SHA-256: "Passwords MUST be prepared with SASLprep,
       * per RFC 5802. Passwords are used directly for key derivation; they
       * MUST NOT be digested as they are in SCRAM-SHA-1." */
      hashed_password =
         _mongoc_sasl_prep (scram->pass, (int) strlen (scram->pass), error);
      if (!hashed_password) {
         goto FAIL;
      }
   } else {
      BSON_ASSERT (false);
   }

   /* we need all of the incoming message for the final client proof */
   if (!_mongoc_scram_buf_write ((char *) inbuf,
                                 inbuflen,
                                 scram->auth_message,
                                 scram->auth_messagemax,
                                 &scram->auth_messagelen)) {
      goto BUFFER_AUTH;
   }

   if (!_mongoc_scram_buf_write (",",
                                 -1,
                                 scram->auth_message,
                                 scram->auth_messagemax,
                                 &scram->auth_messagelen)) {
      goto BUFFER_AUTH;
   }

   for (ptr = inbuf; ptr < inbuf + inbuflen;) {
      switch (*ptr) {
      case 'r':
         current_val = &val_r;
         current_val_len = &val_r_len;
         break;
      case 's':
         current_val = &val_s;
         current_val_len = &val_s_len;
         break;
      case 'i':
         current_val = &val_i;
         current_val_len = &val_i_len;
         break;
      default:
         bson_set_error (error,
                         MONGOC_ERROR_SCRAM,
                         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                         "SCRAM Failure: unknown key (%c) in sasl step 2",
                         *ptr);
         goto FAIL;
      }

      ptr++;

      if (*ptr != '=') {
         bson_set_error (error,
                         MONGOC_ERROR_SCRAM,
                         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                         "SCRAM Failure: invalid parse state in sasl step 2");

         goto FAIL;
      }

      ptr++;

      next_comma =
         (const uint8_t *) memchr (ptr, ',', (inbuf + inbuflen) - ptr);

      if (next_comma) {
         *current_val_len = (uint32_t) (next_comma - ptr);
      } else {
         *current_val_len = (uint32_t) ((inbuf + inbuflen) - ptr);
      }

      *current_val = (uint8_t *) bson_malloc (*current_val_len + 1);
      memcpy (*current_val, ptr, *current_val_len);
      (*current_val)[*current_val_len] = '\0';

      if (next_comma) {
         ptr = next_comma + 1;
      } else {
         break;
      }
   }

   if (!val_r) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: no r param in sasl step 2");

      goto FAIL;
   }

   if (!val_s) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: no s param in sasl step 2");

      goto FAIL;
   }

   if (!val_i) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: no i param in sasl step 2");

      goto FAIL;
   }

   /* verify our nonce */
   if (val_r_len < scram->encoded_nonce_len ||
       mongoc_memcmp (val_r, scram->encoded_nonce, scram->encoded_nonce_len)) {
      bson_set_error (
         error,
         MONGOC_ERROR_SCRAM,
         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
         "SCRAM Failure: client nonce not repeated in sasl step 2");
   }

   *outbuflen = 0;

   if (!_mongoc_scram_buf_write (
          "c=biws,r=", -1, outbuf, outbufmax, outbuflen)) {
      goto BUFFER;
   }

   if (!_mongoc_scram_buf_write (
          (char *) val_r, val_r_len, outbuf, outbufmax, outbuflen)) {
      goto BUFFER;
   }

   if (!_mongoc_scram_buf_write ((char *) outbuf,
                                 *outbuflen,
                                 scram->auth_message,
                                 scram->auth_messagemax,
                                 &scram->auth_messagelen)) {
      goto BUFFER_AUTH;
   }

   if (!_mongoc_scram_buf_write (",p=", -1, outbuf, outbufmax, outbuflen)) {
      goto BUFFER;
   }

   decoded_salt_len =
      bson_b64_pton ((char *) val_s, decoded_salt, sizeof (decoded_salt));

   if (-1 == decoded_salt_len) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: unable to decode salt in sasl step2");
      goto FAIL;
   }

   if (expected_salt_length != decoded_salt_len) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: invalid salt length of %d in sasl step2",
                      decoded_salt_len);
      goto FAIL;
   }

   iterations = (int) bson_ascii_strtoll ((char *) val_i, &tmp, 10);
   /* tmp holds the location of the failed to parse character.  So if it's
    * null, we got to the end of the string and didn't have a parse error */

   if (*tmp) {
      bson_set_error (
         error,
         MONGOC_ERROR_SCRAM,
         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
         "SCRAM Failure: unable to parse iterations in sasl step2");
      goto FAIL;
   }

   if (iterations < 0) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: iterations is negative in sasl step2");
      goto FAIL;
   }

   /* drivers MUST enforce a minimum iteration count of 4096 and MUST error if
    * the authentication conversation specifies a lower count. This mitigates
    * downgrade attacks by a man-in-the-middle attacker. */
   if (iterations < 4096) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: iterations must be at least 4096");
      goto FAIL;
   }

   /* Save the presecrets for caching */
   scram->hashed_password = bson_strdup (hashed_password);
   scram->iterations = iterations;
   memcpy (scram->decoded_salt, decoded_salt, sizeof (scram->decoded_salt));

   if (scram->cache &&
       _mongoc_scram_cache_has_presecrets (scram->cache, scram)) {
      _mongoc_scram_cache_apply_secrets (scram->cache, scram);
   }

   if (!*scram->salted_password) {
      _mongoc_scram_salt_password (scram,
                                   hashed_password,
                                   (uint32_t) strlen (hashed_password),
                                   decoded_salt,
                                   decoded_salt_len,
                                   (uint32_t) iterations);
   }

   _mongoc_scram_generate_client_proof (scram, outbuf, outbufmax, outbuflen);

   goto CLEANUP;

BUFFER_AUTH:
   bson_set_error (
      error,
      MONGOC_ERROR_SCRAM,
      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
      "SCRAM Failure: could not buffer auth message in sasl step2");

   goto FAIL;

BUFFER:
   bson_set_error (error,
                   MONGOC_ERROR_SCRAM,
                   MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                   "SCRAM Failure: could not buffer sasl step2");

   goto FAIL;

FAIL:
   rval = false;

CLEANUP:
   bson_free (val_r);
   bson_free (val_s);
   bson_free (val_i);

   if (hashed_password) {
      bson_zero_free (hashed_password, strlen (hashed_password));
   }

   return rval;
}


static bool
_mongoc_scram_verify_server_signature (mongoc_scram_t *scram,
                                       uint8_t *verification,
                                       uint32_t len)
{
   char encoded_server_signature[MONGOC_SCRAM_B64_HASH_MAX_SIZE];
   int32_t encoded_server_signature_len;
   uint8_t server_signature[MONGOC_SCRAM_HASH_MAX_SIZE];

   if (!*scram->server_key) {
      /* ServerKey := HMAC(SaltedPassword, "Server Key") */
      mongoc_crypto_hmac (&scram->crypto,
                          scram->salted_password,
                          _scram_hash_size (scram),
                          (uint8_t *) MONGOC_SCRAM_SERVER_KEY,
                          strlen (MONGOC_SCRAM_SERVER_KEY),
                          scram->server_key);
   }

   /* ServerSignature := HMAC(ServerKey, AuthMessage) */
   mongoc_crypto_hmac (&scram->crypto,
                       scram->server_key,
                       _scram_hash_size (scram),
                       scram->auth_message,
                       scram->auth_messagelen,
                       server_signature);

   encoded_server_signature_len =
      bson_b64_ntop (server_signature,
                     _scram_hash_size (scram),
                     encoded_server_signature,
                     sizeof (encoded_server_signature));
   if (encoded_server_signature_len == -1) {
      return false;
   }

   return (len == encoded_server_signature_len) &&
          (mongoc_memcmp (verification, encoded_server_signature, len) == 0);
}


static bool
_mongoc_scram_step3 (mongoc_scram_t *scram,
                     const uint8_t *inbuf,
                     uint32_t inbuflen,
                     uint8_t *outbuf,
                     uint32_t outbufmax,
                     uint32_t *outbuflen,
                     bson_error_t *error)
{
   uint8_t *val_e = NULL;
   uint32_t val_e_len;
   uint8_t *val_v = NULL;
   uint32_t val_v_len;

   uint8_t **current_val;
   uint32_t *current_val_len;

   const uint8_t *ptr;
   const uint8_t *next_comma;

   bool rval = true;

   BSON_ASSERT (scram);
   BSON_ASSERT (outbuf);
   BSON_ASSERT (outbufmax);
   BSON_ASSERT (outbuflen);

   for (ptr = inbuf; ptr < inbuf + inbuflen;) {
      switch (*ptr) {
      case 'e':
         current_val = &val_e;
         current_val_len = &val_e_len;
         break;
      case 'v':
         current_val = &val_v;
         current_val_len = &val_v_len;
         break;
      default:
         bson_set_error (error,
                         MONGOC_ERROR_SCRAM,
                         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                         "SCRAM Failure: unknown key (%c) in sasl step 3",
                         *ptr);
         goto FAIL;
      }

      ptr++;

      if (*ptr != '=') {
         bson_set_error (error,
                         MONGOC_ERROR_SCRAM,
                         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                         "SCRAM Failure: invalid parse state in sasl step 3");
         goto FAIL;
      }

      ptr++;

      next_comma =
         (const uint8_t *) memchr (ptr, ',', (inbuf + inbuflen) - ptr);

      if (next_comma) {
         *current_val_len = (uint32_t) (next_comma - ptr);
      } else {
         *current_val_len = (uint32_t) ((inbuf + inbuflen) - ptr);
      }

      *current_val = (uint8_t *) bson_malloc (*current_val_len + 1);
      memcpy (*current_val, ptr, *current_val_len);
      (*current_val)[*current_val_len] = '\0';

      if (next_comma) {
         ptr = next_comma + 1;
      } else {
         break;
      }
   }

   *outbuflen = 0;

   if (val_e) {
      bson_set_error (
         error,
         MONGOC_ERROR_SCRAM,
         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
         "SCRAM Failure: authentication failure in sasl step 3 : %s",
         val_e);
      goto FAIL;
   }

   if (!val_v) {
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: no v param in sasl step 3");
      goto FAIL;
   }

   if (!_mongoc_scram_verify_server_signature (scram, val_v, val_v_len)) {
      bson_set_error (
         error,
         MONGOC_ERROR_SCRAM,
         MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
         "SCRAM Failure: could not verify server signature in sasl step 3");
      goto FAIL;
   }

   /* Update the cache if authentication succeeds */
   _mongoc_scram_update_cache (scram);

   goto CLEANUP;

FAIL:
   rval = false;

CLEANUP:
   bson_free (val_e);
   bson_free (val_v);

   return rval;
}


bool
_mongoc_scram_step (mongoc_scram_t *scram,
                    const uint8_t *inbuf,
                    uint32_t inbuflen,
                    uint8_t *outbuf,
                    uint32_t outbufmax,
                    uint32_t *outbuflen,
                    bson_error_t *error)
{
   BSON_ASSERT (scram);
   BSON_ASSERT (inbuf);
   BSON_ASSERT (outbuf);
   BSON_ASSERT (outbuflen);

   scram->step++;

   switch (scram->step) {
   case 1:
      return _mongoc_scram_start (scram, outbuf, outbufmax, outbuflen, error);
   case 2:
      return _mongoc_scram_step2 (
         scram, inbuf, inbuflen, outbuf, outbufmax, outbuflen, error);
   case 3:
      return _mongoc_scram_step3 (
         scram, inbuf, inbuflen, outbuf, outbufmax, outbuflen, error);
   default:
      bson_set_error (error,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_NOT_DONE,
                      "SCRAM Failure: maximum steps detected");
      return false;
   }
}

bool
_mongoc_sasl_prep_required (const char *str)
{
   unsigned char c;
   while (*str) {
      c = (unsigned char) *str;
      /* characters below 32 contain all of the control characters.
       * characters above 127 are multibyte UTF-8 characters.
       * character 127 is the DEL character. */
      if (c < 32 || c >= 127) {
         return true;
      }
      str++;
   }
   return false;
}

#ifdef MONGOC_ENABLE_ICU
char *
_mongoc_sasl_prep_impl (const char *name,
                        const char *in_utf8,
                        int in_utf8_len,
                        bson_error_t *err)
{
   /* The flow is in_utf8 -> in_utf16 -> SASLPrep -> out_utf16 -> out_utf8. */
   UChar *in_utf16, *out_utf16;
   char *out_utf8;
   int32_t in_utf16_len, out_utf16_len, out_utf8_len;
   UErrorCode error_code = U_ZERO_ERROR;
   UStringPrepProfile *prep;

#define SASL_PREP_ERR_RETURN(msg)                        \
   do {                                                  \
      bson_set_error (err,                               \
                      MONGOC_ERROR_SCRAM,                \
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR, \
                      (msg),                             \
                      name);                             \
      return NULL;                                       \
   } while (0)

   /* 1. convert str to UTF-16. */
   /* preflight to get the destination length. */
   (void) u_strFromUTF8 (
      NULL, 0, &in_utf16_len, in_utf8, in_utf8_len, &error_code);
   if (error_code != U_BUFFER_OVERFLOW_ERROR) {
      SASL_PREP_ERR_RETURN ("could not calculate UTF-16 length of %s");
   }

   /* convert to UTF-16. */
   error_code = U_ZERO_ERROR;
   in_utf16 = bson_malloc (sizeof (UChar) *
                           (in_utf16_len + 1)); /* add one for null byte. */
   (void) u_strFromUTF8 (
      in_utf16, in_utf16_len + 1, NULL, in_utf8, in_utf8_len, &error_code);
   if (error_code) {
      bson_free (in_utf16);
      SASL_PREP_ERR_RETURN ("could not convert %s to UTF-16");
   }

   /* 2. perform SASLPrep. */
   prep = usprep_openByType (USPREP_RFC4013_SASLPREP, &error_code);
   if (error_code) {
      bson_free (in_utf16);
      SASL_PREP_ERR_RETURN ("could not start SASLPrep for %s");
   }
   /* preflight. */
   out_utf16_len = usprep_prepare (
      prep, in_utf16, in_utf16_len, NULL, 0, USPREP_DEFAULT, NULL, &error_code);
   if (error_code != U_BUFFER_OVERFLOW_ERROR) {
      bson_free (in_utf16);
      usprep_close (prep);
      SASL_PREP_ERR_RETURN ("could not calculate SASLPrep length of %s");
   }

   /* convert. */
   error_code = U_ZERO_ERROR;
   out_utf16 = bson_malloc (sizeof (UChar) * (out_utf16_len + 1));
   (void) usprep_prepare (prep,
                          in_utf16,
                          in_utf16_len,
                          out_utf16,
                          out_utf16_len + 1,
                          USPREP_DEFAULT,
                          NULL,
                          &error_code);
   if (error_code) {
      bson_free (in_utf16);
      bson_free (out_utf16);
      usprep_close (prep);
      SASL_PREP_ERR_RETURN ("could not execute SASLPrep for %s");
   }
   bson_free (in_utf16);
   usprep_close (prep);

   /* 3. convert back to UTF-8. */
   /* preflight. */
   (void) u_strToUTF8 (
      NULL, 0, &out_utf8_len, out_utf16, out_utf16_len, &error_code);
   if (error_code != U_BUFFER_OVERFLOW_ERROR) {
      bson_free (out_utf16);
      SASL_PREP_ERR_RETURN ("could not calculate UTF-8 length of %s");
   }

   /* convert. */
   error_code = U_ZERO_ERROR;
   out_utf8 = (char *) bson_malloc (
      sizeof (char) * (out_utf8_len + 1)); /* add one for null byte. */
   (void) u_strToUTF8 (
      out_utf8, out_utf8_len + 1, NULL, out_utf16, out_utf16_len, &error_code);
   if (error_code) {
      bson_free (out_utf8);
      bson_free (out_utf16);
      SASL_PREP_ERR_RETURN ("could not convert %s back to UTF-8");
   }
   bson_free (out_utf16);
   return out_utf8;
#undef SASL_PREP_ERR_RETURN
}
#endif

char *
_mongoc_sasl_prep (const char *in_utf8, int in_utf8_len, bson_error_t *err)
{
#ifdef MONGOC_ENABLE_ICU
   return _mongoc_sasl_prep_impl ("password", in_utf8, in_utf8_len, err);
#else
   if (_mongoc_sasl_prep_required (in_utf8)) {
      bson_set_error (err,
                      MONGOC_ERROR_SCRAM,
                      MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                      "SCRAM Failure: ICU required to SASLPrep password");
      return NULL;
   }
   return bson_strdup (in_utf8);
#endif
}
#endif
