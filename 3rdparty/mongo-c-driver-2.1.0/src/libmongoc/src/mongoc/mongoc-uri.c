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


#include <sys/types.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* strcasecmp on windows */
#include <common-bson-dsl-private.h>
#include <common-string-private.h>
#include <mongoc/mongoc-compression-private.h>
#include <mongoc/mongoc-error-private.h>
#include <mongoc/mongoc-handshake-private.h>
#include <mongoc/mongoc-host-list-private.h>
#include <mongoc/mongoc-oidc-env-private.h>
#include <mongoc/mongoc-read-concern-private.h>
#include <mongoc/mongoc-topology-private.h>
#include <mongoc/mongoc-trace-private.h>
#include <mongoc/mongoc-uri-private.h>
#include <mongoc/mongoc-util-private.h>
#include <mongoc/mongoc-write-concern-private.h>

#include <mongoc/mongoc-config.h>
#include <mongoc/mongoc-host-list.h>
#include <mongoc/mongoc-log.h>
#include <mongoc/mongoc-socket.h>
#include <mongoc/utlist.h>

#include <mlib/intencode.h>
#include <mlib/str.h>

struct _mongoc_uri_t {
   char *str;
   bool is_srv;
   char srv[BSON_HOST_NAME_MAX + 1];
   mongoc_host_list_t *hosts;
   char *username; // MongoCredential.username
   char *password; // MongoCredential.password
   char *database;
   bson_t raw;         // Unparsed options, see mongoc_uri_parse_options
   bson_t options;     // Type-coerced and canonicalized options
   bson_t credentials; // MongoCredential.source, MongoCredential.mechanism, and MongoCredential.mechanism_properties.
   bson_t compressors;
   mongoc_read_prefs_t *read_prefs;
   mongoc_read_concern_t *read_concern;
   mongoc_write_concern_t *write_concern;
};

// Common strings we need to look for
static const mstr_view COLON = {":", 1};
static const mstr_view COMMA = {",", 1};
static const mstr_view QUESTION = {"?", 1};
static const mstr_view SLASH = {"/", 1};
static const mstr_view AT = {"@", 1};

#define MONGOC_URI_ERROR(error, format, ...) \
   _mongoc_set_error (error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, format, __VA_ARGS__)


static const char *escape_instructions = "Percent-encode username and password"
                                         " according to RFC 3986";

static bool
_mongoc_uri_set_option_as_int32 (mongoc_uri_t *uri, const char *option, int32_t value);

static bool
_mongoc_uri_set_option_as_int32_with_error (mongoc_uri_t *uri, const char *option, int32_t value, bson_error_t *error);

static bool
_mongoc_uri_set_option_as_int64_with_error (mongoc_uri_t *uri, const char *option, int64_t value, bson_error_t *error);


#define VALIDATE_SRV_ERR()                                                   \
   do {                                                                      \
      _mongoc_set_error (error,                                              \
                         MONGOC_ERROR_STREAM,                                \
                         MONGOC_ERROR_STREAM_NAME_RESOLUTION,                \
                         "Invalid host \"%s\" returned for service \"%s\": " \
                         "host must be subdomain of service name",           \
                         host,                                               \
                         srv_hostname);                                      \
      return false;                                                          \
   } while (0)


static int
count_dots (const char *s)
{
   int n = 0;
   const char *dot = s;

   while ((dot = strchr (dot + 1, '.'))) {
      n++;
   }

   return n;
}

static char *
lowercase_str_new (const char *key)
{
   char *ret = bson_strdup (key);
   mongoc_lowercase (key, ret);
   return ret;
}

/* at least one character, and does not start with dot */
static bool
valid_hostname (const char *s)
{
   size_t len = strlen (s);

   return len > 1 && s[0] != '.';
}


bool
mongoc_uri_validate_srv_result (const mongoc_uri_t *uri, const char *host, bson_error_t *error)
{
   const char *srv_hostname;
   const char *srv_host;

   srv_hostname = mongoc_uri_get_srv_hostname (uri);
   BSON_ASSERT (srv_hostname);

   if (!valid_hostname (host)) {
      VALIDATE_SRV_ERR ();
   }

   srv_host = strchr (srv_hostname, '.');
   BSON_ASSERT (srv_host);

   /* host must be descendent of service root: if service is
    * "a.foo.co" host can be like "a.foo.co", "b.foo.co", "a.b.foo.co", etc.
    */
   if (strlen (host) < strlen (srv_host)) {
      VALIDATE_SRV_ERR ();
   }

   if (!mongoc_ends_with (host, srv_host)) {
      VALIDATE_SRV_ERR ();
   }

   return true;
}

/* copy and upsert @host into @uri's host list. */
static bool
_upsert_into_host_list (mongoc_uri_t *uri, mongoc_host_list_t *host, bson_error_t *error)
{
   if (uri->is_srv && !mongoc_uri_validate_srv_result (uri, host->host, error)) {
      return false;
   }

   _mongoc_host_list_upsert (&uri->hosts, host);

   return true;
}

bool
mongoc_uri_upsert_host_and_port (mongoc_uri_t *uri, const char *host_and_port, bson_error_t *error)
{
   mongoc_host_list_t temp;

   memset (&temp, 0, sizeof (mongoc_host_list_t));
   if (!_mongoc_host_list_from_string_with_err (&temp, host_and_port, error)) {
      return false;
   }

   return _upsert_into_host_list (uri, &temp, error);
}

bool
mongoc_uri_upsert_host (mongoc_uri_t *uri, const char *host, uint16_t port, bson_error_t *error)
{
   mongoc_host_list_t temp;

   memset (&temp, 0, sizeof (mongoc_host_list_t));
   if (!_mongoc_host_list_from_hostport_with_err (&temp, mstr_cstring (host), port, error)) {
      return false;
   }

   return _upsert_into_host_list (uri, &temp, error);
}

void
mongoc_uri_remove_host (mongoc_uri_t *uri, const char *host, uint16_t port)
{
   _mongoc_host_list_remove_host (&(uri->hosts), host, port);
}


/**
 * @brief %-decode a %-encoded string
 *
 * @param sv The string to be decoded
 * @return char* A pointer to a new C string, which must be freed with `bson_free`,
 * or a null pointer in case of error
 */
static char *
_strdup_pct_decode (mstr_view const sv, bson_error_t *error)
{
   // Compute how many bytes we want to store
   size_t bufsize = 0;
   // Must use safe arithmetic because a pathological sv with `len == SIZE_MAX` is possible
   bool add_okay = !mlib_add (&bufsize, sv.len, 1);
   // Prepare the output region. We can allocate the whole thing up-front, because
   // we know the decode result will be *at most* as long as `sv`, since %-encoding
   // can only ever grow the plaintext string
   char *const buf = add_okay ? bson_malloc0 (bufsize) : NULL;
   // alloc or arithmetic failure
   if (!buf) {
      MONGOC_URI_ERROR (error, "%s", "Failed to allocate memory for the %%-decoding");
      return NULL;
   }

   // char-wise output
   char *out = buf;
   // Consume the input as we go
   mstr_view remain = sv;
   while (remain.len) {
      if (remain.data[0] != '%') {
         // Not a % char, just append it
         *out++ = remain.data[0];
         remain = mstr_substr (remain, 1);
         continue;
      }
      // %-sequence
      if (remain.len < 3) {
         MONGOC_URI_ERROR (
            error, "At offset %zu: Truncated %%-sequence \"%.*s\"", (sv.len - remain.len), MSTR_FMT (remain));
         bson_free (buf);
         return NULL;
      }
      // Grab the next two chars
      mstr_view pair = mstr_substr (remain, 1, 2);
      uint64_t v;
      if (mlib_nat64_parse (pair, 16, &v)) {
         MONGOC_URI_ERROR (error, "At offset %zu: Invalid %%-sequence \"%.3s\"", (sv.len - remain.len), remain.data);
         bson_free (buf);
         return NULL;
      }

      // Append the decoded byte value
      *out++ = (char) v;
      // Drop the "%xy" sequence
      remain = mstr_substr (remain, 3);
   }

   // Check whether the decoded result is valid UTF-8
   size_t len = (size_t) (out - buf);
   if (!bson_utf8_validate (buf, len, false)) {
      MONGOC_URI_ERROR (
         error, "%s", "Invalid %%-encoded string: The decoded result is not valid UTF-8 or contains null characters");
      bson_free (buf);
      return NULL;
   }

   return buf;
}


/**
 * @brief Parse the userinfo segment from a URI string
 *
 * @param uri The URI to be updated
 * @param userpass The userinfo segment from the original URI string
 * @return true If the operation succeeds
 * @return false Otherwise
 */
static bool
_uri_parse_userinfo (mongoc_uri_t *uri, mstr_view userpass, bson_error_t *error)
{
   bson_error_reset (error);
   BSON_ASSERT (uri);

   // Split the user/pass around the colon:
   mstr_view username, password;
   const bool has_password = mstr_split_around (userpass, COLON, &username, &password);

   // Check if the username has invalid unescaped characters
   const mstr_view PROHIBITED_CHARS = mstr_cstring ("@:/");
   if (mstr_find_first_of (username, PROHIBITED_CHARS) != SIZE_MAX) {
      MONGOC_URI_ERROR (error, "Username must not have unescaped chars. %s", escape_instructions);
      return false;
   }
   if (mstr_find_first_of (password, PROHIBITED_CHARS) != SIZE_MAX) {
      MONGOC_URI_ERROR (error, "Password must not have unescaped chars. %s", escape_instructions);
      return false;
   }

   // Store the username and password on the URI
   uri->username = _strdup_pct_decode (username, error);
   if (!uri->username) {
      MONGOC_URI_ERROR (error, "%s", "Invalid %-encoding in username in URI string");
      return false;
   }

   /* Providing password at all is optional */
   if (has_password) {
      uri->password = _strdup_pct_decode (password, error);
      if (!uri->password) {
         MONGOC_URI_ERROR (error, "%s", "Invalid %-encoding in password in URI string");
         return false;
      }
   }

   return true;
}

/**
 * @brief Parse a single host specifier for a URI
 *
 * @param uri The URI object to be updated
 * @param hostport A host specifier, with an optional port
 * @return true If the operation succeeds
 * @return false Otherwise
 */
static bool
_parse_one_host (mongoc_uri_t *uri, mstr_view hostport, bson_error_t *error)
{
   bson_error_reset (error);
   // Don't allow an unescaped "/" in the host string.
   if (mstr_find (hostport, SLASH) != SIZE_MAX) {
      // They were probably trying to do a unix socket. Those slashes must be escaped
      MONGOC_WARNING ("Unix Domain Sockets must be escaped (e.g. / = %%2F)");
      return false;
   }

   /* unescape host. It doesn't hurt including port. */
   char *host_and_port = _strdup_pct_decode (hostport, error);
   if (!host_and_port) {
      /* invalid */
      MONGOC_URI_ERROR (error, "Invalid host specifier \"%.*s\": %s", MSTR_FMT (hostport), error->message);
      return false;
   }

   const bool okay = mongoc_uri_upsert_host_and_port (uri, host_and_port, error);
   if (!okay) {
      MONGOC_URI_ERROR (error, "Invalid host specifier \"%s\": %s", host_and_port, error->message);
   }

   bson_free (host_and_port);
   return okay;
}


/**
 * @brief Parse the single SRV host specifier for a URI
 *
 * @param uri The URI to be updated
 * @param str The host string for the URI. Should specify a single SRV name
 * @return true If the operation succeeds
 * @return false Otherwise
 */
static bool
_parse_srv_hostname (mongoc_uri_t *uri, mstr_view str, bson_error_t *error)
{
   bson_error_reset (error);
   if (str.len == 0) {
      MONGOC_URI_ERROR (error, "%s", "Missing service name in SRV URI");
      return false;
   }

   {
      char *service = _strdup_pct_decode (str, error);
      if (!service || !valid_hostname (service) || count_dots (service) < 2) {
         MONGOC_URI_ERROR (error, "Invalid SRV service name \"%.*s\" in URI: %s", MSTR_FMT (str), error->message);
         bson_free (service);
         return false;
      }

      bson_strncpy (uri->srv, service, sizeof uri->srv);

      bson_free (service);
   }

   if (strchr (uri->srv, ',')) {
      MONGOC_URI_ERROR (error, "%s", "Multiple service names are prohibited in an SRV URI");
      return false;
   }

   if (strchr (uri->srv, ':')) {
      MONGOC_URI_ERROR (error, "%s", "Port numbers are prohibited in an SRV URI");
      return false;
   }

   return true;
}


/**
 * @brief Parse the comma-separate list of host+port specifiers and store them in `uri`
 *
 * @param uri The URI object to be updated
 * @param hosts A non-empty comma-separated list of host specifiers
 * @param error An error object to be updated in case of failure
 * @return true If the operation succeeds and at least one host was added to `uri`
 * @return false Otherise. `error` will be updated.
 */
static bool
_parse_hosts_csv (mongoc_uri_t *uri, mstr_view const hosts, bson_error_t *error)
{
   bson_error_reset (error);
   // Check if there is a question mark in the given hostinfo string. This indicates that
   // the user omitted a required "/" before the query component
   if (mstr_find (hosts, QUESTION) != SIZE_MAX) {
      MONGOC_URI_ERROR (error, "%s", "A '/' is required between the host list and any options.");
      return false;
   }
   // We require at least one host in the host list in order to be a valid host CSV
   if (!hosts.len) {
      MONGOC_URI_ERROR (error, "%s", "Host list of URI string cannot be empty");
      return false;
   }

   // Split around commas
   for (mstr_view remain = hosts; remain.len;) {
      mstr_view host;
      mstr_split_around (remain, COMMA, &host, &remain);
      if (!_parse_one_host (uri, host, error)) {
         return false;
      }
   }

   return true;
}

/**
 * @brief Handle the URI path component
 *
 * @param uri The URI object to be updated
 * @param path The path component of the original URI string. May be empty if
 * there was no path in the input string, but should start with the leading
 * slash if it is non-empty.
 * @return true If the operation succeeds
 * @return false Otherwise
 *
 * We use the URI path to specify the database to be associated with the URI.
 * We only expect a single path element. If the path is just a slash "/", then
 * that is the same as omitting the path entirely.
 */
static bool
_parse_path (mongoc_uri_t *uri, mstr_view path, bson_error_t *error)
{
   bson_error_reset (error);
   // Drop the leading slash, if present. If the URI has no path, then `path`
   // will already be an empty string.
   const mstr_view relative = path.len ? mstr_substr (path, 1) : path;

   if (!relative.len) {
      // Empty/absent path is no database
      uri->database = NULL;
      return true;
   }

   // %-decode the path as the database name
   uri->database = _strdup_pct_decode (relative, error);
   if (!uri->database) {
      // %-decode failure
      MONGOC_URI_ERROR (error, "Invalid database specifier \"%.*s\": %s", MSTR_FMT (relative), error->message);
      return false;
   }

   // Check if the database name contains and invalid characters after the %-decode
   if (mstr_contains_any_of (mstr_cstring (uri->database), mstr_cstring ("/\\. \"$"))) {
      MONGOC_URI_ERROR (error, "Invalid database specifier \"%s\": Contains disallowed characters", uri->database);
      return false;
   }

   return true;
}


static bool
_parse_and_set_auth_mechanism_properties (mongoc_uri_t *uri, const char *str)
{
   bson_t properties = BSON_INITIALIZER;

   mstr_view remain = mstr_cstring (str);
   while (remain.len) {
      // Get the entry until the next comma
      mstr_view entry;
      mstr_split_around (remain, COMMA, &entry, &remain);
      // Split around the colon. If no colon, makes an empty value.
      mstr_view key, value;
      mstr_split_around (entry, COLON, &key, &value);
      // Accumulate properties
      bson_append_utf8 (&properties, key.data, (int) key.len, value.data, (int) value.len);
   }

   /* append our auth properties to our credentials */
   if (!mongoc_uri_set_mechanism_properties (uri, &properties)) {
      bson_destroy (&properties);
      return false;
   }
   bson_destroy (&properties);
   return true;
}


static bool
mongoc_uri_check_srv_service_name (mongoc_uri_t *uri, const char *str)
{
   /* 63 character DNS query limit, excluding prepended underscore. */
   const size_t mongoc_srv_service_name_max = 62u;

   size_t length = 0u;
   size_t num_alpha = 0u;
   size_t i = 0u;
   char prev = '\0';

   BSON_ASSERT_PARAM (uri);
   BSON_ASSERT_PARAM (str);

   length = strlen (str);

   /* Initial DNS Seedlist Discovery Spec: This option specifies a valid SRV
    * service name according to RFC 6335, with the exception that it may exceed
    * 15 characters as long as the 63rd (62nd with prepended underscore)
    * character DNS query limit is not surpassed. */
   if (length > mongoc_srv_service_name_max) {
      return false;
   }

   /* RFC 6335: MUST be at least 1 character. */
   if (length == 0u) {
      return false;
   }

   for (i = 0u; i < length; ++i) {
      const char c = str[i];

      /* RFC 6335: MUST contain only US-ASCII letters 'A' - 'Z' and 'a' - 'z',
       * digits '0' - '9', and hyphens ('-', ASCII 0x2D or decimal 45). */
      if (!isalpha (c) && !isdigit (c) && c != '-') {
         return false;
      }

      /* RFC 6335: hyphens MUST NOT be adjacent to other hyphens. */
      if (c == '-' && prev == '-') {
         return false;
      }

      num_alpha += isalpha (c) ? 1u : 0u;
      prev = c;
   }

   /* RFC 6335: MUST contain at least one letter ('A' - 'Z' or 'a' - 'z') */
   if (num_alpha == 0u) {
      return false;
   }

   /* RFC 6335: MUST NOT begin or end with a hyphen. */
   if (str[0] == '-' || str[length - 1u] == '-') {
      return false;
   }

   return true;
}

static bool
_apply_read_prefs_tags (mongoc_uri_t *uri, /* IN */
                        const char *str)   /* IN */
{
   bson_t b = BSON_INITIALIZER;
   bool okay = false;

   for (mstr_view remain = mstr_cstring (str); remain.len;) {
      mstr_view entry;
      mstr_split_around (remain, COMMA, &entry, &remain);
      mstr_view key, value;
      if (!mstr_split_around (entry, COLON, &key, &value)) {
         // The entry does not have a colon. This is invalid for tags
         MONGOC_WARNING ("Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS "\": \"%s\"", str);
         goto fail;
      }
      bson_append_utf8 (&b, key.data, (int) key.len, value.data, (int) value.len);
   }

   mongoc_read_prefs_add_tag (uri->read_prefs, &b);
   okay = true;

fail:
   bson_destroy (&b);
   return okay;
}

/**
 * @brief Remove a BSON element with the given key, case-insensitive
 *
 * @param doc The document to be updated
 * @param key The key to be removed
 */
static void
_bson_erase_icase (bson_t *doc, mstr_view key)
{
   bson_iter_t iter;
   if (!bson_iter_init (&iter, doc)) {
      return;
   }

   bson_t tmp = BSON_INITIALIZER;
   while (bson_iter_next (&iter)) {
      if (mstr_latin_casecmp (mstr_cstring (bson_iter_key (&iter)), !=, key)) {
         const bson_value_t *const bvalue = bson_iter_value (&iter);
         BSON_APPEND_VALUE (&tmp, bson_iter_key (&iter), bvalue);
      }
   }

   bson_destroy (doc);
   bson_copy_to (&tmp, doc);
   bson_destroy (&tmp);
}

/**
 * @brief Update a BSON document with a UTF-8 value, replacing it if it already
 * exists
 *
 * @param options The doc to be updated
 * @param key The case-insensitive string of the to be added/updated
 * @param value The UTF-8 string that will be inserted or removed
 *
 * @note This will case-normalize the key string to lowercase before inserting it.
 */
static void
_bson_upsert_utf8_icase (bson_t *options, mstr_view key, const char *value)
{
   _bson_erase_icase (options, key);

   // Lowercase the key, preventing the need for all callers to do this normalization
   // themselves.
   char *lower = bson_strndup (key.data, key.len);
   mongoc_lowercase_inplace (lower);
   bson_append_utf8 (options, lower, -1, value, -1);
   bson_free (lower);
}

/**
 * @brief Initialize an iterator to point to the named element, case-insensitive
 *
 * @param iter Storage for an iterator to be updated
 * @param doc The document to be searched
 * @param key The key to find, case-insensitive
 * @return true If the element was found, and `*iter` is updated
 * @return false Otherwise
 */
static inline bool
_bson_init_iter_find_icase (bson_iter_t *iter, bson_t const *doc, mstr_view key)
{
   if (!bson_iter_init (iter, doc)) {
      return false;
   }
   while (bson_iter_next (iter)) {
      if (mstr_latin_casecmp (mstr_cstring (bson_iter_key (iter)), ==, key)) {
         return true;
      }
   }
   return false;
}

bool
mongoc_uri_has_option (const mongoc_uri_t *uri, const char *key)
{
   bson_iter_t iter;
   return _bson_init_iter_find_icase (&iter, &uri->options, mstr_cstring (key));
}

bool
mongoc_uri_option_is_int32 (const char *key)
{
   return mongoc_uri_option_is_int64 (key) || !strcasecmp (key, MONGOC_URI_CONNECTTIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_HEARTBEATFREQUENCYMS) ||
          !strcasecmp (key, MONGOC_URI_SERVERSELECTIONTIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_SOCKETCHECKINTERVALMS) || !strcasecmp (key, MONGOC_URI_SOCKETTIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_LOCALTHRESHOLDMS) || !strcasecmp (key, MONGOC_URI_MAXPOOLSIZE) ||
          !strcasecmp (key, MONGOC_URI_MAXSTALENESSSECONDS) || !strcasecmp (key, MONGOC_URI_WAITQUEUETIMEOUTMS) ||
          !strcasecmp (key, MONGOC_URI_ZLIBCOMPRESSIONLEVEL) || !strcasecmp (key, MONGOC_URI_SRVMAXHOSTS);
}

bool
mongoc_uri_option_is_int64 (const char *key)
{
   return !strcasecmp (key, MONGOC_URI_WTIMEOUTMS);
}

bool
mongoc_uri_option_is_bool (const char *key)
{
   // CDRIVER-5933
   if (!strcasecmp (key, MONGOC_URI_CANONICALIZEHOSTNAME)) {
      MONGOC_WARNING (MONGOC_URI_CANONICALIZEHOSTNAME " is deprecated, use " MONGOC_URI_AUTHMECHANISMPROPERTIES
                                                      " with CANONICALIZE_HOST_NAME instead");
      return true;
   }

   return !strcasecmp (key, MONGOC_URI_DIRECTCONNECTION) || !strcasecmp (key, MONGOC_URI_JOURNAL) ||
          !strcasecmp (key, MONGOC_URI_RETRYREADS) || !strcasecmp (key, MONGOC_URI_RETRYWRITES) ||
          !strcasecmp (key, MONGOC_URI_SAFE) || !strcasecmp (key, MONGOC_URI_SERVERSELECTIONTRYONCE) ||
          !strcasecmp (key, MONGOC_URI_TLS) || !strcasecmp (key, MONGOC_URI_TLSINSECURE) ||
          !strcasecmp (key, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES) ||
          !strcasecmp (key, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES) ||
          !strcasecmp (key, MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK) ||
          !strcasecmp (key, MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK) || !strcasecmp (key, MONGOC_URI_LOADBALANCED) ||
          /* deprecated options with canonical equivalents */
          !strcasecmp (key, MONGOC_URI_SSL) || !strcasecmp (key, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES) ||
          !strcasecmp (key, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES);
}

bool
mongoc_uri_option_is_utf8 (const char *key)
{
   return !strcasecmp (key, MONGOC_URI_APPNAME) || !strcasecmp (key, MONGOC_URI_REPLICASET) ||
          !strcasecmp (key, MONGOC_URI_READPREFERENCE) || !strcasecmp (key, MONGOC_URI_SERVERMONITORINGMODE) ||
          !strcasecmp (key, MONGOC_URI_SRVSERVICENAME) || !strcasecmp (key, MONGOC_URI_TLSCERTIFICATEKEYFILE) ||
          !strcasecmp (key, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD) || !strcasecmp (key, MONGOC_URI_TLSCAFILE) ||
          /* deprecated options with canonical equivalents */
          !strcasecmp (key, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE) ||
          !strcasecmp (key, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD) ||
          !strcasecmp (key, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE);
}

const char *
mongoc_uri_canonicalize_option (const char *key)
{
   if (!strcasecmp (key, MONGOC_URI_SSL)) {
      return MONGOC_URI_TLS;
   } else if (!strcasecmp (key, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE)) {
      return MONGOC_URI_TLSCERTIFICATEKEYFILE;
   } else if (!strcasecmp (key, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD)) {
      return MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD;
   } else if (!strcasecmp (key, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE)) {
      return MONGOC_URI_TLSCAFILE;
   } else if (!strcasecmp (key, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES)) {
      return MONGOC_URI_TLSALLOWINVALIDCERTIFICATES;
   } else if (!strcasecmp (key, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES)) {
      return MONGOC_URI_TLSALLOWINVALIDHOSTNAMES;
   } else {
      return key;
   }
}

/**
 * @brief Test whether the given URI parameter is allowed to be specified in
 * a DNS record.
 *
 * @param key The parameter key string, case-insensitive
 * @return true If the option is valid in a DNS record
 * @return false Otherwise
 */
static bool
dns_option_allowed (mstr_view key)
{
   /* Initial DNS Seedlist Discovery Spec: "A Client MUST only support the
    * authSource, replicaSet, and loadBalanced options through a TXT record, and
    * MUST raise an error if any other option is encountered."
    */
   return mstr_latin_casecmp (key, ==, mstr_cstring (MONGOC_URI_AUTHSOURCE)) ||
          mstr_latin_casecmp (key, ==, mstr_cstring (MONGOC_URI_LOADBALANCED)) ||
          mstr_latin_casecmp (key, ==, mstr_cstring (MONGOC_URI_REPLICASET));
}

/**
 * @brief Apply a single query parameter to a URI from a string
 *
 * @param uri The object to be updated.
 * @param options The URI options data that will also be updated.
 * @param str The percent-encoded query string element to be decoded.
 * @param from_dns Whether this query string comes from a DNS result
 * @retval true Upon success
 * @retval false Otherwise, and sets `*error`
 */
static bool
_handle_pct_uri_query_param (mongoc_uri_t *uri, bson_t *options, mstr_view str, bool from_dns, bson_error_t *error)
{
   bson_error_reset (error);
   // The argument value, with percent-encoding removed
   char *value = NULL;
   // Whether the operation succeeded
   bool ret = false;

   mstr_view key, val_pct;
   if (!mstr_split_around (str, mstr_cstring ("="), &key, &val_pct)) {
      MONGOC_URI_ERROR (error, "URI option \"%.*s\" contains no \"=\" sign", MSTR_FMT (str));
      goto done;
   }

   /* Initial DNS Seedlist Discovery Spec: "A Client MUST only support the
    * authSource, replicaSet, and loadBalanced options through a TXT record, and
    * MUST raise an error if any other option is encountered."*/
   if (from_dns && !dns_option_allowed (key)) {
      MONGOC_URI_ERROR (error, "URI option \"%.*s\" prohibited in TXT records", MSTR_FMT (key));
      goto done;
   }

   value = _strdup_pct_decode (val_pct, error);
   if (!value) {
      /* do_unescape detected invalid UTF-8 and freed value */
      MONGOC_URI_ERROR (error, "Value for URI option \"%.*s\" contains is invalid: %s", MSTR_FMT (key), error->message);
      goto done;
   }

   /* Special case: readPreferenceTags is a composing option.
    * Multiple instances should append, not overwrite.
    * Encode them directly to the options field,
    * bypassing canonicalization and duplicate checks.
    */
   if (mstr_latin_casecmp (key, ==, mstr_cstring (MONGOC_URI_READPREFERENCETAGS))) {
      if (!_apply_read_prefs_tags (uri, value)) {
         MONGOC_URI_ERROR (error, "Unsupported value for \"%.*s\": \"%s\"", MSTR_FMT (key), value);
         goto done;
      } else {
         ret = true;
         goto done;
      }
   }

   // Handle case where the option has already been specified
   bson_iter_t iter;
   if (_bson_init_iter_find_icase (&iter, &uri->raw, key) || _bson_init_iter_find_icase (&iter, options, key)) {
      /* Special case, MONGOC_URI_W == "any non-int" is not overridden
       * by later values.
       */
      size_t opt_len;
      if (mstr_latin_casecmp (key, ==, mstr_cstring (MONGOC_URI_W)) &&
          mlib_i64_parse (mstr_cstring (bson_iter_utf8_unsafe (&iter, &opt_len)), NULL)) {
         // Value is a "w", and is not a valid integer, but we already have a valid "w"
         // value, so don't overwrite it
         ret = true;
         goto done;
      }

      /* Initial DNS Seedlist Discovery Spec: "Client MUST use options
       * specified in the Connection String to override options provided
       * through TXT records." So, do NOT override existing options with TXT
       * options. */
      if (from_dns) {
         if (mstr_latin_casecmp (key, ==, mstr_cstring (MONGOC_URI_AUTHSOURCE))) {
            // Treat `authSource` as a special case. A server may support authentication with multiple mechanisms.
            // MONGODB-X509 requires authSource=$external. SCRAM-SHA-256 requires authSource=admin.
            // Only log a trace message since this may be expected.
            TRACE ("Ignoring URI option \"%.*s\" from TXT record \"%.*s\". Option is already present in URI",
                   MSTR_FMT (key),
                   MSTR_FMT (str));
         } else {
            MONGOC_WARNING ("Ignoring URI option \"%.*s\" from TXT record \"%.*s\". Option is already present in URI",
                            MSTR_FMT (key),
                            MSTR_FMT (str));
         }
         ret = true;
         goto done;
      }
      MONGOC_WARNING ("Overwriting previously provided value for '%.*s'", MSTR_FMT (key));
   }

   // Reject replicaSet=""
   if (mstr_latin_casecmp (key, ==, mstr_cstring (MONGOC_URI_REPLICASET)) && strlen (value) == 0) {
      MONGOC_URI_ERROR (error, "Value for URI option \"%.*s\" cannot be empty string", MSTR_FMT (key));
      goto done;
   }

   _bson_upsert_utf8_icase (options, key, value);
   ret = true;

done:
   bson_free (value);

   return ret;
}


/* Check for canonical/deprecated conflicts
 * between the option list a, and b.
 * If both names exist either way with differing values, error.
 */
static bool
mongoc_uri_options_validate_names (const bson_t *a, const bson_t *b, bson_error_t *error)
{
   bson_iter_t key_iter, canon_iter;
   const char *key = NULL;
   const char *canon = NULL;
   const char *value = NULL;
   const char *cval = NULL;
   size_t value_len = 0;
   size_t cval_len = 0;

   /* Scan `a` looking for deprecated names
    * where the canonical name was also used in `a`,
    * or was used in `b`. */
   if (!bson_iter_init (&key_iter, a)) {
      return false;
   }

   while (bson_iter_next (&key_iter)) {
      key = bson_iter_key (&key_iter);
      value = bson_iter_utf8_unsafe (&key_iter, &value_len);
      canon = mongoc_uri_canonicalize_option (key);

      if (mstr_latin_casecmp (mstr_cstring (key), ==, mstr_cstring (canon))) {
         /* Canonical form, no point checking `b`. */
         continue;
      }

      /* Check for a conflict in `a`. */
      if (bson_iter_init_find (&canon_iter, a, canon)) {
         cval = bson_iter_utf8_unsafe (&canon_iter, &cval_len);
         if (mstr_cmp (mstr_cstring (cval), !=, mstr_cstring (value))) {
            goto HANDLE_CONFLICT;
         }
      }

      /* Check for a conflict in `b`. */
      if (bson_iter_init_find (&canon_iter, b, canon)) {
         cval = bson_iter_utf8_unsafe (&canon_iter, &cval_len);
         if (mstr_cmp (mstr_cstring (cval), !=, mstr_cstring (value))) {
            goto HANDLE_CONFLICT;
         }
      }
   }

   return true;

HANDLE_CONFLICT:
   MONGOC_URI_ERROR (error,
                     "Deprecated option '%s=%s' conflicts with "
                     "canonical name '%s=%s'",
                     key,
                     value,
                     canon,
                     cval);

   return false;
}


#define HANDLE_DUPE()                                                            \
   if (from_dns) {                                                               \
      MONGOC_WARNING ("Cannot override URI option \"%s\" from TXT record", key); \
      continue;                                                                  \
   } else if (1) {                                                               \
      MONGOC_WARNING ("Overwriting previously provided value for '%s'", key);    \
   } else                                                                        \
      (void) 0

static bool
mongoc_uri_apply_options (mongoc_uri_t *uri, const bson_t *options, bool from_dns, bson_error_t *error)
{
   bson_iter_t iter;
   const char *key = NULL;
   const char *canon = NULL;
   const char *value = NULL;
   size_t value_len;
   bool bval;

   if (!bson_iter_init (&iter, options)) {
      return false;
   }

   while (bson_iter_next (&iter)) {
      key = bson_iter_key (&iter);
      canon = mongoc_uri_canonicalize_option (key);
      value = bson_iter_utf8_unsafe (&iter, &value_len);

      /* Keep a record of how the option was originally presented. */
      _bson_upsert_utf8_icase (&uri->raw, mstr_cstring (key), value);

      /* This check precedes mongoc_uri_option_is_int32 as all 64-bit values are
       * also recognised as 32-bit ints.
       */
      if (mongoc_uri_option_is_int64 (key)) {
         if (0 < strlen (value)) {
            int64_t i64 = 42424242;
            if (mlib_i64_parse (mstr_cstring (value), &i64)) {
               goto UNSUPPORTED_VALUE;
            }

            if (!_mongoc_uri_set_option_as_int64_with_error (uri, canon, i64, error)) {
               return false;
            }
         } else {
            MONGOC_WARNING ("Empty value provided for \"%s\"", key);
         }
      } else if (mongoc_uri_option_is_int32 (key)) {
         if (0 < strlen (value)) {
            int32_t i32 = 42424242;
            if (mlib_i32_parse (mstr_cstring (value), &i32)) {
               goto UNSUPPORTED_VALUE;
            }

            if (!_mongoc_uri_set_option_as_int32_with_error (uri, canon, i32, error)) {
               return false;
            }
         } else {
            MONGOC_WARNING ("Empty value provided for \"%s\"", key);
         }
      } else if (!strcmp (key, MONGOC_URI_W)) {
         int32_t i32 = 42424242;
         if (!mlib_i32_parse (mstr_cstring (value), 10, &i32)) {
            // A valid integer 'w' value.
            _mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_W, i32);
         } else if (0 == strcasecmp (value, "majority")) {
            _bson_upsert_utf8_icase (&uri->options, mstr_cstring (MONGOC_URI_W), "majority");
         } else if (*value) {
            _bson_upsert_utf8_icase (&uri->options, mstr_cstring (MONGOC_URI_W), value);
         }
      } else if (mongoc_uri_option_is_bool (key)) {
         if (0 < strlen (value)) {
            if (0 == strcasecmp (value, "true")) {
               bval = true;
            } else if (0 == strcasecmp (value, "false")) {
               bval = false;
            } else if ((0 == strcmp (value, "1")) || (0 == strcasecmp (value, "yes")) ||
                       (0 == strcasecmp (value, "y")) || (0 == strcasecmp (value, "t"))) {
               MONGOC_WARNING ("Deprecated boolean value for \"%s\": \"%s\", "
                               "please update to \"%s=true\"",
                               key,
                               value,
                               key);
               bval = true;
            } else if ((0 == strcasecmp (value, "0")) || (0 == strcasecmp (value, "-1")) ||
                       (0 == strcmp (value, "no")) || (0 == strcmp (value, "n")) || (0 == strcmp (value, "f"))) {
               MONGOC_WARNING ("Deprecated boolean value for \"%s\": \"%s\", "
                               "please update to \"%s=false\"",
                               key,
                               value,
                               key);
               bval = false;
            } else {
               goto UNSUPPORTED_VALUE;
            }

            if (!mongoc_uri_set_option_as_bool (uri, canon, bval)) {
               _mongoc_set_error (
                  error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Failed to set %s to %d", canon, bval);
               return false;
            }
         } else {
            MONGOC_WARNING ("Empty value provided for \"%s\"", key);
         }

      } else if (!strcmp (key, MONGOC_URI_READPREFERENCETAGS)) {
         /* Skip this option here.
          * It was marshalled during mongoc_uri_split_option()
          * as a special case composing option.
          */

      } else if (!strcmp (key, MONGOC_URI_AUTHMECHANISM) || !strcmp (key, MONGOC_URI_AUTHSOURCE)) {
         if (bson_has_field (&uri->credentials, key)) {
            HANDLE_DUPE ();
         }
         _bson_upsert_utf8_icase (&uri->credentials, mstr_cstring (canon), value);

      } else if (!strcmp (key, MONGOC_URI_READCONCERNLEVEL)) {
         if (!mongoc_read_concern_is_default (uri->read_concern)) {
            HANDLE_DUPE ();
         }
         mongoc_read_concern_set_level (uri->read_concern, value);

      } else if (!strcmp (key, MONGOC_URI_GSSAPISERVICENAME)) {
         char *tmp = bson_strdup_printf ("SERVICE_NAME:%s", value);
         if (bson_has_field (&uri->credentials, MONGOC_URI_AUTHMECHANISMPROPERTIES)) {
            MONGOC_WARNING ("authMechanismProperties SERVICE_NAME already set, "
                            "ignoring '%s'",
                            key);
         } else {
            // CDRIVER-5933
            MONGOC_WARNING (MONGOC_URI_GSSAPISERVICENAME " is deprecated, use " MONGOC_URI_AUTHMECHANISMPROPERTIES
                                                         " with SERVICE_NAME instead");

            if (!_parse_and_set_auth_mechanism_properties (uri, tmp)) {
               bson_free (tmp);
               goto UNSUPPORTED_VALUE;
            }
         }
         bson_free (tmp);

      } else if (!strcmp (key, MONGOC_URI_SRVSERVICENAME)) {
         if (!mongoc_uri_check_srv_service_name (uri, value)) {
            goto UNSUPPORTED_VALUE;
         }
         _bson_upsert_utf8_icase (&uri->options, mstr_cstring (canon), value);

      } else if (!strcmp (key, MONGOC_URI_AUTHMECHANISMPROPERTIES)) {
         if (bson_has_field (&uri->credentials, key)) {
            HANDLE_DUPE ();
         }
         if (!_parse_and_set_auth_mechanism_properties (uri, value)) {
            goto UNSUPPORTED_VALUE;
         }

      } else if (!strcmp (key, MONGOC_URI_APPNAME)) {
         /* Part of uri->options */
         if (!mongoc_uri_set_appname (uri, value)) {
            goto UNSUPPORTED_VALUE;
         }

      } else if (!strcmp (key, MONGOC_URI_COMPRESSORS)) {
         if (!bson_empty (mongoc_uri_get_compressors (uri))) {
            HANDLE_DUPE ();
         }
         if (!mongoc_uri_set_compressors (uri, value)) {
            goto UNSUPPORTED_VALUE;
         }

      } else if (!strcmp (key, MONGOC_URI_SERVERMONITORINGMODE)) {
         if (!mongoc_uri_set_server_monitoring_mode (uri, value)) {
            goto UNSUPPORTED_VALUE;
         }

      } else if (mongoc_uri_option_is_utf8 (key)) {
         _bson_upsert_utf8_icase (&uri->options, mstr_cstring (canon), value);

      } else {
         /*
          * Keys that aren't supported by a driver MUST be ignored.
          *
          * A WARN level logging message MUST be issued
          * https://github.com/mongodb/specifications/blob/master/source/connection-string/connection-string-spec.md#keys
          */
         MONGOC_WARNING ("Unsupported URI option \"%s\"", key);
      }
   }

   return true;

UNSUPPORTED_VALUE:
   MONGOC_URI_ERROR (error, "Unsupported value for \"%s\": \"%s\"", key, value);

   return false;
}


/* Processes a query string formatted set of driver options
 * (i.e. tls=true&connectTimeoutMS=250 ) into a BSON dict of values.
 * uri->raw is initially populated with the raw split of key/value pairs,
 * then the keys are canonicalized and the values coerced
 * to their appropriate type and stored in uri->options.
 */
bool
_mongoc_uri_apply_query_string (mongoc_uri_t *uri, mstr_view remain, bool from_dns, bson_error_t *error)
{
   bson_t options = BSON_INITIALIZER;
   for (; remain.len;) {
      mstr_view entry;
      mstr_split_around (remain, mstr_cstring ("&"), &entry, &remain);
      if (!_handle_pct_uri_query_param (uri, &options, entry, from_dns, error)) {
         bson_destroy (&options);
         return false;
      }
   }

   /* Walk both sides of this map to handle each ordering:
    * deprecated first canonical later, and vice-versa.
    * Then finalize parse by writing final values to uri->options.
    */
   if (!mongoc_uri_options_validate_names (&uri->options, &options, error) ||
       !mongoc_uri_options_validate_names (&options, &uri->options, error) ||
       !mongoc_uri_apply_options (uri, &options, from_dns, error)) {
      bson_destroy (&options);
      return false;
   }

   bson_destroy (&options);
   return true;
}


static bool
mongoc_uri_finalize_tls (mongoc_uri_t *uri, bson_error_t *error)
{
   /* Initial DNS Seedlist Discovery Spec: "If mongodb+srv is used, a driver
    * MUST implicitly also enable TLS." */
   if (uri->is_srv && !bson_has_field (&uri->options, MONGOC_URI_TLS)) {
      mongoc_uri_set_option_as_bool (uri, MONGOC_URI_TLS, true);
   }

   /* tlsInsecure implies tlsAllowInvalidCertificates, tlsAllowInvalidHostnames,
    * tlsDisableOCSPEndpointCheck, and tlsDisableCertificateRevocationCheck, so
    * consider it an error to have both. The user might have the wrong idea. */
   if (bson_has_field (&uri->options, MONGOC_URI_TLSINSECURE) &&
       (bson_has_field (&uri->options, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES) ||
        bson_has_field (&uri->options, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES) ||
        bson_has_field (&uri->options, MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK) ||
        bson_has_field (&uri->options, MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK))) {
      MONGOC_URI_ERROR (error,
                        "%s may not be specified with %s, %s, %s, or %s",
                        MONGOC_URI_TLSINSECURE,
                        MONGOC_URI_TLSALLOWINVALIDCERTIFICATES,
                        MONGOC_URI_TLSALLOWINVALIDHOSTNAMES,
                        MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK,
                        MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK);
      return false;
   }

   /* tlsAllowInvalidCertificates implies tlsDisableOCSPEndpointCheck and
    * tlsDisableCertificateRevocationCheck, so consider it an error to have
    * both. The user might have the wrong idea. */
   if (bson_has_field (&uri->options, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES) &&
       (bson_has_field (&uri->options, MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK) ||
        bson_has_field (&uri->options, MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK))) {
      MONGOC_URI_ERROR (error,
                        "%s may not be specified with %s or %s",
                        MONGOC_URI_TLSALLOWINVALIDCERTIFICATES,
                        MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK,
                        MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK);
      return false;
   }

   /*  tlsDisableCertificateRevocationCheck implies tlsDisableOCSPEndpointCheck,
    * so consider it an error to have both. The user might have the wrong idea.
    */
   if (bson_has_field (&uri->options, MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK) &&
       bson_has_field (&uri->options, MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK)) {
      MONGOC_URI_ERROR (error,
                        "%s may not be specified with %s",
                        MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK,
                        MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK);
      return false;
   }

   return true;
}


typedef enum _mongoc_uri_finalize_validate {
   _mongoc_uri_finalize_allowed,
   _mongoc_uri_finalize_required,
   _mongoc_uri_finalize_prohibited,
} mongoc_uri_finalize_validate;


static bool
_finalize_auth_username (const char *username,
                         const char *mechanism,
                         mongoc_uri_finalize_validate validate,
                         bson_error_t *error)
{
   BSON_OPTIONAL_PARAM (username);
   BSON_ASSERT_PARAM (mechanism);
   BSON_OPTIONAL_PARAM (error);

   switch (validate) {
   case _mongoc_uri_finalize_required:
      if (!username || strlen (username) == 0u) {
         MONGOC_URI_ERROR (error, "'%s' authentication mechanism requires a username", mechanism);
         return false;
      }
      break;

   case _mongoc_uri_finalize_prohibited:
      if (username) {
         MONGOC_URI_ERROR (error, "'%s' authentication mechanism does not accept a username", mechanism);
         return false;
      }
      break;

   case _mongoc_uri_finalize_allowed:
   default:
      if (username && strlen (username) == 0u) {
         MONGOC_URI_ERROR (error, "'%s' authentication mechanism requires a non-empty username", mechanism);
         return false;
      }
      break;
   }

   return true;
}

// source MUST be "$external"
static bool
_finalize_auth_source_external (const char *source, const char *mechanism, bson_error_t *error)
{
   BSON_OPTIONAL_PARAM (source);
   BSON_ASSERT_PARAM (mechanism);
   BSON_OPTIONAL_PARAM (error);

   if (source && strcasecmp (source, "$external") != 0) {
      MONGOC_URI_ERROR (error,
                        "'%s' authentication mechanism requires \"$external\" authSource, but \"%s\" was specified",
                        mechanism,
                        source);
      return false;
   }

   return true;
}

// source MUST be "$external" and defaults to "$external".
static bool
_finalize_auth_source_default_external (mongoc_uri_t *uri,
                                        const char *source,
                                        const char *mechanism,
                                        bson_error_t *error)
{
   BSON_ASSERT_PARAM (uri);
   BSON_OPTIONAL_PARAM (source);
   BSON_ASSERT_PARAM (mechanism);
   BSON_OPTIONAL_PARAM (error);

   if (!source) {
      bsonBuildAppend (uri->credentials, kv (MONGOC_URI_AUTHSOURCE, cstr ("$external")));
      if (bsonBuildError) {
         MONGOC_URI_ERROR (error,
                           "unexpected URI credentials BSON error when attempting to default '%s' "
                           "authentication source to '$external': %s",
                           mechanism,
                           bsonBuildError);
         return false;
      }
      return true;
   } else {
      return _finalize_auth_source_external (source, mechanism, error);
   }
}

static bool
_finalize_auth_password (const char *password,
                         const char *mechanism,
                         mongoc_uri_finalize_validate validate,
                         bson_error_t *error)
{
   BSON_OPTIONAL_PARAM (password);
   BSON_ASSERT_PARAM (mechanism);
   BSON_OPTIONAL_PARAM (error);

   switch (validate) {
   case _mongoc_uri_finalize_required:
      // Passwords may be zero length.
      if (!password) {
         MONGOC_URI_ERROR (error, "'%s' authentication mechanism requires a password", mechanism);
         return false;
      }
      break;

   case _mongoc_uri_finalize_prohibited:
      if (password) {
         MONGOC_URI_ERROR (error, "'%s' authentication mechanism does not accept a password", mechanism);
         return false;
      }
      break;

   case _mongoc_uri_finalize_allowed:
   default:
      break;
   }

   return true;
}

typedef struct __supported_mechanism_properties {
   const char *name;
   bson_type_t type;
} supported_mechanism_properties;

static bool
_supported_mechanism_properties_check (const supported_mechanism_properties *supported_properties,
                                       const bson_t *mechanism_properties,
                                       const char *mechanism,
                                       bson_error_t *error)
{
   BSON_ASSERT_PARAM (supported_properties);
   BSON_ASSERT_PARAM (mechanism_properties);
   BSON_ASSERT_PARAM (mechanism);
   BSON_ASSERT_PARAM (error);

   bson_iter_t iter;
   BSON_ASSERT (bson_iter_init (&iter, mechanism_properties));

   // For each element in `MongoCredential.mechanism_properties`...
   while (bson_iter_next (&iter)) {
      const char *const key = bson_iter_key (&iter);

      // ... ensure it matches one of the supported mechanism property fields.
      for (const supported_mechanism_properties *prop = supported_properties; prop->name; ++prop) {
         // Authentication spec: naming of mechanism properties MUST be case-insensitive. For instance, SERVICE_NAME and
         // service_name refer to the same property.
         if (strcasecmp (key, prop->name) == 0) {
            const bson_type_t type = bson_iter_type (&iter);

            if (type == prop->type) {
               goto found_match; // Matches both key and type.
            } else {
               // Authentication spec: Drivers SHOULD raise an error as early as possible when detecting invalid values
               // in a credential. For instance, if a mechanism_property is specified for MONGODB-CR, the driver should
               // raise an error indicating that the property does not apply.
               //
               // Note: this overrides the Connection String spec: Any invalid Values for a given key MUST be ignored
               // and MUST log a WARN level message.
               MONGOC_URI_ERROR (error,
                                 "'%s' authentication mechanism property '%s' has incorrect type '%s', should be '%s'",
                                 key,
                                 mechanism,
                                 _mongoc_bson_type_to_str (type),
                                 _mongoc_bson_type_to_str (prop->type));
               return false;
            }
         }
      }

      // Authentication spec: Drivers SHOULD raise an error as early as possible when detecting invalid values in a
      // credential. For instance, if a mechanism_property is specified for MONGODB-CR, the driver should raise an error
      // indicating that the property does not apply.
      //
      // Note: this overrides the Connection String spec: Any invalid Values for a given key MUST be ignored and MUST
      // log a WARN level message.
      MONGOC_URI_ERROR (error, "Unsupported '%s' authentication mechanism property: '%s'", mechanism, key);
      return false;

   found_match:
      continue;
   }

   return true;
}

static bool
_finalize_auth_gssapi_mechanism_properties (const bson_t *mechanism_properties, bson_error_t *error)
{
   BSON_OPTIONAL_PARAM (mechanism_properties);
   BSON_ASSERT_PARAM (error);

   static const supported_mechanism_properties supported_properties[] = {
      {"SERVICE_NAME", BSON_TYPE_UTF8},
      {"CANONICALIZE_HOST_NAME", BSON_TYPE_UTF8}, // CDRIVER-4128: UTF-8 even when "false" or "true".
      {"SERVICE_REALM", BSON_TYPE_UTF8},
      {"SERVICE_HOST", BSON_TYPE_UTF8},
      {0},
   };

   if (mechanism_properties) {
      return _supported_mechanism_properties_check (supported_properties, mechanism_properties, "GSSAPI", error);
   }

   return true;
}

static bool
_finalize_auth_aws_mechanism_properties (const bson_t *mechanism_properties, bson_error_t *error)
{
   BSON_OPTIONAL_PARAM (mechanism_properties);
   BSON_ASSERT_PARAM (error);

   static const supported_mechanism_properties supported_properties[] = {
      {"AWS_SESSION_TOKEN", BSON_TYPE_UTF8},
      {0},
   };

   if (mechanism_properties) {
      return _supported_mechanism_properties_check (supported_properties, mechanism_properties, "MONGODB-AWS", error);
   }

   return true;
}

static bool
_finalize_auth_oidc_mechanism_properties (const bson_t *mechanism_properties, bson_error_t *error)
{
   BSON_OPTIONAL_PARAM (mechanism_properties);
   BSON_ASSERT_PARAM (error);

   static const supported_mechanism_properties supported_properties[] = {
      {"ENVIRONMENT", BSON_TYPE_UTF8},
      {"TOKEN_RESOURCE", BSON_TYPE_UTF8},
      {0},
   };

   if (mechanism_properties) {
      return _supported_mechanism_properties_check (supported_properties, mechanism_properties, "MONGODB-OIDC", error);
   }

   return true;
}

static bool
mongoc_uri_finalize_auth (mongoc_uri_t *uri, bson_error_t *error)
{
   BSON_ASSERT_PARAM (uri);
   BSON_OPTIONAL_PARAM (error);

   // Most validation of MongoCredential fields below according to the Authentication spec must be deferred to the
   // implementation of the Authentication Handshake algorithm (i.e. `_mongoc_cluster_auth_node`) due to support for
   // partial and late setting of credential fields via `mongoc_uri_set_*` functions. Limit validation to requirements
   // for individual field which are explicitly specified. Do not validate requirements on fields in relation to one
   // another (e.g. "given field A, field B must..."). The username, password, and authSource credential fields are
   // exceptions to this rule for both backward compatibility and spec test compliance.

   bool ret = false;

   bson_iter_t iter;

   const char *const mechanism = mongoc_uri_get_auth_mechanism (uri);
   const char *const username = mongoc_uri_get_username (uri);
   const char *const password = mongoc_uri_get_password (uri);
   const char *const source =
      bson_iter_init_find_case (&iter, &uri->credentials, MONGOC_URI_AUTHSOURCE) ? bson_iter_utf8 (&iter, NULL) : NULL;

   // Satisfy Connection String spec test: "must raise an error when the authSource is empty".
   // This applies even before determining whether or not authentication is required.
   if (source && strlen (source) == 0) {
      MONGOC_URI_ERROR (error, "%s", "authSource may not be specified as an empty string");
      return false;
   }

   // Authentication spec: The presence of a credential delimiter (i.e. '@') in the URI connection string is
   // evidence that the user has unambiguously specified user information and MUST be interpreted as a user
   // configuring authentication credentials (even if the username and/or password are empty strings).
   //
   // Note: username is always set when the credential delimiter `@` is present in the URI as parsed by
   // `mongoc_uri_parse_userpass`.
   //
   // If neither an authentication mechanism nor a username is provided, there is nothing to do.
   if (!mechanism && !username) {
      return true;
   } else {
      // All code below assumes authentication credentials are being configured.
   }

   bson_t *mechanism_properties = NULL;
   bson_t mechanism_properties_owner;
   {
      bson_t tmp;
      if (mongoc_uri_get_mechanism_properties (uri, &tmp)) {
         bson_copy_to (&tmp, &mechanism_properties_owner); // Avoid invalidation by updates to `uri->credentials`.
         mechanism_properties = &mechanism_properties_owner;
      } else {
         bson_init (&mechanism_properties_owner); // Ensure initialization.
      }
   }

   // Default authentication method.
   if (!mechanism) {
      // The authentication mechanism will be derived by `_mongoc_cluster_auth_node` during handshake according to
      // `saslSupportedMechs`.

      // Authentication spec: username: MUST be specified and non-zero length.
      // Default authentication method is used when no mechanism is specified but a username is present; see the
      // `!mechanism && !username` check above.
      if (!_finalize_auth_username (username, "default", _mongoc_uri_finalize_required, error)) {
         goto fail;
      }

      // Defer remaining validation of `MongoCredential` fields to Authentication Handshake.
   }

   // SCRAM-SHA-1, SCRAM-SHA-256, and PLAIN (same validation requirements)
   else if (strcasecmp (mechanism, "SCRAM-SHA-1") == 0 || strcasecmp (mechanism, "SCRAM-SHA-256") == 0 ||
            strcasecmp (mechanism, "PLAIN") == 0) {
      // Authentication spec: username: MUST be specified and non-zero length.
      if (!_finalize_auth_username (username, mechanism, _mongoc_uri_finalize_required, error)) {
         goto fail;
      }

      // Authentication spec: password: MUST be specified.
      if (!_finalize_auth_password (password, mechanism, _mongoc_uri_finalize_required, error)) {
         goto fail;
      }

      // Defer remaining validation of `MongoCredential` fields to Authentication Handshake.
   }

   // MONGODB-X509
   else if (strcasecmp (mechanism, "MONGODB-X509") == 0) {
      // `MongoCredential.username` SHOULD NOT be provided for MongoDB 3.4 and newer.
      // CDRIVER-1959: allow for backward compatibility until the spec states "MUST NOT" instead of "SHOULD NOT" and
      // spec tests are updated accordingly to permit warnings or errors.
      if (!_finalize_auth_username (username, mechanism, _mongoc_uri_finalize_allowed, error)) {
         goto fail;
      }

      // Authentication spec: password: MUST NOT be specified.
      if (!_finalize_auth_password (password, mechanism, _mongoc_uri_finalize_prohibited, error)) {
         goto fail;
      }

      // Authentication spec: source: MUST be "$external" and defaults to "$external".
      if (!_finalize_auth_source_default_external (uri, source, mechanism, error)) {
         goto fail;
      }

      // Defer remaining validation of `MongoCredential` fields to Authentication Handshake.
   }

   // GSSAPI
   else if (strcasecmp (mechanism, "GSSAPI") == 0) {
      // Authentication spec: username: MUST be specified and non-zero length.
      if (!_finalize_auth_username (username, mechanism, _mongoc_uri_finalize_required, error)) {
         goto fail;
      }

      // Authentication spec: source: MUST be "$external" and defaults to "$external".
      if (!_finalize_auth_source_default_external (uri, source, mechanism, error)) {
         goto fail;
      }

      // Authentication spec: password: MAY be specified.
      if (!_finalize_auth_password (password, mechanism, _mongoc_uri_finalize_allowed, error)) {
         goto fail;
      }

      // `MongoCredentials.mechanism_properties` are allowed for GSSAPI.
      if (!_finalize_auth_gssapi_mechanism_properties (mechanism_properties, error)) {
         goto fail;
      }

      // Authentication spec: valid values for CANONICALIZE_HOST_NAME are true, false, "none", "forward",
      // "forwardAndReverse". If a value is provided that does not match one of these the driver MUST raise an error.
      if (mechanism_properties) {
         bsonParse (*mechanism_properties,
                    find (iKeyWithType ("CANONICALIZE_HOST_NAME", utf8),
                          case (when (iStrEqual ("true"), nop),
                                when (iStrEqual ("false"), nop),
                                // CDRIVER-4128: only legacy boolean values are currently supported.
                                else (do ({
                                   bsonParseError =
                                      "'GSSAPI' authentication mechanism requires CANONICALIZE_HOST_NAME is either "
                                      "\"true\" or \"false\"";
                                })))));
         if (bsonParseError) {
            MONGOC_URI_ERROR (error, "%s", bsonParseError);
            goto fail;
         }
      }

      // Authentication spec: Drivers MUST allow the user to specify a different service name. The default is
      // "mongodb".
      if (!mechanism_properties || !bson_iter_init_find_case (&iter, mechanism_properties, "SERVICE_NAME")) {
         bsonBuildDecl (props,
                        if (mechanism_properties, then (insert (*mechanism_properties, always))),
                        kv ("SERVICE_NAME", cstr ("mongodb")));
         const bool success = !bsonBuildError && mongoc_uri_set_mechanism_properties (uri, &props);
         bson_destroy (&props);
         if (!success) {
            MONGOC_URI_ERROR (error,
                              "unexpected URI credentials BSON error when attempting to default 'GSSAPI' "
                              "authentication mechanism property 'SERVICE_NAME' to 'mongodb': %s",
                              bsonBuildError ? bsonBuildError : "mongoc_uri_set_mechanism_properties failed");
            goto fail;
         }
      }

      // Defer remaining validation of `MongoCredential` fields to Authentication Handshake.
   }

   // MONGODB-AWS
   else if (strcasecmp (mechanism, "MONGODB-AWS") == 0) {
      // Authentication spec: username: MAY be specified (as the non-sensitive AWS access key).
      if (!_finalize_auth_username (username, mechanism, _mongoc_uri_finalize_allowed, error)) {
         goto fail;
      }

      // Authentication spec: source: MUST be "$external" and defaults to "$external".
      if (!_finalize_auth_source_default_external (uri, source, mechanism, error)) {
         goto fail;
      }

      // Authentication spec: password: MAY be specified (as the sensitive AWS secret key).
      if (!_finalize_auth_password (password, mechanism, _mongoc_uri_finalize_allowed, error)) {
         goto fail;
      }

      // mechanism_properties are allowed for MONGODB-AWS.
      if (!_finalize_auth_aws_mechanism_properties (mechanism_properties, error)) {
         goto fail;
      }

      // Authentication spec: if a username is provided without a password (or vice-versa), Drivers MUST raise an error.
      if (!username != !password) {
         MONGOC_URI_ERROR (error,
                           "'%s' authentication mechanism does not accept a username or a password without the other",
                           mechanism);
         goto fail;
      }

      // Defer remaining validation of `MongoCredential` fields to Authentication Handshake.
   }

   // MONGODB-OIDC
   else if (strcasecmp (mechanism, "MONGODB-OIDC") == 0) {
      // Authentication spec: username: MAY be specified (with callback/environment defined meaning).
      if (!_finalize_auth_username (username, mechanism, _mongoc_uri_finalize_allowed, error)) {
         goto fail;
      }

      // Authentication spec: source: MUST be "$external" and defaults to "$external".
      if (!_finalize_auth_source_default_external (uri, source, mechanism, error)) {
         goto fail;
      }

      // Authentication spec: password: MUST NOT be specified.
      if (!_finalize_auth_password (password, mechanism, _mongoc_uri_finalize_prohibited, error)) {
         goto fail;
      }

      // mechanism_properties are allowed for MONGODB-OIDC.
      if (!_finalize_auth_oidc_mechanism_properties (mechanism_properties, error)) {
         goto fail;
      }

      // The environment is optional, but if specified it must appear valid.
      if (mechanism_properties && bson_iter_init_find_case (&iter, mechanism_properties, "ENVIRONMENT")) {
         if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
            MONGOC_URI_ERROR (error, "'%s' authentication has non-string %s property", mechanism, "ENVIRONMENT");
            goto fail;
         }

         const mongoc_oidc_env_t *env = mongoc_oidc_env_find (bson_iter_utf8 (&iter, NULL));
         if (!env) {
            MONGOC_URI_ERROR (error,
                              "'%s' authentication has unrecognized %s property '%s'",
                              mechanism,
                              "ENVIRONMENT",
                              bson_iter_utf8 (&iter, NULL));
            goto fail;
         }

         if (username && !mongoc_oidc_env_supports_username (env)) {
            MONGOC_URI_ERROR (error,
                              "'%s' authentication with %s environment does not accept a %s",
                              mechanism,
                              mongoc_oidc_env_name (env),
                              "username");
            goto fail;
         }

         if (bson_iter_init_find_case (&iter, mechanism_properties, "TOKEN_RESOURCE")) {
            if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
               MONGOC_URI_ERROR (error, "'%s' authentication has non-string %s property", mechanism, "TOKEN_RESOURCE");
               goto fail;
            }

            if (!mongoc_oidc_env_requires_token_resource (env)) {
               MONGOC_URI_ERROR (error,
                                 "'%s' authentication with %s environment does not accept a %s",
                                 mechanism,
                                 mongoc_oidc_env_name (env),
                                 "TOKEN_RESOURCE");
               goto fail;
            }
         } else {
            if (mongoc_oidc_env_requires_token_resource (env)) {
               MONGOC_URI_ERROR (error,
                                 "'%s' authentication with %s environment requires a %s",
                                 mechanism,
                                 mongoc_oidc_env_name (env),
                                 "TOKEN_RESOURCE");
               goto fail;
            }
         }
      }

      // Defer remaining validation of `MongoCredential` fields to Authentication Handshake.
   }

   // Invalid or unsupported authentication mechanism.
   else {
      MONGOC_URI_ERROR (
         error,
         "Unsupported value for authMechanism '%s': must be one of "
         "['MONGODB-OIDC', 'SCRAM-SHA-1', 'SCRAM-SHA-256', 'PLAIN', 'MONGODB-X509', 'GSSAPI', 'MONGODB-AWS']",
         mechanism);
      goto fail;
   }

   ret = true;

fail:
   bson_destroy (&mechanism_properties_owner);

   return ret;
}

static bool
mongoc_uri_finalize_directconnection (mongoc_uri_t *uri, bson_error_t *error)
{
   bool directconnection = false;

   directconnection = mongoc_uri_get_option_as_bool (uri, MONGOC_URI_DIRECTCONNECTION, false);
   if (!directconnection) {
      return true;
   }

   /* URI options spec: "The driver MUST report an error if the
    * directConnection=true URI option is specified with an SRV URI, because
    * the URI may resolve to multiple hosts. The driver MUST allow specifying
    * directConnection=false URI option with an SRV URI." */
   if (uri->is_srv) {
      MONGOC_URI_ERROR (error, "%s", "SRV URI not allowed with directConnection option");
      return false;
   }

   /* URI options spec: "The driver MUST report an error if the
    * directConnection=true URI option is specified with multiple seeds." */
   if (uri->hosts && uri->hosts->next) {
      MONGOC_URI_ERROR (error, "%s", "Multiple seeds not allowed with directConnection option");
      return false;
   }

   return true;
}

/**
 * @brief Parse the authority component of the URI string. This is the part following
 * "://" until the path or query
 *
 * @param uri The URI to be updated
 * @param authority The full authority string to be parsed
 */
static bool
_parse_authority (mongoc_uri_t *uri, const mstr_view authority, bson_error_t *error)
{
   // Split around "@" if there is a userinfo
   mstr_view userinfo, hostinfo;
   if (mstr_split_around (authority, AT, &userinfo, &hostinfo)) {
      // We have userinfo. Parse that first
      if (!_uri_parse_userinfo (uri, userinfo, error)) {
         // Fail to parse userinfo. Fail the full authority.
         return false;
      }
      // `hostinfo` now contains the authority part following the first "@"
   } else {
      // No userinfo. The hostinfo is the entire string
      hostinfo = authority;
   }

   // Don't allow the host list to start with "@"
   if (mstr_starts_with (hostinfo, AT)) {
      /* special case: "mongodb://alice@@localhost" */
      MONGOC_URI_ERROR (error, "Invalid username or password. %s", escape_instructions);
      return false;
   }

   if (uri->is_srv) {
      // Parse as an SRV URI
      if (!_parse_srv_hostname (uri, hostinfo, error)) {
         return false;
      }
   } else {
      // Parse a comma-separated host list
      if (!_parse_hosts_csv (uri, hostinfo, error)) {
         return false;
      }
   }

   return true;
}

/**
 * @brief The elements of a decomposed URI string
 *
 * This isn't strictly conformant to any WWW spec, because our URI strings are weird,
 * but the URI components correspond to the same elements in a normal URL or URI
 *
 * Note: We do not include a URI fragment in our string parsing.
 */
typedef struct {
   /// The scheme of the URI, which precedes the "://" substring
   mstr_view scheme;
   /// The authority element, which includes the userinfo and the host specifier(s)
   mstr_view authority;
   /// The userinfo for the URI. If the URI has no userinfo, this is null
   mstr_view userinfo;
   /// The host specifier in the URI
   mstr_view hosts;
   /// The path string, including the leading "/"
   mstr_view path;
   /// The query string, including the leading "?"
   mstr_view query;
} uri_parts;

/**
 * @brief Decompose a URI string into its constituent components
 *
 * @param components Pointer to struct that receives each URI component
 * @param uri The URI string that is being inspected
 * @return true If the decomposition was successful
 * @return false Otherwise
 *
 * This does not allocate any memory or update an data related to `mongoc_uri_t`,
 * it is purely a parsing operation. The string views attached to `*components`
 * are views within the `uri` string.
 *
 * This function does not handle percent-encoding of elements.
 */
static bool
_decompose_uri_string (uri_parts *parts, mstr_view const uri, bson_error_t *error)
{
   BSON_ASSERT_PARAM (parts);

   // Clear out
   *parts = (uri_parts) {{0}};

   // Check that the URI string is valid UTF-8, otherwise we'll refuse to parse it
   if (!bson_utf8_validate (uri.data, uri.len, false /* allow_null */)) {
      MONGOC_URI_ERROR (error, "%s", "Invalid UTF-8 in URI");
      return false;
   }

   // Trim down the string as we read from left to right
   mstr_view remain = uri;

   // * remain = "foo://bar@baz:1234/path?query"
   // Grab the scheme, which is the part preceding "://"
   if (!mstr_split_around (remain, mstr_cstring ("://"), &parts->scheme, &remain)) {
      MONGOC_URI_ERROR (error, "%s", "Invalid URI, no scheme part specified");
      return false;
   }

   // * remain = "bar@baz:1234/path?query"
   // Only ':' is permitted among RFC-3986 gen-delims (":/?#[]@") in userinfo.
   // However, continue supporting these characters for backward compatibility, as permitted by the Connection
   // String spec: for backwards-compatibility reasons, drivers MAY allow reserved characters other than "@" and
   // ":" to be present in user information without percent-encoding.
   // To handle this, we will start scanning for the authority terminator beginning
   // after a possible "@" symbol in the URI. If no "@" is present, we don't need to
   // do anything different.
   {
      size_t userinfo_end_pos = mstr_find (remain, AT);
      if (userinfo_end_pos == SIZE_MAX) {
         // There is no userinfo, so we don't need to do anything special
         userinfo_end_pos = 0;
      }
      // Find the position of the first character that terminates the authority element
      const size_t term_pos = mstr_find_first_of (remain, mstr_cstring ("/?"), userinfo_end_pos);
      mstr_split_at (remain, term_pos, &parts->authority, &remain);

      // Now we should split the authority between the userinfo and the hosts
      {
         const size_t at_pos = mstr_find (parts->authority, AT);
         if (at_pos != SIZE_MAX) {
            // We have a userinfo component
            mstr_split_at (parts->authority, at_pos, 1, &parts->userinfo, &parts->hosts);
         } else {
            // We have no userinfo, so the authority string is just the host list
            parts->hosts = parts->authority;
         }
      }
   }

   // * remain = "/path?query" (Each following component is optional, but this is the proper order)
   const size_t path_end_pos = mstr_find_first_of (remain, mstr_cstring ("?"));
   mstr_split_at (remain, path_end_pos, &parts->path, &remain);
   // * remain = "?query"
   parts->query = remain;
   return true;
}

/**
 * @brief Parse the given URI C string into the URI structure
 *
 * @param uri Pointer to an initialized empty URI object to be updated
 * @param str Pointer to a C string for the URI string itself
 * @return true If the parse operation is successful, and `*uri` is updated
 * @return false Otherwise, and `*uri` contents are unspecified
 */
static bool
mongoc_uri_parse (mongoc_uri_t *uri, const char *str, bson_error_t *error)
{
   BSON_ASSERT_PARAM (uri);
   BSON_ASSERT_PARAM (str);

   // Split the URI into its parts
   mstr_view remain = mstr_cstring (str);
   uri_parts parts;
   if (!_decompose_uri_string (&parts, remain, error)) {
      return false;
   }

   // Detect whether we are a "mongodb" or "mongodb+srv" URI
   if (mstr_cmp (parts.scheme, ==, mstr_cstring ("mongodb"))) {
      uri->is_srv = false;
   } else if (mstr_cmp (parts.scheme, ==, mstr_cstring ("mongodb+srv"))) {
      uri->is_srv = true;
   } else {
      MONGOC_URI_ERROR (error,
                        "Invalid URI scheme \"%.*s://\". Expected one of \"mongodb://\" or \"mongodb+srv://\"",
                        MSTR_FMT (parts.scheme));
      return false;
   }

   // Handle the authority, including the userinfo and host specifier(s)
   if (!_parse_authority (uri, parts.authority, error)) {
      return false;
   }

   // If we have a path, parse that as the auth database
   if (!_parse_path (uri, parts.path, error)) {
      return false;
   }

   // If we have a query, parse that as the URI settings
   if (parts.query.len &&
       !_mongoc_uri_apply_query_string (uri, mstr_substr (parts.query, 1), false /* from DNS */, error)) {
      return false;
   }

   return mongoc_uri_finalize (uri, error);
}


const mongoc_host_list_t *
mongoc_uri_get_hosts (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return uri->hosts;
}


const char *
mongoc_uri_get_replica_set (const mongoc_uri_t *uri)
{
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_REPLICASET) && BSON_ITER_HOLDS_UTF8 (&iter)) {
      return bson_iter_utf8 (&iter, NULL);
   }

   return NULL;
}


const bson_t *
mongoc_uri_get_credentials (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return &uri->credentials;
}


const char *
mongoc_uri_get_auth_mechanism (const mongoc_uri_t *uri)
{
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (bson_iter_init_find_case (&iter, &uri->credentials, MONGOC_URI_AUTHMECHANISM) && BSON_ITER_HOLDS_UTF8 (&iter)) {
      return bson_iter_utf8 (&iter, NULL);
   }

   return NULL;
}


bool
mongoc_uri_set_auth_mechanism (mongoc_uri_t *uri, const char *value)
{
   size_t len;

   BSON_ASSERT (value);

   len = strlen (value);

   if (!bson_utf8_validate (value, len, false)) {
      return false;
   }

   _bson_upsert_utf8_icase (&uri->credentials, mstr_cstring (MONGOC_URI_AUTHMECHANISM), value);

   return true;
}


bool
mongoc_uri_get_mechanism_properties (const mongoc_uri_t *uri, bson_t *properties /* OUT */)
{
   bson_iter_t iter;

   BSON_ASSERT (uri);
   BSON_ASSERT (properties);

   if (bson_iter_init_find_case (&iter, &uri->credentials, MONGOC_URI_AUTHMECHANISMPROPERTIES) &&
       BSON_ITER_HOLDS_DOCUMENT (&iter)) {
      uint32_t len = 0;
      const uint8_t *data = NULL;

      bson_iter_document (&iter, &len, &data);
      BSON_ASSERT (bson_init_static (properties, data, len));

      return true;
   }

   return false;
}


bool
mongoc_uri_set_mechanism_properties (mongoc_uri_t *uri, const bson_t *properties)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (properties);

   bson_t tmp = BSON_INITIALIZER;
   bsonBuildAppend (tmp,
                    // Copy the existing credentials, dropping the existing properties if
                    // present
                    insert (uri->credentials, not (key (MONGOC_URI_AUTHMECHANISMPROPERTIES))),
                    // Append the new properties
                    kv (MONGOC_URI_AUTHMECHANISMPROPERTIES, bson (*properties)));
   bson_reinit (&uri->credentials);
   bsonBuildAppend (uri->credentials, insert (tmp, always));
   bson_destroy (&tmp);
   return bsonBuildError == NULL;
}


static bool
_mongoc_uri_assign_read_prefs_mode (mongoc_uri_t *uri, bson_error_t *error)
{
   BSON_ASSERT (uri);

   mongoc_read_mode_t mode = 0;
   const char *pref = NULL;
   bsonParse (uri->options,
              find (
                 // Find the 'readPreference' string
                 iKeyWithType (MONGOC_URI_READPREFERENCE, utf8),
                 case ( // Switch on the string content:
                    when (iStrEqual ("primary"), do (mode = MONGOC_READ_PRIMARY)),
                    when (iStrEqual ("primaryPreferred"), do (mode = MONGOC_READ_PRIMARY_PREFERRED)),
                    when (iStrEqual ("secondary"), do (mode = MONGOC_READ_SECONDARY)),
                    when (iStrEqual ("secondaryPreferred"), do (mode = MONGOC_READ_SECONDARY_PREFERRED)),
                    when (iStrEqual ("nearest"), do (mode = MONGOC_READ_NEAREST)),
                    else (do ({
                       pref = bsonAs (cstr);
                       bsonParseError = "Unsupported readPreference value";
                    })))));

   if (bsonParseError) {
      const char *prefix = "Error while assigning URI read preference";
      if (pref) {
         MONGOC_URI_ERROR (error, "%s: %s [readPreference=%s]", prefix, bsonParseError, pref);
      } else {
         MONGOC_URI_ERROR (error, "%s: %s", prefix, bsonParseError);
      }
      return false;
   }

   if (mode != 0) {
      mongoc_read_prefs_set_mode (uri->read_prefs, mode);
   }
   return true;
}


static bool
_mongoc_uri_build_write_concern (mongoc_uri_t *uri, bson_error_t *error)
{
   mongoc_write_concern_t *write_concern;
   int64_t wtimeoutms;

   BSON_ASSERT (uri);

   write_concern = mongoc_write_concern_new ();
   uri->write_concern = write_concern;

   bsonParse (uri->options,
              find (iKeyWithType (MONGOC_URI_SAFE, boolean),
                    do (mongoc_write_concern_set_w (write_concern,
                                                    bsonAs (boolean) ? 1 : MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED))));

   if (bsonParseError) {
      MONGOC_URI_ERROR (error, "Error while parsing 'safe' URI option: %s", bsonParseError);
      return false;
   }

   wtimeoutms = mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 0);
   if (wtimeoutms < 0) {
      MONGOC_URI_ERROR (error, "Unsupported wtimeoutMS value [w=%" PRId64 "]", wtimeoutms);
      return false;
   } else if (wtimeoutms > 0) {
      mongoc_write_concern_set_wtimeout_int64 (write_concern, wtimeoutms);
   }

   bsonParse (uri->options,
              find (iKeyWithType (MONGOC_URI_JOURNAL, boolean),
                    do (mongoc_write_concern_set_journal (write_concern, bsonAs (boolean)))));
   if (bsonParseError) {
      MONGOC_URI_ERROR (error, "Error while parsing 'journal' URI option: %s", bsonParseError);
      return false;
   }

   int w_int = INT_MAX;
   const char *w_str = NULL;
   bsonParse (uri->options,
              find (iKey ("w"), //
                    storeInt32 (w_int),
                    storeStrRef (w_str),
                    case (
                       // Special W options:
                       when (eq (int32, MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED),
                             // These conflict with journalling:
                             if (eval (mongoc_write_concern_get_journal (write_concern)),
                                 then (error ("Journal conflicts with w value"))),
                             do (mongoc_write_concern_set_w (write_concern, bsonAs (int32)))),
                       // Other positive 'w' value:
                       when (allOf (type (int32), eval (bsonAs (int32) > 0)),
                             do (mongoc_write_concern_set_w (write_concern, bsonAs (int32)))),
                       // Special "majority" string:
                       when (iStrEqual ("majority"),
                             do (mongoc_write_concern_set_w (write_concern, MONGOC_WRITE_CONCERN_W_MAJORITY))),
                       // Other string:
                       when (type (utf8), do (mongoc_write_concern_set_wtag (write_concern, bsonAs (cstr)))),
                       // Invalid value:
                       else (error ("Unsupported w value")))));

   if (bsonParseError) {
      const char *const prefix = "Error while parsing the 'w' URI option";
      if (w_str) {
         MONGOC_URI_ERROR (error, "%s: %s [w=%s]", prefix, bsonParseError, w_str);
      } else if (w_int != INT_MAX) {
         MONGOC_URI_ERROR (error, "%s: %s [w=%d]", prefix, bsonParseError, w_int);
      } else {
         MONGOC_URI_ERROR (error, "%s: %s", prefix, bsonParseError);
      }
      return false;
   }
   return true;
}

/* can't use mongoc_uri_get_option_as_int32, it treats 0 specially */
static int32_t
_mongoc_uri_get_max_staleness_option (const mongoc_uri_t *uri)
{
   const bson_t *options;
   bson_iter_t iter;
   int32_t retval = MONGOC_NO_MAX_STALENESS;

   if ((options = mongoc_uri_get_options (uri)) &&
       bson_iter_init_find_case (&iter, options, MONGOC_URI_MAXSTALENESSSECONDS) && BSON_ITER_HOLDS_INT32 (&iter)) {
      retval = bson_iter_int32 (&iter);
      if (retval == 0) {
         MONGOC_WARNING ("Unsupported value for \"" MONGOC_URI_MAXSTALENESSSECONDS "\": \"%d\"", retval);
         retval = -1;
      } else if (retval < 0 && retval != -1) {
         MONGOC_WARNING ("Unsupported value for \"" MONGOC_URI_MAXSTALENESSSECONDS "\": \"%d\"", retval);
         retval = MONGOC_NO_MAX_STALENESS;
      }
   }

   return retval;
}

mongoc_uri_t *
mongoc_uri_new_with_error (const char *uri_string, bson_error_t *error)
{
   mongoc_uri_t *uri;
   int32_t max_staleness_seconds;

   uri = BSON_ALIGNED_ALLOC0 (mongoc_uri_t);
   bson_init (&uri->raw);
   bson_init (&uri->options);
   bson_init (&uri->credentials);
   bson_init (&uri->compressors);

   /* Initialize read_prefs, since parsing may add to it */
   uri->read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);

   /* Initialize empty read_concern */
   uri->read_concern = mongoc_read_concern_new ();

   if (!uri_string) {
      uri_string = "mongodb://127.0.0.1/";
   }

   if (!mongoc_uri_parse (uri, uri_string, error)) {
      mongoc_uri_destroy (uri);
      return NULL;
   }

   uri->str = bson_strdup (uri_string);

   if (!_mongoc_uri_assign_read_prefs_mode (uri, error)) {
      mongoc_uri_destroy (uri);
      return NULL;
   }
   max_staleness_seconds = _mongoc_uri_get_max_staleness_option (uri);
   mongoc_read_prefs_set_max_staleness_seconds (uri->read_prefs, max_staleness_seconds);

   if (!mongoc_read_prefs_is_valid (uri->read_prefs)) {
      mongoc_uri_destroy (uri);
      MONGOC_URI_ERROR (error, "%s", "Invalid readPreferences");
      return NULL;
   }

   if (!_mongoc_uri_build_write_concern (uri, error)) {
      mongoc_uri_destroy (uri);
      return NULL;
   }

   if (!mongoc_write_concern_is_valid (uri->write_concern)) {
      mongoc_uri_destroy (uri);
      MONGOC_URI_ERROR (error, "%s", "Invalid writeConcern");
      return NULL;
   }

   return uri;
}

mongoc_uri_t *
mongoc_uri_new (const char *uri_string)
{
   bson_error_t error = {0};
   mongoc_uri_t *uri;

   uri = mongoc_uri_new_with_error (uri_string, &error);
   if (error.domain) {
      MONGOC_WARNING ("Error parsing URI: '%s'", error.message);
   }

   return uri;
}


mongoc_uri_t *
mongoc_uri_new_for_host_port (const char *hostname, uint16_t port)
{
   mongoc_uri_t *uri;
   char *str;

   BSON_ASSERT (hostname);
   BSON_ASSERT (port);

   str = bson_strdup_printf ("mongodb://%s:%hu/", hostname, port);
   uri = mongoc_uri_new (str);
   bson_free (str);

   return uri;
}


const char *
mongoc_uri_get_username (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);

   return uri->username;
}

bool
mongoc_uri_set_username (mongoc_uri_t *uri, const char *username)
{
   size_t len;

   BSON_ASSERT (username);

   len = strlen (username);

   if (!bson_utf8_validate (username, len, false)) {
      return false;
   }

   if (uri->username) {
      bson_free (uri->username);
   }

   uri->username = bson_strdup (username);
   return true;
}


const char *
mongoc_uri_get_password (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);

   return uri->password;
}

bool
mongoc_uri_set_password (mongoc_uri_t *uri, const char *password)
{
   size_t len;

   BSON_ASSERT (password);

   len = strlen (password);

   if (!bson_utf8_validate (password, len, false)) {
      return false;
   }

   if (uri->password) {
      bson_free (uri->password);
   }

   uri->password = bson_strdup (password);
   return true;
}


const char *
mongoc_uri_get_database (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return uri->database;
}

bool
mongoc_uri_set_database (mongoc_uri_t *uri, const char *database)
{
   size_t len;

   BSON_ASSERT (database);

   len = strlen (database);

   if (!bson_utf8_validate (database, len, false)) {
      return false;
   }

   if (uri->database) {
      bson_free (uri->database);
   }

   uri->database = bson_strdup (database);
   return true;
}


const char *
mongoc_uri_get_auth_source (const mongoc_uri_t *uri)
{
   BSON_ASSERT_PARAM (uri);

   // Explicitly set.
   {
      bson_iter_t iter;
      if (bson_iter_init_find_case (&iter, &uri->credentials, MONGOC_URI_AUTHSOURCE)) {
         return bson_iter_utf8 (&iter, NULL);
      }
   }

   // The database name if supplied.
   const char *const db = uri->database;

   // Depending on the authentication mechanism, `MongoCredential.source` has different defaults.
   const char *const mechanism = mongoc_uri_get_auth_mechanism (uri);

   // Default authentication mechanism uses either SCRAM-SHA-1 or SCRAM-SHA-256.
   if (!mechanism) {
      return db ? db : "admin";
   }

   // Defaults to the database name if supplied on the connection string or "admin" for:
   {
      static const char *const matches[] = {
         "SCRAM-SHA-1",
         "SCRAM-SHA-256",
         NULL,
      };

      for (const char *const *match = matches; *match; ++match) {
         if (strcasecmp (mechanism, *match) == 0) {
            return db ? db : "admin";
         }
      }
   }

   // Defaults to the database name if supplied on the connection string or "$external" for:
   //  - PLAIN
   if (strcasecmp (mechanism, "PLAIN") == 0) {
      return db ? db : "$external";
   }

   // Fallback to "$external" for all remaining authentication mechanisms:
   //  - MONGODB-X509
   //  - GSSAPI
   //  - MONGODB-AWS
   return "$external";
}


bool
mongoc_uri_set_auth_source (mongoc_uri_t *uri, const char *value)
{
   size_t len;

   BSON_ASSERT (value);

   len = strlen (value);

   if (!bson_utf8_validate (value, len, false)) {
      return false;
   }

   _bson_upsert_utf8_icase (&uri->credentials, mstr_cstring (MONGOC_URI_AUTHSOURCE), value);

   return true;
}


const char *
mongoc_uri_get_appname (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);

   return mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_APPNAME, NULL);
}


bool
mongoc_uri_set_appname (mongoc_uri_t *uri, const char *value)
{
   BSON_ASSERT (value);

   if (!bson_utf8_validate (value, strlen (value), false)) {
      return false;
   }

   if (!_mongoc_handshake_appname_is_valid (value)) {
      return false;
   }

   _bson_upsert_utf8_icase (&uri->options, mstr_cstring (MONGOC_URI_APPNAME), value);

   return true;
}

bool
mongoc_uri_set_compressors (mongoc_uri_t *uri, const char *value)
{
   bson_reinit (&uri->compressors);

   if (!value) {
      // Just clear the compressors
      return true;
   }

   if (!bson_utf8_validate (value, strlen (value), false)) {
      // Invalid UTF-8 in the string
      return false;
   }

   for (mstr_view remain = mstr_cstring (value); remain.len;) {
      mstr_view entry;
      mstr_split_around (remain, COMMA, &entry, &remain);
      if (mongoc_compressor_supported (entry)) {
         _bson_upsert_utf8_icase (&uri->compressors, entry, "yes");
      } else {
         MONGOC_WARNING ("Unsupported compressor: '%.*s'", MSTR_FMT (entry));
      }
   }

   return true;
}

const bson_t *
mongoc_uri_get_compressors (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return &uri->compressors;
}


/* can't use mongoc_uri_get_option_as_int32, it treats 0 specially */
int32_t
mongoc_uri_get_local_threshold_option (const mongoc_uri_t *uri)
{
   const bson_t *options;
   bson_iter_t iter;
   int32_t retval = MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS;

   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, "localthresholdms") &&
       BSON_ITER_HOLDS_INT32 (&iter)) {
      retval = bson_iter_int32 (&iter);

      if (retval < 0) {
         MONGOC_WARNING ("Invalid localThresholdMS: %d", retval);
         retval = MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS;
      }
   }

   return retval;
}


const char *
mongoc_uri_get_srv_hostname (const mongoc_uri_t *uri)
{
   if (uri->is_srv) {
      return uri->srv;
   }

   return NULL;
}


/* Initial DNS Seedlist Discovery Spec: `srvServiceName` requires a string value
 * and defaults to "mongodb". */
static const char *const mongoc_default_srv_service_name = "mongodb";


const char *
mongoc_uri_get_srv_service_name (const mongoc_uri_t *uri)
{
   bson_iter_t iter;

   BSON_ASSERT_PARAM (uri);

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_SRVSERVICENAME)) {
      BSON_ASSERT (BSON_ITER_HOLDS_UTF8 (&iter));
      return bson_iter_utf8 (&iter, NULL);
   }

   return mongoc_default_srv_service_name;
}


const bson_t *
mongoc_uri_get_options (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return &uri->options;
}


void
mongoc_uri_destroy (mongoc_uri_t *uri)
{
   if (uri) {
      _mongoc_host_list_destroy_all (uri->hosts);
      bson_free (uri->str);
      bson_free (uri->database);
      bson_free (uri->username);
      bson_destroy (&uri->raw);
      bson_destroy (&uri->options);
      bson_destroy (&uri->credentials);
      bson_destroy (&uri->compressors);
      mongoc_read_prefs_destroy (uri->read_prefs);
      mongoc_read_concern_destroy (uri->read_concern);
      mongoc_write_concern_destroy (uri->write_concern);

      if (uri->password) {
         bson_zero_free (uri->password, strlen (uri->password));
      }

      bson_free (uri);
   }
}


mongoc_uri_t *
mongoc_uri_copy (const mongoc_uri_t *uri)
{
   mongoc_uri_t *copy;
   mongoc_host_list_t *iter;
   bson_error_t error;

   BSON_ASSERT (uri);

   copy = BSON_ALIGNED_ALLOC0 (mongoc_uri_t);

   copy->str = bson_strdup (uri->str);
   copy->is_srv = uri->is_srv;
   bson_strncpy (copy->srv, uri->srv, sizeof uri->srv);
   copy->username = bson_strdup (uri->username);
   copy->password = bson_strdup (uri->password);
   copy->database = bson_strdup (uri->database);

   copy->read_prefs = mongoc_read_prefs_copy (uri->read_prefs);
   copy->read_concern = mongoc_read_concern_copy (uri->read_concern);
   copy->write_concern = mongoc_write_concern_copy (uri->write_concern);

   LL_FOREACH (uri->hosts, iter)
   {
      if (!mongoc_uri_upsert_host (copy, iter->host, iter->port, &error)) {
         MONGOC_ERROR ("%s", error.message);
         mongoc_uri_destroy (copy);
         return NULL;
      }
   }

   bson_copy_to (&uri->raw, &copy->raw);
   bson_copy_to (&uri->options, &copy->options);
   bson_copy_to (&uri->credentials, &copy->credentials);
   bson_copy_to (&uri->compressors, &copy->compressors);

   return copy;
}


const char *
mongoc_uri_get_string (const mongoc_uri_t *uri)
{
   BSON_ASSERT (uri);
   return uri->str;
}


char *
mongoc_uri_unescape (const char *escaped_string)
{
   bson_error_t error;
   char *r = _strdup_pct_decode (mstr_cstring (escaped_string), &error);
   if (!r) {
      MONGOC_WARNING ("%s(): Invalid %% escape sequence: %s", BSON_FUNC, error.message);
   }
   return r;
}


const mongoc_read_prefs_t *
mongoc_uri_get_read_prefs_t (const mongoc_uri_t *uri) /* IN */
{
   BSON_ASSERT (uri);

   return uri->read_prefs;
}


void
mongoc_uri_set_read_prefs_t (mongoc_uri_t *uri, const mongoc_read_prefs_t *prefs)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (prefs);

   mongoc_read_prefs_destroy (uri->read_prefs);
   uri->read_prefs = mongoc_read_prefs_copy (prefs);
}


const mongoc_read_concern_t *
mongoc_uri_get_read_concern (const mongoc_uri_t *uri) /* IN */
{
   BSON_ASSERT (uri);

   return uri->read_concern;
}


void
mongoc_uri_set_read_concern (mongoc_uri_t *uri, const mongoc_read_concern_t *rc)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (rc);

   mongoc_read_concern_destroy (uri->read_concern);
   uri->read_concern = mongoc_read_concern_copy (rc);
}


const mongoc_write_concern_t *
mongoc_uri_get_write_concern (const mongoc_uri_t *uri) /* IN */
{
   BSON_ASSERT (uri);

   return uri->write_concern;
}


void
mongoc_uri_set_write_concern (mongoc_uri_t *uri, const mongoc_write_concern_t *wc)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (wc);

   mongoc_write_concern_destroy (uri->write_concern);
   uri->write_concern = mongoc_write_concern_copy (wc);
}


bool
mongoc_uri_get_tls (const mongoc_uri_t *uri) /* IN */
{
   bson_iter_t iter;

   BSON_ASSERT (uri);

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLS) && BSON_ITER_HOLDS_BOOL (&iter)) {
      return bson_iter_bool (&iter);
   }

   if (bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSCERTIFICATEKEYFILE) ||
       bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSCAFILE) ||
       bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES) ||
       bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES) ||
       bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSINSECURE) ||
       bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD) ||
       bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK) ||
       bson_iter_init_find_case (&iter, &uri->options, MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK)) {
      return true;
   }

   return false;
}


const char *
mongoc_uri_get_server_monitoring_mode (const mongoc_uri_t *uri)
{
   BSON_ASSERT_PARAM (uri);

   return mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_SERVERMONITORINGMODE, "auto");
}


bool
mongoc_uri_set_server_monitoring_mode (mongoc_uri_t *uri, const char *value)
{
   BSON_ASSERT_PARAM (uri);
   BSON_ASSERT_PARAM (value);

   // Check for valid value
   if (strcmp (value, "stream") && strcmp (value, "poll") && strcmp (value, "auto")) {
      return false;
   }

   _bson_upsert_utf8_icase (&uri->options, mstr_cstring (MONGOC_URI_SERVERMONITORINGMODE), value);
   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_get_option_as_int32 --
 *
 *       Checks if the URI 'option' is set and of correct type (int32).
 *       The special value '0' is considered as "unset".
 *       This is so users can provide
 *       sprintf("mongodb://localhost/?option=%d", myvalue) style connection
 *       strings, and still apply default values.
 *
 *       If not set, or set to invalid type, 'fallback' is returned.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       The value of 'option' if available as int32 (and not 0), or
 *       'fallback'.
 *
 *--------------------------------------------------------------------------
 */

int32_t
mongoc_uri_get_option_as_int32 (const mongoc_uri_t *uri, const char *option_orig, int32_t fallback)
{
   const char *option;
   const bson_t *options;
   bson_iter_t iter;
   int64_t retval = 0;

   option = mongoc_uri_canonicalize_option (option_orig);

   /* BC layer to allow retrieving 32-bit values stored in 64-bit options */
   if (mongoc_uri_option_is_int64 (option_orig)) {
      retval = mongoc_uri_get_option_as_int64 (uri, option_orig, 0);

      if (retval > INT32_MAX || retval < INT32_MIN) {
         MONGOC_WARNING ("Cannot read 64-bit value for \"%s\": %" PRId64, option_orig, retval);

         retval = 0;
      }
   } else if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option) &&
              BSON_ITER_HOLDS_INT32 (&iter)) {
      retval = bson_iter_int32 (&iter);
   }

   if (!retval) {
      retval = fallback;
   }

   return (int32_t) retval;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_set_option_as_int32 --
 *
 *       Sets a URI option 'after the fact'. Allows users to set individual
 *       URI options without passing them as a connection string.
 *
 *       Only allows a set of known options to be set.
 *       @see mongoc_uri_option_is_int32 ().
 *
 *       Does in-place-update of the option BSON if 'option' is already set.
 *       Appends the option to the end otherwise.
 *
 *       NOTE: If 'option' is already set, and is of invalid type, this
 *       function will return false.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_set_option_as_int32 (mongoc_uri_t *uri, const char *option_orig, int32_t value)
{
   const char *option;
   bson_error_t error;
   bool r;

   if (mongoc_uri_option_is_int64 (option_orig)) {
      return mongoc_uri_set_option_as_int64 (uri, option_orig, value);
   }

   option = mongoc_uri_canonicalize_option (option_orig);

   if (!mongoc_uri_option_is_int32 (option)) {
      MONGOC_WARNING ("Unsupported value for \"%s\": %d, \"%s\" is not an int32 option", option_orig, value, option);
      return false;
   }

   r = _mongoc_uri_set_option_as_int32_with_error (uri, option, value, &error);
   if (!r) {
      MONGOC_WARNING ("%s", error.message);
   }

   return r;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_uri_set_option_as_int32_with_error --
 *
 *       Same as mongoc_uri_set_option_as_int32, with error reporting.
 *
 * Precondition:
 *       mongoc_uri_option_is_int32(option) must be true.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_uri_set_option_as_int32_with_error (mongoc_uri_t *uri,
                                            const char *option_orig,
                                            int32_t value,
                                            bson_error_t *error)
{
   const char *option;
   const bson_t *options;
   bson_iter_t iter;
   char *option_lowercase = NULL;

   option = mongoc_uri_canonicalize_option (option_orig);
   /* Server Discovery and Monitoring Spec: "the driver MUST NOT permit users
    * to configure it less than minHeartbeatFrequencyMS (500ms)." */
   if (!bson_strcasecmp (option, MONGOC_URI_HEARTBEATFREQUENCYMS) &&
       value < MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS) {
      MONGOC_URI_ERROR (error,
                        "Invalid \"%s\" of %d: must be at least %d",
                        option_orig,
                        value,
                        MONGOC_TOPOLOGY_MIN_HEARTBEAT_FREQUENCY_MS);
      return false;
   }

   /* zlib levels are from -1 (default) through 9 (best compression) */
   if (!bson_strcasecmp (option, MONGOC_URI_ZLIBCOMPRESSIONLEVEL) && (value < -1 || value > 9)) {
      MONGOC_URI_ERROR (error, "Invalid \"%s\" of %d: must be between -1 and 9", option_orig, value);
      return false;
   }

   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option)) {
      if (BSON_ITER_HOLDS_INT32 (&iter)) {
         bson_iter_overwrite_int32 (&iter, value);
         return true;
      } else {
         MONGOC_URI_ERROR (error,
                           "Cannot set URI option \"%s\" to %d, it already has "
                           "a non-32-bit integer value",
                           option,
                           value);
         return false;
      }
   }
   option_lowercase = lowercase_str_new (option);
   if (!bson_append_int32 (&uri->options, option_lowercase, -1, value)) {
      bson_free (option_lowercase);
      MONGOC_URI_ERROR (error, "Failed to set URI option \"%s\" to %d", option_orig, value);

      return false;
   }

   bson_free (option_lowercase);
   return true;
}


/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_uri_set_option_as_int32 --
 *
 *       Same as mongoc_uri_set_option_as_int32, except the option is not
 *       validated against valid int32 options
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_uri_set_option_as_int32 (mongoc_uri_t *uri, const char *option_orig, int32_t value)
{
   const char *option;
   const bson_t *options;
   bson_iter_t iter;
   char *option_lowercase = NULL;

   option = mongoc_uri_canonicalize_option (option_orig);
   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option)) {
      if (BSON_ITER_HOLDS_INT32 (&iter)) {
         bson_iter_overwrite_int32 (&iter, value);
         return true;
      } else {
         return false;
      }
   }

   option_lowercase = lowercase_str_new (option);
   bson_append_int32 (&uri->options, option_lowercase, -1, value);
   bson_free (option_lowercase);
   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_get_option_as_int64 --
 *
 *       Checks if the URI 'option' is set and of correct type (int32 or
 *       int64).
 *       The special value '0' is considered as "unset".
 *       This is so users can provide
 *       sprintf("mongodb://localhost/?option=%" PRId64, myvalue) style
 *       connection strings, and still apply default values.
 *
 *       If not set, or set to invalid type, 'fallback' is returned.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       The value of 'option' if available as int64 or int32 (and not 0), or
 *       'fallback'.
 *
 *--------------------------------------------------------------------------
 */

int64_t
mongoc_uri_get_option_as_int64 (const mongoc_uri_t *uri, const char *option_orig, int64_t fallback)
{
   const char *option;
   const bson_t *options;
   bson_iter_t iter;
   int64_t retval = fallback;

   option = mongoc_uri_canonicalize_option (option_orig);
   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option)) {
      if (BSON_ITER_HOLDS_INT (&iter)) {
         if (!(retval = bson_iter_as_int64 (&iter))) {
            retval = fallback;
         }
      }
   }

   return retval;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_set_option_as_int64 --
 *
 *       Sets a URI option 'after the fact'. Allows users to set individual
 *       URI options without passing them as a connection string.
 *
 *       Only allows a set of known options to be set.
 *       @see mongoc_uri_option_is_int64 ().
 *
 *       Does in-place-update of the option BSON if 'option' is already set.
 *       Appends the option to the end otherwise.
 *
 *       NOTE: If 'option' is already set, and is of invalid type, this
 *       function will return false.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_set_option_as_int64 (mongoc_uri_t *uri, const char *option_orig, int64_t value)
{
   const char *option;
   bson_error_t error;
   bool r;

   option = mongoc_uri_canonicalize_option (option_orig);
   if (!mongoc_uri_option_is_int64 (option)) {
      if (mongoc_uri_option_is_int32 (option_orig)) {
         if (value >= INT32_MIN && value <= INT32_MAX) {
            MONGOC_WARNING ("Setting value for 32-bit option \"%s\" through 64-bit method", option_orig);

            return mongoc_uri_set_option_as_int32 (uri, option_orig, (int32_t) value);
         }

         MONGOC_WARNING (
            "Unsupported value for \"%s\": %" PRId64 ", \"%s\" is not an int64 option", option_orig, value, option);
         return false;
      }
   }

   r = _mongoc_uri_set_option_as_int64_with_error (uri, option, value, &error);
   if (!r) {
      MONGOC_WARNING ("%s", error.message);
   }

   return r;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_uri_set_option_as_int64_with_error --
 *
 *       Same as mongoc_uri_set_option_as_int64, with error reporting.
 *
 * Precondition:
 *       mongoc_uri_option_is_int64(option) must be true.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

static bool
_mongoc_uri_set_option_as_int64_with_error (mongoc_uri_t *uri,
                                            const char *option_orig,
                                            int64_t value,
                                            bson_error_t *error)
{
   const char *option;
   const bson_t *options;
   bson_iter_t iter;
   char *option_lowercase = NULL;

   option = mongoc_uri_canonicalize_option (option_orig);

   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option)) {
      if (BSON_ITER_HOLDS_INT64 (&iter)) {
         bson_iter_overwrite_int64 (&iter, value);
         return true;
      } else {
         MONGOC_URI_ERROR (error,
                           "Cannot set URI option \"%s\" to %" PRId64 ", it already has "
                           "a non-64-bit integer value",
                           option,
                           value);
         return false;
      }
   }

   option_lowercase = lowercase_str_new (option);
   if (!bson_append_int64 (&uri->options, option_lowercase, -1, value)) {
      bson_free (option_lowercase);
      MONGOC_URI_ERROR (error, "Failed to set URI option \"%s\" to %" PRId64, option_orig, value);

      return false;
   }
   bson_free (option_lowercase);
   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_get_option_as_bool --
 *
 *       Checks if the URI 'option' is set and of correct type (bool).
 *
 *       If not set, or set to invalid type, 'fallback' is returned.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       The value of 'option' if available as bool, or 'fallback'.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_get_option_as_bool (const mongoc_uri_t *uri, const char *option_orig, bool fallback)
{
   const char *option;
   const bson_t *options;
   bson_iter_t iter;

   option = mongoc_uri_canonicalize_option (option_orig);
   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option) &&
       BSON_ITER_HOLDS_BOOL (&iter)) {
      return bson_iter_bool (&iter);
   }

   return fallback;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_set_option_as_bool --
 *
 *       Sets a URI option 'after the fact'. Allows users to set individual
 *       URI options without passing them as a connection string.
 *
 *       Only allows a set of known options to be set.
 *       @see mongoc_uri_option_is_bool ().
 *
 *       Does in-place-update of the option BSON if 'option' is already set.
 *       Appends the option to the end otherwise.
 *
 *       NOTE: If 'option' is already set, and is of invalid type, this
 *       function will return false.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_set_option_as_bool (mongoc_uri_t *uri, const char *option_orig, bool value)
{
   const char *option;
   char *option_lowercase;
   const bson_t *options;
   bson_iter_t iter;

   option = mongoc_uri_canonicalize_option (option_orig);
   BSON_ASSERT (option);

   if (!mongoc_uri_option_is_bool (option)) {
      return false;
   }

   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option)) {
      if (BSON_ITER_HOLDS_BOOL (&iter)) {
         bson_iter_overwrite_bool (&iter, value);
         return true;
      } else {
         return false;
      }
   }
   option_lowercase = lowercase_str_new (option);
   bson_append_bool (&uri->options, option_lowercase, -1, value);
   bson_free (option_lowercase);
   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_get_option_as_utf8 --
 *
 *       Checks if the URI 'option' is set and of correct type (utf8).
 *
 *       If not set, or set to invalid type, 'fallback' is returned.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       The value of 'option' if available as utf8, or 'fallback'.
 *
 *--------------------------------------------------------------------------
 */

const char *
mongoc_uri_get_option_as_utf8 (const mongoc_uri_t *uri, const char *option_orig, const char *fallback)
{
   const char *option;
   const bson_t *options;
   bson_iter_t iter;

   option = mongoc_uri_canonicalize_option (option_orig);
   if ((options = mongoc_uri_get_options (uri)) && bson_iter_init_find_case (&iter, options, option) &&
       BSON_ITER_HOLDS_UTF8 (&iter)) {
      return bson_iter_utf8 (&iter, NULL);
   }

   return fallback;
}

/*
 *--------------------------------------------------------------------------
 *
 * mongoc_uri_set_option_as_utf8 --
 *
 *       Sets a URI option 'after the fact'. Allows users to set individual
 *       URI options without passing them as a connection string.
 *
 *       Only allows a set of known options to be set.
 *       @see mongoc_uri_option_is_utf8 ().
 *
 *       If the option is not already set, this function will append it to
 *the end of the options bson. NOTE: If the option is already set the entire
 *options bson will be overwritten, containing the new option=value
 *(at the same position).
 *
 *       NOTE: If 'option' is already set, and is of invalid type, this
 *       function will return false.
 *
 *       NOTE: 'option' must be valid utf8.
 *
 *       NOTE: 'option' is case*in*sensitive.
 *
 * Returns:
 *       true on successfully setting the option, false on failure.
 *
 *--------------------------------------------------------------------------
 */

bool
mongoc_uri_set_option_as_utf8 (mongoc_uri_t *uri, const char *option_orig, const char *value)
{
   const char *option;
   size_t len;

   option = mongoc_uri_canonicalize_option (option_orig);
   BSON_ASSERT (option);

   len = strlen (value);

   if (!bson_utf8_validate (value, len, false)) {
      return false;
   }

   if (!mongoc_uri_option_is_utf8 (option)) {
      return false;
   }
   if (!bson_strcasecmp (option, MONGOC_URI_APPNAME)) {
      return mongoc_uri_set_appname (uri, value);
   } else if (!bson_strcasecmp (option, MONGOC_URI_SERVERMONITORINGMODE)) {
      return mongoc_uri_set_server_monitoring_mode (uri, value);
   } else {
      _bson_upsert_utf8_icase (&uri->options, mstr_cstring (option), value);
   }

   return true;
}

/*
 *--------------------------------------------------------------------------
 *
 * _mongoc_uri_requires_auth_negotiation --
 *
 *       Returns true if auth mechanism is necessary for this uri. According
 *       to the auth spec: "If an application provides a username but does
 *       not provide an authentication mechanism, drivers MUST negotiate a
 *       mechanism".
 *
 * Returns:
 *       true if the driver should negotiate the auth mechanism for the uri
 *
 *--------------------------------------------------------------------------
 */
bool
_mongoc_uri_requires_auth_negotiation (const mongoc_uri_t *uri)
{
   return mongoc_uri_get_username (uri) && !mongoc_uri_get_auth_mechanism (uri);
}


/* A bit of a hack. Needed for multi mongos tests to create a URI with the same
 * auth, SSL, and compressors settings but with only one specific host. */
mongoc_uri_t *
_mongoc_uri_copy_and_replace_host_list (const mongoc_uri_t *original, const char *host)
{
   mongoc_uri_t *uri = mongoc_uri_copy (original);
   _mongoc_host_list_destroy_all (uri->hosts);
   uri->hosts = bson_malloc0 (sizeof (mongoc_host_list_t));
   _mongoc_host_list_from_string (uri->hosts, host);
   return uri;
}

bool
mongoc_uri_init_with_srv_host_list (mongoc_uri_t *uri, mongoc_host_list_t *host_list, bson_error_t *error)
{
   mongoc_host_list_t *host;

   BSON_ASSERT (uri->is_srv);
   BSON_ASSERT (!uri->hosts);

   LL_FOREACH (host_list, host)
   {
      if (!mongoc_uri_upsert_host_and_port (uri, host->host_and_port, error)) {
         return false;
      }
   }

   return true;
}

#ifdef MONGOC_ENABLE_CRYPTO
void
_mongoc_uri_init_scram (const mongoc_uri_t *uri, mongoc_scram_t *scram, mongoc_crypto_hash_algorithm_t algo)
{
   BSON_ASSERT (uri);
   BSON_ASSERT (scram);

   _mongoc_scram_init (scram, algo);

   _mongoc_scram_set_pass (scram, mongoc_uri_get_password (uri));
   _mongoc_scram_set_user (scram, mongoc_uri_get_username (uri));
}
#endif

static bool
mongoc_uri_finalize_loadbalanced (const mongoc_uri_t *uri, bson_error_t *error)
{
   if (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_LOADBALANCED, false)) {
      return true;
   }

   /* Load Balancer Spec: When `loadBalanced=true` is provided in the connection
    * string, the driver MUST throw an exception if the connection string
    * contains more than one host/port. */
   if (uri->hosts && uri->hosts->next) {
      MONGOC_URI_ERROR (error, "URI with \"%s\" enabled must not contain more than one host", MONGOC_URI_LOADBALANCED);
      return false;
   }

   if (mongoc_uri_has_option (uri, MONGOC_URI_REPLICASET)) {
      MONGOC_URI_ERROR (error,
                        "URI with \"%s\" enabled must not contain option \"%s\"",
                        MONGOC_URI_LOADBALANCED,
                        MONGOC_URI_REPLICASET);
      return false;
   }

   if (mongoc_uri_has_option (uri, MONGOC_URI_DIRECTCONNECTION) &&
       mongoc_uri_get_option_as_bool (uri, MONGOC_URI_DIRECTCONNECTION, false)) {
      MONGOC_URI_ERROR (error,
                        "URI with \"%s\" enabled must not contain option \"%s\" enabled",
                        MONGOC_URI_LOADBALANCED,
                        MONGOC_URI_DIRECTCONNECTION);
      return false;
   }

   return true;
}

static bool
mongoc_uri_finalize_srv (const mongoc_uri_t *uri, bson_error_t *error)
{
   /* Initial DNS Seedlist Discovery Spec: The driver MUST report an error if
    * either the `srvServiceName` or `srvMaxHosts` URI options are specified
    * with a non-SRV URI. */
   if (!uri->is_srv) {
      const char *option = NULL;

      if (mongoc_uri_has_option (uri, MONGOC_URI_SRVSERVICENAME)) {
         option = MONGOC_URI_SRVSERVICENAME;
      } else if (mongoc_uri_has_option (uri, MONGOC_URI_SRVMAXHOSTS)) {
         option = MONGOC_URI_SRVMAXHOSTS;
      }

      if (option) {
         MONGOC_URI_ERROR (error, "%s must not be specified with a non-SRV URI", option);
         return false;
      }
   }

   if (uri->is_srv) {
      const int32_t max_hosts = mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SRVMAXHOSTS, 0);

      /* Initial DNS Seedless Discovery Spec: This option requires a
       * non-negative integer and defaults to zero (i.e. no limit). */
      if (max_hosts < 0) {
         MONGOC_URI_ERROR (error,
                           "%s is required to be a non-negative integer, but "
                           "has value %" PRId32,
                           MONGOC_URI_SRVMAXHOSTS,
                           max_hosts);
         return false;
      }

      if (max_hosts > 0) {
         /* Initial DNS Seedless Discovery spec: If srvMaxHosts is a positive
          * integer, the driver MUST throw an error if the connection string
          * contains a `replicaSet` option. */
         if (mongoc_uri_has_option (uri, MONGOC_URI_REPLICASET)) {
            MONGOC_URI_ERROR (error, "%s must not be specified with %s", MONGOC_URI_SRVMAXHOSTS, MONGOC_URI_REPLICASET);
            return false;
         }

         /* Initial DNS Seedless Discovery Spec: If srvMaxHosts is a positive
          * integer, the driver MUST throw an error if the connection string
          * contains a `loadBalanced` option with a value of `true`.
          */
         if (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_LOADBALANCED, false)) {
            MONGOC_URI_ERROR (
               error, "%s must not be specified with %s=true", MONGOC_URI_SRVMAXHOSTS, MONGOC_URI_LOADBALANCED);
            return false;
         }
      }
   }

   return true;
}

/* This should be called whenever URI options change (e.g. parsing a new URI
 * string, after setting one or more options explicitly, applying TXT records).
 * While the primary purpose of this function is to validate the URI, it may
 * also alter the URI (e.g. implicitly enable TLS when SRV is used). Returns
 * true on success; otherwise, returns false and sets @error. */
bool
mongoc_uri_finalize (mongoc_uri_t *uri, bson_error_t *error)
{
   BSON_ASSERT_PARAM (uri);

   if (!mongoc_uri_finalize_tls (uri, error)) {
      return false;
   }

   if (!mongoc_uri_finalize_auth (uri, error)) {
      return false;
   }

   if (!mongoc_uri_finalize_directconnection (uri, error)) {
      return false;
   }

   if (!mongoc_uri_finalize_loadbalanced (uri, error)) {
      return false;
   }

   if (!mongoc_uri_finalize_srv (uri, error)) {
      return false;
   }

   return true;
}
