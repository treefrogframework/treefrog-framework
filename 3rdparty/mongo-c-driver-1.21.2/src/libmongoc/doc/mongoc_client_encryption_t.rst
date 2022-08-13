:man_page: mongoc_client_encryption_t

mongoc_client_encryption_t
==========================

Synopsis
--------

.. code-block:: c

   typedef struct _mongoc_client_encryption_t mongoc_client_encryption_t;


``mongoc_client_encryption_t`` provides utility functions for Client-Side Field Level Encryption. See the guide for :doc:`Using Client-Side Field Level Encryption <using_client_side_encryption>`.

Thread Safety
-------------

:symbol:`mongoc_client_encryption_t` is NOT thread-safe and should only be used in the same thread as the :symbol:`mongoc_client_t` that is configured via :symbol:`mongoc_client_encryption_opts_set_keyvault_client()`.

Lifecycle
---------

The key vault client, configured via :symbol:`mongoc_client_encryption_opts_set_keyvault_client()`, must outlive the :symbol:`mongoc_client_encryption_t`.

.. only:: html

  Functions
  ---------

  .. toctree::
    :titlesonly:
    :maxdepth: 1

    mongoc_client_encryption_new
    mongoc_client_encryption_destroy
    mongoc_client_encryption_create_datakey
    mongoc_client_encryption_encrypt
    mongoc_client_encryption_decrypt

.. seealso::

  | :symbol:`mongoc_client_enable_auto_encryption()`

  | :symbol:`mongoc_client_pool_enable_auto_encryption()`

  | The guide for :doc:`Using Client-Side Field Level Encryption <using_client_side_encryption>` for libmongoc

  | The MongoDB Manual for `Client-Side Field Level Encryption <https://docs.mongodb.com/manual/core/security-client-side-encryption/>`_

