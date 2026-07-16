/*
 * Copyright 2009-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mongoc/mongoc-prelude.h>

#ifndef MONGOC_STREAM_TLS_OPENSSL_PRIVATE_H
#define MONGOC_STREAM_TLS_OPENSSL_PRIVATE_H

#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <mongoc/mongoc-stream-tls.h>

#include <bson/bson.h>

#include <openssl/ssl.h>

BSON_BEGIN_DECLS

typedef struct {
   char *host;
   bool allow_invalid_hostname;
   bool weak_cert_validation;
   bool disable_endpoint_check;
   /* If reaching out to an OCSP responder requires TLS,
    * use the same TLS options that the user provided. */
   mongoc_ssl_opt_t ssl_opts;
} mongoc_openssl_ocsp_opt_t;

void
mongoc_openssl_ocsp_opt_destroy(void *ocsp_opt);

/**
 * mongoc_stream_tls_openssl_t:
 *
 * Private storage for handling callbacks from mongoc_stream and BIO_*
 */
typedef struct {
   BIO *bio;
   BIO_METHOD *meth;
   SSL_CTX *ctx;
   mongoc_openssl_ocsp_opt_t *ocsp_opts;
} mongoc_stream_tls_openssl_t;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
MONGOC_EXPORT(mongoc_stream_t *)
mongoc_stream_tls_openssl_new_with_context(mongoc_stream_t *base_stream,
                                           const char *host,
                                           mongoc_ssl_opt_t *opt,
                                           int client,
                                           SSL_CTX *ssl_ctx) BSON_GNUC_WARN_UNUSED_RESULT;
#endif

BSON_END_DECLS

#endif /* MONGOC_ENABLE_SSL_OPENSSL */
#endif /* MONGOC_STREAM_TLS_OPENSSL_PRIVATE_H */
