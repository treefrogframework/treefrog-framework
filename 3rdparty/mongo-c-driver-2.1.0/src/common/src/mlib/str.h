/**
 * @file mlib/str.h
 * @brief String handling utilities
 * @date 2025-04-30
 *
 * This file provides utilities for handling *sized* strings. That is, strings
 * that carry their size, and do not rely on null termination. These APIs also
 * do a lot more bounds checking than is found in `<string.h>`.
 *
 * @copyright Copyright 2009-present MongoDB, Inc.
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
#ifndef MLIB_STR_H_INCLUDED
#define MLIB_STR_H_INCLUDED

#include <mlib/ckdint.h>
#include <mlib/cmp.h>
#include <mlib/config.h>
#include <mlib/intutil.h>
#include <mlib/loop.h>
#include <mlib/test.h>

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief A simple non-owning string-view type.
 *
 * The viewed string can be treated as an array of `char`. It's pointed-to data
 * must not be freed or manipulated.
 *
 * @note The viewed string is NOT guaranteed to be null-terminated. It WILL
 * be null-terminated if: Directly created from a string literal, a C string, or
 * a null-terminated `mstr_view`.
 * @note The viewed string MAY contain nul (zero-value) characters, so using them
 * with C string APIs could truncate unexpectedly.
 * @note The view itself may be "null" if the `data` member of the string view
 * is a null pointer. A zero-initialized `mstr_view` is null.
 */
typedef struct mstr_view {
   /**
    * @brief Pointer to the string data viewed by this object.
    *
    * - This pointer may be null, in which case the string view itself is "null".
    * - If `len > 1`, then this points to a contiguous array of `char` of length
    *   `len`.
    * - If `len == 1`, then this *may* point to a single `char` object.
    * - The pointed-to string might not be a null-terminated C string. Accessing
    *   the `char` value at `data[len]` is undefined behavior.
    */
   const char *data;
   /**
    * @brief The length of the viewed string pointed-to by `data`
    *
    * If `data` points to a single `char` object, then this must be `1`. If
    * `data` is a null pointer, then this value should be zero.
    */
   size_t len;
} mstr_view;

/**
 * @brief Expand to the two printf format arguments required to format an mstr object
 *
 * You should use the format specifier `%.*s' for all mstr strings.
 *
 * This is just a convenience shorthand.
 */
#define MSTR_FMT(S) (int) mstr_view_from (S).len, mstr_view_from (S).data

/**
 * @brief Create an `mstr_view` that views the given array of `char`
 *
 * @param data Pointer to the beginning of the string, or pointer to a single
 * `char`, or a null pointer
 * @param len Length of the new string-view. If `data` points to a single `char`,
 * this must be `0` or `1`. If `data` is a null pointer, this should be `0`.
 *
 * @note This is defined as a macro that expands to a compound literal to prevent
 * proliferation of redundant function calls in debug builds.
 */
#define mstr_view_data(DataPointer, Length) (mlib_init (mstr_view){(DataPointer), (Length)})

#if 1 // See "!! NOTE" below

/**
 * @brief Coerce a string-like object to an `mstr_view` of that string
 *
 * This macro requires that the object have `.data` and `.len` members.
 *
 * @note This macro will double-evaluate its argument.
 */
#define mstr_view_from(X) mstr_view_data ((X).data, (X).len)

/**
 * ! NOTE: The disabled snippet below is kept for posterity as a drop-in replacment
 * ! for mstr_view_from with support for _Generic.
 *
 * When we can increase the compiler requirements to support _Generic, the following
 * macro definition alone makes almost every function in this file significantly
 * more concise to use, as it allows us to pass a C string to any API that
 * expects an `mstr_view`, enabling code like this:
 *
 * ```
 * mstr s = get_string();
 * if (mstr_cmp(s, ==, "magicKeyword")) {
 *    Do something...
 * }
 * ```
 *
 * This also allows us to avoid the double-evaluation problem presented by
 * `mstr_view_from` being defined as above.
 *
 * Without _Generic, we require all C strings to be wrapped with `mstr_cstring`,
 * which isn't especially onerous, but it is annoying. Additionally, the below
 * `_Generic` macro can be extended to support more complex string-like types.
 *
 * For reference, support for _Generic requires the following compilers:
 *
 * - MSVC 19.28.0+ (VS 2019, 16.8.1)
 * - GCC 4.9+
 * - Clang 3.0+
 */

#else

/**
 * @brief Coerce an object to an `mstr_view`
 *
 * The object requires a `data` and `len` member
 */
#define mstr_view_from(X) \
   _Generic ((X), mstr_view: _mstr_view_trivial_copy, char *: mstr_cstring, const char *: mstr_cstring) ((X))
// Just copy an mstr_view by-value
static inline mstr_view
_mstr_view_trivial_copy (mstr_view s)
{
   return s;
}

#endif


/**
 * @brief Create an `mstr_view` referring to the given null-terminated C string
 *
 * @param s Pointer to a C string. The length of the returned string is infered using `strlen`
 *
 * This should not defined as a macro, because defining it as a macro would require
 * double-evaluating for the call to `strlen`.
 */
static inline mstr_view
mstr_cstring (const char *s)
{
   const size_t l = strlen (s);
   return mstr_view_data (s, l);
}

/**
 * @brief Compare two strings lexicographically by each code unit
 *
 * If called with two arguments behaves the same as `strcmp`. If called with
 * three arguments, the center argument should be an infix operator to perform
 * the semantic comparison.
 */
static inline enum mlib_cmp_result
mstr_cmp (mstr_view a, mstr_view b)
{
   size_t l = a.len;
   if (b.len < l) {
      l = b.len;
   }
   // Use `memcmp`, not `strncmp`: We want to respect nul characters
   int r = memcmp (a.data, b.data, l);
   if (r) {
      // Not equal: Compare with zero to normalize to the cmp_result value
      return mlib_cmp (r, 0);
   }
   // Same prefixes, the ordering is now based on their length (longer string > shorter string)
   return mlib_cmp (a.len, b.len);
}

#define mstr_cmp(...) MLIB_ARGC_PICK (_mstr_cmp, __VA_ARGS__)
#define _mstr_cmp_argc_2(A, B) mstr_cmp (mstr_view_from (A), mstr_view_from (B))
#define _mstr_cmp_argc_3(A, Op, B) (_mstr_cmp_argc_2 (A, B) Op 0)

/**
 * @brief If the given codepoint is a Basic Latin (ASCII) uppercase character,
 * return the lowercase character. Other codepoint values are returned unchanged.
 *
 * This is safer than `tolower`, because it doesn't respect locale and has no
 * undefined behavior.
 */
static inline int32_t
mlib_latin_tolower (int32_t a)
{
   if (a >= 0x41 /* "A" */ && a <= 0x5a /* "Z" */) {
      a += 0x20; // Adjust from "A" -> "a"
   }
   return a;
}

/**
 * @brief Compare two individual codepoint values, with case-insensitivity in
 * the Basic Latin range.
 */
static inline enum mlib_cmp_result
mlib_latin_charcasecmp (int32_t a, int32_t b)
{
   return mlib_cmp (mlib_latin_tolower (a), mlib_latin_tolower (b));
}

/**
 * @brief Compare two strings lexicographically, case-insensitive in the Basic
 * Latin range.
 *
 * If called with two arguments, behaves the same as `strcasecmp`. If called with
 * three arguments, the center argument should be an infix operator to perform
 * the semantic comparison.
 */
static inline enum mlib_cmp_result
mstr_latin_casecmp (mstr_view a, mstr_view b)
{
   size_t l = a.len;
   if (b.len < l) {
      l = b.len;
   }
   mlib_foreach_urange (i, l) {
      // We don't need to do any UTF-8 decoding, because our case insensitivity
      // only activates for 1-byte encoded codepoints, and all other valid UTF-8
      // sequences will collate equivalently with byte-wise comparison to a UTF-32
      // encoding.
      enum mlib_cmp_result r = mlib_latin_charcasecmp (a.data[i], b.data[i]);
      if (r) {
         // Not equivalent at this code unit. Return this as the overall string ordering.
         return r;
      }
   }
   // Same prefixes, the ordering is now based on their length (longer string > shorter string)
   return mlib_cmp (a.len, b.len);
}

#define mstr_latin_casecmp(...) MLIB_ARGC_PICK (_mstr_latin_casecmp, __VA_ARGS__)
#define _mstr_latin_casecmp_argc_2(A, B) mstr_latin_casecmp (mstr_view_from (A), mstr_view_from (B))
#define _mstr_latin_casecmp_argc_3(A, Op, B) (_mstr_latin_casecmp_argc_2 (A, B) Op 0)

/**
 * @brief Adjust a possibly negative index position to wrap around for a string
 *
 * @param s The string to be respected for index wrapping
 * @param pos The maybe-negative index to be adjusted
 * @param clamp_to_length If `true` and given a non-negative value, if that
 * value is greater than the string length, this function will return the string
 * length instead.
 * @return size_t The new zero-based non-negative index
 *
 * If `pos` is negative, then it represents indexing from the end of the string,
 * where `-1` refers to the last character in the string, `-2` the penultimate,
 * etc. If the absolute value is greater than the length of the string, the
 * program will be terminated.
 */
static inline size_t
_mstr_adjust_index (mstr_view s, mlib_upsized_integer pos, bool clamp_to_length)
{
   if (clamp_to_length && (mlib_cmp) (pos, mlib_upsize_integer (s.len), 0) == mlib_greater) {
      // We want to clamp to the length, and the given value is greater than the string length.
      return s.len;
   }
   if (pos.is_signed && pos.bits.as_signed < 0) {
      // This will add the negative value to the length of the string. If such
      // an operation would result a negative value, this will terminate the
      // program.
      return mlib_assert_add (size_t, s.len, pos.bits.as_signed);
   }
   // No special behavior, just assert that the given position is in-bounds for the string
   mlib_check (
      pos.bits.as_unsigned <= s.len, because, "the string position index must not be larger than the string length");
   return pos.bits.as_unsigned;
}

/**
 * @brief Create a new `mstr_view` that views a substring within another string
 *
 * @param s The original string view to be inspected
 * @param pos The number of `char` to skip in `s`, or a negative value to
 * pos from the end of the string.
 * @param len The length of the new string view (optional, default SIZE_MAX)
 *
 * The length of the string view is clamped to the characters available in `s`,
 * so passing a too-large value for `len` is well-defined. Passing a too-large
 * value for `pos` will abort the program.
 *
 * Callable as:
 *
 * - `mstr_substr(s, pos)`
 * - `mstr_substr(s, pos, len)`
 */
static inline mstr_view
mstr_substr (mstr_view s, mlib_upsized_integer pos_, size_t len)
{
   const size_t pos = _mstr_adjust_index (s, pos_, false);
   // Number of characters in the string after we remove the prefix
   const size_t remain = s.len - pos;
   // Clamp the new length to the size that is actually available.
   if (len > remain) {
      len = remain;
   }
   return mstr_view_data (s.data + pos, len);
}

#define mstr_substr(...) MLIB_ARGC_PICK (_mstr_substr, __VA_ARGS__)
#define _mstr_substr_argc_2(Str, Start) _mstr_substr_argc_3 (Str, Start, SIZE_MAX)
#define _mstr_substr_argc_3(Str, Start, Stop) mstr_substr (mstr_view_from (Str), mlib_upsize_integer (Start), Stop)

/**
 * @brief Obtain a slice of the given string view, where the two arguments are zero-based indices into the string
 *
 * @param s The string to be sliced
 * @param start The zero-based index of the new string start
 * @param end The zero-based index of the first character to exclude from the new string
 *
 * @note Unlike `substr`, the second argument is required, and must specify the index at which the
 * string will end, rather than the length of the string.
 */
static inline mstr_view
mstr_slice (const mstr_view s, const mlib_upsized_integer start_, const mlib_upsized_integer end_)
{
   const size_t start_pos = _mstr_adjust_index (s, start_, false);
   const size_t end_pos = _mstr_adjust_index (s, end_, true);
   mlib_check (end_pos >= start_pos, because, "Slice positions must end after the start position");
   const size_t sz = (size_t) (end_pos - start_pos);
   return mstr_substr (s, start_pos, sz);
}
#define mstr_slice(S, StartPos, EndPos) \
   mstr_slice (mstr_view_from (S), mlib_upsize_integer ((StartPos)), mlib_upsize_integer ((EndPos)))

/**
 * @brief Find the first occurrence of `needle` within `hay`, returning the zero-based index
 * if found, and `SIZE_MAX` if it is not found.
 *
 * @param hay The string which is being scanned
 * @param needle The substring that we are searching to find
 * @param pos The start position of the search (optional, default zero)
 * @param len The number of characters to search in `hay` (optional, default SIZE_MAX)
 * @return size_t If found, the zero-based index of the first occurrence within
 *    the string. If not found, returns `SIZE_MAX`.
 *
 * The `len` is clamped to the available string length.
 *
 * Callable as:
 *
 * - `mstr_find(hay, needle)`
 * - `mstr_find(hay, needle, pos)`
 * - `mstr_find(hay, needle, pos, len)`
 */
static inline size_t
mstr_find (mstr_view hay, mstr_view const needle, mlib_upsized_integer const pos_, size_t const len)
{
   const size_t pos = _mstr_adjust_index (hay, pos_, false);
   // Trim the hay according to our search window:
   hay = mstr_substr (hay, pos, len);

   // Larger needle can never exist within the smaller string:
   if (hay.len < needle.len) {
      return SIZE_MAX;
   }

   // Set the index at which we can stop searching early. This will never
   // overflow, because we guard against hay.len > needle.len
   size_t stop_idx = hay.len - needle.len;
   // Use "<=", because we do want to include the final search position
   for (size_t offset = 0; offset <= stop_idx; ++offset) {
      if (memcmp (hay.data + offset, needle.data, needle.len) == 0) {
         // Return the found position. Adjust by the start pos since we may
         // have trimmed the search window
         return offset + pos;
      }
   }

   // Nothing was found. Return SIZE_MAX to indicate the not-found
   return SIZE_MAX;
}

#define mstr_find(...) MLIB_ARGC_PICK (_mstr_find, __VA_ARGS__)
#define _mstr_find_argc_2(Hay, Needle) _mstr_find_argc_3 (Hay, Needle, 0)
#define _mstr_find_argc_3(Hay, Needle, Start) _mstr_find_argc_4 (Hay, Needle, Start, SIZE_MAX)
#define _mstr_find_argc_4(Hay, Needle, Start, Stop) \
   mstr_find (mstr_view_from (Hay), mstr_view_from (Needle), mlib_upsize_integer (Start), Stop)

/**
 * @brief Find the zero-based index of the first `char` in `hay` that also occurs in `needles`
 *
 * This is different from `find()` because it considers each char in `needles` as an individual
 * one-character string to be search for in `hay`.
 *
 * @param hay The string to be searched
 * @param needles A string containing a set of characters which are searched for in `hay`
 * @param pos The index at which to begin searching (optional, default is zero)
 * @param len The number of characters in `hay` to consider before stopping (optional, default is SIZE_MAX)
 * @return size_t If a needle is found, returns the zero-based index of that first needle.
 * Otherwise, returns SIZE_MAX.
 *
 * Callable as:
 *
 * - `mstr_find_first_of(hay, needles)`
 * - `mstr_find_first_of(hay, needles, pos)`
 * - `mstr_find_first_of(hay, needles, pos, len)`
 */
static inline size_t
mstr_find_first_of (mstr_view hay, mstr_view const needles, mlib_upsized_integer const pos_, size_t const len)
{
   const size_t pos = _mstr_adjust_index (hay, pos_, false);
   // Trim to fit the search window
   hay = mstr_substr (hay, pos, len);
   // We search by incrementing an index
   mlib_foreach_urange (idx, hay.len) {
      // Grab a substring of the single char at the current search index
      mstr_view one = mstr_substr (hay, idx, 1);
      // Test if the single char occurs anywhere in the needle set
      if (mstr_find (needles, one) != SIZE_MAX) {
         // We found the first index in `hay` where one of the needles occurs. Adjust
         // by `pos` since we may have trimmed
         return idx + pos;
      }
   }
   return SIZE_MAX;
}

#define mstr_find_first_of(...) MLIB_ARGC_PICK (_mstr_find_first_of, __VA_ARGS__)
#define _mstr_find_first_of_argc_2(Hay, Needle) _mstr_find_first_of_argc_3 (Hay, Needle, 0)
#define _mstr_find_first_of_argc_3(Hay, Needle, Pos) _mstr_find_first_of_argc_4 (Hay, Needle, Pos, SIZE_MAX)
#define _mstr_find_first_of_argc_4(Hay, Needle, Pos, Len) \
   mstr_find_first_of (Hay, Needle, mlib_upsize_integer (Pos), Len)

/**
 * @brief Split a single string view into two strings at the given position
 *
 * @param s The string to be split
 * @param pos The position at which the prefix string is ended
 * @param drop [optional] The number of characters to drop between the prefix and suffix
 * @param prefix [out] Updated to point to the part of the string before the split
 * @param suffix [out] Updated to point to the part of the string after the split
 *
 * `pos` and `drop` are clamped to the size of the input string.
 *
 * Callable as:
 *
 * - `mstr_split_at(s, pos,       prefix, suffix)`
 * - `mstr_split_at(s, pos, drop, prefix, suffix)`
 *
 * If either `prefix` or `suffix` is a null pointer, then they will be ignored
 */
static inline void
mstr_split_at (mstr_view s, mlib_upsized_integer pos_, size_t drop, mstr_view *prefix, mstr_view *suffix)
{
   const size_t pos = _mstr_adjust_index (s, pos_, true /* clamp to the string size */);
   // Save the prefix string
   if (prefix) {
      *prefix = mstr_substr (s, 0, pos);
   }
   // Save the suffix string
   if (suffix) {
      // The number of characters that remain after the prefix is removed
      const size_t remain = s.len - pos;
      // Clamp the number of chars to drop to not overrun the input string
      if (remain < drop) {
         drop = remain;
      }
      // The start position of the new string
      const size_t next_start = pos + drop;
      *suffix = mstr_substr (s, next_start, SIZE_MAX);
   }
}

#define mstr_split_at(...) MLIB_ARGC_PICK (_mstr_split_at, __VA_ARGS__)
#define _mstr_split_at_argc_4(Str, Pos, Prefix, Suffix) _mstr_split_at_argc_5 (Str, Pos, 0, Prefix, Suffix)
#define _mstr_split_at_argc_5(Str, Pos, Drop, Prefix, Suffix) \
   mstr_split_at (mstr_view_from (Str), mlib_upsize_integer (Pos), Drop, Prefix, Suffix)

/**
 * @brief Split a string in two around the first occurrence of some infix string.
 *
 * @param s The string to be split in twain
 * @param infix The infix string to be searched for
 * @param prefix The part of the string that precedes the infix (nullable)
 * @param suffix The part of the string that follows the infix (nullable)
 * @return true If the infix was found
 * @return false Otherwise
 *
 * @note If `infix` does not occur in `s`, then `*prefix` will be set equal to `s`,
 * and `*suffix` will be made an empty string, as if the infix occurred at the end
 * of the string.
 */
static inline bool
mstr_split_around (mstr_view s, mstr_view infix, mstr_view *prefix, mstr_view *suffix)
{
   // Find the position of the infix. If it is not found, returns SIZE_MAX
   const size_t pos = mstr_find (s, infix);
   // Split at the infix, dropping as many characters as are in the infix. If
   // the `pos` is SIZE_MAX, then this call will clamp to the end of the string.
   mstr_split_at (s, pos, infix.len, prefix, suffix);
   // Return `true` if we found the infix, indicated by a not-SIZE_MAX `pos`
   return pos != SIZE_MAX;
}

#define mstr_split_around(Str, Infix, PrefixPtr, SuffixPtr) \
   mstr_split_around (mstr_view_from ((Str)), mstr_view_from ((Infix)), (PrefixPtr), (SuffixPtr))

/**
 * @brief Test whether the given string starts with the given prefix
 *
 * @param str The string to be tested
 * @param prefix The prefix to be searched for
 * @return true if-and-only-if `str` starts with `prefix`
 * @return false Otherwise
 */
static inline bool
mstr_starts_with (mstr_view str, mstr_view prefix)
{
   // Trim to match the length of the prefix we want
   str = mstr_substr (str, 0, prefix.len);
   // Check if the trimmed string is the same as the prefix
   return mstr_cmp (str, ==, prefix);
}
#define mstr_starts_with(Str, Prefix) mstr_starts_with (mstr_view_from (Str), mstr_view_from (Prefix))

/**
 * @brief Test whether a substring occurs at any point within the given string
 *
 * @param str The string to be inspected
 * @param needle The substring to be searched for
 * @return true If-and-only-if `str` contains `needle` at any position
 * @return false Otherise
 */
static inline bool
mstr_contains (mstr_view str, mstr_view needle)
{
   return mstr_find (str, needle) != SIZE_MAX;
}
#define mstr_contains(Str, Needle) mstr_contains (mstr_view_from (Str), mstr_view_from (Needle))

/**
 * @brief Test whether a given string contains any of the characters in some other string
 *
 * @param str The string to be inspected
 * @param needle A string to be treated as a set of one-byte characters to search for
 * @return true If-and-only-if `str` contains `needle` at any position
 * @return false Otherise
 *
 * @note This function does not currently support multi-byte codepoints
 */
static inline bool
mstr_contains_any_of (mstr_view str, mstr_view needle)
{
   return mstr_find_first_of (str, needle) != SIZE_MAX;
}
#define mstr_contains_any_of(Str, Needle) mstr_contains_any_of (mstr_view_from (Str), mstr_view_from (Needle))

#endif // MLIB_STR_H_INCLUDED
