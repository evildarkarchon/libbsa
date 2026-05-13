---
phase: 13-host-path-correctness-boundary
reviewed: 2026-05-13T00:00:00Z
depth: standard
files_reviewed: 33
files_reviewed_list:
  - src/detail/host_path.hpp
  - src/detail/host_path.cpp
  - src/detail/host_file.hpp
  - src/detail/host_file.cpp
  - src/archive.cpp
  - src/validation.cpp
  - src/formats/bsa/tes3_bsa_parser.hpp
  - src/formats/bsa/tes3_bsa_parser.cpp
  - src/formats/bsa/tes4_bsa_parser.hpp
  - src/formats/bsa/tes4_bsa_parser.cpp
  - src/formats/ba2/ba2_gnrl_parser.hpp
  - src/formats/ba2/ba2_gnrl_parser.cpp
  - src/formats/ba2/ba2_dx10_parser.hpp
  - src/formats/ba2/ba2_dx10_parser.cpp
  - src/formats/bsa/tes3_bsa_reader.hpp
  - src/formats/bsa/tes3_bsa_reader.cpp
  - src/formats/bsa/tes4_bsa_reader.hpp
  - src/formats/bsa/tes4_bsa_reader.cpp
  - src/formats/ba2/ba2_gnrl_reader.hpp
  - src/formats/ba2/ba2_gnrl_reader.cpp
  - src/formats/ba2/ba2_dx10_reader.hpp
  - src/formats/ba2/ba2_dx10_reader.cpp
  - src/formats/bsa/tes4_bsa_prepare.cpp
  - src/formats/bsa/tes4_bsa_layout.cpp
  - src/formats/ba2/ba2_gnrl_prepare.cpp
  - src/formats/ba2/ba2_dx10_prepare.cpp
  - tests/unit/host_file_tests.cpp
  - tests/unit/host_path_correctness_boundary_tests.cpp
  - tests/unit/tes3_bsa_reader_tests.cpp
  - tests/unit/tes4_bsa_reader_tests.cpp
  - tests/unit/ba2_gnrl_reader_tests.cpp
  - tests/unit/ba2_dx10_extraction_tests.cpp
  - tests/CMakeLists.txt
findings:
  critical: 1
  warning: 3
  info: 0
  total: 4
status: issues_found
---

# Phase 13: Code Review Report

**Reviewed:** 2026-05-13T00:00:00Z
**Depth:** standard
**Files Reviewed:** 33
**Status:** issues_found

## Summary

The host-path read path is materially improved, but the submitted scope still has one correctness blocker and several robustness gaps. The largest problem is that `archive_reader` only remembers a path string and reopens the archive later for extraction, so metadata can be parsed from one file and payload bytes read from a different on-disk file if the path is replaced after `open()`.

## Critical Issues

### CR-01: `archive_reader` is vulnerable to stale-metadata / changed-file extraction

**File:** `src/archive.cpp:29-34`, `src/archive.cpp:87-99`, `src/archive.cpp:258`, `src/archive.cpp:281`, `src/archive.cpp:351`, `src/formats/bsa/tes3_bsa_reader.cpp:40-67`, `src/formats/bsa/tes4_bsa_reader.cpp:113-117`, `src/formats/ba2/ba2_gnrl_reader.cpp:27-32`, `src/formats/ba2/ba2_gnrl_reader.cpp:55-68`, `src/formats/ba2/ba2_dx10_reader.cpp:113-135`

**Issue:** `archive_reader::open()` parses metadata once, stores only `host_file_path`, and every later extract path reopens the archive by filename. If the archive file is replaced or edited after `open()`, extraction runs against the new file while still trusting offsets, sizes, and compression metadata from the old one. That can silently return wrong payload bytes, fail nondeterministically, or validate a different file than the one originally opened.

**Fix:** Keep a stable archive identity for the reader state instead of only a path. On Windows that can be a long-lived file handle or a captured file ID + size tuple that is revalidated before every extract.

```cpp
struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  detail::host_file_path host_path;
  std::uint64_t opened_size{};
  detail::stable_file_id file_id{};
  bool is_ba2_dx10{false};
};

auto current = detail::inspect_host_file_identity(state_->host_path, archive_open_context);
if (!current || current->size != state_->opened_size || current->id != state_->file_id) {
  return error{error_code::io_error, "archive host path changed after open"};
}
```

## Warnings

### WR-01: TES4 writer validation still bypasses UTF-8 host-path resolution

**File:** `src/formats/bsa/tes4_bsa_prepare.cpp:470-475`

**Issue:** `tes4_validate_entries()` still probes disk sources with `std::ifstream input{entry.host_path, ...}`. On Windows that narrow-string overload uses the active code page, not the new UTF-8 host-path resolver. A TES4 writer entry backed by a non-ASCII disk path can therefore fail validation before it ever reaches `resolve_host_file_path()`.

**Fix:** Validate existence/openability through the resolved `std::filesystem::path` helper path used everywhere else.

```cpp
auto resolved = detail::resolve_host_file_path(entry.host_path, tes4_prepare_source_context.open_error);
if (!resolved) {
  return resolved.error();
}
auto input = detail::open_host_file(resolved.value(), tes4_prepare_source_context);
if (!input) {
  return input.error();
}
```

### WR-02: BA2 GNRL writer validation has the same UTF-8 regression

**File:** `src/formats/ba2/ba2_gnrl_prepare.cpp:302-307`

**Issue:** `ba2_gnrl_validate_entries()` also opens `entry.host_path` directly with narrow `std::ifstream`. That means the new non-ASCII host-path support is still broken for BA2 GNRL disk-backed writer inputs even though the later preparation path was converted to `resolve_host_file_path()`.

**Fix:** Route validation through `resolve_host_file_path()` / `open_host_file()` exactly like the later read path.

```cpp
auto resolved = detail::resolve_host_file_path(entry.host_path, ba2_gnrl_prepare_source_context.open_error);
if (!resolved) {
  return resolved.error();
}
auto input = detail::open_host_file(resolved.value(), ba2_gnrl_prepare_source_context);
if (!input) {
  return input.error();
}
```

### WR-03: `validate_archive()` throws away parser-specific open diagnostics

**File:** `src/validation.cpp:49-52`

**Issue:** `report_from_open_error()` converts a rich `error` into a generic message based only on `error_code`. For malformed archives this erases the concrete reason found by the parser, making validation output much less actionable and hiding the exact failure from callers.

**Fix:** Preserve the original parser/open message when converting to a `validation_report`.

```cpp
validation_report report_from_open_error(const error& err) {
  validation_report report;
  append_fatal(report, err);
  return report;
}
```

### WR-04: New host-path tests miss the writer code paths that still regress on Unicode disk sources

**File:** `tests/unit/host_path_correctness_boundary_tests.cpp:156-169`

**Issue:** The added boundary suite only exercises `open()`, `validate_archive()`, and extraction against copied archives. It never covers TES4 or BA2 GNRL disk-backed writer entry validation, which is why the raw-`ifstream` UTF-8 regressions above shipped inside the same phase.

**Fix:** Add at least one non-ASCII temp-source writer case for each disk-backed writer path changed in scope and run it through the public writer API, not helper-only coverage.

```cpp
TEST_CASE("tes4_writer accepts non-ASCII disk source paths", "[unit][host_path]") {
  auto source = make_non_ascii_temp_file("meshes/source-Ångström.nif", payload);
  auto entry = libbsa::tes4_make_writer_entry("meshes/source.nif");
  entry.host_path = utf8_string_from_path(source);
  auto written = writer.add_file(entry);
  REQUIRE(written.has_value());
}
```

---

_Reviewed: 2026-05-13T00:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
