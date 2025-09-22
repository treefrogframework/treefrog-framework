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

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL

#include <common-string-private.h>
#include <mongoc/mongoc-crypto-private.h> // mongoc_crypto_hash
#include <mongoc/mongoc-errno-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-secure-channel-private.h>
#include <mongoc/mongoc-stream-tls-private.h>
#include <mongoc/mongoc-stream-tls-secure-channel-private.h>
#include <mongoc/mongoc-trace-private.h>
#include <mongoc/mongoc-util-private.h> // bin_to_hex

#include <mongoc/mongoc-log.h>
#include <mongoc/mongoc-ssl.h>
#include <mongoc/mongoc-stream-tls.h>

#include <bson/bson.h>

#include <mlib/cmp.h>

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-secure-channel"

#ifdef __MINGW32__
// Define macros omitted from mingw headers:
#ifndef SECBUFFER_ALERT
#define SECBUFFER_ALERT 17
#endif
#ifndef NCRYPTBUFFER_VERSION
#define NCRYPTBUFFER_VERSION 0
#endif
#ifndef NCRYPT_PKCS8_PRIVATE_KEY_BLOB
#define NCRYPT_PKCS8_PRIVATE_KEY_BLOB L"PKCS8_PRIVATEKEY"
#endif
#ifndef NCRYPT_SILENT_FLAG
#define NCRYPT_SILENT_FLAG 0x00000040
#endif
#ifndef MS_KEY_STORAGE_PROVIDER
#define MS_KEY_STORAGE_PROVIDER L"Microsoft Software Key Storage Provider"
#endif
#endif // #ifdef __MINGW32__

// `decode_pem_base64` decodes a base-64 PEM blob with headers.
// Returns NULL on error.
static LPBYTE
decode_pem_base64 (const char *base64_in, DWORD *out_len, const char *descriptor, const char *filename)
{
   BSON_ASSERT_PARAM (base64_in);
   BSON_ASSERT_PARAM (out_len);
   BSON_ASSERT_PARAM (descriptor);
   BSON_ASSERT_PARAM (filename);

   // Get needed output length:
   if (!CryptStringToBinaryA (base64_in, 0, CRYPT_STRING_BASE64HEADER, NULL, out_len, NULL, NULL)) {
      MONGOC_ERROR (
         "Failed to convert base64 %s from '%s'. Error 0x%.8X", descriptor, filename, (unsigned int) GetLastError ());
      return NULL;
   }

   if (*out_len == 0) {
      return NULL;
   }

   LPBYTE out = (LPBYTE) bson_malloc (*out_len);

   if (!CryptStringToBinaryA (base64_in, 0, CRYPT_STRING_BASE64HEADER, out, out_len, NULL, NULL)) {
      MONGOC_ERROR (
         "Failed to convert base64 %s from '%s'. Error 0x%.8X", descriptor, filename, (unsigned int) GetLastError ());
      bson_free (out);
      return NULL;
   }
   return out;
}

// `read_file_and_null_terminate` reads a file into a NUL-terminated string.
// On success: returns a NUL-terminated string and (optionally) sets `*out_len` excluding NUL.
// On error: returns NULL.
static char *
read_file_and_null_terminate (const char *filename, size_t *out_len)
{
   BSON_ASSERT_PARAM (filename);
   BSON_OPTIONAL_PARAM (out_len);

   bool ok = false;
   char *contents = NULL;
   char errmsg_buf[BSON_ERROR_BUFFER_SIZE];

   FILE *file = fopen (filename, "rb");
   if (!file) {
      MONGOC_ERROR ("Failed to open file: '%s' with error: '%s'",
                    filename,
                    bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf));
      goto fail;
   }

   if (0 != fseek (file, 0, SEEK_END)) {
      MONGOC_ERROR ("Failed to seek in file: '%s' with error: '%s'",
                    filename,
                    bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf));
      goto fail;
   }

   const long file_len = ftell (file);
   if (file_len < 0) {
      MONGOC_ERROR ("Failed to get length of file: '%s' with error: '%s'",
                    filename,
                    bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf));
      goto fail;
   }

   if (file_len > LONG_MAX - 1) {
      goto fail;
   }

   if (0 != fseek (file, 0, SEEK_SET)) {
      goto fail;
   }

   // Read the whole file into one NUL-terminated string:
   contents = (char *) bson_malloc ((size_t) file_len + 1u);
   contents[file_len] = '\0';
   if ((size_t) file_len != fread (contents, 1, file_len, file)) {
      SecureZeroMemory (contents, file_len);
      if (feof (file)) {
         MONGOC_ERROR ("Unexpected EOF reading file: '%s'", filename);
         goto fail;
      } else {
         MONGOC_ERROR ("Failed to read file: '%s' with error: '%s'",
                       filename,
                       bson_strerror_r (errno, errmsg_buf, sizeof errmsg_buf));
         goto fail;
      }
   }
   if (out_len) {
      *out_len = (size_t) file_len;
   }

   ok = true;
fail:
   if (file) {
      (void) fclose (file); // Ignore error.
   }
   if (!ok) {
      bson_free (contents);
      contents = NULL;
   }
   return contents;
}


// `decode_object` decodes a cryptographic object from a blob.
// Returns NULL on error.
static LPBYTE
decode_object (const char *structType,
               const LPBYTE data,
               DWORD data_len,
               DWORD *out_len,
               const char *descriptor,
               const char *filename)
{
   BSON_ASSERT_PARAM (structType);
   BSON_ASSERT_PARAM (data);
   BSON_ASSERT_PARAM (structType);
   BSON_ASSERT_PARAM (out_len);
   BSON_ASSERT_PARAM (descriptor);
   BSON_ASSERT_PARAM (filename);
   // Get needed output length:
   if (!CryptDecodeObjectEx (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* dwCertEncodingType */
                             structType,                              /* lpszStructType */
                             data,                                    /* pbEncoded */
                             data_len,                                /* cbEncoded */
                             0,                                       /* dwFlags */
                             NULL,                                    /* pDecodePara */
                             NULL,                                    /* pvStructInfo */
                             out_len                                  /* pcbStructInfo */
                             )) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_ERROR ("Failed to decode %s from '%s': %s", descriptor, filename, msg);
      bson_free (msg);
      return NULL;
   }

   if (*out_len == 0) {
      return NULL;
   }
   LPBYTE out = (LPBYTE) bson_malloc (*out_len);

   if (!CryptDecodeObjectEx (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* dwCertEncodingType */
                             structType,                              /* lpszStructType */
                             data,                                    /* pbEncoded */
                             data_len,                                /* cbEncoded */
                             0,                                       /* dwFlags */
                             NULL,                                    /* pDecodePara */
                             out,                                     /* pvStructInfo */
                             out_len                                  /* pcbStructInfo */
                             )) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_ERROR ("Failed to decode %s from '%s': %s", descriptor, filename, msg);
      bson_free (msg);
      bson_free (out);
      return NULL;
   }

   return out;
}

// `utf8_to_wide` converts a UTF-8 string into a wide string using the Windows API MultiByteToWideChar.
// Returns a NULL-terminated wide character string on success. Returns NULL on error.
static WCHAR *
utf8_to_wide (const char *utf8)
{
   // Get necessary character count (not bytes!) of result:
   int required_wide_chars = MultiByteToWideChar (CP_UTF8, 0, utf8, -1 /* NULL terminated */, NULL, 0);
   if (required_wide_chars == 0) {
      return NULL;
   }

   // Since -1 was passed as the input length, the returned character count includes space for the null character.
   WCHAR *wide_chars = bson_malloc (sizeof (WCHAR) * required_wide_chars);
   if (0 == MultiByteToWideChar (CP_UTF8, 0, utf8, -1 /* NULL terminated */, wide_chars, required_wide_chars)) {
      bson_free (wide_chars);
      return NULL;
   }

   return wide_chars;
}

// `generate_key_name` generates a deterministic name for a key of the form: "libmongoc-<SHA256 fingerprint>-<suffix>".
// Returns NULL on error.
static LPWSTR
generate_key_name (LPBYTE data, DWORD len, const char *suffix)
{
   bool ok = false;
   char *hash_hex = NULL;
   char *key_name = NULL;
   LPWSTR key_name_wide = NULL;

   BSON_ASSERT_PARAM (data);
   BSON_ASSERT_PARAM (suffix);

   // Compute a hash of the certificate:
   {
      unsigned char hash[32];
      mongoc_crypto_t crypto;
      mongoc_crypto_init (&crypto, MONGOC_CRYPTO_ALGORITHM_SHA_256);
      if (!mongoc_crypto_hash (&crypto, (const unsigned char *) data, mlib_assert_narrow (size_t, len), hash)) {
         goto fail;
      }
      // Use uppercase hex to match form of `openssl x509` command:
      hash_hex = bin_to_hex ((const uint8_t *) hash, sizeof (hash));
      if (!hash_hex) {
         goto fail;
      }
   }

   // Convert to a wide string:
   {
      key_name = bson_strdup_printf ("libmongoc-%s-%s", hash_hex, suffix);
      key_name_wide = utf8_to_wide (key_name);
      if (!key_name_wide) {
         goto fail;
      }
   }

   ok = true;
fail:
   bson_free (key_name);
   bson_free (hash_hex);
   if (!ok) {
      bson_free (key_name_wide);
      key_name_wide = NULL;
   }

   return key_name_wide;
}

PCCERT_CONTEXT
mongoc_secure_channel_setup_certificate_from_file (const char *filename)
{
   char *pem;
   bool ret = false;
   bool success;
   size_t pem_length;
   HCRYPTPROV provider = 0u;
   DWORD encoded_cert_len;
   LPBYTE encoded_cert = NULL;
   const char *pem_public;
   const char *pem_private;
   PCCERT_CONTEXT cert = NULL;
   LPBYTE blob_private = NULL;
   DWORD blob_private_len = 0;
   LPBYTE blob_private_rsa = NULL;
   DWORD blob_private_rsa_len = 0;
   DWORD encoded_private_len = 0;
   LPBYTE encoded_private = NULL;
   NCRYPT_PROV_HANDLE cng_provider = 0u;
   LPWSTR key_name = NULL;

   BSON_ASSERT_PARAM (filename);

   pem = read_file_and_null_terminate (filename, &pem_length);
   if (!pem) {
      goto fail;
   }

   pem_public = strstr (pem, "-----BEGIN CERTIFICATE-----");
   if (!pem_public) {
      MONGOC_ERROR ("Can't find public certificate in '%s'", filename);
      goto fail;
   }

   pem_private = strstr (pem, "-----BEGIN ENCRYPTED PRIVATE KEY-----");

   if (pem_private) {
      MONGOC_ERROR ("Detected unsupported encrypted private key");
      goto fail;
   }

   encoded_cert = decode_pem_base64 (pem_public, &encoded_cert_len, "public key", filename);
   if (!encoded_cert) {
      goto fail;
   }
   cert = CertCreateCertificateContext (X509_ASN_ENCODING, encoded_cert, encoded_cert_len);

   if (!cert) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_ERROR ("Failed to extract public key from '%s': %s", filename, msg);
      bson_free (msg);
      goto fail;
   }

   // Import private key as a persisted (not ephemeral) key.
   // Ephemeral keys do not appear to support modern signatures. See CDRIVER-5998.
   if (NULL != (pem_private = strstr (pem, "-----BEGIN RSA PRIVATE KEY-----"))) {
      // Import PKCS#1 as a persisted CAPI key. Windows CNG API does not appear to support PKCS#1.
      encoded_private = decode_pem_base64 (pem_private, &encoded_private_len, "private key", filename);
      if (!encoded_private) {
         goto fail;
      }

      blob_private_rsa = decode_object (
         PKCS_RSA_PRIVATE_KEY, encoded_private, encoded_private_len, &blob_private_rsa_len, "private key", filename);
      if (!blob_private_rsa) {
         goto fail;
      }

      // Import persisted key with a deterministic name of the form "libmongoc-<SHA256 fingerprint>-pkcs1":
      key_name = generate_key_name (encoded_cert, encoded_cert_len, "pkcs1");
      if (!key_name) {
         MONGOC_ERROR ("Failed to generate key name");
         goto fail;
      }

      bool exists = false;
      success = CryptAcquireContextW (&provider,                       /* phProv */
                                      key_name,                        /* pszContainer */
                                      MS_ENHANCED_PROV_W,              /* pszProvider */
                                      PROV_RSA_FULL,                   /* dwProvType */
                                      CRYPT_NEWKEYSET | CRYPT_SILENT); /* dwFlags */
      if (!success) {
         DWORD last_error = GetLastError ();
         exists = last_error == (DWORD) NTE_EXISTS;
         if (!exists) {
            // Unexpected error:
            char *msg = mongoc_winerr_to_string (last_error);
            MONGOC_ERROR ("CryptAcquireContext failed: %s", msg);
            bson_free (msg);
            goto fail;
         }
      }

      if (!exists) {
         // Import CAPI key:
         HCRYPTKEY hKey;
         success = CryptImportKey (provider,             /* hProv */
                                   blob_private_rsa,     /* pbData */
                                   blob_private_rsa_len, /* dwDataLen */
                                   0,                    /* hPubKey */
                                   0,                    /* dwFlags */
                                   &hKey);               /* phKey, OUT */
         if (!success) {
            char *msg = mongoc_winerr_to_string (GetLastError ());
            MONGOC_ERROR ("CryptImportKey for private key failed: %s", msg);
            bson_free (msg);
            goto fail;
         }
         CryptDestroyKey (hKey);
      }

      CRYPT_KEY_PROV_INFO keyProvInfo = {0};
      keyProvInfo.pwszContainerName = key_name;
      keyProvInfo.pwszProvName = MS_ENHANCED_PROV_W,
      keyProvInfo.dwFlags |= CERT_SET_KEY_PROV_HANDLE_PROP_ID | CERT_SET_KEY_CONTEXT_PROP_ID | CRYPT_SILENT;
      keyProvInfo.dwProvType = PROV_RSA_FULL;
      keyProvInfo.dwKeySpec = AT_KEYEXCHANGE;
      success = CertSetCertificateContextProperty (cert,                         /* pCertContext */
                                                   CERT_KEY_PROV_INFO_PROP_ID,   /* dwPropId */
                                                   0,                            /* dwFlags */
                                                   (const void *) &keyProvInfo); /* pvData */
      if (!success) {
         char *msg = mongoc_winerr_to_string (GetLastError ());
         MONGOC_ERROR ("Can't associate private key with public key: %s", msg);
         bson_free (msg);
         goto fail;
      }
   } else if (NULL != (pem_private = strstr (pem, "-----BEGIN PRIVATE KEY-----"))) {
      // Import PKCS#8 as a persisted CNG key.
      encoded_private = decode_pem_base64 (pem_private, &encoded_private_len, "private key", filename);
      if (!encoded_private) {
         goto fail;
      }

      // Open the software key storage provider:
      SECURITY_STATUS status = NCryptOpenStorageProvider (&cng_provider, MS_KEY_STORAGE_PROVIDER, 0);
      if (status != SEC_E_OK) {
         char *msg = mongoc_winerr_to_string (GetLastError ());
         MONGOC_ERROR ("Can't open key storage provider: %s", msg);
         bson_free (msg);
         goto fail;
      }

      // Supply a key name to persist the key:
      NCryptBuffer buffer;
      NCryptBufferDesc bufferDesc;

      // Import persisted key with a deterministic name of the form "libmongoc-<SHA256 fingerprint>-pkcs8":
      key_name = generate_key_name (encoded_cert, encoded_cert_len, "pkcs8");
      if (!key_name) {
         MONGOC_ERROR ("Failed to generate key name");
         goto fail;
      }

      buffer.cbBuffer = (ULONG) (wcslen (key_name) + 1) * sizeof (WCHAR);
      buffer.BufferType = NCRYPTBUFFER_PKCS_KEY_NAME;
      buffer.pvBuffer = key_name;

      bufferDesc.ulVersion = NCRYPTBUFFER_VERSION;
      bufferDesc.cBuffers = 1;
      bufferDesc.pBuffers = &buffer;

      // Import the private key blob as a persisted CNG key:
      {
         NCRYPT_KEY_HANDLE hKey = 0;
         status = NCryptImportKey (cng_provider,
                                   0,
                                   NCRYPT_PKCS8_PRIVATE_KEY_BLOB,
                                   &bufferDesc,
                                   &hKey,
                                   encoded_private,
                                   encoded_private_len,
                                   NCRYPT_SILENT_FLAG);
         if (hKey) {
            NCryptFreeObject (hKey);
         }

         // Ignore `NTE_EXISTS` error since key may have already been imported:
         if (status != SEC_E_OK && status != NTE_EXISTS) {
            char *msg = mongoc_winerr_to_string ((DWORD) status);
            MONGOC_ERROR ("Failed to import key: %s", msg);
            bson_free (msg);
            goto fail;
         }
      }

      // Attach key to certificate:
      {
         CRYPT_KEY_PROV_INFO keyProvInfo = {0};
         keyProvInfo.pwszContainerName = key_name;
         keyProvInfo.pwszProvName = MS_KEY_STORAGE_PROVIDER,
         keyProvInfo.dwFlags |= CERT_SET_KEY_PROV_HANDLE_PROP_ID | CERT_SET_KEY_CONTEXT_PROP_ID | CRYPT_SILENT;
         keyProvInfo.dwProvType = 0 /* CNG */;
         keyProvInfo.dwKeySpec = AT_KEYEXCHANGE;
         if (!CertSetCertificateContextProperty (cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &keyProvInfo)) {
            char *msg = mongoc_winerr_to_string (GetLastError ());
            MONGOC_ERROR ("Failed to attach key to certificate: %s", msg);
            bson_free (msg);
            goto fail;
         }
      }
   } else {
      MONGOC_ERROR ("Can't find private key in '%s'", filename);
      goto fail;
   }


   TRACE ("%s", "Successfully loaded client certificate");
   ret = true;

fail:
   bson_free (key_name);

   if (cng_provider) {
      NCryptFreeObject (cng_provider);
   }

   if (provider) {
      CryptReleaseContext (provider, 0);
   }

   if (pem) {
      SecureZeroMemory (pem, pem_length);
      bson_free (pem);
   }
   bson_free (encoded_cert);
   if (encoded_private) {
      SecureZeroMemory (encoded_private, encoded_private_len);
      bson_free (encoded_private);
   }

   if (blob_private_rsa) {
      SecureZeroMemory (blob_private_rsa, blob_private_rsa_len);
      bson_free (blob_private_rsa);
   }

   if (blob_private) {
      SecureZeroMemory (blob_private, blob_private_len);
      bson_free (blob_private);
   }

   if (!ret) {
      CertFreeCertificateContext (cert);
      return NULL;
   }

   return cert;
}

PCCERT_CONTEXT
mongoc_secure_channel_setup_certificate (const mongoc_ssl_opt_t *opt)
{
   return mongoc_secure_channel_setup_certificate_from_file (opt->pem_file);
}


bool
mongoc_secure_channel_setup_ca (const mongoc_ssl_opt_t *opt)
{
   bool ok = false;
   char *pem = NULL;
   const char *pem_key;
   HCERTSTORE cert_store = NULL;
   PCCERT_CONTEXT cert = NULL;
   DWORD encoded_cert_len = 0;
   LPBYTE encoded_cert = NULL;

   pem = read_file_and_null_terminate (opt->ca_file, NULL);
   if (!pem) {
      return false;
   }

   /* If we have private keys or other fuzz, seek to the good stuff */
   pem_key = strstr (pem, "-----BEGIN CERTIFICATE-----");

   if (!pem_key) {
      MONGOC_WARNING ("Couldn't find certificate in '%s'", opt->ca_file);
      goto fail;
   }

   encoded_cert = decode_pem_base64 (pem_key, &encoded_cert_len, "public key", opt->ca_file);
   if (!encoded_cert) {
      goto fail;
   }

   cert = CertCreateCertificateContext (X509_ASN_ENCODING, encoded_cert, encoded_cert_len);
   if (!cert) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_WARNING ("Could not convert certificate: %s", msg);
      bson_free (msg);
      goto fail;
   }


   cert_store = CertOpenStore (CERT_STORE_PROV_SYSTEM,                  /* provider */
                               X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* certificate encoding */
                               0,                                       /* unused */
                               CERT_SYSTEM_STORE_LOCAL_MACHINE,         /* dwFlags */
                               L"Root");                                /* system store name. "My" or "Root" */

   if (cert_store == NULL) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_ERROR ("Error opening certificate store: %s", msg);
      bson_free (msg);
      goto fail;
   }

   if (!CertAddCertificateContextToStore (cert_store, cert, CERT_STORE_ADD_USE_EXISTING, NULL)) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_WARNING ("Failed adding the cert: %s", msg);
      bson_free (msg);
      goto fail;
   }

   TRACE ("%s", "Added the certificate !");
   ok = true;
fail:
   CertCloseStore (cert_store, 0);
   bson_free (encoded_cert);
   CertFreeCertificateContext (cert);
   bson_free (pem);
   return ok;
}

PCCRL_CONTEXT
mongoc_secure_channel_load_crl (const char *crl_file)
{
   PCCRL_CONTEXT crl = NULL;
   bool ok = false;
   DWORD encoded_crl_len = 0;
   LPBYTE encoded_crl = NULL;

   char *pem = read_file_and_null_terminate (crl_file, NULL);
   if (!pem) {
      goto fail;
   }

   const char *pem_begin = strstr (pem, "-----BEGIN X509 CRL-----");
   if (!pem_begin) {
      MONGOC_WARNING ("Couldn't find CRL in '%s'", crl_file);
      goto fail;
   }

   encoded_crl = decode_pem_base64 (pem_begin, &encoded_crl_len, "CRL", crl_file);
   if (!encoded_crl) {
      goto fail;
   }

   crl = CertCreateCRLContext (X509_ASN_ENCODING, encoded_crl, encoded_crl_len);

   if (!crl) {
      MONGOC_WARNING ("Can't extract CRL from '%s'", crl_file);
      goto fail;
   }

   ok = true;
fail:
   bson_free (encoded_crl);
   bson_free (pem);
   if (!ok) {
      CertFreeCRLContext (crl);
      crl = NULL;
   }
   return crl;
}

bool
mongoc_secure_channel_setup_crl (const mongoc_ssl_opt_t *opt)
{
   HCERTSTORE cert_store = NULL;
   bool ok = false;

   PCCRL_CONTEXT crl = mongoc_secure_channel_load_crl (opt->crl_file);
   if (!crl) {
      goto fail;
   }

   cert_store = CertOpenStore (CERT_STORE_PROV_SYSTEM,                  /* provider */
                               X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* certificate encoding */
                               0,                                       /* unused */
                               CERT_SYSTEM_STORE_LOCAL_MACHINE,         /* dwFlags */
                               L"Root");                                /* system store name. "My" or "Root" */

   if (cert_store == NULL) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_ERROR ("Error opening certificate store: %s", msg);
      bson_free (msg);
      goto fail;
   }

   if (!CertAddCRLContextToStore (cert_store, crl, CERT_STORE_ADD_USE_EXISTING, NULL)) {
      char *msg = mongoc_winerr_to_string (GetLastError ());
      MONGOC_WARNING ("Failed adding the CRL: %s", msg);
      bson_free (msg);
      goto fail;
   }

   TRACE ("%s", "Added the CRL!");
   ok = true;

fail:
   CertCloseStore (cert_store, 0);
   CertFreeCRLContext (crl);
   return ok;
}

ssize_t
mongoc_secure_channel_read (mongoc_stream_tls_t *tls, void *data, size_t data_length)
{
   BSON_ASSERT_PARAM (tls);

   if (BSON_UNLIKELY (!mlib_in_range (int32_t, tls->timeout_msec))) {
      // CDRIVER-4589
      MONGOC_ERROR ("timeout_msec value %" PRId64 " exceeds supported 32-bit range", tls->timeout_msec);
      return -1;
   }

   errno = 0;
   TRACE ("Wanting to read: %zu, timeout is %" PRId64, data_length, tls->timeout_msec);
   /* 4th argument is minimum bytes, while the data_length is the
    * size of the buffer. We are totally fine with just one TLS record (few
    *bytes)
    **/
   const ssize_t length = mongoc_stream_read (tls->base_stream, data, data_length, 0, (int32_t) tls->timeout_msec);

   TRACE ("Got %zd", length);

   if (length > 0) {
      return length;
   }

   return 0;
}

ssize_t
mongoc_secure_channel_write (mongoc_stream_tls_t *tls, const void *data, size_t data_length)
{
   BSON_ASSERT_PARAM (tls);

   if (BSON_UNLIKELY (!mlib_in_range (int32_t, tls->timeout_msec))) {
      // CDRIVER-4589
      MONGOC_ERROR ("timeout_msec value %" PRId64 " exceeds supported 32-bit range", tls->timeout_msec);
      return -1;
   }

   errno = 0;
   TRACE ("Wanting to write: %zu", data_length);
   const ssize_t length =
      mongoc_stream_write (tls->base_stream, (void *) data, data_length, (int32_t) tls->timeout_msec);
   TRACE ("Wrote: %zd", length);

   return length;
}

void
mongoc_secure_channel_realloc_buf (size_t *size, uint8_t **buf, size_t new_size)
{
   *size = bson_next_power_of_two (new_size);
   *buf = bson_realloc (*buf, *size);
}

/**
 * The follow functions comes from one of my favorite project, cURL!
 * Thank you so much for having gone through the Secure Channel pain for me.
 *
 *
 * Copyright (C) 2012 - 2015, Marc Hoersken, <info@marc-hoersken.de>
 * Copyright (C) 2012, Mark Salisbury, <mark.salisbury@hp.com>
 * Copyright (C) 2012 - 2015, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * Based upon the PolarSSL implementation in polarssl.c and polarssl.h:
 *   Copyright (C) 2010, 2011, Hoi-Ho Chan, <hoiho.chan@gmail.com>
 *
 * Based upon the CyaSSL implementation in cyassl.c and cyassl.h:
 *   Copyright (C) 1998 - 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * Thanks for code and inspiration!
 */

void
_mongoc_secure_channel_init_sec_buffer (SecBuffer *buffer,
                                        unsigned long buf_type,
                                        void *buf_data_ptr,
                                        unsigned long buf_byte_size)
{
   buffer->cbBuffer = buf_byte_size;
   buffer->BufferType = buf_type;
   buffer->pvBuffer = buf_data_ptr;
}

void
_mongoc_secure_channel_init_sec_buffer_desc (SecBufferDesc *desc, SecBuffer *buffer_array, unsigned long buffer_count)
{
   desc->ulVersion = SECBUFFER_VERSION;
   desc->pBuffers = buffer_array;
   desc->cBuffers = buffer_count;
}


#define MONGOC_LOG_AND_SET_ERROR(ERROR, DOMAIN, CODE, ...)  \
   do {                                                     \
      MONGOC_ERROR (__VA_ARGS__);                           \
      _mongoc_set_error (ERROR, DOMAIN, CODE, __VA_ARGS__); \
   } while (0)

bool
mongoc_secure_channel_handshake_step_1 (mongoc_stream_tls_t *tls, char *hostname, bson_error_t *error)
{
   SecBuffer outbuf;
   ssize_t written = -1;
   SecBufferDesc outbuf_desc;
   SECURITY_STATUS sspi_status = SEC_E_OK;
   mongoc_stream_tls_secure_channel_t *secure_channel = (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   TRACE ("SSL/TLS connection with '%s' (step 1/3)", hostname);

   /* setup output buffer */
   _mongoc_secure_channel_init_sec_buffer (&outbuf, SECBUFFER_EMPTY, NULL, 0);
   _mongoc_secure_channel_init_sec_buffer_desc (&outbuf_desc, &outbuf, 1);

   /* setup request flags */
   secure_channel->req_flags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY |
                               ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

   /* allocate memory for the security context handle */
   secure_channel->ctxt = (mongoc_secure_channel_ctxt *) bson_malloc0 (sizeof (mongoc_secure_channel_ctxt));

   /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa375924.aspx */
   sspi_status = InitializeSecurityContext (&secure_channel->cred_handle->cred_handle, /* phCredential */
                                            NULL,                                      /* phContext */
                                            hostname,                                  /* pszTargetName */
                                            secure_channel->req_flags,                 /* fContextReq */
                                            0,                                         /* Reserved1, must be 0 */
                                            0,                                         /* TargetDataRep, unused */
                                            NULL,                                      /* pInput */
                                            0,                                         /* Reserved2, must be 0 */
                                            &secure_channel->ctxt->ctxt_handle,        /* phNewContext OUT param */
                                            &outbuf_desc,                              /* pOutput OUT param */
                                            &secure_channel->ret_flags,                /* pfContextAttr OUT param */
                                            &secure_channel->ctxt->time_stamp          /* ptsExpiry OUT param */
   );
   if (sspi_status != SEC_I_CONTINUE_NEEDED) {
      // Cast signed SECURITY_STATUS to unsigned DWORD. FormatMessage expects DWORD.
      char *msg = mongoc_winerr_to_string ((DWORD) sspi_status);
      MONGOC_LOG_AND_SET_ERROR (
         error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "initial InitializeSecurityContext failed: %s", msg);
      bson_free (msg);
      return false;
   }

   TRACE ("sending initial handshake data: sending %lu bytes...", outbuf.cbBuffer);

   /* send initial handshake data which is now stored in output buffer */
   written = mongoc_secure_channel_write (tls, outbuf.pvBuffer, outbuf.cbBuffer);
   FreeContextBuffer (outbuf.pvBuffer);

   if (outbuf.cbBuffer != (size_t) written) {
      MONGOC_LOG_AND_SET_ERROR (error,
                                MONGOC_ERROR_STREAM,
                                MONGOC_ERROR_STREAM_SOCKET,
                                "failed to send initial handshake data: "
                                "sent %zd of %lu bytes",
                                written,
                                outbuf.cbBuffer);
      return false;
   }

   TRACE ("sent initial handshake data: sent %zd bytes", written);

   secure_channel->recv_unrecoverable_err = 0;
   secure_channel->recv_sspi_close_notify = false;
   secure_channel->recv_connection_closed = false;

   /* continue to second handshake step */
   secure_channel->connecting_state = ssl_connect_2;

   return true;
}

bool
mongoc_secure_channel_handshake_step_2 (mongoc_stream_tls_t *tls, char *hostname, bson_error_t *error)
{
   mongoc_stream_tls_secure_channel_t *secure_channel = (mongoc_stream_tls_secure_channel_t *) tls->ctx;
   SECURITY_STATUS sspi_status = SEC_E_OK;
   ssize_t nread = -1, written = -1;
   SecBufferDesc outbuf_desc;
   SecBufferDesc inbuf_desc;
   SecBuffer outbuf[3];
   SecBuffer inbuf[2];
   bool doread;
   int i;

   doread = (secure_channel->connecting_state != ssl_connect_2_writing) ? true : false;

   TRACE ("%s", "SSL/TLS connection with endpoint (step 2/3)");

   if (!secure_channel->cred_handle || !secure_channel->ctxt) {
      MONGOC_LOG_AND_SET_ERROR (
         error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "required TLS credentials or context not provided");

      return false;
   }

   /* grow the buffer if necessary */
   if (secure_channel->encdata_length == secure_channel->encdata_offset) {
      mongoc_secure_channel_realloc_buf (
         &secure_channel->encdata_length, &secure_channel->encdata_buffer, secure_channel->encdata_length + 1);
   }

   for (;;) {
      if (doread) {
         /* read encrypted handshake data from socket */
         nread = mongoc_secure_channel_read (tls,
                                             (char *) (secure_channel->encdata_buffer + secure_channel->encdata_offset),
                                             secure_channel->encdata_length - secure_channel->encdata_offset);

         if (!nread) {
            if (MONGOC_ERRNO_IS_AGAIN (errno)) {
               if (secure_channel->connecting_state != ssl_connect_2_writing) {
                  secure_channel->connecting_state = ssl_connect_2_reading;
               }

               TRACE ("%s", "failed to receive handshake, need more data");
               return true;
            }

            MONGOC_LOG_AND_SET_ERROR (error,
                                      MONGOC_ERROR_STREAM,
                                      MONGOC_ERROR_STREAM_SOCKET,
                                      "failed to receive handshake, SSL/TLS connection failed");

            return false;
         }

         /* increase encrypted data buffer offset */
         secure_channel->encdata_offset += nread;
      }

      TRACE ("encrypted data buffer: offset %d length %d",
             (int) secure_channel->encdata_offset,
             (int) secure_channel->encdata_length);

      /* setup input buffers */
      _mongoc_secure_channel_init_sec_buffer (&inbuf[0],
                                              SECBUFFER_TOKEN,
                                              bson_malloc (secure_channel->encdata_offset),
                                              (unsigned long) (secure_channel->encdata_offset & (size_t) 0xFFFFFFFFUL));
      _mongoc_secure_channel_init_sec_buffer (&inbuf[1], SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer_desc (&inbuf_desc, inbuf, 2);

      /* setup output buffers */
      _mongoc_secure_channel_init_sec_buffer (&outbuf[0], SECBUFFER_TOKEN, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer (&outbuf[1], SECBUFFER_ALERT, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer (&outbuf[2], SECBUFFER_EMPTY, NULL, 0);
      _mongoc_secure_channel_init_sec_buffer_desc (&outbuf_desc, outbuf, 3);

      if (inbuf[0].pvBuffer == NULL) {
         MONGOC_LOG_AND_SET_ERROR (error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "unable to allocate memory");
         return false;
      }

      /* copy received handshake data into input buffer */
      memcpy (inbuf[0].pvBuffer, secure_channel->encdata_buffer, secure_channel->encdata_offset);

      /* https://msdn.microsoft.com/en-us/library/windows/desktop/aa375924.aspx
       */
      sspi_status = InitializeSecurityContext (&secure_channel->cred_handle->cred_handle,
                                               &secure_channel->ctxt->ctxt_handle,
                                               hostname,
                                               secure_channel->req_flags,
                                               0,
                                               0,
                                               &inbuf_desc,
                                               0,
                                               NULL,
                                               &outbuf_desc,
                                               &secure_channel->ret_flags,
                                               &secure_channel->ctxt->time_stamp);

      /* free buffer for received handshake data */
      bson_free (inbuf[0].pvBuffer);

      /* check if the handshake was incomplete */
      if (sspi_status == SEC_E_INCOMPLETE_MESSAGE) {
         secure_channel->connecting_state = ssl_connect_2_reading;
         TRACE ("%s", "received incomplete message, need more data");
         return true;
      }

      /* If the server has requested a client certificate, attempt to continue
       * the handshake without one. This will allow connections to servers which
       * request a client certificate but do not require it. */
      if (sspi_status == SEC_I_INCOMPLETE_CREDENTIALS && !(secure_channel->req_flags & ISC_REQ_USE_SUPPLIED_CREDS)) {
         secure_channel->req_flags |= ISC_REQ_USE_SUPPLIED_CREDS;
         secure_channel->connecting_state = ssl_connect_2_writing;
         TRACE ("%s", "A client certificate has been requested");
         return true;
      }

      /* check if the handshake needs to be continued */
      if (sspi_status == SEC_I_CONTINUE_NEEDED || sspi_status == SEC_E_OK) {
         for (i = 0; i < 3; i++) {
            /* search for handshake tokens that need to be send */
            if (outbuf[i].BufferType == SECBUFFER_TOKEN && outbuf[i].cbBuffer > 0) {
               TRACE ("sending next handshake data: sending %lu bytes...", outbuf[i].cbBuffer);

               /* send handshake token to server */
               written = mongoc_secure_channel_write (tls, outbuf[i].pvBuffer, outbuf[i].cbBuffer);

               if (outbuf[i].cbBuffer != (size_t) written) {
                  MONGOC_LOG_AND_SET_ERROR (error,
                                            MONGOC_ERROR_STREAM,
                                            MONGOC_ERROR_STREAM_SOCKET,
                                            "failed to send next handshake data: "
                                            "sent %zd of %lu bytes",
                                            written,
                                            outbuf[i].cbBuffer);
                  return false;
               }
            }

            /* free obsolete buffer */
            if (outbuf[i].pvBuffer != NULL) {
               FreeContextBuffer (outbuf[i].pvBuffer);
            }
         }
      } else {
         switch (sspi_status) {
         case SEC_E_WRONG_PRINCIPAL:
            MONGOC_LOG_AND_SET_ERROR (error,
                                      MONGOC_ERROR_STREAM,
                                      MONGOC_ERROR_STREAM_SOCKET,
                                      "SSL Certification verification failed: hostname "
                                      "doesn't match certificate");
            break;

         case SEC_E_UNTRUSTED_ROOT:
            MONGOC_LOG_AND_SET_ERROR (error,
                                      MONGOC_ERROR_STREAM,
                                      MONGOC_ERROR_STREAM_SOCKET,
                                      "SSL Certification verification failed: Untrusted "
                                      "root certificate");
            break;

         case SEC_E_CERT_EXPIRED:
            MONGOC_LOG_AND_SET_ERROR (error,
                                      MONGOC_ERROR_STREAM,
                                      MONGOC_ERROR_STREAM_SOCKET,
                                      "SSL Certification verification failed: certificate "
                                      "has expired");
            break;
         case CRYPT_E_NO_REVOCATION_CHECK:
            MONGOC_LOG_AND_SET_ERROR (error,
                                      MONGOC_ERROR_STREAM,
                                      MONGOC_ERROR_STREAM_SOCKET,
                                      "SSL Certification verification failed: certificate "
                                      "does not include revocation check.");
            break;

         case SEC_E_INSUFFICIENT_MEMORY:
         case SEC_E_INTERNAL_ERROR:
         case SEC_E_INVALID_HANDLE:
         case SEC_E_INVALID_TOKEN:
         case SEC_E_LOGON_DENIED:
         case SEC_E_NO_AUTHENTICATING_AUTHORITY:
         case SEC_E_NO_CREDENTIALS:
         case SEC_E_TARGET_UNKNOWN:
         case SEC_E_UNSUPPORTED_FUNCTION:
#ifdef SEC_E_APPLICATION_PROTOCOL_MISMATCH
         /* Not available in VS2010 */
         case SEC_E_APPLICATION_PROTOCOL_MISMATCH:
#endif


         default: {
            // Cast signed SECURITY_STATUS to unsigned DWORD. FormatMessage expects DWORD.
            char *msg = mongoc_winerr_to_string ((DWORD) sspi_status);
            MONGOC_LOG_AND_SET_ERROR (error,
                                      MONGOC_ERROR_STREAM,
                                      MONGOC_ERROR_STREAM_SOCKET,
                                      "Failed to initialize security context: %s",
                                      msg);
            bson_free (msg);
         }
         }
         return false;
      }

      /* check if there was additional remaining encrypted data */
      if (inbuf[1].BufferType == SECBUFFER_EXTRA && inbuf[1].cbBuffer > 0) {
         TRACE ("encrypted data length: %lu", inbuf[1].cbBuffer);

         /*
          * There are two cases where we could be getting extra data here:
          * 1) If we're renegotiating a connection and the handshake is already
          * complete (from the server perspective), it can encrypted app data
          * (not handshake data) in an extra buffer at this point.
          * 2) (sspi_status == SEC_I_CONTINUE_NEEDED) We are negotiating a
          * connection and this extra data is part of the handshake.
          * We should process the data immediately; waiting for the socket to
          * be ready may fail since the server is done sending handshake data.
          */
         /* check if the remaining data is less than the total amount
          * and therefore begins after the already processed data */
         if (secure_channel->encdata_offset > inbuf[1].cbBuffer) {
            memmove (secure_channel->encdata_buffer,
                     (secure_channel->encdata_buffer + secure_channel->encdata_offset) - inbuf[1].cbBuffer,
                     inbuf[1].cbBuffer);
            secure_channel->encdata_offset = inbuf[1].cbBuffer;

            if (sspi_status == SEC_I_CONTINUE_NEEDED) {
               doread = FALSE;
               continue;
            }
         }
      } else {
         secure_channel->encdata_offset = 0;
      }

      break;
   }

   /* check if the handshake needs to be continued */
   if (sspi_status == SEC_I_CONTINUE_NEEDED) {
      secure_channel->connecting_state = ssl_connect_2_reading;
      return true;
   }

   /* check if the handshake is complete */
   if (sspi_status == SEC_E_OK) {
      secure_channel->connecting_state = ssl_connect_3;
      TRACE ("%s", "SSL/TLS handshake complete");
   }

   return true;
}

bool
mongoc_secure_channel_handshake_step_3 (mongoc_stream_tls_t *tls, char *hostname, bson_error_t *error)
{
   mongoc_stream_tls_secure_channel_t *secure_channel = (mongoc_stream_tls_secure_channel_t *) tls->ctx;

   BSON_ASSERT (ssl_connect_3 == secure_channel->connecting_state);

   TRACE ("SSL/TLS connection with %s (step 3/3)", hostname);

   if (!secure_channel->cred_handle) {
      MONGOC_LOG_AND_SET_ERROR (
         error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "required TLS credentials not provided");
      return false;
   }

   /* check if the required context attributes are met */
   if (secure_channel->ret_flags != secure_channel->req_flags) {
      MONGOC_LOG_AND_SET_ERROR (error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "Failed handshake");

      return false;
   }

   secure_channel->connecting_state = ssl_connect_done;

   return true;
}
#endif
