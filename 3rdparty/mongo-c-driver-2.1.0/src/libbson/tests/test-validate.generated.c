// ! This code is GENERATED! Do not edit it directly!
// clang-format off

#include <bson/bson.h>

#include <mlib/test.h>

#include <TestSuite.h>


// ! This code is GENERATED! Do not edit it directly!
// Case: empty
static inline void _test_case_empty(void) {
  /**
   * Test a simple empty document object.
   */
  const uint8_t bytes[] = {
    5, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: bad-element
static inline void _test_case_bad_element(void) {
  /**
   * The element content is not valid
   */
  const uint8_t bytes[] = {
    6, 0, 0, 0, 'f', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 6);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: invalid-type
static inline void _test_case_invalid_type(void) {
  /**
   * The type tag "0x0e" is not a valid type
   */
  const uint8_t bytes[] = {
    0x0d, 0, 0, 0, 0x0e, 'f', 'o', 'o', 0, 'f', 'o', 'o', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/invalid/accept
static inline void _test_case_key_invalid_accept(void) {
  /**
   * The element key contains an invalid UTF-8 byte, but we accept it
   * because we aren't doing UTF-8 validation.
   */
  const uint8_t bytes[] = {
    0x28, 0, 0, 0, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 2, 'f', 'o', 'o', 0xff, 'b',
    'a', 'r', 0, 4, 0, 0, 0, 'b', 'a', 'z', 0, 2, 'c', 0, 2, 0, 0, 0, 'd', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/invalid/reject
static inline void _test_case_key_invalid_reject(void) {
  /**
   * The element key is not valid UTF-8 and we reject it when we do UTF-8
   * validation.
   */
  const uint8_t bytes[] = {
    0x28, 0, 0, 0, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 2, 'f', 'o', 'o', 0xff, 'b',
    'a', 'r', 0, 4, 0, 0, 0, 'b', 'a', 'z', 0, 2, 'c', 0, 2, 0, 0, 0, 'd', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8);
  mlib_check(error.message, str_eq, "Text element is not valid UTF-8");
  mlib_check(offset, eq, 13);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/empty/accept
static inline void _test_case_key_empty_accept(void) {
  /**
   * The element has an empty string key, and we accept this.
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 2, 0, 7, 0, 0, 0, 's', 't', 'r', 'i', 'n', 'g', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/empty/reject
static inline void _test_case_key_empty_reject(void) {
  /**
   * The element has an empty key, and we can reject it.
   */
  const uint8_t bytes[] = {
    0x1b, 0, 0, 0, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 2, 0, 7, 0, 0, 0, 's', 't',
    'r', 'i', 'n', 'g', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_EMPTY_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_EMPTY_KEYS);
  mlib_check(error.message, str_eq, "Element key cannot be an empty string");
  mlib_check(offset, eq, 13);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/empty/accept-if-absent
static inline void _test_case_key_empty_accept_if_absent(void) {
  /**
   * We are checking for empty keys, and accept if they are absent.
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 2, 'f', 'o', 'o', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_EMPTY_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/dot/accept
static inline void _test_case_key_dot_accept(void) {
  /**
   * The element key has an ASCII dot, and we accept this since we don't
   * ask to validate it.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 2, 'f', 'o', 'o', '.', 'b', 'a', 'r', 0, 4, 0, 0, 0, 'b',
    'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_EMPTY_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/dot/reject
static inline void _test_case_key_dot_reject(void) {
  /**
   * The element has an ASCII dot, and we reject it when we ask to validate
   * it.
   */
  const uint8_t bytes[] = {
    0x1f, 0, 0, 0, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 2, 'f', 'o', 'o', '.', 'b',
    'a', 'r', 0, 4, 0, 0, 0, 'b', 'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOT_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOT_KEYS);
  mlib_check(error.message, str_eq, "Disallowed '.' in element key: \"foo.bar\"");
  mlib_check(offset, eq, 13);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/dot/accept-if-absent
static inline void _test_case_key_dot_accept_if_absent(void) {
  /**
   * We are checking for keys with dot '.', and accept if they are absent.
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 2, 'f', 'o', 'o', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOT_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/dollar/accept
static inline void _test_case_key_dollar_accept(void) {
  /**
   * We can accept an element key that starts with a dollar '$' sign.
   */
  const uint8_t bytes[] = {
    0x1c, 0, 0, 0, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 2, '$', 'f', 'o', 'o', 0, 4,
    0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/dollar/reject
static inline void _test_case_key_dollar_reject(void) {
  /**
   * We can reject an element key that starts with a dollar '$' sign.
   */
  const uint8_t bytes[] = {
    0x1c, 0, 0, 0, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 2, '$', 'f', 'o', 'o', 0, 4,
    0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Disallowed '$' in element key: \"$foo\"");
  mlib_check(offset, eq, 13);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/dollar/accept-in-middle
static inline void _test_case_key_dollar_accept_in_middle(void) {
  /**
   * This contains a element key "foo$bar", but we don't reject this, as we
   * only care about keys that *start* with dollars.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 2, 'f', 'o', 'o', '$', 'b', 'a', 'r', 0, 4, 0, 0, 0, 'b',
    'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: key/dollar/accept-if-absent
static inline void _test_case_key_dollar_accept_if_absent(void) {
  /**
   * We are validating for dollar-keys, and we accept because this document
   * doesn't contain any such keys.
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 2, 'f', 'o', 'o', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/simple
static inline void _test_case_utf8_simple(void) {
  /**
   * Simple UTF-8 string element
   */
  const uint8_t bytes[] = {
    0x1d, 0, 0, 0, 2, 's', 't', 'r', 'i', 'n', 'g', 0, 0x0c, 0, 0, 0, 's', 'o',
    'm', 'e', 0x20, 's', 't', 'r', 'i', 'n', 'g', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/missing-null
static inline void _test_case_utf8_missing_null(void) {
  /**
   * The UTF-8 element "a" contains 4 characters and declares its length of 4,
   * but the fourth character is supposed to be a null terminator. In this case,
   * it is the letter 'd'.
   */
  const uint8_t bytes[] = {
    0x10, 0, 0, 0, 2, 'a', 0, 4, 0, 0, 0, 'a', 'b', 'c', 'd', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 14);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/length-zero
static inline void _test_case_utf8_length_zero(void) {
  /**
   * UTF-8 string length must always be at least 1 for the null terminator
   */
  const uint8_t bytes[] = {
    0x0c, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 6);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/length-too-short
static inline void _test_case_utf8_length_too_short(void) {
  /**
   * UTF-8 string is three chars and a null terminator, but the declared length is 3 (should be 4)
   */
  const uint8_t bytes[] = {
    0x0f, 0, 0, 0, 2, 0, 3, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 12);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/header-too-large
static inline void _test_case_utf8_header_too_large(void) {
  /**
   * Data { "foo": "bar" } but the declared length of "bar" is way too large.
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 2, 'f', 'o', 'o', 0, 0xff, 0xff, 0xff, 0xff, 'b', 'a', 'r',
    0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/valid
static inline void _test_case_utf8_valid(void) {
  /**
   * Validate a valid UTF-8 string with UTF-8 validation enabled.
   */
  const uint8_t bytes[] = {
    0x13, 0, 0, 0, 2, 'f', 'o', 'o', 0, 5, 0, 0, 0, 'a', 'b', 'c', 'd', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/invalid/accept
static inline void _test_case_utf8_invalid_accept(void) {
  /**
   * Validate an invalid UTF-8 string, but accept invalid UTF-8.
   */
  const uint8_t bytes[] = {
    0x14, 0, 0, 0, 2, 'f', 'o', 'o', 0, 6, 0, 0, 0, 'a', 'b', 'c', 0xff, 'd', 0,
    0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/invalid/reject
static inline void _test_case_utf8_invalid_reject(void) {
  /**
   * Validate an invalid UTF-8 string, and expect rejection.
   */
  const uint8_t bytes[] = {
    0x14, 0, 0, 0, 2, 'f', 'o', 'o', 0, 6, 0, 0, 0, 'a', 'b', 'c', 0xff, 'd', 0,
    0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8);
  mlib_check(error.message, str_eq, "Text element is not valid UTF-8");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/valid-with-null/accept-1
static inline void _test_case_utf8_valid_with_null_accept_1(void) {
  /**
   * This is a valid UTF-8 string that contains a null character. We accept
   * it because we don't do UTF-8 validation.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 2, 'f', 'o', 'o', 0, 8, 0, 0, 0, 'a', 'b', 'c', 0, '1', '2',
    '3', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/valid-with-null/accept-2
static inline void _test_case_utf8_valid_with_null_accept_2(void) {
  /**
   * This is a valid UTF-8 string that contains a null character. We allow
   * it explicitly when we request UTF-8 validation.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 2, 'f', 'o', 'o', 0, 8, 0, 0, 0, 'a', 'b', 'c', 0, '1', '2',
    '3', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/valid-with-null/reject
static inline void _test_case_utf8_valid_with_null_reject(void) {
  /**
   * This is a valid UTF-8 string that contains a null character. We reject
   * this because we don't pass BSON_VALIDATE_UTF8_ALLOW_NULL.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 2, 'f', 'o', 'o', 0, 8, 0, 0, 0, 'a', 'b', 'c', 0, '1', '2',
    '3', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8_ALLOW_NULL);
  mlib_check(error.message, str_eq, "UTF-8 string contains a U+0000 (null) character");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/overlong-null/accept-1
static inline void _test_case_utf8_overlong_null_accept_1(void) {
  /**
   * This is an *invalid* UTF-8 string, and contains an overlong null. We should
   * accept it because we aren't doing UTF-8 validation.
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 2, 'f', 'o', 'o', 0, 9, 0, 0, 0, 'a', 'b', 'c', 0xc0, 0x80,
    '1', '2', '3', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/overlong-null/accept-2
static inline void _test_case_utf8_overlong_null_accept_2(void) {
  /**
   * ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence
   * 
   * This is an *invalid* UTF-8 string, because it contains an overlong null
   * "0xc0 0x80". Despite being invalid, we accept it because our current UTF-8
   * validation considers the overlong null to be a valid encoding for the null
   * codepoint (it isn't, but changing it would be a breaking change).
   * 
   * If/when UTF-8 validation is changed to reject overlong null, then this
   * test should change to expect rejection the invalid UTF-8.
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 2, 'f', 'o', 'o', 0, 9, 0, 0, 0, 'a', 'b', 'c', 0xc0, 0x80,
    '1', '2', '3', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8/overlong-null/reject
static inline void _test_case_utf8_overlong_null_reject(void) {
  /**
   * ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence
   * 
   * This is an *invalid* UTF-8 string, because it contains an overlong null
   * character. Our UTF-8 validator wrongly accepts overlong null as a valid
   * UTF-8 sequence. This test fails because we disallow null codepoints, not
   * because the UTF-8 is invalid, and the error message reflects that.
   * 
   * If/when UTF-8 validation is changed to reject overlong null, then the
   * expected error code and error message for this test should change.
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 2, 'f', 'o', 'o', 0, 9, 0, 0, 0, 'a', 'b', 'c', 0xc0, 0x80,
    '1', '2', '3', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8_ALLOW_NULL);
  mlib_check(error.message, str_eq, "UTF-8 string contains a U+0000 (null) character");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8-key/invalid/accept
static inline void _test_case_utf8_key_invalid_accept(void) {
  /**
   * The element key is not valid UTf-8, but we accept it if we don't do
   * UTF-8 validation.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 2, 'a', 'b', 'c', 0xff, 'd', 'e', 'f', 0, 4, 0, 0, 0, 'b',
    'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8-key/invalid/reject
static inline void _test_case_utf8_key_invalid_reject(void) {
  /**
   * The element key is not valid UTF-8, and we reject it when we requested
   * UTF-8 validation.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 2, 'a', 'b', 'c', 0xff, 'd', 'e', 'f', 0, 4, 0, 0, 0, 'b',
    'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8);
  mlib_check(error.message, str_eq, "Text element is not valid UTF-8");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8-key/overlong-null/reject
static inline void _test_case_utf8_key_overlong_null_reject(void) {
  /**
   * ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence
   * 
   * The element key is invalid UTF-8 because it contains an overlong null. We accept the
   * overlong null as a valid encoding of U+0000, but we reject the key because
   * we disallow null in UTF-8 strings.
   * 
   * If/when UTF-8 validation is changed to reject overlong null, then the
   * expected error code and error message for this test should change.
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 2, 'a', 'b', 'c', 0xc0, 0x80, 'd', 'e', 'f', 0, 4, 0, 0, 0,
    'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8_ALLOW_NULL);
  mlib_check(error.message, str_eq, "UTF-8 string contains a U+0000 (null) character");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: utf8-key/overlong-null/accept
static inline void _test_case_utf8_key_overlong_null_accept(void) {
  /**
   * ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence
   * 
   * The element key is invalid UTF-8 because it contains an overlong null. We accept the
   * overlong null as a valid encoding of U+0000, and we allow it in an element key because
   * we pass ALLOW_NULL
   * 
   * If/when UTF-8 validation is changed to reject overlong null, then this
   * test case should instead reject the key string as invalid UTF-8.
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 2, 'a', 'b', 'c', 0xc0, 0x80, 'd', 'e', 'f', 0, 4, 0, 0, 0,
    'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: array/empty
static inline void _test_case_array_empty(void) {
  /**
   * Simple empty array element
   */
  const uint8_t bytes[] = {
    0x11, 0, 0, 0, 4, 'a', 'r', 'r', 'a', 'y', 0, 5, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: array/simple
static inline void _test_case_array_simple(void) {
  /**
   * Simple array element of integers
   */
  const uint8_t bytes[] = {
    0x26, 0, 0, 0, 4, 'a', 'r', 'r', 'a', 'y', 0, 0x1a, 0, 0, 0, 0x10, '0', 0,
    0x2a, 0, 0, 0, 0x10, '1', 0, 0xc1, 6, 0, 0, 0x10, '2', 0, 0xf8, 0xff, 0xff,
    0xff, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: array/invalid-element
static inline void _test_case_array_invalid_element(void) {
  /**
   * Simple array element of integers, but one element is truncated
   */
  const uint8_t bytes[] = {
    0x23, 0, 0, 0, 4, 'a', 'r', 'r', 'a', 'y', 0, 0x17, 0, 0, 0, 0x10, '0', 0,
    0x2a, 0, 0, 0, 0x10, '1', 0, 0, 0x10, '2', 0, 0xf8, 0xff, 0xff, 0xff, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 34);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: array/invalid-element-check-offset
static inline void _test_case_array_invalid_element_check_offset(void) {
  /**
   * This is the same as the array/invalid-element test, but with a longer
   * key string on the parent array. This is to check that the error offset
   * is properly adjusted for the additional characters.
   */
  const uint8_t bytes[] = {
    0x2b, 0, 0, 0, 4, 'a', 'r', 'r', 'a', 'y', '-', 's', 'h', 'i', 'f', 't',
    'e', 'd', 0, 0x17, 0, 0, 0, 0x10, '0', 0, 0x2a, 0, 0, 0, 0x10, '1', 0, 0,
    0x10, '2', 0, 0xf8, 0xff, 0xff, 0xff, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 42);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: symbol/simple
static inline void _test_case_symbol_simple(void) {
  /**
   * A simple document: { symbol: Symbol("void 0;") }
   */
  const uint8_t bytes[] = {
    0x19, 0, 0, 0, 0x0e, 's', 'y', 'm', 'b', 'o', 'l', 0, 8, 0, 0, 0, 'v', 'o',
    'i', 'd', 0x20, '0', 0x3b, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: symbol/invalid-utf8/accept
static inline void _test_case_symbol_invalid_utf8_accept(void) {
  /**
   * A simple symbol document, but the string contains invalid UTF-8
   */
  const uint8_t bytes[] = {
    0x1a, 0, 0, 0, 0x0e, 's', 'y', 'm', 'b', 'o', 'l', 0, 9, 0, 0, 0, 'v', 'o',
    'i', 'd', 0xff, 0x20, '0', 0x3b, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: symbol/invalid-utf8/reject
static inline void _test_case_symbol_invalid_utf8_reject(void) {
  /**
   * A simple symbol document, but the string contains invalid UTF-8
   */
  const uint8_t bytes[] = {
    0x1a, 0, 0, 0, 0x0e, 's', 'y', 'm', 'b', 'o', 'l', 0, 9, 0, 0, 0, 'v', 'o',
    'i', 'd', 0xff, 0x20, '0', 0x3b, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8);
  mlib_check(error.message, str_eq, "Text element is not valid UTF-8");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: symbol/length-zero
static inline void _test_case_symbol_length_zero(void) {
  /**
   * Symbol string length must always be at least 1 for the null terminator
   */
  const uint8_t bytes[] = {
    0x0c, 0, 0, 0, 0x0e, 0, 0, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 6);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: symbol/length-too-short
static inline void _test_case_symbol_length_too_short(void) {
  /**
   * Symbol string is three chars and a null terminator, but the declared
   * length is 3 (should be 4)
   */
  const uint8_t bytes[] = {
    0x0f, 0, 0, 0, 0x0e, 0, 3, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 12);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code/simple
static inline void _test_case_code_simple(void) {
  /**
   * A simple document: { code: Code("void 0;") }
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 0x0d, 'c', 'o', 'd', 'e', 0, 8, 0, 0, 0, 'v', 'o', 'i', 'd',
    0x20, '0', 0x3b, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code/invalid-utf8/accept
static inline void _test_case_code_invalid_utf8_accept(void) {
  /**
   * A simple code document, but the string contains invalid UTF-8
   */
  const uint8_t bytes[] = {
    0x18, 0, 0, 0, 0x0d, 'c', 'o', 'd', 'e', 0, 9, 0, 0, 0, 'v', 'o', 'i', 'd',
    0xff, 0x20, '0', 0x3b, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code/invalid-utf8/reject
static inline void _test_case_code_invalid_utf8_reject(void) {
  /**
   * A simple code document, but the string contains invalid UTF-8
   */
  const uint8_t bytes[] = {
    0x18, 0, 0, 0, 0x0d, 'c', 'o', 'd', 'e', 0, 9, 0, 0, 0, 'v', 'o', 'i', 'd',
    0xff, 0x20, '0', 0x3b, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8);
  mlib_check(error.message, str_eq, "Text element is not valid UTF-8");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code/length-zero
static inline void _test_case_code_length_zero(void) {
  /**
   * Code string length must always be at least 1 for the null terminator
   */
  const uint8_t bytes[] = {
    0x10, 0, 0, 0, 0x0d, 'c', 'o', 'd', 'e', 0, 0, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 10);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code/length-too-short
static inline void _test_case_code_length_too_short(void) {
  /**
   * Code string is three chars and a null terminator, but the declared length is 3 (should be 4)
   */
  const uint8_t bytes[] = {
    0x13, 0, 0, 0, 0x0d, 'c', 'o', 'd', 'e', 0, 3, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 16);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code-with-scope/simple
static inline void _test_case_code_with_scope_simple(void) {
  /**
   * A simple valid code-with-scope element
   */
  const uint8_t bytes[] = {
    0x1f, 0, 0, 0, 0x0f, 'f', 'o', 'o', 0, 0x15, 0, 0, 0, 8, 0, 0, 0, 'v', 'o',
    'i', 'd', 0x20, '0', 0x3b, 0, 5, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code-with-scope/invalid-code-length-zero
static inline void _test_case_code_with_scope_invalid_code_length_zero(void) {
  /**
   * Data { "": CodeWithScope("", {}) }, but the code string length is zero, when
   * it must be at least 1
   */
  const uint8_t bytes[] = {
    0x15, 0, 0, 0, 0x0f, 0, 0x0a, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 6);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code-with-scope/invalid-code-length-too-large
static inline void _test_case_code_with_scope_invalid_code_length_too_large(void) {
  /**
   * Data { "": CodeWithScope("", {}) }, but the code string length is way too large
   */
  const uint8_t bytes[] = {
    0x15, 0, 0, 0, 0x0f, 0, 0x0a, 0, 0, 0, 0xff, 0xff, 0xff, 0xff, 0, 5, 0, 0,
    0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 6);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code-with-scope/invalid-scope
static inline void _test_case_code_with_scope_invalid_scope(void) {
  /**
   * A code-with-scope element, but the scope document is corrupted
   */
  const uint8_t bytes[] = {
    0x1e, 0, 0, 0, 0x0f, 'f', 'o', 'o', 0, 0x14, 0, 0, 0, 8, 0, 0, 0, 'v', 'o',
    'i', 'd', 0x20, '0', 0x3b, 0, 5, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 13);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code-with-scope/empty-key-in-scope
static inline void _test_case_code_with_scope_empty_key_in_scope(void) {
  /**
   * A code-with-scope element. The scope itself contains empty keys within
   * objects, and we ask to reject empty keys. But the scope document should
   * be treated as an opaque closure, so our outer validation rules do not
   * apply.
   */
  const uint8_t bytes[] = {
    '7', 0, 0, 0, 0x0f, 'c', 'o', 'd', 'e', 0, 0x2c, 0, 0, 0, 8, 0, 0, 0, 'v',
    'o', 'i', 'd', 0x20, '0', 0x3b, 0, 0x1c, 0, 0, 0, 3, 'o', 'b', 'j', 0, 0x12,
    0, 0, 0, 2, 0, 7, 0, 0, 0, 's', 't', 'r', 'i', 'n', 'g', 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_EMPTY_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code-with-scope/corrupt-scope
static inline void _test_case_code_with_scope_corrupt_scope(void) {
  /**
   * A code-with-scope element, but the scope contains corruption
   */
  const uint8_t bytes[] = {
    0x2a, 0, 0, 0, 0x0f, 'c', 'o', 'd', 'e', 0, 0x1f, 0, 0, 0, 8, 0, 0, 0, 'v',
    'o', 'i', 'd', 0x20, '0', 0x3b, 0, 0x0f, 0, 0, 0, 2, 'f', 'o', 'o', 0, 0, 0,
    0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "Error in scope document for element \"code\": corrupt BSON");
  mlib_check(offset, eq, 13);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: code-with-scope/corrupt-scope-2
static inline void _test_case_code_with_scope_corrupt_scope_2(void) {
  /**
   * A code-with-scope element, but the scope contains corruption
   */
  const uint8_t bytes[] = {
    0x2a, 0, 0, 0, 0x0f, 'c', 'o', 'd', 'e', 0, 0x1f, 0, 0, 0, 8, 0, 0, 0, 'v',
    'o', 'i', 'd', 0x20, '0', 0x3b, 0, 0x0f, 0, 0, 0, 2, 'f', 'o', 'o', 0, 0xff,
    0xff, 0xff, 0xff, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "Error in scope document for element \"code\": corrupt BSON");
  mlib_check(offset, eq, 13);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: regex/simple
static inline void _test_case_regex_simple(void) {
  /**
   * Simple document: { regex: Regex("1234", "gi") }
   */
  const uint8_t bytes[] = {
    0x14, 0, 0, 0, 0x0b, 'r', 'e', 'g', 'e', 'x', 0, '1', '2', '3', '4', 0, 'g',
    'i', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: regex/invalid-opts
static inline void _test_case_regex_invalid_opts(void) {
  /**
   * A regular expression element with missing null terminator. The main
   * option string "foo" has a null terminator, but the option component "bar"
   * does not have a null terminator. A naive parse will see the doc's null
   * terminator as the null terminator for the options string, but that's
   * invalid!
   */
  const uint8_t bytes[] = {
    0x13, 0, 0, 0, 0x0b, 'r', 'e', 'g', 'e', 'x', 0, 'f', 'o', 'o', 0, 'b', 'a',
    'r', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 18);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: regex/double-null
static inline void _test_case_regex_double_null(void) {
  /**
   * A regular expression element with an extra null terminator. Since regex
   * is delimited by its null terminator, the iterator will stop early before
   * the actual EOD.
   */
  const uint8_t bytes[] = {
    0x15, 0, 0, 0, 0x0b, 'r', 'e', 'g', 'e', 'x', 0, 'f', 'o', 'o', 0, 'b', 'a',
    'r', 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 21);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: regex/invalid-utf8/accept
static inline void _test_case_regex_invalid_utf8_accept(void) {
  /**
   * A regular expression that contains invalid UTF-8.
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 0x0b, 'r', 'e', 'g', 'e', 'x', 0, 'f', 'o', 'o', 0xff, 'b',
    'a', 'r', 0, 'g', 'i', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: regex/invalid-utf8/reject
static inline void _test_case_regex_invalid_utf8_reject(void) {
  /**
   * A regular expression that contains invalid UTF-8.
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 0x0b, 'r', 'e', 'g', 'e', 'x', 0, 'f', 'o', 'o', 0xff, 'b',
    'a', 'r', 0, 'g', 'i', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8);
  mlib_check(error.message, str_eq, "Text element is not valid UTF-8");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: regex/invalid-utf8/accept-if-absent
static inline void _test_case_regex_invalid_utf8_accept_if_absent(void) {
  /**
   * A regular valid UTf-8 regex. We check for invalid UTf-8, and accept becaues
   * the regex is fine.
   */
  const uint8_t bytes[] = {
    0x13, 0, 0, 0, 0x0b, 'r', 'e', 'g', 'e', 'x', 0, 'f', 'o', 'o', 0, 'g', 'i',
    0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbpointer/string-length-zero
static inline void _test_case_dbpointer_string_length_zero(void) {
  /**
   * Document { "foo": DBPointer("", <oid>) }, but the length header on the inner
   * string is zero, when it must be at least 1.
   */
  const uint8_t bytes[] = {
    0x1b, 0, 0, 0, 0x0c, 'f', 'o', 'o', 0, 0, 0, 0, 0, 0, 'R', 'Y', 0xb5, 'j',
    0xfa, 0x5b, 0xd8, 'A', 0xd6, 'X', 0x5d, 0x99, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbpointer/string-length-too-big
static inline void _test_case_dbpointer_string_length_too_big(void) {
  /**
   * Document { "foo": DBPointer("foobar", <oid>) }, but the length header on the inner
   * string is far too large
   */
  const uint8_t bytes[] = {
    0x21, 0, 0, 0, 0x0c, 'f', 'o', 'o', 0, 0xff, 0xff, 0xff, 0xff, 'f', 'o',
    'o', 'b', 'a', 'r', 0, 'R', 'Y', 0xb5, 'j', 0xfa, 0x5b, 0xd8, 'A', 0xd6,
    'X', 0x5d, 0x99, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbpointer/truncated
static inline void _test_case_dbpointer_truncated(void) {
  /**
   * Document { "foo": DBPointer("foobar", <oid>) }, but the length header on
   * the string is one byte too large, causing it to use the first byte of the
   * OID as the null terminator. This should fail when iterating.
   */
  const uint8_t bytes[] = {
    '2', 0, 0, 0, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 0x0c, 'f', 'o', 'o', 0, 7, 0,
    0, 0, 'f', 'o', 'o', 'b', 'a', 'r', 0, 'Y', 0xb5, 'j', 0xfa, 0x5b, 0xd8,
    'A', 0xd6, 'X', 0x5d, 0x99, 2, 'a', 0, 2, 0, 0, 0, 'b', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 43);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbpointer/missing-null
static inline void _test_case_dbpointer_missing_null(void) {
  /**
   * Document { "foo": DBPointer("abcd", <oid>) }, the length header on
   * the string is 4, but the fourth byte is not a null terminator.
   */
  const uint8_t bytes[] = {
    0x1e, 0, 0, 0, 0x0c, 'f', 'o', 'o', 0, 4, 0, 0, 0, 'a', 'b', 'c', 'd', 'R',
    'Y', 0xb5, 'j', 0xfa, 0x5b, 0xd8, 'A', 0xd6, 'X', 0x5d, 0x99, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 16);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbpointer/invalid-utf8/accept
static inline void _test_case_dbpointer_invalid_utf8_accept(void) {
  /**
   * DBPointer document, but the collection string contains invalid UTF-8
   */
  const uint8_t bytes[] = {
    0x22, 0, 0, 0, 0x0c, 'f', 'o', 'o', 0, 8, 0, 0, 0, 'a', 'b', 'c', 0xff, 'd',
    'e', 'f', 0, 'R', 'Y', 0xb5, 'j', 0xfa, 0x5b, 0xd8, 'A', 0xd6, 'X', 0x5d,
    0x99, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbpointer/invalid-utf8/reject
static inline void _test_case_dbpointer_invalid_utf8_reject(void) {
  /**
   * DBPointer document, but the collection string contains invalid UTF-8
   */
  const uint8_t bytes[] = {
    0x22, 0, 0, 0, 0x0c, 'f', 'o', 'o', 0, 8, 0, 0, 0, 'a', 'b', 'c', 0xff, 'd',
    'e', 'f', 0, 'R', 'Y', 0xb5, 'j', 0xfa, 0x5b, 0xd8, 'A', 0xd6, 'X', 0x5d,
    0x99, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_UTF8);
  mlib_check(error.message, str_eq, "Text element is not valid UTF-8");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbpointer/invalid-utf8/accept-if-absent
static inline void _test_case_dbpointer_invalid_utf8_accept_if_absent(void) {
  /**
   * DBPointer document, and we validate UTF-8. Accepts because there is no
   * invalid UTF-8 here.
   */
  const uint8_t bytes[] = {
    0x21, 0, 0, 0, 0x0c, 'f', 'o', 'o', 0, 7, 0, 0, 0, 'a', 'b', 'c', 'd', 'e',
    'f', 0, 'R', 'Y', 0xb5, 'j', 0xfa, 0x5b, 0xd8, 'A', 0xd6, 'X', 0x5d, 0x99, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_UTF8, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: subdoc/simple
static inline void _test_case_subdoc_simple(void) {
  /**
   * A simple document: { doc: { foo: "bar" } }
   */
  const uint8_t bytes[] = {
    0x1c, 0, 0, 0, 3, 'd', 'o', 'c', 0, 0x12, 0, 0, 0, 2, 'f', 'o', 'o', 0, 4,
    0, 0, 0, 'b', 'a', 'r', 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: subdoc/invalid-shared-null
static inline void _test_case_subdoc_invalid_shared_null(void) {
  /**
   * A truncated subdocument element, with its null terminator accidentally
   * overlapping the parent document's null.
   */
  const uint8_t bytes[] = {
    0x0e, 0, 0, 0, 3, 'd', 'o', 'c', 0, 5, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: subdoc/overlapping-utf8-null
static inline void _test_case_subdoc_overlapping_utf8_null(void) {
  /**
   * Encodes the document:
   * 
   *     { "foo": { "bar": "baz" } }
   * 
   * but the foo.bar UTF-8 string is truncated improperly and reuses the null
   * terminator for "foo"
   */
  const uint8_t bytes[] = {
    0x1c, 0, 0, 0, 3, 'd', 'o', 'c', 0, 0x12, 0, 0, 0, 2, 'b', 'a', 'r', 0, 5,
    0, 0, 0, 'b', 'a', 'z', 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 18);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: subdoc/invalid-element
static inline void _test_case_subdoc_invalid_element(void) {
  /**
   * A subdocument that contains an invalid element
   */
  const uint8_t bytes[] = {
    0x18, 0, 0, 0, 3, 'd', 'o', 'c', 0, 0x0e, 0, 0, 0, 1, 'd', 'b', 'l', 0, 'a',
    'b', 'c', 'd', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 18);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: subdoc/header-too-large
static inline void _test_case_subdoc_header_too_large(void) {
  /**
   * Data {"foo": {}}, but the subdoc header is too large.
   */
  const uint8_t bytes[] = {
    0x0f, 0, 0, 0, 3, 'f', 'o', 'o', 0, 0xf7, 0xff, 0xff, 0xff, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: subdoc/header-too-small
static inline void _test_case_subdoc_header_too_small(void) {
  /**
   * Nested document with a header value of 4, which is always too small.
   */
  const uint8_t bytes[] = {
    0x0f, 0, 0, 0, 3, 't', 'e', 's', 't', 0, 4, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: subdoc/impossible-size
static inline void _test_case_subdoc_impossible_size(void) {
  /**
   * Data {"foo": {}}, but the subdoc header is UINT32_MAX/INT32_MIN, which
   * becomes is an invalid document header.
   */
  const uint8_t bytes[] = {
    0x0f, 0, 0, 0, 3, 'f', 'o', 'o', 0, 0xff, 0xff, 0xff, 0xff, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: null/simple
static inline void _test_case_null_simple(void) {
  /**
   * A simple document: { "null": null }
   */
  const uint8_t bytes[] = {
    0x0b, 0, 0, 0, 0x0a, 'n', 'u', 'l', 'l', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: undefined/simple
static inline void _test_case_undefined_simple(void) {
  /**
   * A simple document: { "undefined": undefined }
   */
  const uint8_t bytes[] = {
    0x10, 0, 0, 0, 6, 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e', 'd', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: binary/simple
static inline void _test_case_binary_simple(void) {
  /**
   * Simple binary data { "binary": Binary(0x80, b'12345') }
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 5, 'b', 'i', 'n', 'a', 'r', 'y', 0, 5, 0, 0, 0, 0x80, '1',
    '2', '3', '4', '5', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: binary/bad-length-zero-subtype-2
static inline void _test_case_binary_bad_length_zero_subtype_2(void) {
  /**
   * Binary data that has an invalid length header. It is subtype 2,
   * which means it contains an additional length header.
   */
  const uint8_t bytes[] = {
    0x1a, 0, 0, 0, 5, 'b', 'i', 'n', 'a', 'r', 'y', 0, 0, 0, 0, 0, 2, 4, 0, 0,
    0, '1', '2', '3', '4', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 12);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: binary/bad-inner-length-on-subtype-2
static inline void _test_case_binary_bad_inner_length_on_subtype_2(void) {
  /**
   * Binary data that has an valid outer length header, but the inner length
   * header for subtype 2 has an incorrect value.
   */
  const uint8_t bytes[] = {
    0x1a, 0, 0, 0, 5, 'b', 'i', 'n', 'a', 'r', 'y', 0, 8, 0, 0, 0, 2, 2, 0, 0,
    0, '1', '2', '3', '4', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 17);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: binary/bad-length-too-small
static inline void _test_case_binary_bad_length_too_small(void) {
  /**
   * Data { "binary": Binary(0x80, b'1234') }, but the length header on
   * the Binary object is too small.
   * 
   * This won't cause the binary to decode wrong, but it will cause the iterator
   * to jump into the middle of the binary data which will not decode as a
   * proper BSON element.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 5, 'b', 'i', 'n', 'a', 'r', 'y', 0, 2, 0, 0, 0, 0x80, '1',
    '2', '3', '4', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 22);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: binary/bad-length-too-big
static inline void _test_case_binary_bad_length_too_big(void) {
  /**
   * Data { "binary": Binary(0x80, b'1234') }, but the length header on
   * the Binary object is too large.
   */
  const uint8_t bytes[] = {
    0x16, 0, 0, 0, 5, 'b', 'i', 'n', 'a', 'r', 'y', 0, 0xf3, 0xff, 0xff, 0xff,
    0x80, '1', '2', '3', '4', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 12);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: binary/old-invalid/1
static inline void _test_case_binary_old_invalid_1(void) {
  /**
   * This is an old-style binary type 0x2. It has an inner length header of 5,
   * but it should be 4.
   */
  const uint8_t bytes[] = {
    0x1a, 0, 0, 0, 5, 'b', 'i', 'n', 'a', 'r', 'y', 0, 8, 0, 0, 0, 2, 5, 0, 0,
    0, 'a', 'b', 'c', 'd', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 17);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: binary/old-invalid/2
static inline void _test_case_binary_old_invalid_2(void) {
  /**
   * This is an old-style binary type 0x2. The data segment is too small to
   * be valid.
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 5, 'b', 'i', 'n', 0, 3, 0, 0, 0, 2, 'a', 'b', 'c', 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: minkey/simple
static inline void _test_case_minkey_simple(void) {
  /**
   * A simple document with a MinKey element
   */
  const uint8_t bytes[] = {
    0x0a, 0, 0, 0, 0xff, 'm', 'i', 'n', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: maxkey/simple
static inline void _test_case_maxkey_simple(void) {
  /**
   * A simple document with a MaxKey element
   */
  const uint8_t bytes[] = {
    0x0a, 0, 0, 0, 0x7f, 'm', 'a', 'x', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: int32/simple
static inline void _test_case_int32_simple(void) {
  /**
   * A simple document with a valid single int32 element
   */
  const uint8_t bytes[] = {
    0x10, 0, 0, 0, 0x10, 'i', 'n', 't', '3', '2', 0, 0x2a, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: int32/truncated
static inline void _test_case_int32_truncated(void) {
  /**
   * Truncated 32-bit integer
   */
  const uint8_t bytes[] = {
    0x19, 0, 0, 0, 0x10, 'i', 'n', 't', '3', '2', '-', 't', 'r', 'u', 'n', 'c',
    'a', 't', 'e', 'd', 0, 0x2a, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 21);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: timestamp/simple
static inline void _test_case_timestamp_simple(void) {
  /**
   * A simple timestamp element
   */
  const uint8_t bytes[] = {
    0x18, 0, 0, 0, 0x11, 't', 'i', 'm', 'e', 's', 't', 'a', 'm', 'p', 0, 0xc1,
    6, 0, 0, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: timestamp/truncated
static inline void _test_case_timestamp_truncated(void) {
  /**
   * A truncated timestamp element
   */
  const uint8_t bytes[] = {
    0x17, 0, 0, 0, 0x11, 't', 'i', 'm', 'e', 's', 't', 'a', 'm', 'p', 0, 0xc1,
    6, 0, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 15);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: int64/simple
static inline void _test_case_int64_simple(void) {
  /**
   * A simple document with a valid single int64 element
   */
  const uint8_t bytes[] = {
    0x14, 0, 0, 0, 0x12, 'i', 'n', 't', '6', '4', 0, 0xc1, 6, 0, 0, 0, 0, 0, 0,
    0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: int64/truncated
static inline void _test_case_int64_truncated(void) {
  /**
   * Truncated 64-bit integer
   */
  const uint8_t bytes[] = {
    0x1d, 0, 0, 0, 0x12, 'i', 'n', 't', '6', '4', '-', 't', 'r', 'u', 'n', 'c',
    'a', 't', 'e', 'd', 0, 0xc1, 6, 0, 0, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 21);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: double/simple
static inline void _test_case_double_simple(void) {
  /**
   * Simple float64 element
   */
  const uint8_t bytes[] = {
    0x15, 0, 0, 0, 1, 'd', 'o', 'u', 'b', 'l', 'e', 0, 0x1f, 0x85, 0xeb, 'Q',
    0xb8, 0x1e, 9, 0x40, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: double/truncated
static inline void _test_case_double_truncated(void) {
  /**
   * Truncated 64-bit float
   */
  const uint8_t bytes[] = {
    0x1e, 0, 0, 0, 1, 'd', 'o', 'u', 'b', 'l', 'e', '-', 't', 'r', 'u', 'n',
    'c', 'a', 't', 'e', 'd', 0, 0x0a, 0xd7, 0xa3, 'p', 0x3d, 0x0a, 9, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 22);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: boolean/simple-false
static inline void _test_case_boolean_simple_false(void) {
  /**
   * A simple boolean 'false'
   */
  const uint8_t bytes[] = {
    0x0c, 0, 0, 0, 8, 'b', 'o', 'o', 'l', 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: boolean/simple-true
static inline void _test_case_boolean_simple_true(void) {
  /**
   * A simple boolean 'true'
   */
  const uint8_t bytes[] = {
    0x0c, 0, 0, 0, 8, 'b', 'o', 'o', 'l', 0, 1, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: boolean/invalid
static inline void _test_case_boolean_invalid(void) {
  /**
   * An invalid boolean octet. Must be '0' or '1', but is 0xc3.
   */
  const uint8_t bytes[] = {
    0x0c, 0, 0, 0, 8, 'b', 'o', 'o', 'l', 0, 0xc3, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 10);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: datetime/simple
static inline void _test_case_datetime_simple(void) {
  /**
   * Simple datetime element
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 9, 'u', 't', 'c', 0, 0x0b, 0x98, 0x8c, 0x2b, '3', 1, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: datetime/truncated
static inline void _test_case_datetime_truncated(void) {
  /**
   * Truncated datetime element
   */
  const uint8_t bytes[] = {
    0x11, 0, 0, 0, 9, 'u', 't', 'c', 0, 0x0b, 0x98, 0x8c, 0x2b, '3', 1, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, 0, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_CORRUPT);
  mlib_check(error.message, str_eq, "corrupt BSON");
  mlib_check(offset, eq, 9);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/missing-id
static inline void _test_case_dbref_missing_id(void) {
  /**
   * This dbref document is missing an $id element
   */
  const uint8_t bytes[] = {
    0x13, 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Expected an $id element following $ref");
  mlib_check(offset, eq, 18);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/non-id
static inline void _test_case_dbref_non_id(void) {
  /**
   * The 'bar' element should be an '$id' element.
   */
  const uint8_t bytes[] = {
    0x20, 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    'b', 'a', 'r', 0, 4, 0, 0, 0, 'b', 'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Expected an $id element following $ref");
  mlib_check(offset, eq, 18);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/not-first-elements
static inline void _test_case_dbref_not_first_elements(void) {
  /**
   * This would be a valid DBRef, but the "$ref" key must come first.
   */
  const uint8_t bytes[] = {
    0x29, 0, 0, 0, 2, 'f', 'o', 'o', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 2, '$',
    'r', 'e', 'f', 0, 2, 0, 0, 0, 'a', 0, 2, '$', 'i', 'd', 0, 2, 0, 0, 0, 'b',
    0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Disallowed '$' in element key: \"$ref\"");
  mlib_check(offset, eq, 17);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/ref-without-id-with-db
static inline void _test_case_dbref_ref_without_id_with_db(void) {
  /**
   * There should be an $id element, but we skip straight to $db
   */
  const uint8_t bytes[] = {
    0x20, 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'd', 'b', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Expected an $id element following $ref");
  mlib_check(offset, eq, 18);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/non-string-ref
static inline void _test_case_dbref_non_string_ref(void) {
  /**
   * The $ref element must be a string, but is an integer.
   */
  const uint8_t bytes[] = {
    0x0f, 0, 0, 0, 0x10, '$', 'r', 'e', 'f', 0, 0x2a, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "$ref element must be a UTF-8 element");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/non-string-db
static inline void _test_case_dbref_non_string_db(void) {
  /**
   * The $db element should be a string, but is an integer.
   */
  const uint8_t bytes[] = {
    0x29, 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'i', 'd', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 0x10, '$', 'd', 'b', 0,
    0x2a, 0, 0, 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "$db element in DBRef must be a UTF-8 element");
  mlib_check(offset, eq, 31);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/invalid-extras-between
static inline void _test_case_dbref_invalid_extras_between(void) {
  /**
   * Almost a valid DBRef, but there is an extra field before $db. We reject $db
   * as an invalid key.
   */
  const uint8_t bytes[] = {
    0x3e, 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'i', 'd', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 2, 'e', 'x', 't', 'r', 'a',
    0, 6, 0, 0, 0, 'f', 'i', 'e', 'l', 'd', 0, 2, '$', 'd', 'b', 0, 4, 0, 0, 0,
    'b', 'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Disallowed '$' in element key: \"$db\"");
  mlib_check(offset, eq, 48);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/invalid-double-ref
static inline void _test_case_dbref_invalid_double_ref(void) {
  /**
   * Invalid DBRef contains a second $ref element.
   */
  const uint8_t bytes[] = {
    '.', 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 2, '$', 'i', 'd', 0, 4,
    0, 0, 0, 'b', 'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Expected an $id element following $ref");
  mlib_check(offset, eq, 18);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/invalid-missing-ref
static inline void _test_case_dbref_invalid_missing_ref(void) {
  /**
   * DBRef document requires a $ref key to be first.
   */
  const uint8_t bytes[] = {
    0x12, 0, 0, 0, 2, '$', 'i', 'd', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  mlib_check(!is_valid);
  mlib_check(error.code, eq, BSON_VALIDATE_DOLLAR_KEYS);
  mlib_check(error.message, str_eq, "Disallowed '$' in element key: \"$id\"");
  mlib_check(offset, eq, 4);
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/valid/simple
static inline void _test_case_dbref_valid_simple(void) {
  /**
   * This is a simple valid DBRef element.
   */
  const uint8_t bytes[] = {
    0x20, 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'i', 'd', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/valid/simple-with-db
static inline void _test_case_dbref_valid_simple_with_db(void) {
  /**
   * A simple DBRef of the form:
   * 
   *     { $ref: "foo", $id: "bar", $db: "baz" }
   */
  const uint8_t bytes[] = {
    '-', 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'i', 'd', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 2, '$', 'd', 'b', 0, 4, 0,
    0, 0, 'b', 'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/valid/nested-id-doc
static inline void _test_case_dbref_valid_nested_id_doc(void) {
  /**
   * This is a valid DBRef of the form:
   * 
   *     { $ref: foo, $id: { $ref: "foo2", $id: "bar2", $db: "baz2" }, $db: "baz" }
   */
  const uint8_t bytes[] = {
    'U', 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 3,
    '$', 'i', 'd', 0, '0', 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 5, 0, 0, 0, 'f',
    'o', 'o', '2', 0, 2, '$', 'i', 'd', 0, 5, 0, 0, 0, 'b', 'a', 'r', '2', 0, 2,
    '$', 'd', 'b', 0, 5, 0, 0, 0, 'b', 'a', 'z', '2', 0, 0, 2, '$', 'd', 'b', 0,
    4, 0, 0, 0, 'b', 'a', 'z', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/valid/trailing-content
static inline void _test_case_dbref_valid_trailing_content(void) {
  /**
   * A valid DBRef of the form:
   * 
   *     {
   *         $ref: "foo",
   *         $id: "bar",
   *         $db: "baz",
   *         extra: "field",
   *     }
   */
  const uint8_t bytes[] = {
    0x3e, 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'i', 'd', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 2, '$', 'd', 'b', 0, 4, 0,
    0, 0, 'b', 'a', 'z', 0, 2, 'e', 'x', 't', 'r', 'a', 0, 6, 0, 0, 0, 'f', 'i',
    'e', 'l', 'd', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
// Case: dbref/valid/trailing-content-no-db
static inline void _test_case_dbref_valid_trailing_content_no_db(void) {
  /**
   * A valid DBRef of the form:
   * 
   *     {
   *         $ref: "foo",
   *         $id: "bar",
   *         extra: "field",
   *     }
   */
  const uint8_t bytes[] = {
    '1', 0, 0, 0, 2, '$', 'r', 'e', 'f', 0, 4, 0, 0, 0, 'f', 'o', 'o', 0, 2,
    '$', 'i', 'd', 0, 4, 0, 0, 0, 'b', 'a', 'r', 0, 2, 'e', 'x', 't', 'r', 'a',
    0, 6, 0, 0, 0, 'f', 'i', 'e', 'l', 'd', 0, 0
  };
  bson_t doc;
  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));
  bson_error_t error = {0};
  size_t offset = 999999;
  const bool is_valid = bson_validate_with_error_and_offset(&doc, BSON_VALIDATE_DOLLAR_KEYS, &offset, &error);
  ASSERT_OR_PRINT(is_valid, error);
  mlib_check(error.code, eq, 0);
  mlib_check(error.message, str_eq, "");
}

// ! This code is GENERATED! Do not edit it directly!
void test_install_generated_bson_validation(TestSuite* suite) {
  TestSuite_Add(suite, "/bson/validate/" "empty", _test_case_empty);
  TestSuite_Add(suite, "/bson/validate/" "bad-element", _test_case_bad_element);
  TestSuite_Add(suite, "/bson/validate/" "invalid-type", _test_case_invalid_type);
  TestSuite_Add(suite, "/bson/validate/" "key/invalid/accept", _test_case_key_invalid_accept);
  TestSuite_Add(suite, "/bson/validate/" "key/invalid/reject", _test_case_key_invalid_reject);
  TestSuite_Add(suite, "/bson/validate/" "key/empty/accept", _test_case_key_empty_accept);
  TestSuite_Add(suite, "/bson/validate/" "key/empty/reject", _test_case_key_empty_reject);
  TestSuite_Add(suite, "/bson/validate/" "key/empty/accept-if-absent", _test_case_key_empty_accept_if_absent);
  TestSuite_Add(suite, "/bson/validate/" "key/dot/accept", _test_case_key_dot_accept);
  TestSuite_Add(suite, "/bson/validate/" "key/dot/reject", _test_case_key_dot_reject);
  TestSuite_Add(suite, "/bson/validate/" "key/dot/accept-if-absent", _test_case_key_dot_accept_if_absent);
  TestSuite_Add(suite, "/bson/validate/" "key/dollar/accept", _test_case_key_dollar_accept);
  TestSuite_Add(suite, "/bson/validate/" "key/dollar/reject", _test_case_key_dollar_reject);
  TestSuite_Add(suite, "/bson/validate/" "key/dollar/accept-in-middle", _test_case_key_dollar_accept_in_middle);
  TestSuite_Add(suite, "/bson/validate/" "key/dollar/accept-if-absent", _test_case_key_dollar_accept_if_absent);
  TestSuite_Add(suite, "/bson/validate/" "utf8/simple", _test_case_utf8_simple);
  TestSuite_Add(suite, "/bson/validate/" "utf8/missing-null", _test_case_utf8_missing_null);
  TestSuite_Add(suite, "/bson/validate/" "utf8/length-zero", _test_case_utf8_length_zero);
  TestSuite_Add(suite, "/bson/validate/" "utf8/length-too-short", _test_case_utf8_length_too_short);
  TestSuite_Add(suite, "/bson/validate/" "utf8/header-too-large", _test_case_utf8_header_too_large);
  TestSuite_Add(suite, "/bson/validate/" "utf8/valid", _test_case_utf8_valid);
  TestSuite_Add(suite, "/bson/validate/" "utf8/invalid/accept", _test_case_utf8_invalid_accept);
  TestSuite_Add(suite, "/bson/validate/" "utf8/invalid/reject", _test_case_utf8_invalid_reject);
  TestSuite_Add(suite, "/bson/validate/" "utf8/valid-with-null/accept-1", _test_case_utf8_valid_with_null_accept_1);
  TestSuite_Add(suite, "/bson/validate/" "utf8/valid-with-null/accept-2", _test_case_utf8_valid_with_null_accept_2);
  TestSuite_Add(suite, "/bson/validate/" "utf8/valid-with-null/reject", _test_case_utf8_valid_with_null_reject);
  TestSuite_Add(suite, "/bson/validate/" "utf8/overlong-null/accept-1", _test_case_utf8_overlong_null_accept_1);
  TestSuite_Add(suite, "/bson/validate/" "utf8/overlong-null/accept-2", _test_case_utf8_overlong_null_accept_2);
  TestSuite_Add(suite, "/bson/validate/" "utf8/overlong-null/reject", _test_case_utf8_overlong_null_reject);
  TestSuite_Add(suite, "/bson/validate/" "utf8-key/invalid/accept", _test_case_utf8_key_invalid_accept);
  TestSuite_Add(suite, "/bson/validate/" "utf8-key/invalid/reject", _test_case_utf8_key_invalid_reject);
  TestSuite_Add(suite, "/bson/validate/" "utf8-key/overlong-null/reject", _test_case_utf8_key_overlong_null_reject);
  TestSuite_Add(suite, "/bson/validate/" "utf8-key/overlong-null/accept", _test_case_utf8_key_overlong_null_accept);
  TestSuite_Add(suite, "/bson/validate/" "array/empty", _test_case_array_empty);
  TestSuite_Add(suite, "/bson/validate/" "array/simple", _test_case_array_simple);
  TestSuite_Add(suite, "/bson/validate/" "array/invalid-element", _test_case_array_invalid_element);
  TestSuite_Add(suite, "/bson/validate/" "array/invalid-element-check-offset", _test_case_array_invalid_element_check_offset);
  TestSuite_Add(suite, "/bson/validate/" "symbol/simple", _test_case_symbol_simple);
  TestSuite_Add(suite, "/bson/validate/" "symbol/invalid-utf8/accept", _test_case_symbol_invalid_utf8_accept);
  TestSuite_Add(suite, "/bson/validate/" "symbol/invalid-utf8/reject", _test_case_symbol_invalid_utf8_reject);
  TestSuite_Add(suite, "/bson/validate/" "symbol/length-zero", _test_case_symbol_length_zero);
  TestSuite_Add(suite, "/bson/validate/" "symbol/length-too-short", _test_case_symbol_length_too_short);
  TestSuite_Add(suite, "/bson/validate/" "code/simple", _test_case_code_simple);
  TestSuite_Add(suite, "/bson/validate/" "code/invalid-utf8/accept", _test_case_code_invalid_utf8_accept);
  TestSuite_Add(suite, "/bson/validate/" "code/invalid-utf8/reject", _test_case_code_invalid_utf8_reject);
  TestSuite_Add(suite, "/bson/validate/" "code/length-zero", _test_case_code_length_zero);
  TestSuite_Add(suite, "/bson/validate/" "code/length-too-short", _test_case_code_length_too_short);
  TestSuite_Add(suite, "/bson/validate/" "code-with-scope/simple", _test_case_code_with_scope_simple);
  TestSuite_Add(suite, "/bson/validate/" "code-with-scope/invalid-code-length-zero", _test_case_code_with_scope_invalid_code_length_zero);
  TestSuite_Add(suite, "/bson/validate/" "code-with-scope/invalid-code-length-too-large", _test_case_code_with_scope_invalid_code_length_too_large);
  TestSuite_Add(suite, "/bson/validate/" "code-with-scope/invalid-scope", _test_case_code_with_scope_invalid_scope);
  TestSuite_Add(suite, "/bson/validate/" "code-with-scope/empty-key-in-scope", _test_case_code_with_scope_empty_key_in_scope);
  TestSuite_Add(suite, "/bson/validate/" "code-with-scope/corrupt-scope", _test_case_code_with_scope_corrupt_scope);
  TestSuite_Add(suite, "/bson/validate/" "code-with-scope/corrupt-scope-2", _test_case_code_with_scope_corrupt_scope_2);
  TestSuite_Add(suite, "/bson/validate/" "regex/simple", _test_case_regex_simple);
  TestSuite_Add(suite, "/bson/validate/" "regex/invalid-opts", _test_case_regex_invalid_opts);
  TestSuite_Add(suite, "/bson/validate/" "regex/double-null", _test_case_regex_double_null);
  TestSuite_Add(suite, "/bson/validate/" "regex/invalid-utf8/accept", _test_case_regex_invalid_utf8_accept);
  TestSuite_Add(suite, "/bson/validate/" "regex/invalid-utf8/reject", _test_case_regex_invalid_utf8_reject);
  TestSuite_Add(suite, "/bson/validate/" "regex/invalid-utf8/accept-if-absent", _test_case_regex_invalid_utf8_accept_if_absent);
  TestSuite_Add(suite, "/bson/validate/" "dbpointer/string-length-zero", _test_case_dbpointer_string_length_zero);
  TestSuite_Add(suite, "/bson/validate/" "dbpointer/string-length-too-big", _test_case_dbpointer_string_length_too_big);
  TestSuite_Add(suite, "/bson/validate/" "dbpointer/truncated", _test_case_dbpointer_truncated);
  TestSuite_Add(suite, "/bson/validate/" "dbpointer/missing-null", _test_case_dbpointer_missing_null);
  TestSuite_Add(suite, "/bson/validate/" "dbpointer/invalid-utf8/accept", _test_case_dbpointer_invalid_utf8_accept);
  TestSuite_Add(suite, "/bson/validate/" "dbpointer/invalid-utf8/reject", _test_case_dbpointer_invalid_utf8_reject);
  TestSuite_Add(suite, "/bson/validate/" "dbpointer/invalid-utf8/accept-if-absent", _test_case_dbpointer_invalid_utf8_accept_if_absent);
  TestSuite_Add(suite, "/bson/validate/" "subdoc/simple", _test_case_subdoc_simple);
  TestSuite_Add(suite, "/bson/validate/" "subdoc/invalid-shared-null", _test_case_subdoc_invalid_shared_null);
  TestSuite_Add(suite, "/bson/validate/" "subdoc/overlapping-utf8-null", _test_case_subdoc_overlapping_utf8_null);
  TestSuite_Add(suite, "/bson/validate/" "subdoc/invalid-element", _test_case_subdoc_invalid_element);
  TestSuite_Add(suite, "/bson/validate/" "subdoc/header-too-large", _test_case_subdoc_header_too_large);
  TestSuite_Add(suite, "/bson/validate/" "subdoc/header-too-small", _test_case_subdoc_header_too_small);
  TestSuite_Add(suite, "/bson/validate/" "subdoc/impossible-size", _test_case_subdoc_impossible_size);
  TestSuite_Add(suite, "/bson/validate/" "null/simple", _test_case_null_simple);
  TestSuite_Add(suite, "/bson/validate/" "undefined/simple", _test_case_undefined_simple);
  TestSuite_Add(suite, "/bson/validate/" "binary/simple", _test_case_binary_simple);
  TestSuite_Add(suite, "/bson/validate/" "binary/bad-length-zero-subtype-2", _test_case_binary_bad_length_zero_subtype_2);
  TestSuite_Add(suite, "/bson/validate/" "binary/bad-inner-length-on-subtype-2", _test_case_binary_bad_inner_length_on_subtype_2);
  TestSuite_Add(suite, "/bson/validate/" "binary/bad-length-too-small", _test_case_binary_bad_length_too_small);
  TestSuite_Add(suite, "/bson/validate/" "binary/bad-length-too-big", _test_case_binary_bad_length_too_big);
  TestSuite_Add(suite, "/bson/validate/" "binary/old-invalid/1", _test_case_binary_old_invalid_1);
  TestSuite_Add(suite, "/bson/validate/" "binary/old-invalid/2", _test_case_binary_old_invalid_2);
  TestSuite_Add(suite, "/bson/validate/" "minkey/simple", _test_case_minkey_simple);
  TestSuite_Add(suite, "/bson/validate/" "maxkey/simple", _test_case_maxkey_simple);
  TestSuite_Add(suite, "/bson/validate/" "int32/simple", _test_case_int32_simple);
  TestSuite_Add(suite, "/bson/validate/" "int32/truncated", _test_case_int32_truncated);
  TestSuite_Add(suite, "/bson/validate/" "timestamp/simple", _test_case_timestamp_simple);
  TestSuite_Add(suite, "/bson/validate/" "timestamp/truncated", _test_case_timestamp_truncated);
  TestSuite_Add(suite, "/bson/validate/" "int64/simple", _test_case_int64_simple);
  TestSuite_Add(suite, "/bson/validate/" "int64/truncated", _test_case_int64_truncated);
  TestSuite_Add(suite, "/bson/validate/" "double/simple", _test_case_double_simple);
  TestSuite_Add(suite, "/bson/validate/" "double/truncated", _test_case_double_truncated);
  TestSuite_Add(suite, "/bson/validate/" "boolean/simple-false", _test_case_boolean_simple_false);
  TestSuite_Add(suite, "/bson/validate/" "boolean/simple-true", _test_case_boolean_simple_true);
  TestSuite_Add(suite, "/bson/validate/" "boolean/invalid", _test_case_boolean_invalid);
  TestSuite_Add(suite, "/bson/validate/" "datetime/simple", _test_case_datetime_simple);
  TestSuite_Add(suite, "/bson/validate/" "datetime/truncated", _test_case_datetime_truncated);
  TestSuite_Add(suite, "/bson/validate/" "dbref/missing-id", _test_case_dbref_missing_id);
  TestSuite_Add(suite, "/bson/validate/" "dbref/non-id", _test_case_dbref_non_id);
  TestSuite_Add(suite, "/bson/validate/" "dbref/not-first-elements", _test_case_dbref_not_first_elements);
  TestSuite_Add(suite, "/bson/validate/" "dbref/ref-without-id-with-db", _test_case_dbref_ref_without_id_with_db);
  TestSuite_Add(suite, "/bson/validate/" "dbref/non-string-ref", _test_case_dbref_non_string_ref);
  TestSuite_Add(suite, "/bson/validate/" "dbref/non-string-db", _test_case_dbref_non_string_db);
  TestSuite_Add(suite, "/bson/validate/" "dbref/invalid-extras-between", _test_case_dbref_invalid_extras_between);
  TestSuite_Add(suite, "/bson/validate/" "dbref/invalid-double-ref", _test_case_dbref_invalid_double_ref);
  TestSuite_Add(suite, "/bson/validate/" "dbref/invalid-missing-ref", _test_case_dbref_invalid_missing_ref);
  TestSuite_Add(suite, "/bson/validate/" "dbref/valid/simple", _test_case_dbref_valid_simple);
  TestSuite_Add(suite, "/bson/validate/" "dbref/valid/simple-with-db", _test_case_dbref_valid_simple_with_db);
  TestSuite_Add(suite, "/bson/validate/" "dbref/valid/nested-id-doc", _test_case_dbref_valid_nested_id_doc);
  TestSuite_Add(suite, "/bson/validate/" "dbref/valid/trailing-content", _test_case_dbref_valid_trailing_content);
  TestSuite_Add(suite, "/bson/validate/" "dbref/valid/trailing-content-no-db", _test_case_dbref_valid_trailing_content_no_db);
}
