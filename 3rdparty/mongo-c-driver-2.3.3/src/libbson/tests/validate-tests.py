"""
This script generates a C source file containing test cases for BSON validation
and iteration.

Run this script with Python 3.12+, and pipe the output into a file.

This script takes no command-line arguments.
"""

# /// script
# requires-python = ">=3.12"
# dependencies = []
# ///

import argparse
import enum
import json
import re
import struct
import textwrap
from dataclasses import dataclass
from typing import Iterable


class Tag(enum.Enum):
    """BSON type tag byte values"""

    EOD = 0
    Double = 1
    UTF8 = 2
    Document = 3
    Array = 4
    Binary = 5
    Undefined = 6
    OID = 7
    Boolean = 8
    Datetime = 9
    Null = 10
    Regex = 11
    DBPointer = 12
    Code = 13
    Symbol = 14
    CodeWithScope = 15
    Int32 = 16
    Timestamp = 17
    Int64 = 18
    Decimal128 = 19
    MinKey = 0xFF
    MaxKey = 0x7F


type _ByteIter = bytes | Iterable[_ByteIter]
"""A set of bytes, or an iterable that yields more sets of bytes"""


def flatten_bytes(data: _ByteIter) -> bytes:
    """Flatten a (recursive) iterator of bytes into a single bytes object"""
    match data:
        case bytes(data):
            return data
        case it:
            return b''.join(map(flatten_bytes, it))


def i32le(i: int) -> bytes:
    """Encode an integer as a 32-bit little-endian integer"""
    return struct.pack('<i', i)


def i64le(i: int) -> bytes:
    """Encode an integer as a 64-bit little-endian integer"""
    return struct.pack('<q', i)


def f64le(f: float) -> bytes:
    """Encode a float as a 64-bit little-endian float"""
    return struct.pack('<d', f)


def doc(*data: _ByteIter) -> bytes:
    """Add a BSON document header a null terminator to a set of bytes"""
    flat = flatten_bytes(data)
    # +5 for the null terminator and the header bytes
    hdr = i32le(len(flat) + 5)
    return hdr + flat + b'\0'


def code_with_scope(code: str, doc: _ByteIter) -> bytes:
    """Create a BSON code-with-scope object with appropriate header"""
    s = string(code)
    doc = flatten_bytes(doc)
    # +4 to include the length prefix too
    len_prefix = i32le(len(s) + len(doc) + 4)
    return len_prefix + s + doc


def elem(key: str | _ByteIter, tag: int | Tag, *bs: _ByteIter) -> bytes:
    """Add a BSON element header to a set of bytes"""
    if isinstance(tag, Tag):
        tag = tag.value
    return bytes([tag]) + cstring(key) + flatten_bytes(bs)


def binary(subtype: int, *bs: _ByteIter) -> bytes:
    """
    Create a BSON binary object with appropriate header and subtype tag byte.
    """
    flat = flatten_bytes(bs)
    st = bytes([subtype])
    return i32le(len(flat)) + st + flat


def cstring(s: str | _ByteIter) -> bytes:
    """Encode a string as UTF-8 and add a null terminator"""
    match s:
        case str(s):
            return cstring(s.encode('utf-8'))
        case bs:
            bs = flatten_bytes(bs)
            return bs + b'\0'


def string(s: str | _ByteIter) -> bytes:
    """Add a length header and null terminator to a UTF-8 string"""
    cs = cstring(s)
    # Length header includes the null terminator
    hdr = i32le(len(cs))
    return hdr + cs


def utf8elem(key: str | _ByteIter, s: str | _ByteIter) -> bytes:
    """Create a valid UTF-8 BSON element for the given string"""
    return elem(key, Tag.UTF8, string(s))


@dataclass(frozen=True)
class ErrorInfo:
    """
    Information about an expected validation error
    """

    code: str
    """Spellling of the error code to be expected"""
    message: str
    """The expected error message"""
    offset: int
    """The expected error offset"""


@dataclass(frozen=True)
class TestCase:
    """
    Defines a single validation test case.
    """

    name: str
    """The name of the test case, as displayed in test runners, which will have a "/bson/validate" prefix"""
    data: bytes
    """The bytes that will be injested by `bson_init_static` to form the document to be validated"""
    description: str | None
    """A plaintext description of the test case and what it actually does. Rendered as a comment."""
    flags: str = '0'
    """Spelling of the flags argument passed to the validation API"""
    error: ErrorInfo = ErrorInfo('0', '', 0)
    """Expected error, if any"""

    @property
    def fn_name(self) -> str:
        """Get a C identifier function name for this test case"""
        return '_test_case_' + re.sub(r'[^\w]', '_', self.name).lower()


def fmt_byte(n: int) -> str:
    """
    Format an octet value for C code. Will emit a char literal if certain ASCII,
    otherwise an integer literal.
    """
    match n:
        case 0:
            return '0'
        case a if re.match(r'[a-zA-Z0-9.$-]', chr(a)):
            return f"'{chr(a)}'"
        case a if a < 10:
            return str(a)
        case n:
            return f'0x{n:0>2x}'


GENERATED_NOTE = '// ! This code is GENERATED! Do not edit it directly!'

HEADER = rf"""{GENERATED_NOTE}
// clang-format off

#include <bson/bson.h>

#include <mlib/test.h>

#include <TestSuite.h>
"""


def generate(case: TestCase) -> Iterable[str]:
    """
    Generate the lines of a test case function.
    """
    # A comment header
    yield f'{GENERATED_NOTE}\n'
    yield f'// Case: {case.name}\n'
    # The function head
    yield f'static inline void {case.fn_name}(void) {{\n'
    # If we have a description, emit that in a block comment
    if case.description:
        yield '  /**\n'
        lines = textwrap.dedent(case.description).strip().splitlines()
        yield from (f'   * {ln}\n' for ln in lines)
        yield '   */\n'
    # Emit the byte array literal
    yield '  const uint8_t bytes[] = {\n'
    yield '\n'.join(
        textwrap.wrap(
            ', '.join(map(fmt_byte, case.data)),
            subsequent_indent=' ' * 4,
            initial_indent=' ' * 4,
            width=80,
        )
    )
    yield '\n  };\n'
    yield from [
        # Initialize a BSON doc that points to the byte array
        '  bson_t doc;\n',
        '  mlib_check(bson_init_static(&doc, bytes, sizeof bytes));\n',
        # The error object to be filled
        '  bson_error_t error = {0};\n',
        # The error offset. Expected to be reset to zero on success.
        '  size_t offset = 999999;\n'
        # Do the actual validation:
        f'  const bool is_valid = bson_validate_with_error_and_offset(&doc, {case.flags}, &offset, &error);\n',
    ]
    is_error = case.error.code != '0'
    yield from [
        '  mlib_check(!is_valid);\n' if is_error else '  ASSERT_OR_PRINT(is_valid, error);\n',
        f'  mlib_check(error.code, eq, {case.error.code});\n',
        f'  mlib_check(error.message, str_eq, {json.dumps(case.error.message)});\n',
        f'  mlib_check(offset, eq, {case.error.offset});\n' if is_error else '',
    ]
    yield '}\n'


def corruption_at(off: int) -> ErrorInfo:
    """
    Generate an ErrorInfo to expect a message of "corrupt BSON" at the given
    byte offset.

    Note that this won't match if the error message is something other
    than "corrupt BSON".
    """
    return ErrorInfo(BSON_VALIDATE_CORRUPT, 'corrupt BSON', off)


BSON_VALIDATE_CORRUPT = 'BSON_VALIDATE_CORRUPT'
BSON_VALIDATE_DOLLAR_KEYS = 'BSON_VALIDATE_DOLLAR_KEYS'
BSON_VALIDATE_DOT_KEYS = 'BSON_VALIDATE_DOT_KEYS'
BSON_VALIDATE_EMPTY_KEYS = 'BSON_VALIDATE_EMPTY_KEYS'
BSON_VALIDATE_UTF8 = 'BSON_VALIDATE_UTF8'
BSON_VALIDATE_UTF8_ALLOW_NULL = 'BSON_VALIDATE_UTF8_ALLOW_NULL'
MSG_EXPECTED_ID_FOLLOWING_REF = 'Expected an $id element following $ref'


def disallowed_key(char: str, k: str) -> str:
    return f'Disallowed \'{char}\' in element key: "{k}"'


# d888888b d88888b .d8888. d888888b       .o88b.  .d8b.  .d8888. d88888b .d8888.
# `~~88~~' 88'     88'  YP `~~88~~'      d8P  Y8 d8' `8b 88'  YP 88'     88'  YP
#    88    88ooooo `8bo.      88         8P      88ooo88 `8bo.   88ooooo `8bo.
#    88    88~~~~~   `Y8b.    88         8b      88~~~88   `Y8b. 88~~~~~   `Y8b.
#    88    88.     db   8D    88         Y8b  d8 88   88 db   8D 88.     db   8D
#    YP    Y88888P `8888Y'    YP          `Y88P' YP   YP `8888Y' Y88888P `8888Y'

CASES: list[TestCase] = [
    TestCase(
        'empty',
        doc(),
        """Test a simple empty document object.""",
    ),
    TestCase(
        'bad-element',
        doc(b'f'),
        'The element content is not valid',
        error=corruption_at(6),
    ),
    TestCase(
        'invalid-type',
        doc(elem('foo', 0xE, b'foo')),
        """The type tag "0x0e" is not a valid type""",
        error=corruption_at(9),
    ),
    TestCase(
        'key/invalid/accept',
        doc(
            utf8elem('a', 'b'),
            utf8elem(b'foo\xffbar', 'baz'),
            utf8elem('c', 'd'),
        ),
        """
        The element key contains an invalid UTF-8 byte, but we accept it
        because we aren't doing UTF-8 validation.
        """,
    ),
    TestCase(
        'key/invalid/reject',
        doc(
            utf8elem('a', 'b'),
            elem(b'foo\xffbar', Tag.UTF8, string('baz')),
            utf8elem('c', 'd'),
        ),
        """
        The element key is not valid UTF-8 and we reject it when we do UTF-8
        validation.
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8, 'Text element is not valid UTF-8', 13),
    ),
    TestCase(
        'key/empty/accept',
        doc(utf8elem('', 'string')),
        """
        The element has an empty string key, and we accept this.
        """,
    ),
    TestCase(
        'key/empty/reject',
        doc(
            utf8elem('a', 'b'),
            utf8elem('', 'string'),
        ),
        """
        The element has an empty key, and we can reject it.
        """,
        flags=BSON_VALIDATE_EMPTY_KEYS,
        error=ErrorInfo(BSON_VALIDATE_EMPTY_KEYS, 'Element key cannot be an empty string', 13),
    ),
    TestCase(
        'key/empty/accept-if-absent',
        doc(utf8elem('foo', 'bar')),
        """
        We are checking for empty keys, and accept if they are absent.
        """,
        flags=BSON_VALIDATE_EMPTY_KEYS,
    ),
    TestCase(
        'key/dot/accept',
        doc(utf8elem('foo.bar', 'baz')),
        """
        The element key has an ASCII dot, and we accept this since we don't
        ask to validate it.
        """,
        flags=BSON_VALIDATE_EMPTY_KEYS,
    ),
    TestCase(
        'key/dot/reject',
        doc(utf8elem('a', 'b'), utf8elem('foo.bar', 'baz')),
        """
        The element has an ASCII dot, and we reject it when we ask to validate
        it.
        """,
        flags=BSON_VALIDATE_DOT_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOT_KEYS, disallowed_key('.', 'foo.bar'), 13),
    ),
    TestCase(
        'key/dot/accept-if-absent',
        doc(utf8elem('foo', 'bar')),
        """
        We are checking for keys with dot '.', and accept if they are absent.
        """,
        flags=BSON_VALIDATE_DOT_KEYS,
    ),
    TestCase(
        'key/dollar/accept',
        doc(utf8elem('a', 'b'), utf8elem('$foo', 'bar')),
        """
        We can accept an element key that starts with a dollar '$' sign.
        """,
    ),
    TestCase(
        'key/dollar/reject',
        doc(utf8elem('a', 'b'), utf8elem('$foo', 'bar')),
        """
        We can reject an element key that starts with a dollar '$' sign.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, disallowed_key('$', '$foo'), 13),
    ),
    TestCase(
        'key/dollar/accept-in-middle',
        doc(utf8elem('foo$bar', 'baz')),
        """
        This contains a element key "foo$bar", but we don't reject this, as we
        only care about keys that *start* with dollars.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
    ),
    TestCase(
        'key/dollar/accept-if-absent',
        doc(utf8elem('foo', 'bar')),
        """
        We are validating for dollar-keys, and we accept because this document
        doesn't contain any such keys.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
    ),
    TestCase(
        'utf8/simple',
        doc(utf8elem('string', 'some string')),
        'Simple UTF-8 string element',
    ),
    TestCase(
        'utf8/missing-null',
        doc(elem('a', Tag.UTF8, i32le(4), b'abcd')),
        """
        The UTF-8 element "a" contains 4 characters and declares its length of 4,
        but the fourth character is supposed to be a null terminator. In this case,
        it is the letter 'd'.
        """,
        error=corruption_at(14),
    ),
    TestCase(
        'utf8/length-zero',
        doc(elem('', Tag.UTF8, i32le(0), b'\0')),
        'UTF-8 string length must always be at least 1 for the null terminator',
        error=corruption_at(6),
    ),
    TestCase(
        'utf8/length-too-short',
        doc(elem('', Tag.UTF8, i32le(3), b'bar\0')),
        'UTF-8 string is three chars and a null terminator, but the declared length is 3 (should be 4)',
        error=corruption_at(12),
    ),
    TestCase(
        'utf8/header-too-large',
        doc(elem('foo', Tag.UTF8, b'\xff\xff\xff\xffbar\0')),
        """
        Data { "foo": "bar" } but the declared length of "bar" is way too large.
        """,
        error=corruption_at(9),
    ),
    TestCase(
        'utf8/valid',
        doc(elem('foo', Tag.UTF8, string('abcd'))),
        """
        Validate a valid UTF-8 string with UTF-8 validation enabled.
        """,
        flags=BSON_VALIDATE_UTF8,
    ),
    TestCase(
        'utf8/invalid/accept',
        doc(utf8elem('foo', b'abc\xffd')),
        """
        Validate an invalid UTF-8 string, but accept invalid UTF-8.
        """,
    ),
    TestCase(
        'utf8/invalid/reject',
        doc(utf8elem('foo', b'abc\xffd')),
        """
        Validate an invalid UTF-8 string, and expect rejection.
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8, 'Text element is not valid UTF-8', 4),
    ),
    TestCase(
        'utf8/valid-with-null/accept-1',
        doc(utf8elem('foo', b'abc\x00123')),
        """
        This is a valid UTF-8 string that contains a null character. We accept
        it because we don't do UTF-8 validation.
        """,
    ),
    TestCase(
        'utf8/valid-with-null/accept-2',
        doc(utf8elem('foo', b'abc\x00123')),
        """
        This is a valid UTF-8 string that contains a null character. We allow
        it explicitly when we request UTF-8 validation.
        """,
        flags=f'{BSON_VALIDATE_UTF8} | {BSON_VALIDATE_UTF8_ALLOW_NULL}',
    ),
    TestCase(
        'utf8/valid-with-null/reject',
        doc(utf8elem('foo', b'abc\x00123')),
        """
        This is a valid UTF-8 string that contains a null character. We reject
        this because we don't pass BSON_VALIDATE_UTF8_ALLOW_NULL.
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8_ALLOW_NULL, 'UTF-8 string contains a U+0000 (null) character', 4),
    ),
    TestCase(
        'utf8/overlong-null/accept-1',
        doc(utf8elem('foo', b'abc\xc0\x80123')),
        """
        This is an *invalid* UTF-8 string, and contains an overlong null. We should
        accept it because we aren't doing UTF-8 validation.
        """,
    ),
    TestCase(
        'utf8/overlong-null/accept-2',
        doc(utf8elem('foo', b'abc\xc0\x80123')),
        """
        ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence

        This is an *invalid* UTF-8 string, because it contains an overlong null
        "0xc0 0x80". Despite being invalid, we accept it because our current UTF-8
        validation considers the overlong null to be a valid encoding for the null
        codepoint (it isn't, but changing it would be a breaking change).

        If/when UTF-8 validation is changed to reject overlong null, then this
        test should change to expect rejection the invalid UTF-8.
        """,
        flags=f'{BSON_VALIDATE_UTF8} | {BSON_VALIDATE_UTF8_ALLOW_NULL}',
    ),
    TestCase(
        'utf8/overlong-null/reject',
        doc(utf8elem('foo', b'abc\xc0\x80123')),
        """
        ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence

        This is an *invalid* UTF-8 string, because it contains an overlong null
        character. Our UTF-8 validator wrongly accepts overlong null as a valid
        UTF-8 sequence. This test fails because we disallow null codepoints, not
        because the UTF-8 is invalid, and the error message reflects that.

        If/when UTF-8 validation is changed to reject overlong null, then the
        expected error code and error message for this test should change.
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8_ALLOW_NULL, 'UTF-8 string contains a U+0000 (null) character', 4),
    ),
    TestCase(
        'utf8-key/invalid/accept',
        doc(utf8elem(b'abc\xffdef', 'bar')),
        """
        The element key is not valid UTf-8, but we accept it if we don't do
        UTF-8 validation.
        """,
    ),
    TestCase(
        'utf8-key/invalid/reject',
        doc(utf8elem(b'abc\xffdef', 'bar')),
        """
        The element key is not valid UTF-8, and we reject it when we requested
        UTF-8 validation.
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8, 'Text element is not valid UTF-8', 4),
    ),
    TestCase(
        'utf8-key/overlong-null/reject',
        doc(utf8elem(b'abc\xc0\x80def', 'bar')),
        """
        ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence

        The element key is invalid UTF-8 because it contains an overlong null. We accept the
        overlong null as a valid encoding of U+0000, but we reject the key because
        we disallow null in UTF-8 strings.

        If/when UTF-8 validation is changed to reject overlong null, then the
        expected error code and error message for this test should change.
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8_ALLOW_NULL, 'UTF-8 string contains a U+0000 (null) character', 4),
    ),
    TestCase(
        'utf8-key/overlong-null/accept',
        doc(utf8elem(b'abc\xc0\x80def', 'bar')),
        """
        ! NOTE: overlong-null: This test relies on our UTF-8 validation accepting the `c0 80` sequence

        The element key is invalid UTF-8 because it contains an overlong null. We accept the
        overlong null as a valid encoding of U+0000, and we allow it in an element key because
        we pass ALLOW_NULL

        If/when UTF-8 validation is changed to reject overlong null, then this
        test case should instead reject the key string as invalid UTF-8.
        """,
        flags=f'{BSON_VALIDATE_UTF8} | {BSON_VALIDATE_UTF8_ALLOW_NULL}',
    ),
    TestCase(
        'array/empty',
        doc(elem('array', Tag.Array, doc())),
        'Simple empty array element',
    ),
    TestCase(
        'array/simple',
        doc(
            elem(
                'array',
                Tag.Array,
                doc(
                    elem('0', Tag.Int32, i32le(42)),
                    elem('1', Tag.Int32, i32le(1729)),
                    elem('2', Tag.Int32, i32le(-8)),
                ),
            )
        ),
        'Simple array element of integers',
    ),
    TestCase(
        'array/invalid-element',
        doc(
            elem(
                'array',
                Tag.Array,
                doc(
                    elem('0', Tag.Int32, i32le(42)),
                    elem('1', Tag.Int32, i32le(1729)[-1:]),  # Truncated
                    elem('2', Tag.Int32, i32le(-8)),
                ),
            )
        ),
        'Simple array element of integers, but one element is truncated',
        error=corruption_at(34),
    ),
    TestCase(
        'array/invalid-element-check-offset',
        doc(
            elem(
                'array-shifted',
                Tag.Array,
                doc(
                    elem('0', Tag.Int32, i32le(42)),
                    elem('1', Tag.Int32, i32le(1729)[-1:]),  # Truncated
                    elem('2', Tag.Int32, i32le(-8)),
                ),
            )
        ),
        """
        This is the same as the array/invalid-element test, but with a longer
        key string on the parent array. This is to check that the error offset
        is properly adjusted for the additional characters.
        """,
        error=corruption_at(42),
    ),
    TestCase(
        'symbol/simple',
        doc(elem('symbol', Tag.Symbol, string('void 0;'))),
        """
        A simple document: { symbol: Symbol("void 0;") }
        """,
    ),
    TestCase(
        'symbol/invalid-utf8/accept',
        doc(elem('symbol', Tag.Symbol, string(b'void\xff 0;'))),
        """
        A simple symbol document, but the string contains invalid UTF-8
        """,
    ),
    TestCase(
        'symbol/invalid-utf8/reject',
        doc(elem('symbol', Tag.Symbol, string(b'void\xff 0;'))),
        """
        A simple symbol document, but the string contains invalid UTF-8
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8, 'Text element is not valid UTF-8', 4),
    ),
    TestCase(
        'symbol/length-zero',
        doc(b'\x0e\0' + i32le(0) + b'\0'),
        'Symbol string length must always be at least 1 for the null terminator',
        error=corruption_at(6),
    ),
    TestCase(
        'symbol/length-too-short',
        doc(b'\x0e\0' + i32le(3) + b'bar\0'),
        """
        Symbol string is three chars and a null terminator, but the declared
        length is 3 (should be 4)
        """,
        error=corruption_at(12),
    ),
    TestCase(
        'code/simple',
        doc(elem('code', Tag.Code, string('void 0;'))),
        """
        A simple document: { code: Code("void 0;") }
        """,
    ),
    TestCase(
        'code/invalid-utf8/accept',
        doc(elem('code', Tag.Code, string(b'void\xff 0;'))),
        """
        A simple code document, but the string contains invalid UTF-8
        """,
    ),
    TestCase(
        'code/invalid-utf8/reject',
        doc(elem('code', Tag.Code, string(b'void\xff 0;'))),
        """
        A simple code document, but the string contains invalid UTF-8
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8, 'Text element is not valid UTF-8', 4),
    ),
    TestCase(
        'code/length-zero',
        doc(elem('code', Tag.Code, i32le(0), b'\0')),
        'Code string length must always be at least 1 for the null terminator',
        error=corruption_at(10),
    ),
    TestCase(
        'code/length-too-short',
        doc(elem('code', Tag.Code, i32le(3), b'bar\0')),
        'Code string is three chars and a null terminator, but the declared length is 3 (should be 4)',
        error=corruption_at(16),
    ),
    # Code w/ scope
    TestCase(
        'code-with-scope/simple',
        doc(elem('foo', Tag.CodeWithScope, code_with_scope('void 0;', doc()))),
        'A simple valid code-with-scope element',
    ),
    TestCase(
        'code-with-scope/invalid-code-length-zero',
        doc(
            elem(
                '',
                Tag.CodeWithScope,
                i32le(10),
                b'\0\0\0\0',  # strlen
                b'\0',  # code
                doc(),  # scope
            )
        ),
        """
        Data { "": CodeWithScope("", {}) }, but the code string length is zero, when
        it must be at least 1
        """,
        error=corruption_at(6),
    ),
    TestCase(
        'code-with-scope/invalid-code-length-too-large',
        doc(
            elem(
                '',
                Tag.CodeWithScope,
                i32le(10),
                b'\xff\xff\xff\xff',  # strlen (too big)
                b'\0',
                doc(),  # Scope
            )
        ),
        """
        Data { "": CodeWithScope("", {}) }, but the code string length is way too large
        """,
        error=corruption_at(6),
    ),
    TestCase(
        'code-with-scope/invalid-scope',
        doc(elem('foo', Tag.CodeWithScope, code_with_scope('void 0;', doc()[:-1]))),
        'A code-with-scope element, but the scope document is corrupted',
        error=corruption_at(13),
    ),
    TestCase(
        'code-with-scope/empty-key-in-scope',
        doc(
            elem(
                'code',
                Tag.CodeWithScope,
                code_with_scope(
                    'void 0;',
                    doc(
                        elem('obj', Tag.Document, doc(utf8elem('', 'string'))),
                    ),
                ),
            )
        ),
        """
        A code-with-scope element. The scope itself contains empty keys within
        objects, and we ask to reject empty keys. But the scope document should
        be treated as an opaque closure, so our outer validation rules do not
        apply.
        """,
        flags=BSON_VALIDATE_EMPTY_KEYS,
    ),
    TestCase(
        'code-with-scope/corrupt-scope',
        doc(
            elem(
                'code',
                Tag.CodeWithScope,
                code_with_scope(
                    'void 0;',
                    doc(
                        elem(
                            'foo',
                            Tag.UTF8,
                            i32le(0),  # Invalid string length
                            b'\0',
                        )
                    ),
                ),
            )
        ),
        'A code-with-scope element, but the scope contains corruption',
        error=ErrorInfo(BSON_VALIDATE_CORRUPT, 'Error in scope document for element "code": corrupt BSON', offset=13),
    ),
    TestCase(
        'code-with-scope/corrupt-scope-2',
        doc(
            elem(
                'code',
                Tag.CodeWithScope,
                code_with_scope(
                    'void 0;',
                    doc(
                        elem(
                            'foo',
                            Tag.UTF8,
                            b'\xff\xff\xff\xff',  # Invalid string length
                            b'\0',
                        )
                    ),
                ),
            )
        ),
        'A code-with-scope element, but the scope contains corruption',
        error=ErrorInfo(BSON_VALIDATE_CORRUPT, 'Error in scope document for element "code": corrupt BSON', offset=13),
    ),
    TestCase(
        'regex/simple',
        doc(elem('regex', Tag.Regex, b'1234\0gi\0')),
        """
        Simple document: { regex: Regex("1234", "gi") }
        """,
    ),
    TestCase(
        'regex/invalid-opts',
        doc(elem('regex', Tag.Regex, b'foo\0bar')),
        """
        A regular expression element with missing null terminator. The main
        option string "foo" has a null terminator, but the option component "bar"
        does not have a null terminator. A naive parse will see the doc's null
        terminator as the null terminator for the options string, but that's
        invalid!
        """,
        error=corruption_at(18),
    ),
    TestCase(
        'regex/double-null',
        doc(elem('regex', Tag.Regex, b'foo\0bar\0\0')),
        """
        A regular expression element with an extra null terminator. Since regex
        is delimited by its null terminator, the iterator will stop early before
        the actual EOD.
        """,
        error=corruption_at(21),
    ),
    TestCase(
        'regex/invalid-utf8/accept',
        doc(elem('regex', Tag.Regex, b'foo\xffbar\0gi\0')),
        """
        A regular expression that contains invalid UTF-8.
        """,
    ),
    TestCase(
        'regex/invalid-utf8/reject',
        doc(elem('regex', Tag.Regex, b'foo\xffbar\0gi\0')),
        """
        A regular expression that contains invalid UTF-8.
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8, 'Text element is not valid UTF-8', 4),
    ),
    TestCase(
        'regex/invalid-utf8/accept-if-absent',
        doc(elem('regex', Tag.Regex, b'foo\0gi\0')),
        """
        A regular valid UTf-8 regex. We check for invalid UTf-8, and accept becaues
        the regex is fine.
        """,
        flags=BSON_VALIDATE_UTF8,
    ),
    TestCase(
        'dbpointer/string-length-zero',
        doc(
            elem(
                'foo',
                Tag.DBPointer,
                i32le(0),  # String length (invalid)
                b'\0',  # Empty string
                b'\x52\x59\xb5\x6a\xfa\x5b\xd8\x41\xd6\x58\x5d\x99',  # OID
            )
        ),
        """
        Document { "foo": DBPointer("", <oid>) }, but the length header on the inner
        string is zero, when it must be at least 1.
        """,
        error=corruption_at(9),
    ),
    TestCase(
        'dbpointer/string-length-too-big',
        doc(
            elem(
                'foo',
                Tag.DBPointer,
                b'\xff\xff\xff\xff',  # String length  (invalid)
                b'foobar\0',  # Simple string
                b'\x52\x59\xb5\x6a\xfa\x5b\xd8\x41\xd6\x58\x5d\x99',  # OID
            )
        ),
        """
        Document { "foo": DBPointer("foobar", <oid>) }, but the length header on the inner
        string is far too large
        """,
        error=corruption_at(9),
    ),
    TestCase(
        'dbpointer/truncated',
        doc(
            utf8elem('a', 'b'),
            elem(
                'foo',
                Tag.DBPointer,
                i32le(7),  # 7 bytes, bleeding into the null terminator
                b'foobar',  # Simple string, missing a null terminator.
                b'\x00\x59\xb5\x6a\xfa\x5b\xd8\x41\xd6\x58\x5d\x99',  # OID
            ),
            utf8elem('a', 'b'),
        ),
        """
        Document { "foo": DBPointer("foobar", <oid>) }, but the length header on
        the string is one byte too large, causing it to use the first byte of the
        OID as the null terminator. This should fail when iterating.
        """,
        error=corruption_at(43),
    ),
    TestCase(
        'dbpointer/missing-null',
        doc(
            elem(
                'foo',
                Tag.DBPointer,
                i32le(4),
                b'abcd',  # Missing null terminator
                b'\x52\x59\xb5\x6a\xfa\x5b\xd8\x41\xd6\x58\x5d\x99',  # OID
            )
        ),
        """
        Document { "foo": DBPointer("abcd", <oid>) }, the length header on
        the string is 4, but the fourth byte is not a null terminator.
        """,
        error=corruption_at(16),
    ),
    TestCase(
        'dbpointer/invalid-utf8/accept',
        doc(
            elem(
                'foo',
                Tag.DBPointer,
                string(b'abc\xffdef'),  # String with invalid UTF-8
                b'\x52\x59\xb5\x6a\xfa\x5b\xd8\x41\xd6\x58\x5d\x99',  # OID
            )
        ),
        """
        DBPointer document, but the collection string contains invalid UTF-8
        """,
    ),
    TestCase(
        'dbpointer/invalid-utf8/reject',
        doc(
            elem(
                'foo',
                Tag.DBPointer,
                string(b'abc\xffdef'),  # String with invalid UTF-8
                b'\x52\x59\xb5\x6a\xfa\x5b\xd8\x41\xd6\x58\x5d\x99',  # OID
            )
        ),
        """
        DBPointer document, but the collection string contains invalid UTF-8
        """,
        flags=BSON_VALIDATE_UTF8,
        error=ErrorInfo(BSON_VALIDATE_UTF8, 'Text element is not valid UTF-8', 4),
    ),
    TestCase(
        'dbpointer/invalid-utf8/accept-if-absent',
        doc(
            elem(
                'foo',
                Tag.DBPointer,
                string(b'abcdef'),  # Valid string
                b'\x52\x59\xb5\x6a\xfa\x5b\xd8\x41\xd6\x58\x5d\x99',  # OID
            )
        ),
        """
        DBPointer document, and we validate UTF-8. Accepts because there is no
        invalid UTF-8 here.
        """,
        flags=BSON_VALIDATE_UTF8,
    ),
    TestCase(
        'subdoc/simple',
        doc(elem('doc', Tag.Document, doc(utf8elem('foo', 'bar')))),
        """
        A simple document: { doc: { foo: "bar" } }
        """,
    ),
    TestCase(
        'subdoc/invalid-shared-null',
        doc(elem('doc', Tag.Document, doc()[:-1])),
        """
        A truncated subdocument element, with its null terminator accidentally
        overlapping the parent document's null.
        """,
        error=corruption_at(9),
    ),
    TestCase(
        'subdoc/overlapping-utf8-null',
        doc(elem('doc', Tag.Document, doc(utf8elem('bar', 'baz\0')[:-1]))),
        """
        Encodes the document:

            { "foo": { "bar": "baz" } }

        but the foo.bar UTF-8 string is truncated improperly and reuses the null
        terminator for "foo"
        """,
        error=corruption_at(18),
    ),
    TestCase(
        'subdoc/invalid-element',
        doc(elem('doc', Tag.Document, doc(elem('dbl', Tag.Double, b'abcd')))),
        'A subdocument that contains an invalid element',
        error=corruption_at(18),
    ),
    TestCase(
        'subdoc/header-too-large',
        doc(
            elem(
                'foo',
                Tag.Document,
                b'\xf7\xff\xff\xff\0',  # Bad document
            ),
        ),
        """
        Data {"foo": {}}, but the subdoc header is too large.
        """,
        error=corruption_at(9),
    ),
    TestCase(
        'subdoc/header-too-small',
        doc(
            elem(
                'test',
                Tag.Document,
                b'\x04\0\0\0',  # Only four bytes. All docs must be at least 5
            ),
        ),
        """
        Nested document with a header value of 4, which is always too small.
        """,
        error=corruption_at(4),
    ),
    TestCase(
        'subdoc/impossible-size',
        doc(
            elem(
                'foo',
                Tag.Document,
                b'\xff\xff\xff\xff\0',  # Bad document
            ),
        ),
        """
        Data {"foo": {}}, but the subdoc header is UINT32_MAX/INT32_MIN, which
        becomes is an invalid document header.
        """,
        error=corruption_at(9),
    ),
    TestCase(
        'null/simple',
        doc(elem('null', Tag.Null)),
        """
        A simple document: { "null": null }
        """,
    ),
    TestCase(
        'undefined/simple',
        doc(elem('undefined', Tag.Undefined)),
        """
        A simple document: { "undefined": undefined }
        """,
    ),
    TestCase(
        'binary/simple',
        doc(elem('binary', Tag.Binary, binary(0x80, b'12345'))),
        """
        Simple binary data { "binary": Binary(0x80, b'12345') }
        """,
    ),
    TestCase(
        'binary/bad-length-zero-subtype-2',
        doc(
            elem(
                'binary',
                Tag.Binary,
                i32le(0),  # Invalid: Zero length
                b'\x02',  # subtype two
                i32le(4),  # Length of 4
                b'1234',  # payload
            ),
        ),
        """
        Binary data that has an invalid length header. It is subtype 2,
        which means it contains an additional length header.
        """,
        error=corruption_at(12),
    ),
    TestCase(
        'binary/bad-inner-length-on-subtype-2',
        doc(
            elem(
                'binary',
                Tag.Binary,
                i32le(8),  # Valid length
                b'\x02',  # subtype two
                i32le(2),  # Invalid length of (should be 4)
                b'1234',  # payload
            ),
        ),
        """
        Binary data that has an valid outer length header, but the inner length
        header for subtype 2 has an incorrect value.
        """,
        error=corruption_at(17),
    ),
    TestCase(
        'binary/bad-length-too-small',
        doc(
            elem(
                'binary',
                Tag.Binary,
                i32le(2),  # Length prefix (too small)
                b'\x80',  # subtype
                b'1234',  # payload
            ),
        ),
        """
        Data { "binary": Binary(0x80, b'1234') }, but the length header on
        the Binary object is too small.

        This won't cause the binary to decode wrong, but it will cause the iterator
        to jump into the middle of the binary data which will not decode as a
        proper BSON element.
        """,
        error=corruption_at(22),
    ),
    TestCase(
        'binary/bad-length-too-big',
        doc(
            elem(
                'binary',
                Tag.Binary,
                b'\xf3\xff\xff\xff',  # Length prefix (too big)
                b'\x80',  # subtype
                b'1234',  # data
            ),
        ),
        """
        Data { "binary": Binary(0x80, b'1234') }, but the length header on
        the Binary object is too large.
        """,
        error=corruption_at(12),
    ),
    TestCase(
        'binary/old-invalid/1',
        doc(
            elem(
                'binary',
                Tag.Binary,
                binary(
                    2,
                    i32le(5),  # Bad length prefix: Should be 4
                    b'abcd',
                ),
            ),
        ),
        """
        This is an old-style binary type 0x2. It has an inner length header of 5,
        but it should be 4.
        """,
        error=corruption_at(17),
    ),
    TestCase(
        'binary/old-invalid/2',
        doc(
            elem(
                'bin',
                Tag.Binary,
                binary(
                    2,
                    b'abc',  # Bad: Subtype 2 requires at least four bytes
                ),
            )
        ),
        """
        This is an old-style binary type 0x2. The data segment is too small to
        be valid.
        """,
        error=corruption_at(9),
    ),
    TestCase(
        'minkey/simple',
        doc(elem('min', Tag.MinKey)),
        'A simple document with a MinKey element',
    ),
    TestCase(
        'maxkey/simple',
        doc(elem('max', Tag.MaxKey)),
        'A simple document with a MaxKey element',
    ),
    TestCase(
        'int32/simple',
        doc(elem('int32', Tag.Int32, i32le(42))),
        'A simple document with a valid single int32 element',
    ),
    TestCase(
        'int32/truncated',
        doc(elem('int32-truncated', Tag.Int32, i32le(42)[:-1])),
        'Truncated 32-bit integer',
        error=corruption_at(21),
    ),
    TestCase('timestamp/simple', doc(elem('timestamp', Tag.Timestamp, i64le(1729))), """A simple timestamp element"""),
    TestCase(
        'timestamp/truncated',
        doc(elem('timestamp', Tag.Timestamp, i64le(1729)[:-1])),
        """A truncated timestamp element""",
        error=corruption_at(15),
    ),
    TestCase(
        'int64/simple',
        doc(elem('int64', Tag.Int64, i64le(1729))),
        'A simple document with a valid single int64 element',
    ),
    TestCase(
        'int64/truncated',
        doc(elem('int64-truncated', Tag.Int64, i64le(1729)[:-1])),
        'Truncated 64-bit integer',
        error=corruption_at(21),
    ),
    TestCase(
        'double/simple',
        doc(elem('double', Tag.Double, f64le(3.14))),
        'Simple float64 element',
    ),
    TestCase(
        'double/truncated',
        doc(elem('double-truncated', Tag.Double, f64le(3.13)[:-1])),
        'Truncated 64-bit float',
        error=corruption_at(22),
    ),
    TestCase(
        'boolean/simple-false',
        doc(elem('bool', Tag.Boolean, b'\x00')),
        """A simple boolean 'false'""",
    ),
    TestCase(
        'boolean/simple-true',
        doc(elem('bool', Tag.Boolean, b'\x01')),
        """A simple boolean 'true'""",
    ),
    TestCase(
        'boolean/invalid',
        doc(elem('bool', Tag.Boolean, b'\xc3')),
        """
        An invalid boolean octet. Must be '0' or '1', but is 0xc3.
        """,
        error=corruption_at(10),
    ),
    TestCase(
        'datetime/simple',
        doc(elem('utc', Tag.Datetime, b'\x0b\x98\x8c\x2b\x33\x01\x00\x00')),
        'Simple datetime element',
    ),
    TestCase(
        'datetime/truncated',
        doc(elem('utc', Tag.Datetime, b'\x0b\x98\x8c\x2b\x33\x01\x00')),
        'Truncated datetime element',
        error=corruption_at(9),
    ),
    # DBRef
    TestCase(
        'dbref/missing-id',
        doc(utf8elem('$ref', 'foo')),
        """This dbref document is missing an $id element""",
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, MSG_EXPECTED_ID_FOLLOWING_REF, 18),
    ),
    TestCase(
        'dbref/non-id',
        doc(utf8elem('$ref', 'foo'), utf8elem('bar', 'baz')),
        """
        The 'bar' element should be an '$id' element.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, MSG_EXPECTED_ID_FOLLOWING_REF, 18),
    ),
    TestCase(
        'dbref/not-first-elements',
        doc(utf8elem('foo', 'bar'), utf8elem('$ref', 'a'), utf8elem('$id', 'b')),
        """
        This would be a valid DBRef, but the "$ref" key must come first.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, disallowed_key('$', '$ref'), 17),
    ),
    TestCase(
        'dbref/ref-without-id-with-db',
        doc(utf8elem('$ref', 'foo'), utf8elem('$db', 'bar')),
        """
        There should be an $id element, but we skip straight to $db
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, MSG_EXPECTED_ID_FOLLOWING_REF, 18),
    ),
    TestCase(
        'dbref/non-string-ref',
        doc(elem('$ref', Tag.Int32, i32le(42))),
        """
        The $ref element must be a string, but is an integer.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, '$ref element must be a UTF-8 element', 4),
    ),
    TestCase(
        'dbref/non-string-db',
        doc(
            utf8elem('$ref', 'foo'),
            utf8elem('$id', 'bar'),
            elem('$db', Tag.Int32, i32le(42)),
        ),
        """
        The $db element should be a string, but is an integer.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, '$db element in DBRef must be a UTF-8 element', 31),
    ),
    TestCase(
        'dbref/invalid-extras-between',
        doc(
            utf8elem('$ref', 'foo'),
            utf8elem('$id', 'bar'),
            utf8elem('extra', 'field'),
            utf8elem('$db', 'baz'),
        ),
        """
        Almost a valid DBRef, but there is an extra field before $db. We reject $db
        as an invalid key.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, disallowed_key('$', '$db'), 48),
    ),
    TestCase(
        'dbref/invalid-double-ref',
        doc(
            utf8elem('$ref', 'foo'),
            utf8elem('$ref', 'bar'),
            utf8elem('$id', 'baz'),
        ),
        """
        Invalid DBRef contains a second $ref element.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, MSG_EXPECTED_ID_FOLLOWING_REF, 18),
    ),
    TestCase(
        'dbref/invalid-missing-ref',
        doc(utf8elem('$id', 'foo')),
        """
        DBRef document requires a $ref key to be first.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
        error=ErrorInfo(BSON_VALIDATE_DOLLAR_KEYS, disallowed_key('$', '$id'), 4),
    ),
    TestCase(
        'dbref/valid/simple',
        doc(utf8elem('$ref', 'foo'), utf8elem('$id', 'bar')),
        """
        This is a simple valid DBRef element.
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
    ),
    TestCase(
        'dbref/valid/simple-with-db',
        doc(utf8elem('$ref', 'foo'), utf8elem('$id', 'bar'), utf8elem('$db', 'baz')),
        """
        A simple DBRef of the form:

            { $ref: "foo", $id: "bar", $db: "baz" }
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
    ),
    TestCase(
        'dbref/valid/nested-id-doc',
        doc(
            utf8elem('$ref', 'foo'),
            elem(
                '$id',
                Tag.Document,
                doc(
                    utf8elem('$ref', 'foo2'),
                    utf8elem('$id', 'bar2'),
                    utf8elem('$db', 'baz2'),
                ),
            ),
            utf8elem('$db', 'baz'),
        ),
        """
        This is a valid DBRef of the form:

            { $ref: foo, $id: { $ref: "foo2", $id: "bar2", $db: "baz2" }, $db: "baz" }
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
    ),
    TestCase(
        'dbref/valid/trailing-content',
        doc(
            utf8elem('$ref', 'foo'),
            utf8elem('$id', 'bar'),
            utf8elem('$db', 'baz'),
            utf8elem('extra', 'field'),
        ),
        """
        A valid DBRef of the form:

            {
                $ref: "foo",
                $id: "bar",
                $db: "baz",
                extra: "field",
            }
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
    ),
    TestCase(
        'dbref/valid/trailing-content-no-db',
        doc(
            utf8elem('$ref', 'foo'),
            utf8elem('$id', 'bar'),
            utf8elem('extra', 'field'),
        ),
        """
        A valid DBRef of the form:

            {
                $ref: "foo",
                $id: "bar",
                extra: "field",
            }
        """,
        flags=BSON_VALIDATE_DOLLAR_KEYS,
    ),
]

if __name__ == '__main__':
    # We don't take an arguments, but error if any are given
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    # Start with the header
    print(HEADER)
    # Print each test case
    for c in CASES:
        print()
        for part in generate(c):
            print(part, end='')

    # Print the registration function
    print(f'\n{GENERATED_NOTE}')
    print('void test_install_generated_bson_validation(TestSuite* suite) {')
    for c in CASES:
        print(f'  TestSuite_Add(suite, "/bson/validate/" {json.dumps(c.name)}, {c.fn_name});')
    print('}')
