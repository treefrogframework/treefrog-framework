#include <common-string-private.h>
#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-host-list-private.h>
#include <mongoc/mongoc-topology-private.h>
#include <mongoc/mongoc-uri-private.h>
#include <mongoc/mongoc-util-private.h>

#include <mongoc/mongoc.h>

#include <mlib/loop.h>

#include <TestSuite.h>
#include <test-conveniences.h>
#include <test-libmongoc.h>

static void
test_mongoc_uri_new (void)
{
   const mongoc_host_list_t *hosts;
   const bson_t *options;
   const bson_t *read_prefs_tags;
   const mongoc_read_prefs_t *read_prefs;
   mongoc_uri_t *uri;
   bson_iter_t iter;

   /* bad uris */
   ASSERT (!mongoc_uri_new ("mongodb://"));
   ASSERT (!mongoc_uri_new ("mongodb://\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:\x80/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/?ipv6=\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/?foo=\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost/?\x80=bar"));
   ASSERT (!mongoc_uri_new ("mongodb://\x80:pass@localhost"));
   ASSERT (!mongoc_uri_new ("mongodb://user:\x80@localhost"));
   ASSERT (!mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                            "?" MONGOC_URI_AUTHMECHANISM "=\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                            "?" MONGOC_URI_AUTHMECHANISM "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES
                            "=SERVICE_NAME:\x80"));
   ASSERT (!mongoc_uri_new ("mongodb://user%40DOMAIN.COM:password@localhost/"
                            "?" MONGOC_URI_AUTHMECHANISM "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES
                            "=\x80:mongodb"));
   ASSERT (!mongoc_uri_new ("mongodb://::"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]::27017/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost,localhost::"));
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2,local3/d?k"));
   // %-encoded chars that are invalid in the database name
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2,local3/db%2fname")); // "/"
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2,local3/db%20ame"));  // " "
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2,local3/db%5came"));  // "\"
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2,local3/db%24ame"));  // "$"
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2,local3/db%22ame"));  // '"'
   ASSERT (!mongoc_uri_new (""));
   ASSERT (!mongoc_uri_new ("mongodb://,localhost:27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:27017,,b"));
   ASSERT (!mongoc_uri_new ("mongo://localhost:27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost::27017,abc"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:-1"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:65536"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:foo"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:65536/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:0/"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1%lo0]"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:-1"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:foo"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:65536"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:65536/"));
   ASSERT (!mongoc_uri_new ("mongodb://[::1]:0/"));
   ASSERT (!mongoc_uri_new ("mongodb://localhost:27017/test?replicaset="));
   ASSERT (!mongoc_uri_new ("mongodb://local1,local2/?directConnection=true"));
   ASSERT (!mongoc_uri_new ("mongodb+srv://local1/?directConnection=true"));

   uri = mongoc_uri_new ("mongodb://[::1]:27888,[::2]:27999/?ipv6=true&" MONGOC_URI_SAFE "=true");
   ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   ASSERT (hosts);
   ASSERT_CMPSTR (hosts->host, "::1");
   ASSERT_CMPUINT16 (hosts->port, ==, 27888);
   ASSERT_CMPSTR (hosts->host_and_port, "[::1]:27888");
   mongoc_uri_destroy (uri);

   /* should recognize IPv6 "scope" like "::1%lo0", with % escaped  */
   uri = mongoc_uri_new ("mongodb://[::1%25lo0]");
   ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   ASSERT (hosts);
   ASSERT_CMPSTR (hosts->host, "::1%lo0");
   ASSERT_CMPUINT16 (hosts->port, ==, 27017);
   ASSERT_CMPSTR (hosts->host_and_port, "[::1%lo0]:27017");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://%2Ftmp%2Fmongodb-27017.sock/?");
   ASSERT (uri);
   mongoc_uri_destroy (uri);

   /* should normalize to lowercase */
   uri = mongoc_uri_new ("mongodb://cRaZyHoStNaMe");
   ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   ASSERT (hosts);
   ASSERT_CMPSTR (hosts->host, "crazyhostname");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?");
   ASSERT (uri);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost:27017/test?replicaset=foo");
   ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   ASSERT (hosts);
   ASSERT (!hosts->next);
   ASSERT_CMPSTR (hosts->host, "localhost");
   ASSERT_CMPUINT16 (hosts->port, ==, 27017);
   ASSERT_CMPSTR (hosts->host_and_port, "localhost:27017");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "test");
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT_EQUAL_BSON (tmp_bson ("{'replicaset': 'foo'}"), options);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://local1,local2:999,local3/?replicaset=foo");
   ASSERT (uri);
   hosts = mongoc_uri_get_hosts (uri);
   ASSERT (hosts);
   ASSERT (hosts->next);
   ASSERT (hosts->next->next);
   ASSERT (!hosts->next->next->next);
   ASSERT_CMPSTR (hosts->host, "local1");
   ASSERT_CMPUINT16 (hosts->port, ==, 27017);
   ASSERT_CMPSTR (hosts->next->host, "local2");
   ASSERT_CMPUINT16 (hosts->next->port, ==, 999);
   ASSERT_CMPSTR (hosts->next->next->host, "local3");
   ASSERT_CMPUINT16 (hosts->next->next->port, ==, 27017);
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT (bson_iter_init_find (&iter, options, "replicaset"));
   ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "foo");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost:27017/"
                         "?" MONGOC_URI_READPREFERENCE "=secondaryPreferred&" MONGOC_URI_READPREFERENCETAGS
                         "=dc:ny&" MONGOC_URI_READPREFERENCETAGS "=");
   ASSERT (uri);
   read_prefs = mongoc_uri_get_read_prefs_t (uri);
   ASSERT (mongoc_read_prefs_get_mode (read_prefs) == MONGOC_READ_SECONDARY_PREFERRED);
   ASSERT (read_prefs);
   read_prefs_tags = mongoc_read_prefs_get_tags (read_prefs);
   ASSERT (read_prefs_tags);
   ASSERT_EQUAL_BSON (tmp_bson ("[{'dc': 'ny'}, {}]"), read_prefs_tags);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_SAFE "=false&" MONGOC_URI_JOURNAL "=false");
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT_EQUAL_BSON (tmp_bson ("{'%s': false, '%s': false}", MONGOC_URI_SAFE, MONGOC_URI_JOURNAL), options);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://%2Ftmp%2Fmongodb-27017.sock/?" MONGOC_URI_TLS "=false");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host, "/tmp/mongodb-27017.sock");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://%2Ftmp%2Fmongodb-27017.sock,localhost:27017/?" MONGOC_URI_TLS "=false");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host, "/tmp/mongodb-27017.sock");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->next->host_and_port, "localhost:27017");
   ASSERT (!mongoc_uri_get_hosts (uri)->next->next);
   mongoc_uri_destroy (uri);

   /* should assign port numbers to correct hosts */
   uri = mongoc_uri_new ("mongodb://host1,host2:30000/foo");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, "host1:27017");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->next->host_and_port, "host2:30000");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost:27017,%2Ftmp%2Fmongodb-27017.sock/?" MONGOC_URI_TLS "=false");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, "localhost:27017");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->next->host, "/tmp/mongodb-27017.sock");
   ASSERT (!mongoc_uri_get_hosts (uri)->next->next);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_HEARTBEATFREQUENCYMS "=600");
   ASSERT (uri);
   ASSERT_CMPINT32 (600, ==, mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 0));

   mongoc_uri_destroy (uri);

   /* heartbeat frequency too short */
   ASSERT (!mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_HEARTBEATFREQUENCYMS "=499"));

   /* should use the " MONGOC_URI_AUTHSOURCE " over db when both are specified
    */
   uri = mongoc_uri_new ("mongodb://christian:secret@localhost:27017/foo?" MONGOC_URI_AUTHSOURCE "=abcd");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "christian");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "secret");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "abcd");
   mongoc_uri_destroy (uri);

   /* should use the default auth source and mechanism */
   uri = mongoc_uri_new ("mongodb://christian:secret@localhost:27017");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "admin");
   ASSERT (!mongoc_uri_get_auth_mechanism (uri));
   mongoc_uri_destroy (uri);

   /* should use the db when no " MONGOC_URI_AUTHSOURCE " is specified */
   uri = mongoc_uri_new ("mongodb://user:password@localhost/foo");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "foo");
   mongoc_uri_destroy (uri);

   /* should recognize an empty password */
   uri = mongoc_uri_new ("mongodb://samantha:@localhost");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "samantha");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "");
   mongoc_uri_destroy (uri);

   /* should recognize no password */
   uri = mongoc_uri_new ("mongodb://christian@localhost:27017");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "christian");
   ASSERT (!mongoc_uri_get_password (uri));
   mongoc_uri_destroy (uri);

   /* should recognize a url escaped character in the username */
   uri = mongoc_uri_new ("mongodb://christian%40realm:pwd@localhost:27017");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "christian@realm");
   mongoc_uri_destroy (uri);

   /* should recognize a question mark in the userpass instead of mistaking it for the beginning of options */
   uri = mongoc_uri_new ("mongodb://us?r:pa?s@localhost?" MONGOC_URI_AUTHMECHANISM "=SCRAM-SHA-1");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "us?r");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "pa?s");
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "SCRAM-SHA-1");
   mongoc_uri_destroy (uri);

   /* should recognize many reserved characters in the userpass for backward compatibility */
   uri = mongoc_uri_new ("mongodb://user?#[]:pass?#[]@localhost?" MONGOC_URI_AUTHMECHANISM "=SCRAM-SHA-1");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user?#[]");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "pass?#[]");
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "SCRAM-SHA-1");
   mongoc_uri_destroy (uri);

   /* should fail on invalid escaped characters */
   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://u%ser:pwd@localhost:27017");
   ASSERT (!uri);
   ASSERT_CAPTURED_LOG ("uri", MONGOC_LOG_LEVEL_WARNING, "Invalid %-encoding in username in URI string");
   capture_logs (false);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://user:p%wd@localhost:27017");
   ASSERT (!uri);
   ASSERT_CAPTURED_LOG ("uri", MONGOC_LOG_LEVEL_WARNING, "Invalid %-encoding in password in URI string");
   capture_logs (false);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://user:pwd@local% host:27017");
   ASSERT (!uri);
   ASSERT_CAPTURED_LOG ("uri", MONGOC_LOG_LEVEL_WARNING, "Invalid %-sequence \"% h\"");
   capture_logs (false);

   uri = mongoc_uri_new ("mongodb://christian%40realm@localhost:27017/?replicaset=%20");
   ASSERT (uri);
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT_EQUAL_BSON (tmp_bson ("{'replicaset': ' '}"), options);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://christian%40realm@[::6]:27017/?replicaset=%20");
   ASSERT (uri);
   options = mongoc_uri_get_options (uri);
   ASSERT (options);
   ASSERT_EQUAL_BSON (tmp_bson ("{'replicaset': ' '}"), options);
   mongoc_uri_destroy (uri);

   // Should warn on unsupported `minPoolSize`. `minPoolSize` was removed in CDRIVER-2390.
   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://host/?minPoolSize=1");
   ASSERT (uri);
   ASSERT_CAPTURED_LOG (
      "setting URI option minPoolSize=1", MONGOC_LOG_LEVEL_WARNING, "Unsupported URI option \"minpoolsize\"");
   mongoc_uri_destroy (uri);
   capture_logs (false);
}

static void
_auth_mechanism_username_required (const char *mechanism)
{
   // None.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             tmp_str ("'%s' authentication mechanism requires a username", mechanism));
      mongoc_uri_destroy (uri);
   }

   // Empty.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             tmp_str ("'%s' authentication mechanism requires a username", mechanism));
   }
}

static void
_auth_mechanism_password_prohibited (const char *mechanism, const char *user_prefix, const char *uri_suffix)
{
   BSON_ASSERT_PARAM (mechanism);
   BSON_ASSERT_PARAM (user_prefix);

   // None.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://%s@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s%s", user_prefix, mechanism, uri_suffix),
         &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_username (uri), user_prefix);
      ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
      ASSERT_CMPSTR (mongoc_uri_get_password (uri), NULL);
      ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), mechanism);
      mongoc_uri_destroy (uri);
   }

   // Empty.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://%s:@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s%s", user_prefix, mechanism, uri_suffix),
         &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             tmp_str ("'%s' authentication mechanism does not accept a password", mechanism));
      mongoc_uri_destroy (uri);
   }

   // Normal.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://%s:pass@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s%s", user_prefix, mechanism, uri_suffix),
         &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             tmp_str ("'%s' authentication mechanism does not accept a password", mechanism));
      clear_captured_logs ();
      mongoc_uri_destroy (uri);
   }
}

static void
_auth_mechanism_password_required (const char *mechanism)
{
   BSON_ASSERT_PARAM (mechanism);

   // None.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             tmp_str ("'%s' authentication mechanism requires a password", mechanism));
   }

   // Empty.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user:@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user");
      ASSERT_CMPSTR (mongoc_uri_get_password (uri), "");
      ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), mechanism);
      mongoc_uri_destroy (uri);
   }

   // Normal.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user");
      ASSERT_CMPSTR (mongoc_uri_get_password (uri), "pass");
      ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), mechanism);
      mongoc_uri_destroy (uri);
   }
}

static void
_auth_mechanism_password_allowed (const char *mechanism)
{
   BSON_ASSERT_PARAM (mechanism);

   // None.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user");
      ASSERT_CMPSTR (mongoc_uri_get_password (uri), NULL);
      ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), mechanism);
      mongoc_uri_destroy (uri);
   }

   // Empty.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user:@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user");
      ASSERT_CMPSTR (mongoc_uri_get_password (uri), "");
      ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), mechanism);
      mongoc_uri_destroy (uri);
   }

   // Normal.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user");
      ASSERT_CMPSTR (mongoc_uri_get_password (uri), "pass");
      ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), mechanism);
      mongoc_uri_destroy (uri);
   }
}


static void
_auth_mechanism_properties_allowed (const char *mechanism, const char *userpass_prefix, const char *default_properties)
{
   BSON_ASSERT_PARAM (mechanism);
   BSON_ASSERT_PARAM (userpass_prefix);
   BSON_OPTIONAL_PARAM (default_properties);

   // None.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://%slocalhost/?" MONGOC_URI_AUTHMECHANISM "=%s", userpass_prefix, mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);

      bson_t props;
      if (default_properties) {
         ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
         ASSERT_MATCH (&props, "{%s}", default_properties ? default_properties : "");
         bson_destroy (&props);
      } else {
         ASSERT_WITH_MSG (!mongoc_uri_get_mechanism_properties (uri, &props), "expected failure");
      }

      mongoc_uri_destroy (uri);
   }

   // Empty.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (tmp_str ("mongodb://%slocalhost/?" MONGOC_URI_AUTHMECHANISM
                                                                    "=%s&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=",
                                                                    userpass_prefix,
                                                                    mechanism),
                                                           &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);

      bson_t props;
      ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
      ASSERT_MATCH (&props, "{%s}", default_properties ? default_properties : "");

      bson_destroy (&props);
      mongoc_uri_destroy (uri);
   }

   // Invalid properties.
   {
      bson_error_t error;
      mongoc_uri_t *const uri =
         mongoc_uri_new_with_error (tmp_str ("mongodb://%slocalhost/?" MONGOC_URI_AUTHMECHANISM
                                             "=%s&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=invalid:value",
                                             userpass_prefix,
                                             mechanism),
                                    &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             tmp_str ("Unsupported '%s' authentication mechanism property: 'invalid'", mechanism));
   }
}

static void
_auth_mechanism_source_default_db_or_admin (const char *mechanism)
{
   BSON_ASSERT_PARAM (mechanism);

   // None (default).
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "admin");
      mongoc_uri_destroy (uri);
   }

   // Database name.
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user:pass@localhost/db?" MONGOC_URI_AUTHMECHANISM "=%s", mechanism), &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "db");
      mongoc_uri_destroy (uri);
   }

   // `authSource` (highest precedence).
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://user:pass@localhost/db?" MONGOC_URI_AUTHMECHANISM "=%s&" MONGOC_URI_AUTHSOURCE "=source",
                  mechanism),
         &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");
      mongoc_uri_destroy (uri);
   }
}

static void
_auth_mechanism_source_external_only (const char *mechanism, const char *userpass_prefix, const char *uri_suffix)
{
   BSON_ASSERT_PARAM (mechanism);
   BSON_ASSERT_PARAM (userpass_prefix);
   BSON_ASSERT_PARAM (uri_suffix);

   // None (default).
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://%slocalhost/?" MONGOC_URI_AUTHMECHANISM "=%s%s", userpass_prefix, mechanism, uri_suffix),
         &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
      mongoc_uri_destroy (uri);
   }

   // Database name (no effect).
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (
         tmp_str ("mongodb://%slocalhost/db?" MONGOC_URI_AUTHMECHANISM "=%s%s", userpass_prefix, mechanism, uri_suffix),
         &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
      mongoc_uri_destroy (uri);
   }

   // `authSource` (highest precedence, incorrect).
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (tmp_str ("mongodb://%slocalhost/db?" MONGOC_URI_AUTHMECHANISM
                                                                    "=%s&" MONGOC_URI_AUTHSOURCE "=source%s",
                                                                    userpass_prefix,
                                                                    mechanism,
                                                                    uri_suffix),
                                                           &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (
         error,
         MONGOC_ERROR_COMMAND,
         MONGOC_ERROR_COMMAND_INVALID_ARG,
         tmp_str ("'%s' authentication mechanism requires \"$external\" authSource, but \"source\" was specified",
                  mechanism));
      mongoc_uri_destroy (uri);
   }

   // `authSource` (highest precedence, correct).
   {
      bson_error_t error;
      mongoc_uri_t *const uri = mongoc_uri_new_with_error (tmp_str ("mongodb://%slocalhost/db?" MONGOC_URI_AUTHMECHANISM
                                                                    "=%s&" MONGOC_URI_AUTHSOURCE "=$external%s",
                                                                    userpass_prefix,
                                                                    mechanism,
                                                                    uri_suffix),
                                                           &error);
      ASSERT_OR_PRINT (uri, error);
      ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      mongoc_uri_destroy (uri);
   }
}

static void
test_mongoc_uri_auth_mechanism_mongodb_x509 (void)
{
   // Authentication spec: username: SHOULD NOT be provided for MongoDB 3.4+.
   // CDRIVER-1959: allow for backward compatibility until the spec states "MUST NOT" instead of "SHOULD NOT" and
   // spec tests are updated accordingly to permit warnings or errors.
   {
      // None.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-X509", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_username (uri), NULL);
         mongoc_uri_destroy (uri);
      }

      // Empty.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://@localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-X509", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error"); // CDRIVER-1959
         ASSERT (!uri);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                "'MONGODB-X509' authentication mechanism requires a non-empty username");
      }

      // Normal.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-X509", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error"); // CDRIVER-1959
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user");
         mongoc_uri_destroy (uri);
      }
   }

   // Authentication spec: password: MUST NOT be specified.
   _auth_mechanism_password_prohibited ("MONGODB-X509", "user", "");

   // Authentication spec: source: MUST be "$external". Defaults to "$external".
   _auth_mechanism_source_external_only ("MONGODB-X509", "", "");
}

static void
test_mongoc_uri_auth_mechanism_gssapi (void)
{
   // Authentication spec: username: MUST be specified and non-zero length.
   _auth_mechanism_username_required ("GSSAPI");

   // Authentication spec: password: MAY be specified.
   _auth_mechanism_password_allowed ("GSSAPI");

   // mechanism_properties are allowed.
   {
      _auth_mechanism_properties_allowed ("GSSAPI", "user:pass@", "'SERVICE_NAME': 'mongodb'");

      // SERVICE_NAME: Drivers MUST allow the user to specify a different service name. The default is "mongodb".
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                                       "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=SERVICE_NAME:name",
                                       &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);

         bson_t props;
         ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
         ASSERT_EQUAL_BSON (tmp_bson ("{'SERVICE_NAME': 'name'}"), &props);

         bson_destroy (&props);
         mongoc_uri_destroy (uri);
      }

      // SERVICE_NAME: naming of mechanism properties MUST be case-insensitive.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                                       "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=service_name:name",
                                       &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);

         bson_t props;
         ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
         ASSERT_EQUAL_BSON (tmp_bson ("{'service_name': 'name'}"), &props);

         bson_destroy (&props);
         mongoc_uri_destroy (uri);
      }

      // CANONICALIZE_HOST_NAME: Drivers MAY allow the user to request canonicalization of the hostname.
      {
         // CDRIVER-4128: only legacy boolean values are currently supported.
         {
            static const char *const values[] = {"false", "true", NULL};

            for (const char *const *value_ptr = values; *value_ptr; ++value_ptr) {
               const char *const value = *value_ptr;

               bson_error_t error;
               mongoc_uri_t *const uri = mongoc_uri_new_with_error (
                  tmp_str ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                           "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=CANONICALIZE_HOST_NAME:%s",
                           value),
                  &error);
               ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
               ASSERT_OR_PRINT (uri, error);

               bson_t props;
               ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
               ASSERT_MATCH (&props, "{'CANONICALIZE_HOST_NAME': '%s'}", value);

               bson_destroy (&props);
               mongoc_uri_destroy (uri);
            }
         }

         {
            // CDRIVER-4128: only legacy boolean values are currently supported.
            static const char *const values[] = {"none", "forward", "forwardAndReverse", NULL};

            for (const char *const *value_ptr = values; *value_ptr; ++value_ptr) {
               const char *const value = *value_ptr;
               bson_error_t error;
               mongoc_uri_t *const uri = mongoc_uri_new_with_error (
                  tmp_str ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                           "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=CANONICALIZE_HOST_NAME:%s",
                           value),
                  &error);
               ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
               ASSERT (!uri);
               ASSERT_ERROR_CONTAINS (error,
                                      MONGOC_ERROR_COMMAND,
                                      MONGOC_ERROR_COMMAND_INVALID_ARG,
                                      "'GSSAPI' authentication mechanism requires CANONICALIZE_HOST_NAME is either "
                                      "\"true\" or \"false\"");

               mongoc_uri_destroy (uri);
            }
         }
      }

      // SERVICE_REALM: Drivers MAY allow the user to specify a different realm for the service.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                                       "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=SERVICE_REALM:realm",
                                       &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);

         bson_t props;
         ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
         ASSERT_MATCH (&props, "{'SERVICE_NAME': 'mongodb', 'SERVICE_REALM': 'realm'}");

         bson_destroy (&props);
         mongoc_uri_destroy (uri);
      }

      // SERVICE_HOST: Drivers MAY allow the user to specify a different host for the service.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                                       "=GSSAPI&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=SERVICE_HOST:host",
                                       &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);

         bson_t props;
         ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
         ASSERT_MATCH (&props, "{'SERVICE_NAME': 'mongodb', 'SERVICE_HOST': 'host'}");

         bson_destroy (&props);
         mongoc_uri_destroy (uri);
      }
   }

   // Authentication spec: source: MUST be "$external". Defaults to "$external".
   _auth_mechanism_source_external_only ("GSSAPI", "user@", "");
}

static void
test_mongoc_uri_auth_mechanism_plain (void)
{
   // Authentication spec: username: MUST be specified and non-zero length.
   _auth_mechanism_username_required ("PLAIN");

   // Authentication spec: password: MUST be specified.
   _auth_mechanism_password_required ("PLAIN");

   // Authentication spec: source: MUST be specified. Defaults to the database name if supplied on the connection
   // string or "$external".
   {
      // None (default).
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM "=PLAIN", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
         mongoc_uri_destroy (uri);
      }

      // Database name.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/db?" MONGOC_URI_AUTHMECHANISM "=PLAIN", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "db");
         mongoc_uri_destroy (uri);
      }

      // `authSource` (highest precedence).
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error (
            "mongodb://user:pass@localhost/db?" MONGOC_URI_AUTHMECHANISM "=PLAIN&" MONGOC_URI_AUTHSOURCE "=source",
            &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");
         mongoc_uri_destroy (uri);
      }
   }
}

static void
test_mongoc_uri_auth_mechanism_scram_sha_1 (void)
{
   const char *const mechanism = "SCRAM-SHA-1";

   // Authentication spec: username: MUST be specified and non-zero length.
   _auth_mechanism_username_required (mechanism);

   // Authentication spec: password: MUST be specified.
   _auth_mechanism_password_required (mechanism);

   // Authentication spec: source: MUST be specified. Defaults to the database name if supplied on the connection
   // string or "admin".
   _auth_mechanism_source_default_db_or_admin (mechanism);
}

static void
test_mongoc_uri_auth_mechanism_scram_sha_256 (void)
{
   const char *const mechanism = "SCRAM-SHA-256";

   // Authentication spec: username: MUST be specified and non-zero length.
   _auth_mechanism_username_required (mechanism);

   // Authentication spec: password: MUST be specified.
   _auth_mechanism_password_required (mechanism);

   // Authentication spec: source: MUST be specified. Defaults to the database name if supplied on the connection
   // string or "admin".
   _auth_mechanism_source_default_db_or_admin (mechanism);
}

static void
test_mongoc_uri_auth_mechanism_mongodb_aws (void)
{
   // Authentication spec: username: MAY be specified.
   // Authentication spec: if a username is provided without a password (or vice-versa) or if only a session token is
   // provided Drivers MUST raise an error.
   {
      // None.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-AWS", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_username (uri), NULL);
         mongoc_uri_destroy (uri);
      }

      // Empty.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://:@localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-AWS", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT (!uri);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                "'MONGODB-AWS' authentication mechanism requires a non-empty username");
      }

      // Normal.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:@localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-AWS", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_username (uri), "user");
         mongoc_uri_destroy (uri);
      }
   }

   // Authentication spec: password: MAY be specified.
   // Authentication spec: if a username is provided without a password (or vice-versa) or if only a session token is
   // provided Drivers MUST raise an error.
   {
      // None.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user@localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-AWS", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT (!uri);
         ASSERT_ERROR_CONTAINS (
            error,
            MONGOC_ERROR_COMMAND,
            MONGOC_ERROR_COMMAND_INVALID_ARG,
            "'MONGODB-AWS' authentication mechanism does not accept a username or a password without the other");
         mongoc_uri_destroy (uri);
      }

      // Empty.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:@localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-AWS", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         mongoc_uri_destroy (uri);
      }

      // Normal.
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error (
            "mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-AWS", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         mongoc_uri_destroy (uri);
      }
   }

   // mechanism_properties are allowed.
   {
      _auth_mechanism_properties_allowed ("MONGODB-AWS", "", NULL);

      // AWS_SESSION_TOKEN: Drivers MUST allow the user to specify an AWS session token for authentication with
      // temporary credentials.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                                       "=MONGODB-AWS&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=AWS_SESSION_TOKEN:token",
                                       &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);

         bson_t props;
         ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
         ASSERT_EQUAL_BSON (tmp_bson ("{'AWS_SESSION_TOKEN': 'token'}"), &props);

         bson_destroy (&props);
         mongoc_uri_destroy (uri);
      }

      // AWS_SESSION_TOKEN: naming of mechanism properties MUST be case-insensitive.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM
                                       "=MONGODB-AWS&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=aws_session_token:token",
                                       &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);

         bson_t props;
         ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
         ASSERT_EQUAL_BSON (tmp_bson ("{'aws_session_token': 'token'}"), &props);

         bson_destroy (&props);
         mongoc_uri_destroy (uri);
      }
   }

   // Authentication spec: source: MUST be "$external". Defaults to "$external".
   _auth_mechanism_source_external_only ("MONGODB-AWS", "", "");
}

static void
test_mongoc_uri_auth_mechanisms (void)
{
   capture_logs (true);

   // No username or mechanism means no authentication, even if auth fields are present.
   {
      // Authentication spec: the presence of a database name in the URI connection string MUST NOT be interpreted as a
      // user configuring authentication credentials.
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://localhost/db", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_WITH_MSG (!mongoc_uri_get_auth_mechanism (uri),
                          "expected no authMechanism, got %s",
                          mongoc_uri_get_auth_mechanism (uri));
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "db"); // Default.
         mongoc_uri_destroy (uri);
      }

      // Authentication spec: the presence of the authSource option in the URI connection string without other
      // credential data such as Userinfo or authentication parameters in connection options MUST NOT be interpreted as
      // a request for authentication.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHSOURCE "=source", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_WITH_MSG (!mongoc_uri_get_auth_mechanism (uri),
                          "expected no authMechanism, got %s",
                          mongoc_uri_get_auth_mechanism (uri));
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");
         mongoc_uri_destroy (uri);
      }

      // For backward compatibility, `mongoc_uri_get_auth_source` always returns "admin" when no `authMechanism`,
      // database name, or `authSource` is specified (consistent with default authentication method selecting
      // SCRAM-SHA-1 or SCRAM-SHA-256).
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://localhost/", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_WITH_MSG (!mongoc_uri_get_auth_mechanism (uri),
                          "expected no authMechanism, got %s",
                          mongoc_uri_get_auth_mechanism (uri));
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "admin");
         mongoc_uri_destroy (uri);
      }

      // For backward compatibility, `mongoc_uri_get_auth_source` uses the database name when no `authMechanism` or
      // `authSource` is specified (consistent with default authentication method selecting SCRAM-SHA-1 or
      // SCRAM-SHA-256).
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://user:pass@localhost/db", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_WITH_MSG (!mongoc_uri_get_auth_mechanism (uri),
                          "expected no authMechanism, got %s",
                          mongoc_uri_get_auth_mechanism (uri));
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "db");
         mongoc_uri_destroy (uri);
      }

      // For backward compatibility, `mongoc_uri_get_auth_source` uses `authSource` when specified (consistent with
      // default authentication method selecting SCRAM-SHA-1 or SCRAM-SHA-256).
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://user:pass@localhost/db?" MONGOC_URI_AUTHSOURCE "=source", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_WITH_MSG (!mongoc_uri_get_auth_mechanism (uri),
                          "expected no authMechanism, got %s",
                          mongoc_uri_get_auth_mechanism (uri));
         ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");
         mongoc_uri_destroy (uri);
      }

      // `authMechanismProperties` should not be validated without an `authMechanism`.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHMECHANISMPROPERTIES "=x:1", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_WITH_MSG (!mongoc_uri_get_auth_mechanism (uri),
                          "expected no authMechanism, got %s",
                          mongoc_uri_get_auth_mechanism (uri));
         mongoc_uri_destroy (uri);
      }
   }

   // Warn for invalid or unsupported `authMechanism` values.
   {
      // Empty.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHMECHANISM "=", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT (!uri);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                "Unsupported value for authMechanism '': must be one of ['MONGODB-OIDC', "
                                "'SCRAM-SHA-1', 'SCRAM-SHA-256', 'PLAIN', 'MONGODB-X509', 'GSSAPI', 'MONGODB-AWS']");
         mongoc_uri_destroy (uri);
      }

      // Case-insensitivity.
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error (
            "mongodb://user:pass@localhost/?" MONGOC_URI_AUTHMECHANISM "=scram-sha-1", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_OR_PRINT (uri, error);
         ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "scram-sha-1");
         mongoc_uri_destroy (uri);
      }

      // No substring comparison.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHMECHANISM "=SCRAM", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT (!uri);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                "Unsupported value for authMechanism 'SCRAM': must be one of ['MONGODB-OIDC', "
                                "'SCRAM-SHA-1', 'SCRAM-SHA-256', 'PLAIN', 'MONGODB-X509', 'GSSAPI', 'MONGODB-AWS']");
         mongoc_uri_destroy (uri);
      }

      // Unsupported.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHMECHANISM "=MONGODB-CR", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT (!uri);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                "Unsupported value for authMechanism 'MONGODB-CR': must be one of ['MONGODB-OIDC', "
                                "'SCRAM-SHA-1', 'SCRAM-SHA-256', 'PLAIN', 'MONGODB-X509', 'GSSAPI', 'MONGODB-AWS']");
         mongoc_uri_destroy (uri);
      }
   }

   // Default Authentication Mechanism
   {
      // Authentication spec: the presence of a credential delimiter (i.e. @) in the URI connection string is evidence
      // that the user has unambiguously specified user information and MUST be interpreted as a user configuring
      // authentication credentials (even if the username and/or password are empty strings).
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://username@localhost/", &error);
         ASSERT_OR_PRINT (uri, error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_CMPSTR (mongoc_uri_get_username (uri), "username");

         // For backward compatibility, do not forbid missing or empty password even if default authentication method
         // can only resolve to SCRAM-SHA-1 or SCRAM-SHA-256, both of which require a non-empty password.
         ASSERT_CMPSTR (mongoc_uri_get_password (uri), NULL);

         mongoc_uri_destroy (uri);
      }

      // Presence of `:` is interpreted as specifying a password, even if empty.
      {
         bson_error_t error;
         mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://username:@localhost/", &error);
         ASSERT_OR_PRINT (uri, error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT_CMPSTR (mongoc_uri_get_username (uri), "username");

         // For backward compatibility, do not forbid missing or empty password even if default authentication method
         // can only resolve to SCRAM-SHA-1 or SCRAM-SHA-256, both of which require a non-empty password.
         ASSERT_CMPSTR (mongoc_uri_get_password (uri), "");

         mongoc_uri_destroy (uri);
      }

      // Satisfy Connection String spec test: "must raise an error when the authSource is empty".
      // This applies even before determining whether or not authentication is required.
      {
         bson_error_t error;
         mongoc_uri_t *const uri =
            mongoc_uri_new_with_error ("mongodb://localhost/?" MONGOC_URI_AUTHSOURCE "=", &error);
         ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
         ASSERT (!uri);
         ASSERT_ERROR_CONTAINS (error,
                                MONGOC_ERROR_COMMAND,
                                MONGOC_ERROR_COMMAND_INVALID_ARG,
                                "authSource may not be specified as an empty string");
      }
   }

   test_mongoc_uri_auth_mechanism_mongodb_x509 ();
   test_mongoc_uri_auth_mechanism_gssapi ();
   test_mongoc_uri_auth_mechanism_plain ();
   test_mongoc_uri_auth_mechanism_scram_sha_1 ();
   test_mongoc_uri_auth_mechanism_scram_sha_256 ();
   test_mongoc_uri_auth_mechanism_mongodb_aws ();

   capture_logs (false);
}


static void
test_mongoc_uri_functions (void)
{
   mongoc_client_t *client;
   mongoc_uri_t *uri;
   mongoc_database_t *db;
   int32_t i;

   uri = mongoc_uri_new ("mongodb://foo:bar@localhost:27017/baz?" MONGOC_URI_AUTHSOURCE "=source");

   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "foo");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "bar");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "baz");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");

   mongoc_uri_set_username (uri, "longer username that should work");
   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "longer username that should work");

   mongoc_uri_set_password (uri, "longer password that should also work");
   ASSERT_CMPSTR (mongoc_uri_get_password (uri), "longer password that should also work");

   mongoc_uri_set_database (uri, "longer database that should work");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "longer database that should work");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "source");

   mongoc_uri_set_auth_source (uri, "longer authsource that should work");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "longer authsource that should work");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "longer database that should work");

   client = test_framework_client_new_from_uri (uri, NULL);
   mongoc_uri_destroy (uri);

   ASSERT_CMPSTR (mongoc_uri_get_username (client->uri), "longer username that should work");
   ASSERT_CMPSTR (mongoc_uri_get_password (client->uri), "longer password that should also work");
   ASSERT_CMPSTR (mongoc_uri_get_database (client->uri), "longer database that should work");
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (client->uri), "longer authsource that should work");
   mongoc_client_destroy (client);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_SERVERSELECTIONTIMEOUTMS "=3"
                         "&" MONGOC_URI_JOURNAL "=true"
                         "&" MONGOC_URI_WTIMEOUTMS "=42"
                         "&" MONGOC_URI_CANONICALIZEHOSTNAME "=false");
   ASSERT_CAPTURED_LOG ("mongoc_uri_new",
                        MONGOC_LOG_LEVEL_WARNING,
                        MONGOC_URI_CANONICALIZEHOSTNAME " is deprecated, use " MONGOC_URI_AUTHMECHANISMPROPERTIES
                                                        " with CANONICALIZE_HOST_NAME instead");

   ASSERT_CMPINT (mongoc_uri_get_option_as_int32 (uri, "serverselectiontimeoutms", 18), ==, 3);
   ASSERT (mongoc_uri_set_option_as_int32 (uri, "serverselectiontimeoutms", 18));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, "serverselectiontimeoutms", 19), ==, 18);

   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 18), ==, 42);
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 18), ==, 42);
   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 18));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 19), ==, 18);

   ASSERT (mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 20));
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 19), ==, 20);

   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 500));

   i = mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 1000);

   ASSERT_CMPINT32 (i, ==, 500);

   capture_logs (true);

   /* Server Discovery and Monitoring Spec: "the driver MUST NOT permit users to
    * configure it less than minHeartbeatFrequencyMS (500ms)." */
   ASSERT (!mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 499));

   ASSERT_CAPTURED_LOG ("mongoc_uri_set_option_as_int32",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Invalid \"heartbeatfrequencyms\" of 499: must be at least 500");

   /* socketcheckintervalms isn't set, return our fallback */
   ASSERT_CMPINT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 123), ==, 123);
   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 18));
   ASSERT_CMPINT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 19), ==, 18);

   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_JOURNAL, false));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_CANONICALIZEHOSTNAME, true));
   /* tls isn't set, return out fallback */
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLS, true));

   client = test_framework_client_new_from_uri (uri, NULL);
   mongoc_uri_destroy (uri);

   ASSERT (mongoc_uri_get_option_as_bool (client->uri, MONGOC_URI_JOURNAL, false));
   /* tls isn't set, return out fallback */
   ASSERT (mongoc_uri_get_option_as_bool (client->uri, MONGOC_URI_TLS, true));
   mongoc_client_destroy (client);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, "replicaset", "default"), "default");
   ASSERT (mongoc_uri_set_option_as_utf8 (uri, "replicaset", "value"));
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, "replicaset", "default"), "value");

   mongoc_uri_destroy (uri);


   uri =
      mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_SOCKETTIMEOUTMS "=1&" MONGOC_URI_SOCKETCHECKINTERVALMS "=200");
   ASSERT_CMPINT32 (1, ==, mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 0));
   ASSERT_CMPINT32 (200, ==, mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 0));

   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 2);
   ASSERT_CMPINT32 (2, ==, mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 0));

   mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 202);
   ASSERT_CMPINT32 (202, ==, mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 0));


   client = test_framework_client_new_from_uri (uri, NULL);
   ASSERT_CMPINT32 (2, ==, client->cluster.sockettimeoutms);
   ASSERT_CMPINT32 (202, ==, client->cluster.socketcheckintervalms);

   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://host/dbname0");
   ASSERT_CMPSTR (mongoc_uri_get_database (uri), "dbname0");
   mongoc_uri_set_database (uri, "dbname1");
   client = test_framework_client_new_from_uri (uri, NULL);
   db = mongoc_client_get_default_database (client);
   ASSERT_CMPSTR (mongoc_database_get_name (db), "dbname1");

   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://%2Ftmp%2FMongoDB-27017.sock/");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host, "/tmp/MongoDB-27017.sock");
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, "/tmp/MongoDB-27017.sock");

   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://host/?foobar=1");
   ASSERT (uri);
   ASSERT_CAPTURED_LOG ("setting URI option foobar=1", MONGOC_LOG_LEVEL_WARNING, "Unsupported URI option \"foobar\"");

   mongoc_uri_destroy (uri);
}

#define BSON_ERROR_INIT ((bson_error_t) {.code = 0u, .domain = 0u, .message = {0}, .reserved = 0u})

static void
test_mongoc_uri_new_with_error (void)
{
   bson_error_t error = {0};
   mongoc_uri_t *uri;

   capture_logs (true);
   ASSERT (!mongoc_uri_new_with_error ("mongodb://", NULL));
   uri = mongoc_uri_new_with_error ("mongodb://localhost", NULL);
   ASSERT (uri);
   mongoc_uri_destroy (uri);

   ASSERT (!mongoc_uri_new_with_error ("mongodb://", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Host list of URI string cannot be empty");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongo://localhost", &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid URI scheme \"mongo://\". Expected one of \"mongodb://\" or \"mongodb+srv://\"");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/?readPreference=unknown", &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Unsupported readPreference value [readPreference=unknown]");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/"
                                       "?appname="
                                       "WayTooLongAppnameToBeValidSoThisShouldResultInAnErrorWayToLongAppnameToB"
                                       "eValidSoThisShouldResultInAnErrorWayToLongAppnameToBeValidSoThisShouldRe"
                                       "sultInAnError",
                                       &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Unsupported value for \"appname\""); /* ... */

   uri = mongoc_uri_new ("mongodb://localhost");
   ASSERT (!mongoc_uri_set_option_as_utf8 (uri,
                                           MONGOC_URI_APPNAME,
                                           "WayTooLongAppnameToBeValidSoThisShouldResultInAnErrorWayToLongAppnameToB"
                                           "eValidSoThisShouldResultInAnErrorWayToLongAppnameToBeValidSoThisShouldRe"
                                           "sultInAnError"));
   mongoc_uri_destroy (uri);

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://user%p:pass@localhost/", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid %-encoding in username in URI string");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://l%oc, alhost/", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid host specifier \"l%oc\"");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb:///tmp/mongodb.sock", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Host list of URI string cannot be empty");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/db.na%me", &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid %-sequence \"%me\"");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/db?journal=true&w=0", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Journal conflicts with w value [w=0]");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/db?w=-5", &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Unsupported w value [w=-5]");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/db?heartbeatfrequencyms=10", &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid \"heartbeatfrequencyms\" of 10: must be at least 500");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/db?zlibcompressionlevel=10", &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid \"zlibcompressionlevel\" of 10: must be between -1 and 9");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb+srv://", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Missing service name in SRV URI");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb+srv://%", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid SRV service name \"%\" in URI");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb+srv://x", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid SRV service name \"x\" in URI");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb+srv://x.y", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid SRV service name \"x.y\" in URI");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb+srv://a.b.c,d.e.f", &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Multiple service names are prohibited in an SRV URI");

   error = BSON_ERROR_INIT;
   ASSERT (!mongoc_uri_new_with_error ("mongodb+srv://a.b.c:8000", &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Port numbers are prohibited in an SRV URI");
}


#undef ASSERT_SUPPRESS


static void
test_mongoc_uri_compound_setters (void)
{
   mongoc_uri_t *uri;
   mongoc_read_prefs_t *prefs;
   const mongoc_read_prefs_t *prefs_result;
   mongoc_read_concern_t *rc;
   const mongoc_read_concern_t *rc_result;
   mongoc_write_concern_t *wc;
   const mongoc_write_concern_t *wc_result;

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS
                         "=dc:ny&" MONGOC_URI_READCONCERNLEVEL "=majority&" MONGOC_URI_W "=3");

   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_uri_set_read_prefs_t (uri, prefs);
   prefs_result = mongoc_uri_get_read_prefs_t (uri);
   ASSERT_CMPINT ((int) mongoc_read_prefs_get_mode (prefs_result), ==, MONGOC_READ_SECONDARY);
   ASSERT_EQUAL_BSON (tmp_bson ("{}"), mongoc_read_prefs_get_tags (prefs_result));

   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, "whatever");
   mongoc_uri_set_read_concern (uri, rc);
   rc_result = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc_result), "whatever");

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 2);
   mongoc_uri_set_write_concern (uri, wc);
   wc_result = mongoc_uri_get_write_concern (uri);
   ASSERT_CMPINT32 (mongoc_write_concern_get_w (wc_result), ==, 2);

   mongoc_read_prefs_destroy (prefs);
   mongoc_read_concern_destroy (rc);
   mongoc_write_concern_destroy (wc);
   mongoc_uri_destroy (uri);
}


static void
test_mongoc_host_list_from_string (void)
{
   mongoc_host_list_t host_list = {0};

   /* shouldn't be parsable */
   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, ":27017"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Could not parse address");
   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, "example.com:"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Could not parse address");
   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, "localhost:999999999"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Could not parse address");
   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, "::1234"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Could not parse address");

   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, "]:1234"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Could not parse address");

   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, "[]:1234"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Could not parse address");

   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, "[::1] foo"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Could not parse address");

   capture_logs (true);
   ASSERT (!_mongoc_host_list_from_string (&host_list, "[::1]extra_chars:27017"));
   ASSERT_CAPTURED_LOG ("_mongoc_host_list_from_string", MONGOC_LOG_LEVEL_ERROR, "Invalid trailing content");

   /* normal parsing, host and port are split, host is downcased */
   ASSERT (_mongoc_host_list_from_string (&host_list, "localHOST:27019"));
   ASSERT_CMPSTR (host_list.host_and_port, "localhost:27019");
   ASSERT_CMPSTR (host_list.host, "localhost");
   ASSERT_CMPUINT16 (host_list.port, ==, 27019);
   ASSERT (!host_list.next);

   ASSERT (_mongoc_host_list_from_string (&host_list, "localhost"));
   ASSERT_CMPSTR (host_list.host_and_port, "localhost:27017");
   ASSERT_CMPSTR (host_list.host, "localhost");
   ASSERT_CMPUINT16 (host_list.port, ==, 27017);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[::1]"));
   ASSERT_CMPSTR (host_list.host_and_port, "[::1]:27017");
   ASSERT_CMPSTR (host_list.host, "::1"); /* no "[" or "]" */
   ASSERT_CMPUINT16 (host_list.port, ==, 27017);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[Fe80::1]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[fe80::1]:1234");
   ASSERT_CMPSTR (host_list.host, "fe80::1");
   ASSERT_CMPUINT16 (host_list.port, ==, 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[fe80::1%lo0]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[fe80::1%lo0]:1234");
   ASSERT_CMPSTR (host_list.host, "fe80::1%lo0");
   ASSERT_CMPUINT16 (host_list.port, ==, 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[fe80::1%lo0]:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[fe80::1%lo0]:1234");
   ASSERT_CMPSTR (host_list.host, "fe80::1%lo0");
   ASSERT_CMPUINT16 (host_list.port, ==, 1234);

   /* preserves case */
   ASSERT (_mongoc_host_list_from_string (&host_list, "/Path/to/file.sock"));
   ASSERT_CMPSTR (host_list.host_and_port, "/Path/to/file.sock");
   ASSERT_CMPSTR (host_list.host, "/Path/to/file.sock");

   /* weird cases that should still parse, without crashing */
   ASSERT (_mongoc_host_list_from_string (&host_list, "/Path/to/file.sock:1"));
   ASSERT_CMPSTR (host_list.host, "/Path/to/file.sock");
   ASSERT_CMPINT (host_list.family, ==, AF_UNIX);

   ASSERT (_mongoc_host_list_from_string (&host_list, " :1234"));
   ASSERT_CMPSTR (host_list.host_and_port, " :1234");
   ASSERT_CMPSTR (host_list.host, " ");
   ASSERT_CMPUINT16 (host_list.port, ==, 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[:1234"));
   ASSERT_CMPSTR (host_list.host_and_port, "[:1234");
   ASSERT_CMPSTR (host_list.host, "[");
   ASSERT_CMPUINT16 (host_list.port, ==, 1234);

   ASSERT (_mongoc_host_list_from_string (&host_list, "[:]"));
   ASSERT_CMPSTR (host_list.host_and_port, "[:]:27017");
   ASSERT_CMPSTR (host_list.host, ":");
   ASSERT_CMPUINT16 (host_list.port, ==, 27017);
}


static void
test_mongoc_uri_new_for_host_port (void)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_new_for_host_port ("uber", 555);
   ASSERT (uri);
   ASSERT_CMPSTR ("uber", mongoc_uri_get_hosts (uri)->host);
   ASSERT_CMPSTR ("uber:555", mongoc_uri_get_hosts (uri)->host_and_port);
   ASSERT_CMPUINT16 (555, ==, mongoc_uri_get_hosts (uri)->port);
   mongoc_uri_destroy (uri);
}

static void
test_mongoc_uri_compressors (void)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_new ("mongodb://localhost/");

   ASSERT_EQUAL_BSON (tmp_bson ("{}"), mongoc_uri_get_compressors (uri));

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   capture_logs (true);
   mongoc_uri_set_compressors (uri, "snappy,unknown");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'snappy': {'$exists': true}, 'unknown': {'$exists': false}}");
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors", MONGOC_LOG_LEVEL_WARNING, "Unsupported compressor: 'unknown'");
#endif


#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   capture_logs (true);
   mongoc_uri_set_compressors (uri, "snappy");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'snappy': {'$exists': true}, 'unknown': {'$exists': false}}");
   ASSERT_NO_CAPTURED_LOGS ("snappy uri");

   /* Overwrite the previous URI, effectively disabling snappy */
   capture_logs (true);
   mongoc_uri_set_compressors (uri, "unknown");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'snappy': {'$exists': false}, 'unknown': {'$exists': false}}");
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors", MONGOC_LOG_LEVEL_WARNING, "Unsupported compressor: 'unknown'");
#endif

   capture_logs (true);
   mongoc_uri_set_compressors (uri, "");
   ASSERT_EQUAL_BSON (tmp_bson ("{}"), mongoc_uri_get_compressors (uri));
   ASSERT_NO_CAPTURED_LOGS ("Disable compression with empty string");


   /* Disable compression */
   capture_logs (true);
   mongoc_uri_set_compressors (uri, NULL);
   ASSERT_EQUAL_BSON (tmp_bson ("{}"), mongoc_uri_get_compressors (uri));
   ASSERT_NO_CAPTURED_LOGS ("Disable compression");


   mongoc_uri_destroy (uri);


#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=snappy");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'snappy': {'$exists': true}}");
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=snappy,somethingElse");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri),
                 "{'snappy': {'$exists': true}, 'somethingElse': {'$exists': false}}");
   ASSERT_CAPTURED_LOG (
      "mongoc_uri_set_compressors", MONGOC_LOG_LEVEL_WARNING, "Unsupported compressor: 'somethingElse'");
   mongoc_uri_destroy (uri);
#endif


#ifdef MONGOC_ENABLE_COMPRESSION_ZLIB

#ifdef MONGOC_ENABLE_COMPRESSION_SNAPPY
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=snappy,zlib");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'snappy': {'$exists': true}, 'zlib': {'$exists': true}}");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_compressors (uri, "snappy,zlib"));
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'snappy': {'$exists': true}, 'zlib': {'$exists': true}}");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_compressors (uri, "zlib"));
   ASSERT (mongoc_uri_set_compressors (uri, "snappy"));
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'snappy': {'$exists': true}, 'zlib': {'$exists': false}}");
   mongoc_uri_destroy (uri);
#endif

   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'zlib': {'$exists': true}}");
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib,somethingElse");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'zlib': {'$exists': true}, 'somethingElse': {'$exists': false}}");
   ASSERT_CAPTURED_LOG (
      "mongoc_uri_set_compressors", MONGOC_LOG_LEVEL_WARNING, "Unsupported compressor: 'somethingElse'");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib&zlibCompressionLevel=-1");
   ASSERT_MATCH (mongoc_uri_get_compressors (uri), "{'zlib': {'$exists': true}}");
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 1), ==, -1);
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib&zlibCompressionLevel=9");
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 1), ==, 9);
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib&zlibCompressionLevel=-2");
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Invalid \"zlibcompressionlevel\" of -2: must be between -1 and 9");
   mongoc_uri_destroy (uri);

   capture_logs (true);
   uri = mongoc_uri_new ("mongodb://localhost/?compressors=zlib&zlibCompressionLevel=10");
   ASSERT_CAPTURED_LOG ("mongoc_uri_set_compressors",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Invalid \"zlibcompressionlevel\" of 10: must be between -1 and 9");
   mongoc_uri_destroy (uri);

#endif
}

static void
test_mongoc_uri_unescape (void)
{
#define ASSERT_URIDECODE_STR(_s, _e)        \
   do {                                     \
      char *str = mongoc_uri_unescape (_s); \
      ASSERT (!strcmp (str, _e));           \
      bson_free (str);                      \
   } while (0)
#define ASSERT_URIDECODE_FAIL(_s)                                                         \
   do {                                                                                   \
      char *str;                                                                          \
      capture_logs (true);                                                                \
      str = mongoc_uri_unescape (_s);                                                     \
      ASSERT (!str);                                                                      \
      ASSERT_CAPTURED_LOG ("uri", MONGOC_LOG_LEVEL_WARNING, "Invalid % escape sequence"); \
   } while (0)

   ASSERT_URIDECODE_STR ("", "");
   ASSERT_URIDECODE_STR ("%40", "@");
   ASSERT_URIDECODE_STR ("me%40localhost@localhost", "me@localhost@localhost");
   ASSERT_URIDECODE_STR ("%20", " ");
   ASSERT_URIDECODE_STR ("%24%21%40%2A%26%5E%21%40%2A%23%26%5E%21%40%23%2A%26"
                         "%5E%21%40%2A%23%26%5E%21%40%2A%26%23%5E%7D%7B%7D%7B"
                         "%22%22%27%7D%7B%5B%5D%3C%3E%3F",
                         "$!@*&^!@*#&^!@#*&^!@*#&^!@*&#^}{}{\"\"'}{[]<>?");

   ASSERT_URIDECODE_FAIL ("%");
   ASSERT_URIDECODE_FAIL ("%%");
   ASSERT_URIDECODE_FAIL ("%%%");
   ASSERT_URIDECODE_FAIL ("%FF");
   ASSERT_URIDECODE_FAIL ("%CC");
   ASSERT_URIDECODE_FAIL ("%00");

#undef ASSERT_URIDECODE_STR
#undef ASSERT_URIDECODE_FAIL
}


typedef struct {
   const char *uri;
   bool parses;
   mongoc_read_mode_t mode;
   bson_t *tags;
   const char *log_msg;
} read_prefs_test;


static void
test_mongoc_uri_read_prefs (void)
{
   const mongoc_read_prefs_t *rp;
   mongoc_uri_t *uri;
   const read_prefs_test *t;
   int i;

   bson_t *tags_dcny = BCON_NEW ("0", "{", "dc", "ny", "}");
   bson_t *tags_dcny_empty = BCON_NEW ("0", "{", "dc", "ny", "}", "1", "{", "}");
   bson_t *tags_dcnyusessd_dcsf_empty =
      BCON_NEW ("0", "{", "dc", "ny", "use", "ssd", "}", "1", "{", "dc", "sf", "}", "2", "{", "}");
   bson_t *tags_empty = BCON_NEW ("0", "{", "}");

   const char *conflicts = "Invalid readPreferences";

   const read_prefs_test tests[] = {
      {
         .uri = "mongodb://localhost/",
         .parses = true,
         .mode = MONGOC_READ_PRIMARY,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=primary",
         .parses = true,
         .mode = MONGOC_READ_PRIMARY,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=primaryPreferred",
         .parses = true,
         .mode = MONGOC_READ_PRIMARY_PREFERRED,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=secondary",
         .parses = true,
         .mode = MONGOC_READ_SECONDARY,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=secondaryPreferred",
         .parses = true,
         .mode = MONGOC_READ_SECONDARY_PREFERRED,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest",
         .parses = true,
         .mode = MONGOC_READ_NEAREST,
      },
      {
         /* MONGOC_URI_READPREFERENCETAGS conflict with primary mode */
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCETAGS "=",
         .parses = false,
         .mode = MONGOC_READ_PRIMARY,
         .log_msg = conflicts,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=primary&" MONGOC_URI_READPREFERENCETAGS "=",
         .parses = false,
         .mode = MONGOC_READ_PRIMARY,
         .log_msg = conflicts,
      },
      {
         .uri =
            "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=secondaryPreferred&" MONGOC_URI_READPREFERENCETAGS "=",
         .parses = true,
         .mode = MONGOC_READ_SECONDARY_PREFERRED,
         .tags = tags_empty,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=secondaryPreferred&" MONGOC_URI_READPREFERENCETAGS
                "=dc:ny",
         .parses = true,
         .mode = MONGOC_READ_SECONDARY_PREFERRED,
         .tags = tags_dcny,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS
                "=dc:ny&" MONGOC_URI_READPREFERENCETAGS "=",
         .parses = true,
         .mode = MONGOC_READ_NEAREST,
         .tags = tags_dcny_empty,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS
                "=dc:ny,use:ssd&" MONGOC_URI_READPREFERENCETAGS "=dc:sf&" MONGOC_URI_READPREFERENCETAGS "=",
         .parses = true,
         .mode = MONGOC_READ_NEAREST,
         .tags = tags_dcnyusessd_dcsf_empty,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS "=foo",
         .parses = false,
         .mode = MONGOC_READ_NEAREST,
         .log_msg = "Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS "\": \"foo\"",
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS "=foo,bar",
         .parses = false,
         .mode = MONGOC_READ_NEAREST,
         .log_msg = "Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS "\": \"foo,bar\"",
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_READPREFERENCE "=nearest&" MONGOC_URI_READPREFERENCETAGS "=1",
         .parses = false,
         .mode = MONGOC_READ_NEAREST,
         .log_msg = "Unsupported value for \"" MONGOC_URI_READPREFERENCETAGS "\": \"1\"",
      },
      {0}};

   for (i = 0; tests[i].uri; i++) {
      t = &tests[i];

      capture_logs (true);
      uri = mongoc_uri_new (t->uri);
      if (t->parses) {
         ASSERT (uri);
         ASSERT_NO_CAPTURED_LOGS (t->uri);
      } else {
         ASSERT (!uri);
         if (t->log_msg) {
            ASSERT_CAPTURED_LOG (t->uri, MONGOC_LOG_LEVEL_WARNING, t->log_msg);
         }

         continue;
      }

      rp = mongoc_uri_get_read_prefs_t (uri);
      ASSERT (rp);

      ASSERT_CMPINT ((int) t->mode, ==, (int) mongoc_read_prefs_get_mode (rp));

      if (t->tags) {
         ASSERT_EQUAL_BSON (t->tags, mongoc_read_prefs_get_tags (rp));
      }

      mongoc_uri_destroy (uri);
   }

   bson_destroy (tags_dcny);
   bson_destroy (tags_dcny_empty);
   bson_destroy (tags_dcnyusessd_dcsf_empty);
   bson_destroy (tags_empty);
}


typedef struct {
   const char *uri;
   bool parses;
   int32_t w;
   const char *wtag;
   int64_t wtimeoutms;
   const char *log_msg;
} write_concern_test;


static void
test_mongoc_uri_write_concern (void)
{
   static const write_concern_test tests[] = {
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_SAFE "=false",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_SAFE "=true",
         .parses = true,
         .w = 1,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=-1",
         .parses = false,
         .log_msg = "Unsupported w value [w=-1]",
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=0",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=1",
         .parses = true,
         .w = 1,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=2",
         .parses = true,
         .w = 2,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=majority",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_MAJORITY,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=10",
         .parses = true,
         .w = 10,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_DEFAULT,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=mytag",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_TAG,
         .wtag = "mytag",
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=mytag&" MONGOC_URI_SAFE "=false",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_TAG,
         .wtag = "mytag",
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=1&" MONGOC_URI_SAFE "=false",
         .parses = true,
         .w = 1,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_JOURNAL "=true",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_DEFAULT,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=1&" MONGOC_URI_JOURNAL "=true",
         .parses = true,
         .w = 1,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=2&" MONGOC_URI_WTIMEOUTMS "=1000",
         .parses = true,
         .w = 2,
         .wtimeoutms = 1000,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=2&" MONGOC_URI_WTIMEOUTMS "=2147483648",
         .parses = true,
         .w = 2,
         .wtimeoutms = 2147483648LL,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=majority&" MONGOC_URI_WTIMEOUTMS "=1000",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_MAJORITY,
         .wtimeoutms = 1000,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=mytag&" MONGOC_URI_WTIMEOUTMS "=1000",
         .parses = true,
         .w = MONGOC_WRITE_CONCERN_W_TAG,
         .wtag = "mytag",
         .wtimeoutms = 1000,
      },
      {
         .uri = "mongodb://localhost/?" MONGOC_URI_W "=0&" MONGOC_URI_JOURNAL "=true",
         .parses = false,
         .w = MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED,
         .log_msg = "Journal conflicts with w value [" MONGOC_URI_W "=0]",
      },
      {0}};

   for (int i = 0; tests[i].uri; i++) {
      const write_concern_test *const t = &tests[i];

      capture_logs (true);
      mongoc_uri_t *const uri = mongoc_uri_new (t->uri);

      if (tests[i].log_msg) {
         ASSERT_CAPTURED_LOG (tests[i].uri, MONGOC_LOG_LEVEL_WARNING, tests[i].log_msg);
      } else {
         ASSERT_NO_CAPTURED_LOGS (tests[i].uri);
      }

      capture_logs (false); /* clear captured logs */

      if (t->parses) {
         ASSERT_WITH_MSG (uri, "expected the URI to be parsed as valid");
      } else {
         ASSERT_WITH_MSG (!uri, "expected the URI to be parsed as invalid");
         continue;
      }

      const mongoc_write_concern_t *const wr = mongoc_uri_get_write_concern (uri);
      ASSERT (wr);

      ASSERT_CMPINT32 (t->w, ==, mongoc_write_concern_get_w (wr));

      if (t->wtag) {
         ASSERT_CMPSTR (t->wtag, mongoc_write_concern_get_wtag (wr));
      }

      if (t->wtimeoutms) {
         ASSERT_CMPINT64 (t->wtimeoutms, ==, mongoc_write_concern_get_wtimeout_int64 (wr));
      }

      mongoc_uri_destroy (uri);
   }
}

static void
test_mongoc_uri_read_concern (void)
{
   const mongoc_read_concern_t *rc;
   mongoc_uri_t *uri;

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL "=majority");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "majority");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/"
                         "?" MONGOC_URI_READCONCERNLEVEL "=" MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "majority");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/"
                         "?" MONGOC_URI_READCONCERNLEVEL "=" MONGOC_READ_CONCERN_LEVEL_LINEARIZABLE);
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "linearizable");
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL "=local");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "local");
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL "=" MONGOC_READ_CONCERN_LEVEL_LOCAL);
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "local");
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL "=randomstuff");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "randomstuff");
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT (mongoc_read_concern_get_level (rc) == NULL);
   mongoc_uri_destroy (uri);


   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_READCONCERNLEVEL "=");
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT_CMPSTR (mongoc_read_concern_get_level (rc), "");
   mongoc_uri_destroy (uri);
}

static void
test_mongoc_uri_long_hostname (void)
{
   char *host;
   char *host_and_port;
   size_t len = BSON_HOST_NAME_MAX;
   char *uri_str;
   mongoc_uri_t *uri;

   /* hostname of exactly maximum length */
   host = bson_malloc (len + 1);
   memset (host, 'a', len);
   host[len] = '\0';
   host_and_port = bson_strdup_printf ("%s:12345", host);
   uri_str = bson_strdup_printf ("mongodb://%s", host_and_port);
   uri = mongoc_uri_new (uri_str);
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, host_and_port);

   mongoc_uri_destroy (uri);
   uri = mongoc_uri_new_for_host_port (host, 12345);
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_hosts (uri)->host_and_port, host_and_port);

   mongoc_uri_destroy (uri);
   bson_free (uri_str);
   bson_free (host_and_port);
   bson_free (host);

   /* hostname length exceeds maximum by one */
   len++;
   host = bson_malloc (len + 1);
   memset (host, 'a', len);
   host[len] = '\0';
   host_and_port = bson_strdup_printf ("%s:12345", host);
   uri_str = bson_strdup_printf ("mongodb://%s", host_and_port);

   capture_logs (true);
   ASSERT (!mongoc_uri_new (uri_str));
   ASSERT_CAPTURED_LOG ("mongoc_uri_new", MONGOC_LOG_LEVEL_WARNING, "too long");

   clear_captured_logs ();
   ASSERT (!mongoc_uri_new_for_host_port (host, 12345));
   ASSERT_CAPTURED_LOG ("mongoc_uri_new", MONGOC_LOG_LEVEL_WARNING, "too long");

   bson_free (uri_str);
   bson_free (host_and_port);
   bson_free (host);
}

static void
test_mongoc_uri_tls_ssl (const char *tls,
                         const char *tlsCertificateKeyFile,
                         const char *tlsCertificateKeyPassword,
                         const char *tlsCAFile,
                         const char *tlsAllowInvalidCertificates,
                         const char *tlsAllowInvalidHostnames)
{
   const char *tlsalt;
   char url_buffer[2048];
   mongoc_uri_t *uri;
   bson_error_t err;

   bson_snprintf (url_buffer,
                  sizeof (url_buffer),
                  "mongodb://CN=client,OU=kerneluser,O=10Gen,L=New York City,"
                  "ST=New York,C=US@ldaptest.10gen.cc/?"
                  "%s=true&authMechanism=MONGODB-X509&"
                  "%s=tests/x509gen/ldaptest-client-key-and-cert.pem&"
                  "%s=tests/x509gen/ldaptest-ca-cert.crt&"
                  "%s=true",
                  tls,
                  tlsCertificateKeyFile,
                  tlsCAFile,
                  tlsAllowInvalidHostnames);
   uri = mongoc_uri_new (url_buffer);

   ASSERT_CMPSTR (mongoc_uri_get_username (uri), "CN=client,OU=kerneluser,O=10Gen,L=New York City,ST=New York,C=US");
   ASSERT (!mongoc_uri_get_password (uri));
   ASSERT (!mongoc_uri_get_database (uri));
   ASSERT_CMPSTR (mongoc_uri_get_auth_source (uri), "$external");
   ASSERT_CMPSTR (mongoc_uri_get_auth_mechanism (uri), "MONGODB-X509");

   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "none"),
                  "tests/x509gen/ldaptest-client-key-and-cert.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, "none"),
                  "tests/x509gen/ldaptest-ca-cert.crt");
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (url_buffer,
                  sizeof (url_buffer),
                  "mongodb://localhost/?%s=true&%s=key.pem&%s=ca.pem",
                  tls,
                  tlsCertificateKeyFile,
                  tlsCAFile);
   uri = mongoc_uri_new (url_buffer);

   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, "none"), "key.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "none"), "key.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, "none"), "ca.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, "none"), "ca.pem");
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=true", tls);
   uri = mongoc_uri_new (url_buffer);

   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, "none"), "none");
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (url_buffer,
                  sizeof (url_buffer),
                  "mongodb://localhost/?%s=true&%s=pa$$word!&%s=encrypted.pem",
                  tls,
                  tlsCertificateKeyPassword,
                  tlsCertificateKeyFile);
   uri = mongoc_uri_new (url_buffer);

   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE, "none"), "encrypted.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "none"), "encrypted.pem");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD, "none"), "pa$$word!");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, "none"), "pa$$word!");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, "none"), "none");
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (
      url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=true&%s=true", tls, tlsAllowInvalidCertificates);
   uri = mongoc_uri_new (url_buffer);

   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, "none"), "none");
   ASSERT_CMPSTR (mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, "none"), "none");
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=foo.pem", tlsCertificateKeyFile);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);


   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=foo.pem", tlsCAFile);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);


   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=true", tlsAllowInvalidCertificates);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (mongoc_uri_get_tls (uri));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=true", tlsAllowInvalidHostnames);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (mongoc_uri_get_tls (uri));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, false));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (
      url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=false&%s=foo.pem", tls, tlsCertificateKeyFile);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (!mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);


   bson_snprintf (
      url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=false&%s=foo.pem", tls, tlsCertificateKeyFile);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (!mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);


   bson_snprintf (
      url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=false&%s=true", tls, tlsAllowInvalidCertificates);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (!mongoc_uri_get_tls (uri));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SSLALLOWINVALIDCERTIFICATES, false));
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));
   mongoc_uri_destroy (uri);


   bson_snprintf (
      url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=false&%s=false", tls, tlsAllowInvalidHostnames);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (!mongoc_uri_get_tls (uri));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SSLALLOWINVALIDHOSTNAMES, true));
   ASSERT (!mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, true));
   mongoc_uri_destroy (uri);

   if (!strcmp (tls, "ssl")) {
      tlsalt = "tls";
   } else {
      tlsalt = "ssl";
   }

   /* Mixing options okay so long as they match */
   capture_logs (true);
   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=true&%s=true", tls, tlsalt);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (mongoc_uri_get_option_as_bool (uri, tls, false));
   ASSERT_NO_CAPTURED_LOGS (url_buffer);
   mongoc_uri_destroy (uri);

   /* Same option with different values okay, latter overrides */
   capture_logs (true);
   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=true&%s=false", tls, tls);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (!mongoc_uri_get_option_as_bool (uri, tls, true));
   if (strcmp (tls, "tls")) {
      ASSERT_CAPTURED_LOG ("option: ssl", MONGOC_LOG_LEVEL_WARNING, "Overwriting previously provided value for 'ssl'");
   } else {
      ASSERT_CAPTURED_LOG ("option: tls", MONGOC_LOG_LEVEL_WARNING, "Overwriting previously provided value for 'tls'");
   }
   mongoc_uri_destroy (uri);

   /* Mixing options not okay if values differ */
   capture_logs (false);
   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb://localhost/?%s=true&%s=false", tls, tlsalt);
   uri = mongoc_uri_new_with_error (url_buffer, &err);
   if (strcmp (tls, "tls")) {
      ASSERT_ERROR_CONTAINS (err,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "Deprecated option 'ssl=true' conflicts with "
                             "canonical name 'tls=false'");
   } else {
      ASSERT_ERROR_CONTAINS (err,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "Deprecated option 'ssl=false' conflicts with "
                             "canonical name 'tls=true'");
   }
   mongoc_uri_destroy (uri);

   /* No conflict appears with implicit tls=true via SRV */
   capture_logs (false);
   bson_snprintf (url_buffer, sizeof (url_buffer), "mongodb+srv://a.b.c/?%s=foo.pem", tlsCAFile);
   uri = mongoc_uri_new (url_buffer);
   ASSERT (mongoc_uri_get_option_as_bool (uri, tls, false));
   mongoc_uri_destroy (uri);

   /* Set TLS options after creating mongoc_uri_t from connection string */
   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_option_as_utf8 (uri, tlsCertificateKeyFile, "/path/to/pem"));
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_option_as_utf8 (uri, tlsCertificateKeyPassword, "password"));
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_option_as_utf8 (uri, tlsCAFile, "/path/to/pem"));
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_option_as_bool (uri, tlsAllowInvalidCertificates, false));
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_option_as_bool (uri, tlsAllowInvalidHostnames, true));
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);
}

static void
test_mongoc_uri_tls (void)
{
   bson_error_t err = {0};
   mongoc_uri_t *uri;

   test_mongoc_uri_tls_ssl (MONGOC_URI_TLS,
                            MONGOC_URI_TLSCERTIFICATEKEYFILE,
                            MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD,
                            MONGOC_URI_TLSCAFILE,
                            MONGOC_URI_TLSALLOWINVALIDCERTIFICATES,
                            MONGOC_URI_TLSALLOWINVALIDHOSTNAMES);

   /* non-canonical case for tls options */
   test_mongoc_uri_tls_ssl ("tls",
                            "TlsCertificateKeyFile",
                            "tlsCertificateKeyFilePASSWORD",
                            "tlsCAFILE",
                            "TLSALLOWINVALIDCERTIFICATES",
                            "tLSaLLOWiNVALIDhOSTNAMES");

   /* non-canonical case for tls option */
   uri = mongoc_uri_new ("mongodb://localhost/?tLs=true");
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_option_as_bool (uri, "TLS", true));
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);

   /* tls-only option */
   uri = mongoc_uri_new ("mongodb://localhost/?tlsInsecure=true");
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSINSECURE, false));
   mongoc_uri_destroy (uri);

   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/?tlsInsecure=true&tlsAllowInvalidHostnames=false", &err));
   ASSERT_ERROR_CONTAINS (err,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "tlsinsecure may not be specified with "
                          "tlsallowinvalidcertificates, tlsallowinvalidhostnames, "
                          "tlsdisableocspendpointcheck, or tlsdisablecertificaterevocationcheck");

   ASSERT (!mongoc_uri_new_with_error ("mongodb://localhost/"
                                       "?tlsInsecure=true&tlsAllowInvalidCertificates=true",
                                       &err));
   ASSERT_ERROR_CONTAINS (err,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "tlsinsecure may not be specified with "
                          "tlsallowinvalidcertificates, tlsallowinvalidhostnames, "
                          "tlsdisableocspendpointcheck, or tlsdisablecertificaterevocationcheck");
}

static void
test_mongoc_uri_ssl (void)
{
   mongoc_uri_t *uri;

   test_mongoc_uri_tls_ssl (MONGOC_URI_SSL,
                            MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE,
                            MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD,
                            MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE,
                            MONGOC_URI_SSLALLOWINVALIDCERTIFICATES,
                            MONGOC_URI_SSLALLOWINVALIDHOSTNAMES);

   /* non-canonical case for ssl options */
   test_mongoc_uri_tls_ssl ("ssl",
                            "SslClientCertificateKeyFile",
                            "sslClientCertificateKeyPASSWORD",
                            "sslCERTIFICATEAUTHORITYFILE",
                            "SSLALLOWINVALIDCERTIFICATES",
                            "sSLaLLOWiNVALIDhOSTNAMES");

   /* non-canonical case for ssl option */
   uri = mongoc_uri_new ("mongodb://localhost/?sSl=true");
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/");
   ASSERT (mongoc_uri_set_option_as_bool (uri, "SSL", true));
   ASSERT (mongoc_uri_get_tls (uri));
   mongoc_uri_destroy (uri);
}

static void
test_mongoc_uri_local_threshold_ms (void)
{
   mongoc_uri_t *uri;

   uri = mongoc_uri_new ("mongodb://localhost/");

   /* localthresholdms isn't set, return the default */
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS);
   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_LOCALTHRESHOLDMS, 99));
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, 99);

   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_LOCALTHRESHOLDMS "=0");

   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, 0);
   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_LOCALTHRESHOLDMS, 99));
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, 99);

   mongoc_uri_destroy (uri);

   uri = mongoc_uri_new ("mongodb://localhost/?" MONGOC_URI_LOCALTHRESHOLDMS "=-1");

   /* localthresholdms is invalid, return the default */
   capture_logs (true);
   ASSERT_CMPINT (mongoc_uri_get_local_threshold_option (uri), ==, MONGOC_TOPOLOGY_LOCAL_THRESHOLD_MS);
   ASSERT_CAPTURED_LOG (
      "mongoc_uri_get_local_threshold_option", MONGOC_LOG_LEVEL_WARNING, "Invalid localThresholdMS: -1");

   mongoc_uri_destroy (uri);
}


#define INVALID(_uri, _host)                                                                               \
   ASSERT_WITH_MSG (!mongoc_uri_upsert_host ((_uri), (_host), 1, &error), "expected host upsert to fail"); \
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_NAME_RESOLUTION, "must be subdomain")

#define VALID(_uri, _host) ASSERT_OR_PRINT (mongoc_uri_upsert_host ((_uri), (_host), 1, &error), error)


static void
test_mongoc_uri_srv (void)
{
   mongoc_uri_t *uri;
   bson_error_t error;

   capture_logs (true);

   ASSERT (!mongoc_uri_new ("mongodb+srv://"));
   /* requires a subdomain, domain, and TLD: "a.example.com" */
   ASSERT (!mongoc_uri_new ("mongodb+srv://foo"));
   ASSERT (!mongoc_uri_new ("mongodb+srv://foo."));
   ASSERT (!mongoc_uri_new ("mongodb+srv://.foo"));
   ASSERT (!mongoc_uri_new ("mongodb+srv://.."));
   ASSERT (!mongoc_uri_new ("mongodb+srv://.a."));
   ASSERT (!mongoc_uri_new ("mongodb+srv://.a.b.c.com"));
   ASSERT (!mongoc_uri_new ("mongodb+srv://foo\x08\x00bar"));
   ASSERT (!mongoc_uri_new ("mongodb+srv://foo%00bar"));
   ASSERT (!mongoc_uri_new ("mongodb+srv://example.com"));

   uri = mongoc_uri_new_with_error ("mongodb+srv://c.d.com", &error);
   ASSERT_OR_PRINT (uri, error);
   ASSERT_CMPSTR (mongoc_uri_get_srv_hostname (uri), "c.d.com");
   ASSERT (mongoc_uri_get_hosts (uri) == NULL);

   /* tls is set to true when we use SRV */
   ASSERT_EQUAL_BSON (tmp_bson ("{'tls': true}"), mongoc_uri_get_options (uri));

   /* but we can override tls */
   mongoc_uri_destroy (uri);
   uri = mongoc_uri_new_with_error ("mongodb+srv://c.d.com/?tls=false", &error);
   ASSERT_OR_PRINT (uri, error);
   ASSERT_EQUAL_BSON (tmp_bson ("{'tls': false}"), mongoc_uri_get_options (uri));

   INVALID (uri, "com");
   INVALID (uri, "foo.com");
   INVALID (uri, "d.com");
   INVALID (uri, "cd.com");
   VALID (uri, "c.d.com");
   VALID (uri, "bc.d.com");
   VALID (uri, "longer-string.d.com");
   INVALID (uri, ".c.d.com");
   VALID (uri, "b.c.d.com");
   INVALID (uri, ".b.c.d.com");
   INVALID (uri, "..b.c.d.com");
   VALID (uri, "a.b.c.d.com");

   mongoc_uri_destroy (uri);
   uri = mongoc_uri_new ("mongodb+srv://b.c.d.com");

   INVALID (uri, "foo.com");
   INVALID (uri, "a.b.d.com");
   INVALID (uri, "d.com");
   VALID (uri, "b.c.d.com");
   VALID (uri, "a.b.c.d.com");
   VALID (uri, "foo.a.b.c.d.com");

   mongoc_uri_destroy (uri);

   /* trailing dot is OK */
   uri = mongoc_uri_new ("mongodb+srv://service.consul.");
   ASSERT (uri);
   ASSERT_CMPSTR (mongoc_uri_get_srv_hostname (uri), "service.consul.");
   ASSERT (mongoc_uri_get_hosts (uri) == NULL);

   INVALID (uri, ".consul.");
   INVALID (uri, "service.consul");
   INVALID (uri, "a.service.consul");
   INVALID (uri, "service.a.consul");
   INVALID (uri, "a.com");
   VALID (uri, "service.consul.");
   VALID (uri, "a.service.consul.");
   VALID (uri, "a.b.service.consul.");

   mongoc_uri_destroy (uri);
}


#define PROHIBITED(_key, _value, _type, _where)                                                      \
   do {                                                                                              \
      const char *option = _key "=" #_value;                                                         \
      char *lkey = bson_strdup (_key);                                                               \
      mongoc_lowercase (lkey, lkey);                                                                 \
      _mongoc_uri_apply_query_string (uri, mstr_cstring (option), true /* from dns */, &error);      \
      ASSERT_ERROR_CONTAINS (                                                                        \
         error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "prohibited in TXT record"); \
      ASSERT (!bson_has_field (mongoc_uri_get_##_where (uri), lkey));                                \
      bson_free (lkey);                                                                              \
   } while (0)


static void
test_mongoc_uri_dns_options (void)
{
   mongoc_uri_t *uri;
   bson_error_t error;

   uri = mongoc_uri_new ("mongodb+srv://a.b.c");
   ASSERT (uri);

   ASSERT (!_mongoc_uri_apply_query_string (uri, mstr_cstring ("tls=false"), true /* from dsn */, &error));

   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "prohibited in TXT record");

   ASSERT_EQUAL_BSON (tmp_bson ("{'tls': true}"), mongoc_uri_get_options (uri));

   /* key we want to set, value, value type, whether it's option/credential */
   PROHIBITED (MONGOC_URI_TLSALLOWINVALIDHOSTNAMES, true, bool, options);
   PROHIBITED (MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, true, bool, options);
   PROHIBITED (MONGOC_URI_GSSAPISERVICENAME, malicious, utf8, credentials);

   /* the two options allowed in TXT records, case-insensitive */
   ASSERT (_mongoc_uri_apply_query_string (uri, mstr_cstring ("authsource=db"), true, NULL));
   ASSERT (_mongoc_uri_apply_query_string (uri, mstr_cstring ("RepLIcaSET=rs"), true, NULL));

   /* test that URI string overrides TXT record options */
   mongoc_uri_destroy (uri);
   uri = mongoc_uri_new ("mongodb+srv://user@a.b.c/?authSource=db1&replicaSet=rs1");

   // test that parsing warns if replicaSet is ignored from TXT records.
   {
      capture_logs (true);
      ASSERT (_mongoc_uri_apply_query_string (uri, mstr_cstring ("replicaSet=db2"), true, NULL));
      ASSERT_CAPTURED_LOG (
         "parsing replicaSet from TXT", MONGOC_LOG_LEVEL_WARNING, "Ignoring URI option \"replicaSet\"");
      capture_logs (false);
      ASSERT_MATCH (mongoc_uri_get_options (uri), "{'replicaset': 'rs1'}");
   }

   // test that parsing does not warn if authSource is ignored from TXT records.
   {
      capture_logs (true);
      ASSERT (_mongoc_uri_apply_query_string (uri, mstr_cstring ("authSource=db2"), true, NULL));
      ASSERT_NO_CAPTURED_LOGS ("parsing authSource from TXT");
      capture_logs (false);
      ASSERT_MATCH (mongoc_uri_get_credentials (uri), "{'authsource': 'db1'}");
   }

   mongoc_uri_destroy (uri);
}


/* test some invalid accesses and a crash, found with a fuzzer */
static void
test_mongoc_uri_utf8 (void)
{
   bson_error_t err;

   /* start of 3-byte character, but it's incomplete */
   ASSERT (!mongoc_uri_new_with_error ("mongodb://\xe8\x03", &err));
   ASSERT_ERROR_CONTAINS (err, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid UTF-8 in URI");

   /* start of 6-byte CESU-8 character, but it's incomplete */
   ASSERT (!mongoc_uri_new_with_error ("mongodb://\xfa", &err));
   ASSERT_ERROR_CONTAINS (err, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid UTF-8 in URI");


   /* "a<NIL>z" with NIL expressed as two-byte sequence */
   ASSERT (!mongoc_uri_new_with_error ("mongodb://a\xc0\x80z", &err));
   ASSERT_ERROR_CONTAINS (err, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid UTF-8 in URI");
}


/* test behavior on duplicate values for an options. */
static void
test_mongoc_uri_duplicates (void)
{
   mongoc_uri_t *uri = NULL;
   bson_error_t err;
   const char *str;
   const mongoc_write_concern_t *wc;
   const mongoc_read_concern_t *rc;
   const bson_t *bson;
   const mongoc_read_prefs_t *rp;
   bson_iter_t iter = {0};

#define RECREATE_URI(opts)                                                               \
   if (1) {                                                                              \
      mongoc_uri_destroy (uri);                                                          \
      clear_captured_logs ();                                                            \
      uri = mongoc_uri_new_with_error ("mongodb://user:pwd@localhost/test?" opts, &err); \
      ASSERT_OR_PRINT (uri, err);                                                        \
   } else                                                                                \
      (void) 0

#define ASSERT_LOG_DUPE(opt) \
   ASSERT_CAPTURED_LOG ("option: " opt, MONGOC_LOG_LEVEL_WARNING, "Overwriting previously provided value for '" opt "'")

/* iterate iter to key, and check that no other occurrences exist. */
#define BSON_ITER_UNIQUE(key)                                             \
   do {                                                                   \
      bson_iter_t tmp;                                                    \
      ASSERT (bson_iter_init_find (&iter, bson, key));                    \
      tmp = iter;                                                         \
      while (bson_iter_next (&tmp)) {                                     \
         if (strcmp (bson_iter_key (&tmp), key) == 0) {                   \
            ASSERT_WITH_MSG (false, "bson has duplicate keys for: " key); \
         }                                                                \
      }                                                                   \
   } while (0)

   capture_logs (true);

   /* test all URI options, in the order they are defined in mongoc-uri.h. */
   RECREATE_URI (MONGOC_URI_APPNAME "=a&" MONGOC_URI_APPNAME "=b");
   ASSERT_LOG_DUPE (MONGOC_URI_APPNAME);
   str = mongoc_uri_get_appname (uri);
   ASSERT_CMPSTR (str, "b");

   RECREATE_URI (MONGOC_URI_AUTHMECHANISM "=SCRAM-SHA-1&" MONGOC_URI_AUTHMECHANISM "=SCRAM-SHA-256");
   ASSERT_LOG_DUPE (MONGOC_URI_AUTHMECHANISM);
   bson = mongoc_uri_get_credentials (uri);
   BSON_ITER_UNIQUE (MONGOC_URI_AUTHMECHANISM);
   ASSERT (strcmp (bson_iter_utf8 (&iter, NULL), "SCRAM-SHA-256") == 0);

   RECREATE_URI (MONGOC_URI_AUTHMECHANISMPROPERTIES "=a:x&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=b:y");
   ASSERT_LOG_DUPE (MONGOC_URI_AUTHMECHANISMPROPERTIES);
   bson = mongoc_uri_get_credentials (uri);
   ASSERT_EQUAL_BSON (tmp_bson ("{'authmechanismproperties': {'b': 'y'}}"), bson);

   RECREATE_URI (MONGOC_URI_AUTHMECHANISMPROPERTIES "=a:x&" MONGOC_URI_AUTHMECHANISMPROPERTIES "=b:y");
   ASSERT_LOG_DUPE (MONGOC_URI_AUTHMECHANISMPROPERTIES);
   bson = mongoc_uri_get_credentials (uri);
   ASSERT_EQUAL_BSON (tmp_bson ("{'authmechanismproperties': {'b': 'y' }}"), bson);

   RECREATE_URI (MONGOC_URI_AUTHSOURCE "=a&" MONGOC_URI_AUTHSOURCE "=b");
   ASSERT_LOG_DUPE (MONGOC_URI_AUTHSOURCE);
   str = mongoc_uri_get_auth_source (uri);
   ASSERT_CMPSTR (str, "b");

   RECREATE_URI (MONGOC_URI_CANONICALIZEHOSTNAME "=false&" MONGOC_URI_CANONICALIZEHOSTNAME "=true");
   ASSERT_CAPTURED_LOG ("option: " MONGOC_URI_CANONICALIZEHOSTNAME,
                        MONGOC_LOG_LEVEL_WARNING,
                        MONGOC_URI_CANONICALIZEHOSTNAME " is deprecated, use " MONGOC_URI_AUTHMECHANISMPROPERTIES
                                                        " with CANONICALIZE_HOST_NAME instead");
   ASSERT_LOG_DUPE (MONGOC_URI_CANONICALIZEHOSTNAME);
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_CANONICALIZEHOSTNAME, false));

   RECREATE_URI (MONGOC_URI_CONNECTTIMEOUTMS "=1&" MONGOC_URI_CONNECTTIMEOUTMS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_CONNECTTIMEOUTMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 0) == 2);

#if defined(MONGOC_ENABLE_COMPRESSION_SNAPPY) && defined(MONGOC_ENABLE_COMPRESSION_ZLIB)
   RECREATE_URI (MONGOC_URI_COMPRESSORS "=snappy&" MONGOC_URI_COMPRESSORS "=zlib");
   ASSERT_LOG_DUPE (MONGOC_URI_COMPRESSORS);
   bson = mongoc_uri_get_compressors (uri);
   ASSERT_EQUAL_BSON (tmp_bson ("{'zlib': 'yes'}"), bson);
#endif

   RECREATE_URI (MONGOC_URI_GSSAPISERVICENAME "=a&" MONGOC_URI_GSSAPISERVICENAME "=b");
   ASSERT_CAPTURED_LOG ("option: " MONGOC_URI_GSSAPISERVICENAME,
                        MONGOC_LOG_LEVEL_WARNING,
                        MONGOC_URI_GSSAPISERVICENAME " is deprecated, use " MONGOC_URI_AUTHMECHANISMPROPERTIES
                                                     " with SERVICE_NAME instead");
   ASSERT_CAPTURED_LOG ("option: " MONGOC_URI_GSSAPISERVICENAME,
                        MONGOC_LOG_LEVEL_WARNING,
                        "Overwriting previously provided value for 'gssapiservicename'");
   bson = mongoc_uri_get_credentials (uri);
   ASSERT_EQUAL_BSON (tmp_bson ("{'authmechanismproperties': {'SERVICE_NAME': 'b' }}"), bson);

   RECREATE_URI (MONGOC_URI_HEARTBEATFREQUENCYMS "=500&" MONGOC_URI_HEARTBEATFREQUENCYMS "=501");
   ASSERT_LOG_DUPE (MONGOC_URI_HEARTBEATFREQUENCYMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_HEARTBEATFREQUENCYMS, 0) == 501);

   RECREATE_URI (MONGOC_URI_JOURNAL "=false&" MONGOC_URI_JOURNAL "=true");
   ASSERT_LOG_DUPE (MONGOC_URI_JOURNAL);
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_JOURNAL, false));

   RECREATE_URI (MONGOC_URI_LOCALTHRESHOLDMS "=1&" MONGOC_URI_LOCALTHRESHOLDMS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_LOCALTHRESHOLDMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_LOCALTHRESHOLDMS, 0) == 2);

   RECREATE_URI (MONGOC_URI_MAXPOOLSIZE "=1&" MONGOC_URI_MAXPOOLSIZE "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_MAXPOOLSIZE);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_MAXPOOLSIZE, 0) == 2);

   RECREATE_URI (MONGOC_URI_READPREFERENCE "=secondary&" MONGOC_URI_MAXSTALENESSSECONDS
                                           "=1&" MONGOC_URI_MAXSTALENESSSECONDS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_MAXSTALENESSSECONDS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_MAXSTALENESSSECONDS, 0) == 2);

   RECREATE_URI (MONGOC_URI_READCONCERNLEVEL "=local&" MONGOC_URI_READCONCERNLEVEL "=majority");
   ASSERT_LOG_DUPE (MONGOC_URI_READCONCERNLEVEL);
   rc = mongoc_uri_get_read_concern (uri);
   ASSERT (strcmp (mongoc_read_concern_get_level (rc), "majority") == 0);

   RECREATE_URI (MONGOC_URI_READPREFERENCE "=secondary&" MONGOC_URI_READPREFERENCE "=primary");
   ASSERT_LOG_DUPE (MONGOC_URI_READPREFERENCE);
   rp = mongoc_uri_get_read_prefs_t (uri);
   ASSERT (mongoc_read_prefs_get_mode (rp) == MONGOC_READ_PRIMARY);

   /* exception: read preference tags get appended. */
   RECREATE_URI (MONGOC_URI_READPREFERENCE "=secondary&" MONGOC_URI_READPREFERENCETAGS
                                           "=a:x&" MONGOC_URI_READPREFERENCETAGS "=b:y");
   ASSERT_NO_CAPTURED_LOGS (mongoc_uri_get_string (uri));
   rp = mongoc_uri_get_read_prefs_t (uri);
   bson = mongoc_read_prefs_get_tags (rp);
   ASSERT_EQUAL_BSON (tmp_bson ("[{'a': 'x'}, {'b': 'y'}]"), bson);

   RECREATE_URI (MONGOC_URI_REPLICASET "=a&" MONGOC_URI_REPLICASET "=b");
   ASSERT_LOG_DUPE (MONGOC_URI_REPLICASET);
   str = mongoc_uri_get_replica_set (uri);
   ASSERT_CMPSTR (str, "b");

   RECREATE_URI (MONGOC_URI_RETRYREADS "=false&" MONGOC_URI_RETRYREADS "=true");
   ASSERT_LOG_DUPE (MONGOC_URI_RETRYREADS);
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_RETRYREADS, false));

   RECREATE_URI (MONGOC_URI_RETRYWRITES "=false&" MONGOC_URI_RETRYWRITES "=true");
   ASSERT_LOG_DUPE (MONGOC_URI_RETRYWRITES);
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_RETRYWRITES, false));

   RECREATE_URI (MONGOC_URI_SAFE "=false&" MONGOC_URI_SAFE "=true");
   ASSERT_LOG_DUPE (MONGOC_URI_SAFE);
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SAFE, false));

   RECREATE_URI (MONGOC_URI_SERVERMONITORINGMODE "=auto&" MONGOC_URI_SERVERMONITORINGMODE "=stream");
   ASSERT_LOG_DUPE (MONGOC_URI_SERVERMONITORINGMODE);
   str = mongoc_uri_get_server_monitoring_mode (uri);
   ASSERT_CMPSTR (str, "stream");

   RECREATE_URI (MONGOC_URI_SERVERSELECTIONTIMEOUTMS "=1&" MONGOC_URI_SERVERSELECTIONTIMEOUTMS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_SERVERSELECTIONTIMEOUTMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SERVERSELECTIONTIMEOUTMS, 0) == 2);

   RECREATE_URI (MONGOC_URI_SERVERSELECTIONTRYONCE "=false&" MONGOC_URI_SERVERSELECTIONTRYONCE "=true");
   ASSERT_LOG_DUPE (MONGOC_URI_SERVERSELECTIONTRYONCE);
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_SERVERSELECTIONTRYONCE, false));

   RECREATE_URI (MONGOC_URI_SOCKETCHECKINTERVALMS "=1&" MONGOC_URI_SOCKETCHECKINTERVALMS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_SOCKETCHECKINTERVALMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETCHECKINTERVALMS, 0) == 2);

   RECREATE_URI (MONGOC_URI_SOCKETTIMEOUTMS "=1&" MONGOC_URI_SOCKETTIMEOUTMS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_SOCKETTIMEOUTMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_SOCKETTIMEOUTMS, 0) == 2);

   RECREATE_URI (MONGOC_URI_TLS "=false&" MONGOC_URI_TLS "=true");
   ASSERT_LOG_DUPE (MONGOC_URI_TLS);
   ASSERT (mongoc_uri_get_tls (uri));

   RECREATE_URI (MONGOC_URI_TLSCERTIFICATEKEYFILE "=a&" MONGOC_URI_TLSCERTIFICATEKEYFILE "=b");
   ASSERT_LOG_DUPE (MONGOC_URI_TLSCERTIFICATEKEYFILE);
   str = mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "");
   ASSERT_CMPSTR (str, "b");

   RECREATE_URI (MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD "=a&" MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD "=b");
   ASSERT_LOG_DUPE (MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD);
   str = mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD, "");
   ASSERT_CMPSTR (str, "b");

   RECREATE_URI (MONGOC_URI_TLSCAFILE "=a&" MONGOC_URI_TLSCAFILE "=b");
   ASSERT_LOG_DUPE (MONGOC_URI_TLSCAFILE);
   str = mongoc_uri_get_option_as_utf8 (uri, MONGOC_URI_TLSCAFILE, "");
   ASSERT_CMPSTR (str, "b");

   RECREATE_URI (MONGOC_URI_TLSALLOWINVALIDCERTIFICATES "=false&" MONGOC_URI_TLSALLOWINVALIDCERTIFICATES "=true");
   ASSERT_LOG_DUPE (MONGOC_URI_TLSALLOWINVALIDCERTIFICATES);
   ASSERT (mongoc_uri_get_option_as_bool (uri, MONGOC_URI_TLSALLOWINVALIDCERTIFICATES, false));

   RECREATE_URI (MONGOC_URI_W "=1&" MONGOC_URI_W "=0");
   ASSERT_LOG_DUPE (MONGOC_URI_W);
   wc = mongoc_uri_get_write_concern (uri);
   ASSERT (mongoc_write_concern_get_w (wc) == 0);

   /* exception: a string write concern takes precedence over an int */
   RECREATE_URI (MONGOC_URI_W "=majority&" MONGOC_URI_W "=0");
   ASSERT_NO_CAPTURED_LOGS (mongoc_uri_get_string (uri));
   wc = mongoc_uri_get_write_concern (uri);
   ASSERT (mongoc_write_concern_get_w (wc) == MONGOC_WRITE_CONCERN_W_MAJORITY);

   RECREATE_URI (MONGOC_URI_WAITQUEUETIMEOUTMS "=1&" MONGOC_URI_WAITQUEUETIMEOUTMS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_WAITQUEUETIMEOUTMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WAITQUEUETIMEOUTMS, 0) == 2);

   RECREATE_URI (MONGOC_URI_WTIMEOUTMS "=1&" MONGOC_URI_WTIMEOUTMS "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_WTIMEOUTMS);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 0) == 2);
   ASSERT (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 0) == 2);

   RECREATE_URI (MONGOC_URI_ZLIBCOMPRESSIONLEVEL "=1&" MONGOC_URI_ZLIBCOMPRESSIONLEVEL "=2");
   ASSERT_LOG_DUPE (MONGOC_URI_ZLIBCOMPRESSIONLEVEL);
   ASSERT (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 0) == 2);

   mongoc_uri_destroy (uri);
}


/* Tests behavior of int32 and int64 options */
static void
test_mongoc_uri_int_options (void)
{
   mongoc_uri_t *uri;

   capture_logs (true);

   uri = mongoc_uri_new ("mongodb://localhost/");

   /* Set an int64 option as int64 succeeds */
   ASSERT (mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 10));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 0), ==, 10);
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 0), ==, 10);

   /* Set an int64 option as int32 succeeds */
   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 15));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 0), ==, 15);
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 0), ==, 15);

   /* Setting an int32 option through _as_int64 succeeds for 32-bit values but
    * emits a warning */
   ASSERT (mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 9));
   ASSERT_CAPTURED_LOG ("option: " MONGOC_URI_ZLIBCOMPRESSIONLEVEL,
                        MONGOC_LOG_LEVEL_WARNING,
                        "Setting value for 32-bit option "
                        "\"zlibcompressionlevel\" through 64-bit method");
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 0), ==, 9);
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 0), ==, 9);

   clear_captured_logs ();

   ASSERT (!mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 2147483648LL));
   ASSERT_CAPTURED_LOG ("option: " MONGOC_URI_CONNECTTIMEOUTMS,
                        MONGOC_LOG_LEVEL_WARNING,
                        "Unsupported value for \"connecttimeoutms\": 2147483648,"
                        " \"connecttimeoutms\" is not an int64 option");
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 0), ==, 0);
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_CONNECTTIMEOUTMS, 0), ==, 0);

   clear_captured_logs ();

   /* Setting an int32 option as int32 succeeds */
   ASSERT (mongoc_uri_set_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 9));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 0), ==, 9);
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_ZLIBCOMPRESSIONLEVEL, 0), ==, 9);

   /* Truncating a 64-bit value when fetching as 32-bit emits a warning */
   ASSERT (mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 2147483648LL));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 5), ==, 5);
   ASSERT_CAPTURED_LOG ("option: " MONGOC_URI_WTIMEOUTMS " with 64-bit value",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Cannot read 64-bit value for \"wtimeoutms\": 2147483648");
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 5), ==, 2147483648LL);

   clear_captured_logs ();

   ASSERT (mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, -2147483649LL));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 5), ==, 5);
   ASSERT_CAPTURED_LOG ("option: " MONGOC_URI_WTIMEOUTMS " with 64-bit value",
                        MONGOC_LOG_LEVEL_WARNING,
                        "Cannot read 64-bit value for \"wtimeoutms\": -2147483649");
   ASSERT_CMPINT64 (mongoc_uri_get_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, 5), ==, -2147483649LL);

   clear_captured_logs ();

   /* Setting a INT_MAX and INT_MIN values doesn't cause truncation errors */
   ASSERT (mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, INT32_MAX));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 0), ==, INT32_MAX);
   ASSERT_NO_CAPTURED_LOGS ("INT_MAX");
   ASSERT (mongoc_uri_set_option_as_int64 (uri, MONGOC_URI_WTIMEOUTMS, INT32_MIN));
   ASSERT_CMPINT32 (mongoc_uri_get_option_as_int32 (uri, MONGOC_URI_WTIMEOUTMS, 0), ==, INT32_MIN);
   ASSERT_NO_CAPTURED_LOGS ("INT_MIN");

   mongoc_uri_destroy (uri);
}

static void
test_one_tls_option_enables_tls (void)
{
   const char *opts[] = {MONGOC_URI_TLS "=true",
                         MONGOC_URI_TLSCERTIFICATEKEYFILE "=file.pem",
                         MONGOC_URI_TLSCERTIFICATEKEYFILEPASSWORD "=file.pem",
                         MONGOC_URI_TLSCAFILE "=file.pem",
                         MONGOC_URI_TLSALLOWINVALIDCERTIFICATES "=true",
                         MONGOC_URI_TLSALLOWINVALIDHOSTNAMES "=true",
                         MONGOC_URI_TLSINSECURE "=true",
                         MONGOC_URI_SSL "=true",
                         MONGOC_URI_SSLCLIENTCERTIFICATEKEYFILE "=file.pem",
                         MONGOC_URI_SSLCLIENTCERTIFICATEKEYPASSWORD "=file.pem",
                         MONGOC_URI_SSLCERTIFICATEAUTHORITYFILE "=file.pem",
                         MONGOC_URI_SSLALLOWINVALIDCERTIFICATES "=true",
                         MONGOC_URI_SSLALLOWINVALIDHOSTNAMES "=true",
                         MONGOC_URI_TLSDISABLEOCSPENDPOINTCHECK "=true",
                         MONGOC_URI_TLSDISABLECERTIFICATEREVOCATIONCHECK "=true"};

   mlib_foreach_arr (const char *, opt, opts) {
      mongoc_uri_t *uri;
      bson_error_t error;
      char *uri_string;

      uri_string = bson_strdup_printf ("mongodb://localhost:27017/?%s", *opt);
      uri = mongoc_uri_new_with_error (uri_string, &error);
      bson_free (uri_string);
      ASSERT_OR_PRINT (uri, error);
      if (!mongoc_uri_get_tls (uri)) {
         test_error ("unexpected tls not enabled when following option set: %s\n", *opt);
      }
      mongoc_uri_destroy (uri);
   }
}

static void
test_casing_options (void)
{
   mongoc_uri_t *uri;
   bson_error_t error;

   uri = mongoc_uri_new ("mongodb://localhost:27017/");
   mongoc_uri_set_option_as_bool (uri, "TLS", true);
   _mongoc_uri_apply_query_string (uri, mstr_cstring ("ssl=false"), false, &error);
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "conflicts");

   mongoc_uri_destroy (uri);
}

static void
test_parses_long_ipv6 (void)
{
   // Test parsing long malformed IPv6 literals. This is a regression test for
   // CDRIVER-4816.
   bson_error_t error;

   // Test the largest permitted IPv6 literal.
   {
      // Construct a string of repeating `:`.
      mcommon_string_append_t host;
      mcommon_string_new_as_append (&host);
      for (int i = 0; i < BSON_HOST_NAME_MAX - 2; i++) {
         // Max IPv6 literal is two less due to including `[` and `]`.
         mcommon_string_append (&host, ":");
      }
      const char *host_str = mcommon_str_from_append (&host);

      char *host_and_port = bson_strdup_printf ("[%s]:27017", host_str);
      char *uri_string = bson_strdup_printf ("mongodb://%s", host_and_port);
      mongoc_uri_t *uri = mongoc_uri_new_with_error (uri_string, &error);
      ASSERT_OR_PRINT (uri, error);
      const mongoc_host_list_t *hosts = mongoc_uri_get_hosts (uri);
      ASSERT_CMPSTR (hosts->host, host_str);
      ASSERT_CMPSTR (hosts->host_and_port, host_and_port);
      ASSERT_CMPUINT16 (hosts->port, ==, 27017);
      ASSERT (!hosts->next);

      mongoc_uri_destroy (uri);
      bson_free (uri_string);
      bson_free (host_and_port);
      mcommon_string_from_append_destroy (&host);
   }

   // Test one character more than the largest IPv6 literal.
   {
      // Construct a string of repeating `:`.
      mcommon_string_append_t host;
      mcommon_string_new_as_append (&host);
      for (int i = 0; i < BSON_HOST_NAME_MAX - 2 + 1; i++) {
         mcommon_string_append (&host, ":");
      }
      const char *host_str = mcommon_str_from_append (&host);

      char *host_and_port = bson_strdup_printf ("[%s]:27017", host_str);
      char *uri_string = bson_strdup_printf ("mongodb://%s", host_and_port);
      capture_logs (true);
      mongoc_uri_t *uri = mongoc_uri_new_with_error (uri_string, &error);
      ASSERT_NO_CAPTURED_LOGS ("Invalid IPv6 address");
      capture_logs (false);

      ASSERT (!uri);
      ASSERT_ERROR_CONTAINS (
         error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, "Invalid host specifier \"[");

      mongoc_uri_destroy (uri);
      bson_free (uri_string);
      bson_free (host_and_port);
      mcommon_string_from_append_destroy (&host);
   }
}

static void
test_uri_depr (void)
{
   // Test behavior of deprecated URI options.
   // Regression test for CDRIVER-3769 Deprecate unimplemented URI options

   // Test an unsupported option warns.
   {
      capture_logs (true);
      mongoc_uri_t *uri = mongoc_uri_new ("mongodb://host/?foo=bar");
      ASSERT_CAPTURED_LOG ("uri", MONGOC_LOG_LEVEL_WARNING, "Unsupported");
      capture_logs (false);
      mongoc_uri_destroy (uri);
   }
   // Test that waitQueueMultiple warns.
   {
      capture_logs (true);
      mongoc_uri_t *uri = mongoc_uri_new ("mongodb://host/?waitQueueMultiple=123");
      ASSERT_CAPTURED_LOG ("uri", MONGOC_LOG_LEVEL_WARNING, "Unsupported");
      capture_logs (false);
      mongoc_uri_destroy (uri);
   }
   // Test that maxIdleTimeMS warns.
   {
      capture_logs (true);
      mongoc_uri_t *uri = mongoc_uri_new ("mongodb://host/?maxIdleTimeMS=123");
      ASSERT_CAPTURED_LOG ("uri", MONGOC_LOG_LEVEL_WARNING, "Unsupported");
      capture_logs (false);
      mongoc_uri_destroy (uri);
   }
}

// Additional slashes and commas for embedded URIs given to connection options.
// e.g. authMechanismProperties=TOKEN_RESOURCE=mongodb://foo,ENVIRONMENT=azure
//                                                     ^^   ^
static void
test_uri_uri_in_options (void)
{
#define TEST_QUERY MONGOC_URI_AUTHMECHANISMPROPERTIES "=TOKEN_RESOURCE:mongodb://token-resource,ENVIRONMENT:azure"
#define TEST_PROPS "{'TOKEN_RESOURCE': 'mongodb://token-resource', 'ENVIRONMENT': 'azure'}"

   capture_logs (true);

   bson_error_t error;

   // Simple.
   {
      mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://localhost?" TEST_QUERY, &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      bson_t props;
      ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
      ASSERT_MATCH (&props, TEST_PROPS);
      mongoc_uri_destroy (uri);
   }

   // With auth database.
   {
      mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://localhost/db?" TEST_QUERY, &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      bson_t props;
      ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
      ASSERT_MATCH (&props, TEST_PROPS);
      mongoc_uri_destroy (uri);
   }

   // With userinfo.
   {
      mongoc_uri_t *const uri = mongoc_uri_new_with_error ("mongodb://user:pass@localhost/db?" TEST_QUERY, &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      bson_t props;
      ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
      ASSERT_MATCH (&props, TEST_PROPS);
      mongoc_uri_destroy (uri);
   }

   // With alternate hosts.
   {
      mongoc_uri_t *const uri =
         mongoc_uri_new_with_error ("mongodb://user:pass@host1:27017,host2:27018/db?" TEST_QUERY, &error);
      ASSERT_NO_CAPTURED_LOGS ("mongoc_uri_new_with_error");
      ASSERT_OR_PRINT (uri, error);
      bson_t props;
      ASSERT (mongoc_uri_get_mechanism_properties (uri, &props));
      ASSERT_MATCH (&props, TEST_PROPS);
      mongoc_uri_destroy (uri);
   }

   capture_logs (false);

#undef TEST_QUERY
}

void
test_uri_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Uri/new", test_mongoc_uri_new);
   TestSuite_Add (suite, "/Uri/new_with_error", test_mongoc_uri_new_with_error);
   TestSuite_Add (suite, "/Uri/new_for_host_port", test_mongoc_uri_new_for_host_port);
   TestSuite_Add (suite, "/Uri/compressors", test_mongoc_uri_compressors);
   TestSuite_Add (suite, "/Uri/unescape", test_mongoc_uri_unescape);
   TestSuite_Add (suite, "/Uri/read_prefs", test_mongoc_uri_read_prefs);
   TestSuite_Add (suite, "/Uri/read_concern", test_mongoc_uri_read_concern);
   TestSuite_Add (suite, "/Uri/write_concern", test_mongoc_uri_write_concern);
   TestSuite_Add (suite, "/HostList/from_string", test_mongoc_host_list_from_string);
   TestSuite_Add (suite, "/Uri/auth_mechanisms", test_mongoc_uri_auth_mechanisms);
   TestSuite_Add (suite, "/Uri/functions", test_mongoc_uri_functions);
   TestSuite_Add (suite, "/Uri/ssl", test_mongoc_uri_ssl);
   TestSuite_Add (suite, "/Uri/tls", test_mongoc_uri_tls);
   TestSuite_Add (suite, "/Uri/compound_setters", test_mongoc_uri_compound_setters);
   TestSuite_Add (suite, "/Uri/long_hostname", test_mongoc_uri_long_hostname);
   TestSuite_Add (suite, "/Uri/local_threshold_ms", test_mongoc_uri_local_threshold_ms);
   TestSuite_Add (suite, "/Uri/srv", test_mongoc_uri_srv);
   TestSuite_Add (suite, "/Uri/dns_options", test_mongoc_uri_dns_options);
   TestSuite_Add (suite, "/Uri/utf8", test_mongoc_uri_utf8);
   TestSuite_Add (suite, "/Uri/duplicates", test_mongoc_uri_duplicates);
   TestSuite_Add (suite, "/Uri/int_options", test_mongoc_uri_int_options);
   TestSuite_Add (suite, "/Uri/one_tls_option_enables_tls", test_one_tls_option_enables_tls);
   TestSuite_Add (suite, "/Uri/options_casing", test_casing_options);
   TestSuite_Add (suite, "/Uri/parses_long_ipv6", test_parses_long_ipv6);
   TestSuite_Add (suite, "/Uri/depr", test_uri_depr);
   TestSuite_Add (suite, "/Uri/uri_in_options", test_uri_uri_in_options);
}
