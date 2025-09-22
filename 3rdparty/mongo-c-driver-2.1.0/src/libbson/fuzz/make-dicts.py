from __future__ import annotations

import enum
import struct
from typing import Final, Iterable, NamedTuple, Sequence


def generate():
    simple_oid = OID((1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12))
    ITEMS: list[LineItem] = [
        Comment("This file is GENERATED! DO NOT MODIFY!"),
        Comment("Instead, modify the content of make-dicts.py"),
        Line(),
        Comment("Random values"),
        Entry("int32_1729", encode_value(1729)),
        Entry("int64_1729", struct.pack("<q", 1729)),
        Entry("prefixed_string", make_string("prefixed-string")),
        Entry("empty_obj", wrap_obj(b"")),
        Entry("c_string", make_cstring("null-terminated-string")),
        Line(),
        Comment("Elements"),
        Entry("null_elem", element("N", None)),
        Entry("undef_elem", element("U", Undefined)),
        Entry("string_elem", element("S", "string-value")),
        Entry("empty_bin_elem", element("Bg", Binary(0, b""))),
        Entry("empty_regex_elem", element("Rx0", Regex("", ""))),
        Entry("simple_regex_elem", element("Rx1", Regex("foo", "ig"))),
        Entry("encrypted_bin_elem", element("Be", Binary(6, b"meow"))),
        Entry("empty_obj_elem", element("Obj0", Doc())),
        Entry(
            "code_w_s_elem",
            element("Clz", CodeWithScope("void 0;", Doc([Elem("foo", "bar")]))),
        ),
        Entry("code_elem", element("Js", Code("() => 0;"))),
        Entry("symbol_elem", element("Sym", Symbol("symbol"))),
        Entry("oid_elem", element("OID", simple_oid)),
        Entry("dbpointer_elem", element("dbp", DBPointer(String("db"), simple_oid))),
        Line(),
        Comment("Embedded nul"),
        Comment("This string contains an embedded null, which is abnormal but valid"),
        Entry("string_with_null", element("S0", "string\0value")),
        Comment("This regex has an embedded null, which is invalid"),
        Entry("bad_regex_elem", element("RxB", Regex("f\0oo", "ig"))),
        Comment("This element's key contains an embedded null, which is invalid"),
        Entry("bad_key_elem", element("foo\0bar", "string")),
        Line(),
        Comment("Objects"),
        Entry("obj_with_string", wrap_obj(element("single-elem", "foo"))),
        Entry("obj_with_null", wrap_obj(element("null", None))),
        Entry("obj_missing_term", wrap_obj(b"")[:-1]),
    ]

    for it in ITEMS:
        emit(it)


BytesIter = bytes | Iterable["BytesIter"]


def flatten(b: BytesIter) -> bytes:
    if isinstance(b, bytes):
        return b
    else:
        return b"".join(map(flatten, b))


def len_prefix(b: BytesIter) -> bytes:
    """Prepend an i32le byte-length prefix to a set of bytes"""
    b = flatten(b)
    length = len(b)
    return encode_value(length) + b


def make_cstring(s: str) -> bytes:
    """Encode a UTF-8 string and append a null terminator"""
    return s.encode("utf-8") + b"\0"


def make_string(s: str) -> bytes:
    """Create a length-prefixed string byte sequence"""
    return len_prefix(make_cstring(s))


def wrap_obj(items: BytesIter) -> bytes:
    """Wrap a sequence of bytes as if a BSON object (adds a header and trailing nul)"""
    bs = flatten(items)
    header = len(bs) + 5
    return encode_value(header) + bs + b"\0"


class UndefinedType:
    def __bytes__(self) -> bytes:
        return b""


class Binary(NamedTuple):
    tag: int
    data: bytes

    def __bytes__(self) -> bytes:
        return encode_value(len(self.data)) + bytes([self.tag]) + self.data


class OID(NamedTuple):
    octets: tuple[int, int, int, int, int, int, int, int, int, int, int, int]

    def __bytes__(self) -> bytes:
        return bytes(self.octets)


class DBPointer(NamedTuple):
    db: String
    oid: OID

    def __bytes__(self) -> bytes:
        return self.db.__bytes__() + self.oid.__bytes__()


class Regex(NamedTuple):
    rx: str
    opts: str

    def __bytes__(self) -> bytes:
        return make_cstring(self.rx) + make_cstring(self.opts)


class Elem(NamedTuple):
    key: str
    val: ValueType

    def __bytes__(self) -> bytes:
        return element(self.key, self.val)


class Doc(NamedTuple):
    items: Sequence[Elem] = ()

    def __bytes__(self) -> bytes:
        return wrap_obj((e.__bytes__() for e in self.items))


class String(NamedTuple):
    s: str

    def __bytes__(self) -> bytes:
        return make_string(self.s)


class Symbol(String):
    pass


class Code(String):
    pass


class CodeWithScope(NamedTuple):
    code: str
    scope: Doc

    def __bytes__(self) -> bytes:
        string = make_string(self.code)
        doc = self.scope.__bytes__()
        length = len(string) + len(doc) + 4
        return encode_value(length) + string + doc


Undefined: Final = UndefinedType()
ValueType = (
    int
    | str
    | float
    | None
    | UndefinedType
    | Binary
    | Regex
    | Doc
    | CodeWithScope
    | String
    | Symbol
    | Code
    | OID
    | DBPointer
)


def encode_value(val: ValueType) -> bytes:
    match val:
        case int(n):
            return struct.pack("<i", n)
        case str(s):
            return make_string(s)
        case float(f):
            return struct.pack("<d", f)
        case (
            Doc()
            | Binary()
            | Regex()
            | UndefinedType()
            | CodeWithScope()
            | Code()
            | String()
            | Symbol()
            | DBPointer()
            | OID()
        ) as d:
            return d.__bytes__()
        case None:
            return b""


class Tag(enum.Enum):
    EOD = 0
    Double = 1
    UTF8 = 2
    Doc = 3
    Binary = 5
    Undefined = 6
    OID = 7
    Null = 10
    Regex = 11
    DBPointer = 12
    Code = 13
    Symbol = 14
    CodeWithScope = 15
    Int32 = 16
    Int64 = 18


def element(key: str, value: ValueType, *, type: None | Tag = None) -> bytes:
    if type is not None:
        return flatten([bytes([type.value]), make_cstring(key), encode_value(value)])

    match value:
        case int():
            type = Tag.Int32

        case float():
            type = Tag.Double
        case None:
            type = Tag.Null
        case UndefinedType():
            type = Tag.Undefined
        case Binary():
            type = Tag.Binary
        case Regex():
            type = Tag.Regex
        case Doc():
            type = Tag.Doc
        case CodeWithScope():
            type = Tag.CodeWithScope
        case Code():
            type = Tag.Code
        case Symbol():
            type = Tag.Symbol
        case str() | String():  # Must appear after Code()/Symbol()
            type = Tag.UTF8
        case OID():
            type = Tag.OID
        case DBPointer():
            type = Tag.DBPointer
    return element(key, value, type=type)


class Entry(NamedTuple):
    key: str
    "The key for the entry. Only for human readability"
    value: bytes
    "The arbitrary bytes that make up the entry"


class Comment(NamedTuple):
    txt: str


class Line(NamedTuple):
    txt: str = ""


LineItem = Entry | Comment | Line


def escape(b: bytes) -> Iterable[str]:
    s = b.decode("ascii", "backslashreplace")
    for u8 in b:
        s = chr(u8)  # 0 <= u8 and u8 <= 255
        if s.isascii() and s.isprintable():
            yield s
            continue
        # Byte is not valid ASCII, or is not a printable char
        yield f"\\x{u8:0>2x}"


def emit(item: LineItem):
    match item:
        case Line(t):
            print(t)
        case Comment(txt):
            print(f"# {txt}")
        case Entry(key, val):
            s = "".join(escape(val))
            s = s.replace('"', r"\x22")
            print(f'{key}="{s}"')


if __name__ == "__main__":
    generate()
