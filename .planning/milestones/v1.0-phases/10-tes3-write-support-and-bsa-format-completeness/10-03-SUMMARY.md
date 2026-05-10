---
phase: 10-tes3-write-support-and-bsa-format-completeness
plan: 03
subsystem: bsa-writer
tags: [cpp20, tes3, bsa, writer, serialization, catch2]

requires:
  - phase: 10-tes3-write-support-and-bsa-format-completeness
    provides: TES3 writer public API, validation, source ownership, and safe publish from plans 10-01 and 10-02
provides:
  - Byte-level TES3 writer layout tests for header, records, name offsets, hashes, raw offsets, and payload bytes
  - Checked TES3 serializer arithmetic for table sizes, data-section start, payload sizes, and raw offsets
  - Compatibility comments documenting preserved-name hashing, low32/high32 sort order, and data-section-relative offsets

tech-stack:
  added: []
  patterns: [Catch2 byte-level archive assertions, checked uint32 TES3 serialization arithmetic]

key-files:
  created:
    - .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-03-SUMMARY.md
  modified:
    - tests/unit/tes3_bsa_writer_tests.cpp
    - src/formats/bsa/tes3_bsa_writer.cpp

key-decisions:
  - "TES3 writer byte-level tests compute expected order from detail::tes3_hash_sort_key(detail::hash_tes3(serialized_name)) instead of insertion or alphabetical order."
  - "TES3 serializer arithmetic now uses named checked uint32 helpers for table, data-section, payload, and raw-offset values."

patterns-established:
  - "TES3 writer table tests inspect raw little-endian bytes directly before relying on reader-backed validation."
  - "TES3 serializer comments record non-obvious compatibility rules next to the code that applies them."

requirements-completed: [WBSA-04]

duration: 15min
completed: 2026-05-10
---

# Phase 10 Plan 03: Byte-Accurate TES3 Serializer Summary

**Raw TES3 BSA writer serialization now has byte-level proof for header/table/hash/name/raw-offset layout and checked fail-closed uint32 arithmetic.**

## Performance

- **Duration:** 15 min
- **Started:** 2026-05-10T00:00:00Z
- **Completed:** 2026-05-10T00:14:48Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Added a focused byte-level writer test that parses writer-produced TES3 bytes directly and verifies version `0x00000100`, `hash_table_start - 12U`, file records, name offsets, null-terminated names, stored hashes, zero-byte payload offset behavior, and raw payload spans.
- Hardened serializer arithmetic with `checked_u32`, `checked_add_u32`, and `checked_mul_u32` for TES3 uint32 table, data-section, payload, and offset fields.
- Documented compatibility constraints in the serializer: preserved serialized names drive hashes, TES3 hash sort order is low32/high32, and on-disk raw offsets are data-section-relative.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED byte-level TES3 layout tests** - `faaafa5` (test)
2. **Task 2: GREEN checked TES3 serializer** - `4007868` (feat)
3. **Task 3: REFACTOR arithmetic and compatibility comments** - `6aa3262` (refactor)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/unit/tes3_bsa_writer_tests.cpp` - Adds raw byte readers and TES3 layout assertions for hash-sorted writer output.
- `src/formats/bsa/tes3_bsa_writer.cpp` - Adds named checked arithmetic helpers and compatibility comments for TES3 serializer behavior.
- `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-03-SUMMARY.md` - Records plan execution, verification, and TDD notes.

## Decisions Made

- Expected serialized order in tests is derived from `detail::tes3_hash_sort_key(detail::hash_tes3(serialized_name))`, matching Phase 10 D-15 and preventing insertion-order or alphabetical-order false positives.
- The serializer keeps raw/uncompressed TES3 output only; no compression, embedded-name, dedupe, or caller-provided hash branches were added.

## TDD Gate Compliance

- RED commit present: `faaafa5`.
- GREEN commit present after RED: `4007868`.
- REFACTOR commit present after GREEN: `6aa3262`.
- Note: The new RED test passed immediately because the current branch already contained a functional TES3 serializer scaffold from earlier Phase 10 work. The plan continued by hardening serializer arithmetic and documenting the unexpected green as an execution issue rather than weakening the test.

## Deviations from Plan

### Auto-fixed Issues

None - no Rule 1-3 deviations were required.

---

**Total deviations:** 0 auto-fixed.
**Impact on plan:** Scope stayed focused on TES3 byte layout tests, checked arithmetic, and compatibility comments.

## Issues Encountered

- The RED verification did not fail after adding the byte-level tests because the checked TES3 serializer behavior was already present in the working baseline. This was investigated by running the focused TES3 writer suite; the plan proceeded with the required test commit plus serializer hardening and refactor commits.

## Verification

- `cmake --build "build/windows-msvc-debug-static" --target libbsa_tests --config Debug` — passed.
- `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "tes3_bsa_writer|public_include_boundary" --output-on-failure` — 12/12 tests passed.

## Known Stubs

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for 10-04 reader-backed TES3 writer round-trip, lookup, and extraction proof.
- No blockers or auth gates remain.

## Self-Check: PASSED

- Found `tests/unit/tes3_bsa_writer_tests.cpp`.
- Found `src/formats/bsa/tes3_bsa_writer.cpp`.
- Found `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-03-SUMMARY.md`.
- Found commits `faaafa5`, `4007868`, and `6aa3262` in git history.

---
*Phase: 10-tes3-write-support-and-bsa-format-completeness*
*Completed: 2026-05-10*
