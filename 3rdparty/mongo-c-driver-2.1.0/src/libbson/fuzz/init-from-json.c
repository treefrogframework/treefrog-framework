#include <bson/bson.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int
LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
   bson_error_t error;

   bson_t b;
   if (bson_init_from_json (&b, (const char *) data, size, &error)) {
      bson_destroy (&b);
   }

   return 0;
}
