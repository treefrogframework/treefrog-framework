/*
 * Copyright 2016-present MongoDB, Inc.
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

#include <bson/bson.h>

#ifdef _POSIX_VERSION
#include <sys/utsname.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "mongoc-linux-distro-scanner-private.h"
#include "mongoc-handshake.h"
#include "mongoc-handshake-compiler-private.h"
#include "mongoc-handshake-os-private.h"
#include "mongoc-handshake-private.h"
#include "mongoc-client.h"
#include "mongoc-client-private.h"
#include "mongoc-error.h"
#include "mongoc-log.h"
#include "mongoc-version.h"
#include "mongoc-util-private.h"

/*
 * Global handshake data instance. Initialized at startup from mongoc_init
 *
 * Can be modified by calls to mongoc_handshake_data_append
 */
static mongoc_handshake_t gMongocHandshake;

/*
 * Used for thread-safety in mongoc_handshake_data_append
 */
static bson_mutex_t gHandshakeLock;

static void
_set_bit (uint8_t *bf, uint32_t byte_count, uint32_t bit)
{
   uint32_t byte = bit / 8;
   uint32_t bit_of_byte = (bit) % 8;
   /* byte 0 is the last location in bf. */
   bf[(byte_count - 1) - byte] |= 1u << bit_of_byte;
}

/* returns a hex string for all config flag bits, which must be freed. */
char *
_mongoc_handshake_get_config_hex_string (void)
{
   uint32_t byte_count;
   uint8_t *bf;
   bson_string_t *str;
   int i;

   byte_count = (LAST_MONGOC_MD_FLAG + 7) / 8; /* ceil (num_bits / 8) */
   /* allocate enough bytes to fit all config bits. */
   bf = (uint8_t *) bson_malloc0 (byte_count);

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
   _set_bit (bf, byte_count, MONGOC_ENABLE_SSL_SECURE_CHANNEL);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_CNG
   _set_bit (bf, byte_count, MONGOC_ENABLE_CRYPTO_CNG);
#endif

#ifdef MONGOC_ENABLE_SSL_SECURE_TRANSPORT
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SSL_SECURE_TRANSPORT);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO_COMMON_CRYPTO);
#endif

#ifdef MONGOC_ENABLE_SSL_OPENSSL
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SSL_OPENSSL);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_LIBCRYPTO
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO_LIBCRYPTO);
#endif

#ifdef MONGOC_ENABLE_SSL
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SSL);
#endif

#ifdef MONGOC_ENABLE_CRYPTO
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO_SYSTEM_PROFILE);
#endif

#ifdef MONGOC_ENABLE_SASL
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SASL);
#endif

#ifdef MONGOC_HAVE_SASL_CLIENT_DONE
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_HAVE_SASL_CLIENT_DONE);
#endif

#ifdef MONGOC_NO_AUTOMATIC_GLOBALS
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_NO_AUTOMATIC_GLOBALS);
#endif

#ifdef MONGOC_EXPERIMENTAL_FEATURES
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_EXPERIMENTAL_FEATURES);
#endif

#ifdef MONGOC_ENABLE_SSL_LIBRESSL
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SSL_LIBRESSL);
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SASL_CYRUS);
#endif

#ifdef MONGOC_ENABLE_SASL_SSPI
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SASL_SSPI);
#endif

#ifdef MONGOC_HAVE_SOCKLEN
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_HAVE_SOCKLEN);
#endif

#ifdef MONGOC_ENABLE_COMPRESSION
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_COMPRESSION);
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_COMPRESSION_SNAPPY);
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_COMPRESSION_ZLIB);
#endif

#ifdef MONGOC_HAVE_RES_NSEARCH
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_NSEARCH);
#endif

#ifdef MONGOC_HAVE_RES_NDESTROY
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_NDESTROY);
#endif

#ifdef MONGOC_HAVE_RES_NCLOSE
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_NCLOSE);
#endif

#ifdef MONGOC_HAVE_RES_SEARCH
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_SEARCH);
#endif

#ifdef MONGOC_HAVE_DNSAPI
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_DNSAPI);
#endif

#ifdef MONGOC_HAVE_RDTSCP
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_RDTSCP);
#endif

#ifdef MONGOC_HAVE_SCHED_GETCPU
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_HAVE_SCHED_GETCPU);
#endif

#ifdef MONGOC_ENABLE_SHM_COUNTERS
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_SHM_COUNTERS);
#endif

   if (MONGOC_TRACE_ENABLED) {
      _set_bit (bf, byte_count, MONGOC_MD_FLAG_TRACE);
   }

#ifdef MONGOC_ENABLE_ICU
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_ICU);
#endif

#ifdef MONGOC_ENABLE_CLIENT_SIDE_ENCRYPTION
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_CLIENT_SIDE_ENCRYPTION);
#endif

#ifdef MONGOC_ENABLE_MONGODB_AWS_AUTH
   _set_bit (bf, byte_count, MONGOC_MD_FLAG_ENABLE_MONGODB_AWS_AUTH);
#endif

   str = bson_string_new ("0x");
   for (i = 0; i < byte_count; i++) {
      bson_string_append_printf (str, "%02x", bf[i]);
   }
   bson_free (bf);
   /* free the bson_string_t, but keep the underlying char* alive. */
   return bson_string_free (str, false);
}

static char *
_get_os_type (void)
{
#ifdef MONGOC_OS_TYPE
   return bson_strndup (MONGOC_OS_TYPE, HANDSHAKE_OS_TYPE_MAX);
#else
   return bson_strndup ("unknown", HANDSHAKE_OS_TYPE_MAX);
#endif
}

static char *
_get_os_architecture (void)
{
   const char *ret = NULL;

#ifdef _WIN32
   SYSTEM_INFO system_info;
   DWORD arch;
   GetSystemInfo (&system_info);

   arch = system_info.wProcessorArchitecture;

   switch (arch) {
   case PROCESSOR_ARCHITECTURE_AMD64:
      ret = "x86_64";
      break;
   case PROCESSOR_ARCHITECTURE_ARM:
      ret = "ARM";
      break;
   case PROCESSOR_ARCHITECTURE_IA64:
      ret = "IA64";
      break;
   case PROCESSOR_ARCHITECTURE_INTEL:
      ret = "x86";
      break;
   case PROCESSOR_ARCHITECTURE_UNKNOWN:
      ret = "Unknown";
      break;
   default:
      ret = "Other";
      break;
   }

#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname (&system_info) >= 0) {
      ret = system_info.machine;
   }

#endif

   if (ret) {
      return bson_strndup (ret, HANDSHAKE_OS_ARCHITECTURE_MAX);
   }

   return NULL;
}

#ifndef MONGOC_OS_IS_LINUX
static char *
_get_os_name (void)
{
#ifdef MONGOC_OS_NAME
   return bson_strndup (MONGOC_OS_NAME, HANDSHAKE_OS_NAME_MAX);
#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname (&system_info) >= 0) {
      return bson_strndup (system_info.sysname, HANDSHAKE_OS_NAME_MAX);
   }

#endif

   return NULL;
}

static char *
_get_os_version (void)
{
   char *ret = bson_malloc (HANDSHAKE_OS_VERSION_MAX);
   bool found = false;

#ifdef _WIN32
   OSVERSIONINFO osvi;
   ZeroMemory (&osvi, sizeof (OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

   if (GetVersionEx (&osvi)) {
      bson_snprintf (ret,
                     HANDSHAKE_OS_VERSION_MAX,
                     "%lu.%lu (%lu)",
                     osvi.dwMajorVersion,
                     osvi.dwMinorVersion,
                     osvi.dwBuildNumber);
      found = true;
   } else {
      MONGOC_WARNING ("Error with GetVersionEx(): %lu", GetLastError ());
   }

#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname (&system_info) >= 0) {
      bson_strncpy (ret, system_info.release, HANDSHAKE_OS_VERSION_MAX);
      found = true;
   } else {
      MONGOC_WARNING ("Error with uname(): %d", errno);
   }

#endif

   if (!found) {
      bson_free (ret);
      ret = NULL;
   }

   return ret;
}
#endif

static void
_get_system_info (mongoc_handshake_t *handshake)
{
   handshake->os_type = _get_os_type ();

#ifdef MONGOC_OS_IS_LINUX
   _mongoc_linux_distro_scanner_get_distro (&handshake->os_name,
                                            &handshake->os_version);
#else
   handshake->os_name = _get_os_name ();
   handshake->os_version = _get_os_version ();
#endif

   handshake->os_architecture = _get_os_architecture ();
}

static void
_free_system_info (mongoc_handshake_t *handshake)
{
   bson_free (handshake->os_type);
   bson_free (handshake->os_name);
   bson_free (handshake->os_version);
   bson_free (handshake->os_architecture);
}

static void
_get_driver_info (mongoc_handshake_t *handshake)
{
   handshake->driver_name = bson_strndup ("mongoc", HANDSHAKE_DRIVER_NAME_MAX);
   handshake->driver_version =
      bson_strndup (MONGOC_VERSION_S, HANDSHAKE_DRIVER_VERSION_MAX);
}

static void
_free_driver_info (mongoc_handshake_t *handshake)
{
   bson_free (handshake->driver_name);
   bson_free (handshake->driver_version);
}

static void
_set_platform_string (mongoc_handshake_t *handshake)
{
   bson_string_t *str;

   str = bson_string_new ("");

   handshake->platform = bson_string_free (str, false);
}

static void
_set_compiler_info (mongoc_handshake_t *handshake)
{
   bson_string_t *str;
   char *config_str;

   str = bson_string_new ("");

   config_str = _mongoc_handshake_get_config_hex_string ();
   bson_string_append_printf (str, "cfg=%s", config_str);
   bson_free (config_str);

#ifdef _POSIX_VERSION
   bson_string_append_printf (str, " posix=%ld", _POSIX_VERSION);
#endif

#ifdef __STDC_VERSION__
   bson_string_append_printf (str, " stdc=%ld", __STDC_VERSION__);
#endif

   bson_string_append_printf (str, " CC=%s", MONGOC_COMPILER);

#ifdef MONGOC_COMPILER_VERSION
   bson_string_append_printf (str, " %s", MONGOC_COMPILER_VERSION);
#endif
   handshake->compiler_info = bson_string_free (str, false);
}

static void
_set_flags (mongoc_handshake_t *handshake)
{
   bson_string_t *str;

   str = bson_string_new ("");

   if (strlen (MONGOC_EVALUATE_STR (MONGOC_USER_SET_CFLAGS)) > 0) {
      bson_string_append_printf (
         str, " CFLAGS=%s", MONGOC_EVALUATE_STR (MONGOC_USER_SET_CFLAGS));
   }

   if (strlen (MONGOC_EVALUATE_STR (MONGOC_USER_SET_LDFLAGS)) > 0) {
      bson_string_append_printf (
         str, " LDFLAGS=%s", MONGOC_EVALUATE_STR (MONGOC_USER_SET_LDFLAGS));
   }

   handshake->flags = bson_string_free (str, false);
}

static void
_free_platform_string (mongoc_handshake_t *handshake)
{
   bson_free (handshake->platform);
   bson_free (handshake->compiler_info);
   bson_free (handshake->flags);
}

void
_mongoc_handshake_init (void)
{
   _get_system_info (_mongoc_handshake_get ());
   _get_driver_info (_mongoc_handshake_get ());
   _set_platform_string (_mongoc_handshake_get ());
   _set_compiler_info (_mongoc_handshake_get ());
   _set_flags (_mongoc_handshake_get ());

   _mongoc_handshake_get ()->frozen = false;
   bson_mutex_init (&gHandshakeLock);
}

void
_mongoc_handshake_cleanup (void)
{
   _free_system_info (_mongoc_handshake_get ());
   _free_driver_info (_mongoc_handshake_get ());
   _free_platform_string (_mongoc_handshake_get ());

   bson_mutex_destroy (&gHandshakeLock);
}

static void
_append_platform_field (bson_t *doc, const char *platform)
{
   int max_platform_str_size;

   char *compiler_info = _mongoc_handshake_get ()->compiler_info;
   char *flags = _mongoc_handshake_get ()->flags;
   bson_string_t *combined_platform = bson_string_new (platform);

   /* Compute space left for platform field */
   max_platform_str_size =
      HANDSHAKE_MAX_SIZE - ((int) doc->len +
                            /* 1 byte for utf8 tag */
                            1 +

                            /* key size */
                            (int) strlen (HANDSHAKE_PLATFORM_FIELD) + 1 +

                            /* 4 bytes for length of string */
                            4);

   if (max_platform_str_size <= 0) {
      bson_string_free (combined_platform, true);
      return;
   }

   /* We opt to drop compiler info and flags if they can't fit, while the
    * platform information is truncated
    * Try to drop flags first, and if there is still not enough space also drop
    * compiler info */
   if (max_platform_str_size >
       combined_platform->len + strlen (compiler_info) + 1) {
      bson_string_append (combined_platform, compiler_info);
   }
   if (max_platform_str_size > combined_platform->len + strlen (flags) + 1) {
      bson_string_append (combined_platform, flags);
   }

   /* We use the flags_index field to check if the CLAGS/LDFLAGS need to be
    * truncated, and if so we drop them altogether */
   bson_append_utf8 (
      doc,
      HANDSHAKE_PLATFORM_FIELD,
      -1,
      combined_platform->str,
      BSON_MIN (max_platform_str_size - 1, combined_platform->len));

   bson_string_free (combined_platform, true);
   BSON_ASSERT (doc->len <= HANDSHAKE_MAX_SIZE);
}

/*
 * Return true if we build the document, and it's not too big
 * false if there's no way to prevent the doc from being too big. In this
 * case, the caller shouldn't include it with hello
 */
bool
_mongoc_handshake_build_doc_with_application (bson_t *doc, const char *appname)
{
   const mongoc_handshake_t *md = _mongoc_handshake_get ();
   bson_t child;

   if (appname) {
      BSON_APPEND_DOCUMENT_BEGIN (doc, "application", &child);
      BSON_APPEND_UTF8 (&child, "name", appname);
      bson_append_document_end (doc, &child);
   }

   BSON_APPEND_DOCUMENT_BEGIN (doc, "driver", &child);
   BSON_APPEND_UTF8 (&child, "name", md->driver_name);
   BSON_APPEND_UTF8 (&child, "version", md->driver_version);
   bson_append_document_end (doc, &child);

   BSON_APPEND_DOCUMENT_BEGIN (doc, "os", &child);

   BSON_ASSERT (md->os_type);
   BSON_APPEND_UTF8 (&child, "type", md->os_type);

   if (md->os_name) {
      BSON_APPEND_UTF8 (&child, "name", md->os_name);
   }

   if (md->os_version) {
      BSON_APPEND_UTF8 (&child, "version", md->os_version);
   }

   if (md->os_architecture) {
      BSON_APPEND_UTF8 (&child, "architecture", md->os_architecture);
   }

   bson_append_document_end (doc, &child);

   if (doc->len > HANDSHAKE_MAX_SIZE) {
      /* We've done all we can possibly do to ensure the current
       * document is below the maxsize, so if it overflows there is
       * nothing else we can do, so we fail */
      return false;
   }

   if (md->platform) {
      _append_platform_field (doc, md->platform);
   }

   return true;
}

void
_mongoc_handshake_freeze (void)
{
   _mongoc_handshake_get ()->frozen = true;
}

/*
 * free (*s) and make *s point to *s concated with suffix.
 * If *s is NULL it's treated like it's an empty string.
 */
static void
_append_and_truncate (char **s, const char *suffix, int max_len)
{
   char *old_str = *s;
   char *prefix;
   const int delim_len = (int) strlen (" / ");
   int space_for_suffix;

   BSON_ASSERT_PARAM (s);
   BSON_ASSERT_PARAM (suffix);

   prefix = old_str ? old_str : "";

   space_for_suffix = max_len - (int) strlen (prefix) - delim_len;

   if (space_for_suffix <= 0) {
      /* the old string already takes the whole allotted space */
      return;
   }

   *s = bson_strdup_printf ("%s / %.*s", prefix, space_for_suffix, suffix);
   BSON_ASSERT (strlen (*s) <= max_len);

   bson_free (old_str);
}


/*
 * Set some values in our global handshake struct. These values will be sent
 * to the server as part of the initial connection handshake (hello).
 * If this function is called more than once, or after we've connected to a
 * mongod, then it will do nothing and return false. It will return true if it
 * successfully sets the values.
 *
 * All arguments are optional.
 */
bool
mongoc_handshake_data_append (const char *driver_name,
                              const char *driver_version,
                              const char *platform)
{
   int platform_space;

   bson_mutex_lock (&gHandshakeLock);

   if (_mongoc_handshake_get ()->frozen) {
      bson_mutex_unlock (&gHandshakeLock);
      return false;
   }

   BSON_ASSERT (_mongoc_handshake_get ()->platform);

   /* allow practically any size for "platform", we'll trim it down in
    * _mongoc_handshake_build_doc_with_application */
   platform_space =
      HANDSHAKE_MAX_SIZE - (int) strlen (_mongoc_handshake_get ()->platform);

   if (platform) {
      /* we check for an empty string as a special case to avoid an unnecessary
       * delimiter being added in front of the string by _append_and_truncate */
      if (_mongoc_handshake_get ()->platform[0] == '\0') {
         bson_free (_mongoc_handshake_get ()->platform);
         _mongoc_handshake_get ()->platform =
            bson_strdup_printf ("%.*s", platform_space, platform);
      } else {
         _append_and_truncate (
            &_mongoc_handshake_get ()->platform, platform, HANDSHAKE_MAX_SIZE);
      }
   }

   if (driver_name) {
      _append_and_truncate (&_mongoc_handshake_get ()->driver_name,
                            driver_name,
                            HANDSHAKE_DRIVER_NAME_MAX);
   }

   if (driver_version) {
      _append_and_truncate (&_mongoc_handshake_get ()->driver_version,
                            driver_version,
                            HANDSHAKE_DRIVER_VERSION_MAX);
   }

   _mongoc_handshake_freeze ();
   bson_mutex_unlock (&gHandshakeLock);

   return true;
}

mongoc_handshake_t *
_mongoc_handshake_get (void)
{
   return &gMongocHandshake;
}

bool
_mongoc_handshake_appname_is_valid (const char *appname)
{
   return strlen (appname) <= MONGOC_HANDSHAKE_APPNAME_MAX;
}

void
_mongoc_handshake_append_sasl_supported_mechs (const mongoc_uri_t *uri,
                                               bson_t *cmd)
{
   const char *username;
   char *db_user;
   username = mongoc_uri_get_username (uri);
   db_user =
      bson_strdup_printf ("%s.%s", mongoc_uri_get_auth_source (uri), username);
   bson_append_utf8 (cmd, "saslSupportedMechs", 18, db_user, -1);
   bson_free (db_user);
}

void
_mongoc_handshake_parse_sasl_supported_mechs (
   const bson_t *hello,
   mongoc_handshake_sasl_supported_mechs_t *sasl_supported_mechs)
{
   bson_iter_t iter;
   memset (sasl_supported_mechs, 0, sizeof (*sasl_supported_mechs));
   if (bson_iter_init_find (&iter, hello, "saslSupportedMechs")) {
      bson_iter_t array_iter;
      if (BSON_ITER_HOLDS_ARRAY (&iter) &&
          bson_iter_recurse (&iter, &array_iter)) {
         while (bson_iter_next (&array_iter)) {
            if (BSON_ITER_HOLDS_UTF8 (&array_iter)) {
               const char *mechanism_name = bson_iter_utf8 (&array_iter, NULL);
               if (0 == strcmp (mechanism_name, "SCRAM-SHA-256")) {
                  sasl_supported_mechs->scram_sha_256 = true;
               } else if (0 == strcmp (mechanism_name, "SCRAM-SHA-1")) {
                  sasl_supported_mechs->scram_sha_1 = true;
               }
            }
         }
      }
   }
}
