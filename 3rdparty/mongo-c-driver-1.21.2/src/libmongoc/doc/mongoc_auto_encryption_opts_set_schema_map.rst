:man_page: mongoc_auto_encryption_opts_set_schema_map

mongoc_auto_encryption_opts_set_schema_map()
============================================

Synopsis
--------

.. code-block:: c

   void
   mongoc_auto_encryption_opts_set_schema_map (mongoc_auto_encryption_opts_t *opts,
                                               const bson_t *schema_map);


Parameters
----------

* ``opts``: The :symbol:`mongoc_auto_encryption_opts_t`
* ``schema_map``: A :symbol:`bson_t` where keys are collection namespaces and values are JSON schemas.

Supplying a schema map provides more security than relying on JSON Schemas obtained from the server. It protects against a malicious server advertising a false JSON Schema, which could trick the client into sending unencrypted data that should be encrypted.

Schemas supplied in the schema map only apply to configuring automatic encryption for client side encryption. Other validation rules in the JSON schema will not be enforced by the driver and will result in an error.

The following is an example of a schema map which configures automatic encryption for the collection ``db.coll``:

.. code-block:: js

   {
        "db.coll": {
            "properties": {
            "encrypted_string": {
                "encrypt": {
                "keyId": [
                    {
                    "$binary": {
                        "base64": "AAAAAAAAAAAAAAAAAAAAAA==",
                        "subType": "04"
                    }
                    }
                ],
                "bsonType": "string",
                "algorithm": "AEAD_AES_256_CBC_HMAC_SHA_512-Deterministic"
                }
            }
            },
            "bsonType": "object"
        }
    }


.. seealso::

  | :symbol:`mongoc_client_enable_auto_encryption()`

  | The guide for :doc:`Using Client-Side Field Level Encryption <using_client_side_encryption>`

