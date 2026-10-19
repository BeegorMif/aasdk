#!/usr/bin/env python3
"""Compare two trees of .proto files (average-web-dev/aasdk#46, #14).

    tools/proto_compare.py protobuf/aap_protobuf ../open-android-auto/oaa
    tools/proto_compare.py A B --ignore-required --alias 'CarPropertyData=CarPropertyConfig'

Lists, for tree A and tree B:
  - messages and enums that exist on one side only,
  - fields (by number) and enum values (by number) that exist on one side only,
  - fields with the same number but a different type or label, and fields or enum values with the
    same number but a different name (renames are listed, they are no wire difference).

Messages and enums are matched by their name without the package (nested types as Outer.Inner),
because the projects use different packages. Field types are compared by their last name component
for the same reason. Use --alias to match types that were renamed.

Known false differences you may want to hide: --ignore-required (a project that makes everything
optional against one that keeps required), --ignore-names.

Parses proto2 and proto3 (messages, nested types, enums, oneof, map, reserved, options). Standard
library only. Exit status: 0, or 1 with --fail-on-diff when there are differences.
"""

import argparse
import dataclasses
import pathlib
import re
import sys

SCALARS = {
    "double", "float", "int32", "int64", "uint32", "uint64", "sint32", "sint64",
    "fixed32", "fixed64", "sfixed32", "sfixed64", "bool", "string", "bytes",
}
LABELS = ("required", "optional", "repeated")

TOKEN_RE = re.compile(
    r"""
    (?P<string>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')
  | (?P<number>-?(?:0[xX][0-9a-fA-F]+|\d+))
  | (?P<ident>[A-Za-z_.][A-Za-z0-9_.]*)
  | (?P<symbol>[{}\[\]()<>;=,:])
  | (?P<space>\s+)
    """,
    re.VERBOSE,
)


class ParseError(Exception):
    pass


@dataclasses.dataclass
class Field:
    number: int
    name: str
    label: str
    type: str


@dataclasses.dataclass
class Message:
    name: str
    fields: dict = dataclasses.field(default_factory=dict)  # number -> Field
    file: str = ""


@dataclasses.dataclass
class Enum:
    name: str
    values: dict = dataclasses.field(default_factory=dict)  # number -> [names]
    file: str = ""


@dataclasses.dataclass
class Tree:
    messages: dict = dataclasses.field(default_factory=dict)  # name -> Message
    enums: dict = dataclasses.field(default_factory=dict)  # name -> Enum
    warnings: list = dataclasses.field(default_factory=list)


def tokenize(text):
    """Tokens of a .proto source without comments; strings stay one token."""
    # Comments first, but not inside strings.
    out = []
    pos = 0
    length = len(text)
    while pos < length:
        if text.startswith("//", pos):
            end = text.find("\n", pos)
            pos = length if end < 0 else end
            continue
        if text.startswith("/*", pos):
            end = text.find("*/", pos + 2)
            if end < 0:
                raise ParseError("unterminated comment")
            pos = end + 2
            continue
        match = TOKEN_RE.match(text, pos)
        if not match:
            raise ParseError("unexpected character %r" % text[pos])
        pos = match.end()
        if match.lastgroup != "space":
            out.append(match.group())
    return out


class Parser:
    def __init__(self, tokens, path):
        self.tokens = tokens
        self.pos = 0
        self.path = path
        self.proto3 = False
        self.messages = {}
        self.enums = {}

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def next(self):
        token = self.peek()
        if token is None:
            raise ParseError("unexpected end of file")
        self.pos += 1
        return token

    def expect(self, token):
        got = self.next()
        if got != token:
            raise ParseError("expected %r, got %r" % (token, got))

    def skip_balanced(self, open_, close):
        """Skip a bracket group whose opening token was already consumed."""
        depth = 1
        while depth:
            token = self.next()
            if token == open_:
                depth += 1
            elif token == close:
                depth -= 1

    def skip_statement(self):
        """Skip up to and including the ';' (option values may contain braces)."""
        while True:
            token = self.next()
            if token == ";":
                return
            if token == "{":
                self.skip_balanced("{", "}")
            elif token == "[":
                self.skip_balanced("[", "]")

    def skip_field_options(self):
        if self.peek() == "[":
            self.next()
            self.skip_balanced("[", "]")

    def parse(self):
        while self.peek() is not None:
            token = self.next()
            if token == "syntax":
                self.expect("=")
                self.proto3 = self.next().strip("\"'") == "proto3"
                self.expect(";")
            elif token == "message":
                self.parse_message("")
            elif token == "enum":
                self.parse_enum("")
            elif token in ("service", "extend"):
                self.next()
                self.expect("{")
                self.skip_balanced("{", "}")
            elif token == ";":
                continue
            else:  # package, import, option, edition
                self.skip_statement()

    def parse_message(self, prefix):
        name = prefix + self.next()
        message = Message(name, file=self.path)
        self.messages[name] = message
        self.expect("{")
        self.parse_message_body(name, message)

    def parse_message_body(self, name, message):
        while True:
            token = self.next()
            if token == "}":
                return
            if token == ";":
                continue
            if token == "message":
                self.parse_message(name + ".")
            elif token == "enum":
                self.parse_enum(name + ".")
            elif token == "oneof":
                self.next()
                self.expect("{")
                self.parse_message_body(name, message)  # members are fields of the message
            elif token in ("option", "reserved", "extensions"):
                self.skip_statement()
            elif token == "extend":
                self.next()
                self.expect("{")
                self.skip_balanced("{", "}")
            else:
                self.parse_field(message, token)

    def parse_field(self, message, token):
        label = "optional" if self.proto3 else None
        if token in LABELS:
            label = token
            token = self.next()
        elif not self.proto3 and token != "map":
            label = "optional"  # oneof member (proto2 has no unlabeled fields otherwise)
        if token == "map":
            self.expect("<")
            key = self.next()
            self.expect(",")
            value = self.next()
            self.expect(">")
            type_ = "map<%s,%s>" % (key, short(value))
            label = "repeated"
        elif token == "group":
            raise ParseError("groups are not supported")
        else:
            type_ = token
        name = self.next()
        self.expect("=")
        number = int(self.next(), 0)
        self.skip_field_options()
        self.expect(";")
        message.fields[number] = Field(number, name, label or "optional", type_)

    def parse_enum(self, prefix):
        name = prefix + self.next()
        enum = Enum(name, file=self.path)
        self.enums[name] = enum
        self.expect("{")
        while True:
            token = self.next()
            if token == "}":
                return
            if token == ";":
                continue
            if token in ("option", "reserved"):
                self.skip_statement()
                continue
            self.expect("=")
            number = int(self.next(), 0)
            self.skip_field_options()
            self.expect(";")
            enum.values.setdefault(number, []).append(token)


def short(type_name):
    """Last component of a (possibly qualified) type name; scalars stay as they are."""
    return type_name.rsplit(".", 1)[-1]


def parse_tree(root):
    tree = Tree()
    root = pathlib.Path(root)
    if not root.is_dir():
        raise SystemExit("%s: not a directory" % root)
    for path in sorted(root.rglob("*.proto")):
        relative = str(path.relative_to(root))
        try:
            parser = Parser(tokenize(path.read_text(encoding="utf-8", errors="replace")), relative)
            parser.parse()
        except ParseError as error:
            tree.warnings.append("%s: %s" % (relative, error))
            continue
        for kind, found, target in (
            ("message", parser.messages, tree.messages),
            ("enum", parser.enums, tree.enums),
        ):
            for name, item in found.items():
                if name not in target:
                    target[name] = item
                elif content(target[name]) != content(item):
                    # the same name in two packages with the same content is harmless
                    tree.warnings.append(
                        "%s: %s %s is also defined in %s (ignored)" % (relative, kind, name, target[name].file)
                    )
    return tree


def content(item):
    return item.fields if isinstance(item, Message) else item.values


def type_key(type_name, aliases):
    if type_name in SCALARS or type_name.startswith("map<"):
        return type_name
    name = short(type_name)
    return aliases.get(name, name)


@dataclasses.dataclass
class Options:
    ignore_required: bool = False
    ignore_names: bool = False
    aliases_a: dict = dataclasses.field(default_factory=dict)  # name in A -> name in B
    aliases_b: dict = dataclasses.field(default_factory=dict)


def compare(tree_a, tree_b, options=None):
    """Returns a list of (section, text) lines, sorted by section and text."""
    options = options or Options()
    # A's type and message names are translated to B's names, so everything is compared in B's terms.
    to_b = options.aliases_a
    out = []

    def a_name(name):
        return to_b.get(name, name)

    messages_a = {a_name(name): message for name, message in tree_a.messages.items()}
    enums_a = {a_name(name): enum for name, enum in tree_a.enums.items()}

    for kind, side_a, side_b in (("message", messages_a, tree_b.messages), ("enum", enums_a, tree_b.enums)):
        for name in sorted(set(side_a) - set(side_b)):
            out.append(("only in A", "%s %s" % (kind, name)))
        for name in sorted(set(side_b) - set(side_a)):
            out.append(("only in B", "%s %s" % (kind, name)))

    for name in sorted(set(messages_a) & set(tree_b.messages)):
        fields_a = messages_a[name].fields
        fields_b = tree_b.messages[name].fields
        for number in sorted(set(fields_a) - set(fields_b)):
            out.append(("only in A", "%s field %d %s" % (name, number, describe(fields_a[number]))))
        for number in sorted(set(fields_b) - set(fields_a)):
            out.append(("only in B", "%s field %d %s" % (name, number, describe(fields_b[number]))))
        for number in sorted(set(fields_a) & set(fields_b)):
            field_a, field_b = fields_a[number], fields_b[number]
            type_a = type_key(field_a.type, to_b)
            type_b = type_key(field_b.type, options.aliases_b)
            if type_a != type_b:
                out.append(("type", "%s field %d: %s -> %s" % (name, number, field_a.type, field_b.type)))
            label_a, label_b = field_a.label, field_b.label
            if options.ignore_required:
                label_a = "optional" if label_a == "required" else label_a
                label_b = "optional" if label_b == "required" else label_b
            if label_a != label_b:
                out.append(("label", "%s field %d: %s -> %s" % (name, number, field_a.label, field_b.label)))
            if not options.ignore_names and field_a.name != field_b.name:
                out.append(("name", "%s field %d: %s -> %s" % (name, number, field_a.name, field_b.name)))

    for name in sorted(set(enums_a) & set(tree_b.enums)):
        values_a = enums_a[name].values
        values_b = tree_b.enums[name].values
        for number in sorted(set(values_a) - set(values_b)):
            out.append(("only in A", "enum %s value %d %s" % (name, number, "/".join(values_a[number]))))
        for number in sorted(set(values_b) - set(values_a)):
            out.append(("only in B", "enum %s value %d %s" % (name, number, "/".join(values_b[number]))))
        if not options.ignore_names:
            for number in sorted(set(values_a) & set(values_b)):
                if sorted(values_a[number]) != sorted(values_b[number]):
                    out.append(
                        (
                            "name",
                            "enum %s value %d: %s -> %s"
                            % (name, number, "/".join(sorted(values_a[number])), "/".join(sorted(values_b[number]))),
                        )
                    )
    order = {"only in A": 0, "only in B": 1, "type": 2, "label": 3, "name": 4}
    return sorted(out, key=lambda item: (order[item[0]], item[1]))


def describe(field):
    return "%s %s %s" % (field.label, field.type, field.name)


def parse_aliases(pairs):
    aliases = {}
    for pair in pairs or []:
        if "=" not in pair:
            raise SystemExit("--alias expects A_NAME=B_NAME, got %r" % pair)
        left, right = pair.split("=", 1)
        aliases[short(left)] = short(right)
    return aliases


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("tree_a", help="directory with .proto files (side A, e.g. aasdk)")
    parser.add_argument("tree_b", help="directory with .proto files (side B, e.g. another project)")
    parser.add_argument("--ignore-required", action="store_true", help="treat required like optional")
    parser.add_argument("--ignore-names", action="store_true", help="do not list renamed fields and enum values")
    parser.add_argument(
        "--alias",
        action="append",
        metavar="A_NAME=B_NAME",
        help="message, enum or type that is called A_NAME in A and B_NAME in B (repeatable)",
    )
    parser.add_argument("--fail-on-diff", action="store_true", help="exit with status 1 if there are differences")
    args = parser.parse_args(argv)

    tree_a = parse_tree(args.tree_a)
    tree_b = parse_tree(args.tree_b)
    aliases = parse_aliases(args.alias)
    options = Options(args.ignore_required, args.ignore_names, aliases, {})
    result = compare(tree_a, tree_b, options)

    for label, tree in (("A", tree_a), ("B", tree_b)):
        for warning in tree.warnings:
            print("warning (%s): %s" % (label, warning), file=sys.stderr)
    print("A: %d messages, %d enums   B: %d messages, %d enums" % (
        len(tree_a.messages), len(tree_a.enums), len(tree_b.messages), len(tree_b.enums)))
    current = None
    titles = {
        "only in A": "Only in A",
        "only in B": "Only in B",
        "type": "Different type (A -> B)",
        "label": "Different label (A -> B)",
        "name": "Different name (A -> B)",
    }
    for section, text in result:
        if section != current:
            print("\n%s:" % titles[section])
            current = section
        print("  " + text)
    if not result:
        print("\nNo differences.")
    return 1 if (result and args.fail_on_diff) else 0


if __name__ == "__main__":
    sys.exit(main())
