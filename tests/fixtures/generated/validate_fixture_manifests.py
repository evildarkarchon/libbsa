#!/usr/bin/env python3
"""Validate generated TES4-family fixture manifests without invoking libbsa parsers."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any


SUCCESS_MANIFESTS = ("tes4_v103", "tes4_v104", "tes4_v105")
SUCCESS_REQUIRED_TOP_LEVEL = {
    "manifest_kind",
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


def load_json(path: Path) -> dict[str, Any]:
    """Load a manifest as a JSON object and fail if the root shape is not an object."""
    with path.open("r", encoding="utf-8") as manifest_file:
        value = json.load(manifest_file)
    if not isinstance(value, dict):
        raise AssertionError(f"{path.name}: manifest root must be an object")
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
    require_keys(manifest["folder"], {"path", "original_path", "hash", "record_offset"}, f"{stem}.folder")
    require_keys(manifest["provenance"], {"generator", "source"}, f"{stem}.provenance")
    for index, entry in enumerate(manifest["entries"]):
        if not isinstance(entry, dict):
            raise AssertionError(f"{stem}.entries[{index}]: entry must be an object")
        require_keys(entry, SUCCESS_REQUIRED_ENTRY_FIELDS, f"{stem}.entries[{index}]")
        require_keys(entry["expected"], {"bytes_hex", "fnv1a32"}, f"{stem}.entries[{index}].expected")
        if entry["path"] not in entry["lookup_variants"]:
            raise AssertionError(f"{stem}.entries[{index}]: canonical path must be a lookup variant")


def validate_malformed_manifest(archives_dir: Path) -> None:
    """Validate malformed fixture case IDs, expected errors, and referenced archive files."""
    manifest = load_json(archives_dir / "malformed_manifest.json")
    require_keys(manifest, {"manifest_kind", "requirements", "threat_references", "provenance", "cases"}, "malformed")
    cases = manifest["cases"]
    if not isinstance(cases, list):
        raise AssertionError("malformed: cases must be a list")
    seen_case_ids: set[str] = set()
    for index, case in enumerate(cases):
        if not isinstance(case, dict):
            raise AssertionError(f"malformed.cases[{index}]: case must be an object")
        require_keys(
            case,
            {"id", "archive", "requirements", "threat_references", "expected_error", "phase", "description"},
            f"malformed.cases[{index}]",
        )
        seen_case_ids.add(case["id"])
        if case["expected_error"] not in EXPECTED_ERROR_CATEGORIES:
            raise AssertionError(f"malformed.cases[{index}]: unexpected error category {case['expected_error']!r}")
        if not (archives_dir / case["archive"]).is_file():
            raise AssertionError(f"malformed.cases[{index}]: referenced archive is missing: {case['archive']}")
    missing_cases = sorted(MALFORMED_CASE_IDS.difference(seen_case_ids))
    if missing_cases:
        raise AssertionError(f"malformed: missing case IDs: {', '.join(missing_cases)}")


def main(argv: list[str]) -> int:
    """Validate all generated manifest files under the supplied archive directory."""
    archives_dir = Path(argv[1]) if len(argv) > 1 else Path("tests/fixtures/generated/archives")
    for stem in SUCCESS_MANIFESTS:
        validate_success_manifest(archives_dir, stem)
    validate_malformed_manifest(archives_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
