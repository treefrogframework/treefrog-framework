#include <bson/bson.h>

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
   // Reject inputs with lengths greater than ssize_t
   if (size > (size_t) SSIZE_MAX) {
      return -1;
   }

   // Check if input crashes program (ok if return NULL)
   char *got = bson_utf8_escape_for_json ((const char *) data, (ssize_t) size);

   // If UTF-8 is valid, we should get some non-null response
   if (bson_utf8_validate ((const char *) data, size, true)) {
      BSON_ASSERT (got);
   }

   bson_free (got);
   return 0;
}
