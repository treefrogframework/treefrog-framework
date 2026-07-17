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


#include <bson/bson.h>
#include <bsonutil/bson-parser.h>

#include <TestSuite.h>
#include <json-test.h>
#include <test-conveniences.h>

#ifdef _MSC_VER
#define SSCANF sscanf_s
#else
#define SSCANF sscanf
#endif

struct view_abi_reference_type {
   void *data;
   uint32_t data_len;
   uint8_t header_0;
   uint8_t header_1;
};

/* ABI: Make sure vector views have the expected size. */
#define EXPECTED_VECTOR_VIEW_SIZE (sizeof(struct view_abi_reference_type))
BSON_STATIC_ASSERT2(sizeof_bson_vector_int8_const_view_t,
                    sizeof(bson_vector_int8_const_view_t) == EXPECTED_VECTOR_VIEW_SIZE);
BSON_STATIC_ASSERT2(sizeof_bson_vector_int8_view_t, sizeof(bson_vector_int8_view_t) == EXPECTED_VECTOR_VIEW_SIZE);
BSON_STATIC_ASSERT2(sizeof_bson_vector_float32_const_view_t,
                    sizeof(bson_vector_float32_const_view_t) == EXPECTED_VECTOR_VIEW_SIZE);
BSON_STATIC_ASSERT2(sizeof_bson_vector_float32_view_t, sizeof(bson_vector_float32_view_t) == EXPECTED_VECTOR_VIEW_SIZE);
BSON_STATIC_ASSERT2(sizeof_bson_vector_packed_bit_const_view_t,
                    sizeof(bson_vector_packed_bit_const_view_t) == EXPECTED_VECTOR_VIEW_SIZE);
BSON_STATIC_ASSERT2(sizeof_bson_vector_packed_bit_view_t,
                    sizeof(bson_vector_packed_bit_view_t) == EXPECTED_VECTOR_VIEW_SIZE);
#undef EXPECTED_VECTOR_VIEW_SIZE

typedef struct vector_json_test_case_t {
   char *scenario_description, *scenario_test_key;
   char *test_description, *test_dtype_hex_str, *test_dtype_alias_str, *test_canonical_bson_str;
   bson_t *test_vector_array;
   int64_t *test_padding;
   bool *test_valid;
} vector_json_test_case_t;

static bool
append_vector_packed_bit_from_packed_array(
   bson_t *bson, const char *key, int key_length, const bson_iter_t *iter, int64_t padding, bson_error_t *error)
{
   // (Spec test improvement TODO) This implements something the test covers that our API doesn't. If the test
   // were modified to cover element-by-element conversion, this can be replaced with
   // bson_append_vector_packed_bit_from_array.

   BSON_ASSERT_PARAM(bson);
   BSON_ASSERT_PARAM(key);
   BSON_ASSERT_PARAM(iter);

   size_t byte_count = 0;
   {
      bson_iter_t validation_iter = *iter;
      while (bson_iter_next(&validation_iter)) {
         if (!BSON_ITER_HOLDS_INT(&validation_iter)) {
            bson_set_error(error,
                           BSON_ERROR_VECTOR,
                           BSON_VECTOR_ERROR_ARRAY_ELEMENT_TYPE,
                           "expected int32 or int64 in BSON array key '%s', found item type 0x%02X",
                           bson_iter_key(&validation_iter),
                           (unsigned)bson_iter_type(&validation_iter));
            return false;
         }
         int64_t byte_as_int64 = bson_iter_as_int64(&validation_iter);
         if (byte_as_int64 < 0 || byte_as_int64 > UINT8_MAX) {
            bson_set_error(error,
                           BSON_ERROR_VECTOR,
                           BSON_VECTOR_ERROR_ARRAY_ELEMENT_VALUE,
                           "BSON array key '%s' value %" PRId64 " is out of range for packed byte",
                           bson_iter_key(&validation_iter),
                           byte_as_int64);
            return false;
         }
         byte_count++;
      }
   }

   if (padding < 0 || padding > 7) {
      bson_set_error(error,
                     TEST_ERROR_DOMAIN,
                     TEST_ERROR_CODE,
                     "'padding' parameter (%" PRId64 ") for append_vector_packed_bit_from_packed_array is out of range",
                     padding);
      return false;
   }
   if (byte_count < 1 && padding > 0) {
      bson_set_error(error,
                     TEST_ERROR_DOMAIN,
                     TEST_ERROR_CODE,
                     "nonzero 'padding' parameter (%" PRId64
                     ") for zero-length append_vector_packed_bit_from_packed_array",
                     padding);
      return false;
   }

   bson_vector_packed_bit_view_t view;
   if (bson_append_vector_packed_bit_uninit(bson, key, key_length, byte_count * 8u - (size_t)padding, &view)) {
      bson_iter_t copy_iter = *iter;
      for (size_t i = 0; i < byte_count; i++) {
         ASSERT(bson_iter_next(&copy_iter));
         uint8_t packed_byte = (uint8_t)bson_iter_as_int64(&copy_iter);
         ASSERT(bson_vector_packed_bit_view_write_packed(view, &packed_byte, 1, i));

         // Read back the packed byte, interpret any masking as a conversion failure.
         uint8_t packed_byte_check;
         ASSERT(bson_vector_packed_bit_view_read_packed(view, &packed_byte_check, 1, i));
         if (packed_byte != packed_byte_check) {
            bson_set_error(error,
                           TEST_ERROR_DOMAIN,
                           TEST_ERROR_CODE,
                           "byte at index %zu with value 0x%02X included write to masked bits (reads as 0x%02X)",
                           i,
                           packed_byte,
                           packed_byte_check);
            return false;
         }
      }
      return true;
   } else {
      return false;
   }
}

static void
hex_str_to_bson(bson_t *bson_out, const char *hex_str)
{
   uint32_t size = (uint32_t)strlen(hex_str) / 2u;
   uint8_t *buffer = bson_reserve_buffer(bson_out, size);
   for (uint32_t i = 0; i < size; i++) {
      unsigned int byte;
      ASSERT(SSCANF(&hex_str[i * 2], "%2x", &byte) == 1);
      buffer[i] = (uint8_t)byte;
   }
}

// Implement spec tests, given parsed arguments
static void
test_bson_vector_json_case(vector_json_test_case_t *test_case)
{
   bson_t expected_bson = BSON_INITIALIZER;
   if (test_case->test_canonical_bson_str) {
      hex_str_to_bson(&expected_bson, test_case->test_canonical_bson_str);
   }

   ASSERT(test_case->test_valid);
   ASSERT(test_case->test_dtype_hex_str);
   ASSERT(test_case->scenario_test_key);

   bson_t vector_from_array = BSON_INITIALIZER;
   bson_error_t vector_from_array_error;
   bool vector_from_array_ok;

   // Try a format conversion from array to the indicated vector format.
   // The spec calls the first header byte "dtype" (combining the element type and element size fields)
   if (0 == strcmp("0x03", test_case->test_dtype_hex_str)) {
      // int8 vector from int32/int64 array
      bson_iter_t iter;
      bool padding_ok = !test_case->test_padding || *test_case->test_padding == 0;
      vector_from_array_ok = test_case->test_vector_array && padding_ok &&
                             bson_iter_init(&iter, test_case->test_vector_array) &&
                             BSON_APPEND_VECTOR_INT8_FROM_ARRAY(
                                &vector_from_array, test_case->scenario_test_key, &iter, &vector_from_array_error);
   } else if (0 == strcmp("0x27", test_case->test_dtype_hex_str)) {
      // float32 vector from float64 array
      bson_iter_t iter;
      bool padding_ok = !test_case->test_padding || *test_case->test_padding == 0;
      vector_from_array_ok = test_case->test_vector_array && padding_ok &&
                             bson_iter_init(&iter, test_case->test_vector_array) &&
                             BSON_APPEND_VECTOR_FLOAT32_FROM_ARRAY(
                                &vector_from_array, test_case->scenario_test_key, &iter, &vector_from_array_error);
   } else if (0 == strcmp("0x10", test_case->test_dtype_hex_str)) {
      // packed_bit from packed bytes in an int array, with "padding" parameter supplied separately.
      // Suggested changes to reduce the special cases here:
      //  - Array-to-Vector should be defined as an element-by-element conversion. This test shouldn't operate on packed
      //  representations.
      //  - Include additional JSON tests for packed access, distinct from Array conversion.
      //  - Tests should keep the unused bits zeroed as required.
      // (Spec test improvement TODO)
      bson_iter_t iter;
      if (!test_case->test_padding) {
         test_error("test '%s' is missing required 'padding' field", test_case->test_description);
      }
      vector_from_array_ok = test_case->test_vector_array && bson_iter_init(&iter, test_case->test_vector_array) &&
                             append_vector_packed_bit_from_packed_array(&vector_from_array,
                                                                        test_case->scenario_test_key,
                                                                        -1,
                                                                        &iter,
                                                                        *test_case->test_padding,
                                                                        &vector_from_array_error);
   } else {
      test_error(
         "test '%s' has unsupported dtype_hex format '%s'", test_case->test_description, test_case->test_dtype_hex_str);
   }

   if (*test_case->test_valid) {
      /*
       * "To prove correct in a valid case (valid: true), one MUST
       * - encode a document from the numeric values, dtype, and padding, along with the "test_key", and assert this
       * matches the canonical_bson string.
       * - decode the canonical_bson into its binary form, and then assert that the numeric values, dtype, and padding
       * all match those provided in the JSON."
       */

      // Check the vector-from-array performed above ("encode")

      if (!test_case->test_vector_array) {
         test_error("test '%s' should be valid, but missing 'vector' field", test_case->test_description);
      }

      if (!vector_from_array_ok) {
         test_error("test '%s' should be valid, but vector-from-array failed: %s",
                    test_case->test_description,
                    vector_from_array_error.message);
      }

      if (0 != bson_compare(&vector_from_array, &expected_bson)) {
         test_error("test '%s' did not exactly match the reference document.\n "
                    "Actual: %s\n Expected: %s",
                    test_case->test_description,
                    tmp_json(&vector_from_array),
                    tmp_json(&expected_bson));
      }

      // Perform an array-from-vector and check it ("decode")

      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &expected_bson, test_case->scenario_test_key));

      bson_t array_from_vector = BSON_INITIALIZER;
      {
         bson_array_builder_t *array_builder = bson_array_builder_new();
         if (!bson_array_builder_append_vector_elements(array_builder, &iter)) {
            test_error("test '%s' should be valid but failed array-from-vector conversion",
                       test_case->test_description);
         }
         ASSERT(bson_array_builder_build(array_builder, &array_from_vector));
         bson_array_builder_destroy(array_builder);
      }

      if (BSON_ITER_HOLDS_VECTOR_FLOAT32(&iter)) {
         // float32 special case: Due to underspecified rounding and conversion rules we compare value inexactly.
         // (Spec test improvement TODO)

         bson_iter_t actual_iter, expected_iter;
         ASSERT(bson_iter_init(&actual_iter, &array_from_vector));
         ASSERT(bson_iter_init(&expected_iter, test_case->test_vector_array));

         for (size_t i = 0;; i++) {
            bool actual_next = bson_iter_next(&actual_iter);
            bool expected_next = bson_iter_next(&expected_iter);
            if (!actual_next && !expected_next) {
               break;
            } else if (!actual_next) {
               test_error("converted array is shorter than expected");
            } else if (!expected_next) {
               test_error("converted array is longer than expected");
            }

            if (!BSON_ITER_HOLDS_DOUBLE(&actual_iter)) {
               test_error("converted array element %d has unexpected type, should be double", (int)i);
            }
            double actual_double = bson_iter_double(&actual_iter);

            double expected_double;
            if (BSON_ITER_HOLDS_DOUBLE(&expected_iter)) {
               expected_double = bson_iter_double(&expected_iter);
            } else {
               test_error("test-vector array element %d has unexpected type, should be double, 'inf', or '-inf'.",
                          (int)i);
            }

            bool is_sorta_equal = false;
            if (expected_double != expected_double) {
               // Expect NaN, any type is fine.
               if (actual_double != actual_double) {
                  is_sorta_equal = true;
               }
            } else if (expected_double == 0.0 || expected_double * 0.0 != 0.0) {
               // Infinity or zero, equality comparison is fine.
               is_sorta_equal = expected_double == actual_double;
            } else {
               // Finite number, allow +/- error relative to the scale of the expected value.
               // Note that ASSERT_EQUAL_DOUBLE() in TestSuite exists but its fixed error threshold of 20% seems too
               // loose for this application.
               static const double allowed_relative_error = 1e-7;
               double allowed_absolute_error = fabs(allowed_relative_error * expected_double);
               is_sorta_equal = actual_double >= expected_double - allowed_absolute_error &&
                                actual_double <= expected_double + allowed_absolute_error;
            }
            if (!is_sorta_equal) {
               test_error("test-vector array element %d failed inexact float32 match. Actual: %f Expected: %f",
                          (int)i,
                          actual_double,
                          expected_double);
            }
         }

      } else if (BSON_ITER_HOLDS_VECTOR_PACKED_BIT(&iter)) {
         // packed_bit special case: The tests for packed_bit aren't actually testing vector-to-array conversion as we
         // understand it, they're operating on bytes rather than elements. This is the inverse of
         // append_vector_packed_bit_from_packed_array() above, and it bypasses the vector-to-array conversion.
         // 'array_from_vector' is ignored on this path.
         // (Spec test improvement TODO)

         bson_iter_t expected_iter;
         ASSERT(bson_iter_init(&expected_iter, test_case->test_vector_array));
         bson_vector_packed_bit_const_view_t actual_view;
         ASSERT(bson_vector_packed_bit_const_view_from_iter(&actual_view, &iter));

         size_t byte_count = 0;
         while (bson_iter_next(&expected_iter)) {
            int64_t expected_byte;
            if (BSON_ITER_HOLDS_INT(&expected_iter)) {
               expected_byte = bson_iter_as_int64(&expected_iter);
            } else {
               test_error("test-vector array element %d has unexpected type, should be int.", (int)byte_count);
            }

            // Note, the zero initializer is only needed due to a false positive -Wmaybe-uninitialized warning in
            // uncommon configurations where the compiler does not have visibility into memcpy().
            uint8_t actual_byte = 0;
            ASSERT(bson_vector_packed_bit_const_view_read_packed(actual_view, &actual_byte, 1, byte_count));

            if (expected_byte != (int64_t)actual_byte) {
               test_error("failed to match packed byte %d of packed_bit test-vector. Actual: 0x%02x Expected: 0x%02x",
                          (int)byte_count,
                          (unsigned)actual_byte,
                          (unsigned)expected_byte);
            }
            byte_count++;
         }
         ASSERT_CMPSIZE_T(byte_count, ==, bson_vector_packed_bit_const_view_length_bytes(actual_view));

      } else {
         // No special case, expect an exact match. (Used for int8 vectors)
         if (0 != bson_compare(&array_from_vector, test_case->test_vector_array)) {
            test_error("bson_binary_vector JSON scenario '%s' test '%s' did not exactly match the reference array "
                       "after array-from-vector.\n "
                       "Actual: %s\n Expected: %s",
                       test_case->scenario_description,
                       test_case->test_description,
                       tmp_json(&array_from_vector),
                       tmp_json(test_case->test_vector_array));
         }
      }

      bson_destroy(&array_from_vector);
   } else {
      /*
       * "To prove correct in an invalid case (valid:false), one MUST
       * - if the vector field is present, raise an exception when attempting to encode a document from the numeric
       * values, dtype, and padding.
       * - if the canonical_bson field is present, raise an exception when attempting to deserialize it into the
       * corresponding numeric values, as the field contains corrupted data."
       */

      if (vector_from_array_ok) {
         test_error("bson_binary_vector JSON scenario '%s' test '%s' should be invalid but vector-from-array "
                    "succeeded with result: %s",
                    test_case->scenario_description,
                    test_case->test_description,
                    tmp_json(&vector_from_array));
      }

      if (test_case->test_canonical_bson_str) {
         bson_t array_from_vector = BSON_INITIALIZER;
         bson_iter_t iter;
         ASSERT(bson_iter_init_find(&iter, &expected_bson, test_case->scenario_test_key));
         if (BSON_APPEND_ARRAY_FROM_VECTOR(&array_from_vector, "should_fail", &iter)) {
            test_error("bson_binary_vector JSON scenario '%s' test '%s' should be invalid but array-from-vector "
                       "succeeded with result: %s",
                       test_case->scenario_description,
                       test_case->test_description,
                       tmp_json(&array_from_vector));
         }
         bson_destroy(&array_from_vector);
      }
   }

   bson_destroy(&expected_bson);
}

// callback for install_json_test_suite_with_check, implements JSON spec tests
static void
test_bson_vector_json_cb(void *test_arg)
{
   BSON_ASSERT_PARAM(test_arg);
   bson_t *scenario = (bson_t *)test_arg;
   bson_error_t error;
   vector_json_test_case_t test_case;

   bson_parser_t *scenario_opts = bson_parser_new();
   bson_t *tests;
   bson_parser_utf8(scenario_opts, "description", &test_case.scenario_description);
   bson_parser_utf8(scenario_opts, "test_key", &test_case.scenario_test_key);
   bson_parser_array(scenario_opts, "tests", &tests);
   if (!bson_parser_parse(scenario_opts, scenario, &error)) {
      test_error("format error in bson_binary_vector JSON scenario: %s", error.message);
   }

   bson_iter_t tests_iter;
   ASSERT(bson_iter_init(&tests_iter, tests));
   while (bson_iter_next(&tests_iter)) {
      bson_t test_subdoc;
      bson_iter_bson(&tests_iter, &test_subdoc);

      bson_parser_t *test_opts = bson_parser_new();
      bson_parser_utf8(test_opts, "description", &test_case.test_description);
      bson_parser_bool(test_opts, "valid", &test_case.test_valid);
      bson_parser_array_optional(test_opts, "vector", &test_case.test_vector_array);
      bson_parser_utf8(test_opts, "dtype_hex", &test_case.test_dtype_hex_str);
      bson_parser_utf8(test_opts, "dtype_alias", &test_case.test_dtype_alias_str);
      bson_parser_int_optional(test_opts, "padding", &test_case.test_padding);
      bson_parser_utf8_optional(test_opts, "canonical_bson", &test_case.test_canonical_bson_str);
      if (!bson_parser_parse(test_opts, &test_subdoc, &error)) {
         test_error(
            "format error in bson_binary_vector JSON test for '%s': %s", test_case.scenario_description, error.message);
      }

      if (test_suite_debug_output()) {
         printf("bson_binary_vector JSON scenario '%s' test '%s'\n",
                test_case.scenario_description,
                test_case.test_description);
      }

      test_bson_vector_json_case(&test_case);
      bson_parser_destroy_with_parsed_fields(test_opts);
   }
   bson_parser_destroy_with_parsed_fields(scenario_opts);
}

static void
test_bson_vector_view_api_usage_int8(void)
{
   bson_t doc = BSON_INITIALIZER;

   // Construct a small vector by writing individual elements
   {
      bson_vector_int8_view_t view;
      const size_t length = 25;
      ASSERT(BSON_APPEND_VECTOR_INT8_UNINIT(&doc, "vector", length, &view));
      for (size_t i = 0; i < length; i++) {
         int8_t v = (int8_t)i - 9;
         bson_vector_int8_view_write(view, &v, 1, i);
      }
   }

   ASSERT_CMPJSON(
      bson_as_canonical_extended_json(&doc, NULL),
      BSON_STR({"vector" : {"$binary" : {"base64" : "AwD3+Pn6+/z9/v8AAQIDBAUGBwgJCgsMDQ4P", "subType" : "09"}}}));

   // Construct a longer vector by writing individual elements
   {
      bson_vector_int8_view_t view;
      const size_t length = 50000;
      ASSERT(bson_append_vector_int8_uninit(&doc, "longer_vector", -1, length, &view));
      for (size_t i = 0; i < length; i++) {
         int8_t v = (int8_t)(uint8_t)i;
         bson_vector_int8_view_write(view, &v, 1, i);
      }
   }

   // Fail appending a vector that would be too large to represent
   {
      bson_vector_int8_view_t view;
      const size_t length = UINT32_MAX - 1;
      ASSERT(!bson_append_vector_int8_uninit(&doc, "overlong_vector", -1, length, &view));
   }

   // Use a mutable view to partially overwrite "vector"
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      bson_vector_int8_view_t view;
      ASSERT(bson_vector_int8_view_from_iter(&view, &iter));
      ASSERT(bson_vector_int8_view_length(view) == 25);
      int8_t values[5] = {12, 34, 56, 78, 90};
      ASSERT(bson_vector_int8_view_write(view, values, sizeof values / sizeof values[0], 3));
   }

   // Read the modified small vector into an int8_t array
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      ASSERT(BSON_ITER_HOLDS_BINARY(&iter));
      ASSERT(BSON_ITER_HOLDS_VECTOR(&iter));
      ASSERT(BSON_ITER_HOLDS_VECTOR_INT8(&iter));
      ASSERT(!BSON_ITER_HOLDS_VECTOR_FLOAT32(&iter));
      ASSERT(!BSON_ITER_HOLDS_VECTOR_PACKED_BIT(&iter));
      bson_vector_int8_const_view_t view;
      ASSERT(bson_vector_int8_const_view_from_iter(&view, &iter));
      ASSERT(bson_vector_int8_const_view_length(view) == 25);
      int8_t values[25];
      static const int8_t expected_values[25] = {-9, -8, -7, 12, 34, 56, 78, 90, -1, 0,  1,  2, 3,
                                                 4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15};
      ASSERT(bson_vector_int8_const_view_read(view, values, sizeof values / sizeof values[0], 0));
      ASSERT_MEMCMP(values, expected_values, (int)sizeof expected_values);
   }

   // Convert the small vector to a BSON Array, and check the resulting canonical extended JSON.
   // Each element will be losslessly converted to int32.
   // Convert the output back, and add a "round_trip" key to the original document.
   {
      bson_t converted = BSON_INITIALIZER;
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      ASSERT(BSON_APPEND_ARRAY_FROM_VECTOR(&converted, "array", &iter));
      ASSERT_CMPJSON(bson_as_canonical_extended_json(&converted, NULL), BSON_STR({
                        "array" : [
                           {"$numberInt" : "-9"}, {"$numberInt" : "-8"}, {"$numberInt" : "-7"}, {"$numberInt" : "12"},
                           {"$numberInt" : "34"}, {"$numberInt" : "56"}, {"$numberInt" : "78"}, {"$numberInt" : "90"},
                           {"$numberInt" : "-1"}, {"$numberInt" : "0"},  {"$numberInt" : "1"},  {"$numberInt" : "2"},
                           {"$numberInt" : "3"},  {"$numberInt" : "4"},  {"$numberInt" : "5"},  {"$numberInt" : "6"},
                           {"$numberInt" : "7"},  {"$numberInt" : "8"},  {"$numberInt" : "9"},  {"$numberInt" : "10"},
                           {"$numberInt" : "11"}, {"$numberInt" : "12"}, {"$numberInt" : "13"}, {"$numberInt" : "14"},
                           {"$numberInt" : "15"}
                        ]
                     }));

      ASSERT(bson_iter_init_find(&iter, &converted, "array"));
      ASSERT(BSON_ITER_HOLDS_ARRAY(&iter));
      ASSERT(bson_iter_recurse(&iter, &iter));
      ASSERT(BSON_APPEND_VECTOR_INT8_FROM_ARRAY(&doc, "round_trip", &iter, NULL));
      bson_destroy(&converted);
   }

   // The original small vector and round_trip small vector must be identical
   {
      bson_iter_t a, b;
      ASSERT(bson_iter_init_find(&a, &doc, "vector"));
      ASSERT(bson_iter_init_find(&b, &doc, "round_trip"));
      ASSERT(bson_iter_binary_equal(&a, &b));
   }

   // Try the same round trip conversion with our longer vector
   // (Note that BSON arrays special-case keys below "1000")
   {
      bson_t converted = BSON_INITIALIZER;
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "longer_vector"));
      ASSERT(BSON_APPEND_ARRAY_FROM_VECTOR(&converted, "array", &iter));
      ASSERT(bson_iter_init_find(&iter, &converted, "array"));
      ASSERT(BSON_ITER_HOLDS_ARRAY(&iter));
      ASSERT(bson_iter_recurse(&iter, &iter));
      ASSERT(BSON_APPEND_VECTOR_INT8_FROM_ARRAY(&doc, "longer_round_trip", &iter, NULL));
      bson_destroy(&converted);
   }
   {
      bson_iter_t a, b;
      ASSERT(bson_iter_init_find(&a, &doc, "longer_vector"));
      ASSERT(bson_iter_init_find(&b, &doc, "longer_round_trip"));
      ASSERT(bson_iter_binary_equal(&a, &b));
   }

   bson_destroy(&doc);
}

static void
test_bson_vector_view_api_usage_float32(void)
{
   bson_t doc = BSON_INITIALIZER;

   // Construct a small vector by writing individual elements
   {
      bson_vector_float32_view_t view;
      const size_t length = 5;
      ASSERT(BSON_APPEND_VECTOR_FLOAT32_UNINIT(&doc, "vector", length, &view));
      for (size_t i = 0; i < length; i++) {
         float v = 1.0f + 0.25f * (float)i;
         bson_vector_float32_view_write(view, &v, 1, i);
      }
   }

   ASSERT_CMPJSON(
      bson_as_canonical_extended_json(&doc, NULL),
      BSON_STR({"vector" : {"$binary" : {"base64" : "JwAAAIA/AACgPwAAwD8AAOA/AAAAQA==", "subType" : "09"}}}));

   // Construct a longer vector by writing individual elements
   {
      bson_vector_float32_view_t view;
      const size_t length = 10000;
      ASSERT(bson_append_vector_float32_uninit(&doc, "longer_vector", -1, length, &view));
      for (size_t i = 0; i < length; i++) {
         float v = (float)i;
         bson_vector_float32_view_write(view, &v, 1, i);
      }
   }

   // Fail appending a vector that would be too large to represent
   {
      bson_vector_float32_view_t view;
      const size_t length = (UINT32_MAX - 1) / 4;
      ASSERT(!bson_append_vector_float32_uninit(&doc, "overlong_vector", -1, length, &view));
   }

   // Read the small vector into a float array
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      ASSERT(BSON_ITER_HOLDS_BINARY(&iter));
      ASSERT(BSON_ITER_HOLDS_VECTOR(&iter));
      ASSERT(!BSON_ITER_HOLDS_VECTOR_INT8(&iter));
      ASSERT(BSON_ITER_HOLDS_VECTOR_FLOAT32(&iter));
      ASSERT(!BSON_ITER_HOLDS_VECTOR_PACKED_BIT(&iter));
      bson_vector_float32_const_view_t view;
      ASSERT(bson_vector_float32_const_view_from_iter(&view, &iter));
      ASSERT(bson_vector_float32_const_view_length(view) == 5);
      float values[5];
      static const float expected_values[5] = {1.0f, 1.25f, 1.5f, 1.75f, 2.0f};
      ASSERT(bson_vector_float32_const_view_read(view, values, sizeof values / sizeof values[0], 0));
      ASSERT_MEMCMP(values, expected_values, (int)sizeof expected_values);
   }

   // Convert the small vector to a BSON Array, and check the resulting canonical extended JSON.
   // Each element will be converted from 32-bit to 64-bit float.
   // Convert the output back, and add a "round_trip" key to the original document.
   {
      bson_t converted = BSON_INITIALIZER;
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      ASSERT(BSON_APPEND_ARRAY_FROM_VECTOR(&converted, "array", &iter));
      ASSERT_CMPJSON(bson_as_canonical_extended_json(&converted, NULL), BSON_STR({
                        "array" : [
                           {"$numberDouble" : "1.0"},
                           {"$numberDouble" : "1.25"},
                           {"$numberDouble" : "1.5"},
                           {"$numberDouble" : "1.75"},
                           {"$numberDouble" : "2.0"}
                        ]
                     }));

      ASSERT(bson_iter_init_find(&iter, &converted, "array"));
      ASSERT(BSON_ITER_HOLDS_ARRAY(&iter));
      ASSERT(bson_iter_recurse(&iter, &iter));
      ASSERT(BSON_APPEND_VECTOR_FLOAT32_FROM_ARRAY(&doc, "round_trip", &iter, NULL));
      bson_destroy(&converted);
   }

   // The original small vector and round_trip small vector must be identical
   {
      bson_iter_t a, b;
      ASSERT(bson_iter_init_find(&a, &doc, "vector"));
      ASSERT(bson_iter_init_find(&b, &doc, "round_trip"));
      ASSERT(bson_iter_binary_equal(&a, &b));
   }

   // Try the same round trip conversion with our longer vector
   // (Note that BSON arrays special-case keys below "1000")
   {
      bson_t converted = BSON_INITIALIZER;
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "longer_vector"));
      ASSERT(BSON_APPEND_ARRAY_FROM_VECTOR(&converted, "array", &iter));
      ASSERT(bson_iter_init_find(&iter, &converted, "array"));
      ASSERT(BSON_ITER_HOLDS_ARRAY(&iter));
      ASSERT(bson_iter_recurse(&iter, &iter));
      ASSERT(BSON_APPEND_VECTOR_FLOAT32_FROM_ARRAY(&doc, "longer_round_trip", &iter, NULL));
      bson_destroy(&converted);
   }
   {
      bson_iter_t a, b;
      ASSERT(bson_iter_init_find(&a, &doc, "longer_vector"));
      ASSERT(bson_iter_init_find(&b, &doc, "longer_round_trip"));
      ASSERT(bson_iter_binary_equal(&a, &b));
   }

   bson_destroy(&doc);
}

static void
test_bson_vector_view_api_usage_packed_bit(void)
{
   bson_t doc = BSON_INITIALIZER;

   // Construct a small vector by packing individual elements from a 'bool' source
   {
      bson_vector_packed_bit_view_t view;
      const size_t length = 123;
      ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "vector", length, &view));
      for (size_t i = 0; i < length; i++) {
         bool v = (i & 1) != 0;
         bson_vector_packed_bit_view_pack_bool(view, &v, 1, i);
      }
   }

   ASSERT_CMPJSON(bson_as_canonical_extended_json(&doc, NULL),
                  BSON_STR({"vector" : {"$binary" : {"base64" : "EAVVVVVVVVVVVVVVVVVVVVVA", "subType" : "09"}}}));

   // Construct a longer vector by packing individual elements from a 'bool' source
   {
      bson_vector_packed_bit_view_t view;
      const size_t length = 100002;
      ASSERT(bson_append_vector_packed_bit_uninit(&doc, "longer_vector", -1, length, &view));
      for (size_t i = 0; i < length; i++) {
         bool v = (i & 3) != 0;
         bson_vector_packed_bit_view_pack_bool(view, &v, 1, i);
      }
   }

   // Fail appending a vector that would be too large to represent
   {
      bson_vector_int8_view_t view;
      const size_t length = (UINT32_MAX - 1) * (size_t)8;
      ASSERT(!bson_append_vector_int8_uninit(&doc, "overlong_vector", -1, length, &view));
   }

   // Unpack the small vector into a bool array
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      ASSERT(BSON_ITER_HOLDS_BINARY(&iter));
      ASSERT(BSON_ITER_HOLDS_VECTOR(&iter));
      ASSERT(!BSON_ITER_HOLDS_VECTOR_INT8(&iter));
      ASSERT(!BSON_ITER_HOLDS_VECTOR_FLOAT32(&iter));
      ASSERT(BSON_ITER_HOLDS_VECTOR_PACKED_BIT(&iter));
      bson_vector_packed_bit_const_view_t view;
      ASSERT(bson_vector_packed_bit_const_view_from_iter(&view, &iter));
      ASSERT(bson_vector_packed_bit_const_view_length(view) == 123);
      bool values[123];
      bool expected_values[123];
      for (size_t i = 0; i < sizeof expected_values / sizeof expected_values[0]; i++) {
         expected_values[i] = (i & 1) != 0;
      }
      ASSERT(bson_vector_packed_bit_const_view_unpack_bool(view, values, sizeof values / sizeof values[0], 0));
      ASSERT_MEMCMP(values, expected_values, (int)sizeof expected_values);
   }

   // Read the packed representation without unpacking
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      bson_vector_packed_bit_const_view_t view;
      ASSERT(bson_vector_packed_bit_const_view_from_iter(&view, &iter));
      ASSERT(bson_vector_packed_bit_const_view_length(view) == 123);
      uint8_t packed[16];
      static const uint8_t expected_packed[16] = {
         0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x40};
      ASSERT(bson_vector_packed_bit_const_view_read_packed(view, packed, sizeof packed, 0));
      ASSERT_MEMCMP(packed, expected_packed, (int)sizeof expected_packed);
   }

   // Partial overwrite of the packed representation
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      bson_vector_packed_bit_view_t view;
      ASSERT(bson_vector_packed_bit_view_from_iter(&view, &iter));
      uint8_t packed[2] = {0x12, 0x34};
      ASSERT(bson_vector_packed_bit_view_write_packed(view, packed, sizeof packed, 12));
   }

   // Partial read of the packed representation
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      bson_vector_packed_bit_const_view_t view;
      ASSERT(bson_vector_packed_bit_const_view_from_iter(&view, &iter));
      uint8_t packed[5];
      static const uint8_t expected_packed[5] = {0x55, 0x12, 0x34, 0x55, 0x40};
      ASSERT(bson_vector_packed_bit_const_view_read_packed(view, packed, sizeof packed, 11));
      ASSERT_MEMCMP(packed, expected_packed, (int)sizeof expected_packed);
   }

   // Partial write from a bool array, spanning complete and partial packed bytes
   {
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      bson_vector_packed_bit_view_t view;
      ASSERT(bson_vector_packed_bit_view_from_iter(&view, &iter));
      bool values[24] = {
         false, false, false, true,  false, false, false, true,  true,  true,  false, true,
         true,  false, false, false, false, true,  false, false, false, false, false, true,
      };
      ASSERT(bson_vector_packed_bit_view_pack_bool(view, values, sizeof values / sizeof values[0], 3));
   }

   // Convert the small vector to a BSON Array, and check the resulting canonical extended JSON.
   // Each element will be losslessly converted to int32.
   // Convert the output back, and add a "round_trip" key to the original document.
   {
      bson_t converted = BSON_INITIALIZER;
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "vector"));
      ASSERT(BSON_APPEND_ARRAY_FROM_VECTOR(&converted, "array", &iter));
      ASSERT_CMPJSON(bson_as_canonical_extended_json(&converted, NULL), BSON_STR({
                        "array" : [
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "0"},
                           {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"},
                           {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "1"},
                           {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "0"},
                           {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "0"},
                           {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"},
                           {"$numberInt" : "0"}, {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "0"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}, {"$numberInt" : "1"},
                           {"$numberInt" : "0"}, {"$numberInt" : "1"}, {"$numberInt" : "0"}
                        ]
                     }));

      ASSERT(bson_iter_init_find(&iter, &converted, "array"));
      ASSERT(BSON_ITER_HOLDS_ARRAY(&iter));
      ASSERT(bson_iter_recurse(&iter, &iter));
      ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_FROM_ARRAY(&doc, "round_trip", &iter, NULL));
      bson_destroy(&converted);
   }

   // The original small vector and round_trip small vector must be identical
   {
      bson_iter_t a, b;
      ASSERT(bson_iter_init_find(&a, &doc, "vector"));
      ASSERT(bson_iter_init_find(&b, &doc, "round_trip"));
      ASSERT(bson_iter_binary_equal(&a, &b));
   }

   // Try the same round trip conversion with our longer vector
   // (Note that BSON arrays special-case keys below "1000")
   {
      bson_t converted = BSON_INITIALIZER;
      bson_iter_t iter;
      ASSERT(bson_iter_init_find(&iter, &doc, "longer_vector"));
      ASSERT(BSON_APPEND_ARRAY_FROM_VECTOR(&converted, "array", &iter));
      ASSERT(bson_iter_init_find(&iter, &converted, "array"));
      ASSERT(BSON_ITER_HOLDS_ARRAY(&iter));
      ASSERT(bson_iter_recurse(&iter, &iter));
      ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_FROM_ARRAY(&doc, "longer_round_trip", &iter, NULL));
      bson_destroy(&converted);
   }
   {
      bson_iter_t a, b;
      ASSERT(bson_iter_init_find(&a, &doc, "longer_vector"));
      ASSERT(bson_iter_init_find(&b, &doc, "longer_round_trip"));
      ASSERT(bson_iter_binary_equal(&a, &b));
   }

   // Padding bits will be initialized to zero when a packed_bit vector is first allocated by
   // bson_append_vector_packed_bit_uninit
   {
      // Set the uninitialized part of 'doc' to a known value
      static const uint32_t reserve_len = 512;
      memset(bson_reserve_buffer(&doc, doc.len + reserve_len) + doc.len, 0xdd, reserve_len);

      bson_vector_packed_bit_view_t view;
      ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "padding_init_test", 12, &view));
      ASSERT(bson_vector_packed_bit_view_length_bytes(view) == 2);
      ASSERT(bson_vector_packed_bit_view_padding(view) == 4);

      // BSON validity only requires the low 4 bits to be zero, but the entire last
      // byte will be zeroed by our implementation.
      uint8_t bytes[2];
      ASSERT(bson_vector_packed_bit_view_read_packed(view, bytes, sizeof bytes, 0));
      ASSERT_CMPUINT((unsigned)bytes[0], ==, 0xdd);
      ASSERT_CMPUINT((unsigned)bytes[1], ==, 0x00);
   }

   // Padding bits can't be forcibly given nonzero values using bson_vector_packed_bit_view_write_packed
   {
      bson_vector_packed_bit_view_t view;
      ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "padding_mask_test", 13, &view));
      ASSERT(bson_vector_packed_bit_view_length_bytes(view) == 2);
      ASSERT(bson_vector_packed_bit_view_padding(view) == 3);

      uint8_t bytes[2] = {0xff, 0xff};
      ASSERT(bson_vector_packed_bit_view_write_packed(view, bytes, sizeof bytes, 0));
      ASSERT(bson_vector_packed_bit_view_read_packed(view, bytes, sizeof bytes, 0));
      ASSERT_CMPUINT((unsigned)bytes[0], ==, 0xff);
      ASSERT_CMPUINT((unsigned)bytes[1], ==, 0xf8);
   }

   bson_destroy(&doc);
}

// Note: The effective MAX_TESTED_VECTOR_LENGTH is limited to 2047 on Windows due to RAND_MAX==0x7fff
#define MAX_TESTED_VECTOR_LENGTH 10000
#define FUZZ_TEST_ITERS 5000

static void
test_bson_vector_view_api_fuzz_int8(void)
{
   size_t current_length = 0;
   bson_t vector_doc = BSON_INITIALIZER;
   int8_t *expected_elements = BSON_ARRAY_ALLOC(MAX_TESTED_VECTOR_LENGTH, int8_t);
   int8_t *actual_elements = BSON_ARRAY_ALLOC(MAX_TESTED_VECTOR_LENGTH, int8_t);
   for (int fuzz_iter = 0; fuzz_iter < FUZZ_TEST_ITERS; fuzz_iter++) {
      unsigned r = (unsigned)rand();
      unsigned r_operation = r & 0xFu;
      size_t r_param = r >> 4;

      if (current_length == 0 || r_operation == 15) {
         // Resize and fill
         size_t new_length = (size_t)r_param % MAX_TESTED_VECTOR_LENGTH;
         bson_reinit(&vector_doc);
         bson_vector_int8_view_t view;
         ASSERT(BSON_APPEND_VECTOR_INT8_UNINIT(&vector_doc, "vector", new_length, &view));
         for (size_t i = 0; i < new_length; i++) {
            expected_elements[i] = (int8_t)(uint8_t)rand();
         }
         ASSERT(bson_vector_int8_view_write(view, expected_elements, new_length, 0));
         current_length = new_length;

      } else if (r_operation < 7) {
         // Partial write
         size_t element_count = r_param % current_length;
         size_t offset = (size_t)rand() % (current_length - element_count);
         for (size_t i = 0; i < element_count; i++) {
            expected_elements[offset + i] = (int8_t)(uint8_t)rand();
         }
         bson_vector_int8_view_t view;
         bson_iter_t iter;
         ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
         ASSERT(bson_vector_int8_view_from_iter(&view, &iter));
         ASSERT(bson_vector_int8_view_write(view, expected_elements + offset, element_count, offset));

      } else {
         // Partial read
         size_t element_count = r_param % current_length;
         size_t offset = (size_t)rand() % (current_length - element_count);
         bson_vector_int8_const_view_t view;
         bson_iter_t iter;
         ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
         ASSERT(bson_vector_int8_const_view_from_iter(&view, &iter));
         ASSERT(bson_vector_int8_const_view_read(view, actual_elements, element_count, offset));
         ASSERT_MEMCMP(actual_elements, expected_elements + offset, element_count * sizeof *actual_elements);
      }
   }
   bson_destroy(&vector_doc);
   bson_free(expected_elements);
   bson_free(actual_elements);
}

static void
test_bson_vector_view_api_fuzz_float32(void)
{
   size_t current_length = 0;
   bson_t vector_doc = BSON_INITIALIZER;
   float *expected_elements = BSON_ARRAY_ALLOC(MAX_TESTED_VECTOR_LENGTH, float);
   float *actual_elements = BSON_ARRAY_ALLOC(MAX_TESTED_VECTOR_LENGTH, float);
   for (int fuzz_iter = 0; fuzz_iter < FUZZ_TEST_ITERS; fuzz_iter++) {
      unsigned r = (unsigned)rand();
      unsigned r_operation = r & 0xFu;
      size_t r_param = r >> 4;

      if (current_length == 0 || r_operation == 15) {
         // Resize and fill
         size_t new_length = (size_t)r_param % MAX_TESTED_VECTOR_LENGTH;
         bson_reinit(&vector_doc);
         bson_vector_float32_view_t view;
         ASSERT(BSON_APPEND_VECTOR_FLOAT32_UNINIT(&vector_doc, "vector", new_length, &view));
         for (size_t i = 0; i < new_length; i++) {
            expected_elements[i] = (float)rand();
         }
         ASSERT(bson_vector_float32_view_write(view, expected_elements, new_length, 0));
         current_length = new_length;

      } else if (r_operation < 7) {
         // Partial write
         size_t element_count = r_param % current_length;
         size_t offset = (size_t)rand() % (current_length - element_count);
         for (size_t i = 0; i < element_count; i++) {
            expected_elements[offset + i] = (float)rand();
         }
         bson_vector_float32_view_t view;
         bson_iter_t iter;
         ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
         ASSERT(bson_vector_float32_view_from_iter(&view, &iter));
         ASSERT(bson_vector_float32_view_write(view, expected_elements + offset, element_count, offset));

      } else {
         // Partial read
         size_t element_count = r_param % current_length;
         size_t offset = (size_t)rand() % (current_length - element_count);
         bson_vector_float32_const_view_t view;
         bson_iter_t iter;
         ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
         ASSERT(bson_vector_float32_const_view_from_iter(&view, &iter));
         ASSERT(bson_vector_float32_const_view_read(view, actual_elements, element_count, offset));
         ASSERT_MEMCMP(actual_elements, expected_elements + offset, element_count * sizeof *actual_elements);
      }
   }
   bson_destroy(&vector_doc);
   bson_free(expected_elements);
   bson_free(actual_elements);
}

static void
test_bson_vector_view_api_fuzz_packed_bit(void)
{
   size_t current_length = 0;
   bson_t vector_doc = BSON_INITIALIZER;
   bool *expected_elements = BSON_ARRAY_ALLOC(MAX_TESTED_VECTOR_LENGTH, bool);
   bool *actual_elements = BSON_ARRAY_ALLOC(MAX_TESTED_VECTOR_LENGTH, bool);
   uint8_t *packed_buffer = bson_malloc((MAX_TESTED_VECTOR_LENGTH + 7) / 8);
   for (int fuzz_iter = 0; fuzz_iter < FUZZ_TEST_ITERS; fuzz_iter++) {
      unsigned r = (unsigned)rand();
      unsigned r_operation = r & 0xFu;
      size_t r_param = r >> 4;

      if (current_length == 0 || r_operation == 15) {
         // Resize and fill from unpacked bool source
         size_t new_length = (size_t)r_param % MAX_TESTED_VECTOR_LENGTH;
         bson_reinit(&vector_doc);
         bson_vector_packed_bit_view_t view;
         ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&vector_doc, "vector", new_length, &view));
         for (size_t i = 0; i < new_length; i++) {
            expected_elements[i] = ((unsigned)rand() & 1u) != 0u;
         }
         ASSERT(bson_vector_packed_bit_view_pack_bool(view, expected_elements, new_length, 0));
         current_length = new_length;

      } else if (r_operation < 7) {
         // Partial write
         if (r_operation & 1) {
            // Partial write from unpacked bool source
            size_t element_count = r_param % current_length;
            size_t offset = (size_t)rand() % (current_length - element_count);
            for (size_t i = 0; i < element_count; i++) {
               expected_elements[offset + i] = ((unsigned)rand() & 1u) != 0u;
            }
            bson_vector_packed_bit_view_t view;
            bson_iter_t iter;
            ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
            ASSERT(bson_vector_packed_bit_view_from_iter(&view, &iter));
            ASSERT(bson_vector_packed_bit_view_length(view) == current_length);
            ASSERT(bson_vector_packed_bit_view_pack_bool(view, expected_elements + offset, element_count, offset));
         } else {
            // Partial write of packed bytes
            size_t current_length_bytes = (current_length + 7) / 8;
            size_t byte_count = r_param % current_length_bytes;
            size_t byte_offset = (size_t)rand() % (current_length_bytes - byte_count);
            for (size_t i = 0; i < byte_count; i++) {
               uint8_t packed_byte = (uint8_t)rand();
               packed_buffer[i] = packed_byte;
               for (unsigned bit = 0; bit < 8; bit++) {
                  expected_elements[(byte_offset + i) * 8 + bit] = (packed_byte & (0x80 >> bit)) != 0;
               }
            }
            bson_vector_packed_bit_view_t view;
            bson_iter_t iter;
            ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
            ASSERT(bson_vector_packed_bit_view_from_iter(&view, &iter));
            ASSERT(bson_vector_packed_bit_view_length(view) == current_length);
            ASSERT(bson_vector_packed_bit_view_length_bytes(view) == current_length_bytes);
            ASSERT(bson_vector_packed_bit_view_write_packed(view, packed_buffer, byte_count, byte_offset));
         }
      } else {
         // Partial read
         if (r_operation & 1) {
            // Partial read to unpacked bool destination
            size_t element_count = r_param % current_length;
            size_t offset = (size_t)rand() % (current_length - element_count);
            bson_vector_packed_bit_const_view_t view;
            bson_iter_t iter;
            ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
            ASSERT(bson_vector_packed_bit_const_view_from_iter(&view, &iter));
            ASSERT(bson_vector_packed_bit_const_view_length(view) == current_length);
            ASSERT(bson_vector_packed_bit_const_view_unpack_bool(view, actual_elements, element_count, offset));
            ASSERT_MEMCMP(actual_elements, expected_elements + offset, element_count * sizeof *actual_elements);
         } else {
            // Partial read of packed bytes
            size_t current_length_bytes = (current_length + 7) / 8;
            size_t byte_count = r_param % current_length_bytes;
            size_t byte_offset = (size_t)rand() % (current_length_bytes - byte_count);
            bson_vector_packed_bit_const_view_t view;
            bson_iter_t iter;
            ASSERT(bson_iter_init_find(&iter, &vector_doc, "vector"));
            ASSERT(bson_vector_packed_bit_const_view_from_iter(&view, &iter));
            ASSERT(bson_vector_packed_bit_const_view_length(view) == current_length);
            ASSERT(bson_vector_packed_bit_const_view_length_bytes(view) == current_length_bytes);
            ASSERT(bson_vector_packed_bit_const_view_read_packed(view, packed_buffer, byte_count, byte_offset));
            for (size_t i = 0; i < byte_count; i++) {
               uint8_t packed_byte = packed_buffer[i];
               for (unsigned bit = 0; bit < 8; bit++) {
                  ASSERT(expected_elements[(byte_offset + i) * 8 + bit] == ((packed_byte & (0x80 >> bit)) != 0));
               }
            }
         }
      }
   }
   bson_destroy(&vector_doc);
   bson_free(expected_elements);
   bson_free(actual_elements);
   bson_free(packed_buffer);
}

static void
test_bson_vector_example_int8_const_view(void)
{
   // setup: construct a sample document
   bson_t doc = BSON_INITIALIZER;
   {
      static const int8_t values[] = {12, 34, -56};
      bson_vector_int8_view_t view;
      ASSERT(BSON_APPEND_VECTOR_INT8_UNINIT(&doc, "vector", sizeof values / sizeof values[0], &view));
      ASSERT(bson_vector_int8_view_write(view, values, sizeof values / sizeof values[0], 0));
   }

   // bson_vector_int8_const_view_t.rst
   // Edits:
   //  - Added test_suite_debug_output() test.
   //  - Added unnecessary zero initializer to work around false positive compiler warning.
   //    (same as in bson_array_builder_append_vector_int8_elements)
   {
      bson_iter_t iter;
      bson_vector_int8_const_view_t view;

      if (bson_iter_init_find(&iter, &doc, "vector") && bson_vector_int8_const_view_from_iter(&view, &iter)) {
         size_t length = bson_vector_int8_const_view_length(view);
         if (test_suite_debug_output()) {
            printf("Elements in 'vector':\n");
         }
         for (size_t i = 0; i < length; i++) {
            int8_t element = 0; // Workaround
            ASSERT(bson_vector_int8_const_view_read(view, &element, 1, i));
            if (test_suite_debug_output()) {
               printf(" [%d] = %d\n", (int)i, (int)element);
            }
         }
      }
   }

   bson_destroy(&doc);
}

static void
test_bson_vector_example_int8_view(void)
{
   bson_t doc = BSON_INITIALIZER;

   // bson_vector_int8_view_t.rst
   {
      static const int8_t values[] = {1, 2, 3};
      const size_t values_count = sizeof values / sizeof values[0];

      bson_vector_int8_view_t view;
      ASSERT(BSON_APPEND_VECTOR_INT8_UNINIT(&doc, "vector", values_count, &view));
      ASSERT(bson_vector_int8_view_write(view, values, values_count, 0));
   }

   bson_destroy(&doc);
}

static void
test_bson_vector_example_float32_const_view(void)
{
   // setup: construct a sample document
   bson_t doc = BSON_INITIALIZER;
   {
      const float values[] = {5.0f, -1e10f, INFINITY, NAN, -1.0f}; // C2099 "initializer is not a constant"
      bson_vector_float32_view_t view;
      ASSERT(BSON_APPEND_VECTOR_FLOAT32_UNINIT(&doc, "vector", sizeof values / sizeof values[0], &view));
      ASSERT(bson_vector_float32_view_write(view, values, sizeof values / sizeof values[0], 0));
   }

   // bson_vector_float32_const_view_t.rst
   // Edits:
   //  - Added test_suite_debug_output() test.
   {
      bson_iter_t iter;
      bson_vector_float32_const_view_t view;

      if (bson_iter_init_find(&iter, &doc, "vector") && bson_vector_float32_const_view_from_iter(&view, &iter)) {
         size_t length = bson_vector_float32_const_view_length(view);
         if (test_suite_debug_output()) {
            printf("Elements in 'vector':\n");
         }
         for (size_t i = 0; i < length; i++) {
            float element;
            ASSERT(bson_vector_float32_const_view_read(view, &element, 1, i));
            if (test_suite_debug_output()) {
               printf(" [%d] = %f\n", (int)i, element);
            }
         }
      }
   }

   bson_destroy(&doc);
}

static void
test_bson_vector_example_float32_view(void)
{
   bson_t doc = BSON_INITIALIZER;

   // bson_vector_float32_view_t.rst
   {
      static const float values[] = {1.0f, 2.0f, 3.0f};
      const size_t values_count = sizeof values / sizeof values[0];

      bson_vector_float32_view_t view;
      ASSERT(BSON_APPEND_VECTOR_FLOAT32_UNINIT(&doc, "vector", values_count, &view));
      ASSERT(bson_vector_float32_view_write(view, values, values_count, 0));
   }

   bson_destroy(&doc);
}

static void
test_bson_vector_example_packed_bit_const_view(void)
{
   // setup: construct a sample document
   bson_t doc = BSON_INITIALIZER;
   {
      static const bool values[] = {true, false, true, true, false, true, false, true, true, false};
      bson_vector_packed_bit_view_t view;
      ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "vector", sizeof values / sizeof values[0], &view));
      ASSERT(bson_vector_packed_bit_view_pack_bool(view, values, sizeof values / sizeof values[0], 0));
   }

   // bson_vector_packed_bit_const_view_t.rst
   // Edits:
   //  - Added test_suite_debug_output() test.
   //  - Added unnecessary zero initializer to work around false positive compiler warning.
   //    (same as in bson_array_builder_append_vector_int8_elements)
   {
      bson_iter_t iter;
      bson_vector_packed_bit_const_view_t view;

      if (bson_iter_init_find(&iter, &doc, "vector") && bson_vector_packed_bit_const_view_from_iter(&view, &iter)) {
         size_t length = bson_vector_packed_bit_const_view_length(view);
         size_t length_bytes = bson_vector_packed_bit_const_view_length_bytes(view);
         size_t padding = bson_vector_packed_bit_const_view_padding(view);

         if (test_suite_debug_output()) {
            printf("Elements in 'vector':\n");
         }
         for (size_t i = 0; i < length; i++) {
            bool element;
            ASSERT(bson_vector_packed_bit_const_view_unpack_bool(view, &element, 1, i));
            if (test_suite_debug_output()) {
               printf(" elements[%d] = %d\n", (int)i, (int)element);
            }
         }

         if (test_suite_debug_output()) {
            printf("Bytes in 'vector': (%d bits unused)\n", (int)padding);
         }
         for (size_t i = 0; i < length_bytes; i++) {
            uint8_t packed_byte = 0; // Workaround
            ASSERT(bson_vector_packed_bit_const_view_read_packed(view, &packed_byte, 1, i));
            if (test_suite_debug_output()) {
               printf(" bytes[%d] = 0x%02x\n", (int)i, (unsigned)packed_byte);
            }
         }
      }
   }

   bson_destroy(&doc);
}

static void
test_bson_vector_example_packed_bit_view(void)
{
   bson_t doc = BSON_INITIALIZER;

   // bson_vector_packed_bit_view_t.rst
   {
      // Fill a new vector with individual boolean elements
      {
         static const bool bool_values[] = {true, false, true, true, false};
         const size_t bool_values_count = sizeof bool_values / sizeof bool_values[0];

         bson_vector_packed_bit_view_t view;
         ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "from_bool", bool_values_count, &view));
         ASSERT(bson_vector_packed_bit_view_pack_bool(view, bool_values, bool_values_count, 0));
      }

      // Fill another new vector with packed bytes
      {
         static const uint8_t packed_bytes[] = {0xb0};
         const size_t unused_bits_count = 3;
         const size_t packed_values_count = sizeof packed_bytes * 8 - unused_bits_count;

         bson_vector_packed_bit_view_t view;
         ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "from_packed", packed_values_count, &view));
         ASSERT(bson_vector_packed_bit_view_write_packed(view, packed_bytes, sizeof packed_bytes, 0));
      }

      // Compare both vectors. They match exactly.
      {
         bson_iter_t from_bool_iter, from_packed_iter;
         ASSERT(bson_iter_init_find(&from_bool_iter, &doc, "from_bool"));
         ASSERT(bson_iter_init_find(&from_packed_iter, &doc, "from_packed"));
         ASSERT(bson_iter_binary_equal(&from_bool_iter, &from_packed_iter));
      }
   }

   bson_destroy(&doc);
}

// Shared edge case tests that apply to all reader/writer functions.
#define TEST_BSON_VECTOR_RW(_expected, _view, _v, _count, _offset, _read, _write) \
   if (true) {                                                                    \
      ASSERT((_expected) == (_write)((_view), (_v), (_count), (_offset)));        \
      ASSERT((_expected) == (_read)((_view), (_v), (_count), (_offset)));         \
   } else                                                                         \
      ((void)0)

#if defined(__USE_FORTIFY_LEVEL) && __USE_FORTIFY_LEVEL > 0
// Prevent memcpy size overflows even in dead code
#define MAX_TESTABLE_COPY_COUNT (SIZE_MAX / 2u / sizeof(float))
#else
// Allow dead code to contain an oversized or overflowing memcpy
#define MAX_TESTABLE_COPY_COUNT SIZE_MAX
#endif

#define TEST_BSON_VECTOR_EDGE_CASES_RW_COMMON(_view, _alloc_size, _v, _v_size, _read, _write)                  \
   if (true) {                                                                                                 \
      TEST_BSON_VECTOR_RW(false, (_view), (_v), (_alloc_size) + 1u, 0, (_read), (_write));                     \
      TEST_BSON_VECTOR_RW(true, (_view), (_v), (_v_size), (_alloc_size) - (_v_size), (_read), (_write));       \
      TEST_BSON_VECTOR_RW(false, (_view), (_v), (_v_size), (_alloc_size) - (_v_size) + 1u, (_read), (_write)); \
      TEST_BSON_VECTOR_RW(false, (_view), (_v), (_v_size) + 1u, (_alloc_size) - (_v_size), (_read), (_write)); \
      TEST_BSON_VECTOR_RW(                                                                                     \
         false, (_view), (_v), MAX_TESTABLE_COPY_COUNT, (_alloc_size) - (_v_size), (_read), (_write));         \
      TEST_BSON_VECTOR_RW(                                                                                     \
         false, (_view), (_v), MAX_TESTABLE_COPY_COUNT, (_alloc_size) - (_v_size) + 1u, (_read), (_write));    \
      TEST_BSON_VECTOR_RW(true, (_view), (_v), (_v_size), 0, (_read), (_write));                               \
   } else                                                                                                      \
      ((void)0)

static void
test_bson_vector_edge_cases_int8(void)
{
   size_t max_representable_elements = (size_t)UINT32_MAX - BSON_VECTOR_HEADER_LEN;

   // Test binary_data_length (uint32_t) edge cases, without any allocation.
   {
      ASSERT_CMPUINT32(bson_vector_int8_binary_data_length(max_representable_elements - 1u), ==, UINT32_MAX - 1u);
      ASSERT_CMPUINT32(bson_vector_int8_binary_data_length(max_representable_elements), ==, UINT32_MAX);
      ASSERT_CMPUINT32(bson_vector_int8_binary_data_length(max_representable_elements + 1u), ==, 0);
   }

   // Needs little real memory because most bytes are never accessed,
   // but we should require a virtual address space larger than 32 bits.
#if BSON_WORD_SIZE > 32

   size_t expected_bson_overhead =
      5 /* empty bson document */ + 3 /* "v" element header */ + 5 /* binary item header */;
   size_t max_alloc_elements = (size_t)BSON_MAX_SIZE - expected_bson_overhead - BSON_VECTOR_HEADER_LEN;

   bson_t doc = BSON_INITIALIZER;
   bson_vector_int8_view_t view;

   // Test allocation (BSON_MAX_SIZE + uint32_t) edge cases.
   {
      ASSERT(!BSON_APPEND_VECTOR_INT8_UNINIT(&doc, "v", max_representable_elements, &view));
      ASSERT(!BSON_APPEND_VECTOR_INT8_UNINIT(&doc, "v", max_representable_elements + 1u, &view));
      ASSERT(!BSON_APPEND_VECTOR_INT8_UNINIT(&doc, "v", max_alloc_elements + 1u, &view));
      ASSERT(BSON_APPEND_VECTOR_INT8_UNINIT(&doc, "v", max_alloc_elements, &view));
   }

   // Test some read and write boundaries.
   {
      size_t values_size = 100;
      int8_t *values = BSON_ARRAY_ALLOC0(values_size, int8_t);
      TEST_BSON_VECTOR_EDGE_CASES_RW_COMMON(
         view, max_alloc_elements, values, values_size, bson_vector_int8_view_read, bson_vector_int8_view_write);
      bson_free(values);
   }

   bson_destroy(&doc);
#endif // BSON_WORD_SIZE > 32
}

static void
test_bson_vector_edge_cases_float32(void)
{
   size_t max_representable_elements = ((size_t)UINT32_MAX - BSON_VECTOR_HEADER_LEN) / sizeof(float);

   // Test binary_data_length (uint32_t) edge cases, without any allocation.
   // Note that the longest possible multiple of a complete element is 1 byte short of UINT32_MAX.
   {
      ASSERT_CMPUINT32(
         bson_vector_float32_binary_data_length(max_representable_elements - 1u), ==, UINT32_MAX - 1u - 4u);
      ASSERT_CMPUINT32(bson_vector_float32_binary_data_length(max_representable_elements), ==, UINT32_MAX - 1u);
      ASSERT_CMPUINT32(bson_vector_float32_binary_data_length(max_representable_elements + 1u), ==, 0);
   }

   // Needs little real memory because most bytes are never accessed,
   // but we should require a virtual address space larger than 32 bits.
#if BSON_WORD_SIZE > 32

   size_t expected_bson_overhead =
      5 /* empty bson document */ + 3 /* "v" element header */ + 5 /* binary item header */;
   size_t max_alloc_elements =
      ((size_t)BSON_MAX_SIZE - expected_bson_overhead - BSON_VECTOR_HEADER_LEN) / sizeof(float);

   bson_t doc = BSON_INITIALIZER;
   bson_vector_float32_view_t view;

   // Test allocation (BSON_MAX_SIZE + uint32_t) edge cases.
   {
      ASSERT(!BSON_APPEND_VECTOR_FLOAT32_UNINIT(&doc, "v", max_representable_elements, &view));
      ASSERT(!BSON_APPEND_VECTOR_FLOAT32_UNINIT(&doc, "v", max_representable_elements + 1u, &view));
      ASSERT(!BSON_APPEND_VECTOR_FLOAT32_UNINIT(&doc, "v", max_alloc_elements + 1u, &view));
      ASSERT(BSON_APPEND_VECTOR_FLOAT32_UNINIT(&doc, "v", max_alloc_elements, &view));
   }

   // Test some read and write boundaries.
   {
      size_t values_size = 100;
      float *values = BSON_ARRAY_ALLOC0(values_size, float);
      TEST_BSON_VECTOR_EDGE_CASES_RW_COMMON(
         view, max_alloc_elements, values, values_size, bson_vector_float32_view_read, bson_vector_float32_view_write);
      bson_free(values);
   }

   bson_destroy(&doc);
#endif // BSON_WORD_SIZE > 32
}

static void
test_bson_vector_edge_cases_packed_bit(void)
{
   // Test UINT32_MAX as an element count. This is the largest representable on systems with a 32-bit size_t.
   uint32_t len_for_max_count = (uint32_t)(((uint64_t)UINT32_MAX + 7u) / 8u + BSON_VECTOR_HEADER_LEN);
   {
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX), ==, len_for_max_count);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX - 1u), ==, len_for_max_count);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX - 6u), ==, len_for_max_count);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX - 7u), ==, len_for_max_count - 1u);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX - 8u), ==, len_for_max_count - 1u);
   }

   // Test the real max_representable_elements only if size_t is large enough.
#if SIZE_MAX > UINT32_MAX
   size_t max_representable_elements = ((size_t)UINT32_MAX - BSON_VECTOR_HEADER_LEN) * 8u;

   // Test binary_data_length (uint32_t) edge cases, without any allocation.
   {
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX + 1u), ==, len_for_max_count);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX + 2u), ==, len_for_max_count + 1u);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length((size_t)UINT32_MAX + 9u), ==, len_for_max_count + 1u);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length(max_representable_elements - 8u), ==, UINT32_MAX - 1u);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length(max_representable_elements - 7u), ==, UINT32_MAX);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length(max_representable_elements), ==, UINT32_MAX);
      ASSERT_CMPUINT32(bson_vector_packed_bit_binary_data_length(max_representable_elements + 1u), ==, 0);
   }
#endif // SIZE_MAX > UINT32_MAX

   // If we additionally have a 64-bit address space, allocate this max-sized vector and run tests.
   // Needs little real memory because most bytes are never accessed.
#if BSON_WORD_SIZE > 32

#if !(SIZE_MAX > UINT32_MAX)
#error 64-bit platforms should have a 64-bit size_t
#endif

   size_t expected_bson_overhead =
      5 /* empty bson document */ + 3 /* "v" element header */ + 5 /* binary item header */;
   size_t max_alloc_bytes = (size_t)BSON_MAX_SIZE - expected_bson_overhead - BSON_VECTOR_HEADER_LEN;
   size_t max_alloc_elements = max_alloc_bytes * 8u;

   bson_t doc = BSON_INITIALIZER;
   bson_vector_packed_bit_view_t view;

   // Test allocation (BSON_MAX_SIZE + uint32_t) edge cases.
   {
      ASSERT(!BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "v", max_representable_elements, &view));
      ASSERT(!BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "v", max_representable_elements + 1u, &view));
      ASSERT(!BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "v", max_alloc_elements + 1u, &view));
      ASSERT(BSON_APPEND_VECTOR_PACKED_BIT_UNINIT(&doc, "v", max_alloc_elements, &view));
   }

   // Test pack and unpack boundaries with the same tests used for read/write of non-packed element types.
   // Only tests one length, but it's chosen to be greater than 8 and not a multiple of 8.
   {
      size_t values_size = 190;
      bool *values = BSON_ARRAY_ALLOC0(values_size, bool);
      TEST_BSON_VECTOR_EDGE_CASES_RW_COMMON(view,
                                            max_alloc_elements,
                                            values,
                                            values_size,
                                            bson_vector_packed_bit_view_unpack_bool,
                                            bson_vector_packed_bit_view_pack_bool);
      bson_free(values);
   }

   // Test read and write boundaries on packed bytes.
   {
      size_t packed_size = 50;
      uint8_t *packed = bson_malloc0(packed_size);
      TEST_BSON_VECTOR_EDGE_CASES_RW_COMMON(view,
                                            max_alloc_bytes,
                                            packed,
                                            packed_size,
                                            bson_vector_packed_bit_view_read_packed,
                                            bson_vector_packed_bit_view_write_packed);
      bson_free(packed);
   }

   bson_destroy(&doc);
#endif // BSON_WORD_SIZE > 32
}

void
test_bson_vector_install(TestSuite *suite)
{
   install_json_test_suite_with_check(suite, BSON_JSON_DIR, "bson_binary_vector", test_bson_vector_json_cb);

   TestSuite_Add(suite, "/bson_binary_vector/view_api/usage/int8", test_bson_vector_view_api_usage_int8);
   TestSuite_Add(suite, "/bson_binary_vector/view_api/usage/float32", test_bson_vector_view_api_usage_float32);
   TestSuite_Add(suite, "/bson_binary_vector/view_api/usage/packed_bit", test_bson_vector_view_api_usage_packed_bit);

   TestSuite_Add(suite, "/bson_binary_vector/view_api/fuzz/int8", test_bson_vector_view_api_fuzz_int8);
   TestSuite_Add(suite, "/bson_binary_vector/view_api/fuzz/float32", test_bson_vector_view_api_fuzz_float32);
   TestSuite_Add(suite, "/bson_binary_vector/view_api/fuzz/packed_bit", test_bson_vector_view_api_fuzz_packed_bit);

   TestSuite_Add(suite, "/bson_binary_vector/example/int8_const_view", test_bson_vector_example_int8_const_view);
   TestSuite_Add(suite, "/bson_binary_vector/example/int8_view", test_bson_vector_example_int8_view);
   TestSuite_Add(suite, "/bson_binary_vector/example/float32_const_view", test_bson_vector_example_float32_const_view);
   TestSuite_Add(suite, "/bson_binary_vector/example/float32_view", test_bson_vector_example_float32_view);
   TestSuite_Add(
      suite, "/bson_binary_vector/example/packed_bit_const_view", test_bson_vector_example_packed_bit_const_view);
   TestSuite_Add(suite, "/bson_binary_vector/example/packed_bit_view", test_bson_vector_example_packed_bit_view);

   TestSuite_Add(suite, "/bson_binary_vector/edge_cases/int8", test_bson_vector_edge_cases_int8);
   TestSuite_Add(suite, "/bson_binary_vector/edge_cases/float32", test_bson_vector_edge_cases_float32);
   TestSuite_Add(suite, "/bson_binary_vector/edge_cases/packed_bit", test_bson_vector_edge_cases_packed_bit);
}
