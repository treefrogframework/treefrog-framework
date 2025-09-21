bool
map_reduce_advanced (mongoc_database_t *database)
{
   bson_t *command;
   bson_error_t error;
   bool res = true;
   mongoc_read_prefs_t *read_pref;
   bson_t doc;

   /* Construct the mapReduce command */
   /* Other arguments can also be specified here, like "query" or "limit"
      and so on */

   /* Read the results inline from a secondary replica */
   command = BCON_NEW ("mapReduce",
                       BCON_UTF8 (COLLECTION_NAME),
                       "map",
                       BCON_CODE (MAPPER),
                       "reduce",
                       BCON_CODE (REDUCER),
                       "out",
                       "{",
                       "inline",
                       BCON_INT32 (1),
                       "}");

   read_pref = mongoc_read_prefs_new (MONGOC_READ_SECONDARY);
   if (mongoc_database_command_simple (database, command, read_pref, &doc, &error)) {
      print_res (&doc);
   } else {
      fprintf (stderr, "ERROR: %s\n", error.message);
      res = false;
   }

   mongoc_read_prefs_destroy (read_pref);
   bson_destroy (command);
   bson_destroy (&doc);

   return res;
}
