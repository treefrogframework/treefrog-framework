#include "client-side-encryption-helpers.h"

uint8_t *
hex_to_bin (const char *hex, uint32_t *len)
{
   int i;
   int hex_len;
   uint8_t *out;

   hex_len = strlen (hex);
   if (hex_len % 2 != 0) {
      return NULL;
   }

   *len = hex_len / 2;
   out = bson_malloc0 (*len);

   for (i = 0; i < hex_len; i += 2) {
      uint32_t hex_char;

      if (1 != sscanf (hex + i, "%2x", &hex_char)) {
         bson_free (out);
         return NULL;
      }
      out[i / 2] = (uint8_t) hex_char;
   }
   return out;
}

bool
print_one_document (mongoc_collection_t *coll, bson_error_t *error)
{
   bool ret = false;
   mongoc_cursor_t *cursor = NULL;
   const bson_t *found;
   bson_t *filter = NULL;
   char *as_string = NULL;

   filter = bson_new ();
   cursor = mongoc_collection_find_with_opts (
      coll, filter, NULL /* opts  */, NULL /* read prefs */);
   if (!mongoc_cursor_next (cursor, &found)) {
      fprintf (stderr, "error: did not find inserted document\n");
      goto fail;
   }
   if (mongoc_cursor_error (cursor, error)) {
      goto fail;
   }
   as_string = bson_as_canonical_extended_json (found, NULL);
   printf ("%s", as_string);

   ret = true;
fail:
   bson_destroy (filter);
   mongoc_cursor_destroy (cursor);
   bson_free (as_string);
   return ret;
}
