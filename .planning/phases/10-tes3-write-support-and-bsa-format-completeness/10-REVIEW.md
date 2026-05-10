---
phase: 10-tes3-write-support-and-bsa-format-completeness
reviewed: 2026-05-09T12:30:00Z
depth: standard
auto_rereview_iteration: final
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

**Reviewed:** 2026-05-09T12:30:00Z
**Depth:** standard
**Files Reviewed:** 11
**Status:** issues_found

## Summary

Reviewed the final auto-fix state for the TES3 writer public API, atomic publish/replace helpers, TES3 writer implementation, build/test wiring, generated fixture manifest/generator, and unit tests. The previous overwrite-mode gap finding is resolved: overwrite now routes through `replace_file_atomically`, and no-replace publishing still uses an atomic destination-exists failure path.

The final atomic replace fix did not reintroduce the prior backup-stranding issue. However, one remaining production correctness/security issue is still present in the host-path boundary, and one test reliability defect can cause false failures in reused temp directories.

## Critical Issues

### CR-01: Host paths accept embedded NUL bytes and can target a different filesystem path

**Classification:** BLOCKER
**File:** `src/formats/bsa/tes3_bsa_writer.cpp:61-64,372-376`
**Issue:** `add_file` rejects only an empty disk source host path, and `write_tes3_bsa_archive` rejects only an empty output host path before constructing `std::filesystem::path`. Unlike archive-internal paths, host paths containing `\0` are not rejected. Native filesystem APIs treat NUL as a terminator, so a caller-provided `"safe.bsa\0suffix"` or source path with a NUL can be interpreted as `"safe.bsa"` by the OS. With `overwrite_existing = true`, this can overwrite an unintended host file; for disk sources, it can read a different file than the caller-visible string indicates.
**Fix:** Validate every public host-path `std::string_view` before converting it to `std::filesystem::path` or storing it for later I/O. Apply the same rule to source and output host paths, and add regression tests for `add_file("valid/path.nif", "source\0suffix")` and `write_to("archive.bsa\0suffix")`.

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
**Issue:** `writer_test_dir()` always returns the same directory and `output_path()` always uses fixed filenames. Several tests depend on a destination not already existing so they can assert validation-specific error codes, for example duplicate canonical paths and empty archives. If a previous interrupted run, parallel process, or local debugging leaves one of those fixed files behind, `write_to` checks destination existence first and returns `io_error` instead of the expected `format_error` or `invalid_argument`, causing false failures unrelated to the behavior under test.
**Fix:** Give each test case a unique subdirectory or explicitly remove the destination before assertions that depend on non-existence.

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

_Reviewed: 2026-05-09T12:30:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
