---
phase: 260515-95h-codex-review-reject-overlapping-tes4-pay
verified: 2026-05-15T13:49:00Z
status: passed
score: 4/4 must-haves verified
overrides_applied: 0
---

# Quick Task 260515-95h Verification Report

**Task Goal:** Codex Review: Reject overlapping TES4 payload spans and embedded NULs in public host paths.
**Verified:** 2026-05-15T13:49:00Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | TES4-family BSA parsing rejects non-empty partially overlapping payload ranges before exposing entry metadata. | ✓ VERIFIED | `src/formats/bsa/tes4_bsa_parser.cpp:99-124` obtains `make_tes4_bsa_payload_descriptor` before `entries.push_back` and rejects non-exact overlaps with `format_error` message `TES4 BSA entry payload spans partially overlap`; `tests/unit/tes4_bsa_reader_tests.cpp:445-468` verifies both `archive_reader::open` and `validate_archive` fail. |
| 2 | TES4-family BSA parsing still accepts exact duplicate non-empty payload ranges as valid writer dedupe. | ✓ VERIFIED | `src/formats/bsa/tes4_bsa_parser.cpp:112-123` computes `exact_duplicate` and skips overlap rejection for equal offset/size; `tests/unit/tes4_bsa_reader_tests.cpp:470-500` verifies open succeeds and both entries expose identical `payload_offset` and `stored_size`. |
| 3 | Public UTF-8 host path resolution rejects embedded NUL bytes before native Windows path conversion or file I/O. | ✓ VERIFIED | `src/detail/host_file_path.cpp:17-22` checks `host_path.find('\0')` before copying, `MultiByteToWideChar`, or `std::filesystem::path` construction; `tests/unit/host_file_path_tests.cpp:103-110` passes explicit-length `std::string_view{"prefix\0suffix", 13U}` and expects `invalid_argument`. |
| 4 | No changes are made under `TES5Edit/` and no new dependencies are introduced. | ✓ VERIFIED | `git status --short` shows only the quick-task planning directory untracked; implementation files are under `src/` and `tests/`, and no dependency manifest files are changed. |

**Score:** 4/4 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/formats/bsa/tes4_bsa_parser.cpp` | TES4 stored-payload span tracking in `materialize_entries` | ✓ VERIFIED | Defines `stored_payload_span`, `spans_overlap_u64`, reserves `accepted_payload_spans`, skips zero sizes, accepts exact duplicates, rejects partial overlaps before `entries.push_back`. |
| `tests/unit/tes4_bsa_reader_tests.cpp` | Malformed partial-overlap and exact-duplicate TES4 reader regressions | ✓ VERIFIED | Contains focused tests for partial overlap rejection and exact duplicate acceptance with metadata assertions. |
| `src/detail/host_file_path.cpp` | Embedded-NUL rejection at public host-file path boundary | ✓ VERIFIED | Rejects embedded NUL at function entry with `error_code::invalid_argument`, before UTF-8 decoding and native path construction. |
| `tests/unit/host_file_path_tests.cpp` | Embedded-NUL host path regression | ✓ VERIFIED | Explicit-length embedded-NUL test verifies invalid argument result. |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/formats/bsa/tes4_bsa_parser.cpp` | `src/formats/bsa/tes4_bsa_payload_descriptor.cpp` | `make_tes4_bsa_payload_descriptor` output `payload_offset`/`stored_size` | ✓ WIRED | Parser calls descriptor at lines 99-103 and validates `payload.value().payload_offset` plus `payload.value().stored_size` at lines 108-123. |
| `src/formats/bsa/tes4_bsa_parser.cpp` | `src/formats/ba2/ba2_gnrl_parser.cpp` | Same exact-duplicate-allowed, partial-overlap-rejected span semantics | ✓ WIRED | TES4 implementation mirrors BA2 GNRL's accepted span policy from `ba2_gnrl_parser.cpp:410-423`. |
| `tests/unit/host_file_path_tests.cpp` | `src/detail/host_file_path.cpp` | `resolve_host_file_path(std::string_view{..., size})` returns invalid_argument before conversion | ✓ WIRED | Test calls the production helper directly at `host_file_path_tests.cpp:106` and asserts `invalid_argument` at line 109. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `src/formats/bsa/tes4_bsa_parser.cpp` | `payload.value().payload_offset`, `payload.value().stored_size` | `make_tes4_bsa_payload_descriptor(table.header, record, archive_size, table.metadata_table_size, read_payload_bytes)` | Yes | ✓ FLOWING |
| `src/detail/host_file_path.cpp` | `host_path` | Public `std::string_view` argument to `resolve_host_file_path` | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Focused malformed TES4 open and host-path regressions pass | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "tes4_bsa_malformed_open|host_file_path" --output-on-failure` | 21/21 tests passed | ✓ PASS |
| Exact duplicate TES4 payload spans remain accepted | `ctest --preset windows-msvc-debug-static -R "exact duplicate payload spans" --output-on-failure` | 1/1 test passed | ✓ PASS |

### Probe Execution

No task probes were declared or discovered for this quick task.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| `QUICK-260515-95h` | `260515-95h-PLAN.md` | Reject overlapping TES4 payload spans and embedded NUL public host paths while preserving duplicate-span dedupe compatibility. | ✓ SATISFIED | Implementation and tests verify all three planned truths; focused CTest checks pass. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| _None_ | - | - | - | No debt markers, placeholders, empty implementations, or console-only stubs found in modified task files. |

### Human Verification Required

None.

### Gaps Summary

No gaps found. The TES4 parser rejects non-empty partial overlaps before metadata publication, allows exact duplicate dedupe spans, and the shared public host path seam rejects embedded NUL bytes before native Windows path conversion.

---

_Verified: 2026-05-15T13:49:00Z_  
_Verifier: the agent (gsd-verifier)_
