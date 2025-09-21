#include <mongoc/mongoc-ssl-private.h>

#include <mongoc/mongoc.h>

#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <mongoc/mongoc-openssl-private.h>
#endif

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
#include <mongoc/mongoc-secure-channel-private.h>
#endif

#include <TestSuite.h>
#include <test-conveniences.h> // tmp_bson
#include <test-libmongoc.h>

#ifdef MONGOC_ENABLE_OCSP_OPENSSL
/* Test parsing a DER encoded tlsfeature extension contents for the
 * status_request (value 5). This is a SEQUENCE of INTEGER. libmongoc assumes
 * this is a sequence of one byte integers. */

static void
_expect_malformed (const char *data, int32_t len)
{
   bool ret;

   ret = _mongoc_tlsfeature_has_status_request ((const uint8_t *) data, len);
   BSON_ASSERT (!ret);
   ASSERT_CAPTURED_LOG ("mongoc", MONGOC_LOG_LEVEL_ERROR, "malformed");
   clear_captured_logs ();
}

static void
_expect_no_status_request (const char *data, int32_t len)
{
   bool ret;
   ret = _mongoc_tlsfeature_has_status_request ((const uint8_t *) data, len);
   BSON_ASSERT (!ret);
   ASSERT_NO_CAPTURED_LOGS ("mongoc");
}

static void
_expect_status_request (const char *data, int32_t len)
{
   bool ret;
   ret = _mongoc_tlsfeature_has_status_request ((const uint8_t *) data, len);
   BSON_ASSERT (ret);
   ASSERT_NO_CAPTURED_LOGS ("mongoc");
}

static void
test_tlsfeature_parsing (void)
{
   capture_logs (true);
   /* A sequence of one integer = 5. */
   _expect_status_request ("\x30\x03\x02\x01\x05", 5);
   /* A sequence of one integer = 6. */
   _expect_no_status_request ("\x30\x03\x02\x01\x06", 5);
   /* A sequence of two integers = 5,6. */
   _expect_status_request ("\x30\x03\x02\x01\x05\x02\x01\x06", 8);
   /* A sequence of two integers = 6,5. */
   _expect_status_request ("\x30\x03\x02\x01\x06\x02\x01\x05", 8);
   /* A sequence containing a non-integer. Parsing fails. */
   _expect_malformed ("\x30\x03\x03\x01\x05\x02\x01\x06", 8);
   /* A non-sequence. It will not read past the first byte (despite the >1
    * length). */
   _expect_malformed ("\xFF", 2);
   /* A sequence with a length represented in more than one byte. Parsing fails.
    */
   _expect_malformed ("\x30\x82\x04\x48", 4);
   /* An integer with length > 1. Parsing fails. */
   _expect_malformed ("\x30\x03\x02\x02\x05\x05", 6);
}
#endif /* MONGOC_ENABLE_OCSP_OPENSSL */

#ifdef MONGOC_ENABLE_SSL
static void
create_x509_user (void)
{
   bson_error_t error;

   mongoc_client_t *client = test_framework_new_default_client ();
   bool ok =
      mongoc_client_command_simple (client,
                                    "$external",
                                    tmp_bson (BSON_STR ({
                                       "createUser" : "C=US,ST=New York,L=New York City,O=MDB,OU=Drivers,CN=client",
                                       "roles" : [ {"role" : "readWrite", "db" : "db"} ]
                                    })),
                                    NULL /* read_prefs */,
                                    NULL /* reply */,
                                    &error);
   ASSERT_OR_PRINT (ok, error);
   mongoc_client_destroy (client);
}

static void
drop_x509_user (bool ignore_notfound)
{
   bson_error_t error;

   mongoc_client_t *client = test_framework_new_default_client ();
   bool ok = mongoc_client_command_simple (
      client,
      "$external",
      tmp_bson (BSON_STR ({"dropUser" : "C=US,ST=New York,L=New York City,O=MDB,OU=Drivers,CN=client"})),
      NULL /* read_prefs */,
      NULL /* reply */,
      &error);

   if (!ok) {
      ASSERT_OR_PRINT (ignore_notfound && NULL != strstr (error.message, "not found"), error);
   }
   mongoc_client_destroy (client);
}

static mongoc_uri_t *
get_x509_uri (void)
{
   bson_error_t error;
   char *uristr_noauth = test_framework_get_uri_str_no_auth ("db");
   mongoc_uri_t *uri = mongoc_uri_new_with_error (uristr_noauth, &error);
   ASSERT_OR_PRINT (uri, error);
   ASSERT (mongoc_uri_set_auth_mechanism (uri, "MONGODB-X509"));
   ASSERT (mongoc_uri_set_auth_source (uri, "$external"));
   bson_free (uristr_noauth);
   return uri;
}

static bool
try_insert (mongoc_client_t *client, bson_error_t *error)
{
   mongoc_collection_t *coll = mongoc_client_get_collection (client, "db", "coll");
   bool ok = mongoc_collection_insert_one (coll, tmp_bson ("{}"), NULL, NULL, error);
   mongoc_collection_destroy (coll);
   return ok;
}

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
// Define utilities check and delete imported keys for Secure Channel:

// The deterministic name for the certificate in src/libmongoc/tests/x509gen/client-pkcs8-unencrypted.pem.
static LPCWSTR pkcs8_key_name = L"libmongoc-6659E73980D0FB4EB315CF600E0B10CCBB8C3B74FD3ED94DEAF6DC2D2B6B8317-pkcs8";

static void
delete_imported_pkcs8_key (void)
{
   // Open the software key storage provider:
   NCRYPT_PROV_HANDLE hProv = 0;
   SECURITY_STATUS status = NCryptOpenStorageProvider (&hProv, MS_KEY_STORAGE_PROVIDER, 0);
   ASSERT_WITH_MSG (status == SEC_E_OK, "Failed to open key storage provider: %s", mongoc_winerr_to_string (status));

   // Open the key handle:
   NCRYPT_PROV_HANDLE keyHandle = 0;
   status = NCryptOpenKey (hProv, &keyHandle, pkcs8_key_name, 0, 0);
   ASSERT_WITH_MSG (status == SEC_E_OK, "Failed to open key: %s", mongoc_winerr_to_string (status));

   // Delete key:
   status = NCryptDeleteKey (keyHandle, 0); // Also frees handle.
   ASSERT_WITH_MSG (status == SEC_E_OK, "Failed to delete key: %s", mongoc_winerr_to_string (status));

   // NCryptDeleteKey freed handle.
   ASSERT (NCryptFreeObject (hProv) == ERROR_SUCCESS);
}

static bool
has_imported_pkcs8_key (void)
{
   // Open the software key storage provider:
   NCRYPT_PROV_HANDLE hProv = 0;
   SECURITY_STATUS status = NCryptOpenStorageProvider (&hProv, MS_KEY_STORAGE_PROVIDER, 0);
   ASSERT_WITH_MSG (status == SEC_E_OK, "Failed to open key storage provider: %s", mongoc_winerr_to_string (status));

   // Open the key handle:
   NCRYPT_PROV_HANDLE keyHandle = 0;
   status = NCryptOpenKey (hProv, &keyHandle, pkcs8_key_name, 0, 0);
   bool found = (status == SEC_E_OK);
   // NTE_BAD_KEYSET is expected if key is not found.
   ASSERT_WITH_MSG (
      status == SEC_E_OK || status == NTE_BAD_KEYSET, "Failed to open key: %s", mongoc_winerr_to_string (status));

   if (keyHandle) {
      ASSERT (NCryptFreeObject (keyHandle) == ERROR_SUCCESS);
   }
   ASSERT (NCryptFreeObject (hProv) == ERROR_SUCCESS);
   return found;
}

// The deterministic name for the certificate in src/libmongoc/tests/x509gen/client.pem.
static LPCWSTR pkcs1_key_name = L"libmongoc-6659E73980D0FB4EB315CF600E0B10CCBB8C3B74FD3ED94DEAF6DC2D2B6B8317-pkcs1";

static void
delete_imported_pkcs1_key (void)
{
   HCRYPTPROV provider;
   bool success = CryptAcquireContextW (&provider,                          /* phProv */
                                        pkcs1_key_name,                     /* pszContainer */
                                        MS_ENHANCED_PROV_W,                 /* pszProvider */
                                        PROV_RSA_FULL,                      /* dwProvType */
                                        CRYPT_DELETEKEYSET | CRYPT_SILENT); /* dwFlags */
   ASSERT_WITH_MSG (success, "Failed to delete key: %s", mongoc_winerr_to_string (GetLastError ()));
   // CRYPT_DELETEKEYSET leaves provider undefined. Do not call CryptReleaseContext.
}

static bool
has_imported_pkcs1_key (void)
{
   HCRYPTPROV provider = 0;
   bool success = CryptAcquireContextW (&provider,          /* phProv */
                                        pkcs1_key_name,     /* pszContainer */
                                        MS_ENHANCED_PROV_W, /* pszProvider */
                                        PROV_RSA_FULL,      /* dwProvType */
                                        CRYPT_SILENT);      /* dwFlags */
   if (!success) {
      const DWORD lastError = GetLastError ();
      // NTE_BAD_KEYSET is expected if key is not found.
      ASSERT_WITH_MSG (lastError == (DWORD) NTE_BAD_KEYSET,
                       "Unexpected error in acquiring context: %s",
                       mongoc_winerr_to_string (lastError));
      return false;
   }

   ASSERT (CryptReleaseContext (provider, 0));
   return true;
}

#define SCHANNEL_ASSERT_PKCS8_KEY_IMPORTED() ASSERT (has_imported_pkcs8_key ())
#define SCHANNEL_ASSERT_PKCS8_KEY_NOT_IMPORTED() ASSERT (!has_imported_pkcs8_key ())
#define SCHANNEL_DELETE_PKCS8_KEY() \
   if (has_imported_pkcs8_key ())   \
      delete_imported_pkcs8_key (); \
   else                             \
      (void) 0
#define SCHANNEL_ASSERT_PKCS1_KEY_IMPORTED() ASSERT (has_imported_pkcs1_key ())
#define SCHANNEL_ASSERT_PKCS1_KEY_NOT_IMPORTED() ASSERT (!has_imported_pkcs1_key ())
#define SCHANNEL_DELETE_PKCS1_KEY() \
   if (has_imported_pkcs1_key ())   \
      delete_imported_pkcs1_key (); \
   else                             \
      (void) 0
#else
#define SCHANNEL_ASSERT_PKCS8_KEY_IMPORTED() ((void) 0)
#define SCHANNEL_ASSERT_PKCS8_KEY_NOT_IMPORTED() ((void) 0)
#define SCHANNEL_DELETE_PKCS8_KEY() ((void) 0)
#define SCHANNEL_ASSERT_PKCS1_KEY_IMPORTED() ((void) 0)
#define SCHANNEL_ASSERT_PKCS1_KEY_NOT_IMPORTED() ((void) 0)
#define SCHANNEL_DELETE_PKCS1_KEY() ((void) 0)
#endif // MONGOC_ENABLE_SSL_SECURE_CHANNEL

static void
test_x509_auth (void *unused)
{
   BSON_UNUSED (unused);

   drop_x509_user (true /* ignore "not found" error */);
   create_x509_user ();

   // Test auth works with PKCS8 key:
   {
      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (mongoc_uri_set_option_as_utf8 (
            uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, CERT_TEST_DIR "/client-pkcs8-unencrypted.pem"));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;

      // Test SChannel key is not previously imported:
      {
         SCHANNEL_DELETE_PKCS8_KEY ();
         SCHANNEL_ASSERT_PKCS8_KEY_NOT_IMPORTED ();
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL); // Imports key.
         SCHANNEL_ASSERT_PKCS8_KEY_IMPORTED ();
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      // Test SChannel key is previously imported:
      {
         SCHANNEL_ASSERT_PKCS8_KEY_IMPORTED ();
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      ASSERT_OR_PRINT (ok, error);
      mongoc_uri_destroy (uri);
   }

   // Test auth works with a PKCS1 key:
   {
      SCHANNEL_DELETE_PKCS1_KEY ();
      SCHANNEL_ASSERT_PKCS1_KEY_NOT_IMPORTED ();

      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, CERT_CLIENT));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;
      // Test SChannel key is not previously imported:
      {
         SCHANNEL_DELETE_PKCS1_KEY ();
         SCHANNEL_ASSERT_PKCS1_KEY_NOT_IMPORTED ();
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL); // Imports key.
         SCHANNEL_ASSERT_PKCS1_KEY_IMPORTED ();
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      // Test SChannel key is previously imported:
      {
         SCHANNEL_ASSERT_PKCS1_KEY_IMPORTED ();
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      ASSERT_OR_PRINT (ok, error);
      mongoc_uri_destroy (uri);
   }

   // Test auth fails with no client certificate:
   {
      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;
      {
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      ASSERT (!ok);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CLIENT,
                             MONGOC_ERROR_CLIENT_AUTHENTICATE,
                             "" /* message differs between server versions */);
      mongoc_uri_destroy (uri);
   }

   // Test auth works with explicit username:
   {
      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (mongoc_uri_set_username (uri, "C=US,ST=New York,L=New York City,O=MDB,OU=Drivers,CN=client"));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, CERT_CLIENT));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;
      {
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      ASSERT_OR_PRINT (ok, error);
      mongoc_uri_destroy (uri);
   }

   // Test auth fails with wrong username:
   {
      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (mongoc_uri_set_username (uri, "bad"));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, CERT_CLIENT));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;
      {
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      ASSERT (!ok);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CLIENT,
                             MONGOC_ERROR_CLIENT_AUTHENTICATE,
                             "" /* message differs between server versions */);
      mongoc_uri_destroy (uri);
   }

   // Test auth fails with correct username but wrong certificate:
   {
      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (mongoc_uri_set_username (uri, "C=US,ST=New York,L=New York City,O=MDB,OU=Drivers,CN=client"));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, CERT_SERVER));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;
      {
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
         mongoc_client_destroy (client);
      }

      ASSERT (!ok);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CLIENT,
                             MONGOC_ERROR_CLIENT_AUTHENTICATE,
                             "" /* message differs between server versions */);
      mongoc_uri_destroy (uri);
   }

   // Test auth fails when client certificate does not contain public certificate:
   {
      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (
            mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, CERT_TEST_DIR "/client-private.pem"));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
         ASSERT (mongoc_uri_set_option_as_bool (uri, MONGOC_URI_SERVERSELECTIONTRYONCE, true)); // Fail quickly.
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;
      {
         capture_logs (true); // Capture logs before connecting. OpenSSL reads PEM file during client construction.
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
#if defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
         ASSERT_CAPTURED_LOG ("tls", MONGOC_LOG_LEVEL_ERROR, "Type is not supported");
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
         ASSERT_CAPTURED_LOG ("tls", MONGOC_LOG_LEVEL_ERROR, "Can't find public certificate");
#elif defined(MONGOC_ENABLE_SSL_OPENSSL)
         ASSERT_CAPTURED_LOG ("tls", MONGOC_LOG_LEVEL_ERROR, "Cannot find certificate");
#endif
         mongoc_client_destroy (client);
      }

      ASSERT (!ok);
#if defined(MONGOC_ENABLE_SSL_OPENSSL) || defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
      // OpenSSL and Secure Transport fail to create stream (prior to TLS). Resulting in a server selection error.
      ASSERT_ERROR_CONTAINS (
         error, MONGOC_ERROR_SERVER_SELECTION, MONGOC_ERROR_SERVER_SELECTION_FAILURE, "connection error");
#else
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CLIENT,
                             MONGOC_ERROR_CLIENT_AUTHENTICATE,
                             "" /* message differs between server versions */);
#endif
      mongoc_uri_destroy (uri);
   }

   // Test auth fails when client certificate does not exist:
   {
      // Create URI:
      mongoc_uri_t *uri = get_x509_uri ();
      {
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, CERT_TEST_DIR "/foobar.pem"));
         ASSERT (mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, CERT_CA));
         ASSERT (mongoc_uri_set_option_as_bool (uri, MONGOC_URI_SERVERSELECTIONTRYONCE, true)); // Fail quickly.
      }

      // Try auth:
      bson_error_t error = {0};
      bool ok;
      {
         capture_logs (true);
         mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
         ok = try_insert (client, &error);
#if defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT) || defined(MONGOC_ENABLE_SSL_OPENSSL)
         ASSERT_CAPTURED_LOG ("tls", MONGOC_LOG_LEVEL_ERROR, "Cannot find certificate");
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
         ASSERT_CAPTURED_LOG ("tls", MONGOC_LOG_LEVEL_ERROR, "Failed to open file");
#endif
         mongoc_client_destroy (client);
      }

      ASSERT (!ok);
#if defined(MONGOC_ENABLE_SSL_OPENSSL) || defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
      // OpenSSL fails to create stream (prior to TLS). Resulting in a server selection error.
      ASSERT_ERROR_CONTAINS (
         error, MONGOC_ERROR_SERVER_SELECTION, MONGOC_ERROR_SERVER_SELECTION_FAILURE, "connection error");
#else
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CLIENT,
                             MONGOC_ERROR_CLIENT_AUTHENTICATE,
                             "" /* message differs between server versions */);
#endif
      mongoc_uri_destroy (uri);
   }
   drop_x509_user (false);
}

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
static void
remove_crl_for_secure_channel (const char *crl_path)
{
   // Load CRL from file to query system store.
   PCCRL_CONTEXT crl_from_file = mongoc_secure_channel_load_crl (crl_path);
   ASSERT (crl_from_file);

   HCERTSTORE cert_store = CertOpenStore (CERT_STORE_PROV_SYSTEM,                  /* provider */
                                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, /* certificate encoding */
                                          0,                                       /* unused */
                                          CERT_SYSTEM_STORE_LOCAL_MACHINE,         /* dwFlags */
                                          L"Root"); /* system store name. "My" or "Root" */
   ASSERT (cert_store);

   PCCRL_CONTEXT crl_from_store = CertFindCRLInStore (cert_store, 0, 0, CRL_FIND_EXISTING, crl_from_file, NULL);
   ASSERT (crl_from_store);

   if (!CertDeleteCRLFromStore (crl_from_store)) {
      test_error (
         "Failed to delete CRL from store. Delete CRL manually to avoid test errors verifying server certificate.");
   }
   CertFreeCRLContext (crl_from_file);
   CertFreeCRLContext (crl_from_store);
   CertCloseStore (cert_store, 0);
}
#endif // MONGOC_ENABLE_SSL_SECURE_CHANNEL

// test_crl tests connection fails when server certificate is in CRL list.
static void
test_crl (void *unused)
{
   BSON_UNUSED (unused);

#if defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   if (!test_framework_getenv_bool ("MONGOC_TEST_SCHANNEL_CRL")) {
      printf ("Skipping. Test temporarily adds CRL to Windows certificate store. If removing the CRL fails, this may "
              "cause later test failures and require removing the CRL file manually. To run test anyway, set the "
              "environment variable MONGOC_TEST_SCHANNEL_CRL=ON\n");
      return;
   }
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
   printf ("Skipping. Secure Transport does not support crl_file.\n");
   return;
#endif

   // Create URI:
   mongoc_uri_t *uri = test_framework_get_uri ();
   ASSERT (mongoc_uri_set_option_as_bool (uri, MONGOC_URI_SERVERSELECTIONTRYONCE, true)); // Fail quickly.

   // Create SSL options with CRL file:
   mongoc_ssl_opt_t ssl_opts = *test_framework_get_ssl_opts ();
   ssl_opts.crl_file = CERT_TEST_DIR "/crl.pem";

   // Try insert:
   bson_error_t error = {0};
   mongoc_client_t *client = test_framework_client_new_from_uri (uri, NULL);
   mongoc_client_set_ssl_opts (client, &ssl_opts);
   capture_logs (true);
   bool ok = try_insert (client, &error);
#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
   remove_crl_for_secure_channel (ssl_opts.crl_file);
   ASSERT_CAPTURED_LOG ("tls", MONGOC_LOG_LEVEL_ERROR, "The certificate is revoked");
#else
   ASSERT_NO_CAPTURED_LOGS ("tls");
#endif
   ASSERT (!ok);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_SERVER_SELECTION, MONGOC_ERROR_SERVER_SELECTION_FAILURE, "no suitable servers");

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
}
#endif // MONGOC_ENABLE_SSL

void
test_x509_install (TestSuite *suite)
{
#ifdef MONGOC_ENABLE_SSL
   TestSuite_AddFull (suite,
                      "/X509/auth",
                      test_x509_auth,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_auth,
                      test_framework_skip_if_no_server_ssl);
   TestSuite_AddFull (suite, "/X509/crl", test_crl, NULL, NULL, test_framework_skip_if_no_server_ssl);
#endif

#ifdef MONGOC_ENABLE_OCSP_OPENSSL
   TestSuite_Add (suite, "/X509/tlsfeature_parsing", test_tlsfeature_parsing);
#endif
}
