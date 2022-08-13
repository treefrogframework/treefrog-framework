:man_page: mongoc_configuring_tls

Configuring TLS
===============

Configuration with URI options
------------------------------

Enable TLS by including ``tls=true`` in the URI.

.. code:: c
   
  mongoc_uri_t *uri = mongoc_uri_new ("mongodb://localhost:27017/");
  mongoc_uri_set_option_as_bool (uri, MONGOC_URI_TLS, true);

  mongoc_client_t *client = mongoc_client_new_from_uri (uri);


The following URI options may be used to further configure TLS:

.. include:: includes/tls-options.txt

Configuration with mongoc_ssl_opt_t
-----------------------------------

Alternatively, the :symbol:`mongoc_ssl_opt_t` struct may be used to configure TLS with :symbol:`mongoc_client_set_ssl_opts()` or :symbol:`mongoc_client_pool_set_ssl_opts()`. Most of the configurable options can be set using the `Connection String URI <https://docs.mongodb.org/manual/reference/connection-string/>`_.

===============================  ===============================
**mongoc_ssl_opt_t key**         **URI key**
===============================  ===============================
pem_file                         tlsClientCertificateKeyFile
pem_pwd                          tlsClientCertificateKeyPassword
ca_file                          tlsCAFile
weak_cert_validation             tlsAllowInvalidCertificates
allow_invalid_hostname           tlsAllowInvalidHostnames
===============================  ===============================

The only exclusions are ``crl_file`` and ``ca_dir``. Those may only be set with :symbol:`mongoc_ssl_opt_t`.

Client Authentication
---------------------

When MongoDB is started with TLS enabled, it will by default require the client to provide a client certificate issued by a certificate authority specified by ``--tlsCAFile``, or an authority trusted by the native certificate store in use on the server.

To provide the client certificate, set the ``tlsCertificateKeyFile`` in the URI to a PEM armored certificate file.

.. code-block:: c

  mongoc_uri_t *uri = mongoc_uri_new ("mongodb://localhost:27017/");
  mongoc_uri_set_option_as_bool (uri, MONGOC_URI_TLS, true);
  mongoc_uri_set_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "/path/to/client-certificate.pem");

  mongoc_client_t *client = mongoc_client_new_from_uri (uri);

Server Certificate Verification
-------------------------------

The MongoDB C Driver will automatically verify the validity of the server certificate, such as issued by configured Certificate Authority, hostname validation, and expiration.

To overwrite this behavior, it is possible to disable hostname validation, OCSP endpoint revocation checking, revocation checking entirely, and allow invalid certificates.

This behavior is controlled using the ``tlsAllowInvalidHostnames``, ``tlsDisableOCSPEndpointCheck``, ``tlsDisableCertificateRevocationCheck``, and ``tlsAllowInvalidCertificates`` options respectively. By default, all are set to ``false``.

It is not recommended to change these defaults as it exposes the client to *Man In The Middle* attacks (when ``tlsAllowInvalidHostnames`` is set), invalid certificates (when ``tlsAllowInvalidCertificates`` is set), or potentially revoked certificates (when ``tlsDisableOCSPEndpointCheck`` or ``tlsDisableCertificateRevocationCheck`` are set).

Supported Libraries
-------------------

By default, libmongoc will attempt to find a supported TLS library and enable TLS support. This is controlled by the cmake flag ``ENABLE_SSL``, which is set to ``AUTO`` by default. Valid values are:

- ``AUTO`` the default behavior. Link to the system's native TLS library, or attempt to find OpenSSL.
- ``DARWIN`` link to Secure Transport, the native TLS library on macOS.
- ``WINDOWS`` link to Secure Channel, the native TLS library on Windows.
- ``OPENSSL`` link to OpenSSL (libssl). An optional install path may be specified with ``OPENSSL_ROOT``.
- ``LIBRESSL`` link to LibreSSL's libtls. (LibreSSL's compatible libssl may be linked to by setting ``OPENSSL``).
- ``OFF`` disable TLS support.

OpenSSL
```````

The MongoDB C Driver uses OpenSSL, if available, on Linux and Unix platforms (besides macOS). Industry best practices and some regulations require the use of TLS 1.1 or newer, which requires at least OpenSSL 1.0.1. Check your OpenSSL version like so::

  $ openssl version

Ensure your system's OpenSSL is a recent version (at least 1.0.1), or install a recent version in a non-system path and build against it with::

  cmake -DOPENSSL_ROOT_DIR=/absolute/path/to/openssl

When compiled against OpenSSL, the driver will attempt to load the system default certificate store, as configured by the distribution. That can be overridden by setting the ``tlsCAFile`` URI option or with the fields ``ca_file`` and ``ca_dir`` in the :symbol:`mongoc_ssl_opt_t`.

The Online Certificate Status Protocol (OCSP) (see `RFC 6960 <https://tools.ietf.org/html/rfc6960>`_) is fully supported when using OpenSSL 1.0.1+ with the following notes:

- When a ``crl_file`` is set with :symbol:`mongoc_ssl_opt_t`, and the ``crl_file`` revokes the server's certificate, the certificate is considered revoked (even if the certificate has a valid stapled OCSP response)

LibreSSL / libtls
`````````````````

The MongoDB C Driver supports LibreSSL through the use of OpenSSL compatibility checks when configured to compile against ``openssl``. It also supports the new ``libtls`` library when configured to build against ``libressl``.

When compiled against the Windows native libraries, the ``crl_file`` option of a :symbol:`mongoc_ssl_opt_t` is not supported, and will issue an error if used.

Setting ``tlsDisableOCSPEndpointCheck`` and ``tlsDisableCertificateRevocationCheck`` has no effect.

The Online Certificate Status Protocol (OCSP) (see `RFC 6960 <https://tools.ietf.org/html/rfc6960>`_) is partially supported with the following notes:

- The Must-Staple extension (see `RFC 7633 <https://tools.ietf.org/html/rfc7633>`_) is ignored. Connection may continue if a Must-Staple certificate is presented with no stapled response (unless the client receives a revoked response from an OCSP responder).
- Connection will continue if a Must-Staple certificate is presented without a stapled response and the OCSP responder is down.

Native TLS Support on Windows (Secure Channel)
``````````````````````````````````````````````

The MongoDB C Driver supports the Windows native TLS library (Secure Channel, or SChannel), and its native crypto library (Cryptography API: Next Generation, or CNG).

When compiled against the Windows native libraries, the ``ca_dir`` option of a :symbol:`mongoc_ssl_opt_t` is not supported, and will issue an error if used.

Encrypted PEM files (e.g., setting ``tlsCertificateKeyPassword``) are also not supported, and will result in error when attempting to load them.

When ``tlsCAFile`` is set, the driver will only allow server certificates issued by the authority (or authorities) provided. When no ``tlsCAFile`` is set, the driver will look up the Certificate Authority using the ``System Local Machine Root`` certificate store to confirm the provided certificate.

When ``crl_file`` is set with :symbol:`mongoc_ssl_opt_t`, the driver will import the revocation list to the ``System Local Machine Root`` certificate store.

Setting ``tlsDisableOCSPEndpointCheck`` has no effect.

The Online Certificate Status Protocol (OCSP) (see `RFC 6960 <https://tools.ietf.org/html/rfc6960>`_) is partially supported with the following notes:

- The Must-Staple extension (see `RFC 7633 <https://tools.ietf.org/html/rfc7633>`_) is ignored. Connection may continue if a Must-Staple certificate is presented with no stapled response (unless the client receives a revoked response from an OCSP responder).
- When a ``crl_file`` is set with :symbol:`mongoc_ssl_opt_t`, and the ``crl_file`` revokes the server's certificate, the OCSP response takes precedence. E.g. if the server presents a certificate with a valid stapled OCSP response, the certificate is considered valid even if the ``crl_file`` marks it as revoked.
- Connection will continue if a Must-Staple certificate is presented without a stapled response and the OCSP responder is down.

.. _Secure Transport:

Native TLS Support on macOS / Darwin (Secure Transport)
```````````````````````````````````````````````````````

The MongoDB C Driver supports the Darwin (OS X, macOS, iOS, etc.) native TLS library (Secure Transport), and its native crypto library (Common Crypto, or CC).

When compiled against Secure Transport, the ``ca_dir`` and ``crl_file`` options of a :symbol:`mongoc_ssl_opt_t` are not supported. An error is issued if either are used.

When ``tlsCAFile`` is set, the driver will only allow server certificates issued by the authority (or authorities) provided. When no ``tlsCAFile`` is set, the driver will use the Certificate Authorities in the currently unlocked keychains.

Setting ``tlsDisableOCSPEndpointCheck`` and ``tlsDisableCertificateRevocationCheck`` has no effect.

The Online Certificate Status Protocol (OCSP) (see `RFC 6960 <https://tools.ietf.org/html/rfc6960>`_) is partially supported with the following notes.

- The Must-Staple extension (see `RFC 7633 <https://tools.ietf.org/html/rfc7633>`_) is ignored. Connection may continue if a Must-Staple certificate is presented with no stapled response (unless the client receives a revoked response from an OCSP responder).
- Connection will continue if a Must-Staple certificate is presented without a stapled response and the OCSP responder is down.
