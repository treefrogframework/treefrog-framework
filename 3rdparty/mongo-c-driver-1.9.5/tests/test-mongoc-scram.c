#include <mongoc.h>

#include "mongoc-scram-private.h"

#include "TestSuite.h"

#ifdef MONGOC_ENABLE_SSL
static void
test_mongoc_scram_step_username_not_set (void)
{
   mongoc_scram_t scram;
   bool success;
   uint8_t buf[4096] = {0};
   uint32_t buflen = 0;
   bson_error_t error;

   _mongoc_scram_init (&scram);
   _mongoc_scram_set_pass (&scram, "password");

   success = _mongoc_scram_step (
      &scram, buf, buflen, buf, sizeof buf, &buflen, &error);

   ASSERT (!success);
   ASSERT_ERROR_CONTAINS (error,
                          MONGOC_ERROR_SCRAM,
                          MONGOC_ERROR_SCRAM_PROTOCOL_ERROR,
                          "SCRAM Failure: username is not set");

   _mongoc_scram_destroy (&scram);
}
#endif


void
test_scram_install (TestSuite *suite)
{
#ifdef MONGOC_ENABLE_SSL
   TestSuite_Add (suite,
                  "/scram/username_not_set",
                  test_mongoc_scram_step_username_not_set);
#endif
}
