#include <mongoc.h>
#include <mongoc-collection-private.h>
#include <mongoc-server-description-private.h>
#include <mongoc-util-private.h>

#include "json-test.h"
#include "test-libmongoc.h"
#include "mock_server/mock-rs.h"
#include "mock_server/future.h"
#include "mock_server/future-functions.h"


static int
check_server_version (const bson_t *test)
{
   const char *s;
   char *padded;
   server_version_t test_version, server_version;

   if (bson_has_field (test, "maxServerVersion")) {
      s = bson_lookup_utf8 (test, "maxServerVersion");
      /* s is like "3.0", don't skip if server is 3.0.x but skip 3.1+ */
      padded = bson_strdup_printf ("%s.99", s);
      test_version = test_framework_str_to_version (padded);
      bson_free (padded);
      server_version = test_framework_get_server_version ();

      return server_version <= test_version;
   }

   if (bson_has_field (test, "minServerVersion")) {
      s = bson_lookup_utf8 (test, "minServerVersion");
      test_version = test_framework_str_to_version (s);
      server_version = test_framework_get_server_version ();

      return server_version >= test_version;
   }

   /* server version is ok, don't skip the test */
   return true;
}


static void
insert_data (mongoc_collection_t *collection, const bson_t *scenario)
{
   bson_error_t error;
   mongoc_bulk_operation_t *bulk;
   bson_t documents;
   bson_iter_t iter;
   uint32_t server_id;
   bson_t reply;

   if (!mongoc_collection_drop (collection, &error)) {
      if (strcmp (error.message, "ns not found")) {
         /* an error besides ns not found */
         ASSERT_OR_PRINT (false, error);
      }
   }

   bulk = mongoc_collection_create_bulk_operation (collection, true, NULL);

   bson_lookup_doc (scenario, "data", &documents);
   bson_iter_init (&iter, &documents);

   while (bson_iter_next (&iter)) {
      bson_t document;
      bool success;
      bson_t opts = BSON_INITIALIZER;

      bson_iter_bson (&iter, &document);
      success = mongoc_bulk_operation_insert_with_opts (
         bulk, &document, &opts, &error);
      ASSERT_OR_PRINT (success, error);
   }

   server_id = mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_OR_PRINT (server_id, error);

   mongoc_bulk_operation_destroy (bulk);
}


static void
activate_fail_point (mongoc_client_t *client,
                     uint32_t server_id,
                     const bson_t *opts)
{
   bson_t *command;
   bool success;
   bson_error_t error;

   command = tmp_bson ("{'configureFailPoint': 'onPrimaryTransactionalWrite'}");
   bson_copy_to_excluding_noinit (opts, command, "configureFailPoint", NULL);

   success = mongoc_client_command_simple_with_server_id (
      client, "admin", command, NULL, server_id, NULL, &error);
   ASSERT_OR_PRINT (success, error);
}


static void
deactivate_fail_point (mongoc_client_t *client, uint32_t server_id)
{
   bson_t *command;
   bool success;
   bson_error_t error;

   command = tmp_bson (
      "{'configureFailPoint': 'onPrimaryTransactionalWrite', 'mode': 'off'}");

   success = mongoc_client_command_simple_with_server_id (
      client, "admin", command, NULL, server_id, NULL, &error);
   ASSERT_OR_PRINT (success, error);
}


static void
add_request_to_bulk (mongoc_bulk_operation_t *bulk, bson_t *request)
{
   const char *name;
   bson_t args;
   bool success;
   bson_t opts = BSON_INITIALIZER;
   bson_error_t error;

   name = bson_lookup_utf8 (request, "name");
   bson_lookup_doc (request, "arguments", &args);

   if (!strcmp (name, "deleteOne")) {
      bson_t filter;

      bson_lookup_doc (&args, "filter", &filter);

      success = mongoc_bulk_operation_remove_one_with_opts (
         bulk, &filter, &opts, &error);
   } else if (!strcmp (name, "insertOne")) {
      bson_t document;

      bson_lookup_doc (&args, "document", &document);

      success = mongoc_bulk_operation_insert_with_opts (
         bulk, &document, &opts, &error);
   } else if (!strcmp (name, "replaceOne")) {
      bson_t filter;
      bson_t replacement;

      bson_lookup_doc (&args, "filter", &filter);
      bson_lookup_doc (&args, "replacement", &replacement);

      if (bson_has_field (&args, "upsert")) {
         BSON_APPEND_BOOL (
            &opts, "upsert", _mongoc_lookup_bool (&args, "upsert", false));
      }

      success = mongoc_bulk_operation_replace_one_with_opts (
         bulk, &filter, &replacement, &opts, &error);
   } else if (!strcmp (name, "updateOne")) {
      bson_t filter;
      bson_t update;

      bson_lookup_doc (&args, "filter", &filter);
      bson_lookup_doc (&args, "update", &update);

      if (bson_has_field (&args, "upsert")) {
         BSON_APPEND_BOOL (
            &opts, "upsert", _mongoc_lookup_bool (&args, "upsert", false));
      }

      success = mongoc_bulk_operation_update_one_with_opts (
         bulk, &filter, &update, &opts, &error);
   } else {
      test_error ("unrecognized request name %s", name);
      abort ();
   }

   ASSERT_OR_PRINT (success, error);
}


static bson_t *
convert_spec_result_to_bulk_write_result (const bson_t *spec_result)
{
   bson_t *result;
   bson_iter_t iter;

   result = tmp_bson ("{}");

   ASSERT (bson_iter_init (&iter, spec_result));

   while (bson_iter_next (&iter)) {
      /* libmongoc does not report inserted IDs, so ignore those fields */
      if (BSON_ITER_IS_KEY (&iter, "insertedCount")) {
         BSON_APPEND_VALUE (result, "nInserted", bson_iter_value (&iter));
      }
      if (BSON_ITER_IS_KEY (&iter, "deletedCount")) {
         BSON_APPEND_VALUE (result, "nRemoved", bson_iter_value (&iter));
      }
      if (BSON_ITER_IS_KEY (&iter, "matchedCount")) {
         BSON_APPEND_VALUE (result, "nMatched", bson_iter_value (&iter));
      }
      if (BSON_ITER_IS_KEY (&iter, "modifiedCount")) {
         BSON_APPEND_VALUE (result, "nModified", bson_iter_value (&iter));
      }
      if (BSON_ITER_IS_KEY (&iter, "upsertedCount")) {
         BSON_APPEND_VALUE (result, "nUpserted", bson_iter_value (&iter));
      }
      /* convert a single-statement upsertedId result field to a bulk write
       * upsertedIds result field */
      if (BSON_ITER_IS_KEY (&iter, "upsertedId")) {
         bson_t upserted;
         bson_t upsert;

         BSON_APPEND_ARRAY_BEGIN (result, "upserted", &upserted);
         BSON_APPEND_DOCUMENT_BEGIN (&upserted, "0", &upsert);
         BSON_APPEND_INT32 (&upsert, "index", 0);
         BSON_APPEND_VALUE (&upsert, "_id", bson_iter_value (&iter));
         bson_append_document_end (&upserted, &upsert);
         bson_append_array_end (result, &upserted);
      }
      if (BSON_ITER_IS_KEY (&iter, "upsertedIds")) {
         bson_t upserted;
         bson_iter_t inner;
         uint32_t i = 0;

         ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));

         /* include the "upserted" field if upsertedIds isn't empty */
         ASSERT (bson_iter_recurse (&iter, &inner));
         while (bson_iter_next (&inner)) {
            i++;
         }

         if (i) {
            i = 0;
            ASSERT (bson_iter_recurse (&iter, &inner));
            BSON_APPEND_ARRAY_BEGIN (result, "upserted", &upserted);

            while (bson_iter_next (&inner)) {
               bson_t upsert;
               const char *keyptr = NULL;
               char key[12];

               bson_uint32_to_string (i++, &keyptr, key, sizeof key);

               BSON_APPEND_DOCUMENT_BEGIN (&upserted, keyptr, &upsert);
               BSON_APPEND_INT32 (
                  &upsert, "index", atoi (bson_iter_key (&inner)));
               BSON_APPEND_VALUE (&upsert, "_id", bson_iter_value (&inner));
               bson_append_document_end (&upserted, &upsert);
            }

            bson_append_array_end (result, &upserted);
         }
      }
   }

   return result;
}


static void
execute_bulk_operation (mongoc_bulk_operation_t *bulk, const bson_t *test)
{
   uint32_t server_id;
   bson_t reply;
   bson_error_t error;

   server_id = mongoc_bulk_operation_execute (bulk, &reply, &error);

   if (_mongoc_lookup_bool (test, "outcome.error", false)) {
      ASSERT (!server_id);
   } else {
      ASSERT_OR_PRINT (server_id, error);
   }

   if (bson_has_field (test, "outcome.result")) {
      bson_t spec_result;
      bson_t *expected_result;

      bson_lookup_doc (test, "outcome.result", &spec_result);
      expected_result = convert_spec_result_to_bulk_write_result (&spec_result);

      ASSERT (match_bson (&reply, expected_result, false));
   }
}


static bson_t *
create_bulk_write_opts (const bson_t *test, mongoc_client_session_t *session)
{
   bson_t *opts;

   opts = tmp_bson ("{}");

   BSON_APPEND_BOOL (
      opts,
      "ordered",
      _mongoc_lookup_bool (test, "operation.arguments.options.ordered", true));

   if (session) {
      bool success;
      bson_error_t error;

      success = mongoc_client_session_append (session, opts, &error);

      ASSERT_OR_PRINT (success, error);
   }

   return opts;
}


static void
bulk_write (mongoc_collection_t *collection,
            const bson_t *test,
            mongoc_client_session_t *session)
{
   bson_t *opts;
   mongoc_bulk_operation_t *bulk;
   bson_t requests;
   bson_iter_t iter;

   opts = create_bulk_write_opts (test, session);
   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, opts);

   bson_lookup_doc (test, "operation.arguments.requests", &requests);
   ASSERT (bson_iter_init (&iter, &requests));

   while (bson_iter_next (&iter)) {
      bson_t request;

      bson_iter_bson (&iter, &request);
      add_request_to_bulk (bulk, &request);
   }

   execute_bulk_operation (bulk, test);
   mongoc_bulk_operation_destroy (bulk);
}


static void
single_write (mongoc_collection_t *collection,
              const bson_t *test,
              mongoc_client_session_t *session)
{
   bson_t *opts;
   mongoc_bulk_operation_t *bulk;
   bson_t operation;

   opts = create_bulk_write_opts (test, session);
   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, opts);

   bson_lookup_doc (test, "operation", &operation);
   add_request_to_bulk (bulk, &operation);

   execute_bulk_operation (bulk, test);
   mongoc_bulk_operation_destroy (bulk);
}


static mongoc_find_and_modify_opts_t *
create_find_and_modify_opts (const char *name,
                             const bson_t *args,
                             mongoc_client_session_t *session)
{
   mongoc_find_and_modify_opts_t *opts;
   mongoc_find_and_modify_flags_t flags = 0;

   opts = mongoc_find_and_modify_opts_new ();

   if (!strcmp (name, "findOneAndDelete")) {
      flags |= MONGOC_FIND_AND_MODIFY_REMOVE;
   }

   if (!strcmp (name, "findOneAndReplace")) {
      bson_t replacement;
      bson_lookup_doc (args, "replacement", &replacement);
      mongoc_find_and_modify_opts_set_update (opts, &replacement);
   }

   if (!strcmp (name, "findOneAndUpdate")) {
      bson_t update;
      bson_lookup_doc (args, "update", &update);
      mongoc_find_and_modify_opts_set_update (opts, &update);
   }

   if (bson_has_field (args, "sort")) {
      bson_t sort;
      bson_lookup_doc (args, "sort", &sort);
      mongoc_find_and_modify_opts_set_sort (opts, &sort);
   }

   if (_mongoc_lookup_bool (args, "upsert", false)) {
      flags |= MONGOC_FIND_AND_MODIFY_UPSERT;
   }

   if (bson_has_field (args, "returnDocument") &&
       !strcmp ("After", bson_lookup_utf8 (args, "returnDocument"))) {
      flags |= MONGOC_FIND_AND_MODIFY_RETURN_NEW;
   }

   mongoc_find_and_modify_opts_set_flags (opts, flags);

   if (session) {
      bool success;
      bson_t extra = BSON_INITIALIZER;
      bson_error_t error;

      success = mongoc_client_session_append (session, &extra, &error);
      ASSERT_OR_PRINT (success, error);

      ASSERT (mongoc_find_and_modify_opts_append (opts, &extra));
      bson_destroy (&extra);
   }

   return opts;
}


static void
find_and_modify (mongoc_collection_t *collection,
                 bson_t *test,
                 mongoc_client_session_t *session)
{
   const char *name;
   bson_t args;
   bson_t filter;
   mongoc_find_and_modify_opts_t *opts;
   bool success;
   bson_t reply;
   bson_error_t error;

   name = bson_lookup_utf8 (test, "operation.name");
   bson_lookup_doc (test, "operation.arguments", &args);
   bson_lookup_doc (test, "operation.arguments.filter", &filter);

   opts = create_find_and_modify_opts (name, &args, session);
   success = mongoc_collection_find_and_modify_with_opts (
      collection, &filter, opts, &reply, &error);
   mongoc_find_and_modify_opts_destroy (opts);

   if (_mongoc_lookup_bool (test, "outcome.error", false)) {
      ASSERT (!success);
   } else {
      ASSERT_OR_PRINT (success, error);
   }

   if (bson_has_field (test, "outcome.result")) {
      bson_t expected_result;
      bson_t reply_result;

      bson_lookup_doc (test, "outcome.result", &expected_result);
      bson_lookup_doc (&reply, "value", &reply_result);

      ASSERT (match_bson (&reply_result, &expected_result, false));
   }
}


static void
insert_many (mongoc_collection_t *collection,
             const bson_t *test,
             mongoc_client_session_t *session)
{
   bson_t *opts;
   mongoc_bulk_operation_t *bulk;
   bson_t documents;
   bson_iter_t iter;

   opts = create_bulk_write_opts (test, session);
   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, opts);

   bson_lookup_doc (test, "operation.arguments.documents", &documents);
   ASSERT (bson_iter_init (&iter, &documents));

   while (bson_iter_next (&iter)) {
      bson_t document;
      bool success;
      bson_error_t error;

      bson_iter_bson (&iter, &document);
      success =
         mongoc_bulk_operation_insert_with_opts (bulk, &document, NULL, &error);
      ASSERT_OR_PRINT (success, error);
   }

   execute_bulk_operation (bulk, test);
   mongoc_bulk_operation_destroy (bulk);
}


static void
check_outcome_collection (mongoc_collection_t *collection, bson_t *test)
{
   bson_t data;
   bson_iter_t iter;
   mongoc_cursor_t *cursor;
   bson_t query = BSON_INITIALIZER;

   bson_lookup_doc (test, "outcome.collection.data", &data);
   ASSERT (bson_iter_init (&iter, &data));

   cursor = mongoc_collection_find_with_opts (collection, &query, NULL, NULL);

   while (bson_iter_next (&iter)) {
      bson_t expected_doc;
      const bson_t *actual_doc;

      bson_iter_bson (&iter, &expected_doc);
      ASSERT_CURSOR_NEXT (cursor, &actual_doc);
      ASSERT (match_bson (actual_doc, &expected_doc, false));
   }

   ASSERT_CURSOR_DONE (cursor);
}


static void
execute_test (mongoc_collection_t *collection,
              bson_t *test,
              mongoc_client_session_t *session)
{
   uint32_t server_id = 0;
   bson_error_t error;
   const char *op_name;

   if (test_suite_debug_output ()) {
      const char *description = bson_lookup_utf8 (test, "description");
      printf ("  - %s (%s session)\n",
              description,
              session ? "explicit" : "implicit");
      fflush (stdout);
   }

   if (!check_server_version (test) ||
       !test_framework_skip_if_not_rs_version_6 ()) {
      if (test_suite_debug_output ()) {
         printf ("      SKIP, server version or not rs version 6\n");
         fflush (stdout);
      }

      goto done;
   }

   /* Select a primary for testing */
   server_id = mongoc_topology_select_server_id (
      collection->client->topology, MONGOC_SS_WRITE, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);

   if (bson_has_field (test, "failPoint")) {
      bson_t opts;

      bson_lookup_doc (test, "failPoint", &opts);
      activate_fail_point (collection->client, server_id, &opts);
   }

   op_name = bson_lookup_utf8 (test, "operation.name");

   if (!strcmp (op_name, "bulkWrite")) {
      bulk_write (collection, test, session);
   } else if (!strcmp (op_name, "deleteOne") ||
              !strcmp (op_name, "insertOne") ||
              !strcmp (op_name, "replaceOne") ||
              !strcmp (op_name, "updateOne")) {
      single_write (collection, test, session);
   } else if (!strcmp (op_name, "findOneAndDelete") ||
              !strcmp (op_name, "findOneAndReplace") ||
              !strcmp (op_name, "findOneAndUpdate")) {
      find_and_modify (collection, test, session);
   } else if (!strcmp (op_name, "insertMany")) {
      insert_many (collection, test, session);
   } else {
      test_error ("unrecognized operation name %s", op_name);
      abort ();
   }

   if (bson_has_field (test, "outcome.collection")) {
      check_outcome_collection (collection, test);
   }

done:
   if (server_id) {
      deactivate_fail_point (collection->client, server_id);
   }
}


static void
_test_retryable_writes_cb (bson_t *scenario, bool explicit_session)
{
   bson_iter_t scenario_iter;
   bson_iter_t tests_iter;

   ASSERT (scenario);

   ASSERT (bson_iter_init_find (&scenario_iter, scenario, "tests"));
   ASSERT (BSON_ITER_HOLDS_ARRAY (&scenario_iter));
   ASSERT (bson_iter_recurse (&scenario_iter, &tests_iter));

   while (bson_iter_next (&tests_iter)) {
      mongoc_uri_t *uri;
      mongoc_client_t *client;
      uint32_t server_id;
      mongoc_collection_t *collection;
      mongoc_client_session_t *session = NULL;
      bson_t test;
      bson_error_t error;

      ASSERT (BSON_ITER_HOLDS_DOCUMENT (&tests_iter));
      bson_iter_bson (&tests_iter, &test);

      uri = test_framework_get_uri ();
      mongoc_uri_set_option_as_bool (uri, "retryWrites", true);

      client = mongoc_client_new_from_uri (uri);
      test_framework_set_ssl_opts (client);
      mongoc_uri_destroy (uri);

      /* clean up in case a previous test aborted */
      server_id = mongoc_topology_select_server_id (
         client->topology, MONGOC_SS_WRITE, NULL, &error);
      ASSERT_OR_PRINT (server_id, error);
      deactivate_fail_point (client, server_id);

      collection = get_test_collection (client, "retryable_writes");

      if (explicit_session) {
         session = mongoc_client_start_session (client, NULL, &error);
         ASSERT_OR_PRINT (session, error);
      }

      insert_data (collection, scenario);
      execute_test (collection, &test, session);

      if (!mongoc_collection_drop (collection, &error)) {
         if (strcmp (error.message, "ns not found")) {
            /* an error besides ns not found */
            ASSERT_OR_PRINT (false, error);
         }
      }

      if (session) {
         mongoc_client_session_destroy (session);
      }

      mongoc_collection_destroy (collection);
      mongoc_client_destroy (client);
   }
}


/* Callback for JSON tests from Retryable Writes Spec */
static void
test_retryable_writes_cb (bson_t *scenario)
{
   _test_retryable_writes_cb (scenario, true /* explicit_session */);
   _test_retryable_writes_cb (scenario, false /* implicit_session */);
}


/* "Replica Set Failover Test" from Retryable Writes Spec */
static void
test_rs_failover (void)
{
   mock_rs_t *rs;
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_client_session_t *cs;
   bson_t opts = BSON_INITIALIZER;
   future_t *future;
   request_t *request;
   bson_error_t error;

   rs = mock_rs_with_autoismaster (WIRE_VERSION_OP_MSG,
                                   true /* has primary */,
                                   2 /* secondaries */,
                                   0 /* arbiters */);

   mock_rs_run (rs);
   uri = mongoc_uri_copy (mock_rs_get_uri (rs));
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);
   client = mongoc_client_new_from_uri (uri);
   collection = mongoc_client_get_collection (client, "db", "collection");
   cs = mongoc_client_start_session (client, NULL, &error);
   ASSERT_OR_PRINT (cs, error);
   ASSERT_OR_PRINT (mongoc_client_session_append (cs, &opts, &error), error);

   /* initial insert triggers replica set discovery */
   future = future_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);
   request =
      mock_rs_receives_msg (rs, 0, tmp_bson ("{'insert': 'collection'}"));
   mock_server_replies_ok_and_destroys (request);
   BSON_ASSERT (future_get_bool (future));
   future_destroy (future);

   /* failover */
   mock_rs_stepdown (rs);
   mock_rs_elect (rs, 1 /* server id */);

   /* insert receives "not master" from old primary, reselects and retries */
   future = future_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);

   request =
      mock_rs_receives_msg (rs, 0, tmp_bson ("{'insert': 'collection'}"));
   BSON_ASSERT (mock_rs_request_is_to_secondary (rs, request));
   mock_server_replies_simple (request, "{'ok': 0, 'errmsg': 'not master'}");
   request_destroy (request);

   request =
      mock_rs_receives_msg (rs, 0, tmp_bson ("{'insert': 'collection'}"));
   BSON_ASSERT (mock_rs_request_is_to_primary (rs, request));
   mock_server_replies_ok_and_destroys (request);
   BSON_ASSERT (future_get_bool (future));
   future_destroy (future);

   bson_destroy (&opts);
   mongoc_client_session_destroy (cs);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   mock_rs_destroy (rs);
}


/* Test code paths for _mongoc_client_command_with_opts */
static void
test_command_with_opts (void *ctx)
{
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   uint32_t server_id;
   mongoc_collection_t *collection;
   bson_t *cmd;
   bson_t reply;
   bson_t reply_result;
   bson_error_t error;

   uri = test_framework_get_uri ();
   mongoc_uri_set_option_as_bool (uri, "retryWrites", true);

   client = mongoc_client_new_from_uri (uri);
   test_framework_set_ssl_opts (client);
   mongoc_uri_destroy (uri);

   /* clean up in case a previous test aborted */
   server_id = mongoc_topology_select_server_id (
      client->topology, MONGOC_SS_WRITE, NULL, &error);
   ASSERT_OR_PRINT (server_id, error);
   deactivate_fail_point (client, server_id);

   collection = get_test_collection (client, "retryable_writes");

   if (!mongoc_collection_drop (collection, &error)) {
      if (strcmp (error.message, "ns not found")) {
         /* an error besides ns not found */
         ASSERT_OR_PRINT (false, error);
      }
   }

   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (
         collection, tmp_bson ("{'_id':1, 'x': 1}"), NULL, NULL, &error),
      error);

   activate_fail_point (client,
                        server_id,
                        tmp_bson ("{'mode': {'times': 1}, 'data': "
                                  "{'failBeforeCommitExceptionCode': 1}}"));

   cmd = tmp_bson ("{'findAndModify': '%s', 'query': {'_id': 1}, 'update': "
                   "{'$inc': {'x': 1}}, 'new': true}",
                   collection->collection);

   ASSERT_OR_PRINT (mongoc_collection_read_write_command_with_opts (
                       collection, cmd, NULL, NULL, &reply, &error),
                    error);

   bson_lookup_doc (&reply, "value", &reply_result);
   ASSERT (match_bson (&reply_result, tmp_bson ("{'_id': 1, 'x': 2}"), false));

   deactivate_fail_point (client, server_id);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&reply);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


/*
 *-----------------------------------------------------------------------
 *
 * Runner for the JSON tests for retryable writes.
 *
 *-----------------------------------------------------------------------
 */
static void
test_all_spec_tests (TestSuite *suite)
{
   char resolved[PATH_MAX];

   ASSERT (realpath (JSON_DIR "/retryable_writes", resolved));
   install_json_test_suite_with_check (suite,
                                       resolved,
                                       test_retryable_writes_cb,
                                       test_framework_skip_if_no_crypto,
                                       test_framework_skip_if_not_rs_version_6);
}

void
test_retryable_writes_install (TestSuite *suite)
{
   test_all_spec_tests (suite);
   TestSuite_AddMockServerTest (suite,
                                "/retryable_writes/failover",
                                test_rs_failover,
                                test_framework_skip_if_no_crypto);
   TestSuite_AddFull (suite,
                      "/retryable_writes/command_with_opts",
                      test_command_with_opts,
                      NULL,
                      NULL,
                      test_framework_skip_if_not_rs_version_6);
}
