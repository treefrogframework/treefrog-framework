// example-bulkwrite shows use of `mongoc_client_bulkwrite`.

#include <mongoc/mongoc.h>

#define HANDLE_ERROR(...)            \
   if (1) {                          \
      fprintf (stderr, __VA_ARGS__); \
      fprintf (stderr, "\n");        \
      goto fail;                     \
   } else                            \
      (void) 0

int
main (void)
{
   bool ok = false;

   mongoc_init ();

   bson_error_t error;
   mongoc_client_t *client = mongoc_client_new ("mongodb://localhost:27017");
   mongoc_bulkwriteopts_t *bwo = mongoc_bulkwriteopts_new ();
   mongoc_bulkwriteopts_set_verboseresults (bwo, true);
   mongoc_bulkwrite_t *bw = mongoc_client_bulkwrite_new (client);

   // Insert a document to `db.coll1`
   {
      bson_t *doc = BCON_NEW ("foo", "bar");
      if (!mongoc_bulkwrite_append_insertone (bw, "db.coll1", doc, NULL, &error)) {
         HANDLE_ERROR ("Error appending insert one: %s", error.message);
      }
      bson_destroy (doc);
   }
   // Insert a document to `db.coll2`
   {
      bson_t *doc = BCON_NEW ("foo", "baz");
      if (!mongoc_bulkwrite_append_insertone (bw, "db.coll2", doc, NULL, &error)) {
         HANDLE_ERROR ("Error appending insert one: %s", error.message);
      }
      bson_destroy (doc);
   }

   mongoc_bulkwritereturn_t bwr = mongoc_bulkwrite_execute (bw, bwo);

   // Print results.
   {
      BSON_ASSERT (bwr.res); // Has results. NULL only returned for unacknowledged writes.
      printf ("Insert count          : %" PRId64 "\n", mongoc_bulkwriteresult_insertedcount (bwr.res));
      const bson_t *ir = mongoc_bulkwriteresult_insertresults (bwr.res);
      BSON_ASSERT (ir); // Has verbose results. NULL only returned if verbose results not requested.
      char *ir_str = bson_as_relaxed_extended_json (ir, NULL);
      printf ("Insert results        : %s\n", ir_str);
      bson_free (ir_str);
   }

   // Print all error information. To observe: try setting the `_id` fields to cause a duplicate key error.
   if (bwr.exc) {
      const char *msg = "(none)";
      if (mongoc_bulkwriteexception_error (bwr.exc, &error)) {
         msg = error.message;
      }
      const bson_t *we = mongoc_bulkwriteexception_writeerrors (bwr.exc);
      char *we_str = bson_as_relaxed_extended_json (we, NULL);
      const bson_t *wce = mongoc_bulkwriteexception_writeconcernerrors (bwr.exc);
      char *wce_str = bson_as_relaxed_extended_json (wce, NULL);
      const bson_t *er = mongoc_bulkwriteexception_errorreply (bwr.exc);
      char *er_str = bson_as_relaxed_extended_json (er, NULL);
      printf ("Top-level error       : %s\n", msg);
      printf ("Write errors          : %s\n", we_str);
      printf ("Write concern errors  : %s\n", wce_str);
      printf ("Error reply           : %s\n", er_str);
      bson_free (er_str);
      bson_free (wce_str);
      bson_free (we_str);
   }

   mongoc_bulkwriteresult_destroy (bwr.res);
   mongoc_bulkwriteexception_destroy (bwr.exc);
   mongoc_bulkwrite_destroy (bw);

   ok = true;
fail:
   mongoc_client_destroy (client);
   mongoc_bulkwriteopts_destroy (bwo);
   mongoc_cleanup ();
   return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
