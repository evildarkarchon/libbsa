---
phase: 15-reader-backend-dispatch-cleanup
verified: 2026-05-14T02:49:42.0034390-07:00
status: passed
score: 3/3 must-haves verified
overrides_applied: 0
---

# Phase 15: Reader Backend Dispatch Cleanup Verification Report

**Phase Goal:** Reader behavior is selected once at open time so consumers see unchanged archive operations while maintainers stop repeating family dispatch logic across the public reader surface.
**Verified:** 2026-05-14T02:49:42.0034390-07:00
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Consumer can list, look up, check, extract, and bulk extract entries after one open-time backend selection across supported archive families. | ✓ VERIFIED | `src/archive.cpp:176-186,266-408` selects `backend_table` once during `open()` and reuses it for `entries`, `find`, `extract`, `extract_bytes`, and `extract_entries`. `tests/unit/archive_reader_dispatch_tests.cpp:189-311` exercises TES3, TES4, FO4 BA2 GNRL, Starfield BA2 GNRL v3, and BA2 DX10 through the public API. `ctest --preset windows-msvc-debug-static -R "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction" --output-on-failure` passed 15/15. |
| 2 | Those reader operations continue to behave the same as before across supported archive families. | ✓ VERIFIED | `tests/unit/archive_reader_dispatch_tests.cpp:221-309` verifies valid lookup/extraction plus unchanged `not_found` and `invalid_argument` behavior, duplicate exact-request coalescing, and payload bytes across representative families. Existing bulk behavior stayed green in `tests/unit/bulk_extraction_tests.cpp` via the same focused CTest run, and the full suite passed: `ctest --preset windows-msvc-debug-static --output-on-failure` → 377/377 passed with 2 expected opt-in skips. |
| 3 | Maintainer can adjust reader-backend behavior without duplicating archive-family branching across each public reader operation. | ✓ VERIFIED | `src/archive.cpp:37-77` centralizes backend selection in one file-local function table; public methods at `266-408` no longer branch on `metadata.variant`, `metadata.type`, or `is_ba2_dx10`. `tests/unit/archive_reader_dispatch_policy_tests.cpp:46-103` reads `src/archive.cpp` directly and fails if the public reader methods regain family-dispatch tokens; the policy test passed in the focused run. |

**Score:** 3/3 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/archive.cpp` | file-local backend identity plus open-time-selected function table reused by public reader operations | ✓ VERIFIED | Substantive seam at `37-77`; state wiring at `81-88`; one-time selection at `176-186`; public methods route through the selected backend at `266-408`. |
| `tests/unit/archive_reader_dispatch_tests.cpp` | dedicated cross-family runtime regression coverage for the public `archive_reader` surface | ✓ VERIFIED | `189-311` covers `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries` for five representative backends using committed fixtures. |
| `tests/unit/archive_reader_dispatch_policy_tests.cpp` | negative method-scoped source-policy guard against repeated family branching | ✓ VERIFIED | `46-103` extracts each public reader method body from `src/archive.cpp` and rejects forbidden dispatch tokens without freezing one helper name. |
| `tests/CMakeLists.txt` | Catch2 discovery registration for the new focused Phase 15 suites | ✓ VERIFIED | `65-70` adds both Phase 15 test files to `libbsa_tests`, and `ctest -N -R "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` lists the new selectors. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `src/archive.cpp` | `src/formats/bsa/tes3_bsa_reader.hpp` | backend table entries/find/extract callbacks for `tes3_bsa` | WIRED | `src/archive.cpp:51-53` binds `tes3_bsa_entries`, `find_tes3_bsa_entry`, and `extract_tes3_bsa_payload`. |
| `src/archive.cpp` | `src/formats/bsa/tes4_bsa_reader.hpp` | backend table entries/find/extract callbacks for `tes4_bsa` | WIRED | `src/archive.cpp:54-56` binds `tes4_bsa_entries`, `find_tes4_bsa_entry`, and `extract_tes4_bsa_payload_from_file`. |
| `src/archive.cpp` | `src/formats/ba2/ba2_gnrl_reader.hpp` | backend table entries/find/extract callbacks for `ba2_gnrl` | WIRED | `src/archive.cpp:57-59` binds `ba2_gnrl_entries`, `find_ba2_gnrl_entry`, and `extract_ba2_gnrl_payload`. |
| `src/archive.cpp` | `src/formats/ba2/ba2_dx10_reader.hpp` | backend table entries/find/extract callbacks for `ba2_dx10` | WIRED | `src/archive.cpp:60-62` binds `ba2_dx10_entries`, `find_ba2_dx10_entry`, and `extract_ba2_dx10_payload`. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/archive.cpp` | `backend_table` | `make_opened_reader(...)` selects `reader_backend_table(backend_identity)` at `176-186` | Yes — the selected callback table is dereferenced by `entries`, `find`, `extract`, `extract_bytes`, and `extract_entries` at `266-408`. | ✓ FLOWING |
| `src/archive.cpp` | `state_->host_path` | `detail::resolve_host_file_path(host_path)` at `165-168`, stored at `182-186` | Yes — payload extraction reuses the stored resolved path through `extract_entry_payload(...)` at `139-145` and `302,325,395`; full suite test `archive_reader extraction dispatch reuses the stored resolved host path` passed in the 377-test run. | ✓ FLOWING |
| `tests/unit/archive_reader_dispatch_tests.cpp` | fixture manifest + archive payload bytes | committed generated archives and manifests under `tests/fixtures/generated/archives` | Yes — runtime suite compares actual extracted bytes and result records against manifest-backed expectations across five representative archives at `193-309`. | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Focused Phase 15 selectors are discoverable | `ctest --preset windows-msvc-debug-static -N -R "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` | Listed 15 tests including `reader_backend_dispatch preserves reader operations across representative backends` and `archive_reader_dispatch_policy forbids repeated family dispatch in public reader methods`. | ✓ PASS |
| Dispatch seam preserves public reader behavior and bulk semantics | `ctest --preset windows-msvc-debug-static --output-on-failure -R "reader_backend_dispatch|archive_reader_dispatch_policy|bulk_extraction"` | 15/15 tests passed. | ✓ PASS |
| Phase 15 work does not break the supported debug validation lane | `ctest --preset windows-msvc-debug-static --output-on-failure` | 377/377 tests passed with 2 expected opt-in skips. | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
| --- | --- | --- | --- |
| None declared or discovered | `glob scripts/**/probe-*.sh` plus phase-artifact grep | No probe scripts or phase-declared probes found. | SKIP |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| DISP-01 | `15-01-PLAN.md` | Consumer can list, look up, check, extract, and bulk extract entries through one open-time reader backend selection with unchanged behavior across supported archive families. | ✓ SATISFIED | `src/archive.cpp:176-186,266-408` implements one-time backend selection and reuse. `tests/unit/archive_reader_dispatch_tests.cpp:189-311` proves the full public surface across representative families. Focused CTest run passed 15/15. |
| DISP-02 | `15-01-PLAN.md` | Maintainer can add or adjust reader-backend behavior without duplicating archive-family branching across each public reader operation. | ✓ SATISFIED | `src/archive.cpp:37-77` centralizes backend callbacks; public methods at `266-408` use the selected table instead of family branching. `tests/unit/archive_reader_dispatch_policy_tests.cpp:46-103` enforces the negative invariant and passed. |

Phase 15 requirement IDs from plan frontmatter were fully accounted for: `DISP-01`, `DISP-02`.
No orphaned Phase 15 requirement IDs were found in `.planning/REQUIREMENTS.md`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `src/archive.cpp` | `86,178-186` | `backend_identity` is stored in reader state but current post-open behavior dereferences only `backend_table`; no later read of the identity was found. | ⚠️ Warning | Minor maintainability debt: the seam currently keeps both an unused identity and the active callback table. This does not block the Phase 15 goal because dispatch is still selected once and reused correctly, but future edits must keep the parallel fields in sync. |

### Human Verification Required

None.

### Gaps Summary

None.

Phase 15's locked goal is achieved in the live codebase: `archive_reader::open` now chooses one file-local backend seam once, all public reader operations reuse that selection, representative cross-family behavior remains intact under runtime tests, and a dedicated policy test blocks the old per-method family dispatch pattern from returning.

---

_Verified: 2026-05-14T02:49:42.0034390-07:00_
_Verifier: the agent (gsd-verifier)_
