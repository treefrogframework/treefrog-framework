#include <mongoc.h>
#include <mongoc-write-concern-private.h>
#include <mongoc-util-private.h>

#include "TestSuite.h"
#include "test-conveniences.h"
#include "test-libmongoc.h"


static void
test_write_concern_append (void)
{
   mongoc_write_concern_t *wc;
   bson_t *cmd;

   cmd = tmp_bson ("{'foo': 1}");
   capture_logs (true);

   /* cannot append invalid writeConcern */
   wc = NULL;
   BSON_ASSERT (!mongoc_write_concern_append (wc, cmd));

   /* append default writeConcern */
   wc = mongoc_write_concern_new ();
   ASSERT (mongoc_write_concern_is_default (wc));
   ASSERT_MATCH (cmd, "{'foo': 1, 'writeConcern': {'$exists': false}}");

   /* append writeConcern with w */
   mongoc_write_concern_set_w (wc, 1);
   BSON_ASSERT (mongoc_write_concern_append (wc, cmd));

   ASSERT (match_bson (
      cmd, tmp_bson ("{'foo': 1, 'writeConcern': {'w': 1}}"), true));

   mongoc_write_concern_destroy (wc);
}

static void
test_write_concern_basic (void)
{
   mongoc_write_concern_t *write_concern;
   const bson_t *bson;
   bson_iter_t iter;

   write_concern = mongoc_write_concern_new ();

   BEGIN_IGNORE_DEPRECATIONS;

   /*
    * Test defaults.
    */
   ASSERT (write_concern);
   ASSERT (!mongoc_write_concern_get_fsync (write_concern));
   ASSERT (!mongoc_write_concern_get_journal (write_concern));
   ASSERT (mongoc_write_concern_get_w (write_concern) ==
           MONGOC_WRITE_CONCERN_W_DEFAULT);
   ASSERT (!mongoc_write_concern_get_wtimeout (write_concern));
   ASSERT (!mongoc_write_concern_get_wmajority (write_concern));

   mongoc_write_concern_set_fsync (write_concern, true);
   ASSERT (mongoc_write_concern_get_fsync (write_concern));
   mongoc_write_concern_set_fsync (write_concern, false);
   ASSERT (!mongoc_write_concern_get_fsync (write_concern));

   mongoc_write_concern_set_journal (write_concern, true);
   ASSERT (mongoc_write_concern_get_journal (write_concern));
   mongoc_write_concern_set_journal (write_concern, false);
   ASSERT (!mongoc_write_concern_get_journal (write_concern));

   /*
    * Test changes to w.
    */
   mongoc_write_concern_set_w (write_concern, MONGOC_WRITE_CONCERN_W_MAJORITY);
   ASSERT (mongoc_write_concern_get_wmajority (write_concern));
   mongoc_write_concern_set_w (write_concern, MONGOC_WRITE_CONCERN_W_DEFAULT);
   ASSERT (!mongoc_write_concern_get_wmajority (write_concern));
   mongoc_write_concern_set_wmajority (write_concern, 1000);
   ASSERT (mongoc_write_concern_get_wmajority (write_concern));
   ASSERT (mongoc_write_concern_get_wtimeout (write_concern) == 1000);
   mongoc_write_concern_set_wtimeout (write_concern, 0);
   ASSERT (!mongoc_write_concern_get_wtimeout (write_concern));
   mongoc_write_concern_set_w (write_concern, MONGOC_WRITE_CONCERN_W_DEFAULT);
   ASSERT (mongoc_write_concern_get_w (write_concern) ==
           MONGOC_WRITE_CONCERN_W_DEFAULT);
   mongoc_write_concern_set_w (write_concern, 3);
   ASSERT (mongoc_write_concern_get_w (write_concern) == 3);

   /*
    * Check generated bson.
    */
   mongoc_write_concern_set_fsync (write_concern, true);
   mongoc_write_concern_set_journal (write_concern, true);

   bson = _mongoc_write_concern_get_bson (write_concern);
   ASSERT (bson);
   ASSERT (bson_iter_init_find (&iter, bson, "fsync") &&
           BSON_ITER_HOLDS_BOOL (&iter) && bson_iter_bool (&iter));
   ASSERT (bson_iter_init_find (&iter, bson, "j") &&
           BSON_ITER_HOLDS_BOOL (&iter) && bson_iter_bool (&iter));
   ASSERT (bson_iter_init_find (&iter, bson, "w") &&
           BSON_ITER_HOLDS_INT32 (&iter) && bson_iter_int32 (&iter) == 3);

   mongoc_write_concern_destroy (write_concern);

   END_IGNORE_DEPRECATIONS;
}


static void
test_write_concern_bson_omits_defaults (void)
{
   mongoc_write_concern_t *write_concern;
   const bson_t *bson;
   bson_iter_t iter;

   write_concern = mongoc_write_concern_new ();

   /*
    * Check generated bson.
    */
   ASSERT (write_concern);

   bson = _mongoc_write_concern_get_bson (write_concern);
   ASSERT (bson);
   ASSERT (!bson_iter_init_find (&iter, bson, "fsync"));
   ASSERT (!bson_iter_init_find (&iter, bson, "j"));

   mongoc_write_concern_destroy (write_concern);
}


static void
test_write_concern_bson_includes_false_fsync_and_journal (void)
{
   mongoc_write_concern_t *write_concern;
   const bson_t *bson;
   bson_iter_t iter;

   write_concern = mongoc_write_concern_new ();

   /*
    * Check generated bson.
    */
   ASSERT (write_concern);
   mongoc_write_concern_set_fsync (write_concern, false);
   mongoc_write_concern_set_journal (write_concern, false);

   bson = _mongoc_write_concern_get_bson (write_concern);
   ASSERT (bson);
   ASSERT (bson_iter_init_find (&iter, bson, "fsync") &&
           BSON_ITER_HOLDS_BOOL (&iter) && !bson_iter_bool (&iter));
   ASSERT (bson_iter_init_find (&iter, bson, "j") &&
           BSON_ITER_HOLDS_BOOL (&iter) && !bson_iter_bool (&iter));
   ASSERT (!bson_iter_init_find (&iter, bson, "w"));

   mongoc_write_concern_destroy (write_concern);
}


static void
test_write_concern_fsync_and_journal_w1_and_validity (void)
{
   mongoc_write_concern_t *write_concern = mongoc_write_concern_new ();

   /*
    * Journal and fsync should imply w=1 regardless of w; however, journal and
    * fsync logically conflict with w=0 and w=-1, so a write concern with such
    * a combination of options will be considered invalid.
    */

   /* No write concern needs GLE, but not "valid" */
   ASSERT (mongoc_write_concern_is_acknowledged (NULL));
   ASSERT (!mongoc_write_concern_is_valid (NULL));

   /* Default write concern needs GLE and is valid */
   ASSERT (write_concern);
   ASSERT (mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (mongoc_write_concern_is_valid (write_concern));
   ASSERT (!mongoc_write_concern_journal_is_set (write_concern));

   /* w=0 does not need GLE and is valid */
   mongoc_write_concern_set_w (write_concern,
                               MONGOC_WRITE_CONCERN_W_UNACKNOWLEDGED);
   ASSERT (!mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (mongoc_write_concern_is_valid (write_concern));
   ASSERT (!mongoc_write_concern_journal_is_set (write_concern));

   /* fsync=true needs GLE, but it conflicts with w=0 */
   mongoc_write_concern_set_fsync (write_concern, true);
   ASSERT (mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (!mongoc_write_concern_is_valid (write_concern));
   ASSERT (!mongoc_write_concern_journal_is_set (write_concern));
   mongoc_write_concern_set_fsync (write_concern, false);

   /* journal=true needs GLE, but it conflicts with w=0 */
   mongoc_write_concern_set_journal (write_concern, true);
   ASSERT (mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (!mongoc_write_concern_is_valid (write_concern));
   ASSERT (mongoc_write_concern_journal_is_set (write_concern));
   mongoc_write_concern_set_journal (write_concern, false);

   /* w=-1 does not need GLE and is valid */
   mongoc_write_concern_set_w (write_concern,
                               MONGOC_WRITE_CONCERN_W_ERRORS_IGNORED);
   ASSERT (!mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (mongoc_write_concern_is_valid (write_concern));
   ASSERT (mongoc_write_concern_journal_is_set (write_concern));

   /* fsync=true needs GLE, but it conflicts with w=-1 */
   mongoc_write_concern_set_fsync (write_concern, true);
   ASSERT (mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (!mongoc_write_concern_is_valid (write_concern));
   ASSERT (mongoc_write_concern_journal_is_set (write_concern));

   /* journal=true needs GLE, but it conflicts with w=-1 */
   mongoc_write_concern_set_fsync (write_concern, false);
   mongoc_write_concern_set_journal (write_concern, true);
   ASSERT (mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (mongoc_write_concern_journal_is_set (write_concern));

   /* fsync=true with w=default needs GLE and is valid */
   mongoc_write_concern_set_journal (write_concern, false);
   mongoc_write_concern_set_fsync (write_concern, true);
   mongoc_write_concern_set_w (write_concern, MONGOC_WRITE_CONCERN_W_DEFAULT);
   ASSERT (mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (mongoc_write_concern_is_valid (write_concern));
   ASSERT (mongoc_write_concern_journal_is_set (write_concern));

   /* journal=true with w=default needs GLE and is valid */
   mongoc_write_concern_set_journal (write_concern, false);
   mongoc_write_concern_set_fsync (write_concern, true);
   mongoc_write_concern_set_w (write_concern, MONGOC_WRITE_CONCERN_W_DEFAULT);
   ASSERT (mongoc_write_concern_is_acknowledged (write_concern));
   ASSERT (mongoc_write_concern_is_valid (write_concern));
   ASSERT (mongoc_write_concern_journal_is_set (write_concern));

   mongoc_write_concern_destroy (write_concern);
}

static void
test_write_concern_wtimeout_validity (void)
{
   mongoc_write_concern_t *write_concern = mongoc_write_concern_new ();

   /* Test defaults */
   ASSERT (write_concern);
   ASSERT (mongoc_write_concern_get_w (write_concern) ==
           MONGOC_WRITE_CONCERN_W_DEFAULT);
   ASSERT (mongoc_write_concern_get_wtimeout (write_concern) == 0);
   ASSERT (!mongoc_write_concern_get_wmajority (write_concern));

   /* mongoc_write_concern_set_wtimeout() ignores invalid wtimeout */
   mongoc_write_concern_set_wtimeout (write_concern, -1);
   ASSERT (mongoc_write_concern_get_w (write_concern) ==
           MONGOC_WRITE_CONCERN_W_DEFAULT);
   ASSERT (mongoc_write_concern_get_wtimeout (write_concern) == 0);
   ASSERT (!mongoc_write_concern_get_wmajority (write_concern));
   ASSERT (mongoc_write_concern_is_valid (write_concern));

   /* mongoc_write_concern_set_wmajority() ignores invalid wtimeout */
   mongoc_write_concern_set_wmajority (write_concern, -1);
   ASSERT (mongoc_write_concern_get_w (write_concern) ==
           MONGOC_WRITE_CONCERN_W_MAJORITY);
   ASSERT (mongoc_write_concern_get_wtimeout (write_concern) == 0);
   ASSERT (mongoc_write_concern_get_wmajority (write_concern));
   ASSERT (mongoc_write_concern_is_valid (write_concern));

   /* Manually assigning a negative wtimeout will make the write concern invalid
    */
   write_concern->wtimeout = -1;
   ASSERT (!mongoc_write_concern_is_valid (write_concern));

   mongoc_write_concern_destroy (write_concern);
}

static void
_test_write_concern_from_iterator (const char *swc, bool ok, bool is_default)
{
   bson_t *bson = tmp_bson (swc);
   const bson_t *bson2;
   mongoc_write_concern_t *wc;
   bson_iter_t iter;
   bson_error_t error;

   if (test_suite_debug_output ()) {
      fprintf (stdout, "  - %s\n", swc);
      fflush (stdout);
   }

   bson_iter_init_find (&iter, bson, "writeConcern");
   wc = _mongoc_write_concern_new_from_iter (&iter, &error);
   if (ok) {
      BSON_ASSERT (wc);
      ASSERT (mongoc_write_concern_is_default (wc) == is_default);
      bson2 = _mongoc_write_concern_get_bson (wc);
      ASSERT (bson_compare (bson, bson2));
      mongoc_write_concern_destroy (wc);
   } else {
      BSON_ASSERT (!wc);
      ASSERT_ERROR_CONTAINS (error,
                             MONGOC_ERROR_COMMAND,
                             MONGOC_ERROR_COMMAND_INVALID_ARG,
                             "Invalid writeConcern");
   }
}

static void
test_write_concern_from_iterator (void)
{
   _test_write_concern_from_iterator ("{'writeConcern': {}}", true, true);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 'majority'}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 'majority', 'j': true}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 'sometag'}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 'sometag', 'j': true}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 'sometag', 'j': false}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 1, 'j': true}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 1, 'j': false}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 0, 'j': true}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 0, 'j': false}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 42}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 1}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'j': true}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'j': false}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': -2}}", true, true);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': -3}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 'majority', 'wtimeout': 42}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 'sometag', 'wtimeout': 42}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'wtimeout': 42}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 1, 'wtimeout': 42}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 0, 'wtimeout': 42}}", true, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': 1.0}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': {'some': 'stuff'}}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'w': []}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'wtimeout': 'never'}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'j': 'never'}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'j': 1.0}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'fsync': 1.0}}", false, false);
   _test_write_concern_from_iterator (
      "{'writeConcern': {'fsync': true}}", true, false);
}


static void
test_write_concern_always_mutable (void)
{
   mongoc_write_concern_t *write_concern;

   write_concern = mongoc_write_concern_new ();

   ASSERT (write_concern);

   mongoc_write_concern_set_fsync (write_concern, true);
   ASSERT_MATCH (_mongoc_write_concern_get_bson (write_concern),
                 "{'fsync': true}");

   mongoc_write_concern_set_journal (write_concern, true);
   ASSERT_MATCH (_mongoc_write_concern_get_bson (write_concern),
                 "{'fsync': true, 'j': true}");

   mongoc_write_concern_set_w (write_concern, 2);
   ASSERT_MATCH (_mongoc_write_concern_get_bson (write_concern),
                 "{'w': 2, 'fsync': true, 'j': true}");

   mongoc_write_concern_set_wtimeout (write_concern, 100);
   ASSERT_MATCH (_mongoc_write_concern_get_bson (write_concern),
                 "{'w': 2, 'fsync': true, 'j': true, 'wtimeout': 100}");

   mongoc_write_concern_set_wmajority (write_concern, 200);
   ASSERT_MATCH (
      _mongoc_write_concern_get_bson (write_concern),
      "{'w': 'majority', 'fsync': true, 'j': true, 'wtimeout': 200}");

   mongoc_write_concern_set_wtag (write_concern, "MultipleDC");
   ASSERT_MATCH (
      _mongoc_write_concern_get_bson (write_concern),
      "{'w': 'MultipleDC', 'fsync': true, 'j': true, 'wtimeout': 200}");

   mongoc_write_concern_destroy (write_concern);
}


void
test_write_concern_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/WriteConcern/append", test_write_concern_append);
   TestSuite_Add (suite, "/WriteConcern/basic", test_write_concern_basic);
   TestSuite_Add (suite,
                  "/WriteConcern/bson_omits_defaults",
                  test_write_concern_bson_omits_defaults);
   TestSuite_Add (suite,
                  "/WriteConcern/bson_includes_false_fsync_and_journal",
                  test_write_concern_bson_includes_false_fsync_and_journal);
   TestSuite_Add (suite,
                  "/WriteConcern/fsync_and_journal_gle_and_validity",
                  test_write_concern_fsync_and_journal_w1_and_validity);
   TestSuite_Add (suite,
                  "/WriteConcern/wtimeout_validity",
                  test_write_concern_wtimeout_validity);
   TestSuite_Add (
      suite, "/WriteConcern/from_iterator", test_write_concern_from_iterator);
   TestSuite_Add (
      suite, "/WriteConcern/always_mutable", test_write_concern_always_mutable);
}
