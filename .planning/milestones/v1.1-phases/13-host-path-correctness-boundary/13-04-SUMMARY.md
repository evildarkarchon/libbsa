---
phase: 13-host-path-correctness-boundary
plan: 04
subsystem: validation-api
tags: [host_file_path, archive_reader, validation_api, bsa, ba2, tdd]
requires:
  - phase: 13-host-path-correctness-boundary
    provides: stored open-time host_file_path state and parser entry seams from Plan 03
provides:
  - reader reopen helpers now consume detail::host_file_path across TES3, TES4, BA2 GNRL, and BA2 DX10 extraction paths
  - archive_reader extraction dispatch now reuses the stored resolved host path instead of caller UTF-8 text
  - validate_archive setup now relies on archive_reader::open without a duplicate readability preflight
affects: [phase-13-plan-05, host-path-boundary, validation-boundary, extraction-io]
tech-stack:
  added: []
  patterns: [stored resolved-path extraction dispatch, validation setup through archive_reader::open, source-policy seam guards]
key-files:
  created: [.planning/phases/13-host-path-correctness-boundary/13-04-SUMMARY.md]
  modified: [src/archive.cpp, src/validation.cpp, src/formats/bsa/tes3_bsa_reader.hpp, src/formats/bsa/tes3_bsa_reader.cpp, src/formats/bsa/tes4_bsa_reader.hpp, src/formats/bsa/tes4_bsa_reader.cpp, src/formats/ba2/ba2_gnrl_reader.hpp, src/formats/ba2/ba2_gnrl_reader.cpp, src/formats/ba2/ba2_dx10_reader.hpp, src/formats/ba2/ba2_dx10_reader.cpp, tests/unit/host_file_writer_name_tests.cpp, tests/unit/tes3_bsa_reader_tests.cpp, tests/unit/ba2_gnrl_reader_tests.cpp, tests/unit/ba2_dx10_extraction_tests.cpp]
key-decisions:
  - "archive_reader now passes detail::host_file_path through extraction dispatch so follow-on reads reopen only from the stored resolved host path."
  - "Concrete TES3, TES4, BA2 GNRL, and BA2 DX10 reader seams reopen archives through detail::open_host_file rather than raw UTF-8 host-path text."
  - "validate_archive now treats archive_reader::open as the only host-path setup path, preserving direct io_error failures for unreadable paths and report-based diagnostics for readable malformed archives."
patterns-established:
  - "Reader reopen declarations in format headers use detail::host_file_path when later operations must stay on the resolved host-file boundary."
  - "Source-policy tests can lock internal boundary migrations without widening the public host-path API."
requirements-completed: [HOST-01, HOST-02]
duration: 3 min
completed: 2026-05-13
---

# Phase 13 Plan 04: Reader reopen and validation boundary summary

**Stored resolved host-file paths now drive follow-on TES3/TES4/BA2 extraction reopens and validation setup stays unified behind archive_reader::open.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-13T16:33:36-07:00
- **Completed:** 2026-05-13T16:36:11-07:00
- **Tasks:** 3
- **Files modified:** 14

## Accomplishments
- Routed archive extraction dispatch through `detail::host_file_path` so opened reader state, not caller UTF-8 text, controls follow-on host-file reopens.
- Migrated TES3, TES4, BA2 GNRL, and BA2 DX10 reader seams to `detail::open_host_file` while preserving payload range checks, decompression routing, and error-code behavior.
- Removed `validate_archive`'s duplicate readability preflight and kept the public setup-vs-report split intact.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Rewire extraction dispatch and concrete readers onto the stored shared host-file path** - `414412c` (test)
2. **Task 1 GREEN: Rewire extraction dispatch and concrete readers onto the stored shared host-file path** - `5df4e75` (feat)
3. **Task 2 RED: Remove validation preflight drift and preserve the public result/report split** - `3330fe9` (test)
4. **Task 2 GREEN: Remove validation preflight drift and preserve the public result/report split** - `7c25c13` (feat)
5. **Task 3: Sweep remaining read/validate narrow-open drift and lock the resolved-path rule in comments** - `886cb16` (refactor)

**Plan metadata:** pending state/roadmap commit

## Files Created/Modified
- `src/archive.cpp` - Passes stored `detail::host_file_path` through extraction dispatch instead of reopening from `original_utf8` text.
- `src/validation.cpp` - Removes duplicate setup preflight and documents why validation setup is delegated to `archive_reader::open`.
- `src/formats/bsa/tes3_bsa_reader.hpp` - Declares the TES3 reopen seam on `detail::host_file_path` with updated doc comments.
- `src/formats/bsa/tes3_bsa_reader.cpp` - Streams TES3 payloads through `detail::open_host_file` and shared payload helpers.
- `src/formats/bsa/tes4_bsa_reader.hpp` - Declares the TES4 reopen seam on `detail::host_file_path`.
- `src/formats/bsa/tes4_bsa_reader.cpp` - Reopens TES4 archives through the shared host-file helper before payload extraction.
- `src/formats/ba2/ba2_gnrl_reader.hpp` - Declares BA2 GNRL extraction on the resolved-path contract.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - Reopens BA2 GNRL archives through shared host-file helpers for raw and compressed payloads.
- `src/formats/ba2/ba2_dx10_reader.hpp` - Declares BA2 DX10 extraction on the resolved-path contract.
- `src/formats/ba2/ba2_dx10_reader.cpp` - Reopens BA2 DX10 archives through the shared host-file helper before chunk streaming and decompression.
- `tests/unit/host_file_writer_name_tests.cpp` - Adds source-policy coverage for reader reopen and validation boundary migration.
- `tests/unit/tes3_bsa_reader_tests.cpp` - Resolves test reopen paths through `detail::resolve_host_file_path` for TES3 helper coverage.
- `tests/unit/ba2_gnrl_reader_tests.cpp` - Resolves helper test reopen paths through `detail::resolve_host_file_path` for BA2 GNRL extraction coverage.
- `tests/unit/ba2_dx10_extraction_tests.cpp` - Resolves direct BA2 DX10 helper tests through `detail::resolve_host_file_path`.

## Decisions Made
- Kept the public reader and validation host-path API unchanged while moving all follow-on reader reopens onto stored `detail::host_file_path` values.
- Reused `detail::open_host_file` in each concrete reader so Windows path resolution remains centralized and caller-facing diagnostics stay format-specific.
- Added boundary comments instead of new helper abstractions to explain the non-obvious rule that readable malformed archives are report-level diagnostics while unreadable host paths remain direct failures.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Plan 13-05 can now prove non-ASCII host-path open, validation, and canonical extraction on the final reader reopen contract.
- Validation and extraction now share the same resolved-path boundary, so the dedicated regression suite can stay black-box and public-surface-only.

## Known Stubs

None.

## Self-Check: PASSED

- FOUND: `.planning/phases/13-host-path-correctness-boundary/13-04-SUMMARY.md`
- FOUND: `src/archive.cpp`
- FOUND: `src/validation.cpp`
- FOUND: `src/formats/bsa/tes3_bsa_reader.hpp`
- FOUND: `src/formats/bsa/tes3_bsa_reader.cpp`
- FOUND: `src/formats/bsa/tes4_bsa_reader.hpp`
- FOUND: `src/formats/bsa/tes4_bsa_reader.cpp`
- FOUND: `src/formats/ba2/ba2_gnrl_reader.hpp`
- FOUND: `src/formats/ba2/ba2_gnrl_reader.cpp`
- FOUND: `src/formats/ba2/ba2_dx10_reader.hpp`
- FOUND: `src/formats/ba2/ba2_dx10_reader.cpp`
- FOUND COMMIT: `414412c`
- FOUND COMMIT: `5df4e75`
- FOUND COMMIT: `3330fe9`
- FOUND COMMIT: `7c25c13`
- FOUND COMMIT: `886cb16`

---
*Phase: 13-host-path-correctness-boundary*
*Completed: 2026-05-13*
