:man_page: mongoc_auto_encryption_opts_set_kms_providers

mongoc_auto_encryption_opts_set_kms_providers()
===============================================

Synopsis
--------

.. code-block:: c

   void
   mongoc_auto_encryption_opts_set_kms_providers (
      mongoc_auto_encryption_opts_t *opts, const bson_t *kms_providers);


Parameters
----------

* ``opts``: The :symbol:`mongoc_auto_encryption_opts_t`
* ``kms_providers``: A :symbol:`bson_t` containing configuration for an external Key Management Service (KMS).

``kms_providers`` is a BSON document containing configuration for each KMS provider. Currently ``aws``, ``local``, ``azure``, ``gcp``, and ``kmip`` are supported. At least one must be specified.

The format for "aws" is as follows:

.. code-block:: javascript

   aws: {
      accessKeyId: String,
      secretAccessKey: String
   }

The format for "local" is as follows:

.. code-block:: javascript

   local: {
      key: <96 byte BSON binary of subtype 0> or String /* The master key used to encrypt/decrypt data keys. May be passed as a base64 encoded string. */
   }

The format for "azure" is as follows:

.. code-block:: javascript

   azure: {
      tenantId: String,
      clientId: String,
      clientSecret: String,
      identityPlatformEndpoint: Optional<String> /* Defaults to login.microsoftonline.com */
   }

The format for "gcp" is as follows:

.. code-block:: javascript

   gcp: {
      email: String,
      privateKey: byte[] or String, /* May be passed as a base64 encoded string. */
      endpoint: Optional<String> /* Defaults to oauth2.googleapis.com */
   }

The format for "kmip" is as follows:

.. code-block:: javascript

   kmip: {
      endpoint: String
   }

.. seealso::

  | :symbol:`mongoc_client_enable_auto_encryption()`

  | The guide for :doc:`Using Client-Side Field Level Encryption <using_client_side_encryption>`

