:man_page: mongoc_auto_encryption_opts_set_extra

mongoc_auto_encryption_opts_set_extra()
=======================================

Synopsis
--------

.. code-block:: c

   void
   mongoc_auto_encryption_opts_set_extra (mongoc_auto_encryption_opts_t *opts,
                                          const bson_t *extra);


Parameters
----------

* ``opts``: The :symbol:`mongoc_auto_encryption_opts_t`
* ``extra``: A :symbol:`bson_t` of additional options.

``extra`` is a :symbol:`bson_t` containing any of the following optional fields:

* ``mongocryptdURI`` set to a URI to connect to the mongocryptd process (default is "mongodb://localhost:27027").
* ``mongocryptdBypassSpawn`` set to true to prevent the driver from spawning the mongocryptd process (default behavior is to spawn).
* ``mongocryptdSpawnPath`` set to a path (with trailing slash) to search for mongocryptd (defaults to empty string and uses default system paths).
* ``mongocryptdSpawnArgs`` set to an array of string arguments to pass to ``mongocryptd`` when spawning (defaults to ``[ "--idleShutdownTimeoutSecs=60" ]``).

For more information, see the `Client-Side Encryption specification <https://github.com/mongodb/specifications/blob/master/source/client-side-encryption/client-side-encryption.rst#extraoptions>`_.

.. seealso::

  | :symbol:`mongoc_client_enable_auto_encryption()`

  | The guide for :doc:`Using Client-Side Field Level Encryption <using_client_side_encryption>`

