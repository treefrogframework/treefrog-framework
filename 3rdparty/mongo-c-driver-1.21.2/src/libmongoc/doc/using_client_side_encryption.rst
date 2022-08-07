:man-page: using_client_side_encryption

Using Client-Side Field Level Encryption
========================================

New in MongoDB 4.2, Client-Side Field Level Encryption (also referred to as Client-Side Encryption) allows administrators and developers to encrypt specific data fields in addition to other MongoDB encryption features.

With Client-Side Encryption, developers can encrypt fields client side without any server-side configuration or directives. Client-Side Encryption supports workloads where applications must guarantee that unauthorized parties, including server administrators, cannot read the encrypted data.

Automatic encryption, where sensitive fields in commands are encrypted automatically, requires an Enterprise-only process to do query analysis.

Installation
------------

libmongocrypt
`````````````

There is a separate library, `libmongocrypt <https://github.com/mongodb/libmongocrypt>`_, that must be installed prior to configuring libmongoc to enable Client-Side Encryption.

libmongocrypt depends on libbson. To build libmongoc with Client-Side Encryption support you must:

1. Install libbson
2. Build and install libmongocrypt
3. Build libmongoc

To install libbson, follow the instructions to install with a package manager: :ref:`Install libbson with a Package Manager <installing_libbson_with_pkg_manager>` or build from source with cmake (disable building libmongoc with ``-DENABLE_MONGOC=OFF``):

.. parsed-literal::

  $ cd mongo-c-driver
  $ mkdir cmake-build && cd cmake-build
  $ cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_MONGOC=OFF ..
  $ cmake --build . --target install

To build and install libmongocrypt, clone `the repository <https://github.com/mongodb/libmongocrypt>`_ and configure as follows:

.. parsed-literal::

  $ cd libmongocrypt
  $ mkdir cmake-build && cd cmake-build
  $ cmake -DENABLE_SHARED_BSON=ON ..
  $ cmake --build . --target install

Then, you should be able to build libmongoc with Client-Side Encryption.

.. parsed-literal::

  $ cd mongo-c-driver
  $ mkdir cmake-build && cd cmake-build
  $ cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DENABLE_MONGOC=ON -DENABLE_CLIENT_SIDE_ENCRYPTION=ON ..
  $ cmake --build . --target install

mongocryptd
```````````

The ``mongocryptd`` binary is required for automatic Client-Side Encryption and is included as a component in the `MongoDB Enterprise Server package <https://dochub.mongodb.org/core/install-mongodb-enterprise>`_. For detailed installation instructions see the `MongoDB documentation on mongocryptd <https://dochub.mongodb.org/core/client-side-field-level-encryption-mongocryptd>`_.

``mongocryptd`` performs the following:

- Parses the automatic encryption rules specified to the database connection. If the JSON schema contains invalid automatic encryption syntax or any document validation syntax, ``mongocryptd`` returns an error.
- Uses the specified automatic encryption rules to mark fields in read and write operations for encryption.
- Rejects read/write operations that may return unexpected or incorrect results when applied to an encrypted field. For supported and unsupported operations, see `Read/Write Support with Automatic Field Level Encryption <https://dochub.mongodb.org/core/client-side-field-level-encryption-read-write-support>`_.

A :symbol:`mongoc_client_t` configured with auto encryption will automatically spawn the ``mongocryptd`` process from the application's ``PATH``. Applications can control the spawning behavior as part of the automatic encryption options. For example, to set a custom path to the ``mongocryptd`` process, set the ``mongocryptdSpawnPath`` with :symbol:`mongoc_auto_encryption_opts_set_extra()`.

.. code:: c

   bson_t *extra = BCON_NEW ("mongocryptdSpawnPath", "/path/to/mongocryptd");
   mongoc_auto_encryption_opts_set_extra (opts, extra);


To control the logging output of ``mongocryptd`` pass ``mongocryptdSpawnArgs`` to :symbol:`mongoc_auto_encryption_opts_set_extra()`:

.. code:: c

   bson_t *extra = BCON_NEW ("mongocryptdSpawnArgs",
      "[", "--logpath=/path/to/mongocryptd.log", "--logappend", "]");
   mongoc_auto_encryption_opts_set_extra (opts, extra);

If your application wishes to manage the ``mongocryptd`` process manually, it is possible to disable spawning ``mongocryptd``:

.. code:: c

   bson_t *extra = BCON_NEW ("mongocryptdBypassSpawn",
      BCON_BOOL(true), "mongocryptdURI", "mongodb://localhost:27020");
   mongoc_auto_encryption_opts_set_extra (opts, extra);

``mongocryptd`` is only responsible for supporting automatic Client-Side Encryption in the driver and does not itself perform any encryption or decryption.


Automatic Client-Side Field Level Encryption
--------------------------------------------

Automatic Client-Side Encryption is enabled by calling :symbol:`mongoc_client_enable_auto_encryption()` on a :symbol:`mongoc_client_t`. The following examples show how to set up automatic client-side field level encryption using :symbol:`mongoc_client_encryption_t` to create a new encryption data key.

.. note::

   Automatic client-side field level encryption requires MongoDB 4.2 enterprise or a MongoDB 4.2 Atlas cluster. The community version of the server supports automatic decryption as well as :ref:`explicit-client-side-encryption`.

Providing Local Automatic Encryption Rules
``````````````````````````````````````````

The following example shows how to specify automatic encryption rules using a schema map set with :symbol:`mongoc_auto_encryption_opts_set_schema_map()`. The automatic encryption rules are expressed using a strict subset of the JSON Schema syntax.

Supplying a schema map provides more security than relying on JSON Schemas obtained from the server. It protects against a malicious server advertising a false JSON Schema, which could trick the client into sending unencrypted data that should be encrypted.

JSON Schemas supplied in the schema map only apply to configuring automatic client-side field level encryption. Other validation rules in the JSON schema will not be enforced by the driver and will result in an error:

.. literalinclude:: ../examples/client-side-encryption-schema-map.c
   :caption: client-side-encryption-schema-map.c
   :language: c

Server-Side Field Level Encryption Enforcement
``````````````````````````````````````````````

The MongoDB 4.2 server supports using schema validation to enforce encryption of specific fields in a collection. This schema validation will prevent an application from inserting unencrypted values for any fields marked with the "encrypt" JSON schema keyword.

The following example shows how to set up automatic client-side field level encryption using :symbol:`mongoc_client_encryption_t` to create a new encryption data key and create a collection with the Automatic Encryption JSON Schema Syntax:

.. literalinclude:: ../examples/client-side-encryption-server-schema.c
   :caption: client-side-encryption-server-schema.c
   :language: c

.. _explicit-client-side-encryption:

Explicit Encryption
```````````````````

Explicit encryption is a MongoDB community feature and does not use the mongocryptd process. Explicit encryption is provided by the :symbol:`mongoc_client_encryption_t` class, for example:

.. literalinclude:: ../examples/client-side-encryption-explicit.c
   :caption: client-side-encryption-explicit.c
   :language: c

Explicit Encryption with Automatic Decryption
`````````````````````````````````````````````

Although automatic encryption requires MongoDB 4.2 enterprise or a MongoDB 4.2 Atlas cluster, automatic decryption is supported for all users. To configure automatic decryption without automatic encryption set bypass_auto_encryption=True in :symbol:`mongoc_auto_encryption_opts_t`:

.. literalinclude:: ../examples/client-side-encryption-auto-decryption.c
   :caption: client-side-encryption-auto-decryption.c
   :language: c