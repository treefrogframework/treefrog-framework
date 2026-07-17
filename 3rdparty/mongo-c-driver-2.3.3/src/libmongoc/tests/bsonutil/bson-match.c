/*
 * Copyright 2009-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <bsonutil/bson-match.h>

#include <mongoc/mongoc-util-private.h> // hex_to_bin

#include <mongoc/utlist.h>

#include <mlib/loop.h>

#include <TestSuite.h>
#include <test-conveniences.h>
#include <unified/util.h>

typedef struct _special_functor_t {
   special_fn fn;
   void *user_data;
   char *keyword;
   struct _special_functor_t *next;
} special_functor_t;

struct _bson_matcher_t {
   special_functor_t *specials;
};

#define MATCH_ERR(format, ...) test_set_error(error, "match error at path: '%s': " format, context->path, __VA_ARGS__)

static char *
get_first_key(const bson_t *bson)
{
   bson_iter_t iter;

   bson_iter_init(&iter, bson);
   if (!bson_iter_next(&iter)) {
      return "";
   }

   return (char *)bson_iter_key(&iter);
}

static bool
is_special_match(const bson_t *bson)
{
   char *first_key = get_first_key(bson);
   if (strstr(first_key, "$$") != first_key) {
      return false;
   }
   if (bson_count_keys(bson) != 1) {
      return false;
   }
   return true;
}

/* implements $$placeholder */
static bool
special_placeholder(const bson_matcher_context_t *context,
                    const bson_t *assertion,
                    const bson_val_t *actual,
                    void *user_data,
                    bson_error_t *error)
{
   BSON_UNUSED(context);
   BSON_UNUSED(assertion);
   BSON_UNUSED(actual);
   BSON_UNUSED(user_data);
   BSON_UNUSED(error);

   /* Nothing to do (not an operator, just a reserved key value). The meaning
    * and corresponding behavior of $$placeholder depends on context. */
   return true;
}

/* implements $$exists */
static bool
special_exists(const bson_matcher_context_t *context,
               const bson_t *assertion,
               const bson_val_t *actual,
               void *user_data,
               bson_error_t *error)
{
   bool ret = false;
   bson_iter_t iter;
   bool should_exist;

   BSON_UNUSED(context);
   BSON_UNUSED(user_data);

   bson_iter_init(&iter, assertion);
   BSON_ASSERT(bson_iter_next(&iter));

   if (!BSON_ITER_HOLDS_BOOL(&iter)) {
      MATCH_ERR("%s", "unexpected non-bool $$exists assertion");
   }
   should_exist = bson_iter_bool(&iter);

   if (should_exist && NULL == actual) {
      MATCH_ERR("%s", "should exist but does not");
      goto done;
   }

   if (!should_exist && NULL != actual) {
      MATCH_ERR("%s", "should not exist but does");
      goto done;
   }

   ret = true;
done:
   return ret;
}

/* implements $$type */
static bool
special_type(const bson_matcher_context_t *context,
             const bson_t *assertion,
             const bson_val_t *actual,
             void *user_data,
             bson_error_t *error)
{
   bool ret = false;
   bson_iter_t iter;

   BSON_UNUSED(context);
   BSON_UNUSED(user_data);

   bson_iter_init(&iter, assertion);
   BSON_ASSERT(bson_iter_next(&iter));

   if (!actual) {
      MATCH_ERR("%s", "does not exist but should");
      goto done;
   }

   if (BSON_ITER_HOLDS_UTF8(&iter)) {
      bson_type_t expected_type = bson_type_from_string(bson_iter_utf8(&iter, NULL));
      if (expected_type != bson_val_type(actual)) {
         MATCH_ERR("expected type: %s, got: %s",
                   bson_type_to_string(expected_type),
                   bson_type_to_string(bson_val_type(actual)));
         goto done;
      }
   }

   if (BSON_ITER_HOLDS_ARRAY(&iter)) {
      bson_t arr;
      bson_iter_t arriter;
      bool found = false;

      bson_iter_bson(&iter, &arr);
      BSON_FOREACH(&arr, arriter)
      {
         bson_type_t expected_type;

         if (!BSON_ITER_HOLDS_UTF8(&arriter)) {
            MATCH_ERR("%s", "unexpected non-UTF8 $$type assertion");
            goto done;
         }

         expected_type = bson_type_from_string(bson_iter_utf8(&arriter, NULL));
         if (expected_type == bson_val_type(actual)) {
            found = true;
            break;
         }
      }
      if (!found) {
         MATCH_ERR("expected one of type: %s, got %s", tmp_json(&arr), bson_type_to_string(bson_val_type(actual)));
         goto done;
      }
   }

   ret = true;
done:
   return ret;
}

/* implements $$unsetOrMatches */
static bool
special_unset_or_matches(const bson_matcher_context_t *context,
                         const bson_t *assertion,
                         const bson_val_t *actual,
                         void *user_data,
                         bson_error_t *error)
{
   bool ret = false;
   bson_iter_t iter;
   bson_val_t *expected = NULL;

   BSON_UNUSED(user_data);

   bson_iter_init(&iter, assertion);
   BSON_ASSERT(bson_iter_next(&iter));
   expected = bson_val_from_iter(&iter);

   if (actual == NULL) {
      ret = true;
      goto done;
   }

   if (!bson_matcher_match(context, expected, actual, error)) {
      goto done;
   }

   ret = true;
done:
   bson_val_destroy(expected);
   return ret;
}

/* implements $$matchesHexBytes */
static bool
special_matches_hex_bytes(const bson_matcher_context_t *context,
                          const bson_t *assertion,
                          const bson_val_t *actual,
                          void *user_data,
                          bson_error_t *error)
{
   bool ret = false;
   uint8_t *expected_bytes;
   size_t expected_bytes_len;
   const uint8_t *actual_bytes;
   size_t actual_bytes_len;
   uint32_t actual_bytes_len_u32;
   char *expected_bytes_string = NULL;
   char *actual_bytes_string = NULL;
   bson_iter_t iter;

   BSON_UNUSED(context);
   BSON_UNUSED(user_data);

   bson_iter_init(&iter, assertion);
   BSON_ASSERT(bson_iter_next(&iter));

   if (!actual) {
      MATCH_ERR("%s", "does not exist but should");
      goto done;
   }

   if (!BSON_ITER_HOLDS_UTF8(&iter)) {
      MATCH_ERR("%s", "$$matchesHexBytes does not contain utf8");
      goto done;
   }

   if (bson_val_type(actual) != BSON_TYPE_BINARY) {
      MATCH_ERR("%s", "value does not contain binary");
      goto done;
   }

   expected_bytes = hex_to_bin(bson_iter_utf8(&iter, NULL), &expected_bytes_len);
   actual_bytes = bson_val_to_binary(actual, &actual_bytes_len_u32);
   actual_bytes_len = actual_bytes_len_u32;
   expected_bytes_string = bin_to_hex(expected_bytes, expected_bytes_len);
   actual_bytes_string = bin_to_hex(actual_bytes, actual_bytes_len);

   if (expected_bytes_len != actual_bytes_len) {
      MATCH_ERR("expected %zu (%s) but got %zu (%s) bytes",
                expected_bytes_len,
                expected_bytes_string,
                actual_bytes_len,
                actual_bytes_string);
      bson_free(expected_bytes);
      bson_free(expected_bytes_string);
      bson_free(actual_bytes_string);
      goto done;
   }

   if (0 != memcmp(expected_bytes, actual_bytes, expected_bytes_len)) {
      MATCH_ERR("expected %s, but got %s", expected_bytes_string, actual_bytes_string);
      bson_free(expected_bytes);
      bson_free(expected_bytes_string);
      bson_free(actual_bytes_string);
      goto done;
   }

   bson_free(expected_bytes);
   bson_free(expected_bytes_string);
   bson_free(actual_bytes_string);

   ret = true;
done:
   return ret;
}

/* implements $$matchAsDocument */
static bool
special_match_as_document(const bson_matcher_context_t *context,
                          const bson_t *assertion,
                          const bson_val_t *actual,
                          void *user_data,
                          bson_error_t *error)
{
   bool ret = false;
   bson_t actual_as_bson = BSON_INITIALIZER;
   BSON_UNUSED(user_data);

   bson_iter_t iter;
   bson_iter_init(&iter, assertion);
   BSON_ASSERT(bson_iter_next(&iter));
   if (!BSON_ITER_HOLDS_DOCUMENT(&iter)) {
      MATCH_ERR("%s", "$$matchAsDocument does not contain a document");
      goto done;
   }

   if (!actual) {
      MATCH_ERR("%s", "does not exist but should");
      goto done;
   }

   if (bson_val_type(actual) != BSON_TYPE_UTF8) {
      MATCH_ERR("%s", "value type is not utf8");
      goto done;
   }
   const char *actual_json = bson_val_to_utf8(actual);
   if (!bson_init_from_json(&actual_as_bson, actual_json, -1, error)) {
      MATCH_ERR("%s", "value can't be parsed as JSON");
      goto done;
   }

   bson_val_t *expected_val = bson_val_from_iter(&iter);
   bson_val_t *actual_val = bson_val_from_bson(&actual_as_bson);
   ret = bson_matcher_match(context, expected_val, actual_val, error);
   bson_val_destroy(actual_val);
   bson_val_destroy(expected_val);
   bson_destroy(&actual_as_bson);

done:

   return ret;
}

/* implements $$matchAsRoot */
static bool
special_match_as_root(const bson_matcher_context_t *context,
                      const bson_t *assertion,
                      const bson_val_t *actual,
                      void *user_data,
                      bson_error_t *error)
{
   bool ret = false;
   BSON_UNUSED(user_data);

   bson_iter_t iter;
   bson_iter_init(&iter, assertion);
   BSON_ASSERT(bson_iter_next(&iter));
   if (!BSON_ITER_HOLDS_DOCUMENT(&iter)) {
      MATCH_ERR("%s", "$$matchAsRoot does not contain a document");
      goto done;
   }

   if (!actual) {
      MATCH_ERR("%s", "does not exist but should");
      goto done;
   }

   if (bson_val_type(actual) != BSON_TYPE_DOCUMENT) {
      MATCH_ERR("%s", "value is not a document");
      goto done;
   }

   bson_matcher_context_t as_root_context = *context;
   as_root_context.is_root = true;

   bson_val_t *expected_val = bson_val_from_iter(&iter);
   ret = bson_matcher_match(&as_root_context, expected_val, actual, error);
   bson_val_destroy(expected_val);

done:
   return ret;
}

static bool
evaluate_special(const bson_matcher_context_t *context,
                 const bson_t *assertion,
                 const bson_val_t *actual,
                 bson_error_t *error)
{
   bson_iter_t iter;
   const char *assertion_key;
   special_functor_t *special_iter;

   bson_iter_init(&iter, assertion);
   BSON_ASSERT(bson_iter_next(&iter));
   assertion_key = bson_iter_key(&iter);

   LL_FOREACH(context->matcher->specials, special_iter)
   {
      if (0 == strcmp(assertion_key, special_iter->keyword)) {
         return special_iter->fn(context, assertion, actual, special_iter->user_data, error);
      }
   }

   MATCH_ERR("unrecognized special operator: %s", assertion_key);
   return false;
}


bson_matcher_t *
bson_matcher_new(void)
{
   bson_matcher_t *matcher = bson_malloc0(sizeof(bson_matcher_t));
   /* Add default special functions. */
   bson_matcher_add_special(matcher, "$$placeholder", special_placeholder, NULL);
   bson_matcher_add_special(matcher, "$$exists", special_exists, NULL);
   bson_matcher_add_special(matcher, "$$type", special_type, NULL);
   bson_matcher_add_special(matcher, "$$unsetOrMatches", special_unset_or_matches, NULL);
   bson_matcher_add_special(matcher, "$$matchesHexBytes", special_matches_hex_bytes, NULL);
   bson_matcher_add_special(matcher, "$$matchAsDocument", special_match_as_document, NULL);
   bson_matcher_add_special(matcher, "$$matchAsRoot", special_match_as_root, NULL);
   return matcher;
}

/* Add a hook function for matching a special $$ operator */
void
bson_matcher_add_special(bson_matcher_t *matcher, const char *keyword, special_fn special, void *user_data)
{
   special_functor_t *functor;

   if (strstr(keyword, "$$") != keyword) {
      test_error("unexpected special match keyword: %s. Should start with '$$'", keyword);
   }

   functor = bson_malloc(sizeof(special_functor_t));
   functor->keyword = bson_strdup(keyword);
   functor->fn = special;
   functor->user_data = user_data;
   LL_PREPEND(matcher->specials, functor);
}

void
bson_matcher_destroy(bson_matcher_t *matcher)
{
   special_functor_t *special_iter, *tmp;

   if (!matcher) {
      return;
   }

   LL_FOREACH_SAFE(matcher->specials, special_iter, tmp)
   {
      bson_free(special_iter->keyword);
      bson_free(special_iter);
   }
   bson_free(matcher);
}

bool
bson_matcher_match(const bson_matcher_context_t *context,
                   const bson_val_t *expected,
                   const bson_val_t *actual,
                   bson_error_t *error)
{
   bool ret = false;

   if (bson_val_type(expected) == BSON_TYPE_DOCUMENT) {
      bson_iter_t expected_iter;
      const bson_t *expected_bson = bson_val_to_document(expected);
      const bson_t *actual_bson = NULL;
      uint32_t expected_keys;
      uint32_t actual_keys;

      /* handle special operators (e.g. $$type) */
      if (is_special_match(expected_bson)) {
         ret = evaluate_special(context, expected_bson, actual, error);
         goto done;
      }

      if (bson_val_type(actual) != BSON_TYPE_DOCUMENT) {
         MATCH_ERR("expected type document, got %s", bson_type_to_string(bson_val_type(actual)));
         goto done;
      }

      actual_bson = bson_val_to_document(actual);

      BSON_FOREACH(expected_bson, expected_iter)
      {
         const char *key;
         bson_val_t *expected_val = NULL;
         bson_val_t *actual_val = NULL;
         bson_iter_t actual_iter;

         key = bson_iter_key(&expected_iter);
         expected_val = bson_val_from_iter(&expected_iter);

         if (bson_iter_init_find(&actual_iter, actual_bson, key)) {
            actual_val = bson_val_from_iter(&actual_iter);
         }

         if (bson_val_type(expected_val) == BSON_TYPE_DOCUMENT &&
             is_special_match(bson_val_to_document(expected_val))) {
            char *path_child = bson_strdup_printf("%s.%s", context->path, key);
            bson_matcher_context_t special_context = {.matcher = context->matcher, .path = path_child};
            bool special_ret =
               evaluate_special(&special_context, bson_val_to_document(expected_val), actual_val, error);
            bson_free(path_child);
            bson_val_destroy(expected_val);
            bson_val_destroy(actual_val);
            if (!special_ret) {
               goto done;
            }
            continue;
         }

         if (NULL == actual_val) {
            MATCH_ERR("key '%s' is not present", key);
            bson_val_destroy(expected_val);
            bson_val_destroy(actual_val);
            goto done;
         }

         char *path_child = bson_strdup_printf("%s.%s", context->path, key);
         bson_matcher_context_t document_child_context = {
            .matcher = context->matcher,
            .path = path_child,
         };
         bool document_child_ret = bson_matcher_match(&document_child_context, expected_val, actual_val, error);
         bson_val_destroy(expected_val);
         bson_val_destroy(actual_val);
         bson_free(path_child);
         if (!document_child_ret) {
            goto done;
         }
      }

      expected_keys = bson_count_keys(expected_bson);
      actual_keys = bson_count_keys(actual_bson);

      /* Unified test format spec: "When matching root-level documents, test
       * runners MUST permit the actual document to contain additional fields
       * not present in the expected document.""
       *
       * This logic must also handle the case where `expected` is one of any
       * number of root documents within an array (i.e. cursor result); see
       * array_child_context below. */
      if (!context->is_root && expected_keys < actual_keys) {
         MATCH_ERR("expected %" PRIu32 " keys in document, got: %" PRIu32, expected_keys, actual_keys);
         goto done;
      }

      ret = true;
      goto done;
   }

   if (bson_val_type(expected) == BSON_TYPE_ARRAY) {
      bson_iter_t expected_iter;
      const bson_t *expected_bson = bson_val_to_array(expected);
      const bson_t *actual_bson = NULL;
      uint32_t expected_keys = bson_count_keys(expected_bson);
      uint32_t actual_keys;

      if (bson_val_type(actual) != BSON_TYPE_ARRAY) {
         MATCH_ERR("expected array, but got: %s", bson_type_to_string(bson_val_type(actual)));
         goto done;
      }

      actual_bson = bson_val_to_array(actual);
      actual_keys = bson_count_keys(actual_bson);

      if (expected_keys != actual_keys) {
         MATCH_ERR("expected array of size %" PRIu32 ", but got array of size: %" PRIu32, expected_keys, actual_keys);
         goto done;
      }

      BSON_FOREACH(expected_bson, expected_iter)
      {
         bson_val_t *expected_val = bson_val_from_iter(&expected_iter);
         bson_val_t *actual_val = NULL;
         bson_iter_t actual_iter;
         const char *key;

         key = bson_iter_key(&expected_iter);
         if (!bson_iter_init_find(&actual_iter, actual_bson, key)) {
            MATCH_ERR("expected array index: %s, but did not exist", key);
            bson_val_destroy(expected_val);
            bson_val_destroy(actual_val);
            goto done;
         }

         actual_val = bson_val_from_iter(&actual_iter);

         char *path_child = bson_strdup_printf("%s.%s", context->path, key);
         bson_matcher_context_t array_child_context = {
            .matcher = context->matcher,
            .path = path_child,
            .is_root = context->is_root && context->array_of_root_docs,
         };
         bool array_child_ret = bson_matcher_match(&array_child_context, expected_val, actual_val, error);
         bson_val_destroy(expected_val);
         bson_val_destroy(actual_val);
         bson_free(path_child);
         if (!array_child_ret) {
            goto done;
         }
      }
      ret = true;
      goto done;
   }

   if (!bson_val_eq(expected, actual, BSON_VAL_FLEXIBLE_NUMERICS)) {
      MATCH_ERR("value %s != %s", bson_val_to_json(expected), bson_val_to_json(actual));
      goto done;
   }

   ret = true;
done:
   if (!ret && context->is_root) {
      /* Append the error with more context at the root match. */
      bson_error_t tmp_error;

      memcpy(&tmp_error, error, sizeof(bson_error_t));
      test_set_error(error,
                     "BSON match failed: %s\n"
                     "Expected: %s\n"
                     "Actual:   %s",
                     tmp_error.message,
                     bson_val_to_json(expected),
                     bson_val_to_json(actual));
   }
   return ret;
}

bool
bson_match(const bson_val_t *expected, const bson_val_t *actual, bool array_of_root_docs, bson_error_t *error)
{
   bson_matcher_context_t root_context = {
      .matcher = bson_matcher_new(),
      .path = "",
      .is_root = true,
      .array_of_root_docs = array_of_root_docs,
   };
   bool matched = bson_matcher_match(&root_context, expected, actual, error);
   bson_matcher_destroy(root_context.matcher);
   return matched;
}

typedef struct {
   const char *desc;
   const char *expected;
   const char *actual;
   bool expect_match;
} testcase_t;

static void
test_match(void)
{
   testcase_t tests[] = {
      {"int32 ==", "{'a': 1}", "{'a': 1}", true},
      {"int32 !=", "{'a': 1}", "{'a': 0}", false},
      {"$$exists", "{'a': {'$$exists': true}}", "{'a': 0}", true},
      {"$$exists fail", "{'a': {'$$exists': true}}", "{'b': 0}", false},
      {"does not $$exists", "{'a': {'$$exists': false}}", "{'b': 0}", true},
      {"$$unsetOrMatches match", "{'a': {'$$unsetOrMatches': 1}}", "{'a': 1}", true},
      {"$$unsetOrMatches unset", "{'a': {'$$unsetOrMatches': 1}}", "{}", true},
      {"$$unsetOrMatches mismatch", "{'a': {'$$unsetOrMatches': 'abc'}}", "{'a': 1}", false},
      {"$$type match", "{'a': {'$$type': 'string'}}", "{'a': 'abc'}", true},
      {"$$type mismatch", "{'a': {'$$type': 'string'}}", "{'a': 1}", false},
      {"$$type array match", "{'a': {'$$type': ['string', 'int']}}", "{'a': 1}", true},
      {"$$type array mismatch", "{'a': {'$$type': ['string', 'int']}}", "{'a': 1.2}", false},
      {"extra keys in root ok", "{'a': 1}", "{'a': 1, 'b': 2}", true},
      {"extra keys in subdoc not ok", "{'a': {'b': 1}}", "{'a': {'b': 1, 'c': 2}}", false},
      {"extra keys in subdoc allowed explicitly",
       "{'a': {'$$matchAsRoot': {'b': 1}}}",
       "{'a': {'b': 1, 'c': 2}}",
       true},
      {"missing key in matchAsRoot subdoc",
       "{'a': {'$$matchAsRoot': {'b': 1, 'd': 1}}}",
       "{'a': {'b': 1, 'c': 2}}",
       false},
      {"$$matchAsDocument match", "{'a': {'$$matchAsDocument': {'b': 'abc'}}}", "{'a': '{\\'b\\':\\'abc\\'}'}", true},
      {"$$matchAsDocument mismatch",
       "{'a': {'$$matchAsDocument': {'b': 'abc'}}}",
       "{'a': '{\\'b\\':\\'xyz\\'}'}",
       false},
      {"$$matchAsDocument parse error", "{'a': {'$$matchAsDocument': {'b': 'abc'}}}", "{'a': 'nope'}", false},
      {"$$matchAsDocument extra keys not ok",
       "{'a': {'$$matchAsDocument': {'b': 'abc'}}}",
       "{'a': '{\\'b\\':\\'abc\\',\\'c\\':1}'}",
       false},
      {"$$matchAsDocument and $$matchAsRoot, extra keys ok",
       "{'a': {'$$matchAsDocument': {'$$matchAsRoot': {'b': 'abc'}}}}",
       "{'a': '{\\'b\\':\\'abc\\',\\'c\\':1}'}",
       true},
      {"numeric type mismatch is ok", "{'a': 1}", "{'a': 1.0}", true},
      {"comparing number to string is an error", "{'a': 1}", "{'a': 'foo'}", false}};

   mlib_foreach_arr (testcase_t, test, tests) {
      bson_error_t error;
      bson_val_t *expected = bson_val_from_json(test->expected);
      bson_val_t *actual = bson_val_from_json(test->actual);
      bool ret;

      ret = bson_match(expected, actual, false, &error);
      if (test->expect_match) {
         if (!ret) {
            test_error("%s: did not match with error: %s, but should have", test->desc, error.message);
         }
      } else {
         if (ret) {
            test_error("%s: matched, but should not have", test->desc);
         }
      }
      bson_val_destroy(expected);
      bson_val_destroy(actual);
   }
}

void
test_bson_match_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/unified/selftest/bson/match", test_match);
}
