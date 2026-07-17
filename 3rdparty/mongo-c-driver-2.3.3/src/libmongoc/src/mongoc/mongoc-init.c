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


#include <mongoc/mongoc-init.h>

#include <mongoc/mongoc-cluster-aws-private.h>
#include <mongoc/mongoc-counters-private.h>
#include <mongoc/mongoc-handshake-private.h>

#include <mongoc/mongoc-config.h>

#include <bson/bson.h>

#include <mlib/config.h>

#ifdef MONGOC_ENABLE_SSL_OPENSSL
#include <mongoc/mongoc-openssl-private.h>
#endif
#include <common-b64-private.h>
#include <mongoc/mongoc-thread-private.h>
#if defined(MONGOC_ENABLE_CRYPTO_CNG)
#include <mongoc/mongoc-crypto-cng-private.h>
#include <mongoc/mongoc-crypto-private.h>
#endif

#ifdef MONGOC_ENABLE_MONGODB_AWS_AUTH
#include <kms_message/kms_message.h>
#endif

#ifdef MONGOC_ENABLE_OCSP_OPENSSL
#include <mongoc/mongoc-ocsp-cache-private.h>
#endif

// CDRIVER-2722: Cyrus SASL is deprecated on MacOS.
#if defined(MONGOC_ENABLE_SASL_CYRUS) && defined(__APPLE__)
BEGIN_IGNORE_DEPRECATIONS
#endif // defined(MONGOC_ENABLE_SASL_CYRUS) && defined(__APPLE__)

#ifdef MONGOC_ENABLE_SASL_CYRUS
#include <sasl/sasl.h>

static void *
mongoc_cyrus_mutex_alloc(void)
{
   bson_mutex_t *mutex;

   mutex = (bson_mutex_t *)bson_malloc0(sizeof(bson_mutex_t));
   bson_mutex_init(mutex);

   return (void *)mutex;
}


static int
mongoc_cyrus_mutex_lock(void *mutex)
{
   bson_mutex_lock((bson_mutex_t *)mutex);

   return SASL_OK;
}


static int
mongoc_cyrus_mutex_unlock(void *mutex)
{
   bson_mutex_unlock((bson_mutex_t *)mutex);

   return SASL_OK;
}


static void
mongoc_cyrus_mutex_free(void *mutex)
{
   bson_mutex_destroy((bson_mutex_t *)mutex);
   bson_free(mutex);
}

#endif /* MONGOC_ENABLE_SASL_CYRUS */

static bool mongoc_init_called;
bool
mongoc_get_init_called(void)
{
   return mongoc_init_called;
}

static BSON_ONCE_FUN(_mongoc_do_init)
{
   mongoc_init_called = true;
#ifdef MONGOC_ENABLE_SASL_CYRUS
   int status;
#endif
#ifdef MONGOC_ENABLE_SSL_OPENSSL
   _mongoc_openssl_init();
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
   /* The following functions should not use tracing, as they may be invoked
    * before mongoc_log_set_handler() can complete. */
   sasl_set_mutex(
      mongoc_cyrus_mutex_alloc, mongoc_cyrus_mutex_lock, mongoc_cyrus_mutex_unlock, mongoc_cyrus_mutex_free);

   status = sasl_client_init(NULL);
   BSON_ASSERT(status == SASL_OK);
#endif

   _mongoc_counters_init();

#ifdef _WIN32
   {
      WORD wVersionRequested;
      WSADATA wsaData;
      int err;

      wVersionRequested = MAKEWORD(2, 2);

      err = WSAStartup(wVersionRequested, &wsaData);

      /* check the version perhaps? */

      BSON_ASSERT(err == 0);
   }
#endif

#if defined(MONGOC_ENABLE_CRYPTO_CNG)
   mongoc_crypto_cng_init();
#endif

   _mongoc_handshake_init();

#if defined(MONGOC_ENABLE_MONGODB_AWS_AUTH)
   kms_message_init();
   _mongoc_aws_credentials_cache_init();
#endif

#if defined(MONGOC_ENABLE_OCSP_OPENSSL)
   _mongoc_ocsp_cache_init();
#endif

   BSON_ONCE_RETURN;
}

void
mongoc_init(void)
{
   static bson_once_t once = BSON_ONCE_INIT;
   bson_once(&once, _mongoc_do_init);
}

static BSON_ONCE_FUN(_mongoc_do_cleanup)
{
#ifdef MONGOC_ENABLE_SSL_OPENSSL
   _mongoc_openssl_cleanup();
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
#ifdef MONGOC_HAVE_SASL_CLIENT_DONE
   sasl_client_done();
#else
   /* fall back to deprecated function */
   sasl_done();
#endif
#endif

#ifdef _WIN32
   WSACleanup();
#endif

#if defined(MONGOC_ENABLE_CRYPTO_CNG)
   mongoc_crypto_cng_cleanup();
#endif

   _mongoc_counters_cleanup();

   _mongoc_handshake_cleanup();

#if defined(MONGOC_ENABLE_MONGODB_AWS_AUTH)
   kms_message_cleanup();
   _mongoc_aws_credentials_cache_cleanup();
#endif

#if defined(MONGOC_ENABLE_OCSP_OPENSSL)
   _mongoc_ocsp_cache_cleanup();
#endif

   BSON_ONCE_RETURN;
}

void
mongoc_cleanup(void)
{
   static bson_once_t once = BSON_ONCE_INIT;
   bson_once(&once, _mongoc_do_cleanup);
}

// CDRIVER-2722: Cyrus SASL is deprecated on MacOS.
#if defined(MONGOC_ENABLE_SASL_CYRUS) && defined(__APPLE__)
BEGIN_IGNORE_DEPRECATIONS
#endif // defined(MONGOC_ENABLE_SASL_CYRUS) && defined(__APPLE__)
