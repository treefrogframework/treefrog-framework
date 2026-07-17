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

#include <common-atomic-private.h>
#include <common-string-private.h>
#include <mongoc/mongoc-async-cmd-private.h>
#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-cluster-oidc-private.h>
#include <mongoc/mongoc-cluster-private.h>
#include <mongoc/mongoc-counters-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-handshake-private.h>
#include <mongoc/mongoc-host-list-private.h>
#include <mongoc/mongoc-stream-private.h>
#include <mongoc/mongoc-structured-log-private.h>
#include <mongoc/mongoc-topology-private.h>
#include <mongoc/mongoc-topology-scanner-private.h>
#include <mongoc/mongoc-trace-private.h>
#include <mongoc/mongoc-uri-private.h>
#include <mongoc/mongoc-util-private.h>

#include <mongoc/mongoc-config.h>
#include <mongoc/mongoc-handshake.h>
#include <mongoc/mongoc-stream-socket.h>
#include <mongoc/utlist.h>

#include <bson/bson.h>

#include <mlib/cmp.h>
#include <mlib/duration.h>
#include <mlib/time_point.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef MONGOC_ENABLE_SSL
#include <mongoc/mongoc-stream-tls.h>
#endif

#if defined(MONGOC_ENABLE_SSL_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <mongoc/mongoc-stream-tls-private.h>

#include <openssl/ssl.h>
#endif

#if defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
#include <mongoc/mongoc-stream-tls-private.h>
#include <mongoc/mongoc-stream-tls-secure-channel-private.h>
#endif

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "topology_scanner"

#define DNS_CACHE_TIMEOUT_MS 10 * 60 * 1000
#define HAPPY_EYEBALLS_DELAY mlib_duration(250, ms)

/* forward declarations */
static void
_async_connected(mongoc_async_cmd_t *acmd);

static void
_async_success(mongoc_async_cmd_t *acmd, const bson_t *hello_response, mlib_duration elapsed);

static void
_async_error_or_timeout(mongoc_async_cmd_t *acmd, mlib_duration elapsed, const char *default_err_msg);

static void
_async_handler(mongoc_async_cmd_t *acmd,
               mongoc_async_cmd_result_t async_status,
               const bson_t *hello_response,
               mlib_duration duration);

static void
_mongoc_topology_scanner_monitor_heartbeat_started(const mongoc_topology_scanner_t *ts, const mongoc_host_list_t *host);

static void
_mongoc_topology_scanner_monitor_heartbeat_succeeded(const mongoc_topology_scanner_t *ts,
                                                     const mongoc_host_list_t *host,
                                                     const bson_t *reply,
                                                     mlib_duration elapsed);

static void
_mongoc_topology_scanner_monitor_heartbeat_failed(const mongoc_topology_scanner_t *ts,
                                                  const mongoc_host_list_t *host,
                                                  const bson_error_t *error,
                                                  mlib_duration elapsed);


/* reset "retired" nodes that failed or were removed in the previous scan */
static void
_delete_retired_nodes(mongoc_topology_scanner_t *ts);

// Get the scanner node associated with an async command
static mongoc_topology_scanner_node_t *
_scanner_node_of(mongoc_async_cmd_t const *a)
{
   BSON_ASSERT_PARAM(a);
   return _acmd_userdata(mongoc_topology_scanner_node_t, a);
}

// Test whether two async commands are associated with the same topology scanner node,
// and aren't the same command object
static bool
_is_sibling_command(mongoc_async_cmd_t const *l, mongoc_async_cmd_t const *r)
{
   BSON_ASSERT_PARAM(l);
   BSON_ASSERT_PARAM(r);
   return l != r && _scanner_node_of(l) == _scanner_node_of(r);
}

/* cancel any pending async commands for a specific node excluding acmd.
 * If acmd is NULL, cancel all async commands on the node. */
static void
_cancel_commands_excluding(mongoc_topology_scanner_node_t *node, mongoc_async_cmd_t *acmd);

/* return the number of pending async commands for a node. */
static int
_count_acmds(mongoc_topology_scanner_node_t *node);


/**
 * @brief Cause all sibling commands to initiate sooner
 */
static void
_jumpstart_other_acmds(mongoc_async_cmd_t const *const self)
{
   BSON_ASSERT_PARAM(self);
   mongoc_async_cmd_t *other;
   DL_FOREACH(self->async->cmds, other)
   {
      // Only consider commands on the same node
      if (_is_sibling_command(self, other)) {
         // Decrease the delay by the happy eyeballs duration.
         _acmd_adjust_connect_delay(other, mlib_duration(HAPPY_EYEBALLS_DELAY, mul, -1));
      }
   }
}

static void
_add_hello(mongoc_topology_scanner_t *ts)
{
   BSON_APPEND_INT32(&ts->hello_cmd, "hello", 1);
   BSON_APPEND_BOOL(&ts->hello_cmd, "helloOk", true);

   BSON_APPEND_INT32(&ts->legacy_hello_cmd, HANDSHAKE_CMD_LEGACY_HELLO, 1);
   BSON_APPEND_BOOL(&ts->legacy_hello_cmd, "helloOk", true);

   /* Append appropriate server API metadata (such as "serverApi") if selected:
    */
   if (mongoc_topology_scanner_uses_server_api(ts)) {
      _mongoc_cmd_append_server_api(&ts->hello_cmd, ts->api);
   }
}

static void
_init_hello(mongoc_topology_scanner_t *ts)
{
   bson_init(&ts->hello_cmd);
   bson_init(&ts->legacy_hello_cmd);
   ts->handshake_cmd = NULL;

   _add_hello(ts);
}

static void
_reset_hello(mongoc_topology_scanner_t *ts)
{
   bson_t *prev_cmd;
   bson_reinit(&ts->hello_cmd);
   bson_reinit(&ts->legacy_hello_cmd);

   bson_mutex_lock(&ts->handshake_cmd_mtx);
   prev_cmd = ts->handshake_cmd;
   ts->handshake_cmd = NULL;
   ts->handshake_state = HANDSHAKE_CMD_UNINITIALIZED;
   bson_mutex_unlock(&ts->handshake_cmd_mtx);
   bson_destroy(prev_cmd);

   _add_hello(ts);
}

void
_mongoc_topology_scanner_node_parse_sasl_supported_mechs(const bson_t *hello, mongoc_topology_scanner_node_t *node)
{
   _mongoc_handshake_parse_sasl_supported_mechs(hello, &node->sasl_supported_mechs);
   node->negotiated_sasl_supported_mechs = true;
}

const char *
_mongoc_topology_scanner_get_speculative_auth_mechanism(const mongoc_uri_t *uri)
{
   const char *mechanism = mongoc_uri_get_auth_mechanism(uri);
   bool requires_auth = mechanism || mongoc_uri_get_username(uri);

   if (!requires_auth) {
      return NULL;
   }

   if (!mechanism) {
      return "SCRAM-SHA-256";
   }

   return mechanism;
}

void
_mongoc_topology_scanner_add_speculative_authentication(
   bson_t *cmd, const mongoc_uri_t *uri, char *oidc_access_token, uint32_t server_id, mongoc_scram_t *scram /* OUT */)
{
   BSON_ASSERT_PARAM(cmd);
   BSON_ASSERT_PARAM(uri);
   BSON_OPTIONAL_PARAM(oidc_access_token);
   BSON_ASSERT_PARAM(scram);

   bson_t auth_cmd;
   bson_error_t error;
   bool has_auth = false;
   const char *mechanism = _mongoc_topology_scanner_get_speculative_auth_mechanism(uri);

   if (!mechanism) {
      return;
   }

   if (strcasecmp(mechanism, "MONGODB-X509") == 0) {
      /* Ignore errors while building authentication document: we proceed with
       * the handshake as usual and let the subsequent authenticate command
       * fail. */
      if (_mongoc_cluster_get_auth_cmd_x509(uri, &auth_cmd, &error)) {
         has_auth = true;
         BSON_APPEND_UTF8(&auth_cmd, "db", "$external");
      }
   }

#ifdef MONGOC_ENABLE_CRYPTO
   if (strcasecmp(mechanism, "SCRAM-SHA-1") == 0 || strcasecmp(mechanism, "SCRAM-SHA-256") == 0) {
      mongoc_crypto_hash_algorithm_t algo =
         strcasecmp(mechanism, "SCRAM-SHA-1") == 0 ? MONGOC_CRYPTO_ALGORITHM_SHA_1 : MONGOC_CRYPTO_ALGORITHM_SHA_256;

      _mongoc_uri_init_scram(uri, scram, algo);

      if (_mongoc_cluster_get_auth_cmd_scram(algo, scram, &auth_cmd, &error)) {
         const char *auth_source;

         if (!(auth_source = mongoc_uri_get_auth_source(uri)) || (*auth_source == '\0')) {
            auth_source = "admin";
         }

         has_auth = true;
         BSON_APPEND_UTF8(&auth_cmd, "db", auth_source);
      }
   }
#endif

   if (strcasecmp(mechanism, "MONGODB-OIDC") == 0 && oidc_access_token) {
      if (mongoc_oidc_append_speculative_auth(oidc_access_token, server_id, &auth_cmd, &error)) {
         has_auth = true;
      } else {
         MONGOC_ERROR("Error adding MONGODB-OIDC speculative auth: %s", error.message);
      }
   }

   if (has_auth) {
      BSON_APPEND_DOCUMENT(cmd, "speculativeAuthenticate", &auth_cmd);
      bson_destroy(&auth_cmd);
   }
}

void
_mongoc_topology_scanner_parse_speculative_authentication(const bson_t *hello, bson_t *speculative_authenticate)
{
   bson_iter_t iter;
   uint32_t data_len;
   const uint8_t *data;
   bson_t auth_response;

   BSON_ASSERT(hello);
   BSON_ASSERT(speculative_authenticate);

   if (!bson_iter_init_find(&iter, hello, "speculativeAuthenticate")) {
      return;
   }

   bson_iter_document(&iter, &data_len, &data);
   BSON_ASSERT(bson_init_static(&auth_response, data, data_len));

   bson_destroy(speculative_authenticate);
   bson_copy_to(&auth_response, speculative_authenticate);
}

static bson_t *
_build_handshake_cmd(const mongoc_handshake_t *handshake,
                     const bson_t *basis_cmd,
                     const char *appname,
                     const mongoc_uri_t *uri,
                     bool is_loadbalanced)
{
   bson_t *doc = bson_copy(basis_cmd);
   bson_iter_t iter;
   const bson_t *compressors;
   bson_array_builder_t *subarray;

   BSON_ASSERT(doc);
   bson_t *handshake_doc = _mongoc_handshake_build_doc_with_application(handshake, appname);

   if (!handshake_doc) {
      bson_destroy(doc);
      return NULL;
   }
   BSON_APPEND_BOOL(doc, HANDSHAKE_BACKPRESSURE_FIELD, true);
   bson_append_document(doc, HANDSHAKE_FIELD, -1, handshake_doc);
   bson_destroy(handshake_doc);

   BSON_APPEND_ARRAY_BUILDER_BEGIN(doc, "compression", &subarray);
   if (uri) {
      compressors = mongoc_uri_get_compressors(uri);

      if (bson_iter_init(&iter, compressors)) {
         while (bson_iter_next(&iter)) {
            bson_array_builder_append_utf8(subarray, bson_iter_key(&iter), -1);
         }
      }
   }
   bson_append_array_builder_end(doc, subarray);

   if (is_loadbalanced) {
      BSON_APPEND_BOOL(doc, "loadBalanced", true);
   }

   /* Return whether the handshake doc fit the size limit */
   return doc;
}

static bool
_should_use_op_msg(const mongoc_topology_scanner_t *ts)
{
   return mongoc_topology_scanner_uses_server_api(ts) || mongoc_topology_scanner_uses_loadbalanced(ts);
}

const bson_t *
_mongoc_topology_scanner_get_monitoring_cmd(mongoc_topology_scanner_t *ts, bool hello_ok)
{
   return hello_ok || _should_use_op_msg(ts) ? &ts->hello_cmd : &ts->legacy_hello_cmd;
}

// Initialize `ts->handshake_cmd` with `_mongoc_handshake_get()`.
//
// Precondition: ts->handshake_cmd_mtx is locked.
// Postcondition: ts->handshake_cmd_mtx is locked.
static void
_initialize_handshake_cmd(mongoc_topology_scanner_t *ts, const char *appname)
{
   BSON_ASSERT_PARAM(ts);
   BSON_OPTIONAL_PARAM(appname);

   // Double-checked lock: already initialized, no work to be done.
   if (ts->handshake_state != HANDSHAKE_CMD_UNINITIALIZED) {
      return;
   }

   // Invariant when handshake_cmd is uninitialized.
   BSON_ASSERT(ts->handshake_cmd == NULL);

   // Double-checked lock: do not hold lock while building the initial handshake command.
   bson_mutex_unlock(&ts->handshake_cmd_mtx);
   _mongoc_handshake_freeze(); // Global handshake metadata MUST NOT be modified after the first client or client pool
                               // has been initialized.
   bson_t *const new_cmd = _build_handshake_cmd(_mongoc_handshake_get(),
                                                _should_use_op_msg(ts) ? &ts->hello_cmd : &ts->legacy_hello_cmd,
                                                appname,
                                                ts->uri,
                                                ts->loadbalanced);
   bson_mutex_lock(&ts->handshake_cmd_mtx);

   // Double-checked lock: success.
   if (ts->handshake_state == HANDSHAKE_CMD_UNINITIALIZED) {
      BSON_ASSERT(ts->handshake_cmd == NULL);

      if (new_cmd) {
         ts->handshake_state = HANDSHAKE_CMD_OKAY;
         ts->handshake_cmd = new_cmd; // Ownership transfer.
      } else {
         ts->handshake_state = HANDSHAKE_CMD_TOO_BIG;
         MONGOC_WARNING("Handshake doc too big, not including in hello");
      }
   }

   // Double-checked lock: failed: another thread already initialized `ts->handshake_cmd`.
   else {
      bson_destroy(new_cmd);
   }
}

void
_mongoc_topology_scanner_dup_handshake_cmd(mongoc_topology_scanner_t *ts, bson_t *out_cmd)
{
   BSON_ASSERT_PARAM(ts);
   BSON_ASSERT_PARAM(out_cmd);

   // Only ever set at most once: when not-null, value is never subsequently changed.
   const char *const appname = mcommon_atomic_ptr_fetch((void *)&ts->appname, mcommon_memory_order_relaxed);

   bson_mutex_lock(&ts->handshake_cmd_mtx);

   // It doesn't matter who initializes the handshake command first: the result is the same.
   _initialize_handshake_cmd(ts, appname);

   if (ts->handshake_state == HANDSHAKE_CMD_OKAY) {
      bson_copy_to(ts->handshake_cmd, out_cmd);
   } else {
      // Fallback to the minimal hello command.
      bson_copy_to(_should_use_op_msg(ts) ? &ts->hello_cmd : &ts->legacy_hello_cmd, out_cmd);
   }

   bson_mutex_unlock(&ts->handshake_cmd_mtx);
}

typedef struct _metadata_field_t {
   const char *field;
   size_t length;
} _metadata_field_t;


// Note: MongoDB Handshake spec requires the delimiter be "|". However, mongoc historically uses " / " as the
// delimiter. For backward compatibility as permitted by spec (under "Deviations"), keep using " / " as the
// delimiter.
static const char *const metadata_field_delim = " / ";
static const size_t metadata_field_delim_len = 3u;


// "a / b / c" -> ["a", "b", "c"]
static mongoc_array_t
_mongoc_metadata_field_to_array_view(const char *value, size_t value_len)
{
   BSON_OPTIONAL_PARAM(value);

   mongoc_array_t ret;
   _mongoc_array_init(&ret, sizeof(_metadata_field_t));

   if (!value || value[0] == '\0') {
      return ret;
   }

   const char *iter = value;

   for (const char *next_iter = strstr(iter, metadata_field_delim); next_iter;
        next_iter = strstr(iter, metadata_field_delim)) {
      _metadata_field_t field = {.field = iter, .length = (size_t)(next_iter - iter)};
      _mongoc_array_append_val(&ret, field);
      iter = next_iter + metadata_field_delim_len;
   }

   _metadata_field_t field = {.field = iter, .length = value_len - (size_t)(iter - value)};
   _mongoc_array_append_val(&ret, field);

   return ret;
}

static bool
_mongoc_metadata_field_has_match(const mongoc_array_t *fields, const char *value, size_t value_len)
{
   BSON_ASSERT_PARAM(fields);
   BSON_OPTIONAL_PARAM(value);

   // Empty strings are equivalent to an "unset" (NULL) value, which matches anything.
   if (!value || value[0] == '\0') {
      return true;
   }

   for (size_t idx = 0u; idx < fields->len; idx++) {
      const _metadata_field_t field = _mongoc_array_index(fields, _metadata_field_t, idx);

      if (value_len == field.length && strncmp(value, field.field, field.length) == 0) {
         return true;
      }
   }

   return false;
}

static char *
_mongoc_append_metadata_value(char *current, size_t current_len, const char *value, size_t max_len)
{
   BSON_OPTIONAL_PARAM(current);
   BSON_OPTIONAL_PARAM(value);

   BSON_ASSERT(max_len <= HANDSHAKE_MAX_SIZE);
   BSON_ASSERT(current_len <= max_len);

   // Empty strings are equivalent to an "unset" (NULL) value.
   if (!value || value[0] == '\0') {
      return current;
   }

   const char *prefix = current ? current : "";
   const size_t prefix_len = current_len;

   const size_t prefix_with_delim_len = prefix_len > 0u ? prefix_len + metadata_field_delim_len : 0u;

   // Unable to append due to length limits preventing the inclusion of the delimiter.
   if (prefix_with_delim_len >= max_len) {
      return NULL;
   }

   // Always truncate the resulting value to fit within `max_len`.
   const int trunc_len = (int)(max_len - prefix_with_delim_len);

   if (prefix_with_delim_len > 0u) {
      return bson_strdup_printf("%.*s%s%.*s", (int)prefix_len, prefix, metadata_field_delim, trunc_len, value);
   } else {
      return bson_strdup_printf("%.*s", trunc_len, value);
   }
}

// `driverInfoOptions` from the MongoDB Handshake spec.
typedef struct _driver_info_options_t {
   char *name;
   char *version;
   char *platform;
} _driver_info_options_t;

// Return owning copies of metadata fields to be potentially appended-to.
// Precondition: `out` must not contain any owning pointers.
static void
_read_driver_info_options(const bson_t *handshake_cmd, _driver_info_options_t *out)
{
   BSON_ASSERT_PARAM(handshake_cmd);
   BSON_ASSERT_PARAM(out);

   // Invariant: valid handshake command document always contains the "client" field (_build_handshake_cmd).
   bson_iter_t client_iter;
   BSON_ASSERT(bson_iter_init_find(&client_iter, handshake_cmd, HANDSHAKE_FIELD));
   BSON_ASSERT(BSON_ITER_HOLDS_DOCUMENT(&client_iter));

   uint32_t client_len;
   const uint8_t *client_data;
   bson_iter_document(&client_iter, &client_len, &client_data);
   bson_t metadata;
   bson_init_static(&metadata, client_data, client_len);

   bson_iter_t iter;

   if (bson_iter_init_find(&iter, &metadata, "driver") && BSON_ITER_HOLDS_DOCUMENT(&iter)) {
      uint32_t driver_len = 0u;
      const uint8_t *driver_data = NULL;
      bson_iter_document(&iter, &driver_len, &driver_data);

      bson_t driver_doc;
      bson_init_static(&driver_doc, driver_data, driver_len);

      bson_iter_t driver_iter;
      if (bson_iter_init_find(&driver_iter, &driver_doc, "name") && BSON_ITER_HOLDS_UTF8(&driver_iter)) {
         out->name = bson_strdup(bson_iter_utf8(&driver_iter, NULL));
      }
      if (bson_iter_init_find(&driver_iter, &driver_doc, "version") && BSON_ITER_HOLDS_UTF8(&driver_iter)) {
         out->version = bson_strdup(bson_iter_utf8(&driver_iter, NULL));
      }
   }

   if (bson_iter_init_find(&iter, &metadata, "platform") && BSON_ITER_HOLDS_UTF8(&iter)) {
      out->platform = bson_strdup(bson_iter_utf8(&iter, NULL));
   }
}

// To avoid unnecessary allocations, `updated` fields are owning only when they differ from the corresponding
// `current` field. Conditionally destroy `updated` fields when not equal to `current`.
static void
_driver_info_options_destroy(_driver_info_options_t *current, _driver_info_options_t *updated)
{
   BSON_ASSERT_PARAM(current);
   BSON_ASSERT_PARAM(updated);

   if (updated->name != current->name) {
      bson_free(updated->name);
   }

   if (updated->version != current->version) {
      bson_free(updated->version);
   }

   if (updated->platform != current->platform) {
      bson_free(updated->platform);
   }

   bson_free(current->name);
   bson_free(current->version);
   bson_free(current->platform);
}

bool
_mongoc_topology_scanner_append_metadata(mongoc_topology_scanner_t *ts,
                                         const char *name,
                                         const char *version,
                                         const char *platform)
{
   BSON_ASSERT_PARAM(ts);
   BSON_ASSERT_PARAM(name);
   BSON_OPTIONAL_PARAM(version);
   BSON_OPTIONAL_PARAM(platform);

   if (name[0] == '\0') {
      MONGOC_WARNING("Metadata field 'name' must not be an empty string");
      return false;
   }

   // MongoDB Handshake Spec: All strings provided as part of the driver info MUST NOT contain the delimiter used for
   // metadata concatenation.
   {
      if (strstr(name, metadata_field_delim)) {
         MONGOC_WARNING("Metadata field 'name' must not contain the delimiter \"%s\"", metadata_field_delim);
         return false;
      }

      if (version && strstr(version, metadata_field_delim)) {
         MONGOC_WARNING("Metadata field 'version' must not contain the delimiter \"%s\"", metadata_field_delim);
         return false;
      }

      if (platform && strstr(platform, metadata_field_delim)) {
         MONGOC_WARNING("Metadata field 'platform' must not contain the delimiter \"%s\"", metadata_field_delim);
         return false;
      }
   }

   // Only ever set at most once: when not-null, value is never subsequently changed.
   const char *const appname = mcommon_atomic_ptr_fetch((void *)&ts->appname, mcommon_memory_order_relaxed);

   bool ret = false;

   {
      bson_mutex_lock(&ts->handshake_cmd_mtx);

      _driver_info_options_t current = {0};
      _driver_info_options_t updated = {0};

      // Handshake command must have been initialized at least once.
      _initialize_handshake_cmd(ts, appname);

      // Appends only make sense with a valid handshake command.
      if (ts->handshake_state != HANDSHAKE_CMD_OKAY) {
         goto fail;
      }
      BSON_ASSERT(ts->handshake_cmd != NULL);

      _read_driver_info_options(ts->handshake_cmd, &current);

      const size_t current_name_len = current.name ? strlen(current.name) : 0u;
      const size_t current_version_len = current.version ? strlen(current.version) : 0u;
      size_t current_platform_len = current.platform ? strlen(current.platform) : 0u;

      // Duplicates are identified by a set of appendable metadata fields where empty-or-null values are always
      // considered a match. All fields must contain a match for the set to be considered a duplicate.
      {
         mongoc_array_t name_fields = _mongoc_metadata_field_to_array_view(current.name, current_name_len);
         mongoc_array_t version_fields = _mongoc_metadata_field_to_array_view(current.version, current_version_len);
         mongoc_array_t platform_fields = _mongoc_metadata_field_to_array_view(current.platform, current_platform_len);

         // Exclude the conditionally-appended "mongoc" platform values (`compiler_info` and `flags`) from the
         // platform string to be appended-to so user-provided metadata is prioritized for inclusion. Conditionally
         // (re-)append them later using `_build_handshake_cmd()`.
         if (platform_fields.len > 0u) {
            // The "mongoc" platform value is always the last element, when present.
            const _metadata_field_t mongoc_platform =
               _mongoc_array_index(&platform_fields, _metadata_field_t, platform_fields.len - 1u);

            const char *const compiler_info = _mongoc_handshake_get()->compiler_info;

            // `compiler_info` is always present when `flags` is present. Otherwise, neither are present.
            // The unlikely possibility that user-provided platform metadata coincidentally matches `compiler_info`
            // is considered acceptable. Use of `strstr()` is safe given `current.platform` is null-terminated and
            // `mongoc_platform` is always the last element in the string (no intermediate delimiters).
            if (mongoc_platform.field && compiler_info &&
                strstr(mongoc_platform.field, compiler_info) == mongoc_platform.field) {
               // Exclude "<mongoc-platform>" from duplicate checks.
               platform_fields.len -= 1u;

               // "... / <mongoc-platform>" -> "... / " or "<mongoc-platform>" -> ""
               current_platform_len -= mongoc_platform.length;

               // "... / " -> "..." or "" -> ""
               current_platform_len -= platform_fields.len > 0u ? metadata_field_delim_len : 0u;
            }
         }

         const size_t name_len = strlen(name);
         const size_t version_len = version ? strlen(version) : 0u;
         const size_t platform_len = platform ? strlen(platform) : 0u;

         // Duplicates are permitted for a given metadata field when, given a single metadata append operation, the
         // resulting overall metadata contains *any* changes after accounting for deduplication of individual
         // fields:
         //  - ("a", "b", "") + ("a", "b", "") -> ("a", "b", "")
         //  - ("a", "b", "") + ("a", "c", "") -> ("a / a", "b / c", "")
         //  - ("a", "b", "") + ("a", "", "c") -> ("a / a", "b", "c")
         const bool name_match = _mongoc_metadata_field_has_match(&name_fields, name, name_len);
         const bool version_match = _mongoc_metadata_field_has_match(&version_fields, version, version_len);
         const bool platform_match = _mongoc_metadata_field_has_match(&platform_fields, platform, platform_len);

         _mongoc_array_destroy(&name_fields);
         _mongoc_array_destroy(&version_fields);
         _mongoc_array_destroy(&platform_fields);

         // All fields contained a duplicate: the resulting metadata does not change after deduplication.
         if (name_match && version_match && platform_match) {
            goto succeed; // No changes due to duplicates is still considered a success.
         }
      }

      // Append the new set of values to each metadata field.
      updated.name = _mongoc_append_metadata_value(current.name, current_name_len, name, HANDSHAKE_DRIVER_NAME_MAX);
      updated.version =
         _mongoc_append_metadata_value(current.version, current_version_len, version, HANDSHAKE_DRIVER_VERSION_MAX);
      updated.platform =
         _mongoc_append_metadata_value(current.platform, current_platform_len, platform, HANDSHAKE_MAX_SIZE);

      // When any metadata field append fails, the entire append operation is a failure.
      {
         if (!updated.name) {
            MONGOC_WARNING("Failed to append metadata due to 'name' exceeding handshake size limits");
            goto fail;
         }

         if (!updated.version) {
            MONGOC_WARNING("Failed to append metadata due to 'version' exceeding handshake size limits");
            goto fail;
         }

         if (!updated.platform) {
            MONGOC_WARNING("Failed to append metadata due to 'platform' exceeding handshake size limits");
            goto fail;
         }
      }

      // Non-owning copy and view of post-initial (frozen) handshake.
      mongoc_handshake_t md = *_mongoc_handshake_get();

      // Substitute updated metadata fields. All other fields remain unchanged from the initial handshake command.
      md.driver_name = updated.name;
      md.driver_version = updated.version;
      md.platform = updated.platform;

      bson_t *const new_cmd = _build_handshake_cmd(
         &md, _should_use_op_msg(ts) ? &ts->hello_cmd : &ts->legacy_hello_cmd, appname, ts->uri, ts->loadbalanced);

      if (new_cmd) {
         bson_destroy(ts->handshake_cmd);
         ts->handshake_cmd = new_cmd; // Ownership transfer.
         goto succeed;
      } else {
         // Leave the previous valid handshake command unchanged.
         MONGOC_WARNING("Failed to append metadata due to exceeding total handshake size limits");
         goto fail;
      }

   succeed:
      ret = true;

   fail:
      _driver_info_options_destroy(&current, &updated);

      bson_mutex_unlock(&ts->handshake_cmd_mtx);
   }

   return ret;
}

static void
_begin_hello_cmd(mongoc_topology_scanner_node_t *node,
                 mongoc_stream_t *stream,
                 bool is_setup_done,
                 struct addrinfo *dns_result,
                 mlib_duration initiate_delay,
                 bool use_handshake)
{
   mongoc_topology_scanner_t *ts = node->ts;
   bson_t cmd;

   /* If we're asked to use a specific API version, we should send our
   hello handshake via op_msg rather than the legacy op_query: */
   const int32_t cmd_opcode = _should_use_op_msg(ts) ? MONGOC_OP_CODE_MSG : MONGOC_OP_CODE_QUERY;

   if (node->last_used != -1 && node->last_failed == -1 && !use_handshake) {
      /* The node's been used before and not failed recently */
      bson_copy_to(_mongoc_topology_scanner_get_monitoring_cmd(ts, node->hello_ok), &cmd);
   } else {
      _mongoc_topology_scanner_dup_handshake_cmd(ts, &cmd);
   }

   if (node->ts->negotiate_sasl_supported_mechs && !node->negotiated_sasl_supported_mechs) {
      _mongoc_handshake_append_sasl_supported_mechs(ts->uri, &cmd);
   }

   if (node->ts->speculative_authentication && !node->has_auth && bson_empty(&node->speculative_auth_response) &&
       node->scram.step == 0) {
      char *oidc_access_token = mongoc_oidc_cache_get_cached_token(ts->oidc_cache);
      _mongoc_topology_scanner_add_speculative_authentication(&cmd, ts->uri, oidc_access_token, node->id, &node->scram);
      mongoc_oidc_connection_cache_set(node->oidc_connection_cache, oidc_access_token);
      bson_free(oidc_access_token);
   }

   /* if the node should connect with a TCP socket, stream will be null, and
    * dns_result will be set. The async loop is responsible for calling the
    * _tcp_initiator to construct TCP sockets. */
   mongoc_async_cmd_new(ts->async,
                        stream,
                        is_setup_done,
                        dns_result,
                        _mongoc_topology_scanner_tcp_initiate,
                        initiate_delay,
                        ts->setup,
                        node->host.host,
                        "admin",
                        &cmd,
                        cmd_opcode,
                        &_async_handler,
                        node,
                        mlib_duration(ts->connect_timeout_msec, ms));

   bson_destroy(&cmd);
}


mongoc_topology_scanner_t *
mongoc_topology_scanner_new(const mongoc_uri_t *uri,
                            const bson_oid_t *topology_id,
                            mongoc_log_and_monitor_instance_t *log_and_monitor,
                            mongoc_topology_scanner_setup_err_cb_t setup_err_cb,
                            mongoc_topology_scanner_cb_t cb,
                            void *data,
                            int64_t connect_timeout_msec)
{
   mongoc_topology_scanner_t *ts = BSON_ALIGNED_ALLOC0(mongoc_topology_scanner_t);

   ts->async = mongoc_async_new();

   bson_oid_copy(topology_id, &ts->topology_id);
   ts->setup_err_cb = setup_err_cb;
   ts->cb = cb;
   ts->cb_data = data;
   ts->uri = uri;
   ts->appname = NULL;
   ts->log_and_monitor = log_and_monitor;
   ts->api = NULL;
   ts->handshake_state = HANDSHAKE_CMD_UNINITIALIZED;
   ts->connect_timeout_msec = connect_timeout_msec;
   /* may be overridden for testing. */
   ts->dns_cache_timeout_ms = DNS_CACHE_TIMEOUT_MS;
   bson_mutex_init(&ts->handshake_cmd_mtx);
#if defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   ts->secure_channel_cred_ptr = MONGOC_SHARED_PTR_NULL;
#endif

   _init_hello(ts);

   return ts;
}

#ifdef MONGOC_ENABLE_SSL
void
mongoc_topology_scanner_set_ssl_opts(mongoc_topology_scanner_t *ts, mongoc_ssl_opt_t *opts)
{
   ts->ssl_opts = opts;
   ts->setup = mongoc_async_cmd_tls_setup;
}
#endif

void
mongoc_topology_scanner_set_stream_initiator(mongoc_topology_scanner_t *ts, mongoc_stream_initiator_t si, void *ctx)
{
   ts->initiator = si;
   ts->initiator_context = ctx;
   ts->setup = NULL;
}

void
mongoc_topology_scanner_destroy(mongoc_topology_scanner_t *ts)
{
   mongoc_topology_scanner_node_t *ele, *tmp;

   DL_FOREACH_SAFE(ts->nodes, ele, tmp)
   {
      mongoc_topology_scanner_node_destroy(ele, false);
   }

   mongoc_async_destroy(ts->async);
   bson_destroy(&ts->hello_cmd);
   bson_destroy(&ts->legacy_hello_cmd);
   bson_destroy(ts->handshake_cmd);
   mongoc_server_api_destroy(ts->api);
   bson_mutex_destroy(&ts->handshake_cmd_mtx);

#if defined(MONGOC_ENABLE_SSL_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
   SSL_CTX_free(ts->openssl_ctx);
   ts->openssl_ctx = NULL;
#endif

#if defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
   mongoc_shared_ptr_reset_null(&ts->secure_channel_cred_ptr);
#endif

   /* This field can be set by a mongoc_client */
   bson_free((char *)ts->appname);

   bson_free(ts);
}

/* whether the scanner was successfully initialized - false if a mongodb+srv
 * URI failed to resolve to any hosts */
bool
mongoc_topology_scanner_valid(mongoc_topology_scanner_t *ts)
{
   return ts->nodes != NULL;
}

void
mongoc_topology_scanner_add(mongoc_topology_scanner_t *ts, const mongoc_host_list_t *host, uint32_t id, bool hello_ok)
{
   mongoc_topology_scanner_node_t *node;

   node = BSON_ALIGNED_ALLOC0(mongoc_topology_scanner_node_t);

   memcpy(&node->host, host, sizeof(*host));

   node->id = id;
   node->ts = ts;
   node->last_failed = -1;
   node->last_used = -1;
   node->hello_ok = hello_ok;
   node->oidc_connection_cache = mongoc_oidc_connection_cache_new();
   bson_init(&node->speculative_auth_response);

   DL_APPEND(ts->nodes, node);
}

void
mongoc_topology_scanner_scan(mongoc_topology_scanner_t *ts, uint32_t id)
{
   mongoc_topology_scanner_node_t *node;

   node = mongoc_topology_scanner_get_node(ts, id);

   /* begin non-blocking connection, don't wait for success */
   if (node) {
      mongoc_topology_scanner_node_setup(node, &node->last_error);
   }

   /* if setup fails the node stays in the scanner. destroyed after the scan. */
}

void
mongoc_topology_scanner_disconnect(mongoc_topology_scanner_t *scanner)
{
   mongoc_topology_scanner_node_t *node;

   BSON_ASSERT(scanner);
   node = scanner->nodes;

   while (node) {
      mongoc_topology_scanner_node_disconnect(node, false);
      node = node->next;
   }
}

void
mongoc_topology_scanner_node_retire(mongoc_topology_scanner_node_t *node)
{
   /* cancel any pending commands. */
   _cancel_commands_excluding(node, NULL);

   node->retired = true;
}

void
mongoc_topology_scanner_node_disconnect(mongoc_topology_scanner_node_t *node, bool failed)
{
   /* the node may or may not have succeeded in finding a working stream. */
   if (node->stream) {
      if (failed) {
         mongoc_stream_failed(node->stream);
      } else {
         mongoc_stream_destroy(node->stream);
      }

      node->stream = NULL;
   }
   mongoc_server_description_destroy(node->handshake_sd);
   node->handshake_sd = NULL;
   mongoc_oidc_connection_cache_set(node->oidc_connection_cache, NULL);
}

void
mongoc_topology_scanner_node_destroy(mongoc_topology_scanner_node_t *node, bool failed)
{
   DL_DELETE(node->ts->nodes, node);
   mongoc_topology_scanner_node_disconnect(node, failed);
   if (node->dns_results) {
      freeaddrinfo(node->dns_results);
   }

   bson_destroy(&node->speculative_auth_response);

#ifdef MONGOC_ENABLE_CRYPTO
   _mongoc_scram_destroy(&node->scram);
#endif

   mongoc_oidc_connection_cache_destroy(node->oidc_connection_cache);

   bson_free(node);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_get_node --
 *
 *      Return the scanner node with the given id.
 *
 *--------------------------------------------------------------------------
 */
mongoc_topology_scanner_node_t *
mongoc_topology_scanner_get_node(mongoc_topology_scanner_t *ts, uint32_t id)
{
   mongoc_topology_scanner_node_t *ele, *tmp;

   DL_FOREACH_SAFE(ts->nodes, ele, tmp)
   {
      if (ele->id == id) {
         return ele;
      }

      if (ele->id > id) {
         break;
      }
   }

   return NULL;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_has_node_for_host --
 *
 *      Whether the scanner has a node for the given host and port.
 *
 *--------------------------------------------------------------------------
 */
bool
mongoc_topology_scanner_has_node_for_host(mongoc_topology_scanner_t *ts, mongoc_host_list_t *host)
{
   mongoc_topology_scanner_node_t *ele, *tmp;

   DL_FOREACH_SAFE(ts->nodes, ele, tmp)
   {
      if (_mongoc_host_list_compare_one(&ele->host, host)) {
         return true;
      }
   }

   return false;
}

static void
_async_connected(mongoc_async_cmd_t *acmd)
{
   BSON_ASSERT_PARAM(acmd);
   mongoc_topology_scanner_node_t *const node = _scanner_node_of(acmd);
   /* this cmd connected successfully, cancel other cmds on this node. */
   _cancel_commands_excluding(node, acmd);
   node->successful_dns_result = acmd->dns_result;
}

static void
_async_success(mongoc_async_cmd_t *acmd, const bson_t *hello_response, mlib_duration elapsed)
{
   BSON_ASSERT_PARAM(acmd);
   BSON_ASSERT_PARAM(hello_response);
   mongoc_topology_scanner_node_t *const node = _scanner_node_of(acmd);
   mongoc_stream_t *const stream = acmd->stream;
   mongoc_topology_scanner_t *ts = node->ts;

   if (node->retired) {
      if (stream) {
         mongoc_stream_failed(stream);
      }
      return;
   }

   node->last_used = bson_get_monotonic_time();
   node->last_failed = -1;

   _mongoc_topology_scanner_monitor_heartbeat_succeeded(ts, &node->host, hello_response, elapsed);

   /* set our successful stream. */
   BSON_ASSERT(!node->stream);
   node->stream = stream;

   if (!node->handshake_sd) {
      mongoc_server_description_t sd;

      /* Store a server description associated with the handshake. */
      mongoc_server_description_init(&sd, node->host.host_and_port, node->id);
      mongoc_server_description_handle_hello(&sd, hello_response, mlib_milliseconds_count(elapsed), &acmd->error);
      node->handshake_sd = mongoc_server_description_new_copy(&sd);
      mongoc_server_description_cleanup(&sd);
   }

   if (ts->negotiate_sasl_supported_mechs && !node->negotiated_sasl_supported_mechs) {
      _mongoc_topology_scanner_node_parse_sasl_supported_mechs(hello_response, node);
   }

   if (ts->speculative_authentication) {
      _mongoc_topology_scanner_parse_speculative_authentication(hello_response, &node->speculative_auth_response);
   }

   /* mongoc_topology_scanner_cb_t takes rtt_msec, not usec */
   ts->cb(node->id, hello_response, mlib_milliseconds_count(elapsed), ts->cb_data, &acmd->error);
}

static void
_async_error_or_timeout(mongoc_async_cmd_t *acmd, mlib_duration elapsed, const char *default_err_msg)
{
   BSON_ASSERT_PARAM(acmd);
   mongoc_topology_scanner_node_t *const node = _scanner_node_of(acmd);
   mongoc_stream_t *stream = acmd->stream;
   mongoc_topology_scanner_t *ts = node->ts;
   bson_error_t *error = &acmd->error;
   int64_t now = bson_get_monotonic_time();
   const char *message;

   /* the stream may have failed on initiation. */
   if (stream) {
      mongoc_stream_failed(stream);
   }

   if (node->retired) {
      return;
   }

   node->last_used = now;

   if (!node->stream && _count_acmds(node) == 1) {
      /* there are no remaining streams, connecting has failed. */
      node->last_failed = now;
      if (error->code) {
         message = error->message;
      } else {
         message = default_err_msg;
      }

      /* invalidate any cached DNS results. */
      if (node->dns_results) {
         freeaddrinfo(node->dns_results);
         node->dns_results = NULL;
         node->successful_dns_result = NULL;
      }

      _mongoc_set_error(&node->last_error,
                        MONGOC_ERROR_CLIENT,
                        MONGOC_ERROR_STREAM_CONNECT,
                        "%s calling hello on \'%s\'",
                        message,
                        node->host.host_and_port);

      _mongoc_topology_scanner_monitor_heartbeat_failed(ts, &node->host, &node->last_error, elapsed);

      /* call the topology scanner callback. cannot connect to this node.
       * callback takes rtt_msec, not usec. */
      ts->cb(node->id, NULL, mlib_milliseconds_count(elapsed), ts->cb_data, error);

      mongoc_server_description_destroy(node->handshake_sd);
      node->handshake_sd = NULL;
   } else {
      /* there are still more commands left for this node or it succeeded
       * with another stream. skip the topology scanner callback. */
      _jumpstart_other_acmds(acmd);
   }
}

/*
 *-----------------------------------------------------------------------
 *
 * This is the callback passed to async_cmd when we're running
 * hellos from within the topology monitor.
 *
 *-----------------------------------------------------------------------
 */

static void
_async_handler(mongoc_async_cmd_t *acmd,
               mongoc_async_cmd_result_t async_status,
               const bson_t *hello_response,
               mlib_duration duration)
{
   switch (async_status) {
   case MONGOC_ASYNC_CMD_CONNECTED:
      _async_connected(acmd);
      return;
   case MONGOC_ASYNC_CMD_SUCCESS:
      _async_success(acmd, hello_response, duration);
      return;
   case MONGOC_ASYNC_CMD_TIMEOUT:
      _async_error_or_timeout(acmd, duration, "connection timeout");
      return;
   case MONGOC_ASYNC_CMD_ERROR:
      _async_error_or_timeout(acmd, duration, "connection error");
      return;
   case MONGOC_ASYNC_CMD_IN_PROGRESS:
   default:
      fprintf(stderr, "unexpected async status: %d\n", (int)async_status);
      BSON_ASSERT(false);
      return;
   }
}

mongoc_stream_t *
_mongoc_topology_scanner_node_setup_stream_for_tls(mongoc_topology_scanner_node_t *node, mongoc_stream_t *stream)
{
#ifdef MONGOC_ENABLE_SSL
   mongoc_stream_t *tls_stream;
#endif
   if (!stream) {
      return NULL;
   }
#ifdef MONGOC_ENABLE_SSL
   if (node->ts->ssl_opts) {
#if defined(MONGOC_ENABLE_SSL_OPENSSL) && OPENSSL_VERSION_NUMBER >= 0x10100000L
      tls_stream = mongoc_stream_tls_new_with_hostname_and_openssl_context(
         stream, node->host.host, node->ts->ssl_opts, 1, node->ts->openssl_ctx);
#elif defined(MONGOC_ENABLE_SSL_SECURE_CHANNEL)
      tls_stream = mongoc_stream_tls_new_with_secure_channel_cred(
         stream, node->host.host, node->ts->ssl_opts, node->ts->secure_channel_cred_ptr);
#else
      tls_stream = mongoc_stream_tls_new_with_hostname(stream, node->host.host, node->ts->ssl_opts, 1);
#endif
      if (!tls_stream) {
         mongoc_stream_destroy(stream);
         return NULL;
      } else {
         return tls_stream;
      }
   }
#endif
   return stream;
}

/* attempt to create a new socket stream using this dns result. */
mongoc_stream_t *
_mongoc_topology_scanner_tcp_initiate(mongoc_async_cmd_t *acmd)
{
   BSON_ASSERT_PARAM(acmd);
   mongoc_topology_scanner_node_t *const node = _scanner_node_of(acmd);
   struct addrinfo *res = acmd->dns_result;
   mongoc_socket_t *sock = NULL;

   BSON_ASSERT(acmd->dns_result);
   /* create a new non-blocking socket. */
   if (!(sock = mongoc_socket_new(res->ai_family, res->ai_socktype, res->ai_protocol))) {
      return NULL;
   }

   (void)mongoc_socket_connect(sock, res->ai_addr, (mongoc_socklen_t)res->ai_addrlen, 0);

   return _mongoc_topology_scanner_node_setup_stream_for_tls(node, mongoc_stream_socket_new(sock));
}
/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_node_setup_tcp --
 *
 *      Create an async command for each DNS record found for this node.
 *
 * Returns:
 *      A bool. On failure error is set.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_topology_scanner_node_setup_tcp(mongoc_topology_scanner_node_t *node, bson_error_t *error)
{
   struct addrinfo hints;
   struct addrinfo *iter;
   char portstr[8];
   mongoc_host_list_t *host;
   int s;
   mlib_duration delay = mlib_duration();
   int64_t now = bson_get_monotonic_time();

   ENTRY;

   host = &node->host;

   /* if cached dns results are expired, flush. */
   if (node->dns_results && (now - node->last_dns_cache) > node->ts->dns_cache_timeout_ms * 1000) {
      freeaddrinfo(node->dns_results);
      node->dns_results = NULL;
      node->successful_dns_result = NULL;
   }

   if (!node->dns_results) {
      // Expect no truncation.
      int req = bson_snprintf(portstr, sizeof portstr, "%hu", host->port);
      BSON_ASSERT(mlib_cmp(req, <, sizeof portstr));

      memset(&hints, 0, sizeof hints);
      hints.ai_family = host->family;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = 0;
      hints.ai_protocol = 0;

      s = getaddrinfo(host->host, portstr, &hints, &node->dns_results);

      if (s != 0) {
         mongoc_counter_dns_failure_inc();
         _mongoc_set_error(
            error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_NAME_RESOLUTION, "Failed to resolve '%s'", host->host);
         RETURN(false);
      }

      mongoc_counter_dns_success_inc();
      node->last_dns_cache = now;
   }

   if (node->successful_dns_result) {
      _begin_hello_cmd(node,
                       NULL /* stream */,
                       false /* is_setup_done */,
                       node->successful_dns_result,
                       mlib_duration() /* initiate_delay */,
                       true /* use_handshake */);
   } else {
      LL_FOREACH2(node->dns_results, iter, ai_next)
      {
         _begin_hello_cmd(node, NULL /* stream */, false /* is_setup_done */, iter, delay, true /* use_handshake */);
         /* each subsequent DNS result will have an additional 250ms delay. */
         delay = mlib_duration(delay, plus, HAPPY_EYEBALLS_DELAY);
      }
   }

   RETURN(true);
}

bool
mongoc_topology_scanner_node_connect_unix(mongoc_topology_scanner_node_t *node, bson_error_t *error)
{
#ifdef _WIN32
   ENTRY;
   BSON_UNUSED(node);
   _mongoc_set_error(
      error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT, "UNIX domain sockets not supported on win32.");
   RETURN(false);
#else
   struct sockaddr_un saddr;
   mongoc_socket_t *sock;
   mongoc_stream_t *stream;
   mongoc_host_list_t *host;

   ENTRY;

   host = &node->host;

   memset(&saddr, 0, sizeof saddr);
   saddr.sun_family = AF_UNIX;
   // Expect no truncation.
   int req = bson_snprintf(saddr.sun_path, sizeof saddr.sun_path - 1, "%s", host->host);

   if (mlib_cmp(req, >=, sizeof saddr.sun_path - 1)) {
      _mongoc_set_error(
         error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "Failed to define socket address path.");
      RETURN(false);
   }

   sock = mongoc_socket_new(AF_UNIX, SOCK_STREAM, 0);

   if (sock == NULL) {
      _mongoc_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_SOCKET, "Failed to create socket.");
      RETURN(false);
   }

   if (-1 == mongoc_socket_connect(sock, (struct sockaddr *)&saddr, sizeof saddr, -1)) {
      char buf[128];
      char *errstr;

      errstr = bson_strerror_r(mongoc_socket_errno(sock), buf, sizeof(buf));

      _mongoc_set_error(error,
                        MONGOC_ERROR_STREAM,
                        MONGOC_ERROR_STREAM_CONNECT,
                        "Failed to connect to UNIX domain socket: %s",
                        errstr);
      mongoc_socket_destroy(sock);
      RETURN(false);
   }

   stream = _mongoc_topology_scanner_node_setup_stream_for_tls(node, mongoc_stream_socket_new(sock));
   if (stream) {
      _begin_hello_cmd(node,
                       stream,
                       false /* is_setup_done */,
                       NULL /* dns result */,
                       mlib_duration() /* no delay */,
                       true /* use_handshake */);
      RETURN(true);
   }
   _mongoc_set_error(error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_CONNECT, "Failed to create TLS stream");
   RETURN(false);
#endif
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_node_setup --
 *
 *      Create a stream and begin a non-blocking connect.
 *
 * Returns:
 *      true on success, or false and error is set.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_node_setup(mongoc_topology_scanner_node_t *node, bson_error_t *error)
{
   bool success = false;
   mongoc_stream_t *stream;

   _mongoc_topology_scanner_monitor_heartbeat_started(node->ts, &node->host);
   const mlib_time_point start_time = mlib_now();

   /* if there is already a working stream, push it back to be re-scanned. */
   if (node->stream) {
      _begin_hello_cmd(node,
                       node->stream,
                       true /* is_setup_done */,
                       NULL /* dns_result */,
                       mlib_duration() /* initiate_delay */,
                       false /* use_handshake */);
      node->stream = NULL;
      return;
   }

   BSON_ASSERT(!node->retired);

   // If a new stream is needed, reset state authentication state.
   // Authentication state is tied to a stream.
   {
      node->has_auth = false;
      bson_reinit(&node->speculative_auth_response);
#ifdef MONGOC_ENABLE_CRYPTO
      // Destroy and zero `node->scram`.
      _mongoc_scram_destroy(&node->scram);
#endif
      memset(&node->sasl_supported_mechs, 0, sizeof(node->sasl_supported_mechs));
      node->negotiated_sasl_supported_mechs = false;
   }

   if (node->ts->initiator) {
      stream = node->ts->initiator(node->ts->uri, &node->host, node->ts->initiator_context, error);
      if (stream) {
         success = true;
         _begin_hello_cmd(node,
                          stream,
                          false /* is_setup_done */,
                          NULL /* dns_result */,
                          mlib_duration() /* initiate_delay */,
                          true /* use_handshake */);
      }
   } else {
      if (node->host.family == AF_UNIX) {
         success = mongoc_topology_scanner_node_connect_unix(node, error);
      } else {
         success = mongoc_topology_scanner_node_setup_tcp(node, error);
      }
   }

   if (!success) {
      _mongoc_topology_scanner_monitor_heartbeat_failed(node->ts, &node->host, error, mlib_elapsed_since(start_time));

      node->ts->setup_err_cb(node->id, node->ts->cb_data, error);
      return;
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_node_in_cooldown --
 *
 *      Return true if @node has experienced a network error attempting
 *      to call "hello" less than 5 seconds before @when, a timestamp in
 *      microseconds.
 *
 *      Server Discovery and Monitoring Spec: "After a single-threaded client
 *      gets a network error trying to check a server, the client skips
 *      re-checking the server until cooldownMS has passed. This avoids
 *      spending connectTimeoutMS on each unavailable server during each scan.
 *      This value MUST be 5000 ms, and it MUST NOT be configurable."
 *
 *--------------------------------------------------------------------------
 */
bool
mongoc_topology_scanner_node_in_cooldown(mongoc_topology_scanner_node_t *node, int64_t when)
{
   if (node->last_failed == -1 || node->ts->bypass_cooldown) {
      return false; /* node is new, or connected */
   }

   return node->last_failed + 1000 * MONGOC_TOPOLOGY_COOLDOWN_MS >= when;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_in_cooldown --
 *
 *      Return true if all nodes will be in cooldown at time @when, a
 *      timestamp in microseconds.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_topology_scanner_in_cooldown(mongoc_topology_scanner_t *ts, int64_t when)
{
   mongoc_topology_scanner_node_t *node;

   if (ts->bypass_cooldown) {
      return false;
   }
   DL_FOREACH(ts->nodes, node)
   {
      if (!mongoc_topology_scanner_node_in_cooldown(node, when)) {
         return false;
      }
   }

   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_start --
 *
 *      Initializes the scanner and begins a full topology check. This
 *      should be called once before calling mongoc_topology_scanner_work()
 *      to complete the scan.
 *
 *      If "obey_cooldown" is true, this is a single-threaded blocking scan
 *      that must obey the Server Discovery And Monitoring Spec's cooldownMS:
 *
 *      "After a single-threaded client gets a network error trying to check
 *      a server, the client skips re-checking the server until cooldownMS has
 *      passed.
 *
 *      "This avoids spending connectTimeoutMS on each unavailable server
 *      during each scan.
 *
 *      "This value MUST be 5000 ms, and it MUST NOT be configurable."
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_start(mongoc_topology_scanner_t *ts, bool obey_cooldown)
{
   mongoc_topology_scanner_node_t *node, *tmp;
   bool skip;
   int64_t now;

   BSON_ASSERT(ts);

   _delete_retired_nodes(ts);

   now = bson_get_monotonic_time();

   DL_FOREACH_SAFE(ts->nodes, node, tmp)
   {
      skip = obey_cooldown && mongoc_topology_scanner_node_in_cooldown(node, now);

      if (!skip) {
         mongoc_topology_scanner_node_setup(node, &node->last_error);
      }
   }
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_finish_scan --
 *
 *      Summarizes all scanner node errors into one error message,
 *      deletes retired nodes.
 *
 *--------------------------------------------------------------------------
 */

void
_mongoc_topology_scanner_finish(mongoc_topology_scanner_t *ts)
{
   mongoc_topology_scanner_node_t *node, *tmp;

   bson_error_t *error = &ts->error;
   memset(&ts->error, 0, sizeof(bson_error_t));

   mcommon_string_append_t msg;
   mcommon_string_new_as_fixed_capacity_append(&msg, sizeof error->message - 1u);

   DL_FOREACH_SAFE(ts->nodes, node, tmp)
   {
      if (node->last_error.code) {
         if (!mcommon_string_from_append_is_empty(&msg)) {
            mcommon_string_append(&msg, " ");
         }

         mcommon_string_append_printf(&msg, "[%s]", node->last_error.message);

         /* last error domain and code win */
         error->domain = node->last_error.domain;
         error->code = node->last_error.code;
         error->reserved = node->last_error.reserved;
      }
   }

   bson_strncpy((char *)&error->message, mcommon_str_from_append(&msg), sizeof error->message);
   mcommon_string_from_append_destroy(&msg);

   _delete_retired_nodes(ts);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_work --
 *
 *      Crank the knob on the topology scanner state machine. This should
 *      be called only after mongoc_topology_scanner_start() has been used
 *      to begin the scan.
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_work(mongoc_topology_scanner_t *ts)
{
   mongoc_async_run(ts->async);
   BSON_ASSERT(ts->async->ncmds == 0);
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_topology_scanner_get_error --
 *
 *      Copy the scanner's current error; which may no-error (code 0).
 *
 *--------------------------------------------------------------------------
 */

void
mongoc_topology_scanner_get_error(mongoc_topology_scanner_t *ts, bson_error_t *error)
{
   BSON_ASSERT(ts);
   BSON_ASSERT(error);

   *error = ts->error;
}

/*
 * Set a field in the topology scanner.
 */
bool
_mongoc_topology_scanner_set_appname(mongoc_topology_scanner_t *ts, const char *appname)
{
   char *s;
   const char *prev;
   if (!_mongoc_handshake_appname_is_valid(appname)) {
      MONGOC_ERROR("Cannot set appname: %s is invalid", appname);
      return false;
   }

   s = bson_strdup(appname);
   prev = mcommon_atomic_ptr_compare_exchange_strong((void *)&ts->appname, NULL, s, mcommon_memory_order_relaxed);
   if (prev == NULL) {
      return true;
   }

   MONGOC_ERROR("Cannot set appname more than once");
   bson_free(s);
   return false;
}

/* SDAM Monitoring Spec: send HeartbeatStartedEvent */
static void
_mongoc_topology_scanner_monitor_heartbeat_started(const mongoc_topology_scanner_t *ts, const mongoc_host_list_t *host)
{
   mongoc_structured_log(ts->log_and_monitor->structured_log,
                         MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                         MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                         "Server heartbeat started",
                         oid("topologyId", &ts->topology_id),
                         utf8("serverHost", host->host),
                         int32("serverPort", host->port),
                         boolean("awaited", false));

   if (ts->log_and_monitor->apm_callbacks.server_heartbeat_started) {
      mongoc_apm_server_heartbeat_started_t event;
      event.host = host;
      event.context = ts->log_and_monitor->apm_context;
      event.awaited = false;
      ts->log_and_monitor->apm_callbacks.server_heartbeat_started(&event);
   }
}

/* SDAM Monitoring Spec: send HeartbeatSucceededEvent */
static void
_mongoc_topology_scanner_monitor_heartbeat_succeeded(const mongoc_topology_scanner_t *ts,
                                                     const mongoc_host_list_t *host,
                                                     const bson_t *reply,
                                                     mlib_duration elapsed)
{
   /* This redaction is more lenient than the general command redaction in the Command Logging and Monitoring spec and
    * the cmd*() structured log items. In those general command logs, sensitive replies are omitted entirely. In this
    * APM message, the reply is passed through with only the speculativeAuthenticate field stripped. The Server
    * Discovery and Monitoring Logging spec does not mention reply redaction, so we choose to be consistent with the APM
    * event. */

   bson_t hello_redacted;
   bson_init(&hello_redacted);
   bson_copy_to_excluding_noinit(reply, &hello_redacted, "speculativeAuthenticate", NULL);

   mongoc_structured_log(ts->log_and_monitor->structured_log,
                         MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                         MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                         "Server heartbeat succeeded",
                         oid("topologyId", &ts->topology_id),
                         utf8("serverHost", host->host),
                         int32("serverPort", host->port),
                         boolean("awaited", false),
                         monotonic_time_duration(mlib_microseconds_count(elapsed)),
                         bson_as_json("reply", &hello_redacted));

   if (ts->log_and_monitor->apm_callbacks.server_heartbeat_succeeded) {
      mongoc_apm_server_heartbeat_succeeded_t event;
      event.host = host;
      event.context = ts->log_and_monitor->apm_context;
      event.reply = reply;
      event.duration_usec = mlib_microseconds_count(elapsed);
      event.awaited = false;
      ts->log_and_monitor->apm_callbacks.server_heartbeat_succeeded(&event);
   }

   bson_destroy(&hello_redacted);
}

/* SDAM Monitoring Spec: send HeartbeatFailedEvent */
static void
_mongoc_topology_scanner_monitor_heartbeat_failed(const mongoc_topology_scanner_t *ts,
                                                  const mongoc_host_list_t *host,
                                                  const bson_error_t *error,
                                                  mlib_duration elapsed)
{
   mongoc_structured_log(ts->log_and_monitor->structured_log,
                         MONGOC_STRUCTURED_LOG_LEVEL_DEBUG,
                         MONGOC_STRUCTURED_LOG_COMPONENT_TOPOLOGY,
                         "Server heartbeat failed",
                         oid("topologyId", &ts->topology_id),
                         utf8("serverHost", host->host),
                         int32("serverPort", host->port),
                         boolean("awaited", false),
                         monotonic_time_duration(mlib_microseconds_count(elapsed)),
                         error("failure", error));

   if (ts->log_and_monitor->apm_callbacks.server_heartbeat_failed) {
      mongoc_apm_server_heartbeat_failed_t event;
      event.host = host;
      event.context = ts->log_and_monitor->apm_context;
      event.error = error;
      event.duration_usec = mlib_microseconds_count(elapsed);
      event.awaited = false;
      ts->log_and_monitor->apm_callbacks.server_heartbeat_failed(&event);
   }
}

/* this is for testing the dns cache timeout. */
void
_mongoc_topology_scanner_set_dns_cache_timeout(mongoc_topology_scanner_t *ts, int64_t timeout_ms)
{
   ts->dns_cache_timeout_ms = timeout_ms;
}

/* reset "retired" nodes that failed or were removed in the previous scan */
static void
_delete_retired_nodes(mongoc_topology_scanner_t *ts)
{
   mongoc_topology_scanner_node_t *node, *tmp;

   DL_FOREACH_SAFE(ts->nodes, node, tmp)
   {
      if (node->retired) {
         mongoc_topology_scanner_node_destroy(node, true);
      }
   }
}

static void
_cancel_commands_excluding(mongoc_topology_scanner_node_t *node, mongoc_async_cmd_t *acmd)
{
   mongoc_async_cmd_t *iter;
   DL_FOREACH(node->ts->async->cmds, iter)
   {
      if (acmd && _is_sibling_command(iter, acmd)) {
         _acmd_cancel(iter);
      }
   }
}

static int
_count_acmds(mongoc_topology_scanner_node_t *node)
{
   mongoc_async_cmd_t *iter;
   int count = 0;
   DL_FOREACH(node->ts->async->cmds, iter)
   {
      if (_scanner_node_of(iter) == node) {
         ++count;
      }
   }
   return count;
}

void
_mongoc_topology_scanner_set_server_api(mongoc_topology_scanner_t *ts, const mongoc_server_api_t *api)
{
   BSON_ASSERT(ts);
   BSON_ASSERT(api);
   mongoc_server_api_destroy(ts->api);
   ts->api = mongoc_server_api_copy(api);
   _reset_hello(ts);
}

/* This must be called before the handshake command is constructed. */
void
_mongoc_topology_scanner_set_loadbalanced(mongoc_topology_scanner_t *ts, bool val)
{
   BSON_UNUSED(val);

   BSON_ASSERT(ts->handshake_cmd == NULL);
   ts->loadbalanced = true;
}

bool
mongoc_topology_scanner_uses_server_api(const mongoc_topology_scanner_t *ts)
{
   BSON_ASSERT_PARAM(ts);
   return NULL != ts->api;
}

bool
mongoc_topology_scanner_uses_loadbalanced(const mongoc_topology_scanner_t *ts)
{
   BSON_ASSERT_PARAM(ts);
   return ts->loadbalanced;
}

void
_mongoc_topology_scanner_set_oidc_cache(mongoc_topology_scanner_t *ts, mongoc_oidc_cache_t *oidc_cache)
{
   BSON_ASSERT_PARAM(ts);
   BSON_ASSERT_PARAM(oidc_cache);
   ts->oidc_cache = oidc_cache;
}
