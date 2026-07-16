// Demonstrates how to use explicit encryption and decryption using the community version of MongoDB

#include <mongoc/mongoc.h>

#include <stdio.h>
#include <stdlib.h>

#define FAIL(...)                                           \
   fprintf(stderr, "Error [%s:%d]:\n", __FILE__, __LINE__); \
   fprintf(stderr, __VA_ARGS__);                            \
   fprintf(stderr, "\n");                                   \
   abort();

// `init_bson` creates BSON from JSON. Aborts on error. Use the `BSON_STR()` macro to avoid quotes.
#define init_bson(bson, json)                           \
   if (!bson_init_from_json(&bson, json, -1, &error)) { \
      FAIL("Failed to create BSON: %s", error.message); \
   }

int
main(void)
{
   bson_error_t error;

   // The key vault collection stores encrypted data keys:
   const char *keyvault_db_name = "keyvault";
   const char *keyvault_coll_name = "datakeys";

   // Set `local_key` to a 96 byte base64-encoded string:
   const char *local_key =
      "qx/3ydlPRXgUrBvSBWLsllUTaYDcS/pyaVo27qBHkS2AFePjInwhzCmDWHdmCYPmzhO4lRBzeZKFjSafduLL5z5DMvR/"
      "QFfV4zc7btcVmV3QWbDwqZyn6G+Y18ToLHyK";

   const char *uri = "mongodb://localhost/?appname=client-side-encryption";

   mongoc_init();

   // Create client:
   mongoc_client_t *client = mongoc_client_new(uri);
   if (!client) {
      FAIL("Failed to create client");
   }

   // Configure KMS providers used to encrypt data keys:
   bson_t kms_providers;
   {
      char *as_json = bson_strdup_printf(BSON_STR({"local" : {"key" : "%s"}}), local_key);
      init_bson(kms_providers, as_json);
      bson_free(as_json);
   }

   // Set up key vault collection:
   {
      mongoc_collection_t *coll = mongoc_client_get_collection(client, keyvault_db_name, keyvault_coll_name);
      mongoc_collection_drop(coll, NULL); // Clear pre-existing data.

      // Create index to ensure keys have unique keyAltNames:
      bson_t index_keys, index_opts;
      init_bson(index_keys, BSON_STR({"keyAltNames" : 1}));
      init_bson(index_opts,
                BSON_STR({"unique" : true, "partialFilterExpression" : {"keyAltNames" : {"$exists" : true}}}));
      mongoc_index_model_t *index_model = mongoc_index_model_new(&index_keys, &index_opts);
      if (!mongoc_collection_create_indexes_with_opts(
             coll, &index_model, 1, NULL /* opts */, NULL /* reply */, &error)) {
         FAIL("Failed to create index: %s", error.message);
      }

      mongoc_index_model_destroy(index_model);
      bson_destroy(&index_opts);
      bson_destroy(&index_keys);
      mongoc_collection_destroy(coll);
   }

   // Create ClientEncryption object:
   mongoc_client_encryption_t *client_encryption;
   {
      mongoc_client_encryption_opts_t *ce_opts = mongoc_client_encryption_opts_new();
      mongoc_client_encryption_opts_set_kms_providers(ce_opts, &kms_providers);
      mongoc_client_encryption_opts_set_keyvault_namespace(ce_opts, keyvault_db_name, keyvault_coll_name);
      mongoc_client_encryption_opts_set_keyvault_client(ce_opts, client);
      client_encryption = mongoc_client_encryption_new(ce_opts, &error);
      if (!client_encryption) {
         FAIL("Failed to create ClientEncryption: %s", error.message);
      }
      mongoc_client_encryption_opts_destroy(ce_opts);
   }

   // Create data key (see:
   // https://dochub.mongodb.org/core/client-side-field-level-encryption-automatic-encryption-rules):
   bson_value_t datakey_id;
   {
      mongoc_client_encryption_datakey_opts_t *dk_opts = mongoc_client_encryption_datakey_opts_new();
      if (!mongoc_client_encryption_create_datakey(client_encryption, "local", dk_opts, &datakey_id, &error)) {
         FAIL("Failed to create data key: %s", error.message);
      }
      mongoc_client_encryption_datakey_opts_destroy(dk_opts);
   }

   // Explicitly encrypt a value:
   bson_value_t encrypted_value;
   {
      mongoc_client_encryption_encrypt_opts_t *e_opts = mongoc_client_encryption_encrypt_opts_new();
      mongoc_client_encryption_encrypt_opts_set_algorithm(e_opts, MONGOC_AEAD_AES_256_CBC_HMAC_SHA_512_DETERMINISTIC);
      mongoc_client_encryption_encrypt_opts_set_keyid(e_opts, &datakey_id);
      bson_value_t to_encrypt = {.value_type = BSON_TYPE_INT32, .value = {.v_int32 = 123}};
      if (!mongoc_client_encryption_encrypt(client_encryption, &to_encrypt, e_opts, &encrypted_value, &error)) {
         FAIL("Failed to encrypt field: %s", error.message);
      }
      mongoc_client_encryption_encrypt_opts_destroy(e_opts);
   }

   // Explicitly decrypt a value:
   {
      bson_value_t decrypted_value;
      if (!mongoc_client_encryption_decrypt(client_encryption, &encrypted_value, &decrypted_value, &error)) {
         FAIL("Failed to decrypt field: %s", error.message);
      }
      printf("Decrypted value: %" PRId32 "\n", decrypted_value.value.v_int32);
      bson_value_destroy(&decrypted_value);
   }

   bson_value_destroy(&encrypted_value);
   bson_value_destroy(&datakey_id);
   mongoc_client_encryption_destroy(client_encryption);
   bson_destroy(&kms_providers);
   mongoc_client_destroy(client);
   mongoc_cleanup();
   return 0;
}
