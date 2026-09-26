"""Tests for narrowly reviewed, identity-bound native-oracle failures."""

import copy
import importlib.util
from pathlib import Path
import unittest


MODULE = Path(__file__).with_name("oracle_failures.py")
spec = importlib.util.spec_from_file_location("oracle_failures", MODULE)
failures = importlib.util.module_from_spec(spec)
spec.loader.exec_module(failures)


def registry():
    """Return one exact approved failure with an explicitly bounded affected entry."""
    return {"schema": 1, "failures": [{
        "id": "fallout-misc-one-entry",
        "review": {"status": "approved", "gate_qualifies": True,
                   "reviewed_by": "user", "reason": "Reviewed known oracle limitation."},
        "oracle_sha256": "a" * 64, "archive_sha256": "b" * 64,
        "archive_path": "Fallout 3/Fallout - Misc.bsa", "phase": "retail-unpack",
        "exit_code": 1, "allowed_options": [["-mt:yes"], ["-mt:no"]],
        "diagnostics": ['Error unpacking file "menus/s.txt": size mismatch.'],
        "entries": [{"path": "menus/s.txt", "decoded_size": 4,
                     "decoded_sha256": "c" * 64,
                     "fingerprint_provenance": "Pinned libbsa observation; regression guard only, not independent oracle proof"}],
    }]}


class OracleFailureTests(unittest.TestCase):
    """Reject evidence that broadens an approval beyond its reviewed exact failure."""

    def match(self, source=None, **overrides):
        """Evaluate a complete native-process observation with explicit changed fields."""
        source = registry() if source is None else source
        args = {"oracle_sha256": "a" * 64, "archive_sha256": "b" * 64,
                "options": ["-mt:yes"], "exit_code": 1,
                "log_text": 'BSArch v1.0\nError unpacking file "menus/s.txt": size mismatch.\n'}
        args.update(overrides)
        return failures.match_failure(source, **args)

    def test_exact_reviewed_failure_returns_its_entry_scoped_binding(self):
        """A qualifying match retains the reviewed entry evidence, not just a boolean."""
        approved = registry()
        self.assertIsNone(failures.validate_registry(approved))
        self.assertEqual(approved["failures"][0], self.match(approved))

    def test_match_does_not_allow_callers_to_mutate_registry(self):
        """Report annotation cannot silently change the approval used by later cases."""
        source = registry()
        matched = self.match(source)
        matched["entries"][0]["decoded_size"] = 900
        self.assertEqual(4, source["failures"][0]["entries"][0]["decoded_size"])

    def test_wrong_tool_or_archive_cannot_reuse_approval(self):
        """Neither a changed BSArch build nor a different retail file is covered."""
        for field in ("oracle_sha256", "archive_sha256"):
            with self.subTest(field=field):
                self.assertIsNone(self.match(**{field: "d" * 64}))

    def test_only_exact_explicit_command_options_match(self):
        """Additional switches and options represented as command strings are not equivalent."""
        self.assertIsNotNone(self.match(options=["-mt:no"]))
        for options in ([], ["-mt:yes", "-q"], ["-MT:YES"], "-mt:yes", None):
            with self.subTest(options=options):
                self.assertIsNone(self.match(options=options))

    def test_option_order_is_bound(self):
        """Reordering arguments is not authorized by a different saved command."""
        source = registry()
        source["failures"][0]["allowed_options"] = [["-mt:yes", "-q"]]
        self.assertIsNotNone(self.match(source, options=["-mt:yes", "-q"]))
        self.assertIsNone(self.match(source, options=["-q", "-mt:yes"]))

    def test_timeout_crash_and_success_exit_codes_never_match(self):
        """A timeout has no ordinary exit code and cannot be treated as the reviewed failure."""
        for code in (None, 0, 2, -1, 0xC0000005, True, "1"):
            with self.subTest(code=code):
                self.assertIsNone(self.match(exit_code=code))

    def test_unreviewed_and_nonqualifying_records_never_match(self):
        """Pending, rejected and advisory-only approvals cannot qualify release evidence."""
        for status, qualifies in (("pending", True), ("rejected", True), ("approved", False)):
            source = registry()
            source["failures"][0]["review"].update(status=status, gate_qualifies=qualifies)
            with self.subTest(status=status, qualifies=qualifies):
                self.assertIsNone(self.match(source))

    def test_pending_record_can_await_a_reviewer_without_qualifying(self):
        """A saved pending review remains usable as a backlog record, never an approval."""
        source = registry()
        source["failures"][0]["review"].update(status="pending", gate_qualifies=False, reviewed_by="")
        self.assertIsNone(failures.validate_registry(source))
        self.assertIsNone(self.match(source))

    def test_changed_entry_name_is_not_a_fuzzy_diagnostic_match(self):
        """An otherwise identical error involving another file is a new failure."""
        self.assertIsNone(self.match(log_text='Error unpacking file "menus/t.txt": size mismatch.'))

    def test_error_line_case_and_whitespace_are_exact(self):
        """Message normalization must not broaden reviewed diagnostic text."""
        error = registry()["failures"][0]["diagnostics"][0]
        for changed in (error.lower(), " " + error, error + " ", error.replace("size", "Size")):
            with self.subTest(changed=changed):
                self.assertIsNone(self.match(log_text=changed))

    def test_each_expected_diagnostic_occurs_exactly_once(self):
        """Repeated errors are a changed failure even when their strings match."""
        error = registry()["failures"][0]["diagnostics"][0]
        self.assertIsNone(self.match(log_text=error + "\n" + error))
        self.assertIsNone(self.match(log_text="BSArch banner only"))

    def test_extra_errors_exceptions_or_fatal_lines_fail_closed(self):
        """A known error cannot hide another fatal or per-file diagnostic."""
        error = registry()["failures"][0]["diagnostics"][0]
        for extra in ("Error unpacking another file", "Exception: bad data", "Fatal corruption",
                      "fatal: another failure", "  ERROR another file", "EReadError: bad stream"):
            with self.subTest(extra=extra):
                self.assertIsNone(self.match(log_text=error + "\n" + extra))

    def test_carriage_return_progress_preserves_exact_error_lines(self):
        """CR progress updates split into records without fuzzy stripping of errors."""
        error = registry()["failures"][0]["diagnostics"][0]
        self.assertIsNotNone(self.match(log_text="BSArch\r\n[10%]\r[20%]\r" + error + "\r\n[100%]\r"))

    def test_multiple_eligible_records_are_ambiguous(self):
        """Two approvals for identical runtime evidence must be resolved explicitly."""
        source = registry()
        other = copy.deepcopy(source["failures"][0])
        other["id"] = "second-review"
        source["failures"].append(other)
        self.assertIsNone(self.match(source))

    def test_all_expected_diagnostics_are_required(self):
        """An approval for two failures does not implicitly cover only one of them."""
        source = registry()
        other = 'Error unpacking file "menus/t.txt": size mismatch.'
        source["failures"][0]["diagnostics"].append(other)
        source["failures"][0]["entries"].append({"path": "menus/t.txt", "decoded_size": 8})
        self.assertIsNone(self.match(source))
        self.assertIsNotNone(self.match(source, log_text="\n".join(source["failures"][0]["diagnostics"])))

    def test_schema_and_review_fields_are_validated(self):
        """Malformed registry structure is explicit and cannot produce an approval."""
        invalid = []
        for field, value in (("schema", 2), ("schema", True), ("failures", {})):
            item = registry()
            item[field] = value
            invalid.append(item)
        for field, value in (("oracle_sha256", "bad"), ("archive_sha256", "B" * 64),
                             ("phase", "synthetic-unpack"), ("exit_code", 0),
                             ("exit_code", True), ("allowed_options", []),
                             ("diagnostics", []), ("entries", [])):
            item = registry()
            item["failures"][0][field] = value
            invalid.append(item)
        for field, value in (("status", "unknown"), ("gate_qualifies", "yes"),
                             ("reviewed_by", ""), ("reason", "")):
            item = registry()
            item["failures"][0]["review"][field] = value
            invalid.append(item)
        for item in invalid:
            with self.subTest(item=item), self.assertRaises(ValueError):
                failures.validate_registry(item)
            self.assertIsNone(self.match(item))

    def test_missing_required_fields_and_duplicate_ids_fail(self):
        """Partial registry entries and duplicated identifiers have no default approval."""
        for field in ("id", "review", "oracle_sha256", "archive_sha256", "archive_path",
                      "phase", "exit_code", "allowed_options", "diagnostics", "entries"):
            source = registry()
            del source["failures"][0][field]
            with self.subTest(field=field), self.assertRaises(ValueError):
                failures.validate_registry(source)
        source = registry()
        source["failures"].append(copy.deepcopy(source["failures"][0]))
        with self.assertRaises(ValueError):
            failures.validate_registry(source)

    def test_entry_bindings_require_unique_safe_paths_and_exact_sizes(self):
        """A blanket archive waiver or colliding affected-entry list cannot be enrolled."""
        for entry in ({"path": "", "decoded_size": 4}, {"path": "../a", "decoded_size": 4},
                      {"path": "menus/s.txt", "decoded_size": -1},
                      {"path": "menus/s.txt", "decoded_size": True},
                      {"path": "menus/s.txt"},
                      {"path": "menus/s.txt", "decoded_size": 4, "decoded_sha256": "wrong"}):
            source = registry()
            source["failures"][0]["entries"] = [entry]
            with self.subTest(entry=entry), self.assertRaises(ValueError):
                failures.validate_registry(source)
        source = registry()
        source["failures"][0]["entries"].append({"path": "MENUS\\s.txt", "decoded_size": 4})
        with self.assertRaises(ValueError):
            failures.validate_registry(source)

    def test_duplicate_or_multiline_expected_diagnostics_fail_validation(self):
        """An expected diagnostic is one unique exact error line, not a log pattern."""
        for diagnostics in (["Error one", "Error one"], ["Error one\nError two"],
                            [""], ["normal progress only"]):
            source = registry()
            source["failures"][0]["diagnostics"] = diagnostics
            with self.subTest(diagnostics=diagnostics), self.assertRaises(ValueError):
                failures.validate_registry(source)

    def test_empty_registry_is_valid_and_never_matches(self):
        """A fresh checkout may have no enrolled exceptions without disabling comparisons."""
        empty = {"schema": 1, "failures": []}
        self.assertIsNone(failures.validate_registry(empty))
        self.assertIsNone(self.match(empty))


if __name__ == "__main__":
    unittest.main()
