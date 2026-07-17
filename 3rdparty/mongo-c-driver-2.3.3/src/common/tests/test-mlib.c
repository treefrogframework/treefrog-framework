#include <mlib/ckdint.h>
#include <mlib/cmp.h>
#include <mlib/config.h>
#include <mlib/duration.h>
#include <mlib/intencode.h>
#include <mlib/intutil.h>
#include <mlib/loop.h>
#include <mlib/str.h>
#include <mlib/test.h>
#include <mlib/time_point.h>
#include <mlib/timer.h>

#include <TestSuite.h>

#include <stddef.h>

#undef min // Used as a time unit suffix

mlib_diagnostic_push(); // We don't set any diagnostics, we just want to make sure it compiles

// Not relevant, we just want to test that it compiles:
mlib_msvc_warning(disable : 4507);

static void
_test_checks(void)
{
   // Simple condiion
   mlib_check(true);
   mlib_assert_aborts () {
      mlib_check(false);
   }
   // str_eq
   mlib_check("foo", str_eq, "foo");
   mlib_assert_aborts () {
      mlib_check("foo", str_eq, "bar");
   }
   // ptr_eq
   const char *s = "foo";
   mlib_check(s, ptr_eq, s);
   mlib_assert_aborts () {
      mlib_check(s, ptr_eq, NULL);
   }
   // eq
   mlib_check(4, eq, 4);
   mlib_assert_aborts () {
      mlib_check(1, eq, 4);
   }
   // neq
   mlib_check(1, neq, 4);
   mlib_assert_aborts () {
      mlib_check(1, neq, 1);
   }
   // "because" string
   mlib_check(true, because, "just true");
   mlib_assert_aborts () {
      mlib_check(false, because, "this will fail");
   }
   // lt
   mlib_check(1, lt, 4);
   mlib_assert_aborts () {
      mlib_check(4, lt, 1);
   }
   mlib_check(1, lte, 4);
   mlib_check(1, lte, 1);
   mlib_assert_aborts () {
      mlib_check(4, lte, 3);
   }
   // gt
   mlib_check(4, gt, 2);
   mlib_assert_aborts () {
      mlib_check(3, gt, 5);
   }
   mlib_check(3, gte, 2);
   mlib_check(3, gte, 3);
   mlib_assert_aborts () {
      mlib_check(3, gte, 5);
   }

   // An infix with a reason string
   mlib_check(1, eq, 1, because, "1 = 1");
}

static void
_test_bits(void)
{
   mlib_check(mlib_bits(0, 0), eq, 0);           // 0b000
   mlib_check(mlib_bits(1, 0), eq, 1);           // 0b001
   mlib_check(mlib_bits(2, 0), eq, 3);           // 0b011
   mlib_check(mlib_bits(1, 1), eq, 2);           // 0b010
   mlib_check(mlib_bits(5, 3), eq, 248);         // 0b11111000
   mlib_check(mlib_bits(64, 0), eq, UINT64_MAX); // 0b111...
}

static void
_test_minmax(void)
{
   mlib_static_assert(mlib_minof(unsigned) == 0);
   // Ambiguous signedness, still works:
   mlib_static_assert(mlib_minof(char) == CHAR_MIN);
   mlib_static_assert(mlib_maxof(char) == CHAR_MAX);

   mlib_static_assert(mlib_minof(uint8_t) == 0);
   mlib_static_assert(mlib_maxof(uint8_t) == UINT8_MAX);
   mlib_static_assert(mlib_minof(uint16_t) == 0);
   mlib_static_assert(mlib_maxof(uint16_t) == UINT16_MAX);
   mlib_static_assert(mlib_minof(uint32_t) == 0);
   mlib_static_assert(mlib_maxof(uint32_t) == UINT32_MAX);
   mlib_static_assert(mlib_minof(uint64_t) == 0);
   mlib_static_assert(mlib_maxof(uint64_t) == UINT64_MAX);

   mlib_static_assert(mlib_maxof(size_t) == SIZE_MAX);
   mlib_static_assert(mlib_maxof(ptrdiff_t) == PTRDIFF_MAX);

   mlib_static_assert(mlib_minof(int) == INT_MIN);
   mlib_static_assert(mlib_maxof(int) == INT_MAX);
   mlib_static_assert(mlib_maxof(unsigned) == UINT_MAX);

   mlib_static_assert(mlib_is_signed(int));
   mlib_static_assert(mlib_is_signed(signed char));
   mlib_static_assert(mlib_is_signed(int8_t));
   mlib_static_assert(!mlib_is_signed(uint8_t));
   mlib_static_assert(mlib_is_signed(int16_t));
   mlib_static_assert(!mlib_is_signed(uint16_t));
   mlib_static_assert(mlib_is_signed(int32_t));
   mlib_static_assert(!mlib_is_signed(uint32_t));
   mlib_static_assert(mlib_is_signed(int64_t));
   mlib_static_assert(!mlib_is_signed(uint64_t));
   // Ambiguous signedness:
   mlib_static_assert(mlib_is_signed(char) || !mlib_is_signed(char));
}

static void
_test_upsize(void)
{
   struct mlib_upsized_integer up;
   up = mlib_upsize_integer(31);
   mlib_check(up.is_signed);
   mlib_check(up.bits.as_signed == 31);

   // Casting from the max unsigned integer generates an unsigned upsized integer:
   up = mlib_upsize_integer((uintmax_t)1729);
   mlib_check(!up.is_signed);
   mlib_check(up.bits.as_unsigned == 1729);

   // Max signed integer makes a signed upsized integer:
   up = mlib_upsize_integer((intmax_t)1729);
   mlib_check(up.is_signed);
   mlib_check(up.bits.as_signed == 1729);

   // From a literal:
   up = mlib_upsize_integer(UINTMAX_MAX);
   mlib_check(!up.is_signed);
   mlib_check(up.bits.as_unsigned == UINTMAX_MAX);
}

static void
_test_cmp(void)
{
   mlib_check(mlib_cmp(1, 2) == mlib_less);
   mlib_check(mlib_cmp(1, 2) < 0);
   mlib_check(mlib_cmp(1, <, 2));
   mlib_check(mlib_cmp(2, 1) == mlib_greater);
   mlib_check(mlib_cmp(2, 1) > 0);
   mlib_check(mlib_cmp(2, >, 1));
   mlib_check(mlib_cmp(1, 1) == mlib_equal);
   mlib_check(mlib_cmp(1, 1) == 0);
   mlib_check(mlib_cmp(1, ==, 1));

   ASSERT(mlib_cmp(0, ==, 0));
   ASSERT(!mlib_cmp(0, ==, -1));
   ASSERT(!mlib_cmp(0, ==, 1));
   ASSERT(!mlib_cmp(-1, ==, 0));
   ASSERT(mlib_cmp(-1, ==, -1));
   ASSERT(!mlib_cmp(-1, ==, 1));
   ASSERT(!mlib_cmp(1, ==, 0));
   ASSERT(!mlib_cmp(1, ==, -1));
   ASSERT(mlib_cmp(1, ==, 1));

   ASSERT(mlib_cmp(0u, ==, 0u));
   ASSERT(!mlib_cmp(0u, ==, 1u));
   ASSERT(!mlib_cmp(1u, ==, 0u));
   ASSERT(mlib_cmp(1u, ==, 1u));

   ASSERT(mlib_cmp(0, ==, 0u));
   ASSERT(!mlib_cmp(0, ==, 1u));
   ASSERT(!mlib_cmp(-1, ==, 0u));
   ASSERT(!mlib_cmp(-1, ==, 1u));
   ASSERT(!mlib_cmp(1, ==, 0u));
   ASSERT(mlib_cmp(1, ==, 1u));

   ASSERT(mlib_cmp(0u, ==, 0));
   ASSERT(!mlib_cmp(0u, ==, -1));
   ASSERT(!mlib_cmp(0u, ==, 1));
   ASSERT(!mlib_cmp(1u, ==, 0));
   ASSERT(!mlib_cmp(1u, ==, -1));
   ASSERT(mlib_cmp(1u, ==, 1));

   ASSERT(!mlib_cmp(0, !=, 0));
   ASSERT(mlib_cmp(0, !=, -1));
   ASSERT(mlib_cmp(0, !=, 1));
   ASSERT(mlib_cmp(-1, !=, 0));
   ASSERT(!mlib_cmp(-1, !=, -1));
   ASSERT(mlib_cmp(-1, !=, 1));
   ASSERT(mlib_cmp(1, !=, 0));
   ASSERT(mlib_cmp(1, !=, -1));
   ASSERT(!mlib_cmp(1, !=, 1));

   ASSERT(!mlib_cmp(0u, !=, 0u));
   ASSERT(mlib_cmp(0u, !=, 1u));
   ASSERT(mlib_cmp(1u, !=, 0u));
   ASSERT(!mlib_cmp(1u, !=, 1u));

   ASSERT(!mlib_cmp(0, !=, 0u));
   ASSERT(mlib_cmp(0, !=, 1u));
   ASSERT(mlib_cmp(-1, !=, 0u));
   ASSERT(mlib_cmp(-1, !=, 1u));
   ASSERT(mlib_cmp(1, !=, 0u));
   ASSERT(!mlib_cmp(1, !=, 1u));

   ASSERT(!mlib_cmp(0u, !=, 0));
   ASSERT(mlib_cmp(0u, !=, -1));
   ASSERT(mlib_cmp(0u, !=, 1));
   ASSERT(mlib_cmp(1u, !=, 0));
   ASSERT(mlib_cmp(1u, !=, -1));
   ASSERT(!mlib_cmp(1u, !=, 1));

   ASSERT(!mlib_cmp(0, <, 0));
   ASSERT(!mlib_cmp(0, <, -1));
   ASSERT(mlib_cmp(0, <, 1));
   ASSERT(mlib_cmp(-1, <, 0));
   ASSERT(!mlib_cmp(-1, <, -1));
   ASSERT(mlib_cmp(-1, <, 1));
   ASSERT(!mlib_cmp(1, <, 0));
   ASSERT(!mlib_cmp(1, <, -1));
   ASSERT(!mlib_cmp(1, <, 1));

   ASSERT(!mlib_cmp(0u, <, 0u));
   ASSERT(mlib_cmp(0u, <, 1u));
   ASSERT(!mlib_cmp(1u, <, 0u));
   ASSERT(!mlib_cmp(1u, <, 1u));

   ASSERT(!mlib_cmp(0, <, 0u));
   ASSERT(mlib_cmp(0, <, 1u));
   ASSERT(mlib_cmp(-1, <, 0u));
   ASSERT(mlib_cmp(-1, <, 1u));
   ASSERT(!mlib_cmp(1, <, 0u));
   ASSERT(!mlib_cmp(1, <, 1u));

   ASSERT(!mlib_cmp(0u, <, 0));
   ASSERT(!mlib_cmp(0u, <, -1));
   ASSERT(mlib_cmp(0u, <, 1));
   ASSERT(!mlib_cmp(1u, <, 0));
   ASSERT(!mlib_cmp(1u, <, -1));
   ASSERT(!mlib_cmp(1u, <, 1));

   ASSERT(!mlib_cmp(0, >, 0));
   ASSERT(mlib_cmp(0, >, -1));
   ASSERT(!mlib_cmp(0, >, 1));
   ASSERT(!mlib_cmp(-1, >, 0));
   ASSERT(!mlib_cmp(-1, >, -1));
   ASSERT(!mlib_cmp(-1, >, 1));
   ASSERT(mlib_cmp(1, >, 0));
   ASSERT(mlib_cmp(1, >, -1));
   ASSERT(!mlib_cmp(1, >, 1));

   ASSERT(!mlib_cmp(0u, >, 0u));
   ASSERT(!mlib_cmp(0u, >, 1u));
   ASSERT(mlib_cmp(1u, >, 0u));
   ASSERT(!mlib_cmp(1u, >, 1u));

   ASSERT(!mlib_cmp(0, >, 0u));
   ASSERT(!mlib_cmp(0, >, 1u));
   ASSERT(!mlib_cmp(-1, >, 0u));
   ASSERT(!mlib_cmp(-1, >, 1u));
   ASSERT(mlib_cmp(1, >, 0u));
   ASSERT(!mlib_cmp(1, >, 1u));

   ASSERT(!mlib_cmp(0u, >, 0));
   ASSERT(mlib_cmp(0u, >, -1));
   ASSERT(!mlib_cmp(0u, >, 1));
   ASSERT(mlib_cmp(1u, >, 0));
   ASSERT(mlib_cmp(1u, >, -1));
   ASSERT(!mlib_cmp(1u, >, 1));

   ASSERT(mlib_cmp(0, <=, 0));
   ASSERT(!mlib_cmp(0, <=, -1));
   ASSERT(mlib_cmp(0, <=, 1));
   ASSERT(mlib_cmp(-1, <=, 0));
   ASSERT(mlib_cmp(-1, <=, -1));
   ASSERT(mlib_cmp(-1, <=, 1));
   ASSERT(!mlib_cmp(1, <=, 0));
   ASSERT(!mlib_cmp(1, <=, -1));
   ASSERT(mlib_cmp(1, <=, 1));

   ASSERT(mlib_cmp(0u, <=, 0u));
   ASSERT(mlib_cmp(0u, <=, 1u));
   ASSERT(!mlib_cmp(1u, <=, 0u));
   ASSERT(mlib_cmp(1u, <=, 1u));

   ASSERT(mlib_cmp(0, <=, 0u));
   ASSERT(mlib_cmp(0, <=, 1u));
   ASSERT(mlib_cmp(-1, <=, 0u));
   ASSERT(mlib_cmp(-1, <=, 1u));
   ASSERT(!mlib_cmp(1, <=, 0u));
   ASSERT(mlib_cmp(1, <=, 1u));

   ASSERT(mlib_cmp(0u, <=, 0));
   ASSERT(!mlib_cmp(0u, <=, -1));
   ASSERT(mlib_cmp(0u, <=, 1));
   ASSERT(!mlib_cmp(1u, <=, 0));
   ASSERT(!mlib_cmp(1u, <=, -1));
   ASSERT(mlib_cmp(1u, <=, 1));

   ASSERT(mlib_cmp(0, >=, 0));
   ASSERT(mlib_cmp(0, >=, -1));
   ASSERT(!mlib_cmp(0, >=, 1));
   ASSERT(!mlib_cmp(-1, >=, 0));
   ASSERT(mlib_cmp(-1, >=, -1));
   ASSERT(!mlib_cmp(-1, >=, 1));
   ASSERT(mlib_cmp(1, >=, 0));
   ASSERT(mlib_cmp(1, >=, -1));
   ASSERT(mlib_cmp(1, >=, 1));

   ASSERT(mlib_cmp(0u, >=, 0u));
   ASSERT(!mlib_cmp(0u, >=, 1u));
   ASSERT(mlib_cmp(1u, >=, 0u));
   ASSERT(mlib_cmp(1u, >=, 1u));

   ASSERT(mlib_cmp(0, >=, 0u));
   ASSERT(!mlib_cmp(0, >=, 1u));
   ASSERT(!mlib_cmp(-1, >=, 0u));
   ASSERT(!mlib_cmp(-1, >=, 1u));
   ASSERT(mlib_cmp(1, >=, 0u));
   ASSERT(mlib_cmp(1, >=, 1u));

   ASSERT(mlib_cmp(0u, >=, 0));
   ASSERT(mlib_cmp(0u, >=, -1));
   ASSERT(!mlib_cmp(0u, >=, 1));
   ASSERT(mlib_cmp(1u, >=, 0));
   ASSERT(mlib_cmp(1u, >=, -1));
   ASSERT(mlib_cmp(1u, >=, 1));

   size_t big_size = SIZE_MAX;
   ASSERT(mlib_cmp(42, big_size) == mlib_less);
   ASSERT(mlib_cmp(big_size, big_size) == mlib_equal);
   ASSERT(mlib_cmp(big_size, SSIZE_MIN) == mlib_greater);
   uint8_t smol = 7;
   ASSERT(mlib_cmp(smol, SIZE_MAX) == mlib_less);
   int8_t ismol = -4;
   ASSERT(mlib_cmp(ismol, big_size) == mlib_less);

   /// Example: Getting the correct answer:
   // Unintuitive result due to integer promotion:
   mlib_diagnostic_push();
   mlib_gnu_warning_disable("-Wsign-compare");
   mlib_disable_constant_conditional_expression_warnings();
   mlib_msvc_warning(disable : 4308);
   ASSERT(-27 > 20u); // Deliberate signed -> unsigned implicit conversion check.
   mlib_diagnostic_pop();
   // mlib_cmp produces the correct answer:
   ASSERT(mlib_cmp(-27, <, 20u));

   // CDRIVER-6043: until VS 2019 (MSVC 19.20), compound literals seem to "escape" the expression or scope they are
   // meant to be in. This includes the compound literals used by the conditional operator in mlib_upsize_integer.
#if !defined(_MSC_VER) || _MSC_VER >= 1920
   {
      // Check that we do not double-evaluate the operand expression.
      intmax_t a = 4;
      mlib_check(mlib_cmp(++a, ==, 5));
      // We only increment once:
      mlib_check(a, eq, 5);
   }
#endif
}

static void
_test_in_range(void)
{
   const int64_t int8_min = INT8_MIN;
   const int64_t int8_max = INT8_MAX;
   const int64_t int32_min = INT32_MIN;
   const int64_t int32_max = INT32_MAX;

   const uint64_t uint8_max = UINT8_MAX;
   const uint64_t uint32_max = UINT32_MAX;

   const ssize_t ssize_min = SSIZE_MIN;
   const ssize_t ssize_max = SSIZE_MAX;

   ASSERT(!mlib_in_range(int8_t, 1729));
   ASSERT(!mlib_in_range(int, SIZE_MAX));
   ASSERT(mlib_in_range(size_t, SIZE_MAX));
   ASSERT(!mlib_in_range(size_t, -42));
   ASSERT(mlib_in_range(int8_t, -42));
   ASSERT(mlib_in_range(int8_t, -128));
   ASSERT(!mlib_in_range(int8_t, -129));

   ASSERT(!mlib_in_range(int8_t, int8_min - 1));
   ASSERT(mlib_in_range(int8_t, int8_min));
   ASSERT(mlib_in_range(int8_t, 0));
   ASSERT(mlib_in_range(int8_t, int8_max));
   ASSERT(!mlib_in_range(int8_t, int8_max + 1));

   ASSERT(mlib_in_range(int8_t, 0u));
   ASSERT(mlib_in_range(int8_t, (uint64_t)int8_max));
   ASSERT(!mlib_in_range(int8_t, (uint64_t)(int8_max + 1)));

   ASSERT(!mlib_in_range(uint8_t, int8_min - 1));
   ASSERT(!mlib_in_range(uint8_t, int8_min));
   ASSERT(mlib_in_range(uint8_t, 0));
   ASSERT(mlib_in_range(uint8_t, int8_max));
   ASSERT(mlib_in_range(uint8_t, int8_max + 1));
   ASSERT(mlib_in_range(uint8_t, (int64_t)uint8_max));
   ASSERT(!mlib_in_range(uint8_t, (int64_t)uint8_max + 1));

   ASSERT(mlib_in_range(uint8_t, 0u));
   ASSERT(mlib_in_range(uint8_t, uint8_max));
   ASSERT(!mlib_in_range(uint8_t, uint8_max + 1u));

   ASSERT(!mlib_in_range(int32_t, int32_min - 1));
   ASSERT(mlib_in_range(int32_t, int32_min));
   ASSERT(mlib_in_range(int32_t, 0));
   ASSERT(mlib_in_range(int32_t, int32_max));
   ASSERT(!mlib_in_range(int32_t, int32_max + 1));

   ASSERT(mlib_in_range(int32_t, 0u));
   ASSERT(mlib_in_range(int32_t, (uint64_t)int32_max));
   ASSERT(!mlib_in_range(int32_t, (uint64_t)(int32_max + 1)));

   ASSERT(!mlib_in_range(uint32_t, int32_min - 1));
   ASSERT(!mlib_in_range(uint32_t, int32_min));
   ASSERT(mlib_in_range(uint32_t, 0));
   ASSERT(mlib_in_range(uint32_t, int32_max));
   ASSERT(mlib_in_range(uint32_t, int32_max + 1));
   ASSERT(mlib_in_range(uint32_t, (int64_t)uint32_max));
   ASSERT(!mlib_in_range(uint32_t, (int64_t)uint32_max + 1));

   ASSERT(mlib_in_range(uint32_t, 0u));
   ASSERT(mlib_in_range(uint32_t, uint32_max));
   ASSERT(!mlib_in_range(uint32_t, uint32_max + 1u));

   ASSERT(mlib_in_range(ssize_t, ssize_min));
   ASSERT(mlib_in_range(ssize_t, 0));
   ASSERT(mlib_in_range(ssize_t, ssize_max));

   ASSERT(mlib_in_range(ssize_t, 0u));
   ASSERT(mlib_in_range(ssize_t, (size_t)ssize_max));
   ASSERT(!mlib_in_range(ssize_t, (size_t)ssize_max + 1u));

   ASSERT(!mlib_in_range(size_t, ssize_min));
   ASSERT(mlib_in_range(size_t, 0));
   ASSERT(mlib_in_range(size_t, ssize_max));

   ASSERT(mlib_in_range(size_t, 0u));
   ASSERT(mlib_in_range(size_t, (size_t)ssize_max));
   ASSERT(mlib_in_range(size_t, (size_t)ssize_max + 1u));
}

static void
_test_assert_aborts(void)
{
   int a = 0;
   mlib_assert_aborts () {
      a = 4;
      abort();
   }
   // Parent process is unaffected:
   ASSERT(a == 0);
}

static void
_test_int_encoding(void)
{
   {
      const char *buf = "\x01\x02\x03\x04";
      const uint32_t val = mlib_read_u32le(buf);
      mlib_check(val, eq, 0x04030201);
   }

   {
      char buf[9] = {0};
      char *o = mlib_write_i32le(buf, 0x01020304);
      mlib_check(o, ptr_eq, buf + 4);
      mlib_check(buf, str_eq, "\x04\x03\x02\x01");

      o = mlib_write_i32le(o, 42);
      mlib_check(o, ptr_eq, buf + 8);
      mlib_check(buf, str_eq, "\x04\x03\x02\x01*");

      o = mlib_write_i64le(buf, 0x0102030405060708);
      mlib_check(o, ptr_eq, buf + 8);
      mlib_check(buf, str_eq, "\x08\x07\x06\x05\x04\x03\x02\x01");
   }
}

static void
_test_int_parse(void)
{
   const int64_t bogus_value = 2424242424242424242;
   struct case_ {
      const char *in;
      int64_t value;
      int ec;
   } cases[] = {
      // Basics:
      {"0", 0},
      {"1", 1},
      {"+1", 1},
      {"-1", -1},
      // Differences from strtoll
      // We require at least one digit immediately
      {"a1", bogus_value, EINVAL},
      {"", bogus_value, EINVAL},
      // No space skipping
      {" 1", bogus_value, EINVAL},
      {" +42", bogus_value, EINVAL},
      // No trailing characters
      {"123a", bogus_value, EINVAL},
      // strtoll: Set ERANGE if the value is too large
      {"123456789123456789123", bogus_value, ERANGE},
      // Difference: We generate EINVAL if its not an integer, even if strtoll says ERANGE
      {"123456789123456789123abc", bogus_value, EINVAL},
      // Truncated prefix
      {"+", bogus_value, EINVAL},
      {"+0x", bogus_value, EINVAL},
      {"0x", bogus_value, EINVAL},
      {"-0b", bogus_value, EINVAL},
      {"0xff", 0xff},
      {"0xfr", bogus_value, EINVAL},
      {"0x0", 0},
      {"0o755", 0755},
      {"0755", 0755},
      // Boundary cases:
      {"9223372036854775807", INT64_MAX},
      {"-9223372036854775808", INT64_MIN},
   };
   mlib_foreach_arr (struct case_, test, cases) {
      int64_t value = bogus_value;
      int ec = mlib_i64_parse(mstr_cstring(test->in), &value);
      mlib_check(value, eq, test->value);
      mlib_check(ec, eq, test->ec);
   }

   {
      // Parsing stops after the three digits when we slice the string
      int64_t value;
      int ec = mlib_i64_parse(mstr_view_data("123abc", 3), &value);
      mlib_check(ec, eq, 0);
      mlib_check(value, eq, 123);
   }

   {
      // Does not try to parse after the "0x" when we slice
      int ec = mlib_i64_parse(mstr_view_data("0x123", 2), NULL);
      mlib_check(ec, eq, EINVAL);
   }

   {
      // Does not try to read past the "+" into stack memory
      char plus = '+';
      int ec = mlib_i64_parse(mstr_view_data(&plus, 1), NULL);
      mlib_check(ec, eq, EINVAL);
   }
}

static void
_test_foreach(void)
{
   int n_loops = 0;
   mlib_foreach_urange (i, 10) {
      fprintf(stderr, "i: %zu\n", i);
      fprintf(stderr, "counter: %zu\n", i_counter);
      ASSERT(i == loop.index);
      ASSERT(loop.first == (i == 0));
      ASSERT(loop.last == (i == 9));
      ++n_loops;
      (void)i;
      ASSERT(n_loops <= 10);
   }
   ASSERT(n_loops == 10);

   n_loops = 0;
   mlib_foreach_urange (i, 100) {
      if (i == 42) {
         break;
      }
      ++n_loops;
   }
   ASSERT(n_loops == 42);

   n_loops = 0;
   mlib_foreach_urange (i, 1729) {
      (void)i;
      ++n_loops;
   }
   ASSERT(n_loops == 1729);

   mlib_foreach_urange (i, 0) {
      (void)i;
      ASSERT(false); // Shouldn't ever enter the loop
   }

   n_loops = 0;
   mlib_foreach_urange (i, 4, 7) {
      ++n_loops;
      ASSERT(i >= 4);
      ASSERT(i < 7);
   }
   ASSERT(n_loops == 3);

   int arr[] = {1, 2, 3};
   int sum = 0;
   n_loops = 0;
   mlib_foreach_arr (int, n, arr) {
      mlib_check(n_loops, eq, loop.index);
      n_loops++;
      sum += *n;
      ASSERT(loop.first == (n == arr + 0));
      ASSERT(loop.last == (n == arr + 2));
   }
   ASSERT(sum == 6);
   ASSERT(n_loops == 3);
}

static void
_test_cast(void)
{
   int a = 1729;
   // Fine:
   int16_t i16 = mlib_assert_narrow(int16_t, a);
   ASSERT(i16 == 1729);
   // Fine:
   a = -6;
   i16 = mlib_assert_narrow(int16_t, a);
   ASSERT(i16 == -6);
   // Boundary:
   size_t sz = mlib_assert_narrow(size_t, SIZE_MAX);
   ASSERT(sz == SIZE_MAX);
   sz = mlib_assert_narrow(size_t, 0);
   ASSERT(sz == 0);
   // Boundary:
   sz = mlib_assert_narrow(size_t, SSIZE_MAX);
   ASSERT(sz == SSIZE_MAX);

   mlib_assert_aborts () {
      (void)mlib_assert_narrow(size_t, -4);
   }
   mlib_assert_aborts () {
      (void)mlib_assert_narrow(ssize_t, SIZE_MAX);
   }
}

// This is a "partial" test of the ckdint APIs. A fully exhaustive test set is defined
// in `mlib/ckdint.test.cpp`
static void
_test_ckdint_partial(void)
{
   // Small signed
   {
      int a = 42;
      mlib_check(!mlib_add(&a, a, 5)); // a = a + 5
      mlib_check(a, eq, 47);

      mlib_check(!mlib_add(&a, 5)); // a += 5
      mlib_check(a, eq, 52);

      // The `check` arithmetic functions should abort the process immediately
      mlib_assert_aborts () {
         mlib_assert_add(size_t, 41, -42);
      }
      mlib_assert_aborts () {
         mlib_assert_add(ptrdiff_t, 41, SIZE_MAX);
      }
      // Does not abort:
      const size_t sum = mlib_assert_add(size_t, -32, 33);
      mlib_check(sum, eq, 1);

      mlib_check(!mlib_add(&a, a, (size_t)123456));
      mlib_check(a, eq, 123508);

      a = 4;
      mlib_check(mlib_add(&a, a, INT_MAX)); // Indicates overflow
      mlib_check(a, eq, INT_MIN + 3);       // Result is wrapped

      a = -1;
      mlib_check(!mlib_add(&a, a, INT_MAX));
      mlib_check(a, eq, INT_MAX - 1);
   }

   // Small unsigned
   {
      unsigned a = 42;
      mlib_check(!mlib_add(&a, a, 5));
      mlib_check(a, eq, 47);

      mlib_check(!mlib_add(&a, a, INT_MAX));
      mlib_check(a, eq, (unsigned)INT_MAX + 47);
   }

   // Sub with small signed
   {
      int a = -1;
      mlib_check(mlib_sub(&a, INT_MAX, a)); // MAX - (-1) → MAX + 1
      mlib_check(a, eq, INT_MIN);

      a = -1;
      mlib_check(!mlib_sub(&a, INT_MIN, a)); // MIN - (-1) → MIN + 1
      mlib_check(a, eq, INT_MIN + 1);
   }

   // Max precision tests are more interesting, because they excercise the bit-manipulation
   // tricks in the arithmetic functions, while smaller ints are simple bounds checks
   // ==============
   // Maximum precision signed
   {
      intmax_t a = 42;
      mlib_check(!mlib_add(&a, a, 5));
      mlib_check(a, eq, 47);

      mlib_check(mlib_add(&a, 42, INTMAX_MAX)); // Overflows
      mlib_check(a, eq, INTMAX_MIN + 41);       // Wraps

      mlib_check(!mlib_sub(&a, -1, INTMAX_MIN)); // (-N) - (-M) is always well-defined
      mlib_check(a, eq, INTMAX_MAX);

      mlib_check(mlib_sub(&a, -2, INTMAX_MAX));
      mlib_check(a, eq, INTMAX_MAX);

      mlib_check(!mlib_sub(&a, 1, INTMAX_MAX));
      mlib_check(a, eq, INTMAX_MIN + 2);

      mlib_check(!mlib_mul(&a, 1, INTMAX_MAX));
      mlib_check(a, eq, INTMAX_MAX);

      mlib_check(mlib_mul(&a, 2, INTMAX_MAX));
      mlib_check(a, eq, -2);
      mlib_check(mlib_mul(&a, 3, INTMAX_MAX));
      mlib_check(a, eq, INTMAX_MAX - 2);
   }

   // Maximum precision unsigned
   {
      uintmax_t a = 42;
      mlib_check(!mlib_add(&a, a, 5));
      mlib_check(a, eq, 47);

      a = 42;
      mlib_check(mlib_add(&a, a, UINTMAX_MAX)); // Overflows
      mlib_check(a, eq, 41);                    // Wraps

      a = 1;
      mlib_check(mlib_sub(&a, a, INTMAX_MAX)); // Overflows (result is negative)
      mlib_check(a, eq, (uintmax_t)INTMAX_MAX + 3);

      mlib_check(!mlib_sub(&a, -1, INTMAX_MIN)); // (-N) - (-M) is always well-defined
      mlib_check(a, eq, INTMAX_MAX);

      mlib_check(mlib_sub(&a, -2, INTMAX_MAX));
      mlib_check(a, eq, INTMAX_MAX);

      mlib_check(mlib_sub(&a, 1, INTMAX_MAX));
      mlib_check(a, eq, (uintmax_t)INTMAX_MAX + 3);

      mlib_check(!mlib_mul(&a, 1, INTMAX_MAX));
      mlib_check(a, eq, INTMAX_MAX);

      // Just barely enough room:
      mlib_check(!mlib_mul(&a, 2, INTMAX_MAX));
      mlib_check(a, eq, UINTMAX_MAX - 1);
      // Too big:
      mlib_check(mlib_mul(&a, 3, INTMAX_MAX));
      mlib_check(a, eq, INTMAX_MAX - 2);
   }

   {
      // Check asserting form with a dest type smaller than intmax, which triggers
      // a special branch on big-endian systems.
      int32_t const i = mlib_assert_mul(int32_t, -908, 1000);
      mlib_check(i, eq, -908000);
   }
}

static void
_test_str_view(void)
{
   mstr_view sv = mstr_cstring("Hello, world!");
   mlib_check(sv.data, str_eq, "Hello, world!");

   mlib_check(mstr_cmp(sv, ==, mstr_cstring("Hello, world!")));
   mlib_check(mstr_cmp(sv, >, mstr_cstring("Hello")));
   // Longer strings are greater than shorter strings
   mlib_check(mstr_cmp(sv, <, mstr_cstring("ZZZZZ")));
   // str_view_from duplicates a string view:
   mlib_check(mstr_cmp(sv, ==, mstr_view_from(sv)));

   // Substring
   {
      sv = mstr_cstring("foobar");
      // Implicit length includes everything:
      mlib_check(mstr_cmp(mstr_substr(sv, 2), ==, mstr_cstring("obar")));
      // Explicit length trims:
      mlib_check(mstr_cmp(mstr_substr(sv, 2, 1), ==, mstr_cstring("o")));
      // Substring over the whole length:
      mlib_check(mstr_cmp(mstr_substr(sv, sv.len), ==, mstr_cstring("")));
   }

   // Substring from end
   {
      sv = mstr_cstring("foobar");
      mlib_check(mstr_cmp(mstr_substr(sv, -3), ==, mstr_cstring("bar")));
      mlib_check(mstr_cmp(mstr_substr(sv, -6), ==, mstr_cstring("foobar")));
   }

   // Searching forward:
   {
      sv = mstr_cstring("foobar");
      mlib_check(mstr_find(sv, mstr_cstring("foo")), eq, 0);
      mlib_check(mstr_find(sv, mstr_cstring("o")), eq, 1);
      mlib_check(mstr_find(sv, mstr_cstring("foof")), eq, SIZE_MAX);
      mlib_check(mstr_find(sv, mstr_cstring("bar")), eq, 3);
      mlib_check(mstr_find(sv, mstr_cstring("barf")), eq, SIZE_MAX);
      // Start at index 3
      mlib_check(mstr_find(sv, mstr_cstring("bar"), 3), eq, 3);
      // Starting beyond the ocurrence will fail:
      mlib_check(mstr_find(sv, mstr_cstring("b"), 4), eq, SIZE_MAX);
      // Empty string is found immediately:
      mlib_check(mstr_find(sv, mstr_cstring("")), eq, 0);
   }

   {
      // Searching for certain chars
      mstr_view digits = mstr_cstring("1234567890");
      // The needle chars never occur, so returns SIZE_MAX
      mlib_check(mstr_find_first_of(mstr_cstring("foobar"), digits), eq, SIZE_MAX);
      // `1` at the fourth pos
      mlib_check(mstr_find_first_of(mstr_cstring("foo1barbaz4"), digits), eq, 3);
      // `1` at the fourth pos, with a trimmed window:
      mlib_check(mstr_find_first_of(mstr_cstring("foo1barbaz4"), digits, 3), eq, 3);
      // `4` is found, since we drop the `1` from the window:
      mlib_check(mstr_find_first_of(mstr_cstring("foo1barbaz4"), digits, 4), eq, 10);
      // Empty needles string is never found in any string
      mlib_check(mstr_find_first_of(mstr_cstring("foo bar baz"), mstr_cstring("")), eq, SIZE_MAX);
      // Find at the end of the string
      mlib_check(mstr_find_first_of(mstr_cstring("foo bar baz"), mstr_cstring("z")), eq, 10);
   }

   // Splitting
   {
      sv = mstr_cstring("foo bar baz");
      mstr_view a, b;
      // Trim at index 3, drop one char:
      mstr_split_at(sv, 3, 1, &a, &b);
      mlib_check(mstr_cmp(a, ==, mstr_cstring("foo")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("bar baz")));
      // Trim at index 3, default drop=0:
      mstr_split_at(sv, 3, &a, &b);
      mlib_check(mstr_cmp(a, ==, mstr_cstring("foo")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring(" bar baz")));
      // Trim past-the-end
      mstr_split_at(sv, 5000, &a, &b);
      mlib_check(mstr_cmp(a, ==, mstr_cstring("foo bar baz")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("")));
      // Drop too many:
      mstr_split_at(sv, 0, 5000, &a, &b);
      mlib_check(mstr_cmp(a, ==, mstr_cstring("")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("")));
      // Past-the-end and also drop
      mstr_split_at(sv, 4000, 42, &a, &b);
      mlib_check(mstr_cmp(a, ==, mstr_cstring("foo bar baz")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("")));

      // Split using a negative index
      mstr_split_at(sv, -4, 1, &a, &b);
      mlib_check(mstr_cmp(a, ==, mstr_cstring("foo bar")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("baz")));
   }

   // Splitting around an infix
   {
      sv = mstr_cstring("foo bar baz");
      mstr_view a, b;
      // Split around the first space
      const mstr_view space = mstr_cstring(" ");
      mlib_check(mstr_split_around(sv, space, &a, &b));
      mlib_check(mstr_cmp(a, ==, mstr_cstring("foo")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("bar baz")));
      // Split again
      mlib_check(mstr_split_around(b, space, &a, &b));
      mlib_check(mstr_cmp(a, ==, mstr_cstring("bar")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("baz")));
      // Split again. This won't find a space, but will still do something
      mlib_check(!mstr_split_around(b, space, &a, &b));
      mlib_check(mstr_cmp(a, ==, mstr_cstring("baz")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("")));
      // Splitting on the final empty string does nothing
      mlib_check(!mstr_split_around(b, space, &a, &b));
      mlib_check(mstr_cmp(a, ==, mstr_cstring("")));
      mlib_check(mstr_cmp(b, ==, mstr_cstring("")));
   }

   // Case folding
   {
      mlib_check(mlib_latin_tolower('a'), eq, 'a');
      mlib_check(mlib_latin_tolower('z'), eq, 'z');
      mlib_check(mlib_latin_tolower('A'), eq, 'a');
      mlib_check(mlib_latin_tolower('Z'), eq, 'z');
      // Other chars are unchanged:
      mlib_check(mlib_latin_tolower('7'), eq, '7');
      mlib_check(mlib_latin_tolower('?'), eq, '?');
   }

   // Case-insensitive compare
   {
      mlib_check(mstr_latin_casecmp(mstr_cstring("foo"), ==, mstr_cstring("foo")));
      mlib_check(mstr_latin_casecmp(mstr_cstring("foo"), !=, mstr_cstring("bar")));
      mlib_check(mstr_latin_casecmp(mstr_cstring("Foo"), ==, mstr_cstring("foo")));
      mlib_check(mstr_latin_casecmp(mstr_cstring("Foo"), >, mstr_cstring("bar")));
      // "Food" < "foo" when case-sensitive ('F' < 'f'):
      mlib_check(mstr_cmp(mstr_cstring("Food"), <, mstr_cstring("foo")));
      // But "Food" > "foo" when case-insensitive:
      mlib_check(mstr_latin_casecmp(mstr_cstring("Food"), >, mstr_cstring("foo")));
   }

   // Trimming
   {
      mstr_view s = mstr_cstring("  foo bar  \n");
      mlib_check(mstr_cmp(mstr_trim_left(s), ==, mstr_cstring("foo bar  \n")));
      mlib_check(mstr_cmp(mstr_trim_right(s), ==, mstr_cstring("  foo bar")));
      mlib_check(mstr_cmp(mstr_trim(s), ==, mstr_cstring("foo bar")));
   }
}

static inline void
_test_str(void)
{
   // Simple empty
   {
      mstr s = mstr_new(0);
      // Length is zero
      mlib_check(s.len, eq, 0);
      // Data is not null for empty strings, since we want a null terminator
      mlib_check(s.data != NULL);
      // The null terminator is present:
      mlib_check(s.data[0], eq, 0);
      mstr_destroy(&s);
   }

   // Simple copy of a C string
   {
      mstr s = mstr_copy_cstring("foo bar");
      mlib_check(s.len, eq, 7);
      mlib_check(mstr_cmp(s, ==, mstr_cstring("foo bar")));
      mstr_destroy(&s);
   }

   // Concat two strings
   {
      mstr s = mstr_concat(mstr_cstring("foo"), mstr_cstring("bar"));
      mlib_check(mstr_cmp(s, ==, mstr_cstring("foobar")));
      mstr_destroy(&s);
   }

   // Append individual characters
   {
      mstr s = mstr_new(0);
      mstr_append_char(&s, 'f');
      mstr_append_char(&s, 'o');
      mstr_append_char(&s, 'o');
      mlib_check(mstr_cmp(s, ==, mstr_cstring("foo")));
      mstr_destroy(&s);
   }

   // Splice deletion
   {
      mstr s = mstr_copy_cstring("foo bar baz");
      mlib_check(mstr_splice(&s, 4, 3, mstr_cstring("")));
      mlib_check(mstr_cmp(s, ==, mstr_cstring("foo  baz")));

      mstr_assign(&s, mstr_cstring("foo bar baz"));
      mlib_check(mstr_splice(&s, 4, 3, mstr_cstring("quux")));
      mlib_check(mstr_cmp(s, ==, mstr_cstring("foo quux baz")));

      mstr_assign(&s, mstr_cstring("foo bar baz"));
      mlib_check(mstr_splice(&s, 4, 0, mstr_cstring("quux ")));
      mlib_check(mstr_cmp(s, ==, mstr_cstring("foo quux bar baz")));

      mstr_destroy(&s);
   }

   // Replacing
   {
      mstr s = mstr_copy_cstring("abcd abcd");
      mlib_check(mstr_replace(&s, mstr_cstring("b"), mstr_cstring("foo")));
      mlib_check(mstr_cmp(s, ==, mstr_cstring("afoocd afoocd")));

      // Try to replace where the replacement contains the needle
      mstr_assign(&s, mstr_cstring("foo bar baz"));
      mlib_check(mstr_replace(&s, mstr_cstring("bar"), mstr_cstring("foo bar baz")));
      // A naive impl would explode into an infinite string, but we don't try to replace
      // within the already-replaced content:
      mlib_check(s.data, str_eq, "foo foo bar baz baz");

      // Try to replace, where the needle is an empty string.
      mstr_assign(&s, mstr_cstring("foo"));
      mlib_check(mstr_replace(&s, mstr_cstring(""), mstr_cstring("bar")));
      mlib_check(s.data, str_eq, "barfbarobarobar");

      mstr_assign(&s, mstr_cstring("foofoofoo"));
      mlib_check(mstr_replace(&s, mstr_cstring("foo"), mstr_cstring("bar")));
      mlib_check(s.data, str_eq, "barbarbar");

      mstr_destroy(&s);
   }

   // Self-assignment/insertion
   {
      // Use `assign()` over itself
      mstr s = mstr_copy_cstring("foo bar baz");
      mstr_assign(&s, s);
      mlib_check(s.data, str_eq, "foo bar baz");

      // Use splice() where the insertion string overlaps the target
      mstr_view bar = mstr_substr(s, 4, 3);
      mlib_check(mstr_splice(&s, 0, 3, bar));
      mlib_check(mstr_splice(&s, 8, 3, bar));
      mlib_check(s.data, str_eq, "bar bar bar");

      // Use replace() with a needle that points into the target
      mstr_replace(&s, bar, mstr_cstring("foo"));
      mlib_check(s.data, str_eq, "foo foo foo");

      mstr_destroy(&s);
   }
}

static void
_test_str_format(void)
{
   mstr s = mstr_sprintf("foo");
   mlib_check(s.len, eq, 3);
   mlib_check(s.data, str_eq, "foo");
   mstr_sprintf_append(&s, " bar");
   mlib_check(s.data, str_eq, "foo bar");
   mstr_sprintf_append(&s, " %d", 123);
   mlib_check(s.data, str_eq, "foo bar 123");

   // Attempt to format into itself:
   mstr_sprintf_append(&s, " hey %s", s.data);
   mlib_check(s.data, str_eq, "foo bar 123 hey foo bar 123");

   mstr_destroy(&s);
}

static void
_test_duration(void)
{
   mlib_duration d = mlib_duration();
   mlib_check(mlib_microseconds_count(d), eq, 0);

   // Creating durations with the macro name
   d = mlib_duration();
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration()));
   d = mlib_duration(d);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration()));

   d = mlib_duration(10, ms);
   mlib_check(mlib_duration_cmp(d, ==, (10, ms)));
   d = mlib_duration(10, us);
   mlib_check(mlib_duration_cmp(d, ==, (10, us)));
   d = mlib_duration(10, s);
   mlib_check(mlib_duration_cmp(d, ==, (10, s)));

   d = mlib_duration((10, s), mul, 3);
   mlib_check(mlib_duration_cmp(d, ==, (30, s)));

   d = mlib_duration((10, s), plus, (40, ms));
   mlib_check(mlib_duration_cmp(d, ==, (10040, ms)));

   d = mlib_duration((10, s), div, 20);
   mlib_check(mlib_duration_cmp(d, ==, (500, ms)));

   d = mlib_duration((4, s), min, (400, ms));
   mlib_check(mlib_duration_cmp(d, ==, (400, ms)));

   d = mlib_duration(10, mn);
   mlib_check(mlib_duration_cmp(d, ==, (600, s)));

   d = mlib_duration(4, h);
   mlib_check(mlib_duration_cmp(d, ==, (240, mn)));

   d = mlib_duration((10, s), div, 20);
   d = mlib_duration(
      // At least 5 seconds:
      (d, max, (5, s)),
      // At most 90 seconds:
      min,
      (90, s));
   mlib_check(mlib_duration_cmp(d, ==, (5, s)));

   // Comparison
   mlib_check(mlib_duration_cmp(mlib_duration(4, s), mlib_duration(4, s)) == 0);
   mlib_check(mlib_duration_cmp(mlib_duration(4, s), mlib_duration(5, s)) < 0);
   mlib_check(mlib_duration_cmp(mlib_duration(4, s), mlib_duration(-5, s)) > 0);
   mlib_check(mlib_duration_cmp((4, s), ==, (4, s)));
   mlib_check(mlib_duration_cmp((4, s), <, (5, s)));
   mlib_check(mlib_duration_cmp((4, s), >, (-5, s)));

   // Overflow saturates:
   d = mlib_duration(mlib_maxof(mlib_duration_rep_t), s);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_max()));

   d = mlib_duration(d, mul, 16);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_max()));

   // Rounds toward zero
   d = mlib_duration(1050, ms);
   mlib_check(mlib_seconds_count(d), eq, 1);
   d = mlib_duration(-1050, ms);
   mlib_check(mlib_seconds_count(d), eq, -1);
   d = mlib_duration(1729, us);
   mlib_check(mlib_milliseconds_count(d), eq, 1);
   d = mlib_duration(-1729, us);
   mlib_check(mlib_milliseconds_count(d), eq, -1);

   d = mlib_duration((-3, s), plus, mlib_duration_min());
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_min()));
   d = mlib_duration((4, s), plus, mlib_duration_max());
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_max()));

   d = mlib_duration((4, s), minus, (2271, ms));
   mlib_check(mlib_milliseconds_count(d), eq, 1729);
   // Overflow saturates:
   d = mlib_duration((-4, ms), minus, mlib_duration_max());
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_min()));
   d = mlib_duration((4, ms), minus, mlib_duration_min());
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_max()));

   d = mlib_duration((4, s), mul, 5);
   mlib_check(mlib_duration_cmp(d, ==, (20, s)));
   d = mlib_duration(mlib_duration_max(), mul, 2);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_max()));
   d = mlib_duration(mlib_duration_max(), mul, -2);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_min()));
   d = mlib_duration(mlib_duration_min(), mul, 2);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_min()));
   d = mlib_duration(mlib_duration_min(), mul, -2);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_max()));

   d = mlib_duration(mlib_duration_max(), div, -1);
   mlib_check(mlib_duration_cmp(d, mlib_duration()) < 0);
   d = mlib_duration(mlib_duration_min(), div, -1);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration_max()));
   mlib_assert_aborts () {
      // Division by zero
      d = mlib_duration(d, div, 0);
   }

   // To/from timespec
   struct timespec ts;
   ts.tv_sec = 4;
   ts.tv_nsec = 0;
   d = mlib_duration_from_timespec(ts);
   mlib_check(mlib_duration_cmp(d, ==, (4, s)));
   //
   ts.tv_sec = -3;
   ts.tv_nsec = -4000;
   d = mlib_duration_from_timespec(ts);
   mlib_check(mlib_duration_cmp(d, ==, mlib_duration((-3000004, us), plus, (0, us))));
   //
   ts = mlib_duration_to_timespec(mlib_duration(-5000908, us));
   mlib_check(ts.tv_sec, eq, -5);
   mlib_check(ts.tv_nsec, eq, -908000);

// Test when min is defined (CDRIVER-6102)
#pragma push_macro("min")
#undef min
#define min(a, b) ((a) > (b) ? (b) : (a))
   d = mlib_duration((1, s), min, (2, s));
   mlib_check(mlib_duration_cmp(d, ==, (1, s)));
   d = mlib_duration(3, mn);
#pragma pop_macro("min")
}

static void
_test_time_point(void)
{
   mlib_time_point t = mlib_now();

   // Offset the time point
   mlib_time_point later = mlib_time_add(t, (1, s));
   mlib_check(mlib_time_cmp(t, <, later));

   // Difference between two time points is a duration:
   mlib_duration diff = mlib_time_difference(later, t);
   mlib_check(mlib_milliseconds_count(diff), eq, 1000);

   // The time is only ever monotonically increasing
   mlib_foreach_urange (i, 10000) {
      (void)i;
      mlib_check(mlib_time_cmp(t, <=, mlib_now()));
      t = mlib_now();
   }
}

static void
_test_sleep(void)
{
   mlib_time_point start = mlib_now();
   int rc = mlib_sleep_for(mlib_duration(50, ms));
   mlib_check(rc, eq, 0);
   mlib_duration t = mlib_time_difference(mlib_now(), start);
   mlib_check(mlib_milliseconds_count(t), gte, 45);
   mlib_check(mlib_milliseconds_count(t), lt, 200);

   // Sleeping for a negative duration returns immediately with success
   start = mlib_now();
   mlib_check(mlib_sleep_for(-10, s), eq, 0);
   mlib_check(mlib_duration_cmp(mlib_elapsed_since(start), <, (100, ms)));

   // Sleeping until a point in the past returns immediately as well
   mlib_check(mlib_sleep_until(start), eq, 0);
   mlib_check(mlib_duration_cmp(mlib_elapsed_since(start), <, (100, ms)));
}

static void
_test_timer(void)
{
   mlib_timer tm = mlib_expires_after(200, ms);
   mlib_check(!mlib_timer_is_expired(tm));
   mlib_sleep_for(250, ms);
   mlib_check(mlib_timer_is_expired(tm));

   // Test the once-var condition
   bool cond = false;
   // Says not expired on the first call
   mlib_check(!mlib_timer_is_expired(tm, &cond));
   mlib_check(cond); // Was set to `true`
   // Says it was expired on this call:
   mlib_check(mlib_timer_is_expired(tm, &cond));

   // Try with a not-yet-expired timer
   cond = false;
   tm = mlib_expires_after(10, s);
   mlib_check(!mlib_timer_is_expired(tm));
   mlib_check(!mlib_timer_is_expired(tm, &cond));
   // cond was set to `true`, even though we are not yet expired
   mlib_check(cond);

   // Create a timer that expired in the past
   tm = mlib_expires_at(mlib_time_add(mlib_now(), (-10, s)));
   mlib_check(mlib_timer_is_expired(tm));
}

// Tests for `int_vec` assert the behavior of the vector type when handling trivial
// elements.
#define T int
#include <mlib/vec.th>
static void
_test_int_vec(void)
{
   int_vec ints = int_vec_new();
   mlib_check(ints.size, eq, 0, because, "Initial vector is empty");

   // Append an element
   int *el;
   mlib_check((el = int_vec_push(&ints)));
   *el = 42;
   mlib_check(int_vec_begin(&ints)[0], eq, 42);
   mlib_check(ints.size, eq, 1);

   int_vec_erase_at(&ints, 0);
   mlib_check(ints.size, eq, 0, because, "We are back to an empty vector");

   *int_vec_push(&ints) = 42;
   *int_vec_push(&ints) = 1729;
   *int_vec_push(&ints) = 123456;
   *int_vec_push(&ints) = -7;
   mlib_check(ints.size, eq, 4, because, "We added four elements from empty");
   mlib_check(mlib_vec_at(ints, -1), eq, -7, because, "Negative index wraps");

   // Erase in the middle
   int_vec_erase(&ints, ints.data + 1, ints.data + 3);
   mlib_check(ints.size, eq, 2, because, "We erased two elements");
   mlib_check(mlib_vec_at(ints, 1), eq, -7, because, "We erased, so the back elements shifted down");
   mlib_check(mlib_vec_at(ints, -1), eq, -7, because, "-7 is still the last element");

   int_vec_destroy(&ints);
}

#define T char *
#define VecName cstring_vec
#define VecDestroyElement(CStrPtr) ((free(*CStrPtr), *CStrPtr = NULL))
#define VecCopyElement(Dst, Src) ((*Dst = strdup(*Src)))
#include <mlib/vec.th>
static void
_test_cstring_vec(void)
{
   // Simple new and destroy
   {
      cstring_vec v = cstring_vec_new();
      mlib_check(v.size, eq, 0);
      mlib_check(v.capacity, eq, 0);
      cstring_vec_destroy(&v);
   }
   // Simple new and push an element
   {
      cstring_vec v = cstring_vec_new();
      *cstring_vec_push(&v) = strdup("Hey");
      mlib_check(v.size, eq, 1);
      mlib_check(v.capacity, eq, 1);
      mlib_check(cstring_vec_begin(&v)[0], str_eq, "Hey");
      cstring_vec_destroy(&v);
   }
   // Copy an empty
   {
      cstring_vec v = cstring_vec_new();
      cstring_vec b;
      cstring_vec_init_copy(&b, &v);
      cstring_vec_destroy(&v);
      cstring_vec_destroy(&b);
   }
   // Copy non-empty
   {
      cstring_vec a = cstring_vec_new();
      *cstring_vec_push(&a) = strdup("Hello");
      *cstring_vec_push(&a) = strdup("world!");
      mlib_check(a.size, eq, 2);
      mlib_check(a.capacity, eq, 2);
      cstring_vec b;
      mlib_check(cstring_vec_init_copy(&b, &a));
      mlib_check(a.size, eq, 2);
      mlib_check(a.capacity, eq, 2);
      mlib_check(cstring_vec_begin(&a)[0], str_eq, "Hello");
      mlib_check(cstring_vec_begin(&b)[1], str_eq, "world!");
      cstring_vec_destroy(&b);
      cstring_vec_destroy(&a);
   }
}

void
test_mlib_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/mlib/checks", _test_checks);
   TestSuite_Add(suite, "/mlib/intutil/bits", _test_bits);
   TestSuite_Add(suite, "/mlib/intutil/minmax", _test_minmax);
   TestSuite_Add(suite, "/mlib/intutil/upsize", _test_upsize);
   TestSuite_Add(suite, "/mlib/cmp", _test_cmp);
   TestSuite_Add(suite, "/mlib/in-range", _test_in_range);
   TestSuite_Add(suite, "/mlib/assert-aborts", _test_assert_aborts);
   TestSuite_Add(suite, "/mlib/int-encoding", _test_int_encoding);
   TestSuite_Add(suite, "/mlib/int-parse", _test_int_parse);
   TestSuite_Add(suite, "/mlib/foreach", _test_foreach);
   TestSuite_Add(suite, "/mlib/check-cast", _test_cast);
   TestSuite_Add(suite, "/mlib/ckdint-partial", _test_ckdint_partial);
   TestSuite_Add(suite, "/mlib/str_view", _test_str_view);
   TestSuite_Add(suite, "/mlib/str", _test_str);
   TestSuite_Add(suite, "/mlib/str-format", _test_str_format);
   TestSuite_Add(suite, "/mlib/duration", _test_duration);
   TestSuite_Add(suite, "/mlib/time_point", _test_time_point);
   TestSuite_Add(suite, "/mlib/sleep", _test_sleep);
   TestSuite_Add(suite, "/mlib/timer", _test_timer);
   TestSuite_Add(suite, "/mlib/int-vector", _test_int_vec);
   TestSuite_Add(suite, "/mlib/string-vector", _test_cstring_vec);
}

mlib_diagnostic_pop();
