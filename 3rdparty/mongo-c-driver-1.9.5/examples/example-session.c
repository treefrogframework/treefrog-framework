/* gcc example-session.c -o example-session \
 *     $(pkg-config --cflags --libs libmongoc-1.0) */

/* ./example-session [CONNECTION_STRING] */

#include <stdio.h>
#include <mongoc.h>


int
main (int argc, char *argv[])
{
   int exit_code = EXIT_FAILURE;

   mongoc_client_t *client;
   mongoc_client_session_t *client_session = NULL;
   mongoc_collection_t *collection = NULL;
   const char *uristr = "mongodb://127.0.0.1/?appname=session-example";
   bson_error_t error;
   bson_t *selector = NULL;
   bson_t *update = NULL;
   bson_t *update_opts = NULL;
   bson_t *find_opts = NULL;
   mongoc_read_prefs_t *secondary = NULL;
   mongoc_cursor_t *cursor = NULL;
   const bson_t *doc;
   char *str;
   bool r;

   mongoc_init ();

   if (argc > 1) {
      uristr = argv[1];
   }

   client = mongoc_client_new (uristr);

   if (!client) {
      fprintf (stderr, "Failed to parse URI.\n");
      goto done;
   }

   mongoc_client_set_error_api (client, 2);

   /* pass NULL for options - by default the session is causally consistent */
   client_session = mongoc_client_start_session (client, NULL, &error);
   if (!client_session) {
      fprintf (stderr, "Failed to start session: %s\n", error.message);
      goto done;
   }

   collection = mongoc_client_get_collection (client, "test", "collection");
   selector = BCON_NEW ("_id", BCON_INT32 (1));
   update = BCON_NEW ("$inc", "{", "x", BCON_INT32 (1), "}");
   update_opts = bson_new ();
   if (!mongoc_client_session_append (client_session, update_opts, &error)) {
      fprintf (stderr, "Could not add session to opts: %s\n", error.message);
      goto done;
   }

   r = mongoc_collection_update_one (
      collection, selector, update, update_opts, NULL /* reply */, &error);

   if (!r) {
      fprintf (stderr, "Update failed: %s\n", error.message);
      goto done;
   }

   bson_destroy (selector);
   selector = BCON_NEW ("_id", BCON_INT32 (1));
   secondary = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);

   find_opts = BCON_NEW ("maxTimeMS", BCON_INT32 (2000));
   if (!mongoc_client_session_append (client_session, find_opts, &error)) {
      fprintf (stderr, "Could not add session to opts: %s\n", error.message);
      goto done;
   };

   /* read from secondary. since we're in a causally consistent session, the
    * data is guaranteed to reflect the update we did on the primary. the query
    * blocks waiting for the secondary to catch up, if necessary, or times out
    * and fails after 2000 ms.
    */
   cursor = mongoc_collection_find_with_opts (
      collection, selector, find_opts, secondary);

   bson_destroy (selector);
   mongoc_read_prefs_destroy (secondary);
   bson_destroy (find_opts);

   while (mongoc_cursor_next (cursor, &doc)) {
      str = bson_as_json (doc, NULL);
      fprintf (stdout, "%s\n", str);
      bson_free (str);
   }

   if (mongoc_cursor_error (cursor, &error)) {
      fprintf (stderr, "Cursor Failure: %s\n", error.message);
      goto done;
   }

   exit_code = EXIT_SUCCESS;

done:
   if (find_opts) {
      bson_destroy (find_opts);
   }
   if (update) {
      bson_destroy (update);
   }
   if (selector) {
      bson_destroy (selector);
   }
   if (update_opts) {
      bson_destroy (update_opts);
   }
   if (secondary) {
      mongoc_read_prefs_destroy (secondary);
   }
   /* destroy cursor, collection, session before the client they came from */
   if (cursor) {
      mongoc_cursor_destroy (cursor);
   }
   if (collection) {
      mongoc_collection_destroy (collection);
   }
   if (client_session) {
      mongoc_client_session_destroy (client_session);
   }
   if (client) {
      mongoc_client_destroy (client);
   }

   mongoc_cleanup ();

   return exit_code;
}
