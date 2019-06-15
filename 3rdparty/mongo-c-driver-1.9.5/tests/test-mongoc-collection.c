#include <bcon.h>
#include <mongoc.h>
#include <mongoc-client-private.h>
#include <mongoc-cursor-private.h>
#include <mongoc-collection-private.h>
#include <mongoc-write-concern-private.h>
#include <mongoc-read-concern-private.h>
#include <mongoc-util-private.h>

#include "TestSuite.h"

#include "test-libmongoc.h"
#include "test-conveniences.h"
#include "mock_server/future-functions.h"
#include "mock_server/mock-server.h"
#include "mock_server/mock-rs.h"


BEGIN_IGNORE_DEPRECATIONS


static void
test_aggregate_w_write_concern (void)
{
   mongoc_cursor_t *cursor;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *good_wc;
   mongoc_write_concern_t *bad_wc;
   bson_t *pipeline;
   bson_t *opts = NULL;
   char *json;
   const bson_t *doc;
   bson_error_t error;

   /* set up */
   good_wc = mongoc_write_concern_new ();
   bad_wc = mongoc_write_concern_new ();
   opts = bson_new ();

   client = test_framework_client_new ();
   BSON_ASSERT (client);
   ASSERT (mongoc_client_set_error_api (client, 2));

   collection = mongoc_client_get_collection (client, "test", "test");

   /* pipeline that writes to collection */
   json = bson_strdup_printf ("[{'$out': '%s'}]", collection->collection);
   pipeline = tmp_bson (json);

   /* collection aggregate with valid writeConcern: no error */
   mongoc_write_concern_set_w (good_wc, 1);
   bson_reinit (opts);
   mongoc_write_concern_append (good_wc, opts);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, opts, NULL);
   ASSERT (cursor);
   mongoc_cursor_next (cursor, &doc);

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   /* writeConcern that will not pass mongoc_write_concern_is_valid */
   bad_wc->wtimeout = -10;
   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, opts, NULL);
   ASSERT (cursor);
   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT_ERROR_CONTAINS (cursor->error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid writeConcern");
   bad_wc->wtimeout = 0;

   mongoc_write_concern_destroy (good_wc);
   mongoc_write_concern_destroy (bad_wc);
   mongoc_collection_destroy (collection);
   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
   bson_destroy (opts);
   bson_free (json);
}

static void
test_aggregate_inherit_collection (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   mongoc_collection_t *collection;
   const bson_t *doc;
   request_t *request;
   future_t *future;
   bson_t *pipeline;
   bson_t opts = BSON_INITIALIZER;
   mongoc_read_concern_t *rc2;
   mongoc_read_concern_t *rc;
   mongoc_write_concern_t *wc2;
   mongoc_write_concern_t *wc;

   server = mock_server_with_autoismaster (WIRE_VERSION_MAX_STALENESS);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");


   pipeline = BCON_NEW (
      "pipeline", "[", "{", "$out", BCON_UTF8 ("collection2"), "}", "]");

   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   mongoc_read_concern_append (rc, &opts);

   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 2);
   mongoc_write_concern_append (wc, &opts);

   /* Uses the opts */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_SLAVE_OK, pipeline, &opts, NULL);
   future = future_cursor_next (cursor, &doc);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      " { 'aggregate' : 'collection',"
      "   'pipeline' : [ { '$out' : 'collection2' } ],"
      "   'cursor' : {  },"
      "   'readConcern' : { 'level' : 'majority' },"
      "   'writeConcern' : { 'w' : 2 } }");

   mock_server_replies_simple (request, "{'ok': 1}");

   ASSERT (!future_get_bool (future));

   /* Set collection level defaults */
   wc2 = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc2, 3);
   mongoc_collection_set_write_concern (collection, wc2);
   rc2 = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc2, MONGOC_READ_CONCERN_LEVEL_LOCAL);
   mongoc_collection_set_read_concern (collection, rc2);


   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);

   /* Inherits from collection */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_SLAVE_OK, pipeline, NULL, NULL);
   future = future_cursor_next (cursor, &doc);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      " { 'aggregate' : 'collection',"
      "   'pipeline' : [ { '$out' : 'collection2' } ],"
      "   'cursor' : {  },"
      "   'readConcern' : { 'level' : 'local' },"
      "   'writeConcern' : { 'w' : 3 } }");

   mock_server_replies_simple (request, "{'ok': 1}");

   ASSERT (!future_get_bool (future));

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);

   /* Uses the opts, not default collection level */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_SLAVE_OK, pipeline, &opts, NULL);
   future = future_cursor_next (cursor, &doc);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      " { 'aggregate' : 'collection',"
      "   'pipeline' : [ { '$out' : 'collection2' } ],"
      "   'cursor' : {  },"
      "   'readConcern' : { 'level' : 'majority' },"
      "   'writeConcern' : { 'w' : 2 } }");

   mock_server_replies_simple (request, "{'ok': 1}");

   ASSERT (!future_get_bool (future));

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);

   /* Doesn't inherit write concern when not using $out  */
   bson_destroy (pipeline);
   pipeline = BCON_NEW (
      "pipeline", "[", "{", "$in", BCON_UTF8 ("collection2"), "}", "]");

   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_SLAVE_OK, pipeline, NULL, NULL);
   future = future_cursor_next (cursor, &doc);

   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      " { 'aggregate' : 'collection',"
      "   'pipeline' : [ { '$in' : 'collection2' } ],"
      "   'cursor' : {  },"
      "   'readConcern' : { 'level' : 'local' },"
      "   'writeConcern' : { '$exists' : false } }");

   mock_server_replies_simple (request, "{'ok': 1}");
   ASSERT (!future_get_bool (future));

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);

   bson_destroy (pipeline);
   mongoc_read_concern_destroy (rc);
   mongoc_read_concern_destroy (rc2);
   mongoc_write_concern_destroy (wc);
   mongoc_write_concern_destroy (wc2);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

static void
test_read_prefs_is_valid (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   bson_t *pipeline;
   mongoc_read_prefs_t *read_prefs;
   bson_t reply;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_aggregate");
   ASSERT (collection);

   pipeline = BCON_NEW ("pipeline",
                        "[",
                        "{",
                        "$match",
                        "{",
                        "hello",
                        BCON_UTF8 ("world"),
                        "}",
                        "}",
                        "]");

   /* if read prefs is not valid */
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_PRIMARY);
   ASSERT (read_prefs);
   mongoc_read_prefs_set_tags (read_prefs,
                               tmp_bson ("[{'does-not-exist': 'x'}]"));

   /* mongoc_collection_aggregate */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, NULL, read_prefs);
   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   mongoc_cursor_destroy (cursor);

   /* mongoc_collection_command */
   cursor = mongoc_collection_command (collection,
                                       MONGOC_QUERY_NONE,
                                       0,
                                       0,
                                       0,
                                       tmp_bson ("{}"),
                                       NULL,
                                       read_prefs);
   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   mongoc_cursor_destroy (cursor);

   /* mongoc_collection_command_simple */
   ASSERT (!mongoc_collection_command_simple (
      collection, tmp_bson ("{'ping': 1}"), read_prefs, &reply, &error));

   /* mongoc_collection_count_with_opts */
   ASSERT (mongoc_collection_count_with_opts (collection,
                                              MONGOC_QUERY_NONE,
                                              tmp_bson ("{}"),
                                              0,
                                              0,
                                              NULL,
                                              read_prefs,
                                              &error) == -1);

   /* mongoc_collection_find */
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    0,
                                    0,
                                    tmp_bson ("{}"),
                                    NULL,
                                    read_prefs);

   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   mongoc_cursor_destroy (cursor);

   /* mongoc_collection_find_with_opts */
   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{}"), NULL, read_prefs);

   ASSERT (cursor);
   ASSERT (mongoc_cursor_error (cursor, &error));
   mongoc_cursor_destroy (cursor);

   /* if read prefs is valid */
   mongoc_read_prefs_destroy (read_prefs);
   read_prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   ASSERT (read_prefs);

   /* mongoc_collection_aggregate */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, NULL, read_prefs);
   ASSERT (cursor);

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   /* mongoc_collection_command */
   cursor = mongoc_collection_command (collection,
                                       MONGOC_QUERY_NONE,
                                       0,
                                       0,
                                       0,
                                       tmp_bson ("{}"),
                                       NULL,
                                       read_prefs);
   ASSERT (cursor);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   /* mongoc_collection_command_simple */
   ASSERT_OR_PRINT (
      mongoc_collection_command_simple (
         collection, tmp_bson ("{'ping': 1}"), read_prefs, &reply, &error),
      error);
   /* mongoc_collection_count_with_opts */
   ASSERT_OR_PRINT (mongoc_collection_count_with_opts (collection,
                                                       MONGOC_QUERY_NONE,
                                                       tmp_bson ("{}"),
                                                       0,
                                                       0,
                                                       NULL,
                                                       read_prefs,
                                                       &error) != -1,
                    error);

   /* mongoc_collection_find */
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0,
                                    0,
                                    0,
                                    tmp_bson ("{}"),
                                    NULL,
                                    read_prefs);

   ASSERT (cursor);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   /* mongoc_collection_find_with_opts */
   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{}"), NULL, read_prefs);

   ASSERT (cursor);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   mongoc_read_prefs_destroy (read_prefs);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   bson_destroy (pipeline);
   bson_destroy (&reply);
}

static void
test_copy (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_collection_t *copy;
   mongoc_client_t *client;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_insert");
   ASSERT (collection);

   copy = mongoc_collection_copy (collection);
   ASSERT (copy);
   ASSERT (copy->client == collection->client);
   ASSERT (strcmp (copy->ns, collection->ns) == 0);

   mongoc_collection_destroy (copy);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}


static void
test_insert (void)
{
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   unsigned i;
   bson_t b;


   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_insert");
   ASSERT (collection);

   mongoc_collection_drop (collection, &error);

   context = bson_context_new (BSON_CONTEXT_NONE);
   ASSERT (context);

   for (i = 0; i < 10; i++) {
      bson_init (&b);
      bson_oid_init (&oid, context);
      bson_append_oid (&b, "_id", 3, &oid);
      bson_append_utf8 (&b, "hello", 5, "/world", 5);
      ASSERT_OR_PRINT (
         mongoc_collection_insert_one (collection, &b, NULL, NULL, &error),
         error);

      bson_destroy (&b);
   }

   r = mongoc_collection_insert_one (
      collection, tmp_bson ("{'$hello': 1}"), NULL, NULL, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "invalid document");

   r = mongoc_collection_insert_one (
      collection, tmp_bson ("{'a.b': 1}"), NULL, NULL, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "invalid document");

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_context_destroy (context);
   mongoc_client_destroy (client);
}


static void
test_insert_null (void)
{
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   bson_t reply;
   const bson_t *out;
   bool ret;
   bson_t doc;
   bson_t filter = BSON_INITIALIZER;
   bson_iter_t iter;
   uint32_t len;

   client = test_framework_client_new ();
   ASSERT (client);

   collection =
      mongoc_client_get_collection (client, "test", "test_null_insert");
   ASSERT (collection);

   mongoc_collection_drop (collection, &error);

   bson_init (&doc);
   bson_append_utf8 (&doc, "hello", 5, "wor\0ld", 6);
   ret = mongoc_collection_insert_one (collection, &doc, NULL, NULL, &error);
   ASSERT_OR_PRINT (ret, error);

   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, NULL);
   mongoc_bulk_operation_insert (bulk, &doc);
   ret = mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_OR_PRINT (ret, error);
   ASSERT_MATCH (&reply,
                 "{'nInserted': 1,"
                 " 'nMatched':  0,"
                 " 'nModified': 0,"
                 " 'nRemoved':  0,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': []}");
   bson_destroy (&doc);
   bson_destroy (&reply);

   cursor = mongoc_collection_find_with_opts (collection, &filter, NULL, NULL);
   ASSERT (mongoc_cursor_next (cursor, &out));
   ASSERT (bson_iter_init_find (&iter, out, "hello"));
   ASSERT (!memcmp (bson_iter_utf8 (&iter, &len), "wor\0ld", 6));
   ASSERT_CMPINT (len, ==, 6);

   ASSERT (mongoc_cursor_next (cursor, &out));
   ASSERT (bson_iter_init_find (&iter, out, "hello"));
   ASSERT (!memcmp (bson_iter_utf8 (&iter, &len), "wor\0ld", 6));
   ASSERT_CMPINT (len, ==, 6);

   mongoc_cursor_destroy (cursor);
   mongoc_bulk_operation_destroy (bulk);

   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, NULL);
   mongoc_bulk_operation_remove_one (bulk, &doc);
   ret = mongoc_bulk_operation_update_one_with_opts (
      bulk, &doc, tmp_bson ("{'$set': {'x': 1}}"), NULL, &error);
   ASSERT_OR_PRINT (ret, error);
   ret = mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_OR_PRINT (ret, error);

   ASSERT_MATCH (&reply,
                 "{'nInserted': 0,"
                 " 'nMatched':  1,"
                 " 'nModified': 1,"
                 " 'nRemoved':  1,"
                 " 'nUpserted': 0,"
                 " 'writeErrors': []}");

   bson_destroy (&filter);
   bson_destroy (&reply);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_insert_oversize (void *ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t doc = BSON_INITIALIZER;
   bool r;
   bson_error_t error;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_insert_oversize");

   /* two huge strings make the doc too large */
   BSON_ASSERT (bson_append_utf8 (
      &doc, "x", 1, huge_string (client), (int) huge_string_length (client)));

   BSON_ASSERT (bson_append_utf8 (
      &doc, "y", 1, huge_string (client), (int) huge_string_length (client)));


   r = mongoc_collection_insert_one (collection, &doc, NULL, NULL, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, "too large");

   bson_destroy (&doc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_insert_many (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   unsigned i;
   bson_t q;
   bson_t b[10];
   bson_t *bptr[10];
   bson_t reply;
   int64_t count;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_insert_many");
   ASSERT (collection);

   mongoc_collection_drop (collection, &error);

   context = bson_context_new (BSON_CONTEXT_NONE);
   ASSERT (context);

   bson_init (&q);
   bson_append_int32 (&q, "n", -1, 0);

   for (i = 0; i < 10; i++) {
      bson_init (&b[i]);
      bson_oid_init (&oid, context);
      bson_append_oid (&b[i], "_id", -1, &oid);
      bson_append_int32 (&b[i], "n", -1, i % 2);
      bptr[i] = &b[i];
   }

   ASSERT_OR_PRINT (
      mongoc_collection_insert_many (
         collection, (const bson_t **) bptr, 10, NULL, &reply, &error),
      error);

   ASSERT_CMPINT32 (bson_lookup_int32 (&reply, "insertedCount"), ==, 10);
   bson_destroy (&reply);
   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &q, 0, 0, NULL, &error);
   ASSERT (count == 5);

   for (i = 8; i < 10; i++) {
      bson_destroy (&b[i]);
      bson_init (&b[i]);
      bson_oid_init (&oid, context);
      bson_append_oid (&b[i], "_id", -1, &oid);
      bson_append_int32 (&b[i], "n", -1, i % 2);
      bptr[i] = &b[i];
   }

   r = mongoc_collection_insert_many (
      collection, (const bson_t **) bptr, 10, NULL, &reply, &error);

   ASSERT (!r);
   ASSERT (error.code == 11000);
   ASSERT_CMPINT32 (bson_lookup_int32 (&reply, "insertedCount"), ==, 0);
   bson_destroy (&reply);

   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &q, 0, 0, NULL, &error);
   ASSERT (count == 5);

   r = mongoc_collection_insert_many (collection,
                                      (const bson_t **) bptr,
                                      10,
                                      tmp_bson ("{'ordered': false}"),
                                      &reply,
                                      &error);
   ASSERT (!r);
   ASSERT (error.code == 11000);
   ASSERT_CMPINT32 (bson_lookup_int32 (&reply, "insertedCount"), ==, 2);
   bson_destroy (&reply);

   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &q, 0, 0, NULL, &error);
   ASSERT (count == 6);

   /* test validate */
   for (i = 0; i < 10; i++) {
      bson_destroy (&b[i]);
      bson_init (&b[i]);
      BSON_APPEND_INT32 (&b[i], "$invalid_dollar_prefixed_name", i);
      bptr[i] = &b[i];
   }
   r = mongoc_collection_insert_many (collection,
                                      (const bson_t **) bptr,
                                      10,
                                      tmp_bson ("{'ordered': false}"),
                                      NULL,
                                      &error);
   ASSERT (!r);
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);
   ASSERT (error.code == MONGOC_ERROR_COMMAND_INVALID_ARG);

   bson_destroy (&q);
   for (i = 0; i < 10; i++) {
      bson_destroy (&b[i]);
   }

   for (i = 0; i < 10; i++) {
      bson_destroy (&b[i]);
      bson_init (&b[i]);
      BSON_APPEND_INT32 (&b[i], "a.b", i);
      bptr[i] = &b[i];
   }

   r = mongoc_collection_insert_many (
      collection, (const bson_t **) bptr, 10, NULL, NULL, &error);
   ASSERT (!r);
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);
   ASSERT (error.code == MONGOC_ERROR_COMMAND_INVALID_ARG);

   bson_destroy (&q);
   for (i = 0; i < 10; i++) {
      bson_destroy (&b[i]);
   }

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_context_destroy (context);
   mongoc_client_destroy (client);
}


static void
test_insert_bulk_empty (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_error_t error;
   bson_t *bptr = NULL;

   client = test_framework_client_new ();
   database = get_test_database (client);
   collection = get_test_collection (client, "test_insert_bulk_empty");

   BEGIN_IGNORE_DEPRECATIONS;
   ASSERT (!mongoc_collection_insert_bulk (collection,
                                           MONGOC_INSERT_NONE,
                                           (const bson_t **) &bptr,
                                           0,
                                           NULL,
                                           &error));
   END_IGNORE_DEPRECATIONS;

   ASSERT_CMPINT (MONGOC_ERROR_COLLECTION, ==, error.domain);
   ASSERT_CMPINT (MONGOC_ERROR_COLLECTION_INSERT_FAILED, ==, error.code);
   ASSERT_CONTAINS (error.message, "empty insert");

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}


static void
auto_ismaster (mock_server_t *server,
               int32_t max_wire_version,
               int32_t max_message_size,
               int32_t max_bson_size,
               int32_t max_batch_size)
{
   char *response = bson_strdup_printf ("{'ismaster': true, "
                                        " 'maxWireVersion': %d,"
                                        " 'maxBsonObjectSize': %d,"
                                        " 'maxMessageSizeBytes': %d,"
                                        " 'maxWriteBatchSize': %d }",
                                        max_wire_version,
                                        max_bson_size,
                                        max_message_size,
                                        max_batch_size);

   BSON_ASSERT (max_wire_version > 0);
   mock_server_auto_ismaster (server, response);

   bson_free (response);
}


char *
make_string (size_t len)
{
   char *s = (char *) bson_malloc (len);

   memset (s, 'a', len - 1);
   s[len - 1] = '\0';

   return s;
}


bson_t *
make_document (size_t bytes)
{
   bson_t *bson;
   bson_oid_t oid;
   char *s;
   size_t string_len;

   bson_oid_init (&oid, NULL);
   bson = bson_new ();
   BSON_APPEND_OID (bson, "_id", &oid);

   /* make the document exactly n bytes by appending a string. a string has
    * 7 bytes overhead (1 for type code, 2 for key, 4 for length prefix), so
    * make the string (n_bytes - current_length - 7) bytes long. */
   ASSERT_CMPUINT ((unsigned int) bytes, >=, bson->len + 7);
   string_len = bytes - bson->len - 7;
   s = make_string (string_len);
   BSON_APPEND_UTF8 (bson, "s", s);
   bson_free (s);
   ASSERT_CMPUINT ((unsigned int) bytes, ==, bson->len);

   return bson;
}


void
make_bulk_insert (bson_t **bsons, int n, size_t bytes)
{
   int i;

   for (i = 0; i < n; i++) {
      bsons[i] = make_document (bytes);
   }
}


static void
destroy_all (bson_t **ptr, int n)
{
   int i;

   for (i = 0; i < n; i++) {
      bson_destroy (ptr[i]);
   }
}


/* verify an insert command's "documents" array has keys "0", "1", "2", ... */
static void
verify_keys (uint32_t n_documents, const bson_t *insert_command)
{
   bson_iter_t iter;
   uint32_t len;
   const uint8_t *data;
   bson_t document;
   char str[16];
   const char *key;
   uint32_t i;

   ASSERT (bson_iter_init_find (&iter, insert_command, "documents"));
   bson_iter_array (&iter, &len, &data);
   ASSERT (bson_init_static (&document, data, len));

   for (i = 0; i < n_documents; i++) {
      bson_uint32_to_string (i, &key, str, sizeof str);
      ASSERT (bson_has_field (&document, key));
   }
}


/* CDRIVER-845: "insert" command must have array keys "0", "1", "2", ... */
static void
test_insert_command_keys (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   uint32_t i;
   bson_t *doc;
   bson_t reply;
   bson_error_t error;
   future_t *future;
   request_t *request;

   /* maxWireVersion 3 allows write commands */
   server = mock_server_with_autoismaster (3);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "test", "test");
   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, NULL);

   for (i = 0; i < 3; i++) {
      doc = BCON_NEW ("_id", BCON_INT32 (i));
      mongoc_bulk_operation_insert (bulk, doc);
      bson_destroy (doc);
   }

   future = future_bulk_operation_execute (bulk, &reply, &error);
   request = mock_server_receives_command (
      server, "test", MONGOC_QUERY_NONE, "{'insert': 'test'}");

   verify_keys (3, request_get_doc (request, 0));
   mock_server_replies_simple (request, "{'ok': 1}");

   ASSERT_OR_PRINT (future_get_uint32_t (future), error);

   bson_destroy (&reply);
   future_destroy (future);
   request_destroy (request);
   mongoc_bulk_operation_destroy (bulk);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


/* number of docs that should go in a batch that starts at "offset" */
int
expected_batch_size (const bson_t **bsons,
                     int n_bsons,
                     int max_message_size,
                     int max_bson_size,
                     bool continue_on_err,
                     int *offset,         /* IN / OUT */
                     bool *has_oversized) /* OUT */
{
   int batch_sz = 0;
   int msg_sz = 0;
   bool oversized;
   int n_oversized = 0;

   for (; *offset < n_bsons; (*offset)++) {
      oversized = (bsons[*offset]->len > max_bson_size);

      if (oversized) {
         n_oversized++;

         if (!continue_on_err) {
            /* stop here */
            return batch_sz;
         }
      } else {
         /* not oversized, regular document */
         msg_sz += bsons[*offset]->len;

         if (msg_sz >= max_message_size) {
            /* batch is full of regular documents */
            break;
         }

         batch_sz++;
      }
   }

   *has_oversized = (bool) n_oversized;

   return batch_sz;
}


static void
test_save (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bson_oid_t oid;
   unsigned i;
   bson_t b;
   bool r;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_save");
   ASSERT (collection);

   mongoc_collection_drop (collection, &error);

   context = bson_context_new (BSON_CONTEXT_NONE);
   ASSERT (context);

   BEGIN_IGNORE_DEPRECATIONS;

   for (i = 0; i < 10; i++) {
      bson_init (&b);
      bson_oid_init (&oid, context);
      bson_append_oid (&b, "_id", 3, &oid);
      bson_append_utf8 (&b, "hello", 5, "/world", 5);
      ASSERT_OR_PRINT (mongoc_collection_save (collection, &b, NULL, &error),
                       error);
      bson_destroy (&b);
   }

   bson_destroy (&b);

   r = mongoc_collection_save (
      collection, tmp_bson ("{'$hello': 1}"), NULL, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "invalid document");

   r = mongoc_collection_save (
      collection, tmp_bson ("{'a.b': 1}"), NULL, &error);

   END_IGNORE_DEPRECATIONS;

   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "invalid document");

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_context_destroy (context);
   mongoc_client_destroy (client);
}


static void
test_regex (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_write_concern_t *wr;
   bson_t opts = BSON_INITIALIZER;
   mongoc_client_t *client;
   bson_error_t error;
   int64_t count;
   bson_t q = BSON_INITIALIZER;
   bson_t *doc;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_regex");
   ASSERT (collection);

   wr = mongoc_write_concern_new ();
   mongoc_write_concern_set_journal (wr, true);
   mongoc_write_concern_append (wr, &opts);

   doc = BCON_NEW ("hello", "/world");
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (collection, doc, &opts, NULL, &error),
      error);

   BSON_APPEND_REGEX (&q, "hello", "^/wo", "i");

   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &q, 0, 0, NULL, &error);

   ASSERT (count > 0);
   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&opts);
   mongoc_write_concern_destroy (wr);
   bson_destroy (&q);
   bson_destroy (doc);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}


static void
test_decimal128 (void *ctx)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_write_concern_t *wr;
   bson_t opts = BSON_INITIALIZER;
   mongoc_client_t *client;
   bson_error_t error = {0};
   int64_t count;
   bson_t query = BSON_INITIALIZER;
   bson_t *doc;
   const bson_t *dec;
   bson_iter_t dec_iter;
   mongoc_cursor_t *cursor;
   bool r;
   bson_decimal128_t decimal128;
   bson_decimal128_t read_decimal;

   bson_decimal128_from_string ("-123456789.101112E-120", &decimal128);
   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_decimal128");
   ASSERT (collection);

   wr = mongoc_write_concern_new ();
   mongoc_write_concern_set_journal (wr, true);
   mongoc_write_concern_append (wr, &opts);

   doc = BCON_NEW ("the_decimal", BCON_DECIMAL128 (&decimal128));
   r = mongoc_collection_insert_one (collection, doc, NULL, &opts, &error);
   if (!r) {
      MONGOC_WARNING ("test_decimal128: %s\n", error.message);
   }
   ASSERT (r);

   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &query, 0, 0, NULL, &error);
   ASSERT (count > 0);

   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, &query, NULL, NULL);
   ASSERT (mongoc_cursor_next (cursor, &dec));

   ASSERT (bson_iter_init (&dec_iter, dec));

   ASSERT (bson_iter_find (&dec_iter, "the_decimal"));
   ASSERT (BSON_ITER_HOLDS_DECIMAL128 (&dec_iter));
   bson_iter_decimal128 (&dec_iter, &read_decimal);

   ASSERT (read_decimal.high == decimal128.high &&
           read_decimal.low == decimal128.low);

   bson_destroy (doc);
   bson_destroy (&query);
   bson_destroy (&opts);
   mongoc_write_concern_destroy (wr);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}


static void
test_update (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   unsigned i;
   bson_t b;
   bson_t q;
   bson_t u;
   bson_t set;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_update");
   ASSERT (collection);

   context = bson_context_new (BSON_CONTEXT_NONE);
   ASSERT (context);

   for (i = 0; i < 10; i++) {
      bson_init (&b);
      bson_oid_init (&oid, context);
      bson_append_oid (&b, "_id", 3, &oid);
      bson_append_utf8 (&b, "utf8", 4, "utf8 string", 11);
      bson_append_int32 (&b, "int32", 5, 1234);
      bson_append_int64 (&b, "int64", 5, 12345678);
      bson_append_bool (&b, "bool", 4, 1);

      ASSERT_OR_PRINT (
         mongoc_collection_insert_one (collection, &b, NULL, NULL, &error),
         error);

      bson_init (&q);
      bson_append_oid (&q, "_id", 3, &oid);

      bson_init (&u);
      bson_append_document_begin (&u, "$set", 4, &set);
      bson_append_utf8 (&set, "utf8", 4, "updated", 7);
      bson_append_document_end (&u, &set);

      ASSERT_OR_PRINT (mongoc_collection_update (
                          collection, MONGOC_UPDATE_NONE, &q, &u, NULL, &error),
                       error);

      bson_destroy (&b);
      bson_destroy (&q);
      bson_destroy (&u);
   }

   bson_init (&q);
   bson_init (&u);
   BSON_APPEND_INT32 (&u, "abcd", 1);
   BSON_APPEND_INT32 (&u, "$hi", 1);
   r = mongoc_collection_update (
      collection, MONGOC_UPDATE_NONE, &q, &u, NULL, &error);
   ASSERT (!r);
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);
   ASSERT (error.code == MONGOC_ERROR_COMMAND_INVALID_ARG);
   bson_destroy (&q);
   bson_destroy (&u);

   bson_init (&q);
   bson_init (&u);
   BSON_APPEND_INT32 (&u, "a.b.c.d", 1);
   r = mongoc_collection_update (
      collection, MONGOC_UPDATE_NONE, &q, &u, NULL, &error);
   ASSERT (!r);
   ASSERT (error.domain == MONGOC_ERROR_COMMAND);
   ASSERT (error.code == MONGOC_ERROR_COMMAND_INVALID_ARG);
   bson_destroy (&q);
   bson_destroy (&u);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_context_destroy (context);
   mongoc_client_destroy (client);
}


static void
test_update_oversize (void *ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   size_t huger_sz = 20 * 1024 * 1024;
   char *huger;
   bson_t huge = BSON_INITIALIZER;
   bson_t empty = BSON_INITIALIZER;
   bson_t huge_update = BSON_INITIALIZER;
   bson_t child;
   bool r;
   bson_error_t error;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_update_oversize");

   /* first test oversized selector. two huge strings make the doc too large */
   BSON_ASSERT (bson_append_utf8 (
      &huge, "x", 1, huge_string (client), (int) huge_string_length (client)));

   BSON_ASSERT (bson_append_utf8 (
      &huge, "y", 1, huge_string (client), (int) huge_string_length (client)));

   r = mongoc_collection_update (
      collection, MONGOC_UPDATE_NONE, &huge, &empty, NULL, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, "too large");

   /* test oversized update operator */
   huger = bson_malloc (huger_sz + 1);
   memset (huger, 'a', huger_sz);
   huger[huger_sz] = '\0';
   BSON_ASSERT (BSON_APPEND_DOCUMENT_BEGIN (&huge_update, "$set", &child));
   BSON_ASSERT (bson_append_utf8 (&child, "x", 1, huger, (int) huger_sz));
   BSON_ASSERT (bson_append_document_end (&huge_update, &child));

   r = mongoc_collection_update (
      collection, MONGOC_UPDATE_NONE, &empty, &huge_update, NULL, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, "too large");

   bson_free (huger);
   bson_destroy (&huge);
   bson_destroy (&empty);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_remove (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   bson_context_t *context;
   bson_error_t error;
   bool r;
   bson_oid_t oid;
   bson_t b;
   int i;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_remove");
   ASSERT (collection);

   context = bson_context_new (BSON_CONTEXT_NONE);
   ASSERT (context);

   for (i = 0; i < 100; i++) {
      bson_init (&b);
      bson_oid_init (&oid, context);
      bson_append_oid (&b, "_id", 3, &oid);
      bson_append_utf8 (&b, "hello", 5, "world", 5);
      r = mongoc_collection_insert_one (collection, &b, NULL, NULL, &error);
      if (!r) {
         MONGOC_WARNING ("%s\n", error.message);
      }
      ASSERT (r);
      bson_destroy (&b);

      bson_init (&b);
      bson_append_oid (&b, "_id", 3, &oid);
      r = mongoc_collection_delete_many (collection, &b, NULL, NULL, &error);
      if (!r) {
         MONGOC_WARNING ("%s\n", error.message);
      }
      ASSERT (r);
      bson_destroy (&b);
   }

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   bson_context_destroy (context);
   mongoc_client_destroy (client);
}


static void
test_remove_oversize (void *ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t doc = BSON_INITIALIZER;
   bool r;
   bson_error_t error;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_remove_oversize");

   /* two huge strings make the doc too large */
   BSON_ASSERT (bson_append_utf8 (
      &doc, "x", 1, huge_string (client), (int) huge_string_length (client)));

   BSON_ASSERT (bson_append_utf8 (
      &doc, "y", 1, huge_string (client), (int) huge_string_length (client)));

   r = mongoc_collection_delete_many (collection, &doc, NULL, NULL, &error);
   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_BSON, MONGOC_ERROR_BSON_INVALID, "too large");

   bson_destroy (&doc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_insert_w0 (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *wc;
   bson_t opts = BSON_INITIALIZER;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_insert_w0");
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_write_concern_append (wc, &opts);
   r = mongoc_collection_insert_one (
      collection, tmp_bson ("{}"), &opts, NULL, &error);
   ASSERT_OR_PRINT (r, error);
   ASSERT (mongoc_collection_get_last_error (collection) == NULL);

   bson_destroy (&opts);
   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_update_w0 (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *wc;
   bson_error_t error;

   bool r;
   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_update_w0");
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   r = mongoc_collection_update (collection,
                                 MONGOC_UPDATE_NONE,
                                 tmp_bson ("{}"),
                                 tmp_bson ("{'$set': {'x': 1}}"),
                                 wc,
                                 &error);
   ASSERT_OR_PRINT (r, error);
   ASSERT (bson_empty (mongoc_collection_get_last_error (collection)));

   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_remove_w0 (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *wc;
   bson_t opts = BSON_INITIALIZER;
   bson_t reply;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_remove_w0");
   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, 0);
   mongoc_write_concern_append (wc, &opts);
   r = mongoc_collection_delete_many (
      collection, tmp_bson ("{}"), &opts, &reply, &error);
   ASSERT_OR_PRINT (r, error);
   ASSERT (bson_empty (&reply));

   bson_destroy (&reply);
   bson_destroy (&opts);
   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_index (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_index_opt_t opt;
   bson_error_t error;
   bson_t keys;
   bson_t *opts = NULL;
   mongoc_write_concern_t *bad_wc;
   mongoc_write_concern_t *good_wc;
   bool wire_version_5;
   bool r;

   mongoc_index_opt_init (&opt);
   opts = bson_new ();

   client = test_framework_client_new ();
   ASSERT (client);
   mongoc_client_set_error_api (client, 2);

   bad_wc = mongoc_write_concern_new ();
   good_wc = mongoc_write_concern_new ();

   wire_version_5 = test_framework_max_wire_version_at_least (5);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_index");
   ASSERT (collection);

   bson_init (&keys);
   bson_append_int32 (&keys, "hello", -1, 1);
   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &keys, &opt, &error), error);

   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &keys, &opt, &error), error);

   ASSERT_OR_PRINT (
      mongoc_collection_drop_index (collection, "hello_1", &error), error);

   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &keys, &opt, &error), error);

   /* invalid writeConcern */
   bad_wc->wtimeout = -10;
   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   ASSERT (!mongoc_collection_drop_index_with_opts (
      collection, "hello_1", opts, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid writeConcern");
   bad_wc->wtimeout = 0;
   error.code = 0;
   error.domain = 0;

   /* valid writeConcern on all configs*/
   mongoc_write_concern_set_w (good_wc, 1);
   bson_reinit (opts);
   mongoc_write_concern_append (good_wc, opts);
   ASSERT_OR_PRINT (mongoc_collection_drop_index_with_opts (
                       collection, "hello_1", opts, &error),
                    error);
   ASSERT (!error.code);
   ASSERT (!error.domain);

   /* writeConcern that results in writeConcernError */
   mongoc_write_concern_set_w (bad_wc, 99);

   if (!test_framework_is_mongos ()) { /* skip if sharded */
      ASSERT_OR_PRINT (
         mongoc_collection_create_index (collection, &keys, &opt, &error),
         error);
      bson_reinit (opts);
      mongoc_write_concern_append_bad (bad_wc, opts);
      r = mongoc_collection_drop_index_with_opts (
         collection, "hello_1", opts, &error);
      if (wire_version_5) {
         ASSERT (!r);

         if (test_framework_is_replset ()) { /* replica set */
            ASSERT_ERROR_CONTAINS (
               error, MONGOC_ERROR_WRITE_CONCERN, 100, "Write Concern error:");
         } else { /* standalone */
            ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER);
            ASSERT_CMPINT (error.code, ==, 2);
         }
      } else { /* if version <= 4, no error */
         ASSERT_OR_PRINT (r, error);
         ASSERT (!error.code);
         ASSERT (!error.domain);
      }
   }

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   bson_destroy (&keys);
   bson_destroy (opts);
   mongoc_write_concern_destroy (bad_wc);
   mongoc_write_concern_destroy (good_wc);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_index_w_write_concern ()
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_index_opt_t opt;
   mongoc_write_concern_t *good_wc;
   mongoc_write_concern_t *bad_wc;
   bson_error_t error;
   bson_t keys;
   bson_t reply;
   bson_t *opts = NULL;
   bool result;
   bool wire_version_5;
   bool at_least_2 = test_framework_max_wire_version_at_least (2);
   bool is_replicaset = test_framework_is_replset ();
   bool is_mongos = test_framework_is_mongos ();

   mongoc_index_opt_init (&opt);
   opts = bson_new ();

   client = test_framework_client_new ();
   ASSERT (client);

   good_wc = mongoc_write_concern_new ();
   bad_wc = mongoc_write_concern_new ();

   mongoc_client_set_error_api (client, 2);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_index");
   ASSERT (collection);

   wire_version_5 = test_framework_max_wire_version_at_least (5);

   bson_init (&keys);
   bson_append_int32 (&keys, "hello", -1, 1);

   /* writeConcern that will not pass validation */
   bad_wc->wtimeout = -10;
   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   ASSERT (!mongoc_collection_create_index_with_opts (
      collection, &keys, &opt, opts, &reply, &error));

   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid writeConcern");
   bad_wc->wtimeout = 0;
   error.code = 0;
   error.domain = 0;

   /* valid writeConcern on all server configs */
   mongoc_write_concern_set_w (good_wc, 1);
   bson_reinit (opts);
   mongoc_write_concern_append (good_wc, opts);
   result = mongoc_collection_create_index_with_opts (
      collection, &keys, &opt, opts, &reply, &error);
   ASSERT_OR_PRINT (result, error);
   ASSERT (!error.code);

   /* Be sure the reply is valid */
   ASSERT (bson_validate (&reply, 0, NULL));
   result = mongoc_collection_drop_index (collection, "hello_1", &error);
   ASSERT_OR_PRINT (result, error);
   if (at_least_2) {
      ASSERT (!bson_empty (&reply));
   } else {
      /* On very old versions of the server, create_index_with_write_concern
       * will give an empty reply even if the call succeeds */
      ASSERT (bson_empty (&reply));
   }
   bson_destroy (&reply);

   /* writeConcern that will result in writeConcernError */
   mongoc_write_concern_set_w (bad_wc, 99);

   ASSERT (!error.code);

   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   /* skip this part of the test if sharded cluster */
   if (!is_mongos) {
      if (wire_version_5) {
         ASSERT (!mongoc_collection_create_index_with_opts (
            collection, &keys, &opt, opts, &reply, &error));
         if (is_replicaset) { /* replica set */
            ASSERT_ERROR_CONTAINS (
               error, MONGOC_ERROR_WRITE_CONCERN, 100, "Write Concern error:");
         } else { /* standalone */
            ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER);
            ASSERT_CMPINT (error.code, ==, 2);
         }
      } else { /* if wire version <= 4, no error */
         result = mongoc_collection_create_index_with_opts (
            collection, &keys, &opt, opts, &reply, &error);
         ASSERT_OR_PRINT (result, error);
         ASSERT (!error.code);
         ASSERT (!error.domain);
      }

      if (at_least_2) {
         ASSERT (!bson_empty (&reply));
      } else {
         ASSERT (bson_empty (&reply));
      }
      bson_destroy (&reply);
   }

   /* Make sure it doesn't crash with a NULL reply or writeConcern */
   result = mongoc_collection_create_index_with_opts (
      collection, &keys, &opt, NULL, NULL, &error);
   ASSERT_OR_PRINT (result, error);

   ASSERT_OR_PRINT (
      mongoc_collection_drop_index (collection, "hello_1", &error), error);

   /* Now attempt to create an invalid index which the server will reject */
   bson_reinit (&keys);

   /* Try to create an index like {abc: "hallo thar"} (won't work,
      should really be something like {abc: 1})

      This fails both on legacy and modern versions of the server
   */
   BSON_APPEND_UTF8 (&keys, "abc", "hallo thar");
   result = mongoc_collection_create_index_with_opts (
      collection, &keys, &opt, NULL, &reply, &error);

   ASSERT (!result);
   ASSERT (strlen (error.message) > 0);
   memset (&error, 0, sizeof (error));

   /* Try again but with reply NULL. Shouldn't crash */
   result = mongoc_collection_create_index_with_opts (
      collection, &keys, &opt, NULL, NULL, &error);
   ASSERT (!result);
   ASSERT (strlen (error.message) > 0);

   bson_destroy (&keys);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   mongoc_write_concern_destroy (bad_wc);
   mongoc_write_concern_destroy (good_wc);
   bson_destroy (&reply);
   bson_destroy (opts);
}

static void
test_index_compound (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_index_opt_t opt;
   bson_error_t error;
   bson_t keys;

   mongoc_index_opt_init (&opt);

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_index_compound");
   ASSERT (collection);

   bson_init (&keys);
   bson_append_int32 (&keys, "hello", -1, 1);
   bson_append_int32 (&keys, "world", -1, -1);
   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &keys, &opt, &error), error);

   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &keys, &opt, &error), error);

   ASSERT_OR_PRINT (
      mongoc_collection_drop_index (collection, "hello_1_world_-1", &error),
      error);

   bson_destroy (&keys);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static void
test_index_geo (void)
{
   mongoc_server_description_t *description;
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_index_opt_t opt;
   mongoc_index_opt_geo_t geo_opt;
   bson_error_t error;
   bool r;
   bson_t keys;
   uint32_t id;

   mongoc_index_opt_init (&opt);
   mongoc_index_opt_geo_init (&geo_opt);

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_geo_index");
   ASSERT (collection);

   /* Create a basic 2d index */
   bson_init (&keys);
   BSON_APPEND_UTF8 (&keys, "location", "2d");
   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &keys, &opt, &error), error);

   ASSERT_OR_PRINT (
      mongoc_collection_drop_index (collection, "location_2d", &error), error);

   /* Create a 2d index with bells and whistles */
   bson_init (&keys);
   BSON_APPEND_UTF8 (&keys, "location", "2d");

   geo_opt.twod_location_min = -123;
   geo_opt.twod_location_max = +123;
   geo_opt.twod_bits_precision = 30;
   opt.geo_options = &geo_opt;

   /* TODO this hack is needed for single-threaded tests */
   id = client->topology->description.servers->items[0].id;
   description = mongoc_topology_server_by_id (client->topology, id, &error);
   ASSERT_OR_PRINT (description, error);

   if (description->max_wire_version > 0) {
      ASSERT_OR_PRINT (
         mongoc_collection_create_index (collection, &keys, &opt, &error),
         error);

      ASSERT_OR_PRINT (
         mongoc_collection_drop_index (collection, "location_2d", &error),
         error);
   }

   /* Create a Haystack index */
   bson_init (&keys);
   BSON_APPEND_UTF8 (&keys, "location", "geoHaystack");
   BSON_APPEND_INT32 (&keys, "category", 1);

   mongoc_index_opt_geo_init (&geo_opt);
   geo_opt.haystack_bucket_size = 5;

   opt.geo_options = &geo_opt;

   if (description->max_wire_version > 0) {
      ASSERT_OR_PRINT (
         mongoc_collection_create_index (collection, &keys, &opt, &error),
         error);

      r = mongoc_collection_drop_index (
         collection, "location_geoHaystack_category_1", &error);
      ASSERT_OR_PRINT (r, error);
   }

   mongoc_server_description_destroy (description);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}

static char *
storage_engine (mongoc_client_t *client)
{
   bson_iter_t iter;
   bson_error_t error;
   bson_t cmd = BSON_INITIALIZER;
   bson_t reply;

   /* NOTE: this default will change eventually */
   char *engine = bson_strdup ("mmapv1");

   BSON_APPEND_INT32 (&cmd, "getCmdLineOpts", 1);
   ASSERT_OR_PRINT (mongoc_client_command_simple (
                       client, "admin", &cmd, NULL, &reply, &error),
                    error);

   if (bson_iter_init_find (&iter, &reply, "parsed.storage.engine")) {
      engine = bson_strdup (bson_iter_utf8 (&iter, NULL));
   }

   bson_destroy (&reply);
   bson_destroy (&cmd);

   return engine;
}

static void
test_index_storage (void)
{
   mongoc_collection_t *collection = NULL;
   mongoc_database_t *database = NULL;
   mongoc_client_t *client = NULL;
   mongoc_index_opt_t opt;
   mongoc_index_opt_wt_t wt_opt;
   bson_error_t error;
   bson_t keys;
   char *engine = NULL;

   client = test_framework_client_new ();
   ASSERT (client);

   /* Skip unless we are on WT */
   engine = storage_engine (client);
   if (strcmp ("wiredTiger", engine) != 0) {
      goto cleanup;
   }

   mongoc_index_opt_init (&opt);
   mongoc_index_opt_wt_init (&wt_opt);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_storage_index");
   ASSERT (collection);

   /* Create a simple index */
   bson_init (&keys);
   bson_append_int32 (&keys, "hello", -1, 1);

   /* Add storage option to the index */
   wt_opt.base.type = MONGOC_INDEX_STORAGE_OPT_WIREDTIGER;
   wt_opt.config_str = "block_compressor=zlib";

   opt.storage_options = (mongoc_index_opt_storage_t *) &wt_opt;

   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &keys, &opt, &error), error);

cleanup:
   if (engine)
      bson_free (engine);
   if (collection)
      mongoc_collection_destroy (collection);
   if (database)
      mongoc_database_destroy (database);
   if (client)
      mongoc_client_destroy (client);
}

static void
test_count (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   int64_t count;
   bson_t b;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = mongoc_client_get_collection (client, "test", "test");
   ASSERT (collection);

   bson_init (&b);
   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);

   if (count == -1) {
      MONGOC_WARNING ("%s\n", error.message);
   }
   ASSERT (count != -1);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_count_read_pref (void)
{
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_read_prefs_t *prefs;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_mongos_new (WIRE_VERSION_MIN);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");
   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   mongoc_collection_set_read_prefs (collection, prefs);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, NULL, 0, 0, NULL, &error);
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'count': 'collection'},"
      " '$readPreference': {'mode': 'secondary'}}");

   mock_server_replies_simple (request, "{'ok': 1, 'n': 1}");
   ASSERT_OR_PRINT (1 == future_get_int64_t (future), error);

   request_destroy (request);
   future_destroy (future);
   mongoc_read_prefs_destroy (prefs);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_count_read_concern (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_read_concern_t *rc;
   mock_server_t *server;
   request_t *request;
   bson_error_t error;
   future_t *future;
   int64_t count;
   bson_t b;

   /* wire protocol version 4 */
   server = mock_server_with_autoismaster (WIRE_VERSION_READ_CONCERN);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   ASSERT (client);

   collection = mongoc_client_get_collection (client, "test", "test");
   ASSERT (collection);

   bson_init (&b);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   request =
      mock_server_receives_command (server,
                                    "test",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{ 'count' : 'test', 'query' : {  } }");

   mock_server_replies_simple (request, "{ 'n' : 42, 'ok' : 1 } ");
   count = future_get_int64_t (future);
   ASSERT_OR_PRINT (count == 42, error);
   request_destroy (request);
   future_destroy (future);

   /* readConcern: { level: majority } */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   request = mock_server_receives_command (server,
                                           "test",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{ 'count' : 'test', 'query' : {  "
                                           "}, 'readConcern': {'level': "
                                           "'majority'}}");

   mock_server_replies_simple (request, "{ 'n' : 43, 'ok' : 1 } ");
   count = future_get_int64_t (future);
   ASSERT_OR_PRINT (count == 43, error);
   mongoc_read_concern_destroy (rc);
   request_destroy (request);
   future_destroy (future);

   /* readConcern: { level: local } */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_LOCAL);
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   request = mock_server_receives_command (
      server,
      "test",
      MONGOC_QUERY_SLAVE_OK,
      "{ 'count' : 'test', 'query' : {  }, 'readConcern': {'level': 'local'}}");

   mock_server_replies_simple (request, "{ 'n' : 44, 'ok' : 1 } ");
   count = future_get_int64_t (future);
   ASSERT_OR_PRINT (count == 44, error);
   mongoc_read_concern_destroy (rc);
   request_destroy (request);
   future_destroy (future);

   /* readConcern: { level: futureCompatible } */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, "futureCompatible");
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   request = mock_server_receives_command (server,
                                           "test",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{ 'count' : 'test', 'query' : {  "
                                           "}, 'readConcern': {'level': "
                                           "'futureCompatible'}}");

   mock_server_replies_simple (request, "{ 'n' : 45, 'ok' : 1 } ");
   count = future_get_int64_t (future);
   ASSERT_OR_PRINT (count == 45, error);
   mongoc_read_concern_destroy (rc);
   request_destroy (request);
   future_destroy (future);

   /* Setting readConcern to NULL should not send readConcern */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, NULL);
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   request = mock_server_receives_command (server,
                                           "test",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{ 'count' : 'test', 'query' : {  "
                                           "}, 'readConcern': { '$exists': "
                                           "false }}");

   mock_server_replies_simple (request, "{ 'n' : 46, 'ok' : 1 } ");
   count = future_get_int64_t (future);
   ASSERT_OR_PRINT (count == 46, error);
   mongoc_read_concern_destroy (rc);
   request_destroy (request);
   future_destroy (future);

   /* Fresh read_concern should not send readConcern */
   rc = mongoc_read_concern_new ();
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   future = future_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   request = mock_server_receives_command (server,
                                           "test",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{ 'count' : 'test', 'query' : {  "
                                           "}, 'readConcern': { '$exists': "
                                           "false }}");

   mock_server_replies_simple (request, "{ 'n' : 47, 'ok' : 1 } ");
   count = future_get_int64_t (future);
   ASSERT_OR_PRINT (count == 47, error);

   mongoc_read_concern_destroy (rc);
   request_destroy (request);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
_test_count_read_concern_live (bool supports_read_concern)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_read_concern_t *rc;
   bson_error_t error;
   int64_t count;
   bson_t b;


   client = test_framework_client_new ();
   ASSERT (client);

   collection = mongoc_client_get_collection (client, "test", "test");
   ASSERT (collection);

   mongoc_collection_drop (collection, &error);

   bson_init (&b);
   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   ASSERT_OR_PRINT (count == 0, error);

   /* Setting readConcern to NULL should not send readConcern */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, NULL);
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   ASSERT_OR_PRINT (count == 0, error);
   mongoc_read_concern_destroy (rc);

   /* readConcern: { level: local } should raise error pre 3.2 */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_LOCAL);
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   if (supports_read_concern) {
      ASSERT_OR_PRINT (count == 0, error);
   } else {
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                             "The selected server does not support readConcern")
   }
   mongoc_read_concern_destroy (rc);

   /* readConcern: { level: majority } should raise error pre 3.2 */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   mongoc_collection_set_read_concern (collection, rc);

   bson_init (&b);
   count = mongoc_collection_count (
      collection, MONGOC_QUERY_NONE, &b, 0, 0, NULL, &error);
   bson_destroy (&b);
   if (supports_read_concern) {
      ASSERT_OR_PRINT (count == 0, error);
   } else {
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                             "The selected server does not support readConcern")
   }
   mongoc_read_concern_destroy (rc);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

int
skip_unless_server_has_decimal128 (void)
{
   if (!TestSuite_CheckLive ()) {
      return 0;
   }
   if (test_framework_get_server_version () >=
       test_framework_str_to_version ("3.3.5")) {
      return 1;
   }
   return 0;
}

int
mongod_supports_majority_read_concern (void)
{
   return test_framework_getenv_bool ("MONGOC_ENABLE_MAJORITY_READ_CONCERN");
}

static void
test_count_read_concern_live (void *context)
{
   if (test_framework_max_wire_version_at_least (WIRE_VERSION_READ_CONCERN)) {
      _test_count_read_concern_live (true);
   } else {
      _test_count_read_concern_live (false);
   }
}


static void
test_count_with_opts (void)
{
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   future_t *future;
   request_t *request;
   bson_error_t error;

   /* use a mongos since we don't send SLAVE_OK to mongos by default */
   server = mock_mongos_new (WIRE_VERSION_MIN);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   future = future_collection_count_with_opts (collection,
                                               MONGOC_QUERY_SLAVE_OK,
                                               NULL,
                                               0,
                                               0,
                                               tmp_bson ("{'opt': 1}"),
                                               NULL,
                                               &error);

   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'count': 'collection', 'opt': 1}");

   mock_server_replies_simple (request, "{'ok': 1, 'n': 1}");
   ASSERT_OR_PRINT (1 == future_get_int64_t (future), error);

   request_destroy (request);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_count_with_collation (int wire)
{
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   future_t *future;
   request_t *request;
   bson_error_t error;

   server = mock_server_with_autoismaster (wire);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   future = future_collection_count_with_opts (
      collection,
      MONGOC_QUERY_SLAVE_OK,
      NULL,
      0,
      0,
      tmp_bson ("{'collation': {'locale': 'en'}}"),
      NULL,
      &error);

   if (wire == WIRE_VERSION_COLLATION) {
      request = mock_server_receives_command (
         server,
         "db",
         MONGOC_QUERY_SLAVE_OK,
         "{'count': 'collection', 'collation': {'locale': 'en'}}");
      mock_server_replies_simple (request, "{'ok': 1, 'n': 1}");
      ASSERT_OR_PRINT (1 == future_get_int64_t (future), error);
      request_destroy (request);
   } else {
      ASSERT (-1 == future_get_int64_t (future));
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                             "The selected server does not support collation");
   }


   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_count_with_collation_ok (void)
{
   test_count_with_collation (WIRE_VERSION_COLLATION);
}


static void
test_count_with_collation_fail (void)
{
   test_count_with_collation (WIRE_VERSION_COLLATION - 1);
}


static void
test_drop (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_write_concern_t *good_wc;
   mongoc_write_concern_t *bad_wc;
   bool wire_version_5;
   bool r;
   bson_error_t error;
   bson_t *doc;
   bson_t *opts = NULL;

   opts = bson_new ();
   client = test_framework_client_new ();
   ASSERT (client);
   mongoc_client_set_error_api (client, 2);

   bad_wc = mongoc_write_concern_new ();
   good_wc = mongoc_write_concern_new ();

   wire_version_5 = test_framework_max_wire_version_at_least (5);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_drop");
   ASSERT (collection);

   doc = BCON_NEW ("hello", "world");
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (collection, doc, NULL, NULL, &error),
      error);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);
   ASSERT (!mongoc_collection_drop (collection, &error));

   /* invalid writeConcern */
   bad_wc->wtimeout = -10;
   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (collection, doc, NULL, NULL, &error),
      error);
   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   ASSERT (!mongoc_collection_drop_with_opts (collection, opts, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid writeConcern");
   bad_wc->wtimeout = 0;
   error.code = 0;
   error.domain = 0;

   /* valid writeConcern */
   mongoc_write_concern_set_w (good_wc, 1);
   bson_reinit (opts);
   mongoc_write_concern_append (good_wc, opts);
   ASSERT_OR_PRINT (mongoc_collection_drop_with_opts (collection, opts, &error),
                    error);
   ASSERT (!error.code);
   ASSERT (!error.domain);

   /* writeConcern that results in writeConcernError */
   mongoc_write_concern_set_w (bad_wc, 99);

   if (!test_framework_is_mongos ()) { /* skip if sharded */
      ASSERT_OR_PRINT (
         mongoc_collection_insert_one (collection, doc, NULL, NULL, &error),
         error);
      bson_reinit (opts);
      mongoc_write_concern_append_bad (bad_wc, opts);
      r = mongoc_collection_drop_with_opts (collection, opts, &error);
      if (wire_version_5) {
         ASSERT (!r);
         if (test_framework_is_replset ()) { /* replica set */
            ASSERT_ERROR_CONTAINS (
               error, MONGOC_ERROR_WRITE_CONCERN, 100, "Write Concern error:");
         } else { /* standalone */
            ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER);
            ASSERT_CMPINT (error.code, ==, 2);
         }
      } else { /* if wire_version <= 4, no error */
         ASSERT_OR_PRINT (r, error);
         ASSERT (!error.code);
         ASSERT (!error.domain);
      }
   }

   bson_destroy (doc);
   bson_destroy (opts);
   mongoc_write_concern_destroy (good_wc);
   mongoc_write_concern_destroy (bad_wc);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
}


static void
test_aggregate_bypass (void *context)
{
   mongoc_collection_t *data_collection;
   mongoc_collection_t *out_collection;
   mongoc_bulk_operation_t *bulk;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc;
   bson_t *pipeline;
   bson_t *options;
   char *collname;
   char *dbname;
   bson_t reply;
   bool r;
   int i;
   char *json;

   client = test_framework_client_new ();
   BSON_ASSERT (client);

   dbname = gen_collection_name ("dbtest");
   collname = gen_collection_name ("data");
   database = mongoc_client_get_database (client, dbname);
   data_collection = mongoc_database_get_collection (database, collname);
   bson_free (collname);

   collname = gen_collection_name ("bypass");
   options = tmp_bson (
      "{'validator': {'number': {'$gte': 5}}, 'validationAction': 'error'}");
   out_collection =
      mongoc_database_create_collection (database, collname, options, &error);
   ASSERT_OR_PRINT (out_collection, error);

   bson_free (dbname);
   bson_free (collname);

   /* Generate some example data */
   bulk =
      mongoc_collection_create_bulk_operation_with_opts (data_collection, NULL);

   for (i = 0; i < 3; i++) {
      bson_t *document;
      json = bson_strdup_printf ("{'number': 3, 'high': %d }", i);
      document = tmp_bson (json);

      mongoc_bulk_operation_insert (bulk, document);

      bson_free (json);
   }

   r = (bool) mongoc_bulk_operation_execute (bulk, &reply, &error);
   ASSERT_OR_PRINT (r, error);
   mongoc_bulk_operation_destroy (bulk);

   json = bson_strdup_printf ("[{'$out': '%s'}]", out_collection->collection);
   pipeline = tmp_bson (json);

   cursor = mongoc_collection_aggregate (
      data_collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL);
   ASSERT (cursor);
   r = mongoc_cursor_next (cursor, &doc);
   ASSERT (!r);
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_STARTSWITH (error.message, "insert for $out failed");
   mongoc_cursor_destroy (cursor);

   options = tmp_bson ("{'bypassDocumentValidation': true}");
   cursor = mongoc_collection_aggregate (
      data_collection, MONGOC_QUERY_NONE, pipeline, options, NULL);
   ASSERT (cursor);
   ASSERT (!mongoc_cursor_error (cursor, &error));

   ASSERT_OR_PRINT (mongoc_collection_drop (data_collection, &error), error);
   ASSERT_OR_PRINT (mongoc_collection_drop (out_collection, &error), error);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (data_collection);
   mongoc_collection_destroy (out_collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   bson_free (json);
}


static void
test_aggregate (void)
{
   mongoc_collection_t *collection;
   mongoc_database_t *database;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_error_t error;
   bool did_alternate = false;
   bool r;
   bson_t opts;
   bson_t *pipeline;
   bson_t *broken_pipeline;
   bson_t *b;
   bson_iter_t iter;
   int i, j;

   client = test_framework_client_new ();
   ASSERT (client);

   database = get_test_database (client);
   ASSERT (database);

   collection = get_test_collection (client, "test_aggregate");
   ASSERT (collection);

   pipeline = BCON_NEW ("pipeline",
                        "[",
                        "{",
                        "$match",
                        "{",
                        "hello",
                        BCON_UTF8 ("world"),
                        "}",
                        "}",
                        "]");
   broken_pipeline = BCON_NEW ("pipeline",
                               "[",
                               "{",
                               "$asdf",
                               "{",
                               "foo",
                               BCON_UTF8 ("bar"),
                               "}",
                               "}",
                               "]");
   b = BCON_NEW ("hello", BCON_UTF8 ("world"));

   /* empty collection */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL);
   ASSERT (cursor);

   ASSERT (!mongoc_cursor_next (cursor, &doc));
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   /* empty collection */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL);
   ASSERT (cursor);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT (!r);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   mongoc_cursor_destroy (cursor);

   for (i = 0; i < 2; i++) {
      ASSERT_OR_PRINT (
         mongoc_collection_insert_one (collection, b, NULL, NULL, &error),
         error);
   }

again:
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, broken_pipeline, NULL, NULL);
   ASSERT (cursor);

   r = mongoc_cursor_next (cursor, &doc);
   ASSERT (!r);
   ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT (error.code);
   mongoc_cursor_destroy (cursor);

   for (i = 0; i < 2; i++) {
      if (i % 2 == 0) {
         cursor = mongoc_collection_aggregate (
            collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL);
         ASSERT (cursor);
      } else {
         bson_init (&opts);

         BSON_APPEND_BOOL (&opts, "allowDiskUse", true);

         BSON_APPEND_INT32 (&opts, "batchSize", 10);
         cursor = mongoc_collection_aggregate (
            collection, MONGOC_QUERY_NONE, pipeline, &opts, NULL);
         ASSERT (cursor);

         bson_destroy (&opts);
      }

      for (j = 0; j < 2; j++) {
         r = mongoc_cursor_next (cursor, &doc);
         if (mongoc_cursor_error (cursor, &error)) {
            fprintf (
               stderr, "[%d.%d] %s", error.domain, error.code, error.message);

            abort ();
         }

         ASSERT (r);
         ASSERT (doc);

         ASSERT (bson_iter_init_find (&iter, doc, "hello") &&
                 BSON_ITER_HOLDS_UTF8 (&iter));
      }

      r = mongoc_cursor_next (cursor, &doc);
      if (mongoc_cursor_error (cursor, &error)) {
         fprintf (stderr, "%s", error.message);
         abort ();
      }

      ASSERT (!r);
      ASSERT (!doc);

      mongoc_cursor_destroy (cursor);
   }

   if (!did_alternate) {
      did_alternate = true;
      bson_destroy (pipeline);
      pipeline = BCON_NEW (
         "0", "{", "$match", "{", "hello", BCON_UTF8 ("world"), "}", "}");
      goto again;
   }

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_client_destroy (client);
   bson_destroy (b);
   bson_destroy (pipeline);
   bson_destroy (broken_pipeline);
}


static void
test_aggregate_large (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_iter_t iter;
   int32_t i;
   uint32_t server_id;
   mongoc_cursor_t *cursor;
   bson_t *inserted_doc;
   bson_error_t error;
   bson_t *pipeline;
   const bson_t *doc;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = get_test_collection (client, "test_aggregate_large");
   ASSERT (collection);

   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, NULL);

   /* ensure a few batches */
   inserted_doc = tmp_bson ("{'_id': 0}");

   for (i = 0; i < 2000; i++) {
      bson_iter_init_find (&iter, inserted_doc, "_id");
      bson_iter_overwrite_int32 (&iter, i);
      mongoc_bulk_operation_insert (bulk, inserted_doc);
   }

   server_id = mongoc_bulk_operation_execute (bulk, NULL, &error);
   ASSERT_OR_PRINT (server_id > 0, error);

   pipeline = tmp_bson ("[{'$sort': {'_id': 1}}]");

   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, pipeline, NULL, NULL);
   ASSERT (cursor);

   i = 0;
   while (mongoc_cursor_next (cursor, &doc)) {
      ASSERT (bson_iter_init_find (&iter, doc, "_id"));
      ASSERT_CMPINT (i, ==, bson_iter_int32 (&iter));
      i++;
   }

   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);
   ASSERT_CMPINT (i, ==, 2000);

   mongoc_bulk_operation_destroy (bulk);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


typedef struct {
   bool with_batch_size;
   bool with_options;
} test_aggregate_context_t;


static const char *
options_json (test_aggregate_context_t *c)
{
   if (c->with_batch_size && c->with_options) {
      return "{'foo': 1, 'batchSize': 11}";
   } else if (c->with_batch_size) {
      return "{'batchSize': 11}";
   } else if (c->with_options) {
      return "{'foo': 1}";
   } else {
      return "{}";
   }
}


static void
test_aggregate_modern (void *data)
{
   test_aggregate_context_t *context = (test_aggregate_context_t *) data;
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   future_t *future;
   request_t *request;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   if (!TestSuite_CheckMockServerAllowed ()) {
      return;
   }

   server = mock_server_with_autoismaster (WIRE_VERSION_OP_MSG - 1);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   cursor = mongoc_collection_aggregate (collection,
                                         MONGOC_QUERY_NONE,
                                         tmp_bson ("[{'a': 1}]"),
                                         tmp_bson (options_json (context)),
                                         NULL);

   ASSERT (cursor);
   future = future_cursor_next (cursor, &doc);

   /* "cursor" argument always sent if wire version >= 1 */
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'aggregate': 'collection',"
      " 'pipeline': [{'a': 1}],"
      " 'cursor': %s %s}",
      context->with_batch_size ? "{'batchSize': 11}" : "{'$empty': true}",
      context->with_options ? ", 'foo': 1" : "");

   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'db.collection',"
                               "    'firstBatch': [{'_id': 123}]"
                               "}}");

   ASSERT (future_get_bool (future));
   ASSERT_MATCH (doc, "{'_id': 123}");

   /* cursor is completed */
   BSON_ASSERT (!mongoc_cursor_next (cursor, &doc));

   mongoc_cursor_destroy (cursor);
   request_destroy (request);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_aggregate_w_server_id (void)
{
   mock_rs_t *rs;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *opts;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   future_t *future;
   request_t *request;

   rs = mock_rs_with_autoismaster (WIRE_VERSION_AGG_CURSOR,
                                   true /* has primary */,
                                   1 /* secondary   */,
                                   0 /* arbiters    */);

   mock_rs_run (rs);
   client = mongoc_client_new_from_uri (mock_rs_get_uri (rs));
   collection = mongoc_client_get_collection (client, "db", "collection");

   /* use serverId instead of prefs to select the secondary */
   opts = tmp_bson ("{'serverId': 2}");
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson (NULL), opts, NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_rs_receives_command (rs,
                                       "db",
                                       MONGOC_QUERY_SLAVE_OK,
                                       "{'aggregate': 'collection',"
                                       " 'cursor': {},"
                                       " 'serverId': {'$exists': false}}");

   ASSERT (mock_rs_request_is_to_secondary (rs, request));
   mock_rs_replies_simple (request,
                           "{'ok': 1,"
                           " 'cursor': {"
                           "    'ns': 'db.collection',"
                           "    'firstBatch': [{}]}}");
   ASSERT_OR_PRINT (future_get_bool (future), cursor->error);

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_rs_destroy (rs);
}


static void
test_aggregate_w_server_id_sharded (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   bson_t *opts;
   const bson_t *doc;
   future_t *future;
   request_t *request;

   server = mock_mongos_new (WIRE_VERSION_AGG_CURSOR);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   opts = tmp_bson ("{'serverId': 1}");
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson (NULL), opts, NULL);

   future = future_cursor_next (cursor, &doc);

   /* does NOT set slave ok, since this is a sharded topology */
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_NONE,
      "{'aggregate': 'collection', 'serverId': {'$exists': false}}");

   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'ns': 'db.collection',"
                               "    'firstBatch': [{}]}}");

   ASSERT_OR_PRINT (future_get_bool (future), cursor->error);

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_aggregate_server_id_option (void *ctx)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *q;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   client = test_framework_client_new ();
   collection = mongoc_client_get_collection (client, "db", "collection");
   q = tmp_bson (NULL);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, q, tmp_bson ("{'serverId': 'foo'}"), NULL);

   ASSERT_ERROR_CONTAINS (cursor->error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "must be an integer");

   mongoc_cursor_destroy (cursor);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, q, tmp_bson ("{'serverId': 0}"), NULL);

   ASSERT_ERROR_CONTAINS (cursor->error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "must be >= 1");

   mongoc_cursor_destroy (cursor);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, q, tmp_bson ("{'serverId': 1}"), NULL);

   mongoc_cursor_next (cursor, &doc);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_validate (void *ctx)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_iter_t iter;
   bson_error_t error;
   bson_t doc = BSON_INITIALIZER;
   bson_t opts = BSON_INITIALIZER;
   bson_t reply;
   bool r;
   const uint32_t expected_err_domain = MONGOC_ERROR_BSON;
   const uint32_t expected_err_code = MONGOC_ERROR_BSON_INVALID;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = get_test_collection (client, "test_validate");
   ASSERT (collection);

   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (collection, &doc, NULL, NULL, &error),
      error);

   BSON_APPEND_BOOL (&opts, "full", true);

   ASSERT_OR_PRINT (
      mongoc_collection_validate (collection, &opts, &reply, &error), error);

   BSON_ASSERT (bson_iter_init_find (&iter, &reply, "valid"));

   bson_destroy (&reply);

   /* Make sure we don't segfault when reply is NULL */
   ASSERT_OR_PRINT (
      mongoc_collection_validate (collection, &opts, NULL, &error), error);

   bson_reinit (&opts);
   BSON_APPEND_UTF8 (&opts, "full", "bad_value");

   /* invalidate reply */
   reply.len = 0;
   BSON_ASSERT (!bson_validate (&reply, BSON_VALIDATE_NONE, NULL));

   r = mongoc_collection_validate (collection, &opts, &reply, &error);
   BSON_ASSERT (!r);
   BSON_ASSERT (error.domain == expected_err_domain);
   BSON_ASSERT (error.code == expected_err_code);

   /* check that reply has been initialized */
   BSON_ASSERT (bson_validate (&reply, 0, NULL));

   /* Make sure we don't segfault when reply is NULL */
   memset (&error, 0, sizeof (error));
   r = mongoc_collection_validate (collection, &opts, NULL, &error);
   BSON_ASSERT (!r);
   BSON_ASSERT (error.domain == expected_err_domain);
   BSON_ASSERT (error.code == expected_err_code);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
   bson_destroy (&opts);
}


static void
test_rename (void)
{
   mongoc_client_t *client;
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   mongoc_write_concern_t *bad_wc;
   mongoc_write_concern_t *good_wc;
   bool wire_version_5;
   bool r;
   bson_error_t error;
   char *dbname;
   bson_t doc = BSON_INITIALIZER;
   bson_t *opts = NULL;
   char **name;
   char **names;
   bool found;

   client = test_framework_client_new ();
   ASSERT (client);
   mongoc_client_set_error_api (client, 2);
   opts = bson_new ();

   bad_wc = mongoc_write_concern_new ();
   good_wc = mongoc_write_concern_new ();

   wire_version_5 = test_framework_max_wire_version_at_least (5);

   dbname = gen_collection_name ("dbtest");
   database = mongoc_client_get_database (client, dbname);
   collection = mongoc_database_get_collection (database, "test_rename");

   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (collection, &doc, NULL, NULL, &error),
      error);

   ASSERT_OR_PRINT (mongoc_collection_rename (
                       collection, dbname, "test_rename.2", false, &error),
                    error);

   names =
      mongoc_database_get_collection_names_with_opts (database, NULL, &error);
   ASSERT_OR_PRINT (names, error);
   found = false;
   for (name = names; *name; ++name) {
      if (!strcmp (*name, "test_rename.2")) {
         found = true;
      }

      bson_free (*name);
   }

   ASSERT (found);
   ASSERT_CMPSTR (mongoc_collection_get_name (collection), "test_rename.2");

   /* invalid writeConcern */
   bad_wc->wtimeout = -10;
   bson_reinit (opts);
   mongoc_write_concern_append_bad (bad_wc, opts);
   ASSERT (!mongoc_collection_rename_with_opts (
      collection, dbname, "test_rename.3", false, opts, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid writeConcern");
   ASSERT_CMPSTR (mongoc_collection_get_name (collection), "test_rename.2");

   bad_wc->wtimeout = 0;
   error.code = 0;
   error.domain = 0;

   /* valid writeConcern on all configs */
   mongoc_write_concern_set_w (good_wc, 1);
   bson_reinit (opts);
   mongoc_write_concern_append (good_wc, opts);
   r = mongoc_collection_rename_with_opts (
      collection, dbname, "test_rename.3", false, opts, &error);
   ASSERT_OR_PRINT (r, error);
   ASSERT_CMPSTR (mongoc_collection_get_name (collection), "test_rename.3");

   ASSERT (!error.code);
   ASSERT (!error.domain);

   /* writeConcern that results in writeConcernError */
   mongoc_write_concern_set_w (bad_wc, 99);

   if (!test_framework_is_mongos ()) {
      bson_reinit (opts);
      mongoc_write_concern_append_bad (bad_wc, opts);
      r = mongoc_collection_rename_with_opts (
         collection, dbname, "test_rename.4", false, opts, &error);
      if (wire_version_5) {
         ASSERT (!r);

         /* check that collection name has not changed */
         ASSERT_CMPSTR (mongoc_collection_get_name (collection),
                        "test_rename.3");
         if (test_framework_is_replset ()) { /* replica set */
            ASSERT_ERROR_CONTAINS (
               error, MONGOC_ERROR_WRITE_CONCERN, 100, "Write Concern error:");
         } else { /* standalone */
            ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_SERVER);
            ASSERT_CMPINT (error.code, ==, 2);
         }
      } else { /* if wire version <= 4, no error */
         ASSERT_OR_PRINT (r, error);

         /* check that collection has been renamed */
         ASSERT_CMPSTR (mongoc_collection_get_name (collection),
                        "test_rename.4");
         ASSERT (!error.code);
         ASSERT (!error.domain);
      }
   }

   ASSERT_OR_PRINT (mongoc_database_drop (database, &error), error);

   bson_free (names);
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_write_concern_destroy (good_wc);
   mongoc_write_concern_destroy (bad_wc);
   mongoc_client_destroy (client);
   bson_free (dbname);
   bson_destroy (&doc);
   bson_destroy (opts);
}


static void
test_stats (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_t stats;
   bson_t doc = BSON_INITIALIZER;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = get_test_collection (client, "test_stats");
   ASSERT (collection);

   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (collection, &doc, NULL, NULL, &error),
      error);

   ASSERT_OR_PRINT (mongoc_collection_stats (collection, NULL, &stats, &error),
                    error);

   BSON_ASSERT (bson_iter_init_find (&iter, &stats, "ns"));

   BSON_ASSERT (bson_iter_init_find (&iter, &stats, "count"));
   BSON_ASSERT (bson_iter_as_int64 (&iter) >= 1);

   bson_destroy (&stats);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
}


static void
test_stats_read_pref (void)
{
   mock_server_t *server;
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_read_prefs_t *prefs;
   future_t *future;
   request_t *request;
   bson_error_t error;
   bson_t stats;

   server = mock_mongos_new (WIRE_VERSION_MIN);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");
   prefs = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   mongoc_collection_set_read_prefs (collection, prefs);
   future = future_collection_stats (collection, NULL, &stats, &error);
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'collStats': 'collection'},"
      " '$readPreference': {'mode': 'secondary'}}");

   mock_server_replies_ok_and_destroys (request);
   ASSERT_OR_PRINT (future_get_bool (future), error);

   future_destroy (future);
   bson_destroy (&stats);
   mongoc_read_prefs_destroy (prefs);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_find_and_modify_write_concern (int wire_version)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mock_server_t *server;
   request_t *request;
   future_t *future;
   bson_error_t error;
   bson_t *update;
   bson_t doc = BSON_INITIALIZER;
   bson_t reply;
   mongoc_write_concern_t *write_concern;

   server = mock_server_new ();
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   ASSERT (client);

   collection =
      mongoc_client_get_collection (client, "test", "test_find_and_modify");

   auto_ismaster (server,
                  wire_version, /* max_wire_version */
                  48000000,     /* max_message_size */
                  16777216,     /* max_bson_size */
                  1000);        /* max_write_batch_size */

   BSON_APPEND_INT32 (&doc, "superduper", 77889);

   update = BCON_NEW ("$set", "{", "superduper", BCON_INT32 (1234), "}");

   write_concern = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (write_concern, 42);
   mongoc_collection_set_write_concern (collection, write_concern);
   future = future_collection_find_and_modify (
      collection, &doc, NULL, update, NULL, false, false, true, &reply, &error);

   if (wire_version >= 4) {
      request = mock_server_receives_command (
         server,
         "test",
         MONGOC_QUERY_NONE,
         "{ 'findAndModify' : 'test_find_and_modify', "
         "'query' : { 'superduper' : 77889 },"
         "'update' : { '$set' : { 'superduper' : 1234 } },"
         "'new' : true,"
         "'writeConcern' : { 'w' : 42 } }");
   } else {
      request = mock_server_receives_command (
         server,
         "test",
         MONGOC_QUERY_NONE,
         "{ 'findAndModify' : 'test_find_and_modify', "
         "'query' : { 'superduper' : 77889 },"
         "'update' : { '$set' : { 'superduper' : 1234 } },"
         "'new' : true }");
   }

   mock_server_replies_simple (request, "{ 'value' : null, 'ok' : 1 }");
   ASSERT_OR_PRINT (future_get_bool (future), error);

   future_destroy (future);

   bson_destroy (&reply);
   bson_destroy (update);

   request_destroy (request);
   mongoc_write_concern_destroy (write_concern);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
   bson_destroy (&doc);
}

static void
test_find_and_modify_write_concern_wire_32 (void)
{
   test_find_and_modify_write_concern (4);
}

static void
test_find_and_modify_write_concern_wire_pre_32 (void)
{
   test_find_and_modify_write_concern (2);
}

static void
test_find_and_modify (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   bson_error_t error;
   bson_iter_t iter;
   bson_iter_t citer;
   bson_t *update;
   bson_t doc = BSON_INITIALIZER;
   bson_t reply;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = get_test_collection (client, "test_find_and_modify");
   ASSERT (collection);

   BSON_APPEND_INT32 (&doc, "superduper", 77889);

   ASSERT_OR_PRINT (
      mongoc_collection_insert_one (collection, &doc, NULL, NULL, &error),
      error);

   update = BCON_NEW ("$set", "{", "superduper", BCON_INT32 (1234), "}");

   ASSERT_OR_PRINT (mongoc_collection_find_and_modify (collection,
                                                       &doc,
                                                       NULL,
                                                       update,
                                                       NULL,
                                                       false,
                                                       false,
                                                       true,
                                                       &reply,
                                                       &error),
                    error);

   BSON_ASSERT (bson_iter_init_find (&iter, &reply, "value"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
   BSON_ASSERT (bson_iter_recurse (&iter, &citer));
   BSON_ASSERT (bson_iter_find (&citer, "superduper"));
   BSON_ASSERT (BSON_ITER_HOLDS_INT32 (&citer));
   BSON_ASSERT (bson_iter_int32 (&citer) == 1234);

   BSON_ASSERT (bson_iter_init_find (&iter, &reply, "lastErrorObject"));
   BSON_ASSERT (BSON_ITER_HOLDS_DOCUMENT (&iter));
   BSON_ASSERT (bson_iter_recurse (&iter, &citer));
   BSON_ASSERT (bson_iter_find (&citer, "updatedExisting"));
   BSON_ASSERT (BSON_ITER_HOLDS_BOOL (&citer));
   BSON_ASSERT (bson_iter_bool (&citer));

   bson_destroy (&reply);
   bson_destroy (update);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_destroy (&doc);
}


static void
test_large_return (void *ctx)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc = NULL;
   bson_oid_t oid;
   bson_t insert_doc = BSON_INITIALIZER;
   bson_t query = BSON_INITIALIZER;
   size_t len;
   char *str;
   bool r;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = get_test_collection (client, "test_large_return");
   ASSERT (collection);

   len = 1024 * 1024 * 4;
   str = (char *) bson_malloc (len);
   memset (str, (int) ' ', len);
   str[len - 1] = '\0';

   bson_oid_init (&oid, NULL);
   BSON_APPEND_OID (&insert_doc, "_id", &oid);
   BSON_APPEND_UTF8 (&insert_doc, "big", str);

   ASSERT_OR_PRINT (mongoc_collection_insert_one (
                       collection, &insert_doc, NULL, NULL, &error),
                    error);

   bson_destroy (&insert_doc);

   BSON_APPEND_OID (&query, "_id", &oid);

   cursor = mongoc_collection_find (
      collection, MONGOC_QUERY_NONE, 0, 0, 0, &query, NULL, NULL);
   BSON_ASSERT (cursor);
   bson_destroy (&query);

   ASSERT_CURSOR_NEXT (cursor, &doc);
   BSON_ASSERT (doc);

   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (!r);

   mongoc_cursor_destroy (cursor);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   bson_free (str);
}


static void
test_many_return (void)
{
   enum { N_BSONS = 5000 };

   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   bson_error_t error;
   const bson_t *doc = NULL;
   bson_oid_t oid;
   bson_t query = BSON_INITIALIZER;
   bson_t *docs[N_BSONS];
   bool r;
   int i;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = get_test_collection (client, "test_many_return");
   ASSERT (collection);

   for (i = 0; i < N_BSONS; i++) {
      docs[i] = bson_new ();
      bson_oid_init (&oid, NULL);
      BSON_APPEND_OID (docs[i], "_id", &oid);
   }

   ASSERT_OR_PRINT (mongoc_collection_insert_many (collection,
                                                   (const bson_t **) docs,
                                                   (uint32_t) N_BSONS,
                                                   NULL,
                                                   NULL,
                                                   &error),
                    error);

   cursor = mongoc_collection_find_with_opts (collection, &query, NULL, NULL);
   BSON_ASSERT (cursor);
   BSON_ASSERT (mongoc_cursor_is_alive (cursor));
   bson_destroy (&query);

   i = 0;

   while (mongoc_cursor_next (cursor, &doc)) {
      BSON_ASSERT (doc);
      i++;
      BSON_ASSERT (mongoc_cursor_is_alive (cursor));
   }

   BSON_ASSERT (i == N_BSONS);

   BSON_ASSERT (!mongoc_cursor_error (cursor, &error));
   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (!r);
   BSON_ASSERT (!mongoc_cursor_is_alive (cursor));
   /* mongoc_cursor_next after done is considered an error */
   BSON_ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_CURSOR,
                          MONGOC_ERROR_CURSOR_INVALID_CURSOR,
                          "Cannot advance a completed or failed cursor")

   mongoc_cursor_destroy (cursor);

   ASSERT_OR_PRINT (mongoc_collection_drop (collection, &error), error);

   destroy_all (docs, N_BSONS);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static bool
insert_one (mongoc_collection_t *collection,
            const bson_t *doc,
            const bson_t *opts,
            bson_error_t *error)
{
   return mongoc_collection_insert_one (collection, doc, opts, NULL, error);
}


static bool
insert_many (mongoc_collection_t *collection,
             const bson_t *doc,
             const bson_t *opts,
             bson_error_t *error)
{
   return mongoc_collection_insert_many (
      collection, &doc, 1, opts, NULL, error);
}


typedef bool (*insert_fn_t) (mongoc_collection_t *,
                             const bson_t *,
                             const bson_t *,
                             bson_error_t *);


static void
_test_insert_validate (insert_fn_t insert_fn)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;

   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   collection = get_test_collection (client, "test_insert_validate");

   BSON_ASSERT (!insert_fn (collection, tmp_bson ("{'$': 1}"), NULL, &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "invalid document");

   BSON_ASSERT (!insert_fn (collection,
                            tmp_bson ("{'$': 1}"),
                            tmp_bson ("{'validate': false}"),
                            &error));
   ASSERT_CMPUINT32 (error.domain, ==, (uint32_t) MONGOC_ERROR_SERVER);

   BSON_ASSERT (!insert_fn (collection,
                            tmp_bson ("{'$': 1}"),
                            tmp_bson ("{'validate': 'foo'}"),
                            &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid type for option \"validate\", \"UTF8\"");

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_insert_one_validate (void)
{
   _test_insert_validate (insert_one);
}


static void
test_insert_many_validate (void)
{
   _test_insert_validate (insert_many);
}


/* use a mock server to test the "limit" parameter */
static void
test_find_limit (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;

   server = mock_server_with_autoismaster (WIRE_VERSION_MIN);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "test", "test");

   /* test mongoc_collection_find and mongoc_collection_find_with_opts */
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0 /* skip */,
                                    2 /* limit */,
                                    0 /* batch_size */,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_query (server,
                                         "test.test",
                                         MONGOC_QUERY_SLAVE_OK,
                                         0 /* skip */,
                                         2 /* n_return */,
                                         "{}",
                                         NULL);

   mock_server_replies_simple (request, "{}");
   BSON_ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);

   cursor = mongoc_collection_find_with_opts (
      collection,
      tmp_bson ("{}"),
      tmp_bson ("{'limit': {'$numberLong': '2'}}"),
      NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_query (server,
                                         "test.test",
                                         MONGOC_QUERY_SLAVE_OK,
                                         0 /* skip */,
                                         2 /* n_return */,
                                         "{}",
                                         NULL);

   mock_server_replies_simple (request, "{}");
   BSON_ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


/* use a mock server to test the "batch_size" parameter */
static void
test_find_batch_size (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;

   server = mock_server_with_autoismaster (WIRE_VERSION_MIN);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "test", "test");

   /* test mongoc_collection_find and mongoc_collection_find_with_opts */
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_NONE,
                                    0 /* skip */,
                                    0 /* limit */,
                                    2 /* batch_size */,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_query (server,
                                         "test.test",
                                         MONGOC_QUERY_SLAVE_OK,
                                         0 /* skip */,
                                         2 /* n_return */,
                                         "{}",
                                         NULL);

   mock_server_replies_simple (request, "{}");
   BSON_ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);

   cursor = mongoc_collection_find_with_opts (
      collection,
      tmp_bson ("{}"),
      tmp_bson ("{'batchSize': {'$numberLong': '2'}}"),
      NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_query (server,
                                         "test.test",
                                         MONGOC_QUERY_SLAVE_OK,
                                         0 /* skip */,
                                         2 /* n_return */,
                                         "{}",
                                         NULL);

   mock_server_replies_simple (request, "{}");
   BSON_ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_command_fq (void *context)
{
   mongoc_client_t *client;
   mongoc_cursor_t *cursor;
   const bson_t *doc = NULL;
   bson_iter_t iter;
   bson_t *cmd;
   bool r;

   client = test_framework_client_new ();
   ASSERT (client);

   cmd = tmp_bson ("{ 'dbstats': 1}");

   cursor = mongoc_client_command (client,
                                   "sometest.$cmd",
                                   MONGOC_QUERY_SLAVE_OK,
                                   0,
                                   -1,
                                   0,
                                   cmd,
                                   NULL,
                                   NULL);
   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (r);

   if (bson_iter_init_find (&iter, doc, "db") && BSON_ITER_HOLDS_UTF8 (&iter)) {
      ASSERT_CMPSTR (bson_iter_utf8 (&iter, NULL), "sometest");
   } else {
      fprintf (stderr, "dbstats didn't return 'db' key?");
      abort ();
   }


   r = mongoc_cursor_next (cursor, &doc);
   BSON_ASSERT (!r);

   mongoc_cursor_destroy (cursor);
   mongoc_client_destroy (client);
}

static void
test_get_index_info (void)
{
   mongoc_collection_t *collection;
   mongoc_client_t *client;
   mongoc_index_opt_t opt1;
   mongoc_index_opt_t opt2;
   bson_error_t error = {0};
   mongoc_cursor_t *cursor;
   const bson_t *indexinfo;
   bson_t indexkey1;
   bson_t indexkey2;
   bson_t dummy = BSON_INITIALIZER;
   bson_iter_t idx_spec_iter;
   bson_iter_t idx_spec_iter_copy;
   bool r;
   const char *cur_idx_name;
   char *idx1_name = NULL;
   char *idx2_name = NULL;
   const char *id_idx_name = "_id_";
   int num_idxs = 0;

   client = test_framework_client_new ();
   ASSERT (client);

   collection = get_test_collection (client, "test_get_index_info");
   ASSERT (collection);

   /*
    * Try it on a collection that doesn't exist.
    */
   cursor = mongoc_collection_find_indexes_with_opts (collection, NULL);

   ASSERT (!mongoc_cursor_next (cursor, &indexinfo));
   ASSERT (!mongoc_cursor_error (cursor, &error));

   mongoc_cursor_destroy (cursor);

   /* insert a dummy document so that the collection actually exists */
   r = mongoc_collection_insert_one (collection, &dummy, NULL, NULL, &error);
   ASSERT (r);

   /* Try it on a collection with no secondary indexes.
    * We should just get back the index on _id.
    */
   cursor = mongoc_collection_find_indexes_with_opts (collection, NULL);
   ASSERT (!mongoc_cursor_error (cursor, &error));

   while (mongoc_cursor_next (cursor, &indexinfo)) {
      if (bson_iter_init (&idx_spec_iter, indexinfo) &&
          bson_iter_find (&idx_spec_iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&idx_spec_iter) &&
          (cur_idx_name = bson_iter_utf8 (&idx_spec_iter, NULL))) {
         BSON_ASSERT (0 == strcmp (cur_idx_name, id_idx_name));
         ++num_idxs;
      } else {
         BSON_ASSERT (false);
      }
   }

   BSON_ASSERT (1 == num_idxs);

   mongoc_cursor_destroy (cursor);

   num_idxs = 0;
   indexinfo = NULL;

   bson_init (&indexkey1);
   BSON_APPEND_INT32 (&indexkey1, "raspberry", 1);
   idx1_name = mongoc_collection_keys_to_index_string (&indexkey1);
   mongoc_index_opt_init (&opt1);
   opt1.background = true;
   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &indexkey1, &opt1, &error),
      error);

   bson_init (&indexkey2);
   BSON_APPEND_INT32 (&indexkey2, "snozzberry", 1);
   idx2_name = mongoc_collection_keys_to_index_string (&indexkey2);
   mongoc_index_opt_init (&opt2);
   opt2.unique = true;
   ASSERT_OR_PRINT (
      mongoc_collection_create_index (collection, &indexkey2, &opt2, &error),
      error);

   /*
    * Now we try again after creating two indexes.
    */
   cursor = mongoc_collection_find_indexes_with_opts (collection, NULL);
   ASSERT (!mongoc_cursor_error (cursor, &error));

   while (mongoc_cursor_next (cursor, &indexinfo)) {
      if (bson_iter_init (&idx_spec_iter, indexinfo) &&
          bson_iter_find (&idx_spec_iter, "name") &&
          BSON_ITER_HOLDS_UTF8 (&idx_spec_iter) &&
          (cur_idx_name = bson_iter_utf8 (&idx_spec_iter, NULL))) {
         if (0 == strcmp (cur_idx_name, idx1_name)) {
            /* need to use the copy of the iter since idx_spec_iter may have
             * gone
             * past the key we want */
            ASSERT (bson_iter_init_find (
               &idx_spec_iter_copy, indexinfo, "background"));
            ASSERT (BSON_ITER_HOLDS_BOOL (&idx_spec_iter_copy));
            ASSERT (bson_iter_bool (&idx_spec_iter_copy));
         } else if (0 == strcmp (cur_idx_name, idx2_name)) {
            ASSERT (
               bson_iter_init_find (&idx_spec_iter_copy, indexinfo, "unique"));
            ASSERT (BSON_ITER_HOLDS_BOOL (&idx_spec_iter_copy));
            ASSERT (bson_iter_bool (&idx_spec_iter_copy));
         } else {
            ASSERT ((0 == strcmp (cur_idx_name, id_idx_name)));
         }

         ++num_idxs;
      } else {
         BSON_ASSERT (false);
      }
   }

   BSON_ASSERT (3 == num_idxs);

   mongoc_cursor_destroy (cursor);

   bson_free (idx1_name);
   bson_free (idx2_name);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_find_indexes_err (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   future_t *future;
   request_t *request;
   mongoc_cursor_t *cursor;
   bson_error_t error;

   server = mock_server_with_autoismaster (WIRE_VERSION_MIN);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   mongoc_client_set_error_api (client, 2);
   collection = mongoc_client_get_collection (client, "db", "collection");

   future = future_collection_find_indexes_with_opts (collection, NULL);
   request = mock_server_receives_command (
      server, "db", MONGOC_QUERY_SLAVE_OK, "{'listIndexes': 'collection'}");

   mock_server_replies_simple (request,
                               "{'ok': 0, 'code': 1234567, 'errmsg': 'foo'}");
   cursor = future_get_mongoc_cursor_ptr (future);
   BSON_ASSERT (mongoc_cursor_error (cursor, &error));
   ASSERT_ERROR_CONTAINS (error, MONGOC_ERROR_SERVER, 1234567, "foo");

   mongoc_cursor_destroy (cursor);
   request_destroy (request);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_aggregate_install (TestSuite *suite)
{
   static test_aggregate_context_t test_aggregate_contexts[2][2];

   int with_batch_size, with_options;
   char *name;
   test_aggregate_context_t *context;

   for (with_batch_size = 0; with_batch_size < 2; with_batch_size++) {
      for (with_options = 0; with_options < 2; with_options++) {
         context = &test_aggregate_contexts[with_batch_size][with_options];

         context->with_batch_size = (bool) with_batch_size;
         context->with_options = (bool) with_options;

         name = bson_strdup_printf (
            "/Collection/aggregate/%s/%s",
            context->with_batch_size ? "batch_size" : "no_batch_size",
            context->with_options ? "with_options" : "no_options");

         TestSuite_AddWC (
            suite, name, test_aggregate_modern, NULL, (void *) context);
         bson_free (name);
      }
   }
}


static void
test_find_read_concern (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_read_concern_t *rc;
   mongoc_collection_t *collection;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;

   server = mock_server_with_autoismaster (WIRE_VERSION_READ_CONCERN);
   mock_server_run (server);

   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "test", "test");

   /* No read_concern set - test find and find_with_opts */
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_SLAVE_OK,
                                    0 /* skip */,
                                    0 /* limit */,
                                    0 /* batch_size */,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   future = future_cursor_next (cursor, &doc);
   request =
      mock_server_receives_command (server,
                                    "test",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{'find' : 'test', 'filter' : {  } }");
   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'test.test',"
                               "    'firstBatch': [{'_id': 123}]}}");
   ASSERT (future_get_bool (future));
   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);

   /* readConcernLevel = local */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_LOCAL);
   mongoc_collection_set_read_concern (collection, rc);
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_SLAVE_OK,
                                    0 /* skip */,
                                    0 /* limit */,
                                    0 /* batch_size */,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_command (server,
                                           "test",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{"
                                           "  'find' : 'test',"
                                           "  'filter' : {  },"
                                           "  'readConcern': {"
                                           "    'level': 'local'"
                                           "   }"
                                           "}");
   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'test.test',"
                               "    'firstBatch': [{'_id': 123}]}}");
   ASSERT (future_get_bool (future));
   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_read_concern_destroy (rc);

   /* readConcernLevel = random */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, "random");
   mongoc_collection_set_read_concern (collection, rc);
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_SLAVE_OK,
                                    0 /* skip */,
                                    0 /* limit */,
                                    0 /* batch_size */,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_command (server,
                                           "test",
                                           MONGOC_QUERY_SLAVE_OK,
                                           "{"
                                           "  'find' : 'test',"
                                           "  'filter' : {  },"
                                           "  'readConcern': {"
                                           "    'level': 'random'"
                                           "   }"
                                           "}");
   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'test.test',"
                               "    'firstBatch': [{'_id': 123}]}}");
   ASSERT (future_get_bool (future));
   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_read_concern_destroy (rc);

   /* empty readConcernLevel doesn't send anything */
   rc = mongoc_read_concern_new ();
   mongoc_collection_set_read_concern (collection, rc);
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_SLAVE_OK,
                                    0 /* skip */,
                                    0 /* limit */,
                                    0 /* batch_size */,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   future = future_cursor_next (cursor, &doc);
   request =
      mock_server_receives_command (server,
                                    "test",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{"
                                    "  'find' : 'test',"
                                    "  'filter' : {  },"
                                    "  'readConcern': { '$exists': false }"
                                    "}");
   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'test.test',"
                               "    'firstBatch': [{'_id': 123}]}}");
   ASSERT (future_get_bool (future));
   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_read_concern_destroy (rc);

   /* readConcernLevel = NULL doesn't send anything */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, NULL);
   mongoc_collection_set_read_concern (collection, rc);
   cursor = mongoc_collection_find (collection,
                                    MONGOC_QUERY_SLAVE_OK,
                                    0 /* skip */,
                                    0 /* limit */,
                                    0 /* batch_size */,
                                    tmp_bson ("{}"),
                                    NULL,
                                    NULL);

   future = future_cursor_next (cursor, &doc);
   request =
      mock_server_receives_command (server,
                                    "test",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{"
                                    "  'find' : 'test',"
                                    "  'filter' : {  },"
                                    "  'readConcern': { '$exists': false }"
                                    "}");
   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'test.test',"
                               "    'firstBatch': [{'_id': 123}]}}");
   ASSERT (future_get_bool (future));

   future_destroy (future);
   request_destroy (request);
   mongoc_cursor_destroy (cursor);
   mongoc_read_concern_destroy (rc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}

static void
test_getmore_read_concern_live (void *ctx)
{
   mongoc_client_t *client;
   mongoc_read_concern_t *rc;
   mongoc_collection_t *collection;
   mongoc_bulk_operation_t *bulk;
   bson_t *insert_doc;
   mongoc_cursor_t *cursor;
   mongoc_write_concern_t *wc;
   const bson_t *doc;
   bson_error_t error;
   int i = 0;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_read_concern");

   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_LOCAL);
   mongoc_collection_set_read_concern (collection, rc);


   wc = mongoc_write_concern_new ();
   mongoc_write_concern_set_w (wc, MONGOC_WRITE_CONCERN_W_MAJORITY);
   mongoc_collection_set_write_concern (collection, wc);
   bulk = mongoc_collection_create_bulk_operation_with_opts (collection, NULL);
   insert_doc = tmp_bson ("{'a': 1}");

   for (i = 5000; i > 0; i--) {
      mongoc_bulk_operation_insert_with_opts (bulk, insert_doc, NULL, NULL);
   }

   ASSERT_OR_PRINT (mongoc_bulk_operation_execute (bulk, NULL, &error), error);
   mongoc_bulk_operation_destroy (bulk);

   cursor = mongoc_collection_find_with_opts (
      collection, tmp_bson ("{}"), NULL, NULL);

   while (mongoc_cursor_next (cursor, &doc)) {
      i++;
   }
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   ASSERT_CMPINT (i, ==, 5000);
   mongoc_cursor_destroy (cursor);

   mongoc_read_concern_destroy (rc);
   mongoc_write_concern_destroy (wc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

static void
test_aggregate_secondary (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_read_prefs_t *pref;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "aggregate_secondary");
   pref = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson ("[]"), NULL, pref);

   ASSERT (cursor);
   mongoc_cursor_next (cursor, &doc);
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   if (test_framework_is_replset ()) {
      ASSERT (test_framework_server_is_secondary (
         client, mongoc_cursor_get_hint (cursor)));
   }

   mongoc_read_prefs_destroy (pref);
   mongoc_cursor_destroy (cursor);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_aggregate_secondary_sharded (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_read_prefs_t *pref;
   bson_error_t error;
   mongoc_cursor_t *cursor;
   future_t *future;
   request_t *request;
   const bson_t *doc;

   server = mock_mongos_new (2 /* wire version */);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");
   pref = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson ("[]"), NULL, pref);

   ASSERT (cursor);
   future = future_cursor_next (cursor, &doc);
   request = mock_server_receives_command (
      server,
      "db",
      MONGOC_QUERY_SLAVE_OK,
      "{'$query': {'aggregate': 'collection', 'pipeline': []},"
      " '$readPreference': {'mode': 'secondary'}}");

   mock_server_replies_simple (request,
                               "{ 'ok':1,"
                               "  'cursor': {"
                               "     'id': 0,"
                               "     'ns': 'db.collection',"
                               "     'firstBatch': []}}");

   ASSERT (!future_get_bool (future)); /* cursor_next returns false */
   ASSERT_OR_PRINT (!mongoc_cursor_error (cursor, &error), error);

   request_destroy (request);
   future_destroy (future);
   mongoc_cursor_destroy (cursor);
   mongoc_read_prefs_destroy (pref);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_aggregate_read_concern (void)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   mongoc_read_concern_t *rc;
   future_t *future;
   request_t *request;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   server = mock_server_with_autoismaster (WIRE_VERSION_READ_CONCERN);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   /* No readConcern */
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson ("[{'a': 1}]"), NULL, NULL);

   ASSERT (cursor);
   future = future_cursor_next (cursor, &doc);

   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{"
                                    "  'aggregate' : 'collection',"
                                    "  'pipeline' : [{"
                                    "      'a' : 1"
                                    "  }],"
                                    "  'cursor' : {  },"
                                    "  'readConcern': { '$exists': false }"
                                    "}");

   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'db.collection',"
                               "    'firstBatch': [{'_id': 123}]"
                               "}}");

   ASSERT (future_get_bool (future));
   ASSERT_MATCH (doc, "{'_id': 123}");

   /* cursor is completed */
   BSON_ASSERT (!mongoc_cursor_next (cursor, &doc));
   mongoc_cursor_destroy (cursor);
   request_destroy (request);
   future_destroy (future);

   /* readConcern: majority */
   rc = mongoc_read_concern_new ();
   mongoc_read_concern_set_level (rc, MONGOC_READ_CONCERN_LEVEL_MAJORITY);
   mongoc_collection_set_read_concern (collection, rc);
   cursor = mongoc_collection_aggregate (
      collection, MONGOC_QUERY_NONE, tmp_bson ("[{'a': 1}]"), NULL, NULL);

   ASSERT (cursor);
   future = future_cursor_next (cursor, &doc);

   request =
      mock_server_receives_command (server,
                                    "db",
                                    MONGOC_QUERY_SLAVE_OK,
                                    "{"
                                    "  'aggregate' : 'collection',"
                                    "  'pipeline' : [{"
                                    "      'a' : 1"
                                    "  }],"
                                    "  'cursor' : {  },"
                                    "  'readConcern': { 'level': 'majority'}"
                                    "}");

   mock_server_replies_simple (request,
                               "{'ok': 1,"
                               " 'cursor': {"
                               "    'id': 0,"
                               "    'ns': 'db.collection',"
                               "    'firstBatch': [{'_id': 123}]"
                               "}}");

   ASSERT (future_get_bool (future));
   ASSERT_MATCH (doc, "{'_id': 123}");

   /* cursor is completed */
   BSON_ASSERT (!mongoc_cursor_next (cursor, &doc));
   mongoc_cursor_destroy (cursor);
   request_destroy (request);
   future_destroy (future);

   mongoc_read_concern_destroy (rc);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_aggregate_with_collation (int wire)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   future_t *future;
   request_t *request;
   mongoc_cursor_t *cursor;
   const bson_t *doc;
   bson_error_t error;

   /* wire protocol version 0 */
   server = mock_server_with_autoismaster (wire);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   cursor =
      mongoc_collection_aggregate (collection,
                                   MONGOC_QUERY_NONE,
                                   tmp_bson ("[{'a': 1}]"),
                                   tmp_bson ("{'collation': {'locale': 'en'}}"),
                                   NULL);

   future = future_cursor_next (cursor, &doc);

   if (wire == WIRE_VERSION_COLLATION) {
      request =
         mock_server_receives_command (server,
                                       "db",
                                       MONGOC_QUERY_SLAVE_OK,
                                       "{'aggregate': 'collection',"
                                       " 'pipeline': [{'a': 1}],"
                                       " 'collation': {'locale': 'en'}}");

      mock_server_replies_simple (request,
                                  "{'ok': 1,"
                                  " 'cursor': {"
                                  "    'id': 0,"
                                  "    'ns': 'db.collection',"
                                  "    'firstBatch': [{'_id': 123}]"
                                  "}}");
      ASSERT (future_get_bool (future));
      ASSERT_MATCH (doc, "{'_id': 123}");
      /* cursor is completed */
      BSON_ASSERT (!mongoc_cursor_next (cursor, &doc));
      request_destroy (request);
   } else {
      ASSERT (!future_get_bool (future));
      mongoc_cursor_next (cursor, &doc);

      ASSERT (mongoc_cursor_error (cursor, &error));
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                             "The selected server does not support collation");
   }

   mongoc_cursor_destroy (cursor);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_aggregate_with_collation_fail (void)
{
   test_aggregate_with_collation (WIRE_VERSION_COLLATION - 1);
}

static void
test_aggregate_with_collation_ok (void)
{
   test_aggregate_with_collation (WIRE_VERSION_COLLATION);
}


static void
test_index_with_collation (int wire)
{
   mock_server_t *server;
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   request_t *request;
   bson_error_t error;
   bson_t *collation;
   bson_t keys;
   mongoc_index_opt_t opt;
   bson_t reply;
   future_t *future;

   /* wire protocol version 0 */
   server = mock_server_with_autoismaster (wire);
   mock_server_run (server);
   client = mongoc_client_new_from_uri (mock_server_get_uri (server));
   collection = mongoc_client_get_collection (client, "db", "collection");

   bson_init (&keys);
   bson_append_int32 (&keys, "hello", -1, 1);
   mongoc_index_opt_init (&opt);
   collation =
      BCON_NEW ("locale", BCON_UTF8 ("en"), "strength", BCON_INT32 (2));
   opt.collation = collation;

   future = future_collection_create_index_with_opts (
      collection, &keys, &opt, NULL, &reply, &error);

   if (wire == WIRE_VERSION_COLLATION) {
      request = mock_server_receives_command (
         server,
         "db",
         0,
         "{ 'createIndexes' : 'collection',"
         "  'indexes' : ["
         "    {"
         "      'key' : {"
         "        'hello' : 1"
         "      },"
         "      'name' : 'hello_1',"
         "      'collation': {'locale': 'en', 'strength': 2 }"
         "    }"
         "  ]"
         "}");

      mock_server_replies_simple (request, "{'ok': 1}");
      ASSERT (future_get_bool (future));
      request_destroy (request);
   } else {
      ASSERT (!future_get_bool (future));
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_PROTOCOL_BAD_WIRE_VERSION,
                             "The selected server does not support collation");
   }

   bson_destroy (collation);
   bson_destroy (&keys);
   future_destroy (future);
   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
   mock_server_destroy (server);
}


static void
test_index_with_collation_fail (void)
{
   test_index_with_collation (WIRE_VERSION_COLLATION - 1);
}

static void
test_index_with_collation_ok (void)
{
   test_index_with_collation (WIRE_VERSION_COLLATION);
}

static void
test_insert_duplicate_key (void)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_error_t error;

   client = test_framework_client_new ();
   collection = get_test_collection (client, "test_insert_duplicate_key");
   mongoc_collection_insert_one (
      collection, tmp_bson ("{'_id': 1}"), NULL, NULL, NULL);

   ASSERT (!mongoc_collection_insert_one (
      collection, tmp_bson ("{'_id': 1}"), NULL, NULL, &error));
   ASSERT_CMPINT (error.domain, ==, MONGOC_ERROR_COLLECTION);
   ASSERT_CMPINT (error.code, ==, MONGOC_ERROR_DUPLICATE_KEY);

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

static void
test_create_index_fail (void *context)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bool r;
   bson_t reply;
   bson_error_t error;

   client =
      mongoc_client_new ("mongodb://example.doesntexist/?connectTimeoutMS=10");
   collection = mongoc_client_get_collection (client, "test", "test");
   r = mongoc_collection_create_index_with_opts (
      collection, tmp_bson ("{'a': 1}"), NULL, NULL, &reply, &error);

   ASSERT (!r);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_SERVER_SELECTION,
                          MONGOC_ERROR_SERVER_SELECTION_FAILURE,
                          "No suitable servers");

   /* reply was initialized */
   ASSERT (bson_empty (&reply));

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}

/* Tests that documents in `coll` found with `selector` all match `match` */
static void
_test_docs_in_coll_matches (mongoc_collection_t *coll,
                            bson_t *selector,
                            const char *match,
                            uint32_t expected_count)
{
   const bson_t *next_doc;
   mongoc_cursor_t *cursor =
      mongoc_collection_find_with_opts (coll, selector, NULL, NULL);
   while (expected_count > 0) {
      ASSERT (mongoc_cursor_next (cursor, &next_doc));
      if (match) {
         ASSERT_MATCH (next_doc, match);
      }
      --expected_count;
   }
   ASSERT (expected_count == 0);
   mongoc_cursor_destroy (cursor);
}

static void
_test_no_docs_match (mongoc_collection_t *coll, const char *selector)
{
   bson_error_t error;
   int64_t ret;

   ret = mongoc_collection_count_with_opts (
      coll, MONGOC_QUERY_NONE, tmp_bson (selector), 0, 0, NULL, NULL, &error);
   ASSERT_OR_PRINT (ret != -1, error);
   ASSERT_CMPINT64 (ret, ==, (int64_t) 0);
}

typedef struct {
   const char *command_under_test;
   int commands_tested;
   const char *expected_command;
} test_crud_ctx_t;

/* Tests that commands match the `expected_command` in the update ctx */
void
_test_crud_command_start (const mongoc_apm_command_started_t *event)
{
   const bson_t *cmd = mongoc_apm_command_started_get_command (event);
   const char *cmd_name = mongoc_apm_command_started_get_command_name (event);

   test_crud_ctx_t *ctx =
      (test_crud_ctx_t *) mongoc_apm_command_started_get_context (event);

   if (!strcmp (cmd_name, ctx->command_under_test)) {
      ctx->commands_tested++;
      ASSERT_MATCH (cmd, ctx->expected_command);
   }
}

static void
test_insert_one (void)
{
   bson_error_t err = {0};
   bson_t reply;
   bson_t opts_with_wc = BSON_INITIALIZER;
   bool ret;
   mongoc_client_t *client = test_framework_client_new ();
   mongoc_database_t *db = get_test_database (client);
   mongoc_collection_t *coll = mongoc_database_get_collection (db, "coll");
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   mongoc_write_concern_t *wc2 = mongoc_write_concern_new ();
   test_crud_ctx_t ctx;
   mongoc_apm_callbacks_t *callbacks = mongoc_apm_callbacks_new ();

   ctx.command_under_test = "insert";
   ctx.commands_tested = 0;

   /* Give wc and wc2 different j values so we can distinguish them but make
    * sure they have w:1 so the writes are committed when we check results */
   mongoc_write_concern_set_w (wc, 1);
   mongoc_write_concern_set_journal (wc, false);
   mongoc_write_concern_set_w (wc2, 1);
   mongoc_write_concern_set_journal (wc2, true);

   mongoc_collection_set_write_concern (coll, wc);
   mongoc_apm_set_command_started_cb (callbacks, _test_crud_command_start);
   mongoc_client_set_apm_callbacks (client, callbacks, &ctx);
   mongoc_collection_drop (coll, NULL);

   /* Test a simple insert with bypassDocumentValidation */
   ctx.expected_command = "{'insert': 'coll', 'bypassDocumentValidation': "
                          "true, 'writeConcern': {'w': 1, 'j': false}}";
   ret = mongoc_collection_insert_one (
      coll,
      tmp_bson ("{'_id': 1}"),
      tmp_bson ("{'bypassDocumentValidation': true}"),
      &reply,
      &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply, "{'insertedCount': 1}");
   bson_destroy (&reply);
   _test_docs_in_coll_matches (coll, tmp_bson ("{'_id': 1}"), NULL, 1);

   /* Test maxTimeMS */
   ctx.expected_command = "{'insert': 'coll', 'maxTimeMS': 9999, "
                          " 'writeConcern': {'w': 1, 'j': false}}";
   ret = mongoc_collection_insert_one (coll,
                                       tmp_bson ("{'_id': 2}"),
                                       tmp_bson ("{'maxTimeMS': 9999}"),
                                       &reply,
                                       &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply, "{'insertedCount': 1}");
   bson_destroy (&reply);
   _test_docs_in_coll_matches (coll, tmp_bson ("{'_id': 2}"), NULL, 1);

   /* Test passing write concern through the options */
   mongoc_write_concern_append (wc2, &opts_with_wc);
   ctx.expected_command =
      "{'insert': 'coll', 'writeConcern': {'w': 1, 'j': true}}";
   ret = mongoc_collection_insert_one (
      coll, tmp_bson ("{'_id': 3}"), &opts_with_wc, &reply, &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply, "{'insertedCount': 1}");
   bson_destroy (&reply);
   _test_docs_in_coll_matches (coll, tmp_bson ("{'_id':3}"), NULL, 1);

   /* Test passing NULL for opts, reply, and error */
   ctx.expected_command =
      "{'insert': 'coll', 'writeConcern': {'w': 1, 'j': false}}";
   ret = mongoc_collection_insert_one (
      coll, tmp_bson ("{'_id': 4}"), NULL, NULL, NULL);
   ASSERT (ret);
   _test_docs_in_coll_matches (coll, tmp_bson ("{'_id': 4}"), NULL, 1);

   /* Duplicate key error */
   ret = mongoc_collection_insert_one (
      coll, tmp_bson ("{'_id': 4}"), NULL, &reply, &err);
   ASSERT (!ret);
   ASSERT_CMPUINT32 (err.domain, ==, (uint32_t) MONGOC_ERROR_COLLECTION);
   ASSERT_MATCH (&reply,
                 "{'insertedCount': 0,"
                 " 'writeErrors': ["
                 "    {'index': 0, 'code': 11000, 'errmsg': {'$exists': true}}"
                 "]}");
   bson_destroy (&reply);
   ASSERT_CMPINT (ctx.commands_tested, ==, 5);

   if (test_framework_is_replset ()) {
      /* Write concern error */
      ctx.expected_command = "{'insert': 'coll',"
                             " 'writeConcern': {'w': 99, 'wtimeout': 100}}";
      ret = mongoc_collection_insert_one (
         coll,
         tmp_bson ("{}"),
         tmp_bson ("{'writeConcern': {'w': 99, 'wtimeout': 100}}"),
         &reply,
         &err);
      ASSERT (!ret);
      ASSERT_CMPUINT32 (err.domain, ==, (uint32_t) MONGOC_ERROR_WRITE_CONCERN);
      ASSERT_MATCH (&reply,
                    "{'insertedCount': 1,"
                    " 'writeErrors': {'$exists': false},"
                    " 'writeConcernErrors': {'$exists': true}"
                    "}");
      bson_destroy (&reply);
   }

   bson_destroy (&opts_with_wc);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_write_concern_destroy (wc);
   mongoc_write_concern_destroy (wc2);
   mongoc_collection_destroy (coll);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}

typedef bool (*update_fn_t) (mongoc_collection_t *,
                             const bson_t *,
                             const bson_t *,
                             const bson_t *,
                             bson_t *,
                             bson_error_t *);

/* Tests `update_one`, `update_many`, and `replace_one` */
static void
_test_update_and_replace (bool is_replace, bool is_multi)
{
   update_fn_t fn = NULL;
   bson_t *update = NULL;
   bson_error_t err = {0};
   bson_t reply;
   bson_t opts_with_wc = BSON_INITIALIZER;
   bson_t opts_with_wc2 = BSON_INITIALIZER;
   bool ret = false;
   mongoc_client_t *client = test_framework_client_new ();
   mongoc_database_t *db = get_test_database (client);
   mongoc_collection_t *coll = mongoc_database_get_collection (db, "coll");
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   mongoc_write_concern_t *wc2 = mongoc_write_concern_new ();
   test_crud_ctx_t ctx;
   mongoc_apm_callbacks_t *callbacks = mongoc_apm_callbacks_new ();

   ctx.command_under_test = "update";
   ctx.commands_tested = 0;

   /* Give wc and wc2 different j values so we can distinguish them but make
    * sure they have w:1 so the writes are committed when we check results */
   mongoc_write_concern_set_w (wc, 1);
   mongoc_write_concern_set_journal (wc, false);
   mongoc_write_concern_append (wc, &opts_with_wc);
   mongoc_collection_set_write_concern (coll, wc);

   mongoc_write_concern_set_w (wc2, 1);
   mongoc_write_concern_set_journal (wc2, true);
   mongoc_write_concern_append (wc2, &opts_with_wc2);

   mongoc_apm_set_command_started_cb (callbacks, _test_crud_command_start);
   mongoc_client_set_apm_callbacks (client, callbacks, &ctx);
   mongoc_collection_drop (coll, NULL);

   /* Test `replace_one`, `update_one` or `update_many` based on args */
   if (is_replace) {
      ASSERT (!is_multi);
      fn = mongoc_collection_replace_one;
   } else {
      fn = is_multi ? mongoc_collection_update_many
                    : mongoc_collection_update_one;
   }

   /* Test a simple update with bypassDocumentValidation */
   ctx.expected_command = "{'update': 'coll', 'bypassDocumentValidation': "
                          "true, 'writeConcern': {'w': 1, 'j': false}}";
   ret = mongoc_collection_insert_one (
      coll, tmp_bson ("{'_id': 1}"), &opts_with_wc, NULL, &err);
   ASSERT_OR_PRINT (ret, err);
   update =
      is_replace ? tmp_bson ("{'a': 1}") : tmp_bson ("{'$set': {'a': 1}}");
   ret = fn (coll,
             tmp_bson ("{}"),
             update,
             tmp_bson ("{'bypassDocumentValidation': true}"),
             &reply,
             &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply,
                 "{'modifiedCount': 1, 'matchedCount': 1, "
                 "'upsertedId': {'$exists': false}}");
   bson_destroy (&reply);
   _test_docs_in_coll_matches (coll, tmp_bson ("{'_id':1}"), "{'a': 1}", 1);

   /* Test passing an upsert */
   ctx.expected_command =
      "{'update': 'coll', 'writeConcern': {'w': 1, 'j': false}}";
   update = is_replace ? tmp_bson ("{'b': 'TEST'}")
                       : tmp_bson ("{'$set': {'b': 'TEST'}}");
   ret = fn (coll,
             tmp_bson ("{'_id': 2}"),
             update,
             tmp_bson ("{'upsert': true}"),
             &reply,
             &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply,
                 "{'modifiedCount': 0, 'matchedCount': 0, "
                 "'upsertedId': {'$exists': true}}");
   bson_destroy (&reply);
   _test_docs_in_coll_matches (
      coll, tmp_bson ("{'_id':2}"), "{'b': 'TEST'}", 1);

   /* Test collation */
   if (test_framework_max_wire_version_at_least (WIRE_VERSION_COLLATION)) {
      update = is_replace ? tmp_bson ("{'b': 'test'}")
                          : tmp_bson ("{'$set': {'b': 'test'}}");
      ret = fn (coll,
                tmp_bson ("{'b': 'TEST'}"),
                update,
                tmp_bson ("{'collation': {'locale': 'en', 'strength': 2}}"),
                &reply,
                &err);
      ASSERT_OR_PRINT (ret, err);
      ASSERT_MATCH (&reply,
                    "{'modifiedCount': 1, 'matchedCount': 1, "
                    "'upsertedId': {'$exists': false}}");
      bson_destroy (&reply);
      _test_docs_in_coll_matches (
         coll, tmp_bson ("{'_id':2}"), "{'b': 'test'}", 1);
   }

   /* Test passing write concern through the options */
   ctx.expected_command =
      "{'update': 'coll', 'writeConcern': {'w': 1, 'j': true}}";
   update =
      is_replace ? tmp_bson ("{'b': 0}") : tmp_bson ("{'$set': {'b': 0}}");
   ret =
      fn (coll, tmp_bson ("{'_id': 2}"), update, &opts_with_wc2, &reply, &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply,
                 "{'modifiedCount': 1, 'matchedCount': 1, "
                 "'upsertedId': {'$exists': false}}");
   bson_destroy (&reply);
   _test_docs_in_coll_matches (coll, tmp_bson ("{'_id':2}"), "{'b': 0}", 1);

   /* Test passing NULL for opts, reply, and error */
   ctx.expected_command =
      "{'update': 'coll', 'writeConcern': {'w': 1, 'j': false}}";
   update =
      is_replace ? tmp_bson ("{'b': 1}") : tmp_bson ("{'$set': {'b': 1}}");
   ret = fn (coll, tmp_bson ("{'_id': 2}"), update, NULL, NULL, NULL);
   ASSERT (ret);
   _test_docs_in_coll_matches (coll, tmp_bson ("{'_id' :2}"), "{'b': 1}", 1);

   /* Test multiple matching documents */
   ret = mongoc_collection_insert_one (
      coll, tmp_bson ("{'_id': 3, 'a': 1}"), &opts_with_wc, NULL, &err);
   ASSERT_OR_PRINT (ret, err);
   ret = mongoc_collection_insert_one (
      coll, tmp_bson ("{'_id': 4, 'a': 1}"), &opts_with_wc, NULL, &err);
   ASSERT_OR_PRINT (ret, err);
   update =
      is_replace ? tmp_bson ("{'a': 2}") : tmp_bson ("{'$set': {'a': 2}}");
   ret = fn (
      coll, tmp_bson ("{'_id': {'$in': [3,4]}}"), update, NULL, &reply, &err);
   ASSERT_OR_PRINT (ret, err);
   if (is_multi) {
      ASSERT_MATCH (&reply,
                    "{'modifiedCount': 2, 'matchedCount': 2, "
                    "'upsertedId': {'$exists': false}}");
      _test_docs_in_coll_matches (
         coll, tmp_bson ("{'_id': {'$in': [3,4]}}"), "{'a': 2}", 2);
   } else {
      ASSERT_MATCH (&reply,
                    "{'modifiedCount': 1, 'matchedCount': 1, "
                    "'upsertedId': {'$exists': false}}");
      /* omit testing collection since not sure which was updated */
   }

   ctx.expected_command = "{'update': 'coll'}";
   ret = fn (coll, tmp_bson ("{'$badOp': 1}"), update, NULL, &reply, &err);
   ASSERT (!ret);
   ASSERT_CMPUINT32 (err.domain, ==, (uint32_t) MONGOC_ERROR_COLLECTION);
   ASSERT_MATCH (&reply,
                 "{'modifiedCount': 0,"
                 " 'matchedCount': 0,"
                 " 'writeErrors': ["
                 "    {'index': 0, 'code': 2, 'errmsg': {'$exists': true}}"
                 "]}");
   bson_destroy (&reply);

   if (test_framework_is_replset ()) {
      ret = fn (coll,
                tmp_bson ("{'_id': 3}"),
                is_replace ? tmp_bson ("{'a': 3}")
                           : tmp_bson ("{'$set': {'a': 3}}"),
                tmp_bson ("{'writeConcern': {'w': 99, 'wtimeout': 100}}"),
                &reply,
                &err);
      ASSERT (!ret);
      ASSERT_CMPUINT32 (err.domain, ==, (uint32_t) MONGOC_ERROR_WRITE_CONCERN);
      ASSERT_MATCH (&reply,
                    "{'modifiedCount': 1,"
                    " 'matchedCount': 1,"
                    " 'writeErrors': {'$exists': false},"
                    " 'writeConcernErrors': {'$exists': true}"
                    "}");
      bson_destroy (&reply);
   }

   /* Test function specific behavior */
   if (is_replace) {
      /* Test that replace really does replace */
      ret =
         mongoc_collection_insert_one (coll,
                                       tmp_bson ("{'_id': 5, 'a': 1, 'b': 2}"),
                                       &opts_with_wc,
                                       NULL,
                                       &err);
      ASSERT_OR_PRINT (ret, err);
      ret = fn (coll,
                tmp_bson ("{'_id': 5}"),
                tmp_bson ("{'a': 2}"),
                NULL,
                &reply,
                &err);
      ASSERT_OR_PRINT (ret, err);
      ASSERT_MATCH (&reply,
                    "{'modifiedCount': 1, 'matchedCount': 1, "
                    "'upsertedId': {'$exists': false}}");
      _test_docs_in_coll_matches (
         coll, tmp_bson ("{'_id': 5}"), "{'a': 2, 'b': {'$exists': false}}", 1);

      /* Test that a non-replace update fails. */
      ret = fn (coll,
                tmp_bson ("{}"),
                tmp_bson ("{'$set': {'a': 1}}"),
                NULL,
                NULL,
                &err);
      ASSERT (!ret);
      ASSERT_ERROR_CONTAINS (err,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "invalid argument for replace");
   } else {
      /* Test arrayFilters if the servers supports it */
      if (test_framework_max_wire_version_at_least (6)) {
         ret = mongoc_collection_insert_one (
            coll,
            tmp_bson ("{'_id': 6, 'a': [{'x':1},{'x':2}]}"),
            &opts_with_wc,
            NULL,
            &err);
         ASSERT_OR_PRINT (ret, err);
         update = tmp_bson ("{'$set': {'a.$[i].x': 3}}");
         ret = fn (coll,
                   tmp_bson ("{'_id': 6}"),
                   update,
                   tmp_bson ("{'arrayFilters': [{'i.x': {'$gt': 1}}]}"),
                   &reply,
                   &err);
         ASSERT_OR_PRINT (ret, err);
         ASSERT_MATCH (&reply,
                       "{'modifiedCount': 1, 'matchedCount': 1, "
                       "'upsertedId': {'$exists': false}}");
         bson_destroy (&reply);
         _test_docs_in_coll_matches (
            coll, tmp_bson ("{'_id':6}"), "{'a': [{'x':1},{'x':3}]}", 1);
      }

      /* Test update that fails */
      ctx.expected_command = "{'update': 'coll'}";
      ret =
         fn (coll, tmp_bson ("{}"), tmp_bson ("{'a': 1}"), NULL, &reply, &err);
      ASSERT (!ret);
      ASSERT_ERROR_CONTAINS (err,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "Invalid key");
      ASSERT (bson_empty (&reply));
      bson_destroy (&reply);
   }

   ASSERT_CMPINT (ctx.commands_tested, >, 0);

   mongoc_apm_callbacks_destroy (callbacks);
   bson_destroy (&opts_with_wc);
   bson_destroy (&opts_with_wc2);
   mongoc_write_concern_destroy (wc);
   mongoc_write_concern_destroy (wc2);
   mongoc_collection_destroy (coll);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}

static void
test_update_and_replace (void)
{
   _test_update_and_replace (false /* is_replace */, false /* is_multi */);
   _test_update_and_replace (true /* is_replace */, false /* is_multi */);
   _test_update_and_replace (false /* is_replace */, true /* is_multi */);
   /* Note, there is no multi replace */
}


static void
_test_update_validate (update_fn_t update_fn)
{
   mongoc_client_t *client;
   mongoc_collection_t *collection;
   bson_t *selector;
   bson_t *update;
   const char *msg;
   bson_error_t error;
   bool r;

   client = test_framework_client_new ();
   mongoc_client_set_error_api (client, 2);
   collection = get_test_collection (client, "test_update_validate");
   selector = tmp_bson ("{}");

   if (update_fn == mongoc_collection_replace_one) {
      /* prohibited for replace */
      update = tmp_bson ("{'$set': {'x': 1}}");
      msg = "invalid argument for replace";
   } else {
      /* prohibited for update */
      update = tmp_bson ("{'x': 1}");
      msg = "only works with $ operators";
   }

   BSON_ASSERT (!update_fn (collection, selector, update, NULL, NULL, &error));
   ASSERT_ERROR_CONTAINS (
      error, MONGOC_ERROR_COMMAND, MONGOC_ERROR_COMMAND_INVALID_ARG, msg);

   r = update_fn (collection,
                  selector,
                  update,
                  tmp_bson ("{'validate': false}"),
                  NULL,
                  &error);

   /* server may or may not error */
   if (!r) {
      ASSERT_CMPUINT32 (error.domain, ==, (uint32_t) MONGOC_ERROR_SERVER);
   }

   BSON_ASSERT (!update_fn (collection,
                            selector,
                            update,
                            tmp_bson ("{'validate': 'foo'}"),
                            NULL,
                            &error));
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_COMMAND,
                          MONGOC_ERROR_COMMAND_INVALID_ARG,
                          "Invalid type for option \"validate\", \"UTF8\"");

   mongoc_collection_destroy (collection);
   mongoc_client_destroy (client);
}


static void
test_replace_one_validate (void)
{
   _test_update_validate (mongoc_collection_replace_one);
}


static void
test_update_one_validate (void)
{
   _test_update_validate (mongoc_collection_update_one);
}


static void
test_update_many_validate (void)
{
   _test_update_validate (mongoc_collection_update_many);
}


typedef bool (*delete_fn_t) (mongoc_collection_t *,
                             const bson_t *,
                             const bson_t *,
                             bson_t *,
                             bson_error_t *);

static void
_test_delete_one_or_many (bool is_multi)
{
   delete_fn_t fn =
      is_multi ? mongoc_collection_delete_many : mongoc_collection_delete_one;
   bson_error_t err = {0};
   bson_t reply;
   bson_t opts_with_wc = BSON_INITIALIZER;
   bool ret;
   mongoc_client_t *client = test_framework_client_new ();
   mongoc_database_t *db = get_test_database (client);
   mongoc_collection_t *coll = mongoc_database_get_collection (db, "coll");
   mongoc_write_concern_t *wc = mongoc_write_concern_new ();
   mongoc_write_concern_t *wc2 = mongoc_write_concern_new ();
   test_crud_ctx_t ctx;
   mongoc_apm_callbacks_t *callbacks = mongoc_apm_callbacks_new ();
   int i;

   ctx.command_under_test = "delete";
   ctx.commands_tested = 0;

   /* Give wc and wc2 different j values so we can distinguish them but make
    * sure they have w:1 so the writes are committed when we check results */
   mongoc_write_concern_set_w (wc, 1);
   mongoc_write_concern_set_journal (wc, false);
   mongoc_write_concern_set_w (wc2, 1);
   mongoc_write_concern_set_journal (wc2, true);

   mongoc_collection_set_write_concern (coll, wc);
   mongoc_apm_set_command_started_cb (callbacks, _test_crud_command_start);
   mongoc_client_set_apm_callbacks (client, callbacks, &ctx);
   mongoc_collection_drop (coll, NULL);

   for (i = 0; i < 3; i++) {
      ret = mongoc_collection_insert_one (
         coll, tmp_bson ("{'_id': %d}", i), NULL, NULL, &err);
      ASSERT_OR_PRINT (ret, err);
   }

   /* Test maxTimeMS */
   ctx.expected_command = "{'delete': 'coll', 'maxTimeMS': 9999, "
                          " 'writeConcern': {'w': 1, 'j': false}}";
   ret = fn (coll,
             tmp_bson ("{'_id': 1}"),
             tmp_bson ("{'maxTimeMS': 9999}"),
             &reply,
             &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply, "{'deletedCount': 1}");
   bson_destroy (&reply);
   _test_no_docs_match (coll, "{'_id': 1}");

   /* Test passing write concern through the options */
   mongoc_write_concern_append (wc2, &opts_with_wc);
   ctx.expected_command =
      "{'delete': 'coll', 'writeConcern': {'w': 1, 'j': true}}";
   ret = fn (coll, tmp_bson ("{'_id': 2}"), &opts_with_wc, &reply, &err);
   ASSERT_OR_PRINT (ret, err);
   ASSERT_MATCH (&reply, "{'deletedCount': 1}");
   bson_destroy (&reply);
   _test_no_docs_match (coll, "{'_id': 2}");

   /* Test passing NULL for opts, reply, and error */
   ctx.expected_command =
      "{'delete': 'coll', 'writeConcern': {'w': 1, 'j': false}}";
   ret = fn (coll, tmp_bson ("{'_id': 3}"), NULL, NULL, NULL);
   ASSERT (ret);
   _test_no_docs_match (coll, "{'_id': 3}");

   /* Server error */
   ret = fn (coll, tmp_bson ("{'_id': {'$foo': 1}}"), NULL, &reply, &err);
   ASSERT (!ret);
   ASSERT_CMPUINT32 (err.domain, ==, (uint32_t) MONGOC_ERROR_COLLECTION);
   ASSERT_MATCH (&reply,
                 "{'deletedCount': 0,"
                 " 'writeErrors': ["
                 "    {'index': 0,"
                 "     'code': {'$exists': true},"
                 "     'errmsg': {'$exists': true}}"
                 "]}");
   bson_destroy (&reply);
   ASSERT_CMPINT (ctx.commands_tested, ==, 4);

   if (test_framework_is_replset ()) {
      /* Write concern error */
      ctx.expected_command = "{'delete': 'coll',"
                             " 'writeConcern': {'w': 99, 'wtimeout': 100}}";
      ret = fn (coll,
                tmp_bson ("{}"),
                tmp_bson ("{'writeConcern': {'w': 99, 'wtimeout': 100}}"),
                &reply,
                &err);
      ASSERT (!ret);
      ASSERT_CMPUINT32 (err.domain, ==, (uint32_t) MONGOC_ERROR_WRITE_CONCERN);
      ASSERT_MATCH (&reply,
                    "{'deletedCount': 1,"
                    " 'writeErrors': {'$exists': false},"
                    " 'writeConcernErrors': {'$exists': true}"
                    "}");
      bson_destroy (&reply);
      ASSERT_CMPINT (ctx.commands_tested, ==, 5);
   }

   bson_destroy (&opts_with_wc);
   mongoc_apm_callbacks_destroy (callbacks);
   mongoc_write_concern_destroy (wc);
   mongoc_write_concern_destroy (wc2);
   mongoc_collection_destroy (coll);
   mongoc_database_destroy (db);
   mongoc_client_destroy (client);
}

static void
test_delete_one_or_many (void)
{
   _test_delete_one_or_many (true);
   _test_delete_one_or_many (false);
}

void
test_collection_install (TestSuite *suite)
{
   test_aggregate_install (suite);

   TestSuite_AddLive (suite,
                      "/Collection/aggregate/write_concern",
                      test_aggregate_w_write_concern);
   TestSuite_AddLive (
      suite, "/Collection/read_prefs_is_valid", test_read_prefs_is_valid);
   TestSuite_AddLive (suite, "/Collection/insert_many", test_insert_many);
   TestSuite_AddLive (
      suite, "/Collection/insert_bulk_empty", test_insert_bulk_empty);
   TestSuite_AddLive (suite, "/Collection/copy", test_copy);
   TestSuite_AddLive (suite, "/Collection/insert", test_insert);
   TestSuite_AddLive (
      suite, "/Collection/insert/null_string", test_insert_null);
   TestSuite_AddFull (suite,
                      "/Collection/insert/oversize",
                      test_insert_oversize,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddMockServerTest (
      suite, "/Collection/insert/keys", test_insert_command_keys);
   TestSuite_AddLive (suite, "/Collection/save", test_save);
   TestSuite_AddLive (suite, "/Collection/insert/w0", test_insert_w0);
   TestSuite_AddLive (suite, "/Collection/update/w0", test_update_w0);
   TestSuite_AddLive (suite, "/Collection/remove/w0", test_remove_w0);
   TestSuite_AddLive (suite, "/Collection/index", test_index);
   TestSuite_AddLive (
      suite, "/Collection/index_w_write_concern", test_index_w_write_concern);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/index/collation/wire4",
                                test_index_with_collation_fail);
   TestSuite_AddMockServerTest (
      suite, "/Collection/index/collation/wire5", test_index_with_collation_ok);
   TestSuite_AddLive (suite, "/Collection/index_compound", test_index_compound);
   TestSuite_AddLive (suite, "/Collection/index_geo", test_index_geo);
   TestSuite_AddLive (suite, "/Collection/index_storage", test_index_storage);
   TestSuite_AddLive (suite, "/Collection/regex", test_regex);
   TestSuite_AddFull (suite,
                      "/Collection/decimal128",
                      test_decimal128,
                      NULL,
                      NULL,
                      skip_unless_server_has_decimal128);
   TestSuite_AddLive (suite, "/Collection/update", test_update);
   TestSuite_AddFull (suite,
                      "/Collection/update/oversize",
                      test_update_oversize,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (suite, "/Collection/remove", test_remove);
   TestSuite_AddFull (suite,
                      "/Collection/remove/oversize",
                      test_remove_oversize,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (suite, "/Collection/count", test_count);
   TestSuite_AddMockServerTest (
      suite, "/Collection/count_with_opts", test_count_with_opts);
   TestSuite_AddMockServerTest (
      suite, "/Collection/count/read_pref", test_count_read_pref);
   TestSuite_AddMockServerTest (
      suite, "/Collection/count/read_concern", test_count_read_concern);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/count/collation/wire4",
                                test_count_with_collation_fail);
   TestSuite_AddMockServerTest (
      suite, "/Collection/count/collation/wire5", test_count_with_collation_ok);
   TestSuite_AddFull (suite,
                      "/Collection/count/read_concern_live",
                      test_count_read_concern_live,
                      NULL,
                      NULL,
                      mongod_supports_majority_read_concern);
   TestSuite_AddLive (suite, "/Collection/drop", test_drop);
   TestSuite_AddLive (suite, "/Collection/aggregate", test_aggregate);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/aggregate/inherit/collection",
                                test_aggregate_inherit_collection);
   TestSuite_AddLive (
      suite, "/Collection/aggregate/large", test_aggregate_large);
   TestSuite_AddLive (
      suite, "/Collection/aggregate/secondary", test_aggregate_secondary);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/aggregate/secondary/sharded",
                                test_aggregate_secondary_sharded);
   TestSuite_AddMockServerTest (
      suite, "/Collection/aggregate/read_concern", test_aggregate_read_concern);
   TestSuite_AddFull (suite,
                      "/Collection/aggregate/bypass_document_validation",
                      test_aggregate_bypass,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_4);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/aggregate/collation/wire4",
                                test_aggregate_with_collation_fail);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/aggregate/collation/wire5",
                                test_aggregate_with_collation_ok);
   TestSuite_AddMockServerTest (
      suite, "/Collection/aggregate_w_server_id", test_aggregate_w_server_id);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/aggregate_w_server_id/sharded",
                                test_aggregate_w_server_id_sharded);
   TestSuite_AddFull (suite,
                      "/Collection/aggregate_w_server_id/option",
                      test_aggregate_server_id_option,
                      NULL,
                      NULL,
                      test_framework_skip_if_auth);
   TestSuite_AddFull (suite,
                      "/Collection/validate",
                      test_validate,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (suite, "/Collection/rename", test_rename);
   TestSuite_AddLive (suite, "/Collection/stats", test_stats);
   TestSuite_AddMockServerTest (
      suite, "/Collection/stats/read_pref", test_stats_read_pref);
   TestSuite_AddMockServerTest (
      suite, "/Collection/find_read_concern", test_find_read_concern);
   TestSuite_AddFull (suite,
                      "/Collection/getmore_read_concern_live",
                      test_getmore_read_concern_live,
                      NULL,
                      NULL,
                      test_framework_skip_if_max_wire_version_less_than_4);
   TestSuite_AddLive (
      suite, "/Collection/find_and_modify", test_find_and_modify);
   TestSuite_AddMockServerTest (suite,
                                "/Collection/find_and_modify/write_concern",
                                test_find_and_modify_write_concern_wire_32);
   TestSuite_AddMockServerTest (
      suite,
      "/Collection/find_and_modify/write_concern_pre_32",
      test_find_and_modify_write_concern_wire_pre_32);
   TestSuite_AddFull (suite,
                      "/Collection/large_return",
                      test_large_return,
                      NULL,
                      NULL,
                      test_framework_skip_if_slow_or_live);
   TestSuite_AddLive (suite, "/Collection/many_return", test_many_return);
   TestSuite_AddLive (
      suite, "/Collection/insert_one_validate", test_insert_one_validate);
   TestSuite_AddLive (
      suite, "/Collection/insert_many_validate", test_insert_many_validate);
   TestSuite_AddMockServerTest (suite, "/Collection/limit", test_find_limit);
   TestSuite_AddMockServerTest (
      suite, "/Collection/batch_size", test_find_batch_size);
   TestSuite_AddFull (suite,
                      "/Collection/command_fully_qualified",
                      test_command_fq,
                      NULL,
                      NULL,
                      test_framework_skip_if_mongos);
   TestSuite_AddLive (suite, "/Collection/get_index_info", test_get_index_info);
   TestSuite_AddMockServerTest (
      suite, "/Collection/find_indexes/error", test_find_indexes_err);
   TestSuite_AddLive (
      suite, "/Collection/insert/duplicate_key", test_insert_duplicate_key);
   TestSuite_AddFull (suite,
                      "/Collection/create_index/fail",
                      test_create_index_fail,
                      NULL,
                      NULL,
                      test_framework_skip_if_offline);
   TestSuite_AddLive (suite, "/Collection/insert_one", test_insert_one);
   TestSuite_AddLive (
      suite, "/Collection/update_and_replace", test_update_and_replace);
   TestSuite_AddLive (
      suite, "/Collection/replace_one_validate", test_replace_one_validate);
   TestSuite_AddLive (
      suite, "/Collection/update_one_validate", test_update_one_validate);
   TestSuite_AddLive (
      suite, "/Collection/update_many_validate", test_update_many_validate);
   TestSuite_AddLive (
      suite, "/Collection/delete_one_or_many", test_delete_one_or_many);
}
