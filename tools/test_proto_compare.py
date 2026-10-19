#!/usr/bin/env python3
"""Tests for proto_compare.py: python3 -m unittest discover -s tools -p 'test_*.py'"""

import pathlib
import tempfile
import textwrap
import unittest

import proto_compare

REPO = pathlib.Path(__file__).resolve().parent.parent

PROTO_A = """
    syntax = "proto2";
    package a.pkg;
    import "other.proto";
    option java_package = "x.y";

    // comment with message Fake { optional int32 nope = 1; }
    message Request {
      required string id = 1;
      optional Mode mode = 2;
      optional int32 only_a = 3;
      repeated Inner items = 4 [deprecated = true];
      optional string old_name = 5;
      message Inner {
        optional bool flag = 1;
      }
      enum Mode {
        MODE_A = 0;
        MODE_B = 1;
        MODE_ONLY_A = 2;
      }
    }

    message OnlyInA { optional int32 x = 1; }

    enum Color {
      option allow_alias = true;
      RED = 0;
      CRIMSON = 0;
      GREEN = 1;
      BLUE = -1;
    }
"""

PROTO_B = """
    syntax = "proto3";
    package b.other;

    message Request {
      string id = 1;
      Mode mode = 2;
      uint32 only_b = 6;
      repeated Inner items = 4;
      optional string new_name = 5;
      map<string, Inner> by_name = 7;
      oneof choice {
        int64 first = 8;
        string second = 9;
      }
      reserved 10, 11;
      message Inner {
        bool flag = 1;
      }
      enum Mode {
        MODE_A = 0;
        MODE_B = 1;
        MODE_ONLY_B = 3;
      }
    }

    message OnlyInB { int32 x = 1; }

    enum Color {
      RED = 0;
      GREEN = 2;
      BLUE = -1;
    }
"""


def write_tree(directory, name, content):
    root = pathlib.Path(directory) / name
    (root / "sub").mkdir(parents=True)
    (root / "sub" / "file.proto").write_text(textwrap.dedent(content))
    return root


class ParseTest(unittest.TestCase):
    def parse(self, content):
        with tempfile.TemporaryDirectory() as directory:
            return proto_compare.parse_tree(write_tree(directory, "t", content))

    def test_messages_fields_and_nested_types(self):
        tree = self.parse(PROTO_A)
        self.assertEqual(set(tree.messages), {"Request", "Request.Inner", "OnlyInA"})
        request = tree.messages["Request"]
        self.assertEqual(sorted(request.fields), [1, 2, 3, 4, 5])
        self.assertEqual(request.fields[1].label, "required")
        self.assertEqual(request.fields[4].label, "repeated")
        self.assertEqual(request.fields[4].type, "Inner")
        self.assertEqual(tree.warnings, [])

    def test_comments_do_not_define_anything(self):
        tree = self.parse(PROTO_A)
        self.assertNotIn("Fake", tree.messages)

    def test_enums_with_aliases_and_negative_values(self):
        tree = self.parse(PROTO_A)
        self.assertEqual(tree.enums["Color"].values, {0: ["RED", "CRIMSON"], 1: ["GREEN"], -1: ["BLUE"]})
        self.assertIn("Request.Mode", tree.enums)

    def test_proto3_labels_oneof_map_and_reserved(self):
        tree = self.parse(PROTO_B)
        request = tree.messages["Request"]
        self.assertEqual(request.fields[1].label, "optional")
        self.assertEqual(request.fields[7].type, "map<string,Inner>")
        self.assertEqual(request.fields[7].label, "repeated")
        self.assertEqual(request.fields[8].name, "first")
        self.assertEqual(request.fields[9].name, "second")
        self.assertNotIn(10, request.fields)

    def test_parse_error_is_a_warning(self):
        tree = self.parse("message Broken { optional int32 x = ")
        self.assertEqual(tree.messages, {})
        self.assertEqual(len(tree.warnings), 1)

    def test_duplicate_definition_is_a_warning_if_it_differs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = write_tree(directory, "t", "message M { optional int32 x = 1; }")
            (root / "other.proto").write_text("message M { optional int32 y = 2; }")
            (root / "a_same.proto").write_text("package p; message M { optional int32 x = 1; }")
            tree = proto_compare.parse_tree(root)
        self.assertEqual(len(tree.warnings), 1)
        self.assertIn("M", tree.warnings[0])


class CompareTest(unittest.TestCase):
    def compare(self, options=None):
        with tempfile.TemporaryDirectory() as directory:
            tree_a = proto_compare.parse_tree(write_tree(directory, "a", PROTO_A))
            tree_b = proto_compare.parse_tree(write_tree(directory, "b", PROTO_B))
        return proto_compare.compare(tree_a, tree_b, options)

    def lines(self, section, options=None):
        return [text for name, text in self.compare(options) if name == section]

    def test_only_on_one_side(self):
        only_a = self.lines("only in A")
        only_b = self.lines("only in B")
        self.assertIn("message OnlyInA", only_a)
        self.assertIn("message OnlyInB", only_b)
        self.assertIn("Request field 3 optional int32 only_a", only_a)
        self.assertIn("Request field 6 optional uint32 only_b", only_b)
        self.assertIn("Request field 7 repeated map<string,Inner> by_name", only_b)
        self.assertIn("enum Request.Mode value 2 MODE_ONLY_A", only_a)
        self.assertIn("enum Request.Mode value 3 MODE_ONLY_B", only_b)
        self.assertIn("enum Color value 1 GREEN", only_a)
        self.assertIn("enum Color value 2 GREEN", only_b)

    def test_label_difference_and_ignore_required(self):
        self.assertIn("Request field 1: required -> optional", self.lines("label"))
        options = proto_compare.Options(ignore_required=True)
        self.assertEqual(self.lines("label", options), [])

    def test_type_difference(self):
        with tempfile.TemporaryDirectory() as directory:
            tree_a = proto_compare.parse_tree(write_tree(directory, "a", "message M { optional int32 x = 1; optional P y = 2; }"))
            tree_b = proto_compare.parse_tree(write_tree(directory, "b", "message M { optional int64 x = 1; optional q.Q y = 2; }"))
        result = proto_compare.compare(tree_a, tree_b)
        self.assertEqual(
            [item for item in result if item[0] == "type"],
            [("type", "M field 1: int32 -> int64"), ("type", "M field 2: P -> q.Q")],
        )
        # the same message type under another name is no difference with an alias
        options = proto_compare.Options(aliases_a={"P": "Q"})
        result = proto_compare.compare(tree_a, tree_b, options)
        self.assertEqual([item for item in result if item[0] == "type"], [("type", "M field 1: int32 -> int64")])

    def test_package_and_qualified_type_names_do_not_matter(self):
        self.assertEqual(self.lines("type"), [])

    def test_renames(self):
        names = self.lines("name")
        self.assertIn("Request field 5: old_name -> new_name", names)
        self.assertIn("enum Color value 0: CRIMSON/RED -> RED", names)
        self.assertEqual(self.lines("name", proto_compare.Options(ignore_names=True)), [])

    def test_alias_for_renamed_message(self):
        with tempfile.TemporaryDirectory() as directory:
            tree_a = proto_compare.parse_tree(write_tree(directory, "a", "message Old { optional int32 x = 1; }"))
            tree_b = proto_compare.parse_tree(write_tree(directory, "b", "message New { optional int32 x = 1; optional int32 y = 2; }"))
        self.assertEqual(
            proto_compare.compare(tree_a, tree_b),
            [("only in A", "message Old"), ("only in B", "message New")],
        )
        options = proto_compare.Options(aliases_a={"Old": "New"})
        self.assertEqual(
            proto_compare.compare(tree_a, tree_b, options),
            [("only in B", "New field 2 optional int32 y")],
        )

    def test_same_tree_has_no_differences(self):
        with tempfile.TemporaryDirectory() as directory:
            tree = proto_compare.parse_tree(write_tree(directory, "a", PROTO_A))
        self.assertEqual(proto_compare.compare(tree, tree), [])


class RepositoryTest(unittest.TestCase):
    def test_all_protos_of_this_repository_parse(self):
        tree = proto_compare.parse_tree(REPO / "protobuf" / "aap_protobuf")
        self.assertEqual(tree.warnings, [])
        self.assertIn("RadioProgramInfoNotification", tree.messages)
        self.assertEqual(tree.messages["RadioProgramInfoNotification"].fields[3].name, "is_favorite")
        self.assertEqual(proto_compare.compare(tree, tree), [])


class CommandLineTest(unittest.TestCase):
    def test_exit_status_and_output(self):
        import contextlib
        import io

        with tempfile.TemporaryDirectory() as directory:
            root_a = write_tree(directory, "a", "message M { optional int32 x = 1; }")
            root_b = write_tree(directory, "b", "message M { optional int32 x = 1; optional int32 y = 2; }")
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                self.assertEqual(proto_compare.main([str(root_a), str(root_b)]), 0)
                self.assertEqual(proto_compare.main([str(root_a), str(root_b), "--fail-on-diff"]), 1)
                self.assertEqual(proto_compare.main([str(root_a), str(root_a), "--fail-on-diff"]), 0)
        self.assertIn("Only in B:\n  M field 2 optional int32 y", output.getvalue())
        self.assertIn("No differences.", output.getvalue())


if __name__ == "__main__":
    unittest.main()
