/*
 * Copyright 2020-present MongoDB, Inc.
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

#include "entity-map.h"

#include "bsonutil/bson-parser.h"
#include "TestSuite.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"
#include "utlist.h"
#include "util.h"

/* TODO: use public API to reduce min heartbeat once CDRIVER-3130 is resolved.
 */
#include "mongoc-client-private.h"
#include "mongoc-topology-private.h"

#define REDUCED_HEARTBEAT_FREQUENCY_MS 500
#define REDUCED_MIN_HEARTBEAT_FREQUENCY_MS 50

struct _entity_findcursor_t {
   const bson_t *first_result;
   mongoc_cursor_t *cursor;
};

static void
entity_destroy (entity_t *entity);

entity_map_t *
entity_map_new ()
{
   return bson_malloc0 (sizeof (entity_map_t));
}

void
entity_map_destroy (entity_map_t *entity_map)
{
   entity_t *entity, *tmp;
   LL_FOREACH_SAFE (entity_map->entities, entity, tmp)
   {
      entity_destroy (entity);
   }
   bson_free (entity_map);
}

static bool
uri_apply_options (mongoc_uri_t *uri, bson_t *opts, bson_error_t *error)
{
   bson_iter_t iter;
   bool ret = false;
   bool wcSet = false;
   mongoc_write_concern_t *wc = NULL;

   /* There may be multiple URI options (w, wTimeoutMS, journal) for a write
    * concern. Parse all options before setting the write concern on the URI. */
   wc = mongoc_write_concern_new ();

   BSON_FOREACH (opts, iter)
   {
      const char *key;

      key = bson_iter_key (&iter);

      if (0 == strcmp ("readConcernLevel", key)) {
         mongoc_read_concern_t *rc = NULL;

         rc = mongoc_read_concern_new ();
         mongoc_read_concern_set_level (rc, bson_iter_utf8 (&iter, NULL));
         mongoc_uri_set_read_concern (uri, rc);
         mongoc_read_concern_destroy (rc);
      } else if (0 == strcmp ("w", key)) {
         wcSet = true;
         mongoc_write_concern_set_w (wc, bson_iter_int32 (&iter));
      } else if (mongoc_uri_option_is_int32 (key)) {
         mongoc_uri_set_option_as_int32 (uri, key, bson_iter_int32 (&iter));
      } else if (mongoc_uri_option_is_int64 (key)) {
         mongoc_uri_set_option_as_int64 (uri, key, bson_iter_int64 (&iter));
      } else if (mongoc_uri_option_is_bool (key)) {
         mongoc_uri_set_option_as_bool (uri, key, bson_iter_bool (&iter));
      } else if (0 == strcmp ("appname", key)) {
         mongoc_uri_set_appname (uri, bson_iter_utf8 (&iter, NULL));
      } else {
         test_set_error (
            error, "Unimplemented test runner support for URI option: %s", key);
         goto done;
      }
   }

   if (wcSet) {
      mongoc_uri_set_write_concern (uri, wc);
   }

   ret = true;

done:
   mongoc_write_concern_destroy (wc);
   return ret;
}

event_t *
event_new (char *type)
{
   event_t *event = NULL;

   event = bson_malloc0 (sizeof (event_t));
   event->type = bson_strdup (type);
   return event;
}

void
event_destroy (event_t *event)
{
   if (!event) {
      return;
   }

   bson_free (event->command_name);
   bson_free (event->database_name);
   bson_destroy (event->command);
   bson_destroy (event->reply);
   bson_free (event->type);
   bson_free (event);
}

static entity_t *
entity_new (const char *type)
{
   entity_t *entity = NULL;
   entity = bson_malloc0 (sizeof (entity_t));
   entity->type = bson_strdup (type);
   return entity;
}

static bool
is_sensitive_command (event_t *event)
{
   const bson_t *body = event->reply ? event->reply : event->command;
   BSON_ASSERT (body);
   return mongoc_apm_is_sensitive_command_message (event->command_name, body);
}

bool
should_ignore_event (entity_t *client_entity, event_t *event)
{
   bson_iter_t iter;

   if (0 == strcmp (event->command_name, "configureFailPoint")) {
      return true;
   }

   if (client_entity->ignore_command_monitoring_events) {
      BSON_FOREACH (client_entity->ignore_command_monitoring_events, iter)
      {
         if (0 == strcmp (event->command_name, bson_iter_utf8 (&iter, NULL))) {
            return true;
         }
      }
   }

   if (client_entity->observe_sensitive_commands &&
       *client_entity->observe_sensitive_commands) {
      return false;
   }

   /* Sensitive commands need to be ignored */
   return is_sensitive_command (event);
}

static void
command_started (const mongoc_apm_command_started_t *started)
{
   entity_t *entity = NULL;
   event_t *event = NULL;

   entity = (entity_t *) mongoc_apm_command_started_get_context (started);
   event = event_new ("commandStartedEvent");
   event->command =
      bson_copy (mongoc_apm_command_started_get_command (started));
   event->command_name =
      bson_strdup (mongoc_apm_command_started_get_command_name (started));

   event->database_name =
      bson_strdup (mongoc_apm_command_started_get_database_name (started));

   if (mongoc_apm_command_started_get_service_id (started)) {
      bson_oid_copy (mongoc_apm_command_started_get_service_id (started),
                     &event->service_id);
   }

   if (should_ignore_event (entity, event)) {
      event_destroy (event);
      return;
   }

   LL_APPEND (entity->events, event);
}

static void
command_failed (const mongoc_apm_command_failed_t *failed)
{
   entity_t *entity = NULL;
   event_t *event = NULL;

   entity = (entity_t *) mongoc_apm_command_failed_get_context (failed);
   event = event_new ("commandFailedEvent");
   event->reply = bson_copy (mongoc_apm_command_failed_get_reply (failed));
   event->command_name =
      bson_strdup (mongoc_apm_command_failed_get_command_name (failed));

   if (mongoc_apm_command_failed_get_service_id (failed)) {
      bson_oid_copy (mongoc_apm_command_failed_get_service_id (failed),
                     &event->service_id);
   }

   if (should_ignore_event (entity, event)) {
      event_destroy (event);
      return;
   }
   LL_APPEND (entity->events, event);
}

static void
command_succeeded (const mongoc_apm_command_succeeded_t *succeeded)
{
   entity_t *entity = NULL;
   event_t *event = NULL;

   entity = (entity_t *) mongoc_apm_command_succeeded_get_context (succeeded);
   event = event_new ("commandSucceededEvent");
   event->reply =
      bson_copy (mongoc_apm_command_succeeded_get_reply (succeeded));
   event->command_name =
      bson_strdup (mongoc_apm_command_succeeded_get_command_name (succeeded));

   if (mongoc_apm_command_succeeded_get_service_id (succeeded)) {
      bson_oid_copy (mongoc_apm_command_succeeded_get_service_id (succeeded),
                     &event->service_id);
   }

   if (should_ignore_event (entity, event)) {
      event_destroy (event);
      return;
   }
   LL_APPEND (entity->events, event);
}

entity_t *
entity_client_new (entity_map_t *em, bson_t *bson, bson_error_t *error)
{
   bson_parser_t *parser = NULL;
   entity_t *entity = NULL;
   mongoc_client_t *client = NULL;
   mongoc_uri_t *uri = NULL;
   bool ret = false;
   bson_iter_t iter;
   mongoc_apm_callbacks_t *callbacks = NULL;
   bson_t *uri_options = NULL;
   bool *use_multiple_mongoses = NULL;
   bson_t *observe_events = NULL;
   bson_t *store_events_as_entities = NULL;
   bson_t *server_api = NULL;
   bool can_reduce_heartbeat = false;
   mongoc_server_api_t *api = NULL;

   entity = entity_new ("client");
   parser = bson_parser_new ();
   bson_parser_utf8 (parser, "id", &entity->id);
   bson_parser_doc_optional (parser, "uriOptions", &uri_options);
   bson_parser_bool_optional (
      parser, "useMultipleMongoses", &use_multiple_mongoses);
   bson_parser_array_optional (parser, "observeEvents", &observe_events);
   bson_parser_array_optional (parser,
                               "ignoreCommandMonitoringEvents",
                               &entity->ignore_command_monitoring_events);
   bson_parser_doc_optional (parser, "serverApi", &server_api);
   bson_parser_bool_optional (
      parser, "observeSensitiveCommands", &entity->observe_sensitive_commands);
   bson_parser_array_optional (
      parser, "storeEventsAsEntities", &store_events_as_entities);

   if (!bson_parser_parse (parser, bson, error)) {
      goto done;
   }

   if (server_api) {
      bson_parser_t *sapi_parser = NULL;
      char *version = NULL;
      bool *strict = NULL;
      bool *deprecation_errors = NULL;
      mongoc_server_api_version_t api_version;

      sapi_parser = bson_parser_new ();
      bson_parser_utf8 (sapi_parser, "version", &version);
      bson_parser_bool_optional (sapi_parser, "strict", &strict);
      bson_parser_bool_optional (
         sapi_parser, "deprecationErrors", &deprecation_errors);
      if (!bson_parser_parse (sapi_parser, server_api, error)) {
         bson_parser_destroy_with_parsed_fields (sapi_parser);
         goto done;
      }

      BSON_ASSERT (
         mongoc_server_api_version_from_string (version, &api_version));
      api = mongoc_server_api_new (api_version);

      if (strict) {
         mongoc_server_api_strict (api, *strict);
      }

      if (deprecation_errors) {
         mongoc_server_api_deprecation_errors (api, *deprecation_errors);
      }

      bson_parser_destroy_with_parsed_fields (sapi_parser);
   }

   /* Build the client's URI. */
   uri = test_framework_get_uri ();
   /* Apply "useMultipleMongoses" rules to URI.
    * If useMultipleMongoses is true, modify the connection string to add a
    * host. If useMultipleMongoses is false, require that the connection string
    * has one host. If useMultipleMongoses unspecified, make no assertion.
    */
   if (test_framework_is_loadbalanced ()) {
      /* Quoting the unified test runner specification:
       * If the topology type is LoadBalanced, [...] If useMultipleMongoses is
       * true or unset, the test runner MUST use the URI of the load balancer
       * fronting multiple servers. Otherwise, the test runner MUST use the URI
       * of the load balancer fronting a single server.
       */
      if (use_multiple_mongoses == NULL || *use_multiple_mongoses == true) {
         mongoc_uri_destroy (uri);
         uri = test_framework_get_uri_multi_mongos_loadbalanced ();
      }
   } else if (use_multiple_mongoses != NULL) {
      if (!test_framework_uri_apply_multi_mongos (
               uri, *use_multiple_mongoses, error)) {
         goto done;
      }
   }

   if (uri_options) {
      /* Apply URI options. */
      if (!uri_apply_options (uri, uri_options, error)) {
         goto done;
      }
   }

   if (!mongoc_uri_has_option (uri, MONGOC_URI_HEARTBEATFREQUENCYMS)) {
      can_reduce_heartbeat = true;
   }

   if (can_reduce_heartbeat && em->reduced_heartbeat) {
      mongoc_uri_set_option_as_int32 (
         uri, MONGOC_URI_HEARTBEATFREQUENCYMS, REDUCED_HEARTBEAT_FREQUENCY_MS);
   }

   client = test_framework_client_new_from_uri (uri, api);
   test_framework_set_ssl_opts (client);
   entity->value = client;
   callbacks = mongoc_apm_callbacks_new ();

   if (can_reduce_heartbeat && em->reduced_heartbeat) {
      client->topology->min_heartbeat_frequency_msec =
         REDUCED_MIN_HEARTBEAT_FREQUENCY_MS;
   }

   if (observe_events) {
      BSON_FOREACH (observe_events, iter)
      {
         const char *event_type = bson_iter_utf8 (&iter, NULL);

         if (0 == strcmp (event_type, "commandStartedEvent")) {
            mongoc_apm_set_command_started_cb (callbacks, command_started);
         } else if (0 == strcmp (event_type, "commandFailedEvent")) {
            mongoc_apm_set_command_failed_cb (callbacks, command_failed);
         } else if (0 == strcmp (event_type, "commandSucceededEvent")) {
            mongoc_apm_set_command_succeeded_cb (callbacks, command_succeeded);
         } else if (is_unsupported_event_type (event_type)) {
            MONGOC_DEBUG ("Skipping observing unsupported event type: %s", event_type);
            continue;
         } else {
            test_set_error (error, "Unexpected event type: %s", event_type);
            goto done;
         }
      }
   }
   mongoc_client_set_apm_callbacks (client, callbacks, entity);

   if (store_events_as_entities) {
      /* TODO: CDRIVER-3867 Comprehensive Atlas Testing */
      test_set_error (error, "storeEventsAsEntities is not supported");
      goto done;
   }

   ret = true;
done:
   mongoc_uri_destroy (uri);
   bson_parser_destroy (parser);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_server_api_destroy (api);
   bson_destroy (uri_options);
   bson_free (use_multiple_mongoses);
   bson_destroy (observe_events);
   bson_destroy (store_events_as_entities);
   bson_destroy (server_api);
   if (!ret) {
      entity_destroy (entity);
      return NULL;
   }
   return entity;
}
typedef struct {
   mongoc_read_concern_t *rc;
   mongoc_write_concern_t *wc;
   mongoc_read_prefs_t *rp;
} coll_or_db_opts_t;

static coll_or_db_opts_t *
coll_or_db_opts_new ()
{
   return bson_malloc0 (sizeof (coll_or_db_opts_t));
}

static void
coll_or_db_opts_destroy (coll_or_db_opts_t *opts)
{
   if (!opts) {
      return;
   }
   mongoc_read_concern_destroy (opts->rc);
   mongoc_read_prefs_destroy (opts->rp);
   mongoc_write_concern_destroy (opts->wc);
   bson_free (opts);
}

static bool
coll_or_db_opts_parse (coll_or_db_opts_t *opts, bson_t *in, bson_error_t *error)
{
   bson_parser_t *parser = NULL;
   bool ret = false;

   parser = bson_parser_new ();
   bson_parser_read_concern_optional (parser, &opts->rc);
   bson_parser_read_prefs_optional (parser, &opts->rp);
   bson_parser_write_concern_optional (parser, &opts->wc);
   if (!bson_parser_parse (parser, in, error)) {
      goto done;
   }

   ret = true;
done:
   bson_parser_destroy (parser);
   return ret;
}

entity_t *
entity_database_new (entity_map_t *entity_map,
                     bson_t *bson,
                     bson_error_t *error)
{
   bson_parser_t *parser = NULL;
   entity_t *entity = NULL;
   const entity_t *client_entity;
   char *client_id = NULL;
   mongoc_client_t *client = NULL;
   mongoc_database_t *db = NULL;
   char *database_name = NULL;
   bool ret = false;
   bson_t *database_opts = NULL;
   coll_or_db_opts_t *coll_or_db_opts = NULL;

   entity = entity_new ("database");
   parser = bson_parser_new ();
   bson_parser_utf8 (parser, "id", &entity->id);
   bson_parser_utf8 (parser, "client", &client_id);
   bson_parser_utf8 (parser, "databaseName", &database_name);
   bson_parser_doc_optional (parser, "databaseOptions", &database_opts);

   if (!bson_parser_parse (parser, bson, error)) {
      goto done;
   }

   client_entity = entity_map_get (entity_map, client_id, error);
   if (!client_entity) {
      goto done;
   }

   client = (mongoc_client_t *) client_entity->value;
   db = mongoc_client_get_database (client, database_name);
   entity->value = (void *) db;

   if (database_opts) {
      coll_or_db_opts = coll_or_db_opts_new ();
      if (!coll_or_db_opts_parse (coll_or_db_opts, database_opts, error)) {
         goto done;
      }
      if (coll_or_db_opts->rc) {
         mongoc_database_set_read_concern (db, coll_or_db_opts->rc);
      }
      if (coll_or_db_opts->rp) {
         mongoc_database_set_read_prefs (db, coll_or_db_opts->rp);
      }
      if (coll_or_db_opts->wc) {
         mongoc_database_set_write_concern (db, coll_or_db_opts->wc);
      }
   }

   ret = true;
done:
   bson_free (client_id);
   bson_free (database_name);
   bson_parser_destroy (parser);
   bson_destroy (database_opts);
   coll_or_db_opts_destroy (coll_or_db_opts);
   if (!ret) {
      entity_destroy (entity);
      return NULL;
   }
   return entity;
}

entity_t *
entity_collection_new (entity_map_t *entity_map,
                       bson_t *bson,
                       bson_error_t *error)
{
   bson_parser_t *parser = NULL;
   entity_t *entity = NULL;
   entity_t *database_entity = NULL;
   mongoc_database_t *database = NULL;
   mongoc_collection_t *coll = NULL;
   bool ret = false;
   char *database_id = NULL;
   char *collection_name = NULL;
   bson_t *collection_opts = NULL;
   coll_or_db_opts_t *coll_or_db_opts = NULL;

   entity = entity_new ("collection");
   parser = bson_parser_new ();
   bson_parser_utf8 (parser, "id", &entity->id);
   bson_parser_utf8 (parser, "database", &database_id);
   bson_parser_utf8 (parser, "collectionName", &collection_name);
   bson_parser_doc_optional (parser, "collectionOptions", &collection_opts);
   if (!bson_parser_parse (parser, bson, error)) {
      goto done;
   }

   database_entity = entity_map_get (entity_map, database_id, error);
   if (!database_entity) {
      goto done;
   }
   database = (mongoc_database_t *) database_entity->value;
   coll = mongoc_database_get_collection (database, collection_name);
   entity->value = (void *) coll;
   if (collection_opts) {
      coll_or_db_opts = coll_or_db_opts_new ();
      if (!coll_or_db_opts_parse (coll_or_db_opts, collection_opts, error)) {
         goto done;
      }
      if (coll_or_db_opts->rc) {
         mongoc_collection_set_read_concern (coll, coll_or_db_opts->rc);
      }
      if (coll_or_db_opts->rp) {
         mongoc_collection_set_read_prefs (coll, coll_or_db_opts->rp);
      }
      if (coll_or_db_opts->wc) {
         mongoc_collection_set_write_concern (coll, coll_or_db_opts->wc);
      }
   }
   ret = true;
done:
   bson_free (collection_name);
   bson_free (database_id);
   bson_parser_destroy (parser);
   bson_destroy (collection_opts);
   coll_or_db_opts_destroy (coll_or_db_opts);
   if (!ret) {
      entity_destroy (entity);
      return NULL;
   }
   return entity;
}

mongoc_session_opt_t *
session_opts_new (bson_t *bson, bson_error_t *error)
{
   bool ret = false;
   mongoc_session_opt_t *opts = NULL;
   bson_parser_t *bp = NULL;
   bson_parser_t *bp_opts = NULL;
   bool *causal_consistency = NULL;
   bool *snapshot = NULL;
   bson_t *default_transaction_opts = NULL;
   mongoc_write_concern_t *wc = NULL;
   mongoc_read_concern_t *rc = NULL;
   mongoc_read_prefs_t *rp = NULL;
   mongoc_transaction_opt_t *topts = NULL;

   bp = bson_parser_new ();
   bson_parser_bool_optional (bp, "causalConsistency", &causal_consistency);
   bson_parser_bool_optional (bp, "snapshot", &snapshot);
   bson_parser_doc_optional (
      bp, "defaultTransactionOptions", &default_transaction_opts);
   if (!bson_parser_parse (bp, bson, error)) {
      goto done;
   }

   opts = mongoc_session_opts_new ();
   if (causal_consistency) {
      mongoc_session_opts_set_causal_consistency (opts, *causal_consistency);
   }
   if (snapshot) {
      mongoc_session_opts_set_snapshot (opts, *snapshot);
   }

   if (default_transaction_opts) {
      bp_opts = bson_parser_new ();
      topts = mongoc_transaction_opts_new ();

      bson_parser_write_concern_optional (bp_opts, &wc);
      bson_parser_read_concern_optional (bp_opts, &rc);
      bson_parser_read_prefs_optional (bp_opts, &rp);
      if (!bson_parser_parse (bp_opts, default_transaction_opts, error)) {
         goto done;
      }

      if (wc) {
         mongoc_transaction_opts_set_write_concern (topts, wc);
      }

      if (rc) {
         mongoc_transaction_opts_set_read_concern (topts, rc);
      }

      if (rp) {
         mongoc_transaction_opts_set_read_prefs (topts, rp);
      }

      mongoc_session_opts_set_default_transaction_opts (opts, topts);
   }

   ret = true;
done:
   bson_parser_destroy_with_parsed_fields (bp);
   bson_parser_destroy_with_parsed_fields (bp_opts);
   mongoc_transaction_opts_destroy (topts);
   if (!ret) {
      mongoc_session_opts_destroy (opts);
      return NULL;
   }
   return opts;
}

entity_t *
entity_session_new (entity_map_t *entity_map, bson_t *bson, bson_error_t *error)
{
   bson_parser_t *parser = NULL;
   entity_t *entity = NULL;
   entity_t *client_entity = NULL;
   mongoc_client_t *client = NULL;
   char *client_id = NULL;
   bson_t *session_opts_bson = NULL;
   mongoc_session_opt_t *session_opts = NULL;
   bool ret = false;
   mongoc_client_session_t *session = NULL;

   entity = entity_new ("session");
   parser = bson_parser_new ();
   bson_parser_utf8 (parser, "id", &entity->id);
   bson_parser_utf8 (parser, "client", &client_id);
   bson_parser_doc_optional (parser, "sessionOptions", &session_opts_bson);
   if (!bson_parser_parse (parser, bson, error)) {
      goto done;
   }

   client_entity = entity_map_get (entity_map, client_id, error);
   if (!client_entity) {
      goto done;
   }
   client = (mongoc_client_t *) client_entity->value;
   if (!client) {
      goto done;
   }
   if (session_opts_bson) {
      session_opts = session_opts_new (session_opts_bson, error);
      if (!session_opts) {
         goto done;
      }
   }
   session = mongoc_client_start_session (client, session_opts, error);
   if (!session) {
      goto done;
   }
   entity->value = session;
   /* Ending a session destroys the session object.
    * After a session is ended, match assertions may be made on the lsid.
    * So the lsid is copied from the session object on creation. */
   entity->lsid = bson_copy (mongoc_client_session_get_lsid (session));
   ret = true;

   entity->session_client_id = bson_strdup (client_id);
done:
   mongoc_session_opts_destroy (session_opts);
   bson_free (client_id);
   bson_destroy (session_opts_bson);
   bson_parser_destroy (parser);
   if (!ret) {
      entity_destroy (entity);
      return NULL;
   }
   return entity;
}

entity_t *
entity_bucket_new (entity_map_t *entity_map, bson_t *bson, bson_error_t *error)
{
   bson_parser_t *parser = NULL;
   entity_t *entity = NULL;
   mongoc_database_t *database = NULL;
   char *database_id = NULL;
   bool ret = false;
   bson_t *bucket_opts_bson = NULL;
   bson_parser_t *opts_parser = NULL;
   mongoc_read_concern_t *rc = NULL;
   mongoc_write_concern_t *wc = NULL;
   bson_t *opts = NULL;

   entity = entity_new ("bucket");
   parser = bson_parser_new ();
   bson_parser_utf8 (parser, "id", &entity->id);
   bson_parser_utf8 (parser, "database", &database_id);
   bson_parser_doc_optional (parser, "bucketOptions", &bucket_opts_bson);
   if (!bson_parser_parse (parser, bson, error)) {
      goto done;
   }

   database = entity_map_get_database (entity_map, database_id, error);
   if (!database) {
      goto done;
   }

   opts_parser = bson_parser_new ();
   bson_parser_allow_extra (opts_parser, true);
   bson_parser_read_concern_optional (opts_parser, &rc);
   bson_parser_write_concern_optional (opts_parser, &wc);
   opts = bson_new ();
   bson_concat (opts, bson_parser_get_extra (opts_parser));
   if (rc) {
      mongoc_read_concern_append (rc, opts);
   }
   if (wc) {
      mongoc_write_concern_append (wc, opts);
   }

   entity->value =
      mongoc_gridfs_bucket_new (database, opts, NULL /* read prefs */, error);
   if (!entity->value) {
      goto done;
   }

   ret = true;
done:
   bson_free (database_id);
   bson_destroy (bucket_opts_bson);
   bson_parser_destroy (parser);
   bson_parser_destroy_with_parsed_fields (opts_parser);
   bson_destroy (opts);
   if (!ret) {
      entity_destroy (entity);
      return NULL;
   }
   return entity;
}

/* Caveat: The spec encourages, but does not require, that entities are defined
 * in dependency order:
 * "Test files SHOULD define entities in dependency order, such that all
 * referenced entities (e.g. client) are defined before any of their dependent
 * entities (e.g. database, session)."
 * If a test ever does break this pattern (flipping dependency order), that can
 * be solved by:
 * - creating C objects lazily in entity_map_get.
 * - creating entities in dependency order (all clients first, then databases,
 *   etc.)
 * The current implementation here does the simple thing and creates the C
 * object immediately.
 */
bool
entity_map_create (entity_map_t *entity_map, bson_t *bson, bson_error_t *error)
{
   bson_iter_t iter;
   const char *entity_type;
   bson_t entity_bson;
   entity_t *entity = NULL;
   entity_t *entity_iter = NULL;
   bool ret = false;

   bson_iter_init (&iter, bson);
   if (!bson_iter_next (&iter)) {
      test_set_error (error, "Empty entity");
      goto done;
   }

   entity_type = bson_iter_key (&iter);
   bson_iter_bson (&iter, &entity_bson);
   if (bson_iter_next (&iter)) {
      test_set_error (error,
                      "Extra field in entity: %s: %s",
                      bson_iter_key (&iter),
                      tmp_json (bson));
      goto done;
   }

   if (0 == strcmp (entity_type, "client")) {
      entity = entity_client_new (entity_map, &entity_bson, error);
   } else if (0 == strcmp (entity_type, "database")) {
      entity = entity_database_new (entity_map, &entity_bson, error);
   } else if (0 == strcmp (entity_type, "collection")) {
      entity = entity_collection_new (entity_map, &entity_bson, error);
   } else if (0 == strcmp (entity_type, "session")) {
      entity = entity_session_new (entity_map, &entity_bson, error);
   } else if (0 == strcmp (entity_type, "bucket")) {
      entity = entity_bucket_new (entity_map, &entity_bson, error);
   } else {
      test_set_error (
         error, "Unknown entity type: %s: %s", entity_type, tmp_json (bson));
      goto done;
   }

   if (!entity) {
      goto done;
   }

   LL_FOREACH (entity_map->entities, entity_iter)
   {
      if (0 == strcmp (entity_iter->id, entity->id)) {
         test_set_error (
            error, "Attempting to create duplicate entity: '%s'", entity->id);
         entity_destroy (entity);
         goto done;
      }
   }

   ret = true;
done:
   if (!ret) {
      entity_destroy (entity);
   } else {
      LL_PREPEND (entity_map->entities, entity);
   }
   return ret;
}

static void
entity_destroy (entity_t *entity)
{
   event_t *event = NULL;
   event_t *tmp = NULL;

   if (!entity) {
      return;
   }

   BSON_ASSERT (entity->type);

   if (0 == strcmp ("client", entity->type)) {
      mongoc_client_t *client = NULL;

      client = (mongoc_client_t *) entity->value;
      mongoc_client_destroy (client);
   } else if (0 == strcmp ("database", entity->type)) {
      mongoc_database_t *db = NULL;

      db = (mongoc_database_t *) entity->value;
      mongoc_database_destroy (db);
   } else if (0 == strcmp ("collection", entity->type)) {
      mongoc_collection_t *coll = NULL;

      coll = (mongoc_collection_t *) entity->value;
      mongoc_collection_destroy (coll);
   } else if (0 == strcmp ("session", entity->type)) {
      mongoc_client_session_t *sess = NULL;

      sess = (mongoc_client_session_t *) entity->value;
      mongoc_client_session_destroy (sess);
   } else if (0 == strcmp ("changestream", entity->type)) {
      mongoc_change_stream_t *changestream = NULL;

      changestream = (mongoc_change_stream_t *) entity->value;
      mongoc_change_stream_destroy (changestream);
   } else if (0 == strcmp ("bson", entity->type)) {
      bson_val_t *value = entity->value;

      bson_val_destroy (value);
   } else if (0 == strcmp ("bucket", entity->type)) {
      mongoc_gridfs_bucket_t *bucket = entity->value;

      mongoc_gridfs_bucket_destroy (bucket);
   } else if (0 == strcmp ("findcursor", entity->type)) {
      entity_findcursor_t *findcursor = entity->value;

      mongoc_cursor_destroy (findcursor->cursor);
      bson_free (findcursor);
   } else {
      test_error ("Attempting to destroy unrecognized entity type: %s, id: %s",
                  entity->type,
                  entity->id);
   }

   LL_FOREACH_SAFE (entity->events, event, tmp)
   {
      event_destroy (event);
   }

   bson_destroy (entity->ignore_command_monitoring_events);
   bson_free (entity->type);
   bson_free (entity->id);
   bson_destroy (entity->lsid);
   bson_free (entity->session_client_id);
   bson_free (entity->observe_sensitive_commands);
   bson_free (entity);
}

entity_t *
entity_map_get (entity_map_t *entity_map, const char *id, bson_error_t *error)
{
   entity_t *entity = NULL;
   LL_FOREACH (entity_map->entities, entity)
   {
      if (0 == strcmp (entity->id, id)) {
         return entity;
      }
   }

   test_set_error (error, "Entity '%s' not found", id);
   return NULL;
}

bool
entity_map_delete (entity_map_t *em, const char *id, bson_error_t *error)
{
   entity_t *entity = entity_map_get (em, id, error);
   if (!entity) {
      return false;
   }

   LL_DELETE (em->entities, entity);
   entity_destroy (entity);

   return true;
}

static entity_t *
_entity_map_get_by_type (entity_map_t *entity_map,
                         const char *id,
                         const char *type,
                         bson_error_t *error)
{
   entity_t *entity = NULL;

   entity = entity_map_get (entity_map, id, error);
   if (!entity) {
      return NULL;
   }

   if (0 != strcmp (entity->type, type)) {
      test_set_error (error,
                      "Unexpected entity type. Expected: %s, got %s",
                      type,
                      entity->type);
      return NULL;
   }
   return entity;
}

mongoc_client_t *
entity_map_get_client (entity_map_t *entity_map,
                       const char *id,
                       bson_error_t *error)
{
   entity_t *entity = _entity_map_get_by_type (entity_map, id, "client", error);
   if (!entity) {
      return NULL;
   }
   return (mongoc_client_t *) entity->value;
}

mongoc_database_t *
entity_map_get_database (entity_map_t *entity_map,
                         const char *id,
                         bson_error_t *error)
{
   entity_t *entity =
      _entity_map_get_by_type (entity_map, id, "database", error);
   if (!entity) {
      return NULL;
   }
   return (mongoc_database_t *) entity->value;
}

mongoc_collection_t *
entity_map_get_collection (entity_map_t *entity_map,
                           const char *id,
                           bson_error_t *error)
{
   entity_t *entity =
      _entity_map_get_by_type (entity_map, id, "collection", error);
   if (!entity) {
      return NULL;
   }
   return (mongoc_collection_t *) entity->value;
}

mongoc_change_stream_t *
entity_map_get_changestream (entity_map_t *entity_map,
                             const char *id,
                             bson_error_t *error)
{
   entity_t *entity =
      _entity_map_get_by_type (entity_map, id, "changestream", error);
   if (!entity) {
      return NULL;
   }
   return (mongoc_change_stream_t *) entity->value;
}

entity_findcursor_t *
entity_map_get_findcursor (entity_map_t *entity_map,
                           const char *id,
                           bson_error_t *error)
{
   entity_t *entity =
      _entity_map_get_by_type (entity_map, id, "findcursor", error);
   if (!entity) {
      return NULL;
   }
   return (entity_findcursor_t *) entity->value;
}

bson_val_t *
entity_map_get_bson (entity_map_t *entity_map,
                     const char *id,
                     bson_error_t *error)
{
   entity_t *entity = _entity_map_get_by_type (entity_map, id, "bson", error);
   if (!entity) {
      return NULL;
   }
   return (bson_val_t *) entity->value;
}

mongoc_client_session_t *
entity_map_get_session (entity_map_t *entity_map,
                        const char *id,
                        bson_error_t *error)
{
   entity_t *entity =
      _entity_map_get_by_type (entity_map, id, "session", error);
   if (!entity) {
      return NULL;
   }
   if (!entity->value) {
      test_set_error (
         error,
         "entity: %s is an ended session that is no longer valid to use",
         id);
      return NULL;
   }
   return (mongoc_client_session_t *) entity->value;
}

static bson_t *
entity_map_get_lsid (entity_map_t *em, char *session_id, bson_error_t *error)
{
   entity_t *entity = NULL;

   entity = entity_map_get (em, session_id, error);
   if (!entity) {
      return NULL;
   }
   if (!entity->lsid) {
      test_set_error (error,
                      "entity %s of type %s does not have an lsid",
                      session_id,
                      entity->type);
      return NULL;
   }
   return entity->lsid;
}

mongoc_gridfs_bucket_t *
entity_map_get_bucket (entity_map_t *entity_map,
                       const char *id,
                       bson_error_t *error)
{
   entity_t *entity = _entity_map_get_by_type (entity_map, id, "bucket", error);
   if (!entity) {
      return NULL;
   }
   return (mongoc_gridfs_bucket_t *) entity->value;
}

static bool
_entity_map_add (entity_map_t *em,
                 const char *id,
                 const char *type,
                 void *value,
                 bson_error_t *error)
{
   bson_error_t tmperr;
   entity_t *entity = NULL;

   if (NULL != entity_map_get (em, id, &tmperr)) {
      test_set_error (error, "Attempting to overwrite entity: %s", id);
      return false;
   }

   entity = entity_new (type);
   entity->value = value;
   entity->id = bson_strdup (id);
   LL_PREPEND (em->entities, entity);
   return true;
}

bool
entity_map_add_changestream (entity_map_t *em,
                             const char *id,
                             mongoc_change_stream_t *changestream,
                             bson_error_t *error)
{
   return _entity_map_add (
      em, id, "changestream", (void *) changestream, error);
}

void
entity_findcursor_iterate_until_document_or_error (
   entity_findcursor_t *findcursor,
   const bson_t **document,
   bson_error_t *error,
   const bson_t **error_document)
{
   *document = NULL;

   if (findcursor->first_result) {
      *document = findcursor->first_result;
      findcursor->first_result = NULL;
      return;
   }

   while (!mongoc_cursor_next (findcursor->cursor, document)) {
      if (mongoc_cursor_error_document (
             findcursor->cursor, error, error_document)) {
         return;
      }
   }
}

bool
entity_map_add_findcursor (entity_map_t *em,
                           const char *id,
                           mongoc_cursor_t *cursor,
                           const bson_t *first_result,
                           bson_error_t *error)
{
   entity_findcursor_t *findcursor;

   findcursor =
      (entity_findcursor_t *) bson_malloc0 (sizeof (entity_findcursor_t));
   findcursor->cursor = cursor;
   findcursor->first_result = first_result;
   return _entity_map_add (em, id, "findcursor", (void *) findcursor, error);
}

bool
entity_map_add_bson (entity_map_t *em,
                     const char *id,
                     bson_val_t *val,
                     bson_error_t *error)
{
   return _entity_map_add (em, id, "bson", (void *) bson_val_copy (val), error);
}

/* implement $$sessionLsid */
static bool
special_session_lsid (bson_matcher_t *matcher,
                      const bson_t *assertion,
                      const bson_val_t *actual,
                      void *ctx,
                      const char *path,
                      bson_error_t *error)
{
   bool ret = false;
   const char *id;
   bson_val_t *session_val = NULL;
   bson_t *lsid = NULL;
   entity_map_t *em = (entity_map_t *) ctx;
   bson_iter_t iter;

   bson_iter_init (&iter, assertion);
   bson_iter_next (&iter);

   if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
      test_set_error (error,
                      "unexpected $$sessionLsid does not contain utf8: %s",
                      tmp_json (assertion));
      goto done;
   }

   id = bson_iter_utf8 (&iter, NULL);
   lsid = entity_map_get_lsid (em, (char *) id, error);
   if (!lsid) {
      goto done;
   }

   session_val = bson_val_from_bson (lsid);
   if (!bson_matcher_match (matcher, session_val, actual, path, false, error)) {
      goto done;
   }


   ret = true;
done:
   bson_val_destroy (session_val);
   return ret;
}

/* implement $$matchesEntity */
bool
special_matches_entity (bson_matcher_t *matcher,
                        const bson_t *assertion,
                        const bson_val_t *actual,
                        void *ctx,
                        const char *path,
                        bson_error_t *error)
{
   bool ret = false;
   bson_iter_t iter;
   entity_map_t *em = (entity_map_t *) ctx;
   bson_val_t *entity_val = NULL;
   const char *id;

   bson_iter_init (&iter, assertion);
   BSON_ASSERT (bson_iter_next (&iter));

   if (!BSON_ITER_HOLDS_UTF8 (&iter)) {
      test_set_error (error,
                      "unexpected $$matchesEntity does not contain utf8: %s",
                      tmp_json (assertion));
      goto done;
   }

   id = bson_iter_utf8 (&iter, NULL);
   entity_val = entity_map_get_bson (em, id, error);
   if (!entity_val) {
      goto done;
   }

   if (!bson_matcher_match (matcher, entity_val, actual, path, false, error)) {
      goto done;
   }

   ret = true;
done:
   return ret;
}

bool
entity_map_match (entity_map_t *em,
                  const bson_val_t *expected,
                  const bson_val_t *actual,
                  bool allow_extra,
                  bson_error_t *error)
{
   bson_matcher_t *matcher;
   bool ret;

   matcher = bson_matcher_new ();
   bson_matcher_add_special (
      matcher, "$$sessionLsid", special_session_lsid, em);
   bson_matcher_add_special (
      matcher, "$$matchesEntity", special_matches_entity, em);
   ret = bson_matcher_match (matcher, expected, actual, "", allow_extra, error);
   bson_matcher_destroy (matcher);
   return ret;
}

char *
event_list_to_string (event_t *events)
{
   bson_string_t *str = NULL;
   event_t *eiter = NULL;

   str = bson_string_new ("");
   LL_FOREACH (events, eiter)
   {
      bson_string_append_printf (str, "- %s:", eiter->type);
      if (eiter->command_name) {
         bson_string_append_printf (str, " cmd=%s", eiter->command_name);
      }
      if (eiter->database_name) {
         bson_string_append_printf (str, " db=%s", eiter->database_name);
      }
      if (eiter->command) {
         bson_string_append_printf (str, " sent %s", tmp_json (eiter->command));
      }
      if (eiter->reply) {
         bson_string_append_printf (
            str, " received %s", tmp_json (eiter->reply));
      }
      bson_string_append (str, "\n");
   }
   return bson_string_free (str, false);
}


bool
entity_map_end_session (entity_map_t *em, char *session_id, bson_error_t *error)
{
   bool ret = false;
   entity_t *entity = NULL;

   entity = entity_map_get (em, session_id, error);
   if (!entity) {
      goto done;
   }

   if (0 != strcmp (entity->type, "session")) {
      test_set_error (
         error, "expected session for %s but got %s", session_id, entity->type);
      goto done;
   }

   mongoc_client_session_destroy ((mongoc_client_session_t *) entity->value);
   entity->value = NULL;
   ret = true;
done:
   return ret;
}

char *
entity_map_get_session_client_id (entity_map_t *em,
                                  char *session_id,
                                  bson_error_t *error)
{
   char *ret = NULL;
   entity_t *entity = NULL;

   entity = entity_map_get (em, session_id, error);
   if (!entity) {
      goto done;
   }

   if (0 != strcmp (entity->type, "session")) {
      test_set_error (
         error, "expected session for %s but got %s", session_id, entity->type);
      goto done;
   }

   ret = entity->session_client_id;
done:
   return ret;
}

void
entity_map_set_reduced_heartbeat (entity_map_t *em, bool val)
{
   em->reduced_heartbeat = val;
}

void
entity_map_disable_event_listeners (entity_map_t *em)
{
   entity_t *eiter = NULL;

   LL_FOREACH (em->entities, eiter)
   {
      if (0 == strcmp (eiter->type, "client")) {
         mongoc_client_t *client = (mongoc_client_t *) eiter->value;

         mongoc_client_set_apm_callbacks (client, NULL, NULL);
      }
   }
}
