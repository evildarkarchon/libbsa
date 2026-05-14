---
phase: 13-host-path-correctness-boundary
reviewed: 2026-05-14T00:02:04.9752033Z
depth: standard
files_reviewed: 36
files_reviewed_list:
  - CMakeLists.txt
  - src/archive.cpp
  - src/detail/host_file.cpp
  - src/detail/host_file.hpp
  - src/detail/host_file_path.cpp
  - src/detail/host_file_path.hpp
  - src/detail/writer_disk_source.cpp
  - src/detail/writer_disk_source.hpp
  - src/formats/ba2/ba2_dx10_parser.cpp
  - src/formats/ba2/ba2_dx10_parser.hpp
  - src/formats/ba2/ba2_dx10_prepare.cpp
  - src/formats/ba2/ba2_dx10_reader.cpp
  - src/formats/ba2/ba2_dx10_reader.hpp
  - src/formats/ba2/ba2_gnrl_parser.cpp
  - src/formats/ba2/ba2_gnrl_parser.hpp
  - src/formats/ba2/ba2_gnrl_prepare.cpp
  - src/formats/ba2/ba2_gnrl_reader.cpp
  - src/formats/ba2/ba2_gnrl_reader.hpp
  - src/formats/bsa/tes3_bsa_parser.cpp
  - src/formats/bsa/tes3_bsa_parser.hpp
  - src/formats/bsa/tes3_bsa_reader.cpp
  - src/formats/bsa/tes3_bsa_reader.hpp
  - src/formats/bsa/tes4_bsa_layout.cpp
  - src/formats/bsa/tes4_bsa_parser.cpp
  - src/formats/bsa/tes4_bsa_parser.hpp
  - src/formats/bsa/tes4_bsa_prepare.cpp
  - src/formats/bsa/tes4_bsa_reader.cpp
  - src/formats/bsa/tes4_bsa_reader.hpp
  - src/validation.cpp
  - tests/CMakeLists.txt
  - tests/unit/ba2_dx10_extraction_tests.cpp
  - tests/unit/ba2_gnrl_reader_tests.cpp
  - tests/unit/host_file_tests.cpp
  - tests/unit/host_file_writer_name_tests.cpp
  - tests/unit/host_path_correctness_boundary_tests.cpp
  - tests/unit/tes3_bsa_reader_tests.cpp
findings:
  critical: 1
  warning: 2
  info: 0
  total: 3
status: issues_found
---

# Phase 13: Code Review Report

**Reviewed:** 2026-05-14T00:02:04.9752033Z
**Depth:** standard
**Files Reviewed:** 36
**Status:** issues_found

## Summary

Phase 13 successfully moved the reader/parser side onto the new `host_file` seam, but the host-path boundary is not actually closed. The biggest problem is that several writer finalization paths still reopen disk sources through raw `std::string`-based `std::ifstream` calls, so non-ASCII `add_file(...)` sources can still fail after preparation. The new tests also leave important migrated paths unexecuted, and the new `original_utf8` state is currently dead data.

## Critical Issues

### CR-01: Writer-side host-path migration stops before finalization and dedupe

**File:** `src/formats/ba2/ba2_gnrl_layout.cpp:162-199`, `src/formats/ba2/ba2_gnrl_serialize.cpp:94-124`, `src/formats/bsa/tes3_bsa_serialize.cpp:71-104`, `src/formats/bsa/tes4_bsa_serialize.cpp:71-105`

**Issue:** Phase 13 rewired earlier writer stages onto `host_file` (`src/formats/ba2/ba2_gnrl_prepare.cpp:51-108`, `src/formats/bsa/tes4_bsa_prepare.cpp:171-225`, `src/formats/bsa/tes4_bsa_layout.cpp:57-152`), but later writer stages still reopen disk sources with raw `std::ifstream input{host_path, ...}`. That means a non-ASCII source path can pass validation/preparation and then still fail during dedupe comparison or archive publish, which defeats the stated host-path correctness boundary for `add_file(...)` workflows.

**Fix:** Carry a resolved host-path object all the way through prepared writer state and route every later reopen through `detail::open_host_file` / `detail::for_each_host_file_chunk` instead of raw `std::string` opens. For example:

```cpp
struct tes4_prepared_entry {
  // ...
  libbsa::detail::host_file_path raw_disk_host_path;
};

auto input = detail::open_host_file(entry.raw_disk_host_path, tes4_dedupe_source_context);
if (!input) {
  return input.error();
}
```

## Warnings

### WR-01: `original_utf8` is carried through the new seam but never affects behavior

**File:** `src/detail/host_file_path.hpp:11-21`, `src/detail/host_file.cpp:76-77`, `src/archive.cpp:33-34`

**Issue:** The new contract says it preserves the caller's original UTF-8 text for diagnostics, but every `host_file_path` overload immediately forwards only `host_path.resolved`. The added `original_utf8` field is stored in `archive_reader::state` and propagated across the new seam, yet the implementation never uses it to enrich an error message or any other observable behavior. That makes the comments misleading and leaves dead state in a hot path.

**Fix:** Either wire `original_utf8` into emitted diagnostics/logging or remove the field/comment until a real consumer exists. Minimal example:

```cpp
return error{
    error_code::io_error,
    std::string{context.open_error} + ": " + host_path.original_utf8};
```

### WR-02: The new tests miss the exact paths that can still regress

**File:** `tests/unit/host_path_correctness_boundary_tests.cpp:100-114`, `tests/unit/host_path_correctness_boundary_tests.cpp:257-274`, `tests/unit/host_file_writer_name_tests.cpp:25-177`

**Issue:** The new non-ASCII suite only exercises six TES4/BA2 reader fixtures and explicitly stays away from writer APIs; it also omits TES3 entirely. The companion `host_file_writer_name_tests.cpp` suite is mostly source-text inspection, not runtime execution. Because of that, Phase 13 can leave raw writer reopens in place and still look green.

**Fix:** Add fixture-backed runtime tests that:
- call `add_file(...)` and `write_to(...)` with non-ASCII UTF-8 host paths,
- round-trip TES3 as well as TES4/BA2 families,
- verify publish/dedupe paths, not just open/validate/extract,
- use `path.u8string()`/explicit UTF-8 conversion instead of `.string()` for non-ASCII cases.

---

_Reviewed: 2026-05-14T00:02:04.9752033Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
