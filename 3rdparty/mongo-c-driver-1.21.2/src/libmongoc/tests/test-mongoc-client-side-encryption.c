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

#include "json-test.h"
#include "test-libmongoc.h"

/* _mongoc_host_list_from_string_with_err */
#include "mongoc/mongoc-host-list-private.h"

#include "mongoc/mongoc-uri.h"

static void
_before_test (json_test_ctx_t *ctx, const bson_t *test)
{
   mongoc_client_t *client;
   mongoc_collection_t *keyvault_coll;
   bson_iter_t iter;
   bson_error_t error;
   bool ret;
   mongoc_write_concern_t *wc;
   bson_t insert_opts;

   /* Insert data into the key vault. */
   client = test_framework_new_default_client ();
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_wmajority (wc, 1000);
   bson_init (&insert_opts);
   mongoc_write_concern_append (wc, &insert_opts);

   if (bson_iter_init_find (&iter, ctx->config->scenario, "key_vault_data")) {
      keyvault_coll =
         mongoc_client_get_collection (client, "keyvault", "datakeys");

      /* Drop and recreate, inserting data. */
      ret = mongoc_collection_drop (keyvault_coll, &error);
      if (!ret) {
         /* Ignore "namespace does not exist" error. */
         ASSERT_OR_PRINT (error.code == 26, error);
      }

      bson_iter_recurse (&iter, &iter);
      while (bson_iter_next (&iter)) {
         bson_t doc;

         bson_iter_bson (&iter, &doc);
         ret = mongoc_collection_insert_one (
            keyvault_coll, &doc, &insert_opts, NULL /* reply */, &error);
         ASSERT_OR_PRINT (ret, error);
      }
      mongoc_collection_destroy (keyvault_coll);
   }

   /* Collmod to include the json schema. Data was already inserted. */
   if (bson_iter_init_find (&iter, ctx->config->scenario, "json_schema")) {
      bson_t *cmd;
      bson_t json_schema;

      bson_iter_bson (&iter, &json_schema);
      cmd = BCON_NEW ("collMod",
                      BCON_UTF8 (mongoc_collection_get_name (ctx->collection)),
                      "validator",
                      "{",
                      "$jsonSchema",
                      BCON_DOCUMENT (&json_schema),
                      "}");
      ret = mongoc_client_command_simple (
         client, mongoc_database_get_name (ctx->db), cmd, NULL, NULL, &error);
      ASSERT_OR_PRINT (ret, error);
      bson_destroy (cmd);
   }

   bson_destroy (&insert_opts);
   mongoc_write_concern_destroy (wc);
   mongoc_client_destroy (client);
}

static bool
_run_operation (json_test_ctx_t *ctx,
                const bson_t *test,
                const bson_t *operation)
{
   bson_t reply;
   bool res;

   res =
      json_test_operation (ctx, test, operation, ctx->collection, NULL, &reply);

   bson_destroy (&reply);

   return res;
}

static void
test_client_side_encryption_cb (bson_t *scenario)
{
   json_test_config_t config = JSON_TEST_CONFIG_INIT;
   config.before_test_cb = _before_test;
   config.run_operation_cb = _run_operation;
   config.scenario = scenario;
   config.command_started_events_only = true;
   config.command_monitoring_allow_subset = false;
   run_json_general_test (&config);
}

/* This is the hex form of the base64 encoded value:
 * Mng0NCt4ZHVUYUJCa1kxNkVyNUR1QURhZ2h2UzR2d2RrZzh0cFBwM3R6NmdWMDFBMUN3YkQ5aXRRMkhGRGdQV09wOGVNYUMxT2k3NjZKelhaQmRCZGJkTXVyZG9uSjFk
 * From the client side encryption spec.
 */
#define LOCAL_MASTERKEY                                                       \
   "\x32\x78\x34\x34\x2b\x78\x64\x75\x54\x61\x42\x42\x6b\x59\x31\x36\x45\x72" \
   "\x35\x44\x75\x41\x44\x61\x67\x68\x76\x53\x34\x76\x77\x64\x6b\x67\x38\x74" \
   "\x70\x50\x70\x33\x74\x7a\x36\x67\x56\x30\x31\x41\x31\x43\x77\x62\x44\x39" \
   "\x69\x74\x51\x32\x48\x46\x44\x67\x50\x57\x4f\x70\x38\x65\x4d\x61\x43\x31" \
   "\x4f\x69\x37\x36\x36\x4a\x7a\x58\x5a\x42\x64\x42\x64\x62\x64\x4d\x75\x72" \
   "\x64\x6f\x6e\x4a\x31\x64"

/* Convenience helper to check if spawning mongocryptd should be bypassed */
static void
_check_bypass (mongoc_auto_encryption_opts_t *opts)
{
   if (test_framework_getenv_bool ("MONGOC_TEST_MONGOCRYPTD_BYPASS_SPAWN")) {
      bson_t *extra;

      extra = BCON_NEW ("mongocryptdBypassSpawn", BCON_BOOL (true));
      mongoc_auto_encryption_opts_set_extra (opts, extra);
      bson_destroy (extra);
   }
}

static bson_t *
_make_aws_kms_provider (bson_t *kms_providers)
{
   char *aws_secret_access_key;
   char *aws_access_key_id;

   aws_secret_access_key =
      test_framework_getenv_required ("MONGOC_TEST_AWS_SECRET_ACCESS_KEY");
   aws_access_key_id =
      test_framework_getenv_required ("MONGOC_TEST_AWS_ACCESS_KEY_ID");

   if (!kms_providers) {
      kms_providers = bson_new ();
   }

   bson_concat (
      kms_providers,
      tmp_bson ("{ 'aws': { 'secretAccessKey': '%s', 'accessKeyId': '%s' }}",
                aws_secret_access_key,
                aws_access_key_id));

   bson_free (aws_secret_access_key);
   bson_free (aws_access_key_id);

   return kms_providers;
}

static bson_t *
_make_azure_kms_provider (bson_t *kms_providers)
{
   char *azure_tenant_id;
   char *azure_client_id;
   char *azure_client_secret;

   azure_tenant_id =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_TENANT_ID");
   azure_client_id =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_CLIENT_ID");
   azure_client_secret =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_CLIENT_SECRET");

   if (!kms_providers) {
      kms_providers = bson_new ();
   }

   bson_concat (kms_providers,
                tmp_bson ("{ 'azure': { 'tenantId': '%s', 'clientId': '%s', "
                          "'clientSecret': '%s' }}",
                          azure_tenant_id,
                          azure_client_id,
                          azure_client_secret));

   bson_free (azure_tenant_id);
   bson_free (azure_client_id);
   bson_free (azure_client_secret);

   return kms_providers;
}

static bson_t *
_make_gcp_kms_provider (bson_t *kms_providers)
{
   char *gcp_email;
   char *gcp_privatekey;


   gcp_email = test_framework_getenv_required ("MONGOC_TEST_GCP_EMAIL");
   gcp_privatekey =
      test_framework_getenv_required ("MONGOC_TEST_GCP_PRIVATEKEY");

   if (!gcp_email || !gcp_privatekey) {
      fprintf (stderr,
               "Set MONGOC_TEST_GCP_EMAIL and MONGOC_TEST_GCP_PRIVATEKEY to "
               "enable CSFLE tests.");
      abort ();
   }

   if (!kms_providers) {
      kms_providers = bson_new ();
   }

   bson_concat (kms_providers,
                tmp_bson ("{ 'gcp': { 'email': '%s', 'privateKey': '%s' }}",
                          gcp_email,
                          gcp_privatekey));

   bson_free (gcp_email);
   bson_free (gcp_privatekey);

   return kms_providers;
}

static bson_t *
_make_local_kms_provider (bson_t *kms_providers)
{
   bson_t *local = BCON_NEW (
      "local", "{", "key", BCON_BIN (0, (uint8_t *) LOCAL_MASTERKEY, 96), "}");

   if (!kms_providers) {
      kms_providers = bson_new ();
   }

   bson_concat (kms_providers, local);
   bson_destroy (local);

   return kms_providers;
}

static bson_t *
_make_kmip_kms_provider (bson_t *kms_providers)
{
   if (!kms_providers) {
      kms_providers = bson_new ();
   }

   bson_concat (kms_providers,
                tmp_bson ("{ 'kmip': { 'endpoint': 'localhost:5698' } }"));


   return kms_providers;
}

/* Convenience helper for creating KMS providers doc */
static bson_t *
_make_kms_providers (bool with_aws, bool with_local)
{
   bson_t *kms_providers = bson_new ();

   if (with_aws) {
      _make_aws_kms_provider (kms_providers);
      _make_azure_kms_provider (kms_providers);
      _make_gcp_kms_provider (kms_providers);
      _make_kmip_kms_provider (kms_providers);
   }

   if (with_local) {
      _make_local_kms_provider (kms_providers);
   }

   return kms_providers;
}

static bson_t *
_make_tls_opts (void)
{
   bson_t *tls_opts = bson_new ();
   char *kmip_tls_ca_file;
   char *kmip_tls_certificate_key_file;

   kmip_tls_ca_file =
      test_framework_getenv_required ("MONGOC_TEST_CSFLE_TLS_CA_FILE");
   kmip_tls_certificate_key_file = test_framework_getenv_required (
      "MONGOC_TEST_CSFLE_TLS_CERTIFICATE_KEY_FILE");

   bson_concat (
      tls_opts,
      tmp_bson (
         "{ 'kmip': {  'tlsCAFile': '%s', 'tlsCertificateKeyFile': '%s' } }",
         kmip_tls_ca_file,
         kmip_tls_certificate_key_file));

   bson_free (kmip_tls_ca_file);
   bson_free (kmip_tls_certificate_key_file);
   return tls_opts;
}

typedef struct {
   int num_inserts;
} limits_apm_ctx_t;

static void
_command_started (const mongoc_apm_command_started_t *event)
{
   limits_apm_ctx_t *ctx;

   ctx = (limits_apm_ctx_t *) mongoc_apm_command_started_get_context (event);
   if (0 ==
       strcmp ("insert", mongoc_apm_command_started_get_command_name (event))) {
      ctx->num_inserts++;
   }
}

/* Prose test: BSON size limits and batch splitting */
static void
test_bson_size_limits_and_batch_splitting (void *unused)
{
   /* Expect an insert of two documents over 2MiB to split into two inserts but
    * still succeed. */
   mongoc_client_t *client;
   mongoc_auto_encryption_opts_t *opts;
   mongoc_uri_t *uri;
   mongoc_collection_t *coll;
   bson_error_t error;
   bson_t *corpus_schema;
   bson_t *datakey;
   bson_t *cmd;
   bson_t *kms_providers;
   bson_t *docs[2];
   char *as;
   limits_apm_ctx_t ctx = {0};
   mongoc_apm_callbacks_t *callbacks;
   /* Values from the spec to test boundaries. */
   const int size_16mib = 16777216;
   const int size_2mib = 2097152;
   const int exceeds_2mib_after_encryption = size_2mib - 2000;
   const int exceeds_16mib_after_encryption = size_16mib - 2000;

   /* Do the test setup. */

   /* Drop and create db.coll configured with limits-schema.json */
   uri = test_framework_get_uri ();
   client = test_framework_client_new_from_uri (uri, NULL);
   test_framework_set_ssl_opts (client);
   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);
   coll = mongoc_client_get_collection (client, "db", "coll");
   (void) mongoc_collection_drop (coll, NULL);
   corpus_schema = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/limits-schema.json");
   cmd = BCON_NEW ("create",
                   "coll",
                   "validator",
                   "{",
                   "$jsonSchema",
                   BCON_DOCUMENT (corpus_schema),
                   "}");
   ASSERT_OR_PRINT (
      mongoc_client_command_simple (
         client, "db", cmd, NULL /* read prefs */, NULL /* reply */, &error),
      error);

   /* Drop and create the key vault collection, keyvault.datakeys. */
   mongoc_collection_destroy (coll);
   coll = mongoc_client_get_collection (client, "keyvault", "datakeys");
   (void) mongoc_collection_drop (coll, NULL);
   datakey = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/limits-key.json");
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (
         coll, datakey, NULL /* opts */, NULL /* reply */, &error),
      error);

   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);

   client = test_framework_client_new_from_uri (uri, NULL);
   test_framework_set_ssl_opts (client);
   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);

   kms_providers = _make_kms_providers (false /* aws */, true /* local */);
   opts = mongoc_auto_encryption_opts_new ();
   _check_bypass (opts);
   mongoc_auto_encryption_opts_set_keyvault_namespace (
      opts, "keyvault", "datakeys");
   mongoc_auto_encryption_opts_set_kms_providers (opts, kms_providers);

   ASSERT_OR_PRINT (mongoc_client_enable_auto_encryption (client, opts, &error),
                    error);

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (callbacks, _command_started);
   mongoc_client_set_apm_callbacks (client, callbacks, &ctx);

   coll = mongoc_client_get_collection (client, "db", "coll");
   /* End of setup */

   /* Insert { "_id": "over_2mib_under_16mib", "unencrypted": <the string "a"
    * repeated 2097152 times> } */
   docs[0] = BCON_NEW ("_id", "over_2mib_under_16mib");
   as = bson_malloc (size_16mib);
   memset (as, 'a', size_16mib);
   bson_append_utf8 (docs[0], "unencrypted", -1, as, 2097152);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (
         coll, docs[0], NULL /* opts */, NULL /* reply */, &error),
      error);
   bson_destroy (docs[0]);

   /* Insert the document `limits/limits-doc.json <../limits/limits-doc.json>`_
    * concatenated with ``{ "_id": "encryption_exceeds_2mib", "unencrypted": <
    * the string "a" repeated (2097152 - 2000) times > }`` */
   docs[0] = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/limits-doc.json");
   bson_append_utf8 (docs[0], "_id", -1, "encryption_exceeds_2mib", -1);
   bson_append_utf8 (
      docs[0], "unencrypted", -1, as, exceeds_2mib_after_encryption);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (
         coll, docs[0], NULL /* opts */, NULL /* reply */, &error),
      error);
   bson_destroy (docs[0]);

   /* Insert two documents that each exceed 2MiB but no encryption occurs.
    * Expect the bulk write to succeed and run as two separate inserts.
    */
   docs[0] = BCON_NEW ("_id", "over_2mib_1");
   bson_append_utf8 (docs[0], "unencrypted", -1, as, size_2mib);
   docs[1] = BCON_NEW ("_id", "over_2mib_2");
   bson_append_utf8 (docs[1], "unencrypted", -1, as, size_2mib);
   ctx.num_inserts = 0;
   ASSERT_OR_PRINT (mongoc_collection_insert_many (coll,
                                                   (const bson_t **) docs,
                                                   2,
                                                   NULL /* opts */,
                                                   NULL /* reply */,
                                                   &error),
                    error);
   ASSERT_CMPINT (ctx.num_inserts, ==, 2);
   bson_destroy (docs[0]);
   bson_destroy (docs[1]);

   /* Insert two documents that each exceed 2MiB after encryption occurs. Expect
    * the bulk write to succeed and run as two separate inserts.
    */

   docs[0] = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/limits-doc.json");
   bson_append_utf8 (docs[0], "_id", -1, "encryption_exceeds_2mib_1", -1);
   bson_append_utf8 (
      docs[0], "unencrypted", -1, as, exceeds_2mib_after_encryption);
   docs[1] = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/limits-doc.json");
   bson_append_utf8 (docs[1], "_id", -1, "encryption_exceeds_2mib_2", -1);
   bson_append_utf8 (
      docs[1], "unencrypted", -1, as, exceeds_2mib_after_encryption);
   ctx.num_inserts = 0;
   ASSERT_OR_PRINT (mongoc_collection_insert_many (coll,
                                                   (const bson_t **) docs,
                                                   2,
                                                   NULL /* opts */,
                                                   NULL /* reply */,
                                                   &error),
                    error);
   ASSERT_CMPINT (ctx.num_inserts, ==, 2);
   bson_destroy (docs[0]);
   bson_destroy (docs[1]);

   /* Check that inserting close to, but not exceeding, 16MiB, passes */
   docs[0] = bson_new ();
   bson_append_utf8 (docs[0], "_id", -1, "under_16mib", -1);
   bson_append_utf8 (
      docs[0], "unencrypted", -1, as, exceeds_16mib_after_encryption);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (
         coll, docs[0], NULL /* opts */, NULL /* reply */, &error),
      error);
   bson_destroy (docs[0]);

   /* but.. exceeding 16 MiB fails */
   docs[0] = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/limits-doc.json");
   bson_append_utf8 (docs[0], "_id", -1, "under_16mib", -1);
   bson_append_utf8 (
      docs[0], "unencrypted", -1, as, exceeds_16mib_after_encryption);
   BSON_ASSERT (!mongoc_collection_insert_one (
      coll, docs[0], NULL /* opts */, NULL /* reply */, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 2, "too large");
   bson_destroy (docs[0]);

   bson_free (as);
   bson_destroy (kms_providers);
   bson_destroy (corpus_schema);
   bson_destroy (cmd);
   bson_destroy (datakey);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_auto_encryption_opts_destroy (opts);
}

typedef struct {
   bson_t *last_cmd;
} _datakey_and_double_encryption_ctx_t;

static void
_datakey_and_double_encryption_command_started (
   const mongoc_apm_command_started_t *event)
{
   _datakey_and_double_encryption_ctx_t *ctx;

   ctx = (_datakey_and_double_encryption_ctx_t *)
      mongoc_apm_command_started_get_context (event);
   bson_destroy (ctx->last_cmd);
   ctx->last_cmd = bson_copy (mongoc_apm_command_started_get_command (event));
}

static void
test_datakey_and_double_encryption_creating_and_using (
   mongoc_client_encryption_t *client_encryption,
   mongoc_client_t *client,
   mongoc_client_t *client_encrypted,
   const char *kms_provider,
   _datakey_and_double_encryption_ctx_t *test_ctx)
{
   bson_value_t keyid;
   bson_error_t error;
   bool ret;
   mongoc_client_encryption_datakey_opts_t *opts;
   mongoc_client_encryption_encrypt_opts_t *encrypt_opts;
   char *altname;
   mongoc_collection_t *coll;
   mongoc_cursor_t *cursor;
   bson_t filter;
   const bson_t *doc;
   bson_value_t to_encrypt;
   bson_value_t encrypted;
   bson_value_t encrypted_via_altname;
   bson_t to_insert = BSON_INITIALIZER;
   char *hello;

   opts = mongoc_client_encryption_datakey_opts_new ();

   if (0 == strcmp (kms_provider, "aws")) {
      mongoc_client_encryption_datakey_opts_set_masterkey (
         opts,
         tmp_bson ("{ 'region': 'us-east-1', 'key': "
                   "'arn:aws:kms:us-east-1:579766882180:key/"
                   "89fcc2c4-08b0-4bd9-9f25-e30687b580d0' }"));
   } else if (0 == strcmp (kms_provider, "azure")) {
      mongoc_client_encryption_datakey_opts_set_masterkey (
         opts,
         tmp_bson ("{'keyVaultEndpoint': 'key-vault-csfle.vault.azure.net', "
                   "'keyName': 'key-name-csfle'}"));
   } else if (0 == strcmp (kms_provider, "gcp")) {
      mongoc_client_encryption_datakey_opts_set_masterkey (
         opts,
         tmp_bson ("{'projectId': 'devprod-drivers','location': "
                   "'global','keyRing': 'key-ring-csfle','keyName': "
                   "'key-name-csfle'}"));
   } else if (0 == strcmp (kms_provider, "kmip")) {
      mongoc_client_encryption_datakey_opts_set_masterkey (opts,
                                                           tmp_bson ("{}"));
   }

   altname = bson_strdup_printf ("%s_altname", kms_provider);
   mongoc_client_encryption_datakey_opts_set_keyaltnames (opts, &altname, 1);

   ret = mongoc_client_encryption_create_datakey (
      client_encryption, kms_provider, opts, &keyid, &error);
   ASSERT_OR_PRINT (ret, error);

   /* Expect a BSON binary with subtype 4 to be returned */
   BSON_ASSERT (keyid.value_type == BSON_TYPE_BINARY);
   BSON_ASSERT (keyid.value.v_binary.subtype == BSON_SUBTYPE_UUID);

   /* Check that client captured a command_started event for the insert command
    * containing a majority writeConcern. */
   BSON_ASSERT (match_bson (
      test_ctx->last_cmd,
      tmp_bson ("{'insert': 'datakeys', 'writeConcern': { 'w': 'majority' } }"),
      false));

   /* Use client to run a find on keyvault.datakeys */
   coll = mongoc_client_get_collection (client, "keyvault", "datakeys");
   bson_init (&filter);
   BSON_APPEND_VALUE (&filter, "_id", &keyid);
   cursor = mongoc_collection_find_with_opts (
      coll, &filter, NULL /* opts */, NULL /* read prefs */);
   mongoc_collection_destroy (coll);

   /* Expect that exactly one document is returned with the "masterKey.provider"
    * equal to <kms_provider> */
   BSON_ASSERT (mongoc_cursor_next (cursor, &doc));
   BSON_ASSERT (
      0 == strcmp (kms_provider, bson_lookup_utf8 (doc, "masterKey.provider")));
   BSON_ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   /* Call client_encryption.encrypt() with the value "hello <kms provider>" */
   encrypt_opts = mongoc_client_encryption_encrypt_opts_new ();
   mongoc_client_encryption_encrypt_opts_set_algorithm (
      encrypt_opts, MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_DETERMINISTIC);
   mongoc_client_encryption_encrypt_opts_set_keyid (encrypt_opts, &keyid);

   hello = bson_strdup_printf ("hello %s", kms_provider);

   to_encrypt.value_type = BSON_TYPE_UTF8;
   to_encrypt.value.v_utf8.str = bson_strdup (hello);
   to_encrypt.value.v_utf8.len = strlen (to_encrypt.value.v_utf8.str);

   ret = mongoc_client_encryption_encrypt (
      client_encryption, &to_encrypt, encrypt_opts, &encrypted, &error);
   ASSERT_OR_PRINT (ret, error);
   mongoc_client_encryption_encrypt_opts_destroy (encrypt_opts);

   /* Expect the return value to be a BSON binary subtype 6 */
   BSON_ASSERT (encrypted.value_type == BSON_TYPE_BINARY);
   BSON_ASSERT (encrypted.value.v_binary.subtype == BSON_SUBTYPE_ENCRYPTED);

   /* Use client_encrypted to insert { _id: "<kms provider>", "value":
    * <encrypted> } into db.coll */
   coll = mongoc_client_get_collection (client_encrypted, "db", "coll");
   BSON_APPEND_UTF8 (&to_insert, "_id", kms_provider);
   BSON_APPEND_VALUE (&to_insert, "value", &encrypted);
   ret = mongoc_collection_insert_one (
      coll, &to_insert, NULL /* opts */, NULL /* reply */, &error);
   ASSERT_OR_PRINT (ret, error);

   /* Use client_encrypted to run a find querying with _id of <kms_provider> and
    * expect value to be "hello <kms_provider>". */
   cursor = mongoc_collection_find_with_opts (
      coll,
      tmp_bson ("{ '_id': '%s' }", kms_provider),
      NULL /* opts */,
      NULL /* read prefs */);
   BSON_ASSERT (mongoc_cursor_next (cursor, &doc));
   BSON_ASSERT (0 == strcmp (hello, bson_lookup_utf8 (doc, "value")));
   BSON_ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (coll);

   /* Call client_encryption.encrypt() with the value "hello <kms provider>",
    * the algorithm AEAD_AES_256_CBC_HMAC_SHA_512-Deterministic, and the
    * key_alt_name of <kms provider>_altname. */
   encrypt_opts = mongoc_client_encryption_encrypt_opts_new ();
   mongoc_client_encryption_encrypt_opts_set_algorithm (
      encrypt_opts, MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_DETERMINISTIC);
   mongoc_client_encryption_encrypt_opts_set_keyaltname (encrypt_opts, altname);

   ret = mongoc_client_encryption_encrypt (client_encryption,
                                           &to_encrypt,
                                           encrypt_opts,
                                           &encrypted_via_altname,
                                           &error);
   ASSERT_OR_PRINT (ret, error);
   mongoc_client_encryption_encrypt_opts_destroy (encrypt_opts);

   /* Expect the return value to be a BSON binary subtype 6. Expect the value to
    * exactly match the value of encrypted. */
   BSON_ASSERT (encrypted_via_altname.value_type == BSON_TYPE_BINARY);
   BSON_ASSERT (encrypted_via_altname.value.v_binary.subtype ==
                BSON_SUBTYPE_ENCRYPTED);
   BSON_ASSERT (encrypted_via_altname.value.v_binary.data_len ==
                encrypted.value.v_binary.data_len);
   BSON_ASSERT (0 == memcmp (encrypted_via_altname.value.v_binary.data,
                             encrypted.value.v_binary.data,
                             encrypted.value.v_binary.data_len));

   bson_value_destroy (&encrypted);
   bson_value_destroy (&encrypted_via_altname);
   bson_free (hello);
   bson_destroy (&to_insert);
   bson_value_destroy (&to_encrypt);
   bson_value_destroy (&keyid);
   bson_free (altname);
   bson_destroy (&filter);
   mongoc_client_encryption_datakey_opts_destroy (opts);
}

/* Prose test "Data key and double encryption" */
static void
test_datakey_and_double_encryption (void *unused)
{
   mongoc_client_t *client;
   mongoc_client_t *client_encrypted;
   mongoc_client_encryption_t *client_encryption;
   mongoc_apm_callbacks_t *callbacks;
   mongoc_collection_t *coll;
   bson_error_t error;
   bson_t *kms_providers;
   bson_t *tls_opts;
   mongoc_auto_encryption_opts_t *auto_encryption_opts;
   mongoc_client_encryption_opts_t *client_encryption_opts;
   bson_t *schema_map;
   bool ret;
   _datakey_and_double_encryption_ctx_t test_ctx = {0};

   /* Test setup */
   /* Create a MongoClient without encryption enabled (referred to as client).
    * Enable command monitoring to listen for command_started events. */
   client = test_framework_new_default_client ();
   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_command_started_cb (
      callbacks, _datakey_and_double_encryption_command_started);
   mongoc_client_set_apm_callbacks (client, callbacks, &test_ctx);

   /* Using client, drop the collections keyvault.datakeys and db.coll. */
   coll = mongoc_client_get_collection (client, "keyvault", "datakeys");
   (void) mongoc_collection_drop (coll, NULL);
   mongoc_collection_destroy (coll);
   coll = mongoc_client_get_collection (client, "db", "coll");
   (void) mongoc_collection_drop (coll, NULL);
   mongoc_collection_destroy (coll);

   /* Create a MongoClient configured with auto encryption (referred to as
    * client_encrypted) */
   auto_encryption_opts = mongoc_auto_encryption_opts_new ();
   kms_providers = _make_kms_providers (true /* aws */, true /* local */);
   tls_opts = _make_tls_opts ();
   _check_bypass (auto_encryption_opts);
   mongoc_auto_encryption_opts_set_kms_providers (auto_encryption_opts,
                                                  kms_providers);
   mongoc_auto_encryption_opts_set_tls_opts (auto_encryption_opts, tls_opts);
   mongoc_auto_encryption_opts_set_keyvault_namespace (
      auto_encryption_opts, "keyvault", "datakeys");
   schema_map = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/"
      "datakey-and-double-encryption-schemamap.json");
   mongoc_auto_encryption_opts_set_schema_map (auto_encryption_opts,
                                               schema_map);

   client_encrypted = test_framework_new_default_client ();
   ret = mongoc_client_enable_auto_encryption (
      client_encrypted, auto_encryption_opts, &error);
   ASSERT_OR_PRINT (ret, error);

   /* Create a ClientEncryption object (referred to as client_encryption) */
   client_encryption_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_kms_providers (client_encryption_opts,
                                                    kms_providers);
   mongoc_client_encryption_opts_set_tls_opts (client_encryption_opts,
                                               tls_opts);
   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                      client);
   client_encryption =
      mongoc_client_encryption_new (client_encryption_opts, &error);
   ASSERT_OR_PRINT (client_encryption, error);

   test_datakey_and_double_encryption_creating_and_using (
      client_encryption, client, client_encrypted, "local", &test_ctx);
   test_datakey_and_double_encryption_creating_and_using (
      client_encryption, client, client_encrypted, "aws", &test_ctx);
   test_datakey_and_double_encryption_creating_and_using (
      client_encryption, client, client_encrypted, "azure", &test_ctx);
   test_datakey_and_double_encryption_creating_and_using (
      client_encryption, client, client_encrypted, "gcp", &test_ctx);
   test_datakey_and_double_encryption_creating_and_using (
      client_encryption, client, client_encrypted, "kmip", &test_ctx);

   bson_destroy (kms_providers);
   bson_destroy (tls_opts);
   bson_destroy (schema_map);
   mongoc_client_encryption_opts_destroy (client_encryption_opts);
   mongoc_auto_encryption_opts_destroy (auto_encryption_opts);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_client_destroy (client);
   mongoc_client_destroy (client_encrypted);
   mongoc_client_encryption_destroy (client_encryption);
   bson_destroy (test_ctx.last_cmd);
}

static void
_test_key_vault (bool with_external_key_vault)
{
   mongoc_client_t *client;
   mongoc_client_t *client_encrypted;
   mongoc_client_t *client_external;
   mongoc_client_encryption_t *client_encryption;
   mongoc_uri_t *external_uri;
   mongoc_collection_t *coll;
   bson_t *datakey;
   bson_t *kms_providers;
   bson_error_t error;
   mongoc_write_concern_t *wc;
   bson_t *schema;
   bson_t *schema_map;
   mongoc_auto_encryption_opts_t *auto_encryption_opts;
   mongoc_client_encryption_opts_t *client_encryption_opts;
   mongoc_client_encryption_encrypt_opts_t *encrypt_opts;
   bool res;
   const bson_value_t *keyid;
   bson_value_t value;
   bson_value_t ciphertext;
   bson_iter_t iter;

   external_uri = test_framework_get_uri ();
   mongoc_uri_set_username (external_uri, "fake-user");
   mongoc_uri_set_password (external_uri, "fake-pwd");
   client_external = test_framework_client_new_from_uri (external_uri, NULL);
   test_framework_set_ssl_opts (client_external);

   /* Using client, drop the collections keyvault.datakeys and db.coll. */
   client = test_framework_new_default_client ();
   coll = mongoc_client_get_collection (client, "db", "coll");
   (void) mongoc_collection_drop (coll, NULL);
   mongoc_collection_destroy (coll);
   coll = mongoc_client_get_collection (client, "keyvault", "datakeys");
   (void) mongoc_collection_drop (coll, NULL);

   /* Insert the document external-key.json into ``keyvault.datakeys``. */
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_wmajority (wc, 1000);
   mongoc_collection_set_write_concern (coll, wc);
   datakey = get_bson_from_json_file ("./src/libmongoc/tests/"
                                      "client_side_encryption_prose/external/"
                                      "external-key.json");
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (coll, datakey, NULL, NULL, &error), error);
   mongoc_collection_destroy (coll);

   /* Create a MongoClient configured with auto encryption. */
   client_encrypted = test_framework_new_default_client ();
   mongoc_client_set_error_api (client_encrypted, MONGOC_ERROR_API_VERSION_2);
   auto_encryption_opts = mongoc_auto_encryption_opts_new ();
   _check_bypass (auto_encryption_opts);
   schema = get_bson_from_json_file ("./src/libmongoc/tests/"
                                     "client_side_encryption_prose/external/"
                                     "external-schema.json");
   schema_map = BCON_NEW ("db.coll", BCON_DOCUMENT (schema));
   kms_providers = _make_kms_providers (false /* aws */, true /* local */);
   mongoc_auto_encryption_opts_set_kms_providers (auto_encryption_opts,
                                                  kms_providers);
   mongoc_auto_encryption_opts_set_keyvault_namespace (
      auto_encryption_opts, "keyvault", "datakeys");
   mongoc_auto_encryption_opts_set_schema_map (auto_encryption_opts,
                                               schema_map);
   if (with_external_key_vault) {
      mongoc_auto_encryption_opts_set_keyvault_client (auto_encryption_opts,
                                                       client_external);
   }
   ASSERT_OR_PRINT (mongoc_client_enable_auto_encryption (
                       client_encrypted, auto_encryption_opts, &error),
                    error);

   /* Create a ClientEncryption object. */
   client_encryption_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_kms_providers (client_encryption_opts,
                                                    kms_providers);
   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts, "keyvault", "datakeys");
   if (with_external_key_vault) {
      mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                         client_external);
   } else {
      mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                         client);
   }
   client_encryption =
      mongoc_client_encryption_new (client_encryption_opts, &error);
   ASSERT_OR_PRINT (client_encryption, error);

   /* Use client_encrypted to insert the document {"encrypted": "test"} into
    * db.coll. */
   coll = mongoc_client_get_collection (client_encrypted, "db", "coll");
   res = mongoc_collection_insert_one (
      coll, tmp_bson ("{'encrypted': 'test'}"), NULL, NULL, &error);
   if (with_external_key_vault) {
      BSON_ASSERT (!res);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CLIENT,
                             MONGOC_ERROR_CLIENT_AUTHENTICATE,
                             "Authentication failed");
   } else {
      ASSERT_OR_PRINT (res, error);
   }

   /* Use client_encryption to explicitly encrypt the string "test" with key ID
    * ``LOCALAAAAAAAAAAAAAAAAA==`` and deterministic algorithm. */
   encrypt_opts = mongoc_client_encryption_encrypt_opts_new ();
   mongoc_client_encryption_encrypt_opts_set_algorithm (
      encrypt_opts, MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_DETERMINISTIC);
   BSON_ASSERT (bson_iter_init_find (&iter, datakey, "_id"));
   keyid = bson_iter_value (&iter);
   mongoc_client_encryption_encrypt_opts_set_keyid (encrypt_opts, keyid);
   value.value_type = BSON_TYPE_UTF8;
   value.value.v_utf8.str = "test";
   value.value.v_utf8.len = 4;
   res = mongoc_client_encryption_encrypt (
      client_encryption, &value, encrypt_opts, &ciphertext, &error);
   if (with_external_key_vault) {
      BSON_ASSERT (!res);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_CLIENT,
                             MONGOC_ERROR_CLIENT_AUTHENTICATE,
                             "Authentication failed");
   } else {
      ASSERT_OR_PRINT (res, error);
   }

   bson_destroy (schema);
   bson_destroy (schema_map);
   bson_destroy (datakey);
   bson_value_destroy (&ciphertext);
   mongoc_write_concern_destroy (wc);
   bson_destroy (kms_providers);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   mongoc_client_destroy (client_encrypted);
   mongoc_client_destroy (client_external);
   mongoc_uri_destroy (external_uri);
   mongoc_auto_encryption_opts_destroy (auto_encryption_opts);
   mongoc_client_encryption_opts_destroy (client_encryption_opts);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_encrypt_opts_destroy (encrypt_opts);
}

static void
test_external_key_vault (void *unused)
{
   _test_key_vault (false /* external */);
   _test_key_vault (true /* external */);
}

static void
test_views_are_prohibited (void *unused)
{
   mongoc_client_t *client;
   mongoc_client_t *client_encrypted;
   mongoc_collection_t *coll;
   bool res;
   bson_error_t error;

   mongoc_auto_encryption_opts_t *auto_encryption_opts;
   bson_t *kms_providers;

   client = test_framework_new_default_client ();

   /* Using client, drop and create a view named db.view with an empty pipeline.
    * E.g. using the command { "create": "view", "viewOn": "coll" }. */
   coll = mongoc_client_get_collection (client, "db", "view");
   (void) mongoc_collection_drop (coll, NULL);
   res = mongoc_client_command_simple (
      client,
      "db",
      tmp_bson ("{'create': 'view', 'viewOn': 'coll'}"),
      NULL,
      NULL,
      &error);
   ASSERT_OR_PRINT (res, error);

   client_encrypted = test_framework_new_default_client ();
   auto_encryption_opts = mongoc_auto_encryption_opts_new ();
   _check_bypass (auto_encryption_opts);
   kms_providers = _make_kms_providers (false /* aws */, true /* local */);
   mongoc_auto_encryption_opts_set_kms_providers (auto_encryption_opts,
                                                  kms_providers);
   mongoc_auto_encryption_opts_set_keyvault_namespace (
      auto_encryption_opts, "keyvault", "datakeys");
   ASSERT_OR_PRINT (mongoc_client_enable_auto_encryption (
                       client_encrypted, auto_encryption_opts, &error),
                    error);

   mongoc_collection_destroy (coll);
   coll = mongoc_client_get_collection (client_encrypted, "db", "view");
   res = mongoc_collection_insert_one (
      coll, tmp_bson ("{'x': 1}"), NULL, NULL, &error);
   BSON_ASSERT (!res);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
                          1,
                          "cannot auto encrypt a view");

   bson_destroy (kms_providers);
   mongoc_collection_destroy (coll);
   mongoc_auto_encryption_opts_destroy (auto_encryption_opts);
   mongoc_client_destroy (client_encrypted);
   mongoc_client_destroy (client);
}

static void
_endpoint_setup (mongoc_client_t *keyvault_client,
                 mongoc_client_encryption_t **client_encryption,
                 mongoc_client_encryption_t **client_encryption_invalid)
{
   bson_t *tls_opts;
   bson_t *kms_providers;
   bson_t *kms_providers_invalid;
   mongoc_client_encryption_opts_t *client_encryption_opts;
   mongoc_client_encryption_opts_t *client_encryption_opts_invalid;
   bson_error_t error;

   char *mongoc_test_aws_access_key_id =
      test_framework_getenv_required ("MONGOC_TEST_AWS_ACCESS_KEY_ID");
   char *mongoc_test_aws_secret_access_key =
      test_framework_getenv_required ("MONGOC_TEST_AWS_SECRET_ACCESS_KEY");
   char *mongoc_test_azure_tenant_id =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_TENANT_ID");
   char *mongoc_test_azure_client_id =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_CLIENT_ID");
   char *mongoc_test_azure_client_secret =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_CLIENT_SECRET");
   char *mongoc_test_gcp_email =
      test_framework_getenv_required ("MONGOC_TEST_GCP_EMAIL");
   char *mongoc_test_gcp_privatekey =
      test_framework_getenv_required ("MONGOC_TEST_GCP_PRIVATEKEY");
   char *ca_file =
      test_framework_getenv_required ("MONGOC_TEST_CSFLE_TLS_CA_FILE");
   char *certificate_key_file = test_framework_getenv_required (
      "MONGOC_TEST_CSFLE_TLS_CERTIFICATE_KEY_FILE");


   kms_providers =
      tmp_bson ("{'aws': {'accessKeyId': '%s', 'secretAccessKey': '%s'}}",
                mongoc_test_aws_access_key_id,
                mongoc_test_aws_secret_access_key);
   bson_concat (
      kms_providers,
      tmp_bson (
         "{'azure': {'tenantId': '%s', 'clientId': '%s', 'clientSecret': '%s', "
         "'identityPlatformEndpoint': 'login.microsoftonline.com:443'}}",
         mongoc_test_azure_tenant_id,
         mongoc_test_azure_client_id,
         mongoc_test_azure_client_secret));
   bson_concat (kms_providers,
                tmp_bson ("{'gcp': { 'email': '%s', 'privateKey': '%s', "
                          "'endpoint': 'oauth2.googleapis.com:443'}}",
                          mongoc_test_gcp_email,
                          mongoc_test_gcp_privatekey));
   bson_concat (kms_providers,
                tmp_bson ("{'kmip': { 'endpoint': 'localhost:5698' }}"));
   tls_opts = tmp_bson (
      "{'kmip': {  'tlsCAFile': '%s', 'tlsCertificateKeyFile': '%s' }}",
      ca_file,
      certificate_key_file);

   kms_providers_invalid =
      tmp_bson ("{'aws': {'accessKeyId': '%s', 'secretAccessKey': '%s'}}",
                mongoc_test_aws_access_key_id,
                mongoc_test_aws_secret_access_key);
   bson_concat (
      kms_providers_invalid,
      tmp_bson (
         "{'azure': {'tenantId': '%s', 'clientId': '%s', 'clientSecret': '%s', "
         "'identityPlatformEndpoint': 'example.com:443'}}",
         mongoc_test_azure_tenant_id,
         mongoc_test_azure_client_id,
         mongoc_test_azure_client_secret));
   bson_concat (kms_providers_invalid,
                tmp_bson ("{'gcp': { 'email': '%s', 'privateKey': '%s', "
                          "'endpoint': 'example.com'}}",
                          mongoc_test_gcp_email,
                          mongoc_test_gcp_privatekey));
   bson_concat (
      kms_providers_invalid,
      tmp_bson ("{'kmip': { 'endpoint': 'doesnotexist.local:5698' }}"));

   client_encryption_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_kms_providers (client_encryption_opts,
                                                    kms_providers);
   mongoc_client_encryption_opts_set_tls_opts (client_encryption_opts,
                                               tls_opts);
   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                      keyvault_client);
   *client_encryption =
      mongoc_client_encryption_new (client_encryption_opts, &error);
   ASSERT_OR_PRINT (client_encryption, error);

   client_encryption_opts_invalid = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_kms_providers (
      client_encryption_opts_invalid, kms_providers_invalid);
   mongoc_client_encryption_opts_set_tls_opts (client_encryption_opts_invalid,
                                               tls_opts);
   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts_invalid, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (
      client_encryption_opts_invalid, keyvault_client);
   *client_encryption_invalid =
      mongoc_client_encryption_new (client_encryption_opts_invalid, &error);
   ASSERT_OR_PRINT (client_encryption_invalid, error);

   mongoc_client_encryption_opts_destroy (client_encryption_opts);
   mongoc_client_encryption_opts_destroy (client_encryption_opts_invalid);
   bson_free (mongoc_test_aws_access_key_id);
   bson_free (mongoc_test_aws_secret_access_key);
   bson_free (mongoc_test_azure_tenant_id);
   bson_free (mongoc_test_azure_client_id);
   bson_free (mongoc_test_azure_client_secret);
   bson_free (mongoc_test_gcp_email);
   bson_free (mongoc_test_gcp_privatekey);
   bson_free (ca_file);
   bson_free (certificate_key_file);
}

/* Use the returned UUID of the key to explicitly encrypt and decrypt the
 * string "test" to validate it works. */
#define TEST_ENCRYPT_DECRYPT(keyid, client_encryption, res, error)           \
   do {                                                                      \
      bson_value_t ciphertext;                                               \
      bson_value_t plaintext;                                                \
      bson_value_t test;                                                     \
      mongoc_client_encryption_encrypt_opts_t *encrypt_opts;                 \
                                                                             \
      test.value_type = BSON_TYPE_UTF8;                                      \
      test.value.v_utf8.str = "test";                                        \
      test.value.v_utf8.len = 4;                                             \
                                                                             \
      encrypt_opts = mongoc_client_encryption_encrypt_opts_new ();           \
      mongoc_client_encryption_encrypt_opts_set_algorithm (                  \
         encrypt_opts, MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_DETERMINISTIC);  \
      mongoc_client_encryption_encrypt_opts_set_keyid (encrypt_opts, keyid); \
      res = mongoc_client_encryption_encrypt (                               \
         client_encryption, &test, encrypt_opts, &ciphertext, &error);       \
      ASSERT_OR_PRINT (res, error);                                          \
      res = mongoc_client_encryption_decrypt (                               \
         client_encryption, &ciphertext, &plaintext, &error);                \
      ASSERT_OR_PRINT (res, error);                                          \
      if (plaintext.value_type != BSON_TYPE_UTF8) {                          \
         test_error (                                                        \
            "expected decrypted result to be value type UTF-8, got %s",      \
            _mongoc_bson_type_to_str (plaintext.value_type));                \
      }                                                                      \
      ASSERT_CMPSTR (plaintext.value.v_utf8.str, test.value.v_utf8.str);     \
      bson_value_destroy (&ciphertext);                                      \
      bson_value_destroy (&plaintext);                                       \
      mongoc_client_encryption_encrypt_opts_destroy (encrypt_opts);          \
   } while (0)

static void
test_custom_endpoint (void *unused)
{
   mongoc_client_t *keyvault_client;
   mongoc_client_encryption_t *client_encryption = NULL;
   mongoc_client_encryption_t *client_encryption_invalid = NULL;
   mongoc_client_encryption_datakey_opts_t *datakey_opts;
   bson_error_t error;
   bool res;
   bson_t *masterkey;
   bson_value_t keyid;

   keyvault_client = test_framework_new_default_client ();

   datakey_opts = mongoc_client_encryption_datakey_opts_new ();

   /* Case 1: No endpoint, expect to succeed. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("region",
                         "us-east-1",
                         "key",
                         "arn:aws:kms:us-east-1:579766882180:key/"
                         "89fcc2c4-08b0-4bd9-9f25-e30687b580d0");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", datakey_opts, &keyid, &error);

   TEST_ENCRYPT_DECRYPT (&keyid, client_encryption, res, error);
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 2: Custom endpoint, with the same as the default. Expect to succeed
    */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("region",
                         "us-east-1",
                         "key",
                         "arn:aws:kms:us-east-1:579766882180:key/"
                         "89fcc2c4-08b0-4bd9-9f25-e30687b580d0",
                         "endpoint",
                         "kms.us-east-1.amazonaws.com");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", datakey_opts, &keyid, &error);

   TEST_ENCRYPT_DECRYPT (&keyid, client_encryption, res, error);
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 3: Custom endpoint, with the same as the default but port included.
    * Expect to succeed */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("region",
                         "us-east-1",
                         "key",
                         "arn:aws:kms:us-east-1:579766882180:key/"
                         "89fcc2c4-08b0-4bd9-9f25-e30687b580d0",
                         "endpoint",
                         "kms.us-east-1.amazonaws.com:443");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", datakey_opts, &keyid, &error);

   TEST_ENCRYPT_DECRYPT (&keyid, client_encryption, res, error);
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 4: Custom endpoint, with the same as the default but wrong port
    * included.
    * Expect to fail with socket error */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("region",
                         "us-east-1",
                         "key",
                         "arn:aws:kms:us-east-1:579766882180:key/"
                         "89fcc2c4-08b0-4bd9-9f25-e30687b580d0",
                         "endpoint",
                         "kms.us-east-1.amazonaws.com:12345");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", datakey_opts, &keyid, &error);
   BSON_ASSERT (!res);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_STREAM,
                          MONGOC_ERROR_STREAM_CONNECT,
                          "Failed to connect");
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 5: Custom endpoint, but wrong region. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("region",
                         "us-east-1",
                         "key",
                         "arn:aws:kms:us-east-1:579766882180:key/"
                         "89fcc2c4-08b0-4bd9-9f25-e30687b580d0",
                         "endpoint",
                         "kms.us-east-2.amazonaws.com");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   memset (&error, 0, sizeof (bson_error_t));
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", datakey_opts, &keyid, &error);
   BSON_ASSERT (!res);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION, 1, "us-east-1");
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 6: Custom endpoint to example.com. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("region",
                         "us-east-1",
                         "key",
                         "arn:aws:kms:us-east-1:579766882180:key/"
                         "89fcc2c4-08b0-4bd9-9f25-e30687b580d0",
                         "endpoint",
                         "example.com");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   memset (&error, 0, sizeof (bson_error_t));
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", datakey_opts, &keyid, &error);
   BSON_ASSERT (!res);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION, 1, "parse error");
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 7: Azure successful case */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("keyVaultEndpoint",
                         "key-vault-csfle.vault.azure.net",
                         "keyName",
                         "key-name-csfle");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "azure", datakey_opts, &keyid, &error);

   TEST_ENCRYPT_DECRYPT (&keyid, client_encryption, res, error);
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Azure invalid case. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   res = mongoc_client_encryption_create_datakey (
      client_encryption_invalid, "azure", datakey_opts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION, 1, "parse error");
   BSON_ASSERT (!res);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 8: GCP successful case. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("projectId",
                         "devprod-drivers",
                         "location",
                         "global",
                         "keyRing",
                         "key-ring-csfle",
                         "keyName",
                         "key-name-csfle",
                         "endpoint",
                         "cloudkms.googleapis.com:443");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "gcp", datakey_opts, &keyid, &error);

   TEST_ENCRYPT_DECRYPT (&keyid, client_encryption, res, error);
   bson_value_destroy (&keyid);
   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* GCP invalid case. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   res = mongoc_client_encryption_create_datakey (
      client_encryption_invalid, "gcp", datakey_opts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION, 1, "parse error");
   BSON_ASSERT (!res);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 9: GCP invalid key endpoint. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("projectId",
                         "devprod-drivers",
                         "location",
                         "global",
                         "keyRing",
                         "key-ring-csfle",
                         "keyName",
                         "key-name-csfle",
                         "endpoint",
                         "example.com:443");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "gcp", datakey_opts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION, 1, "Invalid KMS response");
   BSON_ASSERT (!res);

   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 10: KMIP no endpoint. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("keyId", "1");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "kmip", datakey_opts, &keyid, &error);
   ASSERT_OR_PRINT (res, error);

   TEST_ENCRYPT_DECRYPT (&keyid, client_encryption, res, error);
   bson_value_destroy (&keyid);

   /* Attempt to use client_encryption_invalid with the same masterKey. Expect
    * an error. */
   res = mongoc_client_encryption_create_datakey (
      client_encryption_invalid, "kmip", datakey_opts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_STREAM,
                          MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                          "Failed to resolve");
   bson_value_destroy (&keyid);

   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 11: KMIP overriding with valid endpoint. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("keyId", "1", "endpoint", "localhost:5698");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "kmip", datakey_opts, &keyid, &error);
   ASSERT_OR_PRINT (res, error);

   TEST_ENCRYPT_DECRYPT (&keyid, client_encryption, res, error);
   bson_value_destroy (&keyid);

   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   /* Case 12: KMIP overriding with invalid endpoint. */
   _endpoint_setup (
      keyvault_client, &client_encryption, &client_encryption_invalid);
   masterkey = BCON_NEW ("keyId", "1", "endpoint", "doesnotexist.local:5698");
   mongoc_client_encryption_datakey_opts_set_masterkey (datakey_opts,
                                                        masterkey);
   res = mongoc_client_encryption_create_datakey (
      client_encryption, "kmip", datakey_opts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_STREAM,
                          MONGOC_ERROR_STREAM_NAME_RESOLUTION,
                          "Failed to resolve");
   BSON_ASSERT (!res);
   bson_value_destroy (&keyid);

   bson_destroy (masterkey);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_encryption_destroy (client_encryption_invalid);

   mongoc_client_encryption_datakey_opts_destroy (datakey_opts);
   mongoc_client_destroy (keyvault_client);
}

typedef struct {
   const char *kms;
   const char *type;
   const char *algo;
   const char *method;
   const char *identifier;
   bool allowed;
   bson_value_t value; /* a copy */
} corpus_field_t;

static corpus_field_t *
_corpus_field_new (bson_iter_t *top_iter)
{
   bson_iter_t iter;
   corpus_field_t *field;

   field = bson_malloc0 (sizeof (corpus_field_t));
   memset (field, 0, sizeof (*field));
   BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (top_iter));
   bson_iter_recurse (top_iter, &iter);
   while (bson_iter_next (&iter)) {
      if (0 == strcmp ("kms", bson_iter_key (&iter))) {
         field->kms = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp ("type", bson_iter_key (&iter))) {
         field->type = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp ("algo", bson_iter_key (&iter))) {
         field->algo = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp ("method", bson_iter_key (&iter))) {
         field->method = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp ("identifier", bson_iter_key (&iter))) {
         field->identifier = bson_iter_utf8 (&iter, NULL);
      } else if (0 == strcmp ("allowed", bson_iter_key (&iter))) {
         field->allowed = bson_iter_bool (&iter);
      } else if (0 == strcmp ("value", bson_iter_key (&iter))) {
         bson_value_copy (bson_iter_value (&iter), &field->value);
      } else {
         fprintf (stderr, "unexpected field: %s\n", bson_iter_key (&iter));
         BSON_ASSERT (false);
      }
   }
   return field;
}

static void
_corpus_field_destroy (corpus_field_t *field)
{
   if (!field) {
      return;
   }
   bson_value_destroy (&field->value);
   bson_free (field);
}

#define LOCAL_UUID \
   "\x2c\xe0\x80\x2c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#define AWS_UUID \
   "\x01\x64\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#define AZURE_UUID \
   "\x01\x95\x11\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#define GCP_UUID \
   "\x18\x23\xc0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#define KMIP_UUID \
   "\x28\xc2\x0f\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

static void
_corpus_copy_field (mongoc_client_encryption_t *client_encryption,
                    bson_iter_t *iter,
                    bson_t *corpus_copied)
{
   corpus_field_t *field;
   const char *key = bson_iter_key (iter);
   mongoc_client_encryption_encrypt_opts_t *encrypt_opts;
   bson_value_t ciphertext;
   bool res;
   bson_error_t error;

   if (0 == strcmp ("_id", key) || 0 == strcmp ("altname_aws", key) ||
       0 == strcmp ("altname_local", key) ||
       0 == strcmp ("altname_azure", key) || 0 == strcmp ("altname_gcp", key) ||
       0 == strcmp ("altname_kmip", key)) {
      bson_append_value (corpus_copied, key, -1, bson_iter_value (iter));
      return;
   }
   field = _corpus_field_new (iter);

   if (0 == strcmp ("auto", field->method)) {
      bson_append_value (corpus_copied, key, -1, bson_iter_value (iter));
      _corpus_field_destroy (field);
      return;
   }

   /* Otherwise, use explicit encryption. */
   encrypt_opts = mongoc_client_encryption_encrypt_opts_new ();
   if (0 == strcmp ("rand", field->algo)) {
      mongoc_client_encryption_encrypt_opts_set_algorithm (
         encrypt_opts, MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_RANDOM);
   } else if (0 == strcmp ("det", field->algo)) {
      mongoc_client_encryption_encrypt_opts_set_algorithm (
         encrypt_opts, MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_DETERMINISTIC);
   }

   if (0 == strcmp ("id", field->identifier)) {
      bson_value_t uuid;
      uuid.value_type = BSON_TYPE_BINARY;
      uuid.value.v_binary.subtype = BSON_SUBTYPE_UUID;
      uuid.value.v_binary.data_len = 16;
      if (0 == strcmp ("local", field->kms)) {
         uuid.value.v_binary.data = (uint8_t *) LOCAL_UUID;
      } else if (0 == strcmp ("aws", field->kms)) {
         uuid.value.v_binary.data = (uint8_t *) AWS_UUID;
      } else if (0 == strcmp ("azure", field->kms)) {
         uuid.value.v_binary.data = (uint8_t *) AZURE_UUID;
      } else if (0 == strcmp ("gcp", field->kms)) {
         uuid.value.v_binary.data = (uint8_t *) GCP_UUID;
      } else if (0 == strcmp ("kmip", field->kms)) {
         uuid.value.v_binary.data = (uint8_t *) KMIP_UUID;
      }
      mongoc_client_encryption_encrypt_opts_set_keyid (encrypt_opts, &uuid);
   } else if (0 == strcmp ("altname", field->identifier)) {
      mongoc_client_encryption_encrypt_opts_set_keyaltname (encrypt_opts,
                                                            field->kms);
   }

   res = mongoc_client_encryption_encrypt (
      client_encryption, &field->value, encrypt_opts, &ciphertext, &error);

   if (field->allowed) {
      bson_t new_field;
      ASSERT_OR_PRINT (res, error);
      bson_append_document_begin (corpus_copied, key, -1, &new_field);
      BSON_APPEND_UTF8 (&new_field, "kms", field->kms);
      BSON_APPEND_UTF8 (&new_field, "type", field->type);
      BSON_APPEND_UTF8 (&new_field, "algo", field->algo);
      BSON_APPEND_UTF8 (&new_field, "method", field->method);
      BSON_APPEND_UTF8 (&new_field, "identifier", field->identifier);
      BSON_APPEND_BOOL (&new_field, "allowed", field->allowed);
      BSON_APPEND_VALUE (&new_field, "value", &ciphertext);
      bson_append_document_end (corpus_copied, &new_field);
   } else {
      BSON_ASSERT (!res);
      bson_append_value (corpus_copied, key, -1, bson_iter_value (iter));
   }

   bson_value_destroy (&ciphertext);
   mongoc_client_encryption_encrypt_opts_destroy (encrypt_opts);
   _corpus_field_destroy (field);
}

static void
_corpus_check_encrypted (mongoc_client_encryption_t *client_encryption,
                         bson_iter_t *expected_iter,
                         bson_iter_t *actual_iter)
{
   corpus_field_t *expected;
   corpus_field_t *actual;
   const char *key;
   bson_error_t error;
   match_ctx_t match_ctx;

   memset (&match_ctx, 0, sizeof (match_ctx));
   key = bson_iter_key (expected_iter);
   if (0 == strcmp ("_id", key) || 0 == strcmp ("altname_aws", key) ||
       0 == strcmp ("altname_local", key) ||
       0 == strcmp ("altname_azure", key) || 0 == strcmp ("altname_gcp", key) ||
       0 == strcmp ("altname_kmip", key)) {
      return;
   }

   expected = _corpus_field_new (expected_iter);
   actual = _corpus_field_new (actual_iter);

   /* If the algo is det, that the value equals the value of the corresponding
    * field
    * in corpus_encrypted_actual.
    */
   if (0 == strcmp (expected->algo, "det")) {
      BSON_ASSERT (
         match_bson_value (&expected->value, &actual->value, &match_ctx));
   }

   /* If the algo is rand and allowed is true, that the value does not equal the
    * value of the corresponding field in corpus_encrypted_actual. */
   if (0 == strcmp (expected->algo, "rand") && expected->allowed) {
      BSON_ASSERT (
         !match_bson_value (&expected->value, &actual->value, &match_ctx));
   }

   /* If allowed is true, decrypt the value with client_encryption. Decrypt the
    * value of the corresponding field of corpus_encrypted and validate that
    * they are both equal */
   if (expected->allowed) {
      bson_value_t expected_decrypted;
      bson_value_t actual_decrypted;
      bool res;

      res = mongoc_client_encryption_decrypt (
         client_encryption, &expected->value, &expected_decrypted, &error);
      ASSERT_OR_PRINT (res, error);

      res = mongoc_client_encryption_decrypt (
         client_encryption, &actual->value, &actual_decrypted, &error);
      ASSERT_OR_PRINT (res, error);

      BSON_ASSERT (
         match_bson_value (&expected_decrypted, &actual_decrypted, &match_ctx));
      bson_value_destroy (&expected_decrypted);
      bson_value_destroy (&actual_decrypted);
   }

   /* If allowed is false, validate the value exactly equals the value of the
    * corresponding field of corpus (neither was encrypted). */
   if (!expected->allowed) {
      BSON_ASSERT (
         match_bson_value (&expected->value, &actual->value, &match_ctx));
   }

   _corpus_field_destroy (expected);
   _corpus_field_destroy (actual);
}

static void
_insert_from_file (mongoc_collection_t *coll, char *path)
{
   bson_t *datakey;
   bool res;
   bson_error_t error;

   datakey = get_bson_from_json_file (path);
   res = mongoc_collection_insert_one (coll, datakey, NULL, NULL, &error);
   ASSERT_OR_PRINT (res, error);
   bson_destroy (datakey);
}

static void
_test_corpus (bool local_schema)
{
   mongoc_client_t *client;
   mongoc_collection_t *coll;
   bson_t *schema;
   bson_t *create_cmd;
   bool res;
   bson_error_t error;
   mongoc_write_concern_t *wc;
   mongoc_client_t *client_encrypted;
   mongoc_auto_encryption_opts_t *auto_encryption_opts;
   bson_t *kms_providers;
   mongoc_client_encryption_t *client_encryption;
   mongoc_client_encryption_opts_t *client_encryption_opts;
   bson_t *corpus;
   bson_t corpus_copied;
   const bson_t *corpus_decrypted;
   bson_t *corpus_encrypted_expected;
   const bson_t *corpus_encrypted_actual;
   bson_iter_t iter;
   mongoc_cursor_t *cursor;
   bson_t *schema_map;
   bson_t *tls_opts;

   /* Create a MongoClient without encryption enabled */
   client = test_framework_new_default_client ();
   coll = mongoc_client_get_collection (client, "db", "coll");
   (void) mongoc_collection_drop (coll, NULL);
   schema = get_bson_from_json_file ("./src/libmongoc/tests/"
                                     "client_side_encryption_prose/corpus/"
                                     "corpus-schema.json");
   schema_map = BCON_NEW ("db.coll", BCON_DOCUMENT (schema));
   create_cmd = BCON_NEW ("create",
                          "coll",
                          "validator",
                          "{",
                          "$jsonSchema",
                          BCON_DOCUMENT (schema),
                          "}");

   if (!local_schema) {
      /* Drop and create the collection db.coll configured with the included
       * JSON schema corpus-schema.json */
      res = mongoc_client_command_simple (
         client, "db", create_cmd, NULL, NULL, &error);
      ASSERT_OR_PRINT (res, error);
   }

   /* Drop the collection keyvault.datakeys. Insert the key documents for each
    * KMS provider. */
   mongoc_collection_destroy (coll);
   coll = mongoc_client_get_collection (client, "keyvault", "datakeys");
   (void) mongoc_collection_drop (coll, NULL);
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_wmajority (wc, 1000);
   mongoc_collection_set_write_concern (coll, wc);
   _insert_from_file (coll,
                      "./src/libmongoc/tests/client_side_encryption_prose/"
                      "corpus/corpus-key-aws.json");
   _insert_from_file (coll,
                      "./src/libmongoc/tests/client_side_encryption_prose/"
                      "corpus/corpus-key-azure.json");
   _insert_from_file (coll,
                      "./src/libmongoc/tests/client_side_encryption_prose/"
                      "corpus/corpus-key-gcp.json");
   _insert_from_file (coll,
                      "./src/libmongoc/tests/client_side_encryption_prose/"
                      "corpus/corpus-key-local.json");
   _insert_from_file (coll,
                      "./src/libmongoc/tests/client_side_encryption_prose/"
                      "corpus/corpus-key-kmip.json");

   /* Create a MongoClient configured with auto encryption */
   client_encrypted = test_framework_new_default_client ();
   auto_encryption_opts = mongoc_auto_encryption_opts_new ();
   mongoc_auto_encryption_opts_set_schema_map (auto_encryption_opts,
                                               schema_map);
   _check_bypass (auto_encryption_opts);
   kms_providers = _make_kms_providers (true /* aws */, true /* local */);
   mongoc_auto_encryption_opts_set_kms_providers (auto_encryption_opts,
                                                  kms_providers);
   tls_opts = _make_tls_opts ();
   mongoc_auto_encryption_opts_set_tls_opts (auto_encryption_opts, tls_opts);
   mongoc_auto_encryption_opts_set_keyvault_namespace (
      auto_encryption_opts, "keyvault", "datakeys");
   res = mongoc_client_enable_auto_encryption (
      client_encrypted, auto_encryption_opts, &error);
   ASSERT_OR_PRINT (res, error);

   /* Create a ClientEncryption object */
   client_encryption_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_kms_providers (client_encryption_opts,
                                                    kms_providers);
   mongoc_client_encryption_opts_set_tls_opts (client_encryption_opts,
                                               tls_opts);
   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                      client);
   client_encryption =
      mongoc_client_encryption_new (client_encryption_opts, &error);
   ASSERT_OR_PRINT (client_encryption, error);

   corpus = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/corpus/corpus.json");

   /* Try each field individually */
   bson_iter_init (&iter, corpus);
   bson_init (&corpus_copied);
   while (bson_iter_next (&iter)) {
      _corpus_copy_field (client_encryption, &iter, &corpus_copied);
   }

   /* Insert corpus_copied with auto encryption  */
   mongoc_collection_destroy (coll);
   coll = mongoc_client_get_collection (client_encrypted, "db", "coll");
   res =
      mongoc_collection_insert_one (coll, &corpus_copied, NULL, NULL, &error);
   ASSERT_OR_PRINT (res, error);

   /* Get the automatically decrypted corpus */
   cursor =
      mongoc_collection_find_with_opts (coll, tmp_bson ("{}"), NULL, NULL);
   BSON_ASSERT (mongoc_cursor_next (cursor, &corpus_decrypted));

   /* It should exactly match corpus. match_bson does a subset match, so match
    * in  both directions */
   BSON_ASSERT (match_bson (corpus, corpus_decrypted, false));
   BSON_ASSERT (match_bson (corpus_decrypted, corpus, false));
   mongoc_cursor_destroy (cursor);

   /* Load corpus-encrypted.json */
   corpus_encrypted_expected =
      get_bson_from_json_file ("./src/libmongoc/tests/"
                               "client_side_encryption_prose/"
                               "corpus/corpus-encrypted.json");
   /* Get the actual encrypted document from unencrypted client */
   mongoc_collection_destroy (coll);
   coll = mongoc_client_get_collection (client, "db", "coll");
   cursor =
      mongoc_collection_find_with_opts (coll, tmp_bson ("{}"), NULL, NULL);
   BSON_ASSERT (mongoc_cursor_next (cursor, &corpus_encrypted_actual));

   /* Iterate over corpus_encrypted_expected, and check corpus_encrypted_actual
    */
   bson_iter_init (&iter, corpus_encrypted_expected);
   while (bson_iter_next (&iter)) {
      bson_iter_t actual_iter;

      BSON_ASSERT (bson_iter_init_find (
         &actual_iter, corpus_encrypted_actual, bson_iter_key (&iter)));
      _corpus_check_encrypted (client_encryption, &iter, &actual_iter);
   }

   mongoc_cursor_destroy (cursor);
   bson_destroy (corpus_encrypted_expected);
   bson_destroy (corpus);
   bson_destroy (&corpus_copied);
   mongoc_auto_encryption_opts_destroy (auto_encryption_opts);
   mongoc_client_destroy (client_encrypted);
   mongoc_client_encryption_opts_destroy (client_encryption_opts);
   mongoc_client_encryption_destroy (client_encryption);
   bson_destroy (tls_opts);
   bson_destroy (kms_providers);
   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (coll);
   bson_destroy (schema);
   bson_destroy (schema_map);
   bson_destroy (create_cmd);
   mongoc_client_destroy (client);
}

static void
test_corpus (void *unused)
{
   _test_corpus (false /* local schema */);
   _test_corpus (true /* local schema */);
}

/* Begin C driver specific, non-spec tests: */
static void
_reset (mongoc_client_pool_t **pool,
        mongoc_client_t **singled_threaded_client,
        mongoc_client_t **multi_threaded_client,
        mongoc_auto_encryption_opts_t **opts,
        bool recreate)
{
   bson_t *kms_providers;
   mongoc_uri_t *uri;
   bson_t *extra;
   bson_t *schema;
   bson_t *schema_map;

   mongoc_auto_encryption_opts_destroy (*opts);
   *opts = mongoc_auto_encryption_opts_new ();
   extra = BCON_NEW ("mongocryptdBypassSpawn", BCON_BOOL (true));
   mongoc_auto_encryption_opts_set_extra (*opts, extra);
   mongoc_auto_encryption_opts_set_keyvault_namespace (*opts, "db", "keyvault");
   kms_providers = _make_kms_providers (false /* aws */, true /* local */);
   mongoc_auto_encryption_opts_set_kms_providers (*opts, kms_providers);
   schema = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/schema.json");
   BSON_ASSERT (schema);
   schema_map = BCON_NEW ("db.coll", BCON_DOCUMENT (schema));
   mongoc_auto_encryption_opts_set_schema_map (*opts, schema_map);

   if (*multi_threaded_client) {
      mongoc_client_pool_push (*pool, *multi_threaded_client);
   }

   mongoc_client_destroy (*singled_threaded_client);
   /* Workaround to hide unnecessary logs per CDRIVER-3322 */
   capture_logs (true);
   mongoc_client_pool_destroy (*pool);
   capture_logs (false);

   if (recreate) {
      mongoc_collection_t *coll;
      bson_t *datakey;
      bson_error_t error;

      uri = test_framework_get_uri ();
      *pool = test_framework_client_pool_new_from_uri (uri, NULL);
      test_framework_set_pool_ssl_opts (*pool);
      *singled_threaded_client = test_framework_client_new_from_uri (uri, NULL);
      test_framework_set_ssl_opts (*singled_threaded_client);
      *multi_threaded_client = mongoc_client_pool_pop (*pool);
      mongoc_uri_destroy (uri);

      /* create key */
      coll = mongoc_client_get_collection (
         *singled_threaded_client, "db", "keyvault");
      (void) mongoc_collection_drop (coll, NULL);
      datakey = get_bson_from_json_file (
         "./src/libmongoc/tests/client_side_encryption_prose/limits-key.json");
      BSON_ASSERT (datakey);
      ASSERT_OR_PRINT (
         mongoc_collection_insert_one (
            coll, datakey, NULL /* opts */, NULL /* reply */, &error),
         error);

      bson_destroy (datakey);
      mongoc_collection_destroy (coll);
   }
   bson_destroy (schema);
   bson_destroy (schema_map);
   bson_destroy (extra);
   bson_destroy (kms_providers);
}

static void
_perform_op (mongoc_client_t *client_encrypted)
{
   bool ret;
   bson_error_t error;
   mongoc_collection_t *coll;

   coll = mongoc_client_get_collection (client_encrypted, "db", "coll");
   ret = mongoc_collection_insert_one (coll,
                                       tmp_bson ("{'encrypted_string': 'abc'}"),
                                       NULL /* opts */,
                                       NULL /* reply */,
                                       &error);
   ASSERT_OR_PRINT (ret, error);
   mongoc_collection_destroy (coll);
}

static void
_perform_op_pooled (mongoc_client_pool_t *client_pool_encrypted)
{
   mongoc_client_t *client_encrypted;

   client_encrypted = mongoc_client_pool_pop (client_pool_encrypted);
   _perform_op (client_encrypted);
   mongoc_client_pool_push (client_pool_encrypted, client_encrypted);
}

static void
test_invalid_single_and_pool_mismatches (void *unused)
{
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *single_threaded_client = NULL;
   mongoc_client_t *multi_threaded_client = NULL;
   mongoc_auto_encryption_opts_t *opts = NULL;
   bson_error_t error;
   bool ret;

   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);

   /* single threaded client, single threaded setter => ok */
   ret = mongoc_client_enable_auto_encryption (
      single_threaded_client, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   _perform_op (single_threaded_client);

   /* multi threaded client, single threaded setter => bad */
   ret = mongoc_client_enable_auto_encryption (
      multi_threaded_client, opts, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "Cannot enable auto encryption on a pooled client");

   /* pool - pool setter */
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   _perform_op_pooled (pool);

   /* single threaded client, single threaded key vault client => ok */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client (opts,
                                                    single_threaded_client);
   ret = mongoc_client_enable_auto_encryption (
      single_threaded_client, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   _perform_op (single_threaded_client);

   /* single threaded client, multi threaded key vault client => bad */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client (opts,
                                                    multi_threaded_client);
   ret = mongoc_client_enable_auto_encryption (
      single_threaded_client, opts, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "The key vault client must be single threaded, not "
                          "be from a client pool");

   /* single threaded client, pool key vault client => bad */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client_pool (opts, pool);
   ret = mongoc_client_enable_auto_encryption (
      single_threaded_client, opts, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "The key vault client pool only applies to a client "
                          "pool, not a single threaded client");

   /* pool, singled threaded key vault client => bad */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client (opts,
                                                    single_threaded_client);
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "The key vault client only applies to a single "
                          "threaded client not a client pool. Set a "
                          "key vault client pool");

   /* pool, multi threaded key vault client => bad */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client (opts,
                                                    multi_threaded_client);
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "The key vault client only applies to a single "
                          "threaded client not a client pool. Set a "
                          "key vault client pool");

   /* pool, pool key vault client => ok */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client_pool (opts, pool);
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   _perform_op_pooled (pool);

   /* double enabling */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   ret = mongoc_client_enable_auto_encryption (
      single_threaded_client, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   ret = mongoc_client_enable_auto_encryption (
      single_threaded_client, opts, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                          "Automatic encryption already set");
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_STATE,
                          "Automatic encryption already set");

   /* single threaded, using self as key vault client => redundant, but ok */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client (opts,
                                                    single_threaded_client);
   ret = mongoc_client_enable_auto_encryption (
      single_threaded_client, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   _perform_op (single_threaded_client);

   /* pool, using self as key vault client pool => redundant, but ok */
   _reset (&pool, &single_threaded_client, &multi_threaded_client, &opts, true);
   mongoc_auto_encryption_opts_set_keyvault_client_pool (opts, pool);
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   ASSERT_OR_PRINT (ret, error);
   _perform_op_pooled (pool);

   _reset (
      &pool, &single_threaded_client, &multi_threaded_client, &opts, false);
   mongoc_auto_encryption_opts_destroy (opts);
}

static BSON_THREAD_FUN (_worker_thread, client_ptr)
{
   mongoc_client_t *client_encrypted;
   mongoc_collection_t *coll;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_t filter = BSON_INITIALIZER;
   bson_t *to_insert = BCON_NEW ("encrypted_string", "abc");
   int i;
   bool ret;
   bson_error_t error;

   client_encrypted = client_ptr;
   coll = mongoc_client_get_collection (client_encrypted, "db", "coll");

   for (i = 0; i < 100; i++) {
      ret = mongoc_collection_insert_one (
         coll, to_insert, NULL /* opts */, NULL /* reply */, &error);
      ASSERT_OR_PRINT (ret, error);
      cursor = mongoc_collection_find_with_opts (
         coll, &filter, NULL /* opts */, NULL /* read_prefs */);
      mongoc_cursor_next (cursor, &doc);
      mongoc_cursor_destroy (cursor);
   }
   mongoc_collection_destroy (coll);
   bson_destroy (&filter);
   bson_destroy (to_insert);
   BSON_THREAD_RETURN;
}

static void
_test_multi_threaded (bool external_key_vault)
{
   /* Spawn two threads and do repeated encryption/decryption operations. */
   bson_thread_t threads[2];
   mongoc_uri_t *uri;
   mongoc_client_pool_t *pool;
   mongoc_client_t *client;
   mongoc_client_t *client1;
   mongoc_client_t *client2;
   mongoc_auto_encryption_opts_t *opts;
   bson_t *datakey;
   mongoc_collection_t *coll;
   bson_t *schema;
   bson_t *schema_map;
   bool ret;
   bson_error_t error;
   bson_t *kms_providers;
   int r;
   int i;

   uri = test_framework_get_uri ();
   pool = test_framework_client_pool_new_from_uri (uri, NULL);
   test_framework_set_pool_ssl_opts (pool);
   client = test_framework_client_new_from_uri (uri, NULL);
   test_framework_set_ssl_opts (client);
   opts = mongoc_auto_encryption_opts_new ();

   /* Do setup: create a data key and configure pool for auto encryption. */
   coll = mongoc_client_get_collection (client, "db", "keyvault");
   (void) mongoc_collection_drop (coll, NULL);
   datakey = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/limits-key.json");
   BSON_ASSERT (datakey);
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (
         coll, datakey, NULL /* opts */, NULL /* reply */, &error),
      error);

   /* create pool with auto encryption */
   _check_bypass (opts);

   mongoc_auto_encryption_opts_set_keyvault_namespace (opts, "db", "keyvault");
   kms_providers = _make_kms_providers (false /* aws */, true /* local */);
   mongoc_auto_encryption_opts_set_kms_providers (opts, kms_providers);

   if (external_key_vault) {
      mongoc_auto_encryption_opts_set_keyvault_client_pool (opts, pool);
   }

   schema = get_bson_from_json_file (
      "./src/libmongoc/tests/client_side_encryption_prose/schema.json");
   BSON_ASSERT (schema);
   schema_map = BCON_NEW ("db.coll", BCON_DOCUMENT (schema));
   mongoc_auto_encryption_opts_set_schema_map (opts, schema_map);
   ret = mongoc_client_pool_enable_auto_encryption (pool, opts, &error);
   ASSERT_OR_PRINT (ret, error);

   client1 = mongoc_client_pool_pop (pool);
   client2 = mongoc_client_pool_pop (pool);

   r = COMMON_PREFIX (thread_create) (threads, _worker_thread, client1);
   BSON_ASSERT (r == 0);

   r = COMMON_PREFIX (thread_create) (threads + 1, _worker_thread, client2);
   BSON_ASSERT (r == 0);

   for (i = 0; i < 2; i++) {
      r = COMMON_PREFIX (thread_join) (threads[i]);
      BSON_ASSERT (r == 0);
   }

   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client);
   mongoc_client_pool_push (pool, client1);
   mongoc_client_pool_push (pool, client2);
   mongoc_client_pool_destroy (pool);
   bson_destroy (schema);
   bson_destroy (schema_map);
   bson_destroy (datakey);
   mongoc_auto_encryption_opts_destroy (opts);
   mongoc_uri_destroy (uri);
   bson_destroy (kms_providers);
}

static void
test_multi_threaded (void *ctx_unused)
{
   _test_multi_threaded (true);
   _test_multi_threaded (false);
}

static void
test_malformed_explicit (void *unused)
{
   mongoc_client_t *client;
   bson_t *kms_providers;
   mongoc_client_encryption_t *client_encryption;
   mongoc_client_encryption_opts_t *client_encryption_opts;
   bson_value_t value;
   bson_value_t ciphertext;
   bool ret;
   bson_error_t error;

   /* Create a MongoClient without encryption enabled */
   client = test_framework_new_default_client ();
   kms_providers = _make_kms_providers (false /* aws */, true /* local */);

   /* Create a ClientEncryption object */
   client_encryption_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_kms_providers (client_encryption_opts,
                                                    kms_providers);
   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                      client);
   client_encryption =
      mongoc_client_encryption_new (client_encryption_opts, &error);
   ASSERT_OR_PRINT (client_encryption, error);

   /* Test attempting to decrypt a malformed value */
   ciphertext.value_type = BSON_TYPE_DOUBLE;
   ciphertext.value.v_double = 1.23;
   ret = mongoc_client_encryption_decrypt (
      client_encryption, &ciphertext, &value, &error);
   BSON_ASSERT (!ret);
   bson_value_destroy (&value);

   mongoc_client_encryption_opts_destroy (client_encryption_opts);
   mongoc_client_encryption_destroy (client_encryption);
   bson_destroy (kms_providers);
   mongoc_client_destroy (client);
}

static void
_check_mongocryptd_not_spawned (void)
{
   mongoc_client_t *client;
   bson_t *cmd;
   bson_error_t error;
   bool ret;

   client = test_framework_client_new (
      "mongodb://localhost:27021/db?serverSelectionTimeoutMS=1000", NULL);
   cmd = BCON_NEW (HANDSHAKE_CMD_LEGACY_HELLO, BCON_INT32 (1));
   ret = mongoc_client_command_simple (
      client, "keyvault", cmd, NULL /* read prefs */, NULL /* reply */, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_SERVER_SELECTION,
                          MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                          "No suitable servers");
   mongoc_client_destroy (client);
   bson_destroy (cmd);
}

static void
test_bypass_spawning_via_mongocryptdBypassSpawn (void *unused)
{
   mongoc_client_t *client_encrypted;
   mongoc_auto_encryption_opts_t *auto_encryption_opts;
   bson_t *kms_providers;
   bson_t *doc_to_insert;
   bson_t *extra;
   bson_t *schema_map;
   bson_t *schema;
   bool ret;
   bson_error_t error;
   mongoc_collection_t *coll;

   auto_encryption_opts = mongoc_auto_encryption_opts_new ();
   kms_providers = _make_kms_providers (false /* aws */, true /* local */);
   mongoc_auto_encryption_opts_set_kms_providers (auto_encryption_opts,
                                                  kms_providers);
   mongoc_auto_encryption_opts_set_keyvault_namespace (
      auto_encryption_opts, "keyvault", "datakeys");
   schema = get_bson_from_json_file ("./src/libmongoc/tests/"
                                     "client_side_encryption_prose/external/"
                                     "external-schema.json");
   schema_map = BCON_NEW ("db.coll", BCON_DOCUMENT (schema));

   /* Create a MongoClient with encryption enabled */
   client_encrypted = test_framework_new_default_client ();
   extra =
      BCON_NEW ("mongocryptdBypassSpawn",
                BCON_BOOL (true),
                "mongocryptdSpawnArgs",
                "[",
                "--pidfilepath=bypass-spawning-mongocryptd.pid",
                "--port=27021",
                "]",
                "mongocryptdURI",
                "mongodb://localhost:27021/?serverSelectionTimeoutMS=1000");
   mongoc_auto_encryption_opts_set_extra (auto_encryption_opts, extra);
   mongoc_auto_encryption_opts_set_schema_map (auto_encryption_opts,
                                               schema_map);
   bson_destroy (extra);
   ret = mongoc_client_enable_auto_encryption (
      client_encrypted, auto_encryption_opts, &error);
   ASSERT_OR_PRINT (ret, error);

   /* Insert { 'encrypt': 'test' }. Should fail with a server selection error.
    */
   coll = mongoc_client_get_collection (client_encrypted, "db", "coll");
   doc_to_insert = BCON_NEW ("encrypt", "test");
   ret = mongoc_collection_insert_one (
      coll, doc_to_insert, NULL /* opts */, NULL /* reply */, &error);
   BSON_ASSERT (!ret);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_SERVER_SELECTION,
                          MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                          "mongocryptd error: No suitable servers found");

   _check_mongocryptd_not_spawned ();
   bson_destroy (schema_map);
   bson_destroy (schema);
   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client_encrypted);
   bson_destroy (doc_to_insert);
   mongoc_auto_encryption_opts_destroy (auto_encryption_opts);
   bson_destroy (kms_providers);
}

static void
test_bypass_spawning_via_bypassAutoEncryption (void *unused)
{
   mongoc_client_t *client_encrypted;
   mongoc_auto_encryption_opts_t *auto_encryption_opts;
   bson_t *kms_providers;
   bson_t *doc_to_insert;
   bson_t *extra;
   bool ret;
   bson_error_t error;
   mongoc_collection_t *coll;

   auto_encryption_opts = mongoc_auto_encryption_opts_new ();
   kms_providers = _make_kms_providers (false /* aws */, true /* local */);
   mongoc_auto_encryption_opts_set_kms_providers (auto_encryption_opts,
                                                  kms_providers);
   mongoc_auto_encryption_opts_set_keyvault_namespace (
      auto_encryption_opts, "keyvault", "datakeys");
   mongoc_auto_encryption_opts_set_bypass_auto_encryption (auto_encryption_opts,
                                                           true);

   /* Create a MongoClient with encryption enabled */
   client_encrypted = test_framework_new_default_client ();
   extra = BCON_NEW ("mongocryptdSpawnArgs",
                     "[",
                     "--pidfilepath=bypass-spawning-mongocryptd.pid",
                     "--port=27021",
                     "]");
   mongoc_auto_encryption_opts_set_extra (auto_encryption_opts, extra);
   bson_destroy (extra);
   ret = mongoc_client_enable_auto_encryption (
      client_encrypted, auto_encryption_opts, &error);
   ASSERT_OR_PRINT (ret, error);

   /* Insert { 'encrypt': 'test' }. Should succeed. */
   coll = mongoc_client_get_collection (client_encrypted, "db", "coll");
   doc_to_insert = BCON_NEW ("unencrypted", "test");
   ret = mongoc_collection_insert_one (
      coll, doc_to_insert, NULL /* opts */, NULL /* reply */, &error);
   ASSERT_OR_PRINT (ret, error);

   _check_mongocryptd_not_spawned ();

   mongoc_collection_destroy (coll);
   mongoc_client_destroy (client_encrypted);
   bson_destroy (doc_to_insert);
   mongoc_auto_encryption_opts_destroy (auto_encryption_opts);
   bson_destroy (kms_providers);
}


static mongoc_client_encryption_t *
_make_kms_certificate_client_encryption (mongoc_client_t *client,
                                         bson_error_t *error)
{
   mongoc_client_encryption_opts_t *client_encryption_opts =
      mongoc_client_encryption_opts_new ();

   {
      bson_t *kms_providers = _make_aws_kms_provider (NULL);
      mongoc_client_encryption_opts_set_kms_providers (client_encryption_opts,
                                                       kms_providers);
      bson_destroy (kms_providers);
   }

   {
      char *tls_ca_file =
         test_framework_getenv_required ("MONGOC_TEST_CSFLE_TLS_CA_FILE");
      char *tls_cert_key_file = test_framework_getenv_required (
         "MONGOC_TEST_CSFLE_TLS_CERTIFICATE_KEY_FILE");
      bson_t *tls_opts = tmp_bson ("{ 'aws': { '%s': '%s', '%s': '%s' } }",
                                   MONGOC_URI_TLSCAFILE,
                                   tls_ca_file,
                                   MONGOC_URI_TLSCERTIFICATEKEYFILE,
                                   tls_cert_key_file);

      mongoc_client_encryption_opts_set_tls_opts (client_encryption_opts,
                                                  tls_opts);

      bson_free (tls_cert_key_file);
      bson_free (tls_ca_file);
   }

   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                      client);

   mongoc_client_encryption_t *client_encryption =
      mongoc_client_encryption_new (client_encryption_opts, error);
   ASSERT_OR_PRINT (client_encryption, (*error));

   mongoc_client_encryption_opts_destroy (client_encryption_opts);
   return client_encryption;
}


static void
test_kms_tls_cert_valid (void *unused)
{
   const int32_t connecttimeoutms = MONGOC_DEFAULT_CONNECTTIMEOUTMS;
   const int is_client = 1;

   bson_error_t error;
   mongoc_host_list_t host;
   mongoc_stream_t *base_stream;
   mongoc_ssl_opt_t ssl_opts;
   mongoc_stream_t *tls_stream;

   char *tls_ca_file =
      test_framework_getenv_required ("MONGOC_TEST_CSFLE_TLS_CA_FILE");
   char *tls_cert_key_file = test_framework_getenv_required (
      "MONGOC_TEST_CSFLE_TLS_CERTIFICATE_KEY_FILE");

#if defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   /* Certificate verification fails with Secure Channel given
    * "127.0.0.1:7999" with error: "hostname doesn't match certificate". */
   ASSERT_OR_PRINT (
      _mongoc_host_list_from_string_with_err (&host, "localhost:7999", &error),
      error);
#else
   ASSERT_OR_PRINT (
      _mongoc_host_list_from_string_with_err (&host, "127.0.0.1:7999", &error),
      error);
#endif

   base_stream = mongoc_client_connect_tcp (connecttimeoutms, &host, &error);
   ASSERT_OR_PRINT (base_stream, error);

   ssl_opts = *test_framework_get_ssl_opts ();
   ssl_opts.ca_file = tls_ca_file;
   ssl_opts.pem_file = tls_cert_key_file;

   tls_stream = mongoc_stream_tls_new_with_hostname (
      base_stream, host.host, &ssl_opts, is_client);

   ASSERT_OR_PRINT (mongoc_stream_tls_handshake_block (
                       tls_stream, host.host, connecttimeoutms, &error),
                    error);

   mongoc_stream_destroy (tls_stream); /* Also destroys base_stream. */

   bson_free (tls_cert_key_file);
   bson_free (tls_ca_file);
}

static void
test_kms_tls_cert_expired (void *unused)
{
   bson_error_t error;

   mongoc_client_t *client = test_framework_new_default_client ();

   mongoc_client_encryption_t *client_encryption =
      _make_kms_certificate_client_encryption (client, &error);

   mongoc_client_encryption_datakey_opts_t *opts =
      mongoc_client_encryption_datakey_opts_new ();

   mongoc_client_encryption_datakey_opts_set_masterkey (
      opts,
      tmp_bson ("{ 'region': 'us-east-1', 'key': "
                "'arn:aws:kms:us-east-1:579766882180:key/"
                "89fcc2c4-08b0-4bd9-9f25-e30687b580d0', "
                "'endpoint': '127.0.0.1:8000' }"));

   bson_value_t keyid;
   bool ret = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", opts, &keyid, &error);

   BSON_ASSERT (!ret);

#if defined(MONGOC_ENABLE_SSL_OPENSSL)
   ASSERT_CONTAINS (error.message, "certificate has expired");
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
   ASSERT_CONTAINS (error.message, "CSSMERR_TP_CERT_EXPIRED");
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   ASSERT_CONTAINS (error.message, "certificate has expired");
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
   ASSERT_CONTAINS (error.message, "certificate has expired");
#endif

   mongoc_client_encryption_datakey_opts_destroy (opts);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_destroy (client);
}


static void
test_kms_tls_cert_wrong_host (void *unused)
{
   bson_error_t error;

   mongoc_client_t *client = test_framework_new_default_client ();

   mongoc_client_encryption_t *client_encryption =
      _make_kms_certificate_client_encryption (client, &error);

   mongoc_client_encryption_datakey_opts_t *opts =
      mongoc_client_encryption_datakey_opts_new ();

   mongoc_client_encryption_datakey_opts_set_masterkey (
      opts,
      tmp_bson ("{ 'region': 'us-east-1', 'key': "
                "'arn:aws:kms:us-east-1:579766882180:key/"
                "89fcc2c4-08b0-4bd9-9f25-e30687b580d0', "
                "'endpoint': '127.0.0.1:8001' }"));

   bson_value_t keyid;
   bool ret = mongoc_client_encryption_create_datakey (
      client_encryption, "aws", opts, &keyid, &error);

   BSON_ASSERT (!ret);

#if defined(MONGOC_ENABLE_SSL_OPENSSL)
   ASSERT_CONTAINS (error.message, "IP address mismatch");
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
   ASSERT_CONTAINS (error.message, "Host name mismatch");
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   ASSERT_CONTAINS (error.message, "hostname doesn't match certificate");
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
   ASSERT_CONTAINS (error.message, "not present in server certificate");
#endif

   mongoc_client_encryption_datakey_opts_destroy (opts);
   mongoc_client_encryption_destroy (client_encryption);
   mongoc_client_destroy (client);
}

typedef enum {
   NO_CLIENT_CERT,
   WITH_TLS,
   INVALID_HOSTNAME,
   EXPIRED
} tls_test_ce_t;

static mongoc_client_encryption_t *
_tls_test_make_client_encryption (mongoc_client_t *keyvault_client,
                                  tls_test_ce_t test_ce)
{
   bson_t *kms_providers;
   mongoc_client_encryption_opts_t *client_encryption_opts;
   bson_error_t error = {0};
   mongoc_client_encryption_t *client_encryption;
   bson_t *tls_opts = NULL;

   char *mongoc_test_aws_access_key_id =
      test_framework_getenv_required ("MONGOC_TEST_AWS_ACCESS_KEY_ID");
   char *mongoc_test_aws_secret_access_key =
      test_framework_getenv_required ("MONGOC_TEST_AWS_SECRET_ACCESS_KEY");
   char *mongoc_test_azure_tenant_id =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_TENANT_ID");
   char *mongoc_test_azure_client_id =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_CLIENT_ID");
   char *mongoc_test_azure_client_secret =
      test_framework_getenv_required ("MONGOC_TEST_AZURE_CLIENT_SECRET");
   char *mongoc_test_gcp_email =
      test_framework_getenv_required ("MONGOC_TEST_GCP_EMAIL");
   char *mongoc_test_gcp_privatekey =
      test_framework_getenv_required ("MONGOC_TEST_GCP_PRIVATEKEY");
   char *ca_file =
      test_framework_getenv_required ("MONGOC_TEST_CSFLE_TLS_CA_FILE");
   char *certificate_key_file = test_framework_getenv_required (
      "MONGOC_TEST_CSFLE_TLS_CERTIFICATE_KEY_FILE");

   if (test_ce == WITH_TLS) {
      kms_providers =
         tmp_bson ("{'aws': {'accessKeyId': '%s', 'secretAccessKey': '%s' }}",
                   mongoc_test_aws_access_key_id,
                   mongoc_test_aws_secret_access_key);

      tls_opts = tmp_bson (
         "{'aws': {'tlsCaFile': '%s', 'tlsCertificateKeyFile': '%s' }}",
         ca_file,
         certificate_key_file);
      bson_concat (kms_providers,
                   tmp_bson ("{'azure': {'tenantId': '%s', 'clientId': '%s', "
                             "'clientSecret': '%s', "
                             "'identityPlatformEndpoint': '127.0.0.1:8002' }}",
                             mongoc_test_azure_tenant_id,
                             mongoc_test_azure_client_id,
                             mongoc_test_azure_client_secret));
      bson_concat (
         tls_opts,
         tmp_bson (
            "{'azure': {'tlsCaFile': '%s', 'tlsCertificateKeyFile': '%s' }}",
            ca_file,
            certificate_key_file));

      bson_concat (kms_providers,
                   tmp_bson ("{'gcp': { 'email': '%s', 'privateKey': '%s', "
                             "'endpoint': '127.0.0.1:8002' }}",
                             mongoc_test_gcp_email,
                             mongoc_test_gcp_privatekey));
      bson_concat (
         tls_opts,
         tmp_bson (
            "{'gcp': {'tlsCaFile': '%s', 'tlsCertificateKeyFile': '%s' }}",
            ca_file,
            certificate_key_file));

      bson_concat (kms_providers,
                   tmp_bson ("{'kmip': { 'endpoint': '127.0.0.1:5698'}}"));
      bson_concat (
         tls_opts,
         tmp_bson (
            "{'kmip': {'tlsCaFile': '%s', 'tlsCertificateKeyFile': '%s' }}",
            ca_file,
            certificate_key_file));
   } else if (test_ce == NO_CLIENT_CERT) {
      kms_providers =
         tmp_bson ("{'aws': {'accessKeyId': '%s', 'secretAccessKey': '%s' }}",
                   mongoc_test_aws_access_key_id,
                   mongoc_test_aws_secret_access_key);
      bson_concat (kms_providers,
                   tmp_bson ("{'azure': {'tenantId': '%s', 'clientId': '%s', "
                             "'clientSecret': '%s', "
                             "'identityPlatformEndpoint': '127.0.0.1:8002'}}",
                             mongoc_test_azure_tenant_id,
                             mongoc_test_azure_client_id,
                             mongoc_test_azure_client_secret));
      bson_concat (kms_providers,
                   tmp_bson ("{'gcp': { 'email': '%s', 'privateKey': '%s', "
                             "'endpoint': '127.0.0.1:8002'}}",
                             mongoc_test_gcp_email,
                             mongoc_test_gcp_privatekey));
      bson_concat (kms_providers,
                   tmp_bson ("{'kmip': { 'endpoint': '127.0.0.1:5698' }}"));
   } else if (test_ce == EXPIRED) {
      kms_providers =
         tmp_bson ("{'aws': {'accessKeyId': '%s', 'secretAccessKey': '%s' }}",
                   mongoc_test_aws_access_key_id,
                   mongoc_test_aws_secret_access_key);
      tls_opts = tmp_bson ("{'aws': {'tlsCaFile': '%s'} }", ca_file);

      bson_concat (kms_providers,
                   tmp_bson ("{'azure': {'tenantId': '%s', 'clientId': '%s', "
                             "'clientSecret': '%s', "
                             "'identityPlatformEndpoint': '127.0.0.1:8000'}}",
                             mongoc_test_azure_tenant_id,
                             mongoc_test_azure_client_id,
                             mongoc_test_azure_client_secret));
      bson_concat (tls_opts,
                   tmp_bson ("{'azure': {'tlsCaFile': '%s'} }", ca_file));

      bson_concat (kms_providers,
                   tmp_bson ("{'gcp': { 'email': '%s', 'privateKey': '%s', "
                             "'endpoint': '127.0.0.1:8000'}}",
                             mongoc_test_gcp_email,
                             mongoc_test_gcp_privatekey));
      bson_concat (tls_opts,
                   tmp_bson ("{'gcp': {'tlsCaFile': '%s'} }", ca_file));

      bson_concat (kms_providers,
                   tmp_bson ("{'kmip': { 'endpoint': '127.0.0.1:8000' }}"));
      bson_concat (tls_opts,
                   tmp_bson ("{'kmip': {'tlsCaFile': '%s'} }", ca_file));
   } else if (test_ce == INVALID_HOSTNAME) {
      kms_providers =
         tmp_bson ("{'aws': {'accessKeyId': '%s', 'secretAccessKey': '%s' } }",
                   mongoc_test_aws_access_key_id,
                   mongoc_test_aws_secret_access_key);
      tls_opts = tmp_bson ("{'aws': {'tlsCaFile': '%s'} }", ca_file);

      bson_concat (kms_providers,
                   tmp_bson ("{'azure': {'tenantId': '%s', 'clientId': '%s', "
                             "'clientSecret': '%s', "
                             "'identityPlatformEndpoint': '127.0.0.1:8001' }}",
                             mongoc_test_azure_tenant_id,
                             mongoc_test_azure_client_id,
                             mongoc_test_azure_client_secret));
      bson_concat (tls_opts,
                   tmp_bson ("{'azure': {'tlsCaFile': '%s'} }", ca_file));

      bson_concat (kms_providers,
                   tmp_bson ("{'gcp': { 'email': '%s', 'privateKey': '%s', "
                             "'endpoint': '127.0.0.1:8001' }}",
                             mongoc_test_gcp_email,
                             mongoc_test_gcp_privatekey));
      bson_concat (tls_opts,
                   tmp_bson ("{'gcp': {'tlsCaFile': '%s'} }", ca_file));

      bson_concat (kms_providers,
                   tmp_bson ("{'kmip': { 'endpoint': '127.0.0.1:8001' }}"));
      bson_concat (tls_opts,
                   tmp_bson ("{'kmip': {'tlsCaFile': '%s'} }", ca_file));
   } else {
      BSON_UNREACHABLE ("Invalid value for test_ce");
   }

   client_encryption_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_kms_providers (client_encryption_opts,
                                                    kms_providers);
   mongoc_client_encryption_opts_set_keyvault_namespace (
      client_encryption_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (client_encryption_opts,
                                                      keyvault_client);
   mongoc_client_encryption_opts_set_tls_opts (client_encryption_opts,
                                               tls_opts);

   client_encryption =
      mongoc_client_encryption_new (client_encryption_opts, &error);
   ASSERT_OR_PRINT (client_encryption, error);

   mongoc_client_encryption_opts_destroy (client_encryption_opts);
   bson_free (mongoc_test_aws_access_key_id);
   bson_free (mongoc_test_aws_secret_access_key);
   bson_free (mongoc_test_azure_tenant_id);
   bson_free (mongoc_test_azure_client_id);
   bson_free (mongoc_test_azure_client_secret);
   bson_free (mongoc_test_gcp_email);
   bson_free (mongoc_test_gcp_privatekey);
   bson_free (ca_file);
   bson_free (certificate_key_file);

   return client_encryption;
}

#if defined(MONGOC_ENABLE_SSL_OPENSSL)
#define ASSERT_EXPIRED(error) \
   ASSERT_CONTAINS (error.message, "certificate has expired");
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
#define ASSERT_EXPIRED(error) \
   ASSERT_CONTAINS (error.message, "CSSMERR_TP_CERT_EXPIRED");
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
#define ASSERT_EXPIRED(error) \
   ASSERT_CONTAINS (error.message, "certificate has expired");
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
#define ASSERT_EXPIRED(error) \
   ASSERT_CONTAINS (error.message, "certificate has expired");
#else
#define ASSERT_EXPIRED(error)
#endif

#if defined(MONGOC_ENABLE_SSL_OPENSSL)
#define ASSERT_INVALID_HOSTNAME(error) \
   ASSERT_CONTAINS (error.message, "IP address mismatch");
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
#define ASSERT_INVALID_HOSTNAME(error) \
   ASSERT_CONTAINS (error.message, "Host name mismatch");
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
#define ASSERT_INVALID_HOSTNAME(error) \
   ASSERT_CONTAINS (error.message, "hostname doesn't match certificate");
#elif defined(MONGOC_ENABLE_SSL_LIBRESSL)
#define ASSERT_INVALID_HOSTNAME(error) \
   ASSERT_CONTAINS (error.message, "not present in server certificate");
#else
#define ASSERT_INVALID_HOSTNAME(error)
#endif

static void
test_kms_tls_options (void *unused)
{
   mongoc_client_t *keyvault_client;
   mongoc_client_encryption_t *client_encryption_no_client_cert = NULL;
   mongoc_client_encryption_t *client_encryption_with_tls = NULL;
   mongoc_client_encryption_t *client_encryption_expired = NULL;
   mongoc_client_encryption_t *client_encryption_invalid_hostname = NULL;
   bson_value_t keyid;
   mongoc_client_encryption_datakey_opts_t *dkopts;
   bson_error_t error;
   bool res;
   const int mongocrypt_errno =
      1; /* libmongocrypt returns all errors with code 1. */

   keyvault_client = test_framework_new_default_client ();
   client_encryption_no_client_cert =
      _tls_test_make_client_encryption (keyvault_client, NO_CLIENT_CERT);
   client_encryption_with_tls =
      _tls_test_make_client_encryption (keyvault_client, WITH_TLS);
   client_encryption_expired =
      _tls_test_make_client_encryption (keyvault_client, EXPIRED);
   client_encryption_invalid_hostname =
      _tls_test_make_client_encryption (keyvault_client, INVALID_HOSTNAME);

   /* Case 1: AWS - no client cert. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'region': 'us-east-1', 'key': "
                "'arn:aws:kms:us-east-1:579766882180:key/"
                "89fcc2c4-08b0-4bd9-9f25-e30687b580d0', 'endpoint': "
                "'127.0.0.1:8002' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_no_client_cert, "aws", dkopts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "");
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* AWS - with TLS. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'region': 'us-east-1', 'key': "
                "'arn:aws:kms:us-east-1:579766882180:key/"
                "89fcc2c4-08b0-4bd9-9f25-e30687b580d0', 'endpoint': "
                "'127.0.0.1:8002' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_with_tls, "aws", dkopts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
                          mongocrypt_errno,
                          "parse error");
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* AWS - expired. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'region': 'us-east-1', 'key': "
                "'arn:aws:kms:us-east-1:579766882180:key/"
                "89fcc2c4-08b0-4bd9-9f25-e30687b580d0', 'endpoint': "
                "'127.0.0.1:8000' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_expired, "aws", dkopts, &keyid, &error);
   ASSERT_EXPIRED (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* AWS - invalid hostname. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'region': 'us-east-1', 'key': "
                "'arn:aws:kms:us-east-1:579766882180:key/"
                "89fcc2c4-08b0-4bd9-9f25-e30687b580d0', 'endpoint': "
                "'127.0.0.1:8001' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_invalid_hostname, "aws", dkopts, &keyid, &error);
   ASSERT_INVALID_HOSTNAME (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* Case 2: Azure - no client cert. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson (
         "{ 'keyVaultEndpoint': 'doesnotexist.local', 'keyName': 'foo' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_no_client_cert, "azure", dkopts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "");
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* Azure - with TLS. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson (
         "{ 'keyVaultEndpoint': 'doesnotexist.local', 'keyName': 'foo' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_with_tls, "azure", dkopts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
                          mongocrypt_errno,
                          "HTTP status=404");
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* Azure - expired. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson (
         "{ 'keyVaultEndpoint': 'doesnotexist.local', 'keyName': 'foo' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_expired, "azure", dkopts, &keyid, &error);
   ASSERT_EXPIRED (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* Azure - invalid hostname. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson (
         "{ 'keyVaultEndpoint': 'doesnotexist.local', 'keyName': 'foo' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_invalid_hostname, "azure", dkopts, &keyid, &error);
   ASSERT_INVALID_HOSTNAME (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* Case 3: GCP - no client cert. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'projectId': 'pid', 'location': 'l', 'keyRing': 'kr', "
                "'keyName': 'kn' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_no_client_cert, "gcp", dkopts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "");
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* GCP - with TLS. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'projectId': 'pid', 'location': 'l', 'keyRing': 'kr', "
                "'keyName': 'kn' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_with_tls, "gcp", dkopts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
                          mongocrypt_errno,
                          "HTTP status=404");
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* GCP - expired. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'projectId': 'pid', 'location': 'l', 'keyRing': 'kr', "
                "'keyName': 'kn' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_expired, "gcp", dkopts, &keyid, &error);
   ASSERT_EXPIRED (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* GCP - invalid hostname. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (
      dkopts,
      tmp_bson ("{ 'projectId': 'pid', 'location': 'l', 'keyRing': 'kr', "
                "'keyName': 'kn' }"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_invalid_hostname, "gcp", dkopts, &keyid, &error);
   ASSERT_INVALID_HOSTNAME (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* Case 4: KMIP - no client cert. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (dkopts,
                                                        tmp_bson ("{}"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_no_client_cert, "kmip", dkopts, &keyid, &error);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "");
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* KMIP - with TLS. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (dkopts,
                                                        tmp_bson ("{}"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_with_tls, "kmip", dkopts, &keyid, &error);
   ASSERT_OR_PRINT (res, error);
   bson_value_destroy (&keyid);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* KMIP - expired. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (dkopts,
                                                        tmp_bson ("{}"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_expired, "kmip", dkopts, &keyid, &error);
   ASSERT_EXPIRED (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   /* KMIP - invalid hostname. */
   memset (&error, 0, sizeof (bson_error_t));
   dkopts = mongoc_client_encryption_datakey_opts_new ();
   mongoc_client_encryption_datakey_opts_set_masterkey (dkopts,
                                                        tmp_bson ("{}"));
   res = mongoc_client_encryption_create_datakey (
      client_encryption_invalid_hostname, "kmip", dkopts, &keyid, &error);
   ASSERT_INVALID_HOSTNAME (error);
   ASSERT (!res);
   mongoc_client_encryption_datakey_opts_destroy (dkopts);

   mongoc_client_encryption_destroy (client_encryption_invalid_hostname);
   mongoc_client_encryption_destroy (client_encryption_expired);
   mongoc_client_encryption_destroy (client_encryption_with_tls);
   mongoc_client_encryption_destroy (client_encryption_no_client_cert);
   mongoc_client_destroy (keyvault_client);
}

static void
test_kms_tls_options_extra_rejected (void *unused)
{
   mongoc_client_encryption_t *ce;
   mongoc_client_encryption_opts_t *ce_opts;
   mongoc_client_t *keyvault_client;
   bson_error_t error;
   bson_t *kms_providers =
      tmp_bson ("{'aws': {'accessKeyId': 'foo', 'secretAccessKey': 'bar'}}");

   keyvault_client = test_framework_new_default_client ();

   /* Test that the "local" KMS provider is rejected. */
   ce_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_keyvault_namespace (
      ce_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (ce_opts, keyvault_client);
   mongoc_client_encryption_opts_set_kms_providers (ce_opts, kms_providers);
   mongoc_client_encryption_opts_set_tls_opts (
      ce_opts, tmp_bson ("{'local': {'tlsCaFile': 'ca.pem'}}"));
   ce = mongoc_client_encryption_new (ce_opts, &error);
   ASSERT_ERROR_CONTAINS (
      error,
      MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
      MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
      "Cannot configure TLS options for KMS provider: local");
   ASSERT (NULL == ce);
   mongoc_client_encryption_opts_destroy (ce_opts);

   /* Test that insecure TLS options are rejected. */
   memset (&error, 0, sizeof (bson_error_t));
   ce_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_keyvault_namespace (
      ce_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (ce_opts, keyvault_client);
   mongoc_client_encryption_opts_set_kms_providers (ce_opts, kms_providers);
   mongoc_client_encryption_opts_set_tls_opts (
      ce_opts, tmp_bson ("{'aws': {'tlsInsecure': true}}"));
   ce = mongoc_client_encryption_new (ce_opts, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "Error setting TLS option tlsInsecure for aws. "
                          "Insecure TLS options prohibited");
   ASSERT (NULL == ce);
   mongoc_client_encryption_opts_destroy (ce_opts);

   /* Test that extra TLS options are rejected. */
   memset (&error, 0, sizeof (bson_error_t));
   ce_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_keyvault_namespace (
      ce_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (ce_opts, keyvault_client);
   mongoc_client_encryption_opts_set_kms_providers (ce_opts, kms_providers);
   mongoc_client_encryption_opts_set_tls_opts (
      ce_opts, tmp_bson ("{'aws': {'extra': true}}"));
   ce = mongoc_client_encryption_new (ce_opts, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "Error setting TLS option extra for aws. Insecure "
                          "TLS options prohibited");
   ASSERT (NULL == ce);
   mongoc_client_encryption_opts_destroy (ce_opts);

   /* Test that TLS options for duplicate providers are rejected. */
   memset (&error, 0, sizeof (bson_error_t));
   ce_opts = mongoc_client_encryption_opts_new ();
   mongoc_client_encryption_opts_set_keyvault_namespace (
      ce_opts, "keyvault", "datakeys");
   mongoc_client_encryption_opts_set_keyvault_client (ce_opts, keyvault_client);
   mongoc_client_encryption_opts_set_kms_providers (ce_opts, kms_providers);
   mongoc_client_encryption_opts_set_tls_opts (
      ce_opts,
      tmp_bson (
         "{'aws': {'tlsCAFile': 'foo.pem'}, 'aws': {'tlsCAFile': 'foo.pem'}}"));
   ce = mongoc_client_encryption_new (ce_opts, &error);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CLIENT_SIDE_ENCRYPTION,
                          MONGOC_ERROR_CLIENT_INVALID_ENCRYPTION_ARG,
                          "Error parsing duplicate TLS options for aws");
   ASSERT (NULL == ce);
   mongoc_client_encryption_opts_destroy (ce_opts);

   mongoc_client_destroy (keyvault_client);
}

void
test_client_side_encryption_install (TestSuite *suite)
{
   install_json_test_suite_with_check (
      suite,
      JSON_DIR,
      "client_side_encryption",
      test_client_side_encryption_cb,
      test_framework_skip_if_no_client_side_encryption);
   /* Prose tests from the spec. */
   TestSuite_AddFull (suite,
                      "/client_side_encryption/datakey_and_double_encryption",
                      test_datakey_and_double_encryption,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8,
                      test_framework_skip_if_offline /* requires AWS */);
   TestSuite_AddFull (
      suite,
      "/client_side_encryption/external_key_vault",
      test_external_key_vault,
      NULL,
      NULL,
      test_framework_skip_if_no_client_side_encryption,
      test_framework_skip_if_max_wire_version_less_than_8,
      test_framework_skip_if_no_auth /* requires auth for error check */);
   TestSuite_AddFull (
      suite,
      "/client_side_encryption/bson_size_limits_and_batch_splitting",
      test_bson_size_limits_and_batch_splitting,
      NULL,
      NULL,
      test_framework_skip_if_no_client_side_encryption,
      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/views_are_prohibited",
                      test_views_are_prohibited,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/corpus",
                      test_corpus,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8,
                      test_framework_skip_if_offline /* requires AWS */);
   TestSuite_AddFull (
      suite,
      "/client_side_encryption/custom_endpoint",
      test_custom_endpoint,
      NULL,
      NULL,
      test_framework_skip_if_no_client_side_encryption,
      test_framework_skip_if_max_wire_version_less_than_8,
      test_framework_skip_if_offline /* requires AWS, Azure, and GCP */);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/bypass_spawning_mongocryptd/"
                      "mongocryptdBypassSpawn",
                      test_bypass_spawning_via_mongocryptdBypassSpawn,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/bypass_spawning_mongocryptd/"
                      "bypassAutoEncryption",
                      test_bypass_spawning_via_bypassAutoEncryption,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/kms_tls/valid",
                      test_kms_tls_cert_valid,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/kms_tls/expired",
                      test_kms_tls_cert_expired,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/kms_tls/wrong_host",
                      test_kms_tls_cert_wrong_host,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);

   /* Other, C driver specific, tests. */
   TestSuite_AddFull (suite,
                      "/client_side_encryption/single_and_pool_mismatches",
                      test_invalid_single_and_pool_mismatches,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/multi_threaded",
                      test_multi_threaded,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (suite,
                      "/client_side_encryption/malformed_explicit",
                      test_malformed_explicit,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption,
                      test_framework_skip_if_max_wire_version_less_than_8);
   TestSuite_AddFull (
      suite,
      "/client_side_encryption/kms_tls_options",
      test_kms_tls_options,
      NULL,
      NULL,
      test_framework_skip_if_no_client_side_encryption,
      test_framework_skip_if_max_wire_version_less_than_8,
      test_framework_skip_if_offline /* requires AWS, Azure, and GCP */,
      /* Do not run on Windows due to CDRIVER-4181. Tests use a literal IP with
         a TLS connection. */
      test_framework_skip_if_windows);

   TestSuite_AddFull (suite,
                      "/client_side_encryption/kms_tls_options/extra_rejected",
                      test_kms_tls_options_extra_rejected,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_client_side_encryption);
}
