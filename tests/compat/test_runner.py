"""Behavioral checks that prevent false-green compatibility verdicts."""
import tempfile
import copy
import unittest
from pathlib import Path

import runner


class RunnerContracts(unittest.TestCase):
    """Exercise missing coverage, stale identity, and false-green boundaries."""

    def test_recursive_discovery_includes_nested_and_uppercase_archives(self):
        """Dropping nested archives must lose coverage visibly."""
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "game").mkdir()
            (root / "game" / "retail.BSA").write_bytes(b"archive")
            (root / "retail.ba2").write_bytes(b"archive")
            (root / "notes.txt").write_text("not an archive")
            self.assertEqual(len(runner.discover(root)), 2)

    def test_missing_and_modified_baseline_archives_fail(self):
        """A reduced or replaced corpus cannot silently qualify a release."""
        baseline = {"archives": [{"path": "a.bsa", "sha256": "aaa"},
                                  {"path": "b.ba2", "sha256": "bbb"}]}
        actual = [{"path": "b.ba2", "sha256": "changed"}]
        self.assertEqual(len(runner.verify_baseline(baseline, actual)), 2)
        self.assertEqual(runner.verify_baseline(baseline, baseline["archives"]), [])

    def test_cache_identity_invalidates_all_evidence_inputs(self):
        """An oracle or comparison change must not reuse stale evidence."""
        original = runner.cache_identity("a", "b", ["-mt:no"], 1)
        for args in [("x", "b", ["-mt:no"], 1), ("a", "x", ["-mt:no"], 1),
                     ("a", "b", ["-mt:yes"], 1), ("a", "b", ["-mt:no"], 2)]:
            self.assertNotEqual(original, runner.cache_identity(*args))

    def test_release_rejects_missing_failed_or_dirty_evidence(self):
        """No results, skipped work, or dirty source can be a release pass."""
        for report in [{}, {"source": {"clean": False}, "cases": []},
                       {"source": {"clean": True, "commit": "abc"},
                        "cases": [{"status": "failed"}]}]:
            self.assertTrue(runner.release_errors(report))

    def test_release_reconstructs_required_cases_instead_of_trusting_count(self):
        """A truncated report cannot choose its own smaller definition of complete."""
        from synthetic import cases
        baseline_path = runner.HERE / "retail-baseline.json"
        baseline = runner.read_json(baseline_path)
        results = [{"kind": "synthetic", "id": case["id"], "status": "passed"} for case in cases()]
        results += [{"kind": "retail", "id": archive["path"], "status": "passed"}
                    for archive in baseline["archives"]]
        report = {"schema": runner.SCHEMA, "mode": "thorough", "finished": 1,
                  "status": "passed", "source": {"clean": True, "commit": "abc"},
                  "build": {"configuration": "Release"}, "oracle": {"sha256": "oracle"},
                  "oracle_failures_sha256": runner.sha256(runner.HERE / "oracle-failures.json"),
                  "baseline_sha256": runner.sha256(baseline_path), "corpus": baseline["archives"],
                  "expected_cases": len(results), "cases": results}
        self.assertEqual(runner.release_errors(report), [])
        report["cases"] = results[:-1]
        report["expected_cases"] -= 1
        self.assertTrue(runner.release_errors(report))
        report["cases"] = results[:-1] + [results[0]]
        report["expected_cases"] += 1
        self.assertTrue(runner.release_errors(report))

    def test_reviewed_oracle_failure_can_qualify_without_hiding_other_gaps(self):
        """Only the approved entry gap may qualify; incomplete comparisons still fail."""
        from synthetic import cases
        registry_path = runner.HERE / "oracle-failures.json"
        failure = runner.read_json(registry_path)["failures"][0]
        baseline_path = runner.HERE / "retail-baseline.json"
        corpus = runner.read_json(baseline_path)["archives"]
        results = [{"kind": "synthetic", "id": c["id"], "status": "passed"} for c in cases()]
        results += [{"kind": "retail", "id": a["path"], "status": "passed"} for a in corpus]
        waived = next(c for c in results if c["id"] == failure["archive_path"])
        record = next(a for a in corpus if a["path"] == failure["archive_path"])
        count = record["profile"]["entry_count"]
        exception = {key: copy.deepcopy(failure[key]) for key in
                     ("id", "oracle_sha256", "archive_sha256", "entries", "exit_code", "diagnostics")}
        exception["options"] = ["-mt:yes"]
        exception["original_content_verified"] = False
        waived.update(status="accepted_oracle_failure", entries=count,
                      oracle_compared_entries=count - 1, repack_verified_entries=count,
                      oracle_exceptions=[exception])
        report = {"schema": runner.SCHEMA, "mode": "thorough", "finished": 1,
                  "status": "passed_with_exceptions", "source": {"clean": True, "commit": "abc"},
                  "build": {"configuration": "Release"}, "oracle": {"sha256": failure["oracle_sha256"]},
                  "baseline_sha256": runner.sha256(baseline_path), "corpus": corpus,
                  "oracle_failures_sha256": runner.sha256(registry_path),
                  "expected_cases": len(results), "cases": results}
        self.assertEqual(runner.release_errors(report), [])
        for mutation in (lambda r: r.update(oracle_failures_sha256="stale approval"),
                         lambda r: r["oracle"].update(sha256="changed tool"),
                         lambda r: next(c for c in r["cases"] if c["id"] == waived["id"]).update(
                             repack_verified_entries=count - 1),
                         lambda r: next(c for c in r["cases"] if c["id"] == waived["id"]).update(
                             oracle_compared_entries=count - 2),
                         lambda r: next(c for c in r["cases"] if c["id"] == waived["id"]).update(
                             status="passed")):
            modified = copy.deepcopy(report)
            mutation(modified)
            self.assertTrue(runner.release_errors(modified))


if __name__ == "__main__":
    unittest.main()
