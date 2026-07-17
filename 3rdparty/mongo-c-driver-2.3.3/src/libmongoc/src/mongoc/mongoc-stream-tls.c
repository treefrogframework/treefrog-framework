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

#include <mongoc/mongoc-config.h>

#ifdef MONGOC_ENABLE_SSL

#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-stream-private.h>
#include <mongoc/mongoc-stream-tls-private.h>
#include <mongoc/mongoc-trace-private.h>

#include <mongoc/mongoc-log.h>

#include <bson/bson.h>

#include <errno.h>
#include <string.h>
#if defined(MONGOC_ENABLE_SSL_OPENSSL)
#include <mongoc/mongoc-openssl-private.h>

#include <mongoc/mongoc-stream-tls-openssl.h>
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
#include <mongoc/mongoc-secure-transport-private.h>

#include <mongoc/mongoc-stream-tls-secure-transport.h>
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
#include <mongoc/mongoc-secure-channel-private.h>

#include <mongoc/mongoc-stream-tls-secure-channel.h>
#endif
#include <common-macros-private.h> // BEGIN_IGNORE_DEPRECATIONS

#include <mongoc/mongoc-stream-tls.h>

#include <mlib/cmp.h>

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "stream-tls"


/**
 * mongoc_stream_tls_handshake:
 *
 * Performs TLS handshake dance
 */
bool
mongoc_stream_tls_handshake(
   mongoc_stream_t *stream, const char *host, int32_t timeout_msec, int *events, bson_error_t *error)
{
   mongoc_stream_tls_t *stream_tls = (mongoc_stream_tls_t *)mongoc_stream_get_tls_stream(stream);

   BSON_ASSERT(stream_tls);
   BSON_ASSERT(stream_tls->handshake);

   stream_tls->timeout_msec = timeout_msec;

   return stream_tls->handshake(stream, host, events, error);
}

bool
mongoc_stream_tls_handshake_block(mongoc_stream_t *stream, const char *host, int32_t timeout_msec, bson_error_t *error)
{
   int events;
   ssize_t ret = 0;
   mongoc_stream_poll_t poller;
   int64_t expire = 0;

   if (timeout_msec >= 0) {
      expire = bson_get_monotonic_time() + (timeout_msec * 1000);
   }

   /*
    * error variables get re-used a lot. To prevent cross-contamination of error
    * messages, and still be able to provide a generic failure message when
    * mongoc_stream_tls_handshake fails without a specific reason, we need to
    * init
    * the error code to 0.
    */
   if (error) {
      error->code = 0;
   }
   do {
      events = 0;

      if (mongoc_stream_tls_handshake(stream, host, timeout_msec, &events, error)) {
         return true;
      }

      if (events) {
         poller.stream = stream;
         poller.events = events;
         poller.revents = 0;

         if (expire >= 0) {
            const int64_t now = bson_get_monotonic_time();
            const int64_t remaining = expire - now;
            if (remaining < 0) {
               _mongoc_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "TLS handshake timed out.");
               return false;
            } else {
               const int64_t msec = remaining / 1000;
               BSON_ASSERT(mlib_in_range(int32_t, msec));
               timeout_msec = (int32_t)msec;
            }
         }
         ret = mongoc_stream_poll(&poller, 1, timeout_msec);
      }
   } while (events && ret > 0);

   if (error && !error->code) {
      _mongoc_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "TLS handshake failed.");
   }
   return false;
}

// mongoc_stream_tls_new_with_hostname creates a TLS stream.
//
// base_stream: underlying data stream. Ownership is transferred to the returned stream on success.
// host: hostname used to verify the the server certificate.
// opt: TLS options.
// client: indicates a client or server stream. Secure Channel implementation does not support server streams.
//
// Side effect: May set opt->allow_invalid_hostname to true.
//
// Returns a new stream on success. Returns `NULL` on failure.
mongoc_stream_t *
mongoc_stream_tls_new_with_hostname(mongoc_stream_t *base_stream, const char *host, mongoc_ssl_opt_t *opt, int client)
{
   BSON_ASSERT_PARAM(base_stream);
   BSON_OPTIONAL_PARAM(host);
   BSON_ASSERT_PARAM(opt);

   /* !client is only used for testing,
    * when the streams are pretending to be the server */
   if (!client || opt->weak_cert_validation) {
      opt->allow_invalid_hostname = true;
   }

#ifndef _WIN32
   /* Silly check for Unix Domain Sockets */
   if (!host || (host[0] == '/' && !access(host, F_OK))) {
      opt->allow_invalid_hostname = true;
   }
#endif

#if defined(MONGOC_ENABLE_SSL_OPENSSL)
   return mongoc_stream_tls_openssl_new(base_stream, host, opt, client);
#elif defined(MONGOC_ENABLE_SSL_SECURE_TRANSPORT)
   return mongoc_stream_tls_secure_transport_new(base_stream, host, opt, client);
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   return mongoc_stream_tls_secure_channel_new(base_stream, host, opt, client);
#else
#error "Don't know how to create TLS stream"
#endif
}

#if defined(MONGOC_ENABLE_SSL_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
// Create an OpenSSL TLS stream with a shared context.
//
// This is an internal extension to mongoc_stream_tls_new_with_hostname.
//
// base_stream: underlying data stream. Ownership is transferred to the returned stream on success.
// host: hostname used to verify the the server certificate.
// opt: TLS options.
// client: indicates a client or server stream.
// ssl_ctx: shared context.
//
// Side effect: May set opt->allow_invalid_hostname to true for compatibility with mongoc_stream_tls_new_with_hostname.
//
// Returns a new stream on success. Returns `NULL` on failure.
mongoc_stream_t *
mongoc_stream_tls_new_with_hostname_and_openssl_context(
   mongoc_stream_t *base_stream, const char *host, mongoc_ssl_opt_t *opt, int client, SSL_CTX *ssl_ctx)
{
   BSON_ASSERT_PARAM(base_stream);
   BSON_OPTIONAL_PARAM(host);
   BSON_ASSERT_PARAM(opt);
   BSON_OPTIONAL_PARAM(ssl_ctx);

   /* !client is only used for testing,
    * when the streams are pretending to be the server */
   if (!client || opt->weak_cert_validation) {
      opt->allow_invalid_hostname = true;
   }

#ifndef _WIN32
   /* Silly check for Unix Domain Sockets */
   if (!host || (host[0] == '/' && !access(host, F_OK))) {
      opt->allow_invalid_hostname = true;
   }
#endif

   return mongoc_stream_tls_openssl_new_with_context(base_stream, host, opt, client, ssl_ctx);
}
#endif

#if defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
// Create a Secure Channel TLS stream with shared credentials.
//
// This is an internal extension to mongoc_stream_tls_new_with_hostname.
//
// base_stream: underlying data stream. Ownership is transferred to the returned stream on success.
// opt: TLS options.
// secure_channel_cred_ptr: optional shared credentials. May be MONGOC_SHARED_PTR_NULL.
//
// Side effect: May set opt->allow_invalid_hostname to true for compatibility with mongoc_stream_tls_new_with_hostname.
//
// Returns a new stream on success. Returns `NULL` on failure.
mongoc_stream_t *
mongoc_stream_tls_new_with_secure_channel_cred(mongoc_stream_t *base_stream,
                                               const char *host,
                                               mongoc_ssl_opt_t *opt,
                                               mongoc_shared_ptr secure_channel_cred_ptr)
{
   BSON_ASSERT_PARAM(base_stream);
   BSON_ASSERT_PARAM(opt);

   if (opt->weak_cert_validation) {
      // For compatibility with `mongoc_stream_tls_new_with_hostname`, modify `opt` directly:
      opt->allow_invalid_hostname = true;
   }
   return mongoc_stream_tls_secure_channel_new_with_creds(base_stream, host, opt, secure_channel_cred_ptr);
}
#endif // MONGOC_ENABLE_SSL_SECURE_CHANNEL

#endif
