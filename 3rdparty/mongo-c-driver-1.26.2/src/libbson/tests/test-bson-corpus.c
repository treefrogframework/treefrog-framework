#include <bson/bson.h>
#include "TestSuite.h"
#include "json-test.h"
#include "corpus-test.h"


#define IS_NAN(dec) (dec).high == 0x7c00000000000000ull


typedef struct {
   const char *scenario;
   const char *test;
} skipped_corpus_test_t;

skipped_corpus_test_t SKIPPED_CORPUS_TESTS[] = {
   /* CDRIVER-1879, can't make Code with embedded NIL */
   {"Javascript Code", "Embedded nulls"},
   {"Javascript Code with Scope",
    "Unicode and embedded null in code string, empty scope"},
   /* CDRIVER-2223, legacy extended JSON $date syntax uses numbers */
   {"Top-level document validity", "Bad $date (number, not string or hash)"},
   /* CDRIVER-3500, floating point output differs */
   {"Double type", "1.2345678921232E+18"},
   {"Double type", "-1.2345678921232E+18"},
   /* CDRIVER-4017, libbson does not emit escape sequences */
   {"Javascript Code", "two-byte UTF-8 (\xc3\xa9)"}, /* \u00e9 */
   {"Javascript Code", "three-byte UTF-8 (\xe2\x98\x86)"}, /* \u2606 */
   {"String", "two-byte UTF-8 (\xc3\xa9)"}, /* \u00e9 */
   {"String", "three-byte UTF-8 (\xe2\x98\x86)"}, /* \u2606 */
   {0}};


skipped_corpus_test_t VS2013_SKIPPED_CORPUS_TESTS[] = {
   /* VS 2013 and older is imprecise stackoverflow.com/questions/32232331 */
   {"Double type", "1.23456789012345677E+18"},
   {"Double type", "-1.23456789012345677E+18"},
   {0}};


static void
compare_data (const uint8_t *a,
              uint32_t a_len,
              const uint8_t *b,
              uint32_t b_len)
{
   bson_string_t *a_str;
   bson_string_t *b_str;
   uint32_t i;

   if (a_len != b_len || memcmp (a, b, (size_t) a_len)) {
      a_str = bson_string_new (NULL);
      for (i = 0; i < a_len; i++) {
         bson_string_append_printf (a_str, "%02X", (int) a[i]);
      }

      b_str = bson_string_new (NULL);
      for (i = 0; i < b_len; i++) {
         bson_string_append_printf (b_str, "%02X", (int) b[i]);
      }

      fprintf (stderr,
               "unequal data of length %d and %d:\n%s\n%s\n",
               a_len,
               b_len,
               a_str->str,
               b_str->str);

      abort ();
   }
}


static bool
is_test_skipped (const char *scenario, const char *description)
{
   skipped_corpus_test_t *skip;

   for (skip = SKIPPED_CORPUS_TESTS; skip->scenario != NULL; skip++) {
      if (!strcmp (skip->scenario, scenario) &&
          !strcmp (skip->test, description)) {
         return true;
      }
   }

/* _MSC_VER 1900 is Visual Studio 2015 */
#if (defined(_MSC_VER) && _MSC_VER < 1900)
   for (skip = VS2013_SKIPPED_CORPUS_TESTS; skip->scenario != NULL; skip++) {
      if (!strcmp (skip->scenario, scenario) &&
          !strcmp (skip->test, description)) {
         return true;
      }
   }
#endif

   return false;
}


/*
See:
github.com/mongodb/specifications/blob/master/source/bson-corpus/bson-corpus.rst
#testing-validity

* for cB input:
    * bson_to_canonical_extended_json(cB) = cE
    * bson_to_relaxed_extended_json(cB) = rE (if rE exists)

* for cE input:
    * json_to_bson(cE) = cB (unless lossy)

* for dB input (if it exists):
    * bson_to_canonical_extended_json(dB) = cE
    * bson_to_relaxed_extended_json(dB) = rE (if rE exists)

* for dE input (if it exists):
  * json_to_bson(dE) = cB (unless lossy)

* for rE input (if it exists):
    bson_to_relaxed_extended_json(json_to_bson(rE)) = rE

 */
static void
test_bson_corpus_valid (test_bson_valid_type_t *test)
{
   bson_t cB;
   bson_t dB;
   bson_t *decode_cE;
   bson_t *decode_dE;
   bson_t *decode_rE;
   bson_error_t error;

   BSON_ASSERT (test->cB);
   BSON_ASSERT (test->cE);

   if (is_test_skipped (test->scenario_description, test->test_description)) {
      if (test_suite_debug_output ()) {
         printf ("      SKIP\n");
         fflush (stdout);
      }

      return;
   }

   BSON_ASSERT (bson_init_static (&cB, test->cB, test->cB_len));
   ASSERT_CMPJSON (bson_as_canonical_extended_json (&cB, NULL), test->cE);

   if (test->rE) {
      ASSERT_CMPJSON (bson_as_relaxed_extended_json (&cB, NULL), test->rE);
   }

   decode_cE = bson_new_from_json ((const uint8_t *) test->cE, -1, &error);

   ASSERT_OR_PRINT (decode_cE, error);

   if (!test->lossy) {
      compare_data (
         bson_get_data (decode_cE), decode_cE->len, test->cB, test->cB_len);
   }

   if (test->dB) {
      BSON_ASSERT (bson_init_static (&dB, test->dB, test->dB_len));
      ASSERT_CMPJSON (bson_as_canonical_extended_json (&dB, NULL), test->cE);

      if (test->rE) {
         ASSERT_CMPJSON (bson_as_relaxed_extended_json (&dB, NULL), test->rE);
      }

      bson_destroy (&dB);
   }

   if (test->dE) {
      decode_dE = bson_new_from_json ((const uint8_t *) test->dE, -1, &error);

      ASSERT_OR_PRINT (decode_dE, error);
      ASSERT_CMPJSON (bson_as_canonical_extended_json (decode_dE, NULL),
                      test->cE);

      if (!test->lossy) {
         compare_data (
            bson_get_data (decode_dE), decode_dE->len, test->cB, test->cB_len);
      }

      bson_destroy (decode_dE);
   }

   if (test->rE) {
      decode_rE = bson_new_from_json ((const uint8_t *) test->rE, -1, &error);

      ASSERT_OR_PRINT (decode_rE, error);
      ASSERT_CMPJSON (bson_as_relaxed_extended_json (decode_rE, NULL),
                      test->rE);

      bson_destroy (decode_rE);
   }

   bson_destroy (decode_cE);
   bson_destroy (&cB);
}


/*
See:
github.com/mongodb/specifications/blob/master/source/bson-corpus/bson-corpus.rst
#testing-decode-errors
 */
static void
test_bson_corpus_decode_error (test_bson_decode_error_type_t *test)
{
   bson_t invalid_bson;

   BSON_ASSERT (test->bson);

   if (is_test_skipped (test->scenario_description, test->test_description)) {
      if (test_suite_debug_output ()) {
         printf ("      SKIP\n");
         fflush (stdout);
      }

      return;
   }

   ASSERT (test->bson);
   ASSERT (!bson_init_static (&invalid_bson, test->bson, test->bson_len) ||
           bson_empty (&invalid_bson) ||
           !bson_as_canonical_extended_json (&invalid_bson, NULL));
}


/*
See:
github.com/mongodb/specifications/blob/master/source/bson-corpus/bson-corpus.rst
#testing-parsing-errors
 */
static void
test_bson_corpus_parse_error (test_bson_parse_error_type_t *test)
{
   BSON_ASSERT (test->str);

   if (is_test_skipped (test->scenario_description, test->test_description)) {
      if (test_suite_debug_output ()) {
         printf ("      SKIP\n");
         fflush (stdout);
      }

      return;
   }

   switch (test->bson_type) {
   case BSON_TYPE_EOD: /* top-level document to be parsed as JSON */
   case BSON_TYPE_BINARY:
      ASSERT (!bson_new_from_json ((uint8_t *) test->str, test->str_len, NULL));
      break;
   case BSON_TYPE_DECIMAL128: {
      bson_decimal128_t dec;
      ASSERT (!bson_decimal128_from_string (test->str, &dec));
      ASSERT (IS_NAN (dec));
      break;
   }
   case BSON_TYPE_DOUBLE:
   case BSON_TYPE_UTF8:
   case BSON_TYPE_DOCUMENT:
   case BSON_TYPE_ARRAY:
   case BSON_TYPE_UNDEFINED:
   case BSON_TYPE_OID:
   case BSON_TYPE_BOOL:
   case BSON_TYPE_DATE_TIME:
   case BSON_TYPE_NULL:
   case BSON_TYPE_REGEX:
   case BSON_TYPE_DBPOINTER:
   case BSON_TYPE_CODE:
   case BSON_TYPE_SYMBOL:
   case BSON_TYPE_CODEWSCOPE:
   case BSON_TYPE_INT32:
   case BSON_TYPE_TIMESTAMP:
   case BSON_TYPE_INT64:
   case BSON_TYPE_MAXKEY:
   case BSON_TYPE_MINKEY:
   default:
      fprintf (stderr, "Unsupported parseError type: %#x\n", test->bson_type);
      abort ();
   }
}


static void
test_bson_corpus_cb (bson_t *scenario)
{
   corpus_test (scenario,
                test_bson_corpus_valid,
                test_bson_corpus_decode_error,
                test_bson_corpus_parse_error);
}

static void
test_bson_corpus_prose_1 (void) {
   bson_t *bson;
   bool ok;
   bson_t subdoc;

   /* Field name within a root document */
   bson = bson_new ();
   ok = bson_append_int32 (bson, "a\0b", 3, 123);
   BSON_ASSERT (!ok);
   bson_destroy (bson);

   /* Field name within a sub-document */
   bson = bson_new();
   bson_append_document_begin (bson, "subdoc", -1, &subdoc);
   ok = bson_append_int32 (&subdoc, "a\0b", 3, 123);
   BSON_ASSERT (!ok);
   bson_destroy (&subdoc);
   bson_destroy (bson);

   /* Pattern for a regular expression */
   bson = bson_new ();
   ok = bson_append_regex_w_len (bson, "key", 3, "a\0b", 3, "");
   BSON_ASSERT (!ok);
   bson_destroy (bson);

   /* Testing options for a regular expression is not possible, since
    * bson_append_regex_w_len does not accept a length for options. */
}

void
test_bson_corpus_install (TestSuite *suite)
{
   install_json_test_suite_with_check (
      suite, BSON_JSON_DIR, "bson_corpus", test_bson_corpus_cb);
   TestSuite_Add (suite, "/bson_corpus/prose_1", test_bson_corpus_prose_1);
}
