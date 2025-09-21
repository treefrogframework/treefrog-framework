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

#ifndef MONGOC_STREAM_TLS_PRIVATE_H
#define MONGOC_STREAM_TLS_PRIVATE_H

#include <mongoc/mongoc-ssl.h>
#include <mongoc/mongoc-stream.h>

#include <bson/bson.h>

#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <openssl/ssl.h>
#endif

#include <mongoc/mongoc-shared-private.h>

BSON_BEGIN_DECLS

/**
 * mongoc_stream_tls_t:
 *
 * Overloaded mongoc_stream_t with additional TLS handshake and verification
 * callbacks.
 *
 */
struct _mongoc_stream_tls_t {
   mongoc_stream_t parent;       /* The TLS stream wrapper */
   mongoc_stream_t *base_stream; /* The underlying actual stream */
   void *ctx;                    /* TLS lib specific configuration or wrappers */
   int64_t timeout_msec;
   mongoc_ssl_opt_t ssl_opts;
   bool (*handshake) (mongoc_stream_t *stream, const char *host, int *events /* OUT*/, bson_error_t *error);
};

#if defined(MONGOC_ENABLE_SSL_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
MONGOC_EXPORT (mongoc_stream_t *)
mongoc_stream_tls_new_with_hostname_and_openssl_context (mongoc_stream_t *base_stream,
                                                         const char *host,
                                                         mongoc_ssl_opt_t *opt,
                                                         int client,
                                                         SSL_CTX *ssl_ctx) BSON_GNUC_WARN_UNUSED_RESULT;
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
mongoc_stream_t *
mongoc_stream_tls_new_with_secure_channel_cred (mongoc_stream_t *base_stream,
                                                mongoc_ssl_opt_t *opt,
                                                mongoc_shared_ptr secure_channel_cred_ptr) BSON_GNUC_WARN_UNUSED_RESULT;
#endif // MONGOC_ENABLE_SSL_SECURE_CHANNEL

BSON_END_DECLS

#endif /* MONGOC_STREAM_TLS_PRIVATE_H */
