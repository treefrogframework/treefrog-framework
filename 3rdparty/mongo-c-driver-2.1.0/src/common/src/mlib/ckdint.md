# `mlib/ckdint.h` C23 `stdckdint.h` for C99

The `mlib/ckdint.h` header implements the [C23 checked arithmetic][stdckdint]
functionality in a C99-compatible manner. There are only three caveats to keep
in mind:

[stdckdint]: https://en.cppreference.com/w/c/numeric#Checked_integer_arithmetic

- The backport relies assumes two's complement signed integer encoding.
- The operand expressions of a call to the ckdint function-like macros may be
  evaluated more than once.
- The output parameter is read-from before the operation begins, meaning that it
  must be initialized to some value to prevent an uninitialized-read from being
  seen by the compiler. (The value isn't used, and the compiler will easily
  elide the dead store/read.)

Implementing this correctly, especially without the aide of `_Generic`, requires
quite a few tricks, but the results are correct (tested against GCC's
`__builtin_<op>_overflow` intrinsic, which is how `glibc` implements C23
[stdckdint][]).


# How to Use

The following function-like macros are defined:

- Regular: `mlib_add`, `mlib_sub`, and `mlib_mul`, with an additional
  `mlib_narrow` macro.
- Asserting: `mlib_assert_add`, `mlib_assert_sub`, `mlib_assert_mul`, and
  `mlib_assert_narrow`


## Regular Macros

The "regular" macros have the same API as [stdckdint][], with an additional
feature: If called with two arguments, the output parameter is used as the
left-hand operand of the operation:

```c
int foo = 42;
mlib_add(&foo, n);  // Equivalent to `foo += n`
```

`mlib_narrow(Dst, I)` is not from [stdckdint][], but is useful to check that an
integral cast operation does not modify the value:

```c
void foo(size_t N) {
  int a = 0;
  if (mlib_narrow(&a, N)) {
    fprintf(stderr, "Invalid operand: N is too large\n");
    abort();
  }
  // …
}
```

All of the "regular" macros return a boolean. If they return `true`, then the
result written to the destination DOES NOT represent the true arithmetic result
(i.e. the operation overflowed or narrowed). If it returns `false`, then the
operation succeeded without issue.

This allows one to chain arithmetic operations together with the logical-or
operator, short-circuiting when the operation fails:

```c
ssize_t grow_size(size_t sz, size_t elem_size, size_t count) {
  ssize_t ret = 0;
  // Compute: ret = sz + (elem_size × count)
  if (mlib_mul(&elem_size, count) ||   // elem_size *= count
      mlib_add(&ret, sz, elem_size)) { // ret = sz + elem_size
    // Overflow. Indicate an error.
    return SSIZE_MIN;
  }
  return ret;
}
```


## Asserting Macros

The `mlib_assert_…` macros take a type as their first argument instead of a
destination pointer. The macro yields the result of the operation as a value of
the specified type, asserting at runtime that no overflow or narrowing occurs.
If the operation results in information loss, the program terminates at the call
site.


# How it Works

This section details how it works, since it isn't straightforward from reading.


## Max-Precision Arithmetic

The basis of the checked arithmetic is to do the math in the maximum width
unsigned integer type, which is well-defined. We can then treat the unsigned bit
pattern as a signed or unsigned integer as appropriate to perform the arithmetic
correctly and check for overflow. This arithmetic is implemented in the
`mlib_add`, `mlib_sub`, and `mlib_mul` *functions* (not the macros). The bit
fiddling tricks are a combination of straightforward arithmetic checks and more
esoteric algorithms. The checks for addition and subtraction are fairly
straightforward, while the multiplication implementation is substantially more
complicated since its overflow semantics are much more pernicious.

The bit hacks are described within each function. They are split between each
combination of signed/unsigned treatment for the dest/left/right operands. The
basis of the bit checks is in treating the high bit as a special boolean: For
unsigned types, a set high bit represents a value outside the bounds of the
signed equivalent. For signed types, a set high bit indicates a negative value
that cannot be stored in an unsigned integer. Thus, logical-bit operations on
integers and then comparing the result as less-than-zero effectively treats the
high bit as a boolean, e.g.:

- For signed X and Y, `(X ^ Y) < 0` yield `true` iff `X` and `Y` have different
  sign.
- `(X & Y) < 0` tests that both X and Y are negative.
- `(X | Y) < 0` tests that either X or Y are negative.

The very terse bit-manipulation expressions are difficult to parse at first, and
have been expanded below each occurrence to explain what they are actually
testing. The terse bit-manip tests are left as the main condition for overflow
checking, as they generate significantly better machine code, even with the
optimizer enabled.

If the arithmetic overflows in the max precision integer, then we can assume
that it overflows for any smaller integer types.

For this integer promotion at macro sites, we use `mlib_upscale_integer`,
defined in `mlib/intutil.h`.


## Final Narrowing

While it is simple enough to perform arithmetic in the max precision, we need
to narrow the result to the target type, and that requires knowing the min/max
bounds of that type. This was the most difficult challenge, because it requires
the following:

1. Given a pointer to an integer type $T$, what is the minimum value of $T$?
2. ... what is the maximum value of $T$?
3. How do we cast from a `uintmax_t` to $T$ through a generic `void*`?

Point (3) is fairly simple: If we know the byte-size of $T$, we can bit-copy the
integer representation from `uintmax_t` into the `void*`, preserving
endian-encoding. For little-endian encoding, this is as simple as copying the first
$N$ bytes from the `uintmax_t` into the target, truncating to the target size.
For big-endian encoding, we just adjust a pointer into the object representation
of the `uintmax_t` to drop the high bytes that we don't need.

Points (1) and (2) are more subtle. We need a way to obtain a bit pattern that
respects the "min" and "max" two's complement values. While one can easily form
an aribtrary bit pattern using bit-shifts and `sizeof(*ptr)`, the trouble is
that the min/max values depend on whether the target is signed, and it is *not
possible* in C99 to ask whether an arbitrary integer expression is
signed/unsigned.


### Things that Don't Work™

Given a type `T`, we can check whether it is signed with a simple macro:

```c
#define IS_SIGNED(T) ((T)-1 < 0)
```

Unfortunately, we don't have a type `T`. We have an expression `V`:

```c
#define IS_SIGNED_TYPEOF(V) ???
```

With C23 or GNU's `__typeof__`, we could do this easily (see below).

There is one close call, that allows us to grab a zero and subtract one:

```c
#define IS_SIGNED_TYPEOF(V) ((0 & V) - 1 < 0)
```

This seems promising, but this **doesn't work**, because of C's awful,
horrible, no-good, very-bad integer promotion rules. The expression `0 & V`
*will* yield zero, but if `V` is smaller than `int`, it will be immediately
promoted to `signed int` beforehand, regardless of the sign of `V`. This macro
gives the correct answer for `(unsigned) int` and larger, but `(unsigned) short`
and `(unsigned) char` will always yield `true`.


### How `mlib/ckdint.h` Does It

There is one set of C operators that *don't* perform integer promotion:
assignment and in-place arithmetic. This macro *does* work:

```c
#define IS_SIGNED_TYPEOF(V) (((V) = -1) < 0)
```

But this obviously can't be used, because we're modifying the operand! Right...?

Except: We're only needing to do this check on the *destination* of the
arithmetic function. We already know that it's modifiable and that we're going
to reassign to it, so it doesn't matter that we temporarily write a garbage `-1`
into it!

With this, we can write our needed support macros:

```c
#define MINOF_TYPEOF(V) \
    IS_SIGNED_TYPEOF(V) \
        ? MIN_TYPEOF_SIGNED(V) \
        : MIN_TYPEOF_UNSIGNED(V)
```

With this, a call-site of our checked arithmetic macros can inject the
appropriate min/max values of the destination operand, and the checked
arithmetic functions can do the final bounds check.

Almost


### Big Problem, Though

Suppose the following:

```c
int a = 42;
mlib_add(&a, a, 5); // 42 + 5 ?
```

The correct result of `a` is `47`, but the value is unspecified: It is either
`4` or `47`, depending on argument evaluation order, because we are silently
overwriting the value in `a` to `-1` before doing the operation. We need to save
the value of `a`, do the check, and then restore the value of `a`, all in a
single go. Thus we have a much hairier macro:

```c
static thread_local uintmax_t P;
static thread_local bool S;
#define IS_SIGNED_TYPEOF(V) \
  (( \
    P = 0ull | (uintmax_t) V, \
    V = 0, \
    S = (--V < 0), \
    V = 0, \
    V |= P, \
    S \
  ))
```

This uses the comma-operator the enforce evaluation of each sub-expression:

1. Save the bit pattern of `V` in a global static temporary $P$.
2. Set `V` to zero.
3. Decrement `V` and check if the result is negative. Save that value in a
   separate global $S$.
4. Restore the value of `V` by writing the bit pattern stored in $P$ back into
   `V`. (The use of `= 0` + `|= P` prevents compilers from emitting any
   integer conversion warnings)
5. Yield the bool we saved in $S$.

$P$ and $S$ are `thread_local` to allow multiple threads to evaluate the macro
simultaneously without interfering. The `static` allows the optimizer to delete
$P$ and $S$ from the translation unit when it can statically determine that the
values written into these variables are never read from after constant folding
(usually: MSVC is currently unable to elide the writes, but is still able to
constant-fold across these assignments, which is the most important optimization
we need to ensure works to eliminate redundant branches after inlining).

With this modified roundabout definition, we can perform in-place checked
arithmetic where the output can also be used as an input of the operation.


### Optimize: Use `__typeof__`

If we have `__typeof__` (available in GCC, Clang, and MSVC 19.39+) or C23
`typeof` , we can simplify our macro to a trivial one:

```c
#define IS_SIGNED_TYPEOF(V) IS_SIGNED(__typeof__(V))
```

This will yield an equivalent result, but improves debug codegen and gives the
optimizer an easier time doing constant folding across function calls.
