#include <bson/bson.h>

#include "TestSuite.h"


static void
test_get_monotonic_time (void)
{
   int64_t t;
   int64_t t2;

   t = bson_get_monotonic_time ();
   t2 = bson_get_monotonic_time ();
   BSON_ASSERT (t);
   BSON_ASSERT (t2);
   ASSERT_CMPINT64 (t, <=, t2);
}


void
test_clock_install (TestSuite *suite)
{
   TestSuite_Add (
      suite, "/bson/clock/get_monotonic_time", test_get_monotonic_time);
}
