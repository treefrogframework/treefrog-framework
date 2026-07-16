#include <mongoc/mongoc.h>

#include <stdio.h>

#ifndef EXPECT_MONGOC_VERSION
#error This file requires EXPECT_MONGOC_VERSION to be defined
#define EXPECT_MONGOC_VERSION ""
#endif

int
main(void)
{
   if (strcmp(MONGOC_VERSION_S, EXPECT_MONGOC_VERSION)) {
      fprintf(stderr,
              "Wrong MONGOC_MAJOR_VERSION found (Expected “%s”, but got “%s”)",
              EXPECT_MONGOC_VERSION,
              MONGOC_VERSION_S);
      return 2;
   }
   return 0;
}
