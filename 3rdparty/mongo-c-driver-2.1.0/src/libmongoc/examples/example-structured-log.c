/* gcc example-structured-log.c -o example-structured-log \
 *     $(pkg-config --cflags --libs libmongoc-1.0) */

#include <mongoc/mongoc.h>

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t handler_mutex;

static void
example_handler (const mongoc_structured_log_entry_t *entry, void *user_data)
{
   (void) user_data;

   mongoc_structured_log_component_t component = mongoc_structured_log_entry_get_component (entry);
   mongoc_structured_log_level_t level = mongoc_structured_log_entry_get_level (entry);
   const char *message_string = mongoc_structured_log_entry_get_message_string (entry);

   /*
    * With a single-threaded mongoc_client_t, handlers will always be called
    * by the thread that owns the client. On a mongoc_client_pool_t, handlers
    * are shared by multiple threads and must be reentrant.
    *
    * Note that unstructured logging includes a global mutex in the API,
    * but structured logging allows applications to avoid lock contention
    * even when multiple threads are issuing commands simultaneously.
    *
    * Simple apps like this example can achieve thread safety by adding their
    * own global mutex. For other apps, this would be a performance bottleneck
    * and it would be more appropriate for handlers to process their log
    * messages concurrently.
    *
    * In this example, our mutex protects access to a global log counter.
    * In a real application, you may need to protect access to a shared stream
    * or queue.
    */
   pthread_mutex_lock (&handler_mutex);

   static unsigned log_serial_number = 0;

   printf ("%u. Log entry with component=%s level=%s message_string='%s'\n",
           ++log_serial_number,
           mongoc_structured_log_get_component_name (component),
           mongoc_structured_log_get_level_name (level),
           message_string);

   /*
    * At this point, the handler might make additional filtering decisions
    * before asking for a bson_t. As an example, let's log the component and
    * level for all messages but only show contents for command logs.
    */
   if (component == MONGOC_STRUCTURED_LOG_COMPONENT_COMMAND) {
      bson_t *message = mongoc_structured_log_entry_message_as_bson (entry);
      char *json = bson_as_relaxed_extended_json (message, NULL);
      printf ("Full log message, as json: %s\n", json);
      bson_destroy (message);
      bson_free (json);
   }

   pthread_mutex_unlock (&handler_mutex);
}

int
main (void)
{
   const char *uri_string = "mongodb://localhost:27017";
   int result = EXIT_FAILURE;
   bson_error_t error;
   mongoc_uri_t *uri = NULL;
   mongoc_structured_log_opts_t *log_opts = NULL;
   mongoc_client_t *client = NULL;
   mongoc_client_pool_t *pool = NULL;

   /*
    * Note that structured logging only applies per-client or per-pool,
    * and it won't be used during or before mongoc_init.
    */
   mongoc_init ();

   /*
    * Logging options are represented by a mongoc_structured_log_opts_t,
    * which can be copied into a mongoc_client_t or mongoc_client_pool_t
    * using mongoc_client_set_structured_log_opts() or
    * mongoc_client_pool_set_structured_log_opts(), respectively.
    *
    * Default settings are captured from the environment into
    * this structure when it's constructed.
    */
   log_opts = mongoc_structured_log_opts_new ();

   /*
    * For demonstration purposes, set up a handler that receives all possible log messages.
    */
   pthread_mutex_init (&handler_mutex, NULL);
   mongoc_structured_log_opts_set_max_level_for_all_components (log_opts, MONGOC_STRUCTURED_LOG_LEVEL_TRACE);
   mongoc_structured_log_opts_set_handler (log_opts, example_handler, NULL);

   /*
    * By default libmongoc proceses log options from the environment first,
    * and then allows you to apply programmatic overrides. To request the
    * opposite behavior, allowing the environment to override programmatic
    * defaults, you can ask for the environment to be re-read after setting
    * your own defaults.
    */
   mongoc_structured_log_opts_set_max_levels_from_env (log_opts);

   /*
    * Create a MongoDB URI object. This example assumes a local server.
    */
   uri = mongoc_uri_new_with_error (uri_string, &error);
   if (!uri) {
      fprintf (stderr, "URI parse error: %s\n", error.message);
      goto done;
   }

   /*
    * Create a new client pool.
    */
   pool = mongoc_client_pool_new (uri);
   if (!pool) {
      goto done;
   }

   /*
    * Set the client pool's log options.
    * This must happen only once, and only before the first mongoc_client_pool_pop.
    * There's no need to keep log_opts after this point.
    */
   mongoc_client_pool_set_structured_log_opts (pool, log_opts);

   /*
    * Check out a client, and do some work that we'll see logs from.
    * This example just sends a 'ping' command.
    */
   client = mongoc_client_pool_pop (pool);
   if (!client) {
      goto done;
   }

   bson_t *command = BCON_NEW ("ping", BCON_INT32 (1));
   bson_t reply;
   bool command_ret = mongoc_client_command_simple (client, "admin", command, NULL, &reply, &error);
   bson_destroy (command);
   bson_destroy (&reply);
   mongoc_client_pool_push (pool, client);
   if (!command_ret) {
      fprintf (stderr, "Command error: %s\n", error.message);
      goto done;
   }

   result = EXIT_SUCCESS;
done:
   mongoc_uri_destroy (uri);
   mongoc_structured_log_opts_destroy (log_opts);
   mongoc_client_pool_destroy (pool);
   mongoc_cleanup ();
   return result;
}
