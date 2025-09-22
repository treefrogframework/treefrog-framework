#include <bson/bson.h>

#include <stdio.h>

#ifndef EXPECT_BSON_VERSION
#error This file requires EXPECT_BSON_VERSION to be defined
#define EXPECT_BSON_VERSION ""
#endif

#if EXPECT_BSON_STATIC != defined(BSON_STATIC)
#error Static/dynamic mismatch
#endif

int
main (void)
{
   if (strcmp (BSON_VERSION_S, EXPECT_BSON_VERSION)) {
      fprintf (
         stderr, "Wrong BSON_MAJOR_VERSION found (Expected “%s”, but got “%s”)", EXPECT_BSON_VERSION, BSON_VERSION_S);
      return 2;
   }
   return 0;
}
