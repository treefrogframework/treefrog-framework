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

#include <bson/bson.h>

#ifdef _POSIX_VERSION
#include <sys/utsname.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <common-bson-dsl-private.h>
#include <common-string-private.h>
#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-config-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-handshake-compiler-private.h>
#include <mongoc/mongoc-handshake-os-private.h>
#include <mongoc/mongoc-handshake-private.h>
#include <mongoc/mongoc-linux-distro-scanner-private.h>
#include <mongoc/mongoc-util-private.h>

#include <mongoc/mongoc-client.h>
#include <mongoc/mongoc-error.h>
#include <mongoc/mongoc-handshake.h>
#include <mongoc/mongoc-log.h>
#include <mongoc/mongoc-version.h>

#include <bson/compat.h>

#include <mlib/cmp.h>
#include <mlib/config.h>

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
_set_bit(uint8_t *bf, uint32_t byte_count, uint32_t bit)
{
   uint32_t byte = bit / 8;
   uint32_t bit_of_byte = (bit) % 8;
   /* byte 0 is the last location in bf. */
   bf[(byte_count - 1) - byte] |= 1u << bit_of_byte;
}

/* returns a hex string for all config flag bits, which must be freed. */
char *
_mongoc_handshake_get_config_hex_string(void)
{
   const uint32_t byte_count = (LAST_MONGOC_MD_FLAG + 7) / 8; /* ceil (num_bits / 8) */
   /* allocate enough bytes to fit all config bits. */
   uint8_t *const bf = (uint8_t *)bson_malloc0(byte_count);

#ifdef MONGOC_ENABLE_SSL_SECURE_CHANNEL
   _set_bit(bf, byte_count, MONGOC_ENABLE_SSL_SECURE_CHANNEL);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_CNG
   _set_bit(bf, byte_count, MONGOC_ENABLE_CRYPTO_CNG);
#endif

#ifdef MONGOC_HAVE_BCRYPT_PBKDF2
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_HAVE_BCRYPT_PBKDF2);
#endif

#ifdef MONGOC_ENABLE_SSL_SECURE_TRANSPORT
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SSL_SECURE_TRANSPORT);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO_COMMON_CRYPTO);
#endif

#ifdef MONGOC_ENABLE_SSL_OPENSSL
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SSL_OPENSSL);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_LIBCRYPTO
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO_LIBCRYPTO);
#endif

#ifdef MONGOC_ENABLE_SSL
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SSL);
#endif

#ifdef MONGOC_ENABLE_CRYPTO
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO);
#endif

#ifdef MONGOC_ENABLE_CRYPTO_SYSTEM_PROFILE
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_CRYPTO_SYSTEM_PROFILE);
#endif

#ifdef MONGOC_ENABLE_SASL
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SASL);
#endif

#ifdef MONGOC_HAVE_SASL_CLIENT_DONE
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_HAVE_SASL_CLIENT_DONE);
#endif

#ifdef MONGOC_EXPERIMENTAL_FEATURES
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_EXPERIMENTAL_FEATURES);
#endif

#ifdef MONGOC_ENABLE_SASL_CYRUS
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SASL_CYRUS);
#endif

#ifdef MONGOC_ENABLE_SASL_SSPI
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SASL_SSPI);
#endif

#ifdef MONGOC_HAVE_SOCKLEN
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_HAVE_SOCKLEN);
#endif

#ifdef MONGOC_ENABLE_COMPRESSION
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_COMPRESSION);
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_COMPRESSION_SNAPPY);
#endif

#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_COMPRESSION_ZLIB);
#endif

#ifdef MONGOC_HAVE_RES_NSEARCH
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_NSEARCH);
#endif

#ifdef MONGOC_HAVE_RES_NDESTROY
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_NDESTROY);
#endif

#ifdef MONGOC_HAVE_RES_NCLOSE
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_NCLOSE);
#endif

#ifdef MONGOC_HAVE_RES_SEARCH
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_RES_SEARCH);
#endif

#ifdef MONGOC_HAVE_DNSAPI
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_DNSAPI);
#endif

#ifdef MONGOC_HAVE_RDTSCP
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_RDTSCP);
#endif

#ifdef MONGOC_HAVE_SCHED_GETCPU
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_HAVE_SCHED_GETCPU);
#endif

#ifdef MONGOC_ENABLE_SHM_COUNTERS
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SHM_COUNTERS);
#endif

   mlib_diagnostic_push();
   mlib_disable_constant_conditional_expression_warnings();
   if (MONGOC_TRACE_ENABLED) {
      _set_bit(bf, byte_count, MONGOC_MD_FLAG_TRACE);
   }
   mlib_diagnostic_pop();

#ifdef MONGOC_ENABLE_CLIENT_SIDE_ENCRYPTION
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_CLIENT_SIDE_ENCRYPTION);
#endif

#ifdef MONGOC_ENABLE_MONGODB_AWS_AUTH
   _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_MONGODB_AWS_AUTH);
#endif

   mlib_diagnostic_push();
   mlib_disable_constant_conditional_expression_warnings();
   if (MONGOC_SRV_ENABLED) {
      _set_bit(bf, byte_count, MONGOC_MD_FLAG_ENABLE_SRV);
   }
   mlib_diagnostic_pop();

   mcommon_string_append_t append;
   mcommon_string_set_append(mcommon_string_new_with_capacity("0x", 2, 2 + byte_count * 2), &append);
   for (uint32_t i = 0u; i < byte_count; i++) {
      mcommon_string_append_printf(&append, "%02x", bf[i]);
   }
   bson_free(bf);
   return mcommon_string_from_append_destroy_with_steal(&append);
}

static char *
_get_os_type(void)
{
#ifdef MONGOC_OS_TYPE
   return bson_strndup(MONGOC_OS_TYPE, HANDSHAKE_OS_TYPE_MAX);
#else
   return bson_strndup("unknown", HANDSHAKE_OS_TYPE_MAX);
#endif
}

static char *
_get_os_architecture(void)
{
   const char *ret = NULL;

#ifdef _WIN32
   SYSTEM_INFO system_info;
   DWORD arch;
   GetSystemInfo(&system_info);

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

   if (uname(&system_info) >= 0) {
      ret = system_info.machine;
   }

#endif

   if (ret) {
      return bson_strndup(ret, HANDSHAKE_OS_ARCHITECTURE_MAX);
   }

   return NULL;
}

#ifndef MONGOC_OS_IS_LINUX
static char *
_get_os_name(void)
{
#ifdef MONGOC_OS_NAME
   return bson_strndup(MONGOC_OS_NAME, HANDSHAKE_OS_NAME_MAX);
#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname(&system_info) >= 0) {
      return bson_strndup(system_info.sysname, HANDSHAKE_OS_NAME_MAX);
   }

   return NULL;
#else
   return NULL;
#endif
}

#ifdef _WIN32
static bool
_win32_get_os_version(RTL_OSVERSIONINFOEXW *osvi)
{
   // not using GetVersionEx here as this function has special behavior:
   // it returns the version the application has been "manifested" (targeted) for, or Windows 8 if not manifested
   // instead of the version it's actually running on
   // see: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa

   // instead, we are using RtlGetVersion
   // this function is not publicly documented and no declaration is available in a header file as a result
   // instead, it must be looked up manually in ntdll.dll

   // ntdll.dll is always loaded into every process - this call is expected to always succeed
   const HMODULE ntdll_handle = GetModuleHandleW(L"ntdll.dll");
   if (!ntdll_handle) {
      return false;
   }

   NTSTATUS(NTAPI *const rtlgetversion_ptr)(RTL_OSVERSIONINFOEXW *) =
      (NTSTATUS(NTAPI *)(RTL_OSVERSIONINFOEXW *))GetProcAddress(ntdll_handle, "RtlGetVersion");
   if (!rtlgetversion_ptr) {
      return false;
   }

   ZeroMemory(osvi, sizeof(*osvi));
   osvi->dwOSVersionInfoSize = sizeof(*osvi);

   const NTSTATUS status = rtlgetversion_ptr(osvi);
   if (status != 0) {
      MONGOC_WARNING("RtlGetVersion() returned NTSTATUS: %08lX", (unsigned long)status);
      return false;
   }

   return true;
}
#endif


static char *
_get_os_version(void)
{
   char *ret = bson_malloc(HANDSHAKE_OS_VERSION_MAX);
   bool found = false;

#ifdef _WIN32

   RTL_OSVERSIONINFOEXW osvi;
   if (_win32_get_os_version(&osvi)) {
      // Truncation is OK.
      int req = bson_snprintf(
         ret, HANDSHAKE_OS_VERSION_MAX, "%lu.%lu (%lu)", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
      BSON_ASSERT(req > 0);
      found = true;
   }

#elif defined(_POSIX_VERSION)
   struct utsname system_info;

   if (uname(&system_info) >= 0) {
      bson_strncpy(ret, system_info.release, HANDSHAKE_OS_VERSION_MAX);
      found = true;
   } else {
      MONGOC_WARNING("Error with uname(): %d", errno);
   }

#endif

   if (!found) {
      bson_free(ret);
      ret = NULL;
   }

   return ret;
}
#endif

static void
_get_system_info(void)
{
   gMongocHandshake.os_type = _get_os_type();

#ifdef MONGOC_OS_IS_LINUX
   _mongoc_linux_distro_scanner_get_distro(&gMongocHandshake.os_name, &gMongocHandshake.os_version);
#else
   gMongocHandshake.os_name = _get_os_name();
   gMongocHandshake.os_version = _get_os_version();
#endif

   gMongocHandshake.os_architecture = _get_os_architecture();
}

static void
_get_driver_info(void)
{
   gMongocHandshake.driver_name = bson_strndup("mongoc", HANDSHAKE_DRIVER_NAME_MAX);
   gMongocHandshake.driver_version = bson_strndup(MONGOC_VERSION_S, HANDSHAKE_DRIVER_VERSION_MAX);

   // For backward compatibility with how the platform string is handled (appending `compiler_info` and `flags`), the
   // platform metadata associated with the "mongoc" client field is always the LAST element in the "platform" metadata
   // field, not the first:
   //
   //  - "name":     [            "mongoc",     "Library Platform",  "Framework Platform"]
   //  - "version":  [  "<mongoc version>",    "<library version>", "<framework version>"]
   //  - "platform": ["<library platform>", "<framework platform>",   "<mongoc platform>"] (!!)
   //
   // Due to handshake length limits and truncation, the "<mongoc platform>" value may be excluded completely:
   //
   //  - "name":     [            "mongoc",     "Library Platform"]
   //  - "version":  [  "<mongoc version>",    "<library version>"]
   //  - "platform": ["<library platform>"                        ] (!!)
}

static void
_set_platform_string(void)
{
   gMongocHandshake.platform = bson_strdup("");
}

static void
_get_container_info(void)
{
   char *kubernetes_env = _mongoc_getenv("KUBERNETES_SERVICE_HOST");
   gMongocHandshake.kubernetes = kubernetes_env;

   gMongocHandshake.docker = false;
#ifdef _WIN32
   gMongocHandshake.docker = (_access_s("C:\\.dockerenv", 0) == 0);
#else
   gMongocHandshake.docker = (access("/.dockerenv", F_OK) == 0);
#endif

   bson_free(kubernetes_env);
}

static void
_get_env_info(void)
{
   char *aws_env = _mongoc_getenv("AWS_EXECUTION_ENV");
   char *aws_lambda = _mongoc_getenv("AWS_LAMBDA_RUNTIME_API");
   char *vercel_env = _mongoc_getenv("VERCEL");
   char *azure_env = _mongoc_getenv("FUNCTIONS_WORKER_RUNTIME");
   char *gcp_env = _mongoc_getenv("K_SERVICE");
   char *memory_str = NULL;
   char *timeout_str = NULL;
   char *region_str = NULL;

   bool is_aws =
      (aws_env && strlen(aws_env) && (aws_env == strstr(aws_env, "AWS_Lambda_"))) || (aws_lambda && strlen(aws_lambda));
   bool is_vercel = vercel_env && strlen(vercel_env);
   bool is_azure = azure_env && strlen(azure_env);
   bool is_gcp = gcp_env && strlen(gcp_env);

   gMongocHandshake.env = MONGOC_HANDSHAKE_ENV_NONE;
   gMongocHandshake.env_region = NULL;
   gMongocHandshake.env_memory_mb.set = false;
   gMongocHandshake.env_timeout_sec.set = false;

   if ((is_aws || is_vercel) + is_azure + is_gcp != 1) {
      goto cleanup;
   }

   if (is_aws && !is_vercel) {
      gMongocHandshake.env = MONGOC_HANDSHAKE_ENV_AWS;
      region_str = _mongoc_getenv("AWS_REGION");
      memory_str = _mongoc_getenv("AWS_LAMBDA_FUNCTION_MEMORY_SIZE");
   } else if (is_vercel) {
      gMongocHandshake.env = MONGOC_HANDSHAKE_ENV_VERCEL;
      region_str = _mongoc_getenv("VERCEL_REGION");
   } else if (is_gcp) {
      gMongocHandshake.env = MONGOC_HANDSHAKE_ENV_GCP;
      region_str = _mongoc_getenv("FUNCTION_REGION");
      memory_str = _mongoc_getenv("FUNCTION_MEMORY_MB");
      timeout_str = _mongoc_getenv("FUNCTION_TIMEOUT_SEC");
   } else if (is_azure) {
      gMongocHandshake.env = MONGOC_HANDSHAKE_ENV_AZURE;
   }

   if (memory_str) {
      char *endptr;
      int64_t env_memory_mb = bson_ascii_strtoll(memory_str, &endptr, 10);
      bool parse_ok = endptr == memory_str + (strlen(memory_str));
      bool in_range = mlib_in_range(int32_t, env_memory_mb);

      if (parse_ok && in_range) {
         gMongocHandshake.env_memory_mb.set = true;
         gMongocHandshake.env_memory_mb.value = (int32_t)env_memory_mb;
      }
   }
   if (timeout_str) {
      char *endptr;
      int64_t env_timeout_sec = bson_ascii_strtoll(timeout_str, &endptr, 10);
      bool parse_ok = endptr == timeout_str + (strlen(timeout_str));
      bool in_range = mlib_in_range(int32_t, env_timeout_sec);

      if (parse_ok && in_range) {
         gMongocHandshake.env_timeout_sec.set = true;
         gMongocHandshake.env_timeout_sec.value = (int32_t)env_timeout_sec;
      }
   }
   if (region_str && strlen(region_str)) {
      gMongocHandshake.env_region = bson_strdup(region_str);
   }

cleanup:
   bson_free(aws_env);
   bson_free(aws_lambda);
   bson_free(vercel_env);
   bson_free(azure_env);
   bson_free(gcp_env);
   bson_free(memory_str);
   bson_free(timeout_str);
   bson_free(region_str);
}

static void
_set_compiler_info(void)
{
   mcommon_string_append_t append;
   mcommon_string_new_as_append(&append);

   char *config_str = _mongoc_handshake_get_config_hex_string();
   mcommon_string_append_printf(&append, "cfg=%s", config_str);
   bson_free(config_str);

#ifdef _POSIX_VERSION
   mcommon_string_append_printf(&append, " posix=%ld", _POSIX_VERSION);
#endif

#ifdef __STDC_VERSION__
   mcommon_string_append_printf(&append, " stdc=%ld", __STDC_VERSION__);
#endif

   mcommon_string_append_printf(&append, " CC=%s", MONGOC_COMPILER);

#ifdef MONGOC_COMPILER_VERSION
   mcommon_string_append_printf(&append, " %s", MONGOC_COMPILER_VERSION);
#endif

#ifdef MONGOC_CXX_COMPILER_ID
   if (0 != strlen(MONGOC_CXX_COMPILER_ID)) {
      mcommon_string_append(&append, " CXX=" MONGOC_CXX_COMPILER_ID);
   }
#endif

#ifdef MONGOC_CXX_COMPILER_VERSION
   if (0 != strlen(MONGOC_CXX_COMPILER_VERSION)) {
      mcommon_string_append(&append, " " MONGOC_CXX_COMPILER_VERSION);
   }
#endif

   gMongocHandshake.compiler_info = mcommon_string_from_append_destroy_with_steal(&append);
}

static void
_set_flags(void)
{
   mcommon_string_append_t append;
   mcommon_string_new_as_append(&append);

   if (strlen(MONGOC_EVALUATE_STR(MONGOC_USER_SET_CFLAGS)) > 0) {
      mcommon_string_append_printf(&append, " CFLAGS=%s", MONGOC_EVALUATE_STR(MONGOC_USER_SET_CFLAGS));
   }

   if (strlen(MONGOC_EVALUATE_STR(MONGOC_USER_SET_LDFLAGS)) > 0) {
      mcommon_string_append_printf(&append, " LDFLAGS=%s", MONGOC_EVALUATE_STR(MONGOC_USER_SET_LDFLAGS));
   }

   gMongocHandshake.flags = mcommon_string_from_append_destroy_with_steal(&append);
}

void
_mongoc_handshake_init(void)
{
   _get_system_info();
   _get_driver_info();
   _set_platform_string();
   _get_env_info();
   _get_container_info();
   _set_compiler_info();
   _set_flags();

   gMongocHandshake.frozen = false;
   bson_mutex_init(&gHandshakeLock);
}

void
_mongoc_handshake_cleanup(void)
{
   bson_free(gMongocHandshake.os_type);
   bson_free(gMongocHandshake.os_name);
   bson_free(gMongocHandshake.os_version);
   bson_free(gMongocHandshake.os_architecture);
   bson_free(gMongocHandshake.driver_name);
   bson_free(gMongocHandshake.driver_version);
   bson_free(gMongocHandshake.platform);
   bson_free(gMongocHandshake.compiler_info);
   bson_free(gMongocHandshake.flags);
   bson_free(gMongocHandshake.env_region);
   gMongocHandshake = (mongoc_handshake_t){0};

   bson_mutex_destroy(&gHandshakeLock);
}

static void
_append_platform_field(bson_t *doc, const mongoc_handshake_t *handshake, bool truncate)
{
   const char *const platform = handshake->platform;
   const char *const compiler_info = handshake->compiler_info;
   const char *const flags = handshake->flags;

   const bool platform_is_empty = platform == NULL || platform[0] == '\0';
   const bool compiler_info_is_empty = compiler_info == NULL || compiler_info[0] == '\0';
   const bool flags_is_empty = flags == NULL || flags[0] == '\0';

   const uint32_t overhead = (/* 1 byte for utf8 tag */
                              1 +
                              /* key size */
                              (int)strlen(HANDSHAKE_PLATFORM_FIELD) + 1 +
                              /* 4 bytes for length of string */
                              4 +
                              /* NUL terminator */
                              1);

   if (truncate && doc->len >= HANDSHAKE_MAX_SIZE - overhead) {
      return;
   }

   mcommon_string_append_t combined_platform;
   mcommon_string_set_append_with_limit(mcommon_string_new_with_capacity("", 0, HANDSHAKE_MAX_SIZE - overhead),
                                        &combined_platform,
                                        truncate ? HANDSHAKE_MAX_SIZE - overhead - doc->len : UINT32_MAX - 1u);

   if (!platform_is_empty) {
      mcommon_string_append(&combined_platform, platform);
   }

   // `platform` must be delimited with " / " from any subsequent field values.
   if (!compiler_info_is_empty && !flags_is_empty) {
      bool success = false;

      // First try to append both `compiler_info` and `flags`.
      {
         char *both = bson_strdup_printf("%s%s", compiler_info, flags);

         if (platform_is_empty) {
            success = mcommon_string_append_all_or_none(&combined_platform, both);
         } else {
            char *delimited = bson_strdup_printf(" / %s", both);
            success = mcommon_string_append_all_or_none(&combined_platform, delimited);
            bson_free(delimited);
         }

         bson_free(both);
      }

      // Fallback to only `compiler_info`.
      if (!success) {
         combined_platform._max_len_exceeded = false;

         if (platform_is_empty) {
            mcommon_string_append_all_or_none(&combined_platform, compiler_info);
         } else {
            char *delimited = bson_strdup_printf(" / %s", compiler_info);
            mcommon_string_append_all_or_none(&combined_platform, delimited);
            bson_free(delimited);
         }
      }
   }

   // Only `compiler_info` or `flags` is present.
   else if (!compiler_info_is_empty) {
      if (platform_is_empty) {
         mcommon_string_append_all_or_none(&combined_platform, compiler_info);
      } else {
         char *delimited = bson_strdup_printf(" / %s", compiler_info);
         mcommon_string_append_all_or_none(&combined_platform, delimited);
         bson_free(delimited);
      }
   } else if (!flags_is_empty) {
      if (platform_is_empty) {
         mcommon_string_append_all_or_none(&combined_platform, flags);
      } else {
         char *delimited = bson_strdup_printf(" / %s", flags);
         mcommon_string_append_all_or_none(&combined_platform, delimited);
         bson_free(delimited);
      }
   }

   bson_append_utf8(doc,
                    HANDSHAKE_PLATFORM_FIELD,
                    strlen(HANDSHAKE_PLATFORM_FIELD),
                    mcommon_str_from_append(&combined_platform),
                    mcommon_strlen_from_append(&combined_platform));

   mcommon_string_from_append_destroy(&combined_platform);
}

static bool
_get_subdoc_static(bson_t *doc, char *subdoc_name, bson_t *out)
{
   bson_iter_t iter;
   if (bson_iter_init_find(&iter, doc, subdoc_name) && BSON_ITER_HOLDS_DOCUMENT(&iter)) {
      uint32_t len;
      const uint8_t *data;
      bson_iter_document(&iter, &len, &data);
      BSON_ASSERT(bson_init_static(out, data, len));

      return true;
   }
   return false;
}

static bool
_truncate_handshake(bson_t **doc, const mongoc_handshake_t *md)
{
   if ((*doc)->len > HANDSHAKE_MAX_SIZE) {
      bson_t env_doc;
      if (_get_subdoc_static(*doc, "env", &env_doc)) {
         bson_t *new_env = bson_new();
         bson_copy_to_including_noinit(&env_doc, new_env, "name", NULL);

         bson_t *new_doc = bson_new();
         bson_copy_to_excluding_noinit(*doc, new_doc, "env", NULL);

         bson_append_document(new_doc, "env", -1, new_env);
         bson_destroy(new_env);
         bson_destroy(*doc);
         *doc = new_doc;
      }
   }

   if ((*doc)->len > HANDSHAKE_MAX_SIZE) {
      bson_t os_doc;
      if (_get_subdoc_static(*doc, "os", &os_doc)) {
         bson_t *new_os = bson_new();
         bson_copy_to_including_noinit(&os_doc, new_os, "type", NULL);

         bson_t *new_doc = bson_new();
         bson_copy_to_excluding_noinit(*doc, new_doc, "os", NULL);

         bson_append_document(new_doc, "os", -1, new_os);
         bson_destroy(new_os);
         bson_destroy(*doc);
         *doc = new_doc;
      }
   }

   if ((*doc)->len > HANDSHAKE_MAX_SIZE) {
      bson_t *new_doc = bson_new();
      bson_copy_to_excluding_noinit(*doc, new_doc, "env", NULL);
      bson_destroy(*doc);
      *doc = new_doc;
   }

   if ((*doc)->len > HANDSHAKE_MAX_SIZE && md->platform) {
      bson_t *new_doc = bson_new();
      bson_copy_to_excluding_noinit(*doc, new_doc, "platform", NULL);
      _append_platform_field(new_doc, md, true);
      bson_destroy(*doc);
      *doc = new_doc;
   }

   return (*doc)->len <= HANDSHAKE_MAX_SIZE;
}

/*
 * Return true if we build the document, and it's not too big
 * false if there's no way to prevent the doc from being too big. In this
 * case, the caller shouldn't include it with hello
 */
bson_t *
_mongoc_handshake_build_doc_with_application(const mongoc_handshake_t *md, const char *appname)
{
   char *env_name = NULL;
   switch (md->env) {
   case MONGOC_HANDSHAKE_ENV_AWS:
      env_name = "aws.lambda";
      break;
   case MONGOC_HANDSHAKE_ENV_GCP:
      env_name = "gcp.func";
      break;
   case MONGOC_HANDSHAKE_ENV_AZURE:
      env_name = "azure.func";
      break;
   case MONGOC_HANDSHAKE_ENV_VERCEL:
      env_name = "vercel";
      break;
   case MONGOC_HANDSHAKE_ENV_NONE:
      env_name = NULL;
      break;
   default:
      break;
   }

   bson_t *doc = bson_new();
   // Optimistically include all handshake data
   bsonBuildAppend(
      *doc,
      if (appname, then(kv("application", doc(kv("name", cstr(appname)))))),
      kv("driver", doc(kv("name", cstr(md->driver_name)), kv("version", cstr(md->driver_version)))),
      kv("os",
         doc(kv("type", cstr(md->os_type)),
             if (md->os_name, then(kv("name", cstr(md->os_name)))),
             if (md->os_version, then(kv("version", cstr(md->os_version)))),
             if (md->os_architecture, then(kv("architecture", cstr(md->os_architecture)))))),
      if (env_name,
          then(kv("env",
                  doc(kv("name", cstr(env_name)),
                      if (md->env_timeout_sec.set, then(kv("timeout_sec", int32(md->env_timeout_sec.value)))),
                      if (md->env_memory_mb.set, then(kv("memory_mb", int32(md->env_memory_mb.value)))),
                      if (md->env_region, then(kv("region", cstr(md->env_region)))))))),
      if (md->kubernetes || md->docker,
          then(kv("container",
                  doc(if (md->docker, then(kv("runtime", cstr("docker")))),
                      if (md->kubernetes, then(kv("orchestrator", cstr("kubernetes")))))))));

   if (md->platform) {
      _append_platform_field(doc, md, false);
   }

   if (_truncate_handshake(&doc, md)) {
      return doc;
   } else {
      bson_destroy(doc);
      return NULL;
   }
}

void
_mongoc_handshake_freeze(void)
{
   // Ensure all writes to gMongocHandshake are ordered BEFORE this store.
   mcommon_atomic_int8_exchange(&gMongocHandshake.frozen, true, mcommon_memory_order_release);
}

/*
 * free (*s) and make *s point to *s concated with suffix.
 * If *s is NULL it's treated like it's an empty string.
 */
static void
_append_and_truncate(char **s, const char *suffix, size_t max_len)
{
   BSON_ASSERT_PARAM(s);
   BSON_ASSERT_PARAM(suffix);

   // `max_len` is at most `HANDSHAKE_MAX_SIZE`, which fits well within the range of `int`.
   BSON_ASSERT(mlib_in_range(int, max_len));

   char *old_str = *s;
   const char *const delim = " / ";
   const size_t delim_len = strlen(delim);

   // For backward compatibility, allow `suffix` to contain the " / " delimiter at the end of its value, but strip it so
   // that `_append_platform_field()` can re-append it only when it is necessary.
   size_t suffix_len = strlen(suffix);
   if (suffix_len >= delim_len && strncmp(suffix + (suffix_len - delim_len), delim, delim_len) == 0) {
      suffix_len -= delim_len;
   }

   const char *const prefix = old_str ? old_str : "";

   const size_t required_space = strlen(prefix) + delim_len;

   if (max_len <= required_space) {
      /* the old string already takes the whole allotted space */
      return;
   }

   // `INT_MAX > HANDSHAKE_MAX_SIZE >= max_len > required_space`, therefore `space_for_suffix` is always within range.
   const int space_for_suffix = (int)(max_len - required_space);

   // Strip the trailing " / " delimiter and/or truncate to fit within `max_len`.
   const int truncated_len = mlib_cmp(suffix_len, <, space_for_suffix) ? (int)suffix_len : space_for_suffix;

   *s = bson_strdup_printf("%s / %.*s", prefix, truncated_len, suffix);
   BSON_ASSERT(strlen(*s) <= max_len);

   bson_free(old_str);
}


/*
 * Set some values in our global handshake struct. These values will be sent
 * to the server as part of the initial connection handshake (hello).
 * If this function is called more than once, or after we've connected to a
 * mongod, then it will do nothing and return false. It will return true if
 * it successfully sets the values.
 *
 * All arguments are optional.
 */
bool
mongoc_handshake_data_append(const char *driver_name, const char *driver_version, const char *platform)
{
   int platform_space;

   if (_mongoc_handshake_is_frozen()) {
      return false;
   }

   bson_mutex_lock(&gHandshakeLock);

   // TOCTOU: another thread may have frozen the handshake before this lock was acquired.
   if (_mongoc_handshake_is_frozen()) {
      bson_mutex_unlock(&gHandshakeLock);
      return false;
   }

   BSON_ASSERT(gMongocHandshake.platform);

   /* allow practically any size for "platform", we'll trim it down in
    * _mongoc_handshake_build_doc_with_application */
   platform_space = HANDSHAKE_MAX_SIZE - (int)strlen(gMongocHandshake.platform);

   if (platform) {
      /* we check for an empty string as a special case to avoid an
       * unnecessary delimiter being added in front of the string by
       * _append_and_truncate */
      if (gMongocHandshake.platform[0] == '\0') {
         bson_free(gMongocHandshake.platform);
         gMongocHandshake.platform = bson_strdup_printf("%.*s", platform_space, platform);
      } else {
         _append_and_truncate(&gMongocHandshake.platform, platform, HANDSHAKE_MAX_SIZE);
      }
   }

   if (driver_name) {
      _append_and_truncate(&gMongocHandshake.driver_name, driver_name, HANDSHAKE_DRIVER_NAME_MAX);
   }

   if (driver_version) {
      _append_and_truncate(&gMongocHandshake.driver_version, driver_version, HANDSHAKE_DRIVER_VERSION_MAX);
   }

   _mongoc_handshake_freeze();
   bson_mutex_unlock(&gHandshakeLock);

   return true;
}

const mongoc_handshake_t *
_mongoc_handshake_get(void)
{
   BSON_ASSERT(_mongoc_handshake_is_frozen());
   return &gMongocHandshake;
}

mongoc_handshake_t *
_mongoc_handshake_get_unfrozen(void)
{
   return &gMongocHandshake;
}

bool
_mongoc_handshake_is_frozen(void)
{
   return mcommon_atomic_int8_fetch(&gMongocHandshake.frozen, mcommon_memory_order_acquire) != 0;
}

bool
_mongoc_handshake_appname_is_valid(const char *appname)
{
   return strlen(appname) <= MONGOC_HANDSHAKE_APPNAME_MAX;
}

void
_mongoc_handshake_append_sasl_supported_mechs(const mongoc_uri_t *uri, bson_t *cmd)
{
   const char *username;
   char *db_user;
   username = mongoc_uri_get_username(uri);
   db_user = bson_strdup_printf("%s.%s", mongoc_uri_get_auth_source(uri), username);
   bson_append_utf8(cmd, "saslSupportedMechs", 18, db_user, -1);
   bson_free(db_user);
}

void
_mongoc_handshake_parse_sasl_supported_mechs(const bson_t *hello,
                                             mongoc_handshake_sasl_supported_mechs_t *sasl_supported_mechs)
{
   memset(sasl_supported_mechs, 0, sizeof(*sasl_supported_mechs));
   bsonParse(*hello,
             find(keyWithType("saslSupportedMechs", array),
                  visitEach(case (when(strEqual("SCRAM-SHA-256"), do(sasl_supported_mechs->scram_sha_256 = true)),
                                  when(strEqual("SCRAM-SHA-1"), do(sasl_supported_mechs->scram_sha_1 = true))))));
}
