#include <mongoc-util-private.h>
#include <mongoc-client-pool-private.h>
#include "mongoc.h"
#include "mongoc-host-list-private.h"
#include "mongoc-thread-private.h"

#include "json-test.h"
#include "test-libmongoc.h"


static void
_assert_options_match (const bson_t *test, mongoc_client_t *client)
{
   char errmsg[1000];
   match_ctx_t ctx = {0};
   bson_iter_t iter;
   bson_t opts_from_test;
   const bson_t *opts_from_uri;
   const bson_t *creds_from_uri;
   const bson_t *opts_or_creds;
   bson_iter_t test_opts_iter;
   bson_iter_t uri_opts_iter;
   const char *opt_name;
   const bson_value_t *test_value, *uri_value;

   ctx.errmsg = errmsg;
   ctx.errmsg_len = sizeof errmsg;

   if (!bson_iter_init_find (&iter, test, "options")) {
      /* no URI options specified in the test */
      return;
   }

   bson_iter_bson (&iter, &opts_from_test);
   BSON_ASSERT (bson_iter_init (&test_opts_iter, &opts_from_test));

   /* the client's URI is not updated from TXT, but the topology's copy is */
   opts_from_uri = mongoc_uri_get_options (client->topology->uri);
   creds_from_uri = mongoc_uri_get_credentials (client->topology->uri);

   while (bson_iter_next (&test_opts_iter)) {
      opt_name = bson_iter_key (&test_opts_iter);
      opts_or_creds = !bson_strcasecmp (opt_name, "authSource") ? creds_from_uri
                                                                : opts_from_uri;
      if (!bson_iter_init_find_case (&uri_opts_iter, opts_or_creds, opt_name)) {
         fprintf (stderr,
                  "URI options incorrectly set from TXT record: "
                  "no option named \"%s\"\n"
                  "expected: %s\n"
                  "actual: %s\n",
                  opt_name,
                  bson_as_json (&opts_from_test, NULL),
                  bson_as_json (opts_or_creds, NULL));
         abort ();
      }

      test_value = bson_iter_value (&test_opts_iter);
      uri_value = bson_iter_value (&uri_opts_iter);
      if (!match_bson_value (uri_value, test_value, &ctx)) {
         fprintf (stderr,
                  "URI option \"%s\" incorrectly set from TXT record: %s\n"
                  "expected: %s\n"
                  "actual: %s\n",
                  opt_name,
                  ctx.errmsg,
                  bson_as_json (&opts_from_test, NULL),
                  bson_as_json (opts_from_uri, NULL));
         abort ();
      }
   }
}


typedef struct {
   mongoc_mutex_t mutex;
   mongoc_host_list_t *hosts;
} context_t;


static void
topology_changed (const mongoc_apm_topology_changed_t *event)
{
   context_t *ctx;
   const mongoc_topology_description_t *td;
   size_t i;
   size_t n;
   mongoc_server_description_t **sds;

   ctx = (context_t *) mongoc_apm_topology_changed_get_context (event);

   td = mongoc_apm_topology_changed_get_new_description (event);
   sds = mongoc_topology_description_get_servers (td, &n);

   mongoc_mutex_lock (&ctx->mutex);
   _mongoc_host_list_destroy_all (ctx->hosts);
   ctx->hosts = NULL;
   for (i = 0; i < n; i++) {
      ctx->hosts = _mongoc_host_list_push (
         sds[i]->host.host, sds[i]->host.port, AF_UNSPEC, ctx->hosts);
   }
   mongoc_mutex_unlock (&ctx->mutex);

   mongoc_server_descriptions_destroy_all (sds, n);
}


static bool
host_list_contains (const mongoc_host_list_t *hl, const char *host_and_port)
{
   while (hl) {
      if (!strcmp (hl->host_and_port, host_and_port)) {
         return true;
      }

      hl = hl->next;
   }

   return false;
}


static int
hosts_count (const bson_t *test)
{
   bson_iter_t iter;
   bson_iter_t hosts;
   int c = 0;

   BSON_ASSERT (bson_iter_init_find (&iter, test, "hosts"));
   BSON_ASSERT (bson_iter_recurse (&iter, &hosts));
   while (bson_iter_next (&hosts)) {
      c++;
   }

   return c;
}


static bool
_host_list_matches (const bson_t *test, context_t *ctx)
{
   bson_iter_t iter;
   bson_iter_t hosts;
   const char *host_and_port;
   bool ret = true;

   BSON_ASSERT (bson_iter_init_find (&iter, test, "hosts"));
   BSON_ASSERT (bson_iter_recurse (&iter, &hosts));

   mongoc_mutex_lock (&ctx->mutex);
   BSON_ASSERT (bson_iter_recurse (&iter, &hosts));
   while (bson_iter_next (&hosts)) {
      host_and_port = bson_iter_utf8 (&hosts, NULL);
      if (!host_list_contains (ctx->hosts, host_and_port)) {
         ret = false;
         break;
      }
   }

   _mongoc_host_list_destroy_all (ctx->hosts);
   ctx->hosts = NULL;
   mongoc_mutex_unlock (&ctx->mutex);

   return ret;
}


static void
_test_dns_maybe_pooled (bson_t *test, bool pooled)
{
   context_t ctx;
   bool expect_ssl;
   bool expect_error;
   mongoc_uri_t *uri;
   const bson_t *uri_opts;
   mongoc_apm_callbacks_t *callbacks;
   mongoc_client_pool_t *pool = NULL;
   mongoc_client_t *client;
   int n_hosts;
   bson_error_t error;
   bool r;

   if (!test_framework_get_ssl ()) {
      fprintf (stderr,
               "Must configure an SSL replica set and set MONGOC_TEST_SSL=on "
               "and other ssl options to test DNS\n");
      abort ();
   }

   mongoc_mutex_init (&ctx.mutex);
   ctx.hosts = NULL;
   expect_ssl = strstr (bson_lookup_utf8 (test, "uri"), "ssl=false") == NULL;
   expect_error = _mongoc_lookup_bool (test, "error", false /* default */);

   uri = mongoc_uri_new_with_error (bson_lookup_utf8 (test, "uri"), &error);
   if (!expect_error) {
      ASSERT_OR_PRINT (uri, error);
   }

   if (!uri) {
      /* expected failure, e.g. we're testing an invalid URI */
      return;
   }

   callbacks = mongoc_apm_callbacks_new ();
   mongoc_apm_set_topology_changed_cb (callbacks, topology_changed);

   /* suppress "cannot override URI option" messages */
   capture_logs (true);

   if (pooled) {
      pool = mongoc_client_pool_new (uri);

      /* before we set SSL on so that we can connect to the test replica set,
       * assert that the URI has SSL on by default, and SSL off if "ssl=false"
       * is in the URI string */
      uri_opts =
         mongoc_uri_get_options (_mongoc_client_pool_get_topology (pool)->uri);
      BSON_ASSERT (_mongoc_lookup_bool (uri_opts, "ssl", !expect_ssl) ==
                   expect_ssl);
      test_framework_set_pool_ssl_opts (pool);
      mongoc_client_pool_set_apm_callbacks (pool, callbacks, &ctx);
      client = mongoc_client_pool_pop (pool);
   } else {
      client = mongoc_client_new_from_uri (uri);
      uri_opts = mongoc_uri_get_options (client->uri);
      BSON_ASSERT (_mongoc_lookup_bool (uri_opts, "ssl", !expect_ssl) ==
                   expect_ssl);
      test_framework_set_ssl_opts (client);
      mongoc_client_set_apm_callbacks (client, callbacks, &ctx);
   }

   n_hosts = hosts_count (test);

   if (n_hosts && !expect_error) {
      r = mongoc_client_command_simple (
         client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);
      ASSERT_OR_PRINT (r, error);
      WAIT_UNTIL (_host_list_matches (test, &ctx));
   } else {
      r = mongoc_client_command_simple (
         client, "admin", tmp_bson ("{'ping': 1}"), NULL, NULL, &error);
      BSON_ASSERT (!r);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_SERVER_SELECTION,
                             MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                             "");
   }

   _assert_options_match (test, client);

   /* the client has a copy of the topology's URI, assert they're the same */
   ASSERT (bson_equal (mongoc_uri_get_options (client->uri),
                       mongoc_uri_get_options (client->topology->uri)));
   ASSERT (bson_equal (mongoc_uri_get_credentials (client->uri),
                       mongoc_uri_get_credentials (client->topology->uri)));
   if (!mongoc_uri_get_hosts (client->uri)) {
      ASSERT (!mongoc_uri_get_hosts (client->topology->uri));
   } else {
      _mongoc_host_list_equal (mongoc_uri_get_hosts (client->uri),
                               mongoc_uri_get_hosts (client->topology->uri));
   }

   if (pooled) {
      mongoc_client_pool_push (pool, client);
      mongoc_client_pool_destroy (pool);
   } else {
      mongoc_client_destroy (client);
   }

   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_uri_destroy (uri);
}


static void
test_dns (bson_t *test)
{
   _test_dns_maybe_pooled (test, false);
   _test_dns_maybe_pooled (test, true);
}


static int
test_dns_check (void)
{
   return test_framework_getenv_bool ("MONGOC_TEST_DNS") ? 1 : 0;
}


/* ensure mongoc_topology_select_server_id handles a NULL error pointer in the
 * code path it follows when the topology scanner is invalid */
static void
test_null_error_pointer (void *ctx)
{
   mongoc_client_t *client;

   client = mongoc_client_new ("mongodb+srv://doesntexist.example.com");
   ASSERT (!mongoc_topology_select_server_id (client->topology,
                                              MONGOC_SS_READ,
                                              NULL /* read prefs */,
                                              NULL /* error */));

   mongoc_client_destroy (client);
}


/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for mongodb+srv URIs.
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests (TestSuite *suite)
{
   char resolved[PATH_MAX];

   ASSERT (realpath (JSON_DIR "/initial_dns_seedlist_discovery", resolved));
   install_json_test_suite_with_check (suite,
                                       resolved,
                                       test_dns,
                                       test_dns_check,
                                       test_framework_skip_if_no_crypto);
}


void
test_dns_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
   TestSuite_AddFull (suite,
                      "/initial_dns_seedlist_discovery/null_error_pointer",
                      test_null_error_pointer,
                      NULL,
                      NULL,
                      test_framework_skip_if_no_crypto);
}
