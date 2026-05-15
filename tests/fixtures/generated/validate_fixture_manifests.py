#!/usr/bin/env python3
"""Validate generated TES4-family fixture manifests without invoking libbsa parsers."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

SUCCESS_MANIFESTS = ("tes4_v103", "tes4_v104", "tes4_v105")
SUCCESS_REQUIRED_TOP_LEVEL = {
    "variant",
    "version",
    "flags",
    "file_count",
    "folder",
    "provenance",
    "entries",
}
SUCCESS_REQUIRED_ENTRY_FIELDS = {
    "path",
    "original_path",
    "lookup_variants",
    "folder",
    "file",
    "hash",
    "record_flags",
    "compression",
    "raw_size",
    "stored_size",
    "offset",
    "has_embedded_name",
    "embedded_name_prefix_size",
    "embedded_name",
    "expected",
}
MALFORMED_CASE_IDS = {
    "unsupported_version",
    "truncated_header",
    "truncated_table",
    "duplicate_canonical_path",
    "corrupt_compressed_payload",
    "size_mismatch",
    "non_bsa_bytes",
}
EXPECTED_ERROR_CATEGORIES = {"unsupported", "format_error"}
COMPATIBILITY_MATRIX_REQUIRED_TOP_LEVEL = {"matrix_kind", "requirements", "rows"}
COMPATIBILITY_MATRIX_REQUIRED_ROW_FIELDS = {
    "id",
    "family",
    "category",
    "evidence_type",
    "phase",
    "expected_error",
}
COMPATIBILITY_MATRIX_REQUIRED_FAMILIES = {
    "tes3_bsa",
    "tes4_bsa",
    "ba2_gnrl",
    "ba2_dx10",
}
COMPATIBILITY_MATRIX_REQUIRED_CATEGORIES = {
    "truncated_structure",
    "duplicate_canonical_path",
    "invalid_payload_span",
    "unsupported_route",
    "decompression_failure",
    "oversized_arithmetic",
    "dds_chunk_layout",
}
COMPATIBILITY_MATRIX_PHASES = {"open", "extraction"}


def load_json(path: Path) -> dict[str, Any]:
    """Load a manifest as a JSON object and fail if the root shape is not an object."""
    with path.open("r", encoding="utf-8") as manifest_file:
        value = json.load(manifest_file)
    if not isinstance(value, dict):
        raise TypeError(f"{path.name}: manifest root must be an object")
    return value


def require_keys(mapping: dict[str, Any], required: set[str], label: str) -> None:
    """Assert that a JSON object contains the required field names."""
    missing = sorted(required.difference(mapping))
    if missing:
        raise AssertionError(f"{label}: missing required keys: {', '.join(missing)}")


def validate_success_manifest(archives_dir: Path, stem: str) -> None:
    """Validate one successful fixture manifest's metadata and entry expectation shape."""
    manifest = load_json(archives_dir / f"{stem}_manifest.json")
    require_keys(manifest, SUCCESS_REQUIRED_TOP_LEVEL, stem)
    if manifest["variant"] != stem:
        raise AssertionError(f"{stem}: variant does not match manifest filename")
    if manifest["file_count"] != len(manifest["entries"]):
        raise AssertionError(f"{stem}: file_count does not match entries length")
    require_keys(
        manifest["folder"],
        {"path", "original_path", "hash", "record_offset"},
        f"{stem}.folder",
    )
    require_keys(manifest["provenance"], {"generator", "source"}, f"{stem}.provenance")
    for index, entry in enumerate(manifest["entries"]):
        if not isinstance(entry, dict):
            raise TypeError(f"{stem}.entries[{index}]: entry must be an object")
        require_keys(entry, SUCCESS_REQUIRED_ENTRY_FIELDS, f"{stem}.entries[{index}]")
        require_keys(
            entry["expected"],
            {"bytes_hex", "fnv1a32"},
            f"{stem}.entries[{index}].expected",
        )
        if entry["path"] not in entry["lookup_variants"]:
            raise AssertionError(
                f"{stem}.entries[{index}]: canonical path must be a lookup variant"
            )


def validate_malformed_manifest(archives_dir: Path) -> None:
    """Validate malformed fixture case IDs, expected errors, and referenced archive files."""
    manifest = load_json(archives_dir / "malformed_manifest.json")
    require_keys(
        manifest,
        {"manifest_kind", "requirements", "threat_references", "provenance", "cases"},
        "malformed",
    )
    cases = manifest["cases"]
    if not isinstance(cases, list):
        raise TypeError("malformed: cases must be a list")
    seen_case_ids: set[str] = set()
    for index, case in enumerate(cases):
        if not isinstance(case, dict):
            raise TypeError(f"malformed.cases[{index}]: case must be an object")
        require_keys(
            case,
            {
                "id",
                "archive",
                "requirements",
                "threat_references",
                "expected_error",
                "phase",
                "description",
            },
            f"malformed.cases[{index}]",
        )
        seen_case_ids.add(case["id"])
        if case["expected_error"] not in EXPECTED_ERROR_CATEGORIES:
            raise AssertionError(
                f"malformed.cases[{index}]: unexpected error category {case['expected_error']!r}"
            )
        if not (archives_dir / case["archive"]).is_file():
            raise AssertionError(
                f"malformed.cases[{index}]: referenced archive is missing: {case['archive']}"
            )
    missing_cases = sorted(MALFORMED_CASE_IDS.difference(seen_case_ids))
    if missing_cases:
        raise AssertionError(f"malformed: missing case IDs: {', '.join(missing_cases)}")


def find_manifest_case(manifest: dict[str, Any], case_id: str) -> dict[str, Any]:
    """Return a manifest case by ID, failing if the manifest case list is malformed or missing the ID."""
    cases = manifest.get("cases")
    if not isinstance(cases, list):
        raise TypeError(
            "compatibility matrix: referenced manifest cases must be a list"
        )
    for case in cases:
        if not isinstance(case, dict):
            raise TypeError(
                "compatibility matrix: referenced manifest case must be an object"
            )
        if case.get("id") == case_id:
            return case
    raise AssertionError(
        f"compatibility matrix: referenced case_id is missing: {case_id}"
    )


def validate_compatibility_matrix(archives_dir: Path) -> None:
    """Validate the consolidated malformed matrix and its manifest/test evidence references."""
    matrix_path = archives_dir.parent / "compatibility_matrix.json"
    matrix = load_json(matrix_path)
    require_keys(
        matrix, COMPATIBILITY_MATRIX_REQUIRED_TOP_LEVEL, "compatibility_matrix"
    )
    if matrix["matrix_kind"] != "phase11_malformed_hardening":
        raise AssertionError(
            "compatibility_matrix: matrix_kind must be phase11_malformed_hardening"
        )
    if matrix["requirements"] != ["COMP-04", "COMP-05"]:
        raise AssertionError(
            "compatibility_matrix: requirements must be COMP-04 and COMP-05"
        )

    rows = matrix["rows"]
    if not isinstance(rows, list):
        raise TypeError("compatibility_matrix: rows must be a list")

    manifests: dict[str, dict[str, Any]] = {}
    observed_families: set[str] = set()
    observed_categories: set[str] = set()
    observed_test_evidence = False
    observed_manifest_evidence = False

    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise TypeError(
                f"compatibility_matrix.rows[{index}]: row must be an object"
            )
        require_keys(
            row,
            COMPATIBILITY_MATRIX_REQUIRED_ROW_FIELDS,
            f"compatibility_matrix.rows[{index}]",
        )

        family = row["family"]
        category = row["category"]
        expected_error = row["expected_error"]
        phase = row["phase"]
        evidence_type = row["evidence_type"]
        if family not in COMPATIBILITY_MATRIX_REQUIRED_FAMILIES:
            raise AssertionError(
                f"compatibility_matrix.rows[{index}]: unknown family {family!r}"
            )
        if category not in COMPATIBILITY_MATRIX_REQUIRED_CATEGORIES:
            raise AssertionError(
                f"compatibility_matrix.rows[{index}]: unknown category {category!r}"
            )
        if expected_error not in EXPECTED_ERROR_CATEGORIES:
            raise AssertionError(
                f"compatibility_matrix.rows[{index}]: unexpected error category {expected_error!r}"
            )
        if phase not in COMPATIBILITY_MATRIX_PHASES:
            raise AssertionError(
                f"compatibility_matrix.rows[{index}]: unknown phase {phase!r}"
            )

        observed_families.add(family)
        observed_categories.add(category)

        if evidence_type == "manifest":
            observed_manifest_evidence = True
            require_keys(
                row,
                {"archive", "manifest", "case_id"},
                f"compatibility_matrix.rows[{index}]",
            )
            if "test_file" in row or "test_name" in row:
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: manifest rows must not include test evidence"
                )
            if not (archives_dir / row["archive"]).is_file():
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: referenced archive is missing: {row['archive']}"
                )
            manifest_path = archives_dir / row["manifest"]
            if not manifest_path.is_file():
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: referenced manifest is missing: {row['manifest']}"
                )
            manifest = manifests.setdefault(row["manifest"], load_json(manifest_path))
            manifest_case = find_manifest_case(manifest, row["case_id"])
            if manifest_case.get("archive") != row["archive"]:
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: archive does not match manifest case"
                )
            if manifest_case.get("phase") != phase:
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: phase does not match manifest case"
                )
            if manifest_case.get("expected_error") != expected_error:
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: expected_error does not match manifest case"
                )
            continue

        if evidence_type == "test":
            observed_test_evidence = True
            require_keys(
                row, {"test_file", "test_name"}, f"compatibility_matrix.rows[{index}]"
            )
            if "archive" in row or "manifest" in row or "case_id" in row:
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: test rows must not include manifest evidence"
                )
            test_path = Path(row["test_file"])
            if not test_path.is_file():
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: referenced test file is missing: {row['test_file']}"
                )
            if row["test_name"] not in test_path.read_text(encoding="utf-8"):
                raise AssertionError(
                    f"compatibility_matrix.rows[{index}]: test_name token is missing from test file"
                )
            continue

        raise AssertionError(
            f"compatibility_matrix.rows[{index}]: unknown evidence_type {evidence_type!r}"
        )

    missing_families = sorted(
        COMPATIBILITY_MATRIX_REQUIRED_FAMILIES.difference(observed_families)
    )
    if missing_families:
        raise AssertionError(
            f"compatibility_matrix: missing families: {', '.join(missing_families)}"
        )
    missing_categories = sorted(
        COMPATIBILITY_MATRIX_REQUIRED_CATEGORIES.difference(observed_categories)
    )
    if missing_categories:
        raise AssertionError(
            f"compatibility_matrix: missing categories: {', '.join(missing_categories)}"
        )
    if not observed_manifest_evidence:
        raise AssertionError("compatibility_matrix: missing manifest-backed evidence")
    if not observed_test_evidence:
        raise AssertionError("compatibility_matrix: missing test-backed evidence")


def main(argv: list[str]) -> int:
    """Validate all generated manifest files under the supplied archive directory."""
    archives_dir = (
        Path(argv[1]) if len(argv) > 1 else Path("tests/fixtures/generated/archives")
    )
    for stem in SUCCESS_MANIFESTS:
        validate_success_manifest(archives_dir, stem)
    validate_malformed_manifest(archives_dir)
    validate_compatibility_matrix(archives_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
