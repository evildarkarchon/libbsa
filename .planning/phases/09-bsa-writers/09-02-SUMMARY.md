---
phase: 09-bsa-writers
plan: 02
subsystem: bsa-writer
tags: [cpp20, bsa, tes4, writer, tdd, roundtrip]

requires:
  - phase: 09-bsa-writers
    provides: BSA writer API seam from plan 09-01
provides:
  - TES4-family v103/v104/v105 native raw BSA planning and finalization
  - Read-after-write tests for native TES4-family writer output
  - Plan-owned native table bytes for deterministic BSA finalization
affects: [bsa-writers, compression-writers, validation]

tech-stack:
  added: []
  patterns: [TDD RED-GREEN, native BSA table serialization, plan-owned finalization bytes]

key-files:
  created:
    - .planning/phases/09-bsa-writers/09-02-SUMMARY.md
  modified:
    - tests/bsa_writer_tests.cpp
    - src/bsa_writer.cpp
    - include/libbsa/bsa_writer.hpp

key-decisions:
  - "TES4-family BSA finalization now writes plan-owned native table bytes instead of recomputing layout during sink emission."
  - "FileFlags are computed from known entry extensions and serialized in the native TES4-family header without adding a public override."

patterns-established:
  - "Native BSA writer plans own `table_bytes` plus payload regions; finalization streams both verbatim."
  - "TES4-family writer tests prove output by reopening through `open_bsa` and extracting through `extract_bsa_entry`."

requirements-completed: [WRT-01]

duration: 3min
completed: 2026-05-07
---

# Phase 09 Plan 02: TES4-Family Raw BSA Writer Layout Summary

**Native TES4-family BSA writer layout for v103/v104/v105 with hash-ordered tables, FileFlags, plan-owned table bytes, and read-after-write payload verification**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-07T06:48:56Z
- **Completed:** 2026-05-07T06:52:14Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Replaced TES4-family placeholder writer tests with RED coverage for native raw BSA output, native table regions, FileFlags, and reference-hash ordering.
- Implemented native TES4-family BSA planning for Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 memory inputs.
- Finalization streams planned native header/table bytes followed by planned payload regions to caller-owned sinks; no `LBSW` or generic `finalize_archive_write` production path is used.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add failing TES4-family raw layout tests** - `0b627e9` (test)
2. **Task 2 GREEN: Implement TES4-family raw native writer** - `82ca8ed` (feat)

**Plan metadata:** created in final `docs(09-02)` commit

## Files Created/Modified

- `tests/bsa_writer_tests.cpp` - Defines TES4-family native raw writer RED/GREEN tests for versions, counts, FileFlags, native table regions, hash ordering, metadata offsets, and extraction.
- `src/bsa_writer.cpp` - Implements TES4-family memory planning, native table serialization, extension-derived FileFlags, compression-state payload shaping, disk-to-memory planning adapter, and finalization.
- `include/libbsa/bsa_writer.hpp` - Adds plan-owned native `table_bytes` with Doxygen documentation so finalization can stream planned layout bytes verbatim.
- `.planning/phases/09-bsa-writers/09-02-SUMMARY.md` - Records execution outcome and verification.

## Decisions Made

- TES4-family finalization writes plan-owned table bytes. This preserves the plan/finalize invariant that all compatibility-sensitive layout decisions happen during planning.
- Known FileFlags are computed from entry file extensions (`.nif`, `.dds`, `.xml`, `.wav`/audio) and serialized into the TES4-family header; unknown extensions remain valid by contributing no extra file flag.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Added plan-owned native table bytes to the public BSA write plan**
- **Found during:** Task 2 (GREEN implementation)
- **Issue:** The existing `bsa_write_plan` exposed regions and payloads but had no owned storage for native header/table bytes, which would force finalization to recompute layout and violate the plan's finalization requirement.
- **Fix:** Added Doxygen-documented `table_bytes` to `bsa_write_plan` and made `finalize_bsa_write` stream those bytes before payload regions.
- **Files modified:** `include/libbsa/bsa_writer.hpp`, `src/bsa_writer.cpp`
- **Verification:** `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_writer_tests`
- **Committed in:** `82ca8ed`

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** Required for correctness and deterministic finalization; no scope creep beyond the native BSA writer contract.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None found in files modified by this plan.

## Threat Flags

None - the new trust-boundary code matches the plan threat model for caller entries, layout arithmetic, path normalization, and sink finalization.

## TDD Gate Compliance

- RED gate: `0b627e9` (`test(09-02): add failing TES4-family BSA writer layout tests`)
- GREEN gate: `82ca8ed` (`feat(09-02): implement TES4-family native BSA writer`)
- REFACTOR gate: not needed; no behavior-neutral cleanup commit was created.

## Verification

- PASS: `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_bsa_writer_tests`
- PASS: `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_writer_tests`
- PASS: `rg -n "TES4-family raw BSA|native TES4 table regions|computes TES4 FileFlags|orders TES4 folders and files" tests/bsa_writer_tests.cpp`
- PASS: `rg -n "BSA\\0|0x67|0x68|0x69|folder_record|file_name|hash_tes4|archive_default_compressed" src/bsa_writer.cpp`
- PASS: `rg -n "LBSW|finalize_archive_write" src/bsa_writer.cpp` returned no matches.

## Next Phase Readiness

Plan 09-03 can build on the native TES4-family layout foundation to add TES4 compression and embedded-name behavior. TES3 writing remains intentionally unimplemented for its dedicated later plan.

## Self-Check: PASSED

- FOUND: `tests/bsa_writer_tests.cpp`
- FOUND: `src/bsa_writer.cpp`
- FOUND: `include/libbsa/bsa_writer.hpp`
- FOUND: `.planning/phases/09-bsa-writers/09-02-SUMMARY.md`
- FOUND: commit `0b627e9`
- FOUND: commit `82ca8ed`

---
*Phase: 09-bsa-writers*
*Completed: 2026-05-07*
