#include <mlib/ckdint.h>

#include <mlib/test.h>

#include <cstdlib>
#include <initializer_list>
#include <limits>

#define have_ckdint_builtins() 0
// Check for the stdckdint builtins, but don't use Clang, as it has bugs on some platforms.
#if defined(__has_builtin) && !defined(__clang__)
#if __has_builtin(__builtin_add_overflow)
#undef have_ckdint_builtins
#define have_ckdint_builtins() 1
#endif // __has_builtin
#endif

template <typename... Ts> struct typelist {
};
using integer_types = typelist<char,
                               unsigned char,
                               signed char,
                               short,
                               unsigned short,
                               int,
                               unsigned int,
                               long,
                               unsigned long,
                               long long,
                               unsigned long long>;

/**
 * @brief For every "interesting" value of integer type `T`, call `F`
 *
 * @tparam T An integer type to be inspected
 * @param fn A function to be called with every "interesting value"
 */
template <typename T, typename F>
void
with_interesting_values(F &&fn)
{
   using lim = std::numeric_limits<T>;
   // A list of the values of T that are potentially problematic
   // when combined in various ways.
   const T interesting_values[] = {
      0,
      (T)(-1),
      (T)(-2),
      (T)(-10),
      1,
      2,
      10,
      // Min value
      lim::min(),
      // Half min
      (T)(lim::min() / 2),
      (T)(lim::min() / 2 + 1),
      (T)(lim::min() / 2 - 1),
      // Max value
      lim::max(),
      // Half max
      (T)(lim::max() / 2),
      (T)(lim::max() / 2 + 1),
      (T)(lim::max() / 2 - 1),
   };
   // Call with each value:
   for (T v : interesting_values) {
      fn(v);
   }
}

/**
 * @brief Perform one set of tests with the given integer values
 *
 * @tparam Dst The target type for the operation
 * @param lhs The left-hand operand
 * @param rhs The right-hand operand
 *
 * If you identify a problematic combination, you can call this
 * directly in `main` with the known-bad values for easier debugging.
 *
 * This function is a no-op if the compiler does not provide the checked-arithmetic
 * builtins.
 */
template <typename Dst, typename L, typename R>
void
test_case(L lhs, R rhs)
{
   (void)lhs;
   (void)rhs;
#if have_ckdint_builtins()
   Dst mres, gres;
   // Test addition:
   mlib_check(mlib_add(&mres, lhs, rhs), eq, __builtin_add_overflow(lhs, rhs, &gres));
   mlib_check(mres, eq, gres);

   // Test subtraction:
   mlib_check(mlib_sub(&mres, lhs, rhs), eq, __builtin_sub_overflow(lhs, rhs, &gres));
   mlib_check(mres, eq, gres);

   // Test multiplication
   mlib_check(mlib_mul(&mres, lhs, rhs), eq, __builtin_mul_overflow(lhs, rhs, &gres));
   mlib_check(mres, eq, gres);

   // Test narrowing (both operands)
   mlib_check(mlib_narrow(&mres, lhs), eq, __builtin_add_overflow(lhs, 0, &gres));
   mlib_check(mres, eq, gres);
   mlib_check(mlib_narrow(&mres, rhs), eq, __builtin_add_overflow(rhs, 0, &gres));
   mlib_check(mres, eq, gres);
#endif
}

template <typename Dst, typename L, typename R>
int
test_arithmetic()
{
   with_interesting_values<L>([&](L lhs) {    //
      with_interesting_values<R>([&](R rhs) { //
         test_case<Dst>(lhs, rhs);
      });
   });
   return 0;
}

template <typename Dst, typename Lhs, typename... Rhs>
int
test_rhs(typelist<Rhs...>)
{
   // Call with every Rhs type
   auto arr = {test_arithmetic<Dst, Lhs, Rhs>()...};
   (void)arr;
   return 0;
}

template <typename Dest, typename... Lhs>
int
test_lhs(typelist<Lhs...>)
{
   // Expand to a call of test_rhs for every Lhs type
   auto arr = {test_rhs<Dest, Lhs>(integer_types{})...};
   (void)arr;
   return 0;
}

template <typename... Dst>
void
test_dst_types(typelist<Dst...>)
{
   // Expand to a call of test_lhs for each Dst type
   auto arr = {test_lhs<Dst>(integer_types{})...};
   (void)arr;
}

int
main()
{
   // Test that the dest can be used as an operand simultaneously:
   int a = 42;
   mlib_add(&a, a, 5);    // `a` is both an addend and the dst
   mlib_check(a, eq, 47); // Check that the addition respected the `42`

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

   // Test all integer types:
   test_dst_types(integer_types{});
   if (!have_ckdint_builtins()) {
      puts("@@ctest-skipped@@ - No __builtin_<op>_overflow builtins to test against");
   }
}
