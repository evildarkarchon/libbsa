---
phase: 10-tes3-write-support-and-bsa-format-completeness
reviewed: 2026-05-09T13:15:00Z
depth: standard
files_reviewed: 11
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/writer.hpp
  - src/detail/atomic_file_ops.hpp
  - src/formats/bsa/tes3_bsa_writer.cpp
  - src/formats/bsa/tes3_bsa_writer.hpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/archives/tes3_writer_canonical.bsa
  - tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json
  - tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp
  - tests/unit/public_include_boundary_tests.cpp
  - tests/unit/tes3_bsa_writer_tests.cpp
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
status: issues_found
---

# Phase 10: Code Review Report

**Reviewed:** 2026-05-09T13:15:00Z
**Depth:** standard
**Files Reviewed:** 11
**Status:** issues_found

## Summary

Reviewed the listed TES3 writer public API, atomic publish helpers, TES3 writer implementation, build/test wiring, generated fixture manifest/generator, and unit tests. The two open findings from the previous review are still present: public host paths still accept embedded NUL bytes, and tests still reuse deterministic temp filenames without cleanup. I did not find additional source-level regressions in the recent fixes.

## Critical Issues

### CR-01: Host paths accept embedded NUL bytes and can target a different filesystem path

**Classification:** BLOCKER
**File:** `src/formats/bsa/tes3_bsa_writer.cpp:61-64,372-376`
**Issue:** `add_file` rejects only an empty disk source host path, and `write_tes3_bsa_archive` rejects only an empty output host path before storing or converting host-path strings. Archive-internal paths explicitly reject NUL bytes, but source and output host paths do not. Native filesystem APIs treat NUL as a terminator at the C-string boundary, so caller input such as `"safe.bsa\0suffix"` or a disk source path with an embedded NUL can be interpreted as a different host path than the `std::string_view` appears to name. With `overwrite_existing = true`, this can overwrite an unintended host file; for disk sources, it can read an unintended file.
**Fix:** Validate all public host paths before storing them or constructing `std::filesystem::path`, and add regression tests for both source and output paths containing embedded NUL bytes.

```cpp
result<void> validate_host_path(std::string_view host_path, std::string_view description) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, std::string{description} + " must not be empty"};
  }
  if (host_path.find('\0') != std::string_view::npos) {
    return error{error_code::invalid_argument, std::string{description} + " must not contain NUL bytes"};
  }
  return {};
}
```

## Warnings

### WR-01: TES3 writer tests reuse deterministic temp filenames without cleanup

**Classification:** WARNING
**File:** `tests/unit/tes3_bsa_writer_tests.cpp:24-30,531-548`
**Issue:** `writer_test_dir()` always returns the same temp directory, and `output_path()` always returns fixed filenames under that directory. Several tests expect the destination not to exist so they can assert validation-specific failures, such as duplicate canonical paths returning `format_error` and empty archives returning `invalid_argument`. If an interrupted run, parallel process, or local debugging leaves one of those files behind, `write_to` checks destination existence first and returns `io_error`, causing false failures unrelated to the behavior under test.
**Fix:** Give each test case a unique subdirectory, or remove the destination before assertions that depend on a non-existing output path.

```cpp
std::filesystem::path output_path(std::string name) {
  static std::atomic_uint64_t counter{0};
  auto path = std::filesystem::temp_directory_path() / "libbsa_tes3_bsa_writer_tests" /
              std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
  std::filesystem::create_directories(path);
  return path / std::move(name);
}
```

---

_Reviewed: 2026-05-09T13:15:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
