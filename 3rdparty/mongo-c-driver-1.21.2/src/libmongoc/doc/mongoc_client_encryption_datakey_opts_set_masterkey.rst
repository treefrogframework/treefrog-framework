:man_page: mongoc_client_encryption_datakey_opts_set_masterkey

mongoc_client_encryption_datakey_opts_set_masterkey()
=====================================================

Synopsis
--------

.. code-block:: c

   void
   mongoc_client_encryption_datakey_opts_set_masterkey (
      mongoc_client_encryption_datakey_opts_t *opts, const bson_t *masterkey);

Identifies the masterkey for the Key Management Service (KMS) provider used to encrypt a new data key.

Parameters
----------

* ``opts``: A :symbol:`mongoc_client_encryption_datakey_opts_t`
* ``masterkey``: A :symbol:`bson_t` document describing the KMS provider specific masterkey.

Description
-----------

Setting the masterkey is required when creating a data key with the KMS providers: ``aws``, ``azure``, ``gcp``, and ``kmip``.

Setting the masterkey is prohibited with the KMS provider ``local``.

The format of ``masterkey`` for "aws" is as follows:

.. code-block:: javascript

   {
      region: String,
      key: String, /* The Amazon Resource Name (ARN) to the AWS customer master key (CMK). */
      endpoint: Optional<String> /* An alternate host identifier to send KMS requests to. May include port number. Defaults to "kms.<region>.amazonaws.com" */
   }

The format of ``masterkey`` for "azure" is as follows:

.. code-block:: javascript

   {
      keyVaultEndpoint: String, /* Host with optional port. Example: "example.vault.azure.net". */
      keyName: String,
      keyVersion: Optional<String> /* A specific version of the named key, defaults to using the key's primary version. */
   }

The format of ``masterkey`` for "gcp" is as follows:

.. code-block:: javascript

   {
      projectId: String,
      location: String,
      keyRing: String,
      keyName: String,
      keyVersion: Optional<String>, /* A specific version of the named key, defaults to using the key's primary version. */
      endpoint: Optional<String> /* Host with optional port. Defaults to "cloudkms.googleapis.com". */
   }

The format of ``masterkey`` for "kmip" is as follows:

.. code-block:: javascript

   {
      keyId: Optional<String>,
      endpoint: Optional<String> /* Host with optional port. */
   }