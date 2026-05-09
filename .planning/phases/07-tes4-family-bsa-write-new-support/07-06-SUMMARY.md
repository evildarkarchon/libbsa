---
phase: 07-tes4-family-bsa-write-new-support
plan: 06
subsystem: archive-writer
tags: [cpp20, bsa, tes4-family, writer, deduplication, ctest]

requires:
  - phase: 07-tes4-family-bsa-write-new-support
    provides: Public TES4-family writer contract, raw serialization, compression routing, and embedded-name payload encoding
provides:
  - Opt-in final-stored-byte payload deduplication for TES4-family BSA writer output
  - Reader-backed regression coverage for dedupe enabled, dedupe disabled, compression mismatch, and embedded-name mismatch cases
  - Final Phase 7 verification gates for writer, public boundary, and TES5Edit read-only status
affects: [tes4-bsa-writer, archive-reader-validation, phase-08-ba2-writer]

tech-stack:
  added: []
  patterns:
    - Final stored payload vectors are the dedupe key after embedded-name prefixing, compression size-prefixing, and codec output generation
    - Duplicate entries share metadata offsets while only the first owner writes the shared payload bytes

key-files:
  created: []
  modified:
    - src/formats/bsa/tes4_bsa_writer.cpp
    - tests/unit/tes4_bsa_writer_tests.cpp

key-decisions:
  - "TES4-family writer deduplication remains opt-in through tes4_bsa_writer_options::deduplicate_payloads and is disabled by default."
  - "Dedupe eligibility is based on byte-identical final stored payload vectors, not source bytes, so compression-policy and embedded-name differences keep distinct offsets."

patterns-established:
  - "Stored-payload ownership flag: shared entries reuse the original archive-absolute payload offset and skip duplicate byte emission."
  - "Dedupe tests validate behavior only through public archive_reader metadata and extraction APIs."

requirements-completed: [WBSA-01, WBSA-02, WBSA-03, WBSA-05, WBSA-06, WBSA-07, WBSA-08, WBSA-09, WBSA-10]

duration: 5min
completed: 2026-05-09
---

# Phase 07 Plan 06: Final Stored-Byte Deduplication Summary

**Opt-in TES4-family BSA writer deduplication now shares archive offsets only for byte-identical final stored payloads after all storage encoding steps.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-05-09T06:36:00Z
- **Completed:** 2026-05-09T06:41:12Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added RED writer regression tests proving dedupe is off by default, on only by explicit option, and constrained by final stored bytes.
- Implemented final-stored-byte dedupe after embedded-name prefixing, compression raw-size prefixing, and target codec output generation.
- Verified Phase 7 closeout gates: writer tests, public include boundary tests, and empty `TES5Edit/` status.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Add final-stored-byte dedupe tests** - `be05262` (test)
2. **Task 2 GREEN: Implement final-stored-byte dedupe and full phase gates** - `c228793` (feat)

_Note: This was a TDD plan and produced RED then GREEN commits._

## Files Created/Modified

- `tests/unit/tes4_bsa_writer_tests.cpp` - Adds public-reader-backed dedupe tests for disabled, enabled, compression-mismatch, and embedded-name-mismatch behavior.
- `src/formats/bsa/tes4_bsa_writer.cpp` - Adds final stored-payload dedupe lookup, shared offset assignment, and single-owner payload emission.

## Decisions Made

- Kept dedupe disabled by default and activated only through `tes4_bsa_writer_options::deduplicate_payloads`, preserving D-19 opt-in behavior.
- Used the complete `std::vector<std::byte>` stored payload as the deterministic in-memory key so offsets are shared only after all storage encoding steps match.
- Stored a per-entry payload ownership flag so duplicate entries share offsets without writing redundant payload bytes.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None.

## Issues Encountered

None.

## Verification

- `cmake --build --preset windows-msvc-debug-static` — passed.
- RED gate: `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` failed before implementation on the opt-in shared-offset assertion (`120 == 174`).
- GREEN/final gate: `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` — passed, 16/16 tests.
- `ctest --preset windows-msvc-debug-static -R "tes4_bsa_reader|public_include_boundary" --output-on-failure` — passed, 2/2 matching public boundary tests.
- `pwsh -NoProfile -Command '$status = git -C TES5Edit status --short; if ($status) { throw "TES5Edit has unexpected changes: $status" }'` — passed with no output.

## TDD Gate Compliance

- RED commit present before implementation: `be05262 test(07-06): add failing stored-payload dedupe tests`.
- GREEN commit present after RED: `c228793 feat(07-06): implement stored-payload dedupe`.
- REFACTOR commit: not needed; no separate cleanup changes were made after GREEN.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 7 writer support is complete and ready for Phase 8 BA2 GNRL write-new planning. TES4-family writer output now covers public API creation, target profiles, sorted indexes, flags, compression overrides, embedded names, opt-in dedupe, reader round-trip extraction, and the read-only `TES5Edit/` boundary.

## Self-Check: PASSED

- Found modified implementation file: `src/formats/bsa/tes4_bsa_writer.cpp`.
- Found modified test file: `tests/unit/tes4_bsa_writer_tests.cpp`.
- Found summary file: `.planning/phases/07-tes4-family-bsa-write-new-support/07-06-SUMMARY.md`.
- Found task commits: `be05262`, `c228793`.

---
*Phase: 07-tes4-family-bsa-write-new-support*
*Completed: 2026-05-09*
