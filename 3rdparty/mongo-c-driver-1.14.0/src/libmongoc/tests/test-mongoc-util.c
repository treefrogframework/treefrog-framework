#include <mongoc/mongoc.h>
#include <mongoc/mongoc-util-private.h>

#include "TestSuite.h"
#include "test-conveniences.h"

#undef MONGOC_LOG_DOMAIN
#define MONGOC_LOG_DOMAIN "test-util"


static void
test_command_name (void)
{
   bson_t *commands[] = {
      tmp_bson ("{'foo': 1}"),
      tmp_bson ("{'query': {'foo': 1}}"),
      tmp_bson ("{'query': {'foo': 1}, '$readPreference': 1}"),
      tmp_bson ("{'$query': {'foo': 1}}"),
      tmp_bson ("{'$query': {'foo': 1}, '$readPreference': 1}"),
      tmp_bson ("{'$readPreference': 1, '$query': {'foo': 1}}"),
   };

   size_t i;

   for (i = 0; i < sizeof (commands) / sizeof (bson_t *); i++) {
      ASSERT_CMPSTR ("foo", _mongoc_get_command_name (commands[i]));
   }
}


static void
test_rand_simple (void)
{
   int i;
   unsigned int seed = 0;
   int value, first_value;

   first_value = _mongoc_rand_simple (&seed);

   for (i = 0; i < 1000; i++) {
      value = _mongoc_rand_simple (&seed);
      if (value != first_value) {
         /* success */
         break;
      }
   }
}

static void
test_lowercase_utf8 (void)
{
   char *snowman = "\xE2\x9b\x84";
   char *letters = "aBcDe";
   char *buf = bson_malloc0 (strlen (snowman) + 1);

   mongoc_lowercase (snowman, buf);
   ASSERT_CMPSTR (snowman, buf);
   bson_free (buf);

   buf = bson_malloc0 (strlen (letters) + 1);
   mongoc_lowercase (letters, buf);
   ASSERT_CMPSTR ("abcde", buf);
   bson_free (buf);
}


void
test_util_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/Util/command_name", test_command_name);
   TestSuite_Add (suite, "/Util/rand_simple", test_rand_simple);
   TestSuite_Add (suite, "/Util/lowercase_utf8", test_lowercase_utf8);
}
