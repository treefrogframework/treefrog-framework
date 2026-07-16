// Demonstrates automatic encryption with a client-side schema map. Requires mongocryptd/crypt_shared.

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

   // The encrypted collection stores application data:
   const char *encrypted_db_name = "db";
   const char *encrypted_coll_name = "coll";

   // Set `local_key` to a 96 byte base64-encoded string:
   const char *local_key =
      "qx/3ydlPRXgUrBvSBWLsllUTaYDcS/pyaVo27qBHkS2AFePjInwhzCmDWHdmCYPmzhO4lRBzeZKFjSafduLL5z5DMvR/"
      "QFfV4zc7btcVmV3QWbDwqZyn6G+Y18ToLHyK";

   const char *uri = "mongodb://localhost/?appname=client-side-encryption";

   mongoc_init();

   // Configure KMS providers used to encrypt data keys:
   bson_t kms_providers;
   {
      char *as_json = bson_strdup_printf(BSON_STR({"local" : {"key" : "%s"}}), local_key);
      init_bson(kms_providers, as_json);
      bson_free(as_json);
   }

   // Set up key vault collection:
   mongoc_client_t *keyvault_client;
   {
      keyvault_client = mongoc_client_new(uri);
      if (!keyvault_client) {
         FAIL("Failed to create keyvault client");
      }
      mongoc_collection_t *coll = mongoc_client_get_collection(keyvault_client, keyvault_db_name, keyvault_coll_name);
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
      mongoc_client_encryption_opts_set_keyvault_client(ce_opts, keyvault_client);
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

   // Create a schema map:
   bson_t schema_map = BSON_INITIALIZER;
   {
      /*
         {
            "db.coll": {
               "properties" : {
                  "encryptedField" : {
                     "encrypt" : {
                        "keyId" : [ "<key ID>" ],
                        "bsonType" : "string",
                        "algorithm" : "AEAD_AES_256_CBC_HMAC_SHA_512-Deterministic"
                     }
                  }
               },
               "bsonType" : "object"
            }
         }
      */
      bson_t key_ids = BSON_INITIALIZER;
      BSON_APPEND_VALUE(&key_ids, "0", &datakey_id);

      bson_t encrypt = BSON_INITIALIZER;
      BSON_APPEND_ARRAY(&encrypt, "keyId", &key_ids);
      BSON_APPEND_UTF8(&encrypt, "bsonType", "string");
      BSON_APPEND_UTF8(&encrypt, "algorithm", "AEAD_AES_256_CBC_HMAC_SHA_512-Deterministic");

      bson_t encryptedField = BSON_INITIALIZER;
      BSON_APPEND_DOCUMENT(&encryptedField, "encrypt", &encrypt);

      bson_t properties = BSON_INITIALIZER;
      BSON_APPEND_DOCUMENT(&properties, "encryptedField", &encryptedField);

      bson_t db_coll = BSON_INITIALIZER;
      BSON_APPEND_DOCUMENT(&db_coll, "properties", &properties);
      BSON_APPEND_UTF8(&db_coll, "bsonType", "object");

      BSON_APPEND_DOCUMENT(&schema_map, "db.coll", &db_coll);

      bson_destroy(&key_ids);
      bson_destroy(&db_coll);
      bson_destroy(&encrypt);
      bson_destroy(&encryptedField);
      bson_destroy(&properties);
   }

   // Create client configured to automatically encrypt:
   mongoc_client_t *encrypted_client;
   {
      encrypted_client = mongoc_client_new(uri);
      if (!encrypted_client) {
         FAIL("Failed to create client");
      }
      mongoc_auto_encryption_opts_t *ae_opts = mongoc_auto_encryption_opts_new();
      mongoc_auto_encryption_opts_set_schema_map(ae_opts, &schema_map);
      mongoc_auto_encryption_opts_set_keyvault_namespace(ae_opts, keyvault_db_name, keyvault_coll_name);
      mongoc_auto_encryption_opts_set_kms_providers(ae_opts, &kms_providers);
      if (!mongoc_client_enable_auto_encryption(encrypted_client, ae_opts, &error)) {
         FAIL("Failed to enable auto encryption: %s", error.message);
      }
      mongoc_auto_encryption_opts_destroy(ae_opts);
   }

   // Insert a document:
   mongoc_collection_t *encrypted_coll =
      mongoc_client_get_collection(encrypted_client, encrypted_db_name, encrypted_coll_name);
   {
      mongoc_collection_drop(encrypted_coll, NULL); // Clear pre-existing data.

      bson_t to_insert = BSON_INITIALIZER;
      BSON_APPEND_UTF8(&to_insert, "encryptedField", "foobar");
      if (!mongoc_collection_insert_one(encrypted_coll, &to_insert, NULL /* opts */, NULL /* reply */, &error)) {
         FAIL("Failed to insert: %s", error.message);
      }
      char *as_str = bson_as_relaxed_extended_json(&to_insert, NULL);
      printf("Inserted document with automatic encryption: %s\n", as_str);

      bson_free(as_str);
      bson_destroy(&to_insert);
   }

   // Retrieve document with automatic decryption:
   {
      bson_t filter = BSON_INITIALIZER;
      mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(encrypted_coll, &filter, NULL, NULL);
      const bson_t *result;
      if (!mongoc_cursor_next(cursor, &result)) {
         FAIL("Failed to find inserted document: %s", error.message);
      }
      char *as_str = bson_as_relaxed_extended_json(result, NULL);
      printf("Retrieved document with automatic decryption: %s\n", as_str);
      bson_free(as_str);
      mongoc_cursor_destroy(cursor);
      bson_destroy(&filter);
   }

   // Retrieve document without decryption:
   {
      mongoc_collection_t *unencrypted_coll =
         mongoc_client_get_collection(keyvault_client, encrypted_db_name, encrypted_coll_name);
      bson_t filter = BSON_INITIALIZER;
      mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(unencrypted_coll, &filter, NULL, NULL);
      const bson_t *result;
      if (!mongoc_cursor_next(cursor, &result)) {
         FAIL("Failed to find inserted document: %s", error.message);
      }
      char *as_str = bson_as_relaxed_extended_json(result, NULL);
      printf("Retrieved document without automatic decryption: %s\n", as_str);
      bson_free(as_str);
      mongoc_cursor_destroy(cursor);
      bson_destroy(&filter);
      mongoc_collection_destroy(unencrypted_coll);
   }

   mongoc_collection_destroy(encrypted_coll);
   mongoc_client_destroy(encrypted_client);
   bson_destroy(&schema_map);
   bson_value_destroy(&datakey_id);
   mongoc_client_encryption_destroy(client_encryption);
   bson_destroy(&kms_providers);
   mongoc_client_destroy(keyvault_client);
   mongoc_cleanup();
   return 0;
}
