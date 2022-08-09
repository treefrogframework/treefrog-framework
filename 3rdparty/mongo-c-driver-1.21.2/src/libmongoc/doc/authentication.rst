:man_page: mongoc_authentication

Authentication
==============

This guide covers the use of authentication options with the MongoDB C Driver. Ensure that the MongoDB server is also properly configured for authentication before making a connection. For more information, see the `MongoDB security documentation <https://docs.mongodb.org/manual/administration/security/>`_.

The MongoDB C driver supports several authentication mechanisms through the use of MongoDB connection URIs.

By default, if a username and password are provided as part of the connection string (and an optional authentication database), they are used to connect via the default authentication mechanism of the server.

To select a specific authentication mechanism other than the default, see the list of supported mechanism below.

.. code-block:: none

  mongoc_client_t *client = mongoc_client_new ("mongodb://user:password@localhost/?authSource=mydb");

Currently supported values for the authMechanism connection string option are:

* :ref:`SCRAM-SHA-1 <authentication_scram_sha_1>`
* :ref:`MONGODB-CR (deprecated) <authentication_mongodbcr>`
* :ref:`GSSAPI <authentication_kerberos>`
* :ref:`PLAIN <authentication_plain>`
* :ref:`X509 <authentication_x509>`
* :ref:`MONGODB-AWS <authentication_aws>`

.. _authentication_scram_sha_256:

Basic Authentication (SCRAM-SHA-256)
------------------------------------

MongoDB 4.0 introduces support for authenticating using the SCRAM protocol
with the more secure SHA-256 hash described in `RFC 7677
<https://tools.ietf.org/html/rfc7677>`_. Using this authentication mechanism
means that the password is never actually sent over the wire when
authenticating, but rather a computed proof that the client password is the
same as the password the server knows. In MongoDB 4.0, the C driver can
determine the correct default authentication mechanism for users with stored
SCRAM-SHA-1 and SCRAM-SHA-256 credentials:

.. code-block:: none

  mongoc_client_t *client =  mongoc_client_new ("mongodb://user:password@localhost/?authSource=mydb");
  /* the correct authMechanism is negotiated between the driver and server. */

Alternatively, SCRAM-SHA-256 can be explicitly specified as an authMechanism.

.. code-block:: none

  mongoc_client_t *client =  mongoc_client_new ("mongodb://user:password@localhost/?authMechanism=SCRAM-SHA-256&authSource=mydb");

Passwords for SCRAM-SHA-256 undergo the preprocessing step known as SASLPrep
specified in `RFC 4013 <https://tools.ietf.org/html/rfc4013>`_. SASLPrep will
only be performed for passwords containing non-ASCII characters.  SASLPrep
requires libicu. If libicu is not available, attempting to authenticate over
SCRAM-SHA-256 with non-ASCII passwords will result in error.

Usernames *never* undergo SASLPrep.

By default, when building the C driver libicu is linked if available. This can
be changed with the ``ENABLE_ICU`` cmake option. To specify an installation
path of libicu, specify ``ICU_ROOT`` as a cmake option. See the
`FindICU <https://cmake.org/cmake/help/v3.7/module/FindICU.html>`_ documentation
for more information.


.. _authentication_scram_sha_1:

Basic Authentication (SCRAM-SHA-1)
----------------------------------

The default authentication mechanism before MongoDB 4.0 is ``SCRAM-SHA-1`` (`RFC 5802 <http://tools.ietf.org/html/rfc5802>`_). Using this authentication mechanism means that the password is never actually sent over the wire when authenticating, but rather a computed proof that the client password is the same as the password the server knows.

.. code-block:: none

  mongoc_client_t *client = mongoc_client_new ("mongodb://user:password@localhost/?authMechanism=SCRAM-SHA-1&authSource=mydb");

.. note::

  ``SCRAM-SHA-1`` authenticates against the ``admin`` database by default. If the user is created in another database, then specifying the authSource is required.

.. _authentication_mongodbcr:

Legacy Authentication (MONGODB-CR)
----------------------------------

The MONGODB-CR authMechanism is deprecated and will no longer function in MongoDB 4.0. Instead, specify no authMechanism and the driver
will use an authentication mechanism compatible with your server.

.. _authentication_kerberos:

GSSAPI (Kerberos) Authentication
--------------------------------

.. note::

  Kerberos support requires compiling the driver against ``cyrus-sasl`` on UNIX-like environments. On Windows, configure the driver to build against the Windows Native SSPI.

``GSSAPI`` (Kerberos) authentication is available in the Enterprise Edition of MongoDB. To authenticate using ``GSSAPI``, the MongoDB C driver must be installed with SASL support.

On UNIX-like environments, run the ``kinit`` command before using the following authentication methods:

.. code-block:: none

  $ kinit mongodbuser@EXAMPLE.COM
  mongodbuser@EXAMPLE.COM's Password:
  $ klistCredentials cache: FILE:/tmp/krb5cc_1000
          Principal: mongodbuser@EXAMPLE.COM

    Issued                Expires               Principal
  Feb  9 13:48:51 2013  Feb  9 23:48:51 2013  krbtgt/EXAMPLE.COM@EXAMPLE.COM

Now authenticate using the MongoDB URI. ``GSSAPI`` authenticates against the ``$external`` virtual database, so a database does not need to be specified in the URI. Note that the Kerberos principal *must* be URL-encoded:

.. code-block:: none

  mongoc_client_t *client;

  client = mongoc_client_new ("mongodb://mongodbuser%40EXAMPLE.COM@mongo-server.example.com/?authMechanism=GSSAPI");

.. note::

  ``GSSAPI`` authenticates against the ``$external`` database, so specifying the authSource database is not required.

The driver supports these GSSAPI properties:

* ``CANONICALIZE_HOST_NAME``: This might be required with Cyrus-SASL when the hosts report different hostnames than what is used in the Kerberos database. The default is "false".
* ``SERVICE_NAME``: Use a different service name than the default, "mongodb".

Set properties in the URL:

.. code-block:: none

  mongoc_client_t *client;

  client = mongoc_client_new ("mongodb://mongodbuser%40EXAMPLE.COM@mongo-server.example.com/?authMechanism=GSSAPI&"
                              "authMechanismProperties=SERVICE_NAME:other,CANONICALIZE_HOST_NAME:true");

If you encounter errors such as ``Invalid net address``, check if the application is behind a NAT (Network Address Translation) firewall. If so, create a ticket that uses ``forwardable`` and ``addressless`` Kerberos tickets. This can be done by passing ``-f -A`` to ``kinit``.

.. code-block:: none

  $ kinit -f -A mongodbuser@EXAMPLE.COM

.. _authentication_plain:

SASL Plain Authentication
-------------------------

.. note::

  The MongoDB C Driver must be compiled with SASL support in order to use ``SASL PLAIN`` authentication.

MongoDB Enterprise Edition supports the ``SASL PLAIN`` authentication mechanism, initially intended for delegating authentication to an LDAP server. Using the ``SASL PLAIN`` mechanism is very similar to the challenge response mechanism with usernames and passwords. This authentication mechanism uses the ``$external`` virtual database for ``LDAP`` support:

.. note::

  ``SASL PLAIN`` is a clear-text authentication mechanism. It is strongly recommended to connect to MongoDB using TLS with certificate validation when using the ``PLAIN`` mechanism.

.. code-block:: none

  mongoc_client_t *client;

  client = mongoc_client_new ("mongodb://user:password@example.com/?authMechanism=PLAIN");

``PLAIN`` authenticates against the ``$external`` database, so specifying the authSource database is not required.

.. _authentication_x509:

X.509 Certificate Authentication
--------------------------------

.. note::

  The MongoDB C Driver must be compiled with TLS support for X.509 authentication support. Once this is done, start a server with the following options:

  .. code-block:: none

    $ mongod --tlsMode requireTLS --tlsCertificateKeyFile server.pem --tlsCAFile ca.pem

The ``MONGODB-X509`` mechanism authenticates a username derived from the distinguished subject name of the X.509 certificate presented by the driver during TLS negotiation. This authentication method requires the use of TLS connections with certificate validation.

.. code-block:: none

  mongoc_client_t *client;
  mongoc_ssl_opt_t ssl_opts = { 0 };

  ssl_opts.pem_file = "mycert.pem";
  ssl_opts.pem_pwd = "mycertpassword";
  ssl_opts.ca_file = "myca.pem";
  ssl_opts.ca_dir = "trust_dir";
  ssl_opts.weak_cert_validation = false;

  client = mongoc_client_new ("mongodb://x509_derived_username@localhost/?authMechanism=MONGODB-X509");
  mongoc_client_set_ssl_opts (client, &ssl_opts);

``MONGODB-X509`` authenticates against the ``$external`` database, so specifying the authSource database is not required. For more information on the x509_derived_username, see the MongoDB server `x.509 tutorial <https://docs.mongodb.com/manual/tutorial/configure-x509-client-authentication/#add-x-509-certificate-subject-as-a-user>`_.

.. note::

  The MongoDB C Driver will attempt to determine the x509 derived username when none is provided, and as of MongoDB 3.4 providing the username is not required at all.

.. _authentication_aws:

Authentication via AWS IAM
--------------------------

The ``MONGODB-AWS`` mechanism authenticates to MongoDB servers with credentials provided by AWS Identity and Access Management (IAM).

To authenticate, create a user with an associated Amazon Resource Name (ARN) on the ``$external`` database, and specify the ``MONGODB-AWS`` ``authMechanism`` in the URI.

.. code-block:: c

   mongoc_uri_t *uri = mongoc_uri_new ("mongodb://localhost/?authMechanism=MONGODB-AWS");

Since ``MONGODB-AWS`` always authenticates against the ``$external`` database, so specifying the authSource database is not required.

Credentials include the ``access key id``, ``secret access key``, and optional ``session token``. They may be obtained from the following ways.

AWS credentials via URI
```````````````````````

Credentials may be passed directly in the URI as username/password.

.. code-block:: c

   mongoc_uri_t *uri = mongoc_uri_new ("mongodb://<access key id>:<secret access key>localhost/?authMechanism=MONGODB-AWS");

This may include a ``session token`` passed with ``authMechanismProperties``.

.. code-block:: c

   mongoc_uri_t *uri = mongoc_uri_new ("mongodb://<access key id>:<secret access key>localhost/?authMechanism=MONGODB-AWS&authMechanismProperties=AWS_SESSION_TOKEN:<token>");

AWS credentials via environment
```````````````````````````````

If credentials are not passed through the URI, libmongoc will check for the following environment variables.

- AWS_ACCESS_KEY_ID
- AWS_SECRET_ACCESS_KEY
- AWS_SESSION_TOKEN (optional)

AWS Credentials via ECS
```````````````````````

If credentials are not passed in the URI or with environment variables, libmongoc will check if the environment variable ``AWS_CONTAINER_CREDENTIALS_RELATIVE_URI`` is set, and if so, attempt to retrieve temporary credentials from the ECS task metadata by querying a link local address.

AWS Credentials via EC2
```````````````````````

If credentials are not passed in the URI or with environment variables, and the environment variable ``AWS_CONTAINER_CREDENTIALS_RELATIVE_URI`` is not set, libmongoc will attempt to retrieve temporary credentials from the EC2 machine metadata by querying link local addresses.

.. only:: html

  .. include:: includes/seealso/authmechanism.txt
