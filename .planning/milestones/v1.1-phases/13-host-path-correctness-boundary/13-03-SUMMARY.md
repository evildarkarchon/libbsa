---
phase: 13-host-path-correctness-boundary
plan: 03
subsystem: api
tags: [host_file_path, archive_reader, parser, ba2, bsa, catch2, tdd]
requires:
  - phase: 13-host-path-correctness-boundary
    provides: shared host_file_path contract and host_file helper overloads from Plan 02
provides:
  - archive_reader open-time state stores resolved host paths once per open
  - TES3, TES4, BA2 GNRL, and BA2 DX10 parser entry seams consume host_file_path
  - parser archive-file opens use shared host_file helpers instead of raw narrow host-path streams
affects: [phase-13-plan-04, archive-open-boundary, parser-io, validation-boundary]
tech-stack:
  added: []
  patterns: [open-time resolve-once reader state, parser host_file_path contracts, diagnostics-only original UTF-8 text]
key-files:
  created: [.planning/phases/13-host-path-correctness-boundary/13-03-SUMMARY.md]
  modified: [src/archive.cpp, src/formats/bsa/tes3_bsa_parser.hpp, src/formats/bsa/tes3_bsa_parser.cpp, src/formats/bsa/tes4_bsa_parser.hpp, src/formats/bsa/tes4_bsa_parser.cpp, src/formats/ba2/ba2_gnrl_parser.hpp, src/formats/ba2/ba2_gnrl_parser.cpp, src/formats/ba2/ba2_dx10_parser.hpp, src/formats/ba2/ba2_dx10_parser.cpp, tests/unit/host_file_writer_name_tests.cpp, tests/unit/ba2_gnrl_reader_tests.cpp]
key-decisions:
  - "archive_reader::open now resolves caller UTF-8 text once, stores detail::host_file_path in reader state, and uses the resolved path for detection and size probes."
  - "Parser entry seams now accept detail::host_file_path so archive metadata opens stay on the shared host_file boundary."
  - "Original UTF-8 host-path text remains diagnostics-only across the open/parser seam, and source comments now state that it must not become a fallback I/O path."
patterns-established:
  - "Read-side host-file seams resolve public UTF-8 input once, then pass detail::host_file_path through later open-time parser work."
  - "Internal parser regression tests may resolve host_file_path explicitly when they exercise archive-file entry seams directly."
requirements-completed: [HOST-01]
duration: 6 min
completed: 2026-05-13
---

# Phase 13 Plan 03: Stored open-time host path and parser seam migration summary

**archive_reader now stores one resolved host-file path per open while TES3/TES4/BA2 parser entry seams reopen metadata only through the shared host_file boundary.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-05-13T16:16:59-07:00
- **Completed:** 2026-05-13T16:23:01-07:00
- **Tasks:** 3
- **Files modified:** 11

## Accomplishments
- Moved `archive_reader::open` onto `detail::host_file_path` so resolution happens once and detection/size probes use the resolved path.
- Changed TES3, TES4, BA2 GNRL, and BA2 DX10 parser archive-file seams to consume `detail::host_file_path` and open streams through `detail::open_host_file`.
- Tightened source comments to state that original UTF-8 host-path text is diagnostics-only after successful resolution in the open/parser seam.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Resolve and store the shared host-file path once in archive_reader::open** - `24136ab` (test)
2. **Task 1 GREEN: Resolve and store the shared host-file path once in archive_reader::open** - `7950aec` (feat)
3. **Task 2 RED: Migrate TES3, TES4, and BA2 parser contracts plus parser opens onto the stored path boundary** - `bcd9a61` (test)
4. **Task 2 GREEN: Migrate TES3, TES4, and BA2 parser contracts plus parser opens onto the stored path boundary** - `97912e7` (feat)
5. **Task 3: Sweep remaining raw host-path opens from the open/parser seam and lock the contract comments** - `817d0f9` (refactor)

**Plan metadata:** pending state/roadmap commit

## Files Created/Modified
- `src/archive.cpp` - Stores `detail::host_file_path` in reader state and routes open-time detection and size probes through shared host-file helpers.
- `src/formats/bsa/tes3_bsa_parser.hpp` - Documents the resolved host-file contract on the TES3 parser archive-file entry seam.
- `src/formats/bsa/tes3_bsa_parser.cpp` - Opens TES3 metadata through `detail::open_host_file` instead of a raw narrow host-path stream.
- `src/formats/bsa/tes4_bsa_parser.hpp` - Documents the resolved host-file contract on the TES4 parser archive-file entry seam.
- `src/formats/bsa/tes4_bsa_parser.cpp` - Opens TES4 metadata through the shared host-file seam and keeps bounded payload-prefix reads intact.
- `src/formats/ba2/ba2_gnrl_parser.hpp` - Documents the resolved host-file contract on the BA2 GNRL parser archive-file entry seam.
- `src/formats/ba2/ba2_gnrl_parser.cpp` - Opens BA2 GNRL metadata and filename-table reads through the shared host-file seam.
- `src/formats/ba2/ba2_dx10_parser.hpp` - Documents the resolved host-file contract on the BA2 DX10 parser archive-file entry seam.
- `src/formats/ba2/ba2_dx10_parser.cpp` - Opens BA2 DX10 metadata and filename-table reads through the shared host-file seam.
- `tests/unit/host_file_writer_name_tests.cpp` - Locks the archive-reader and parser seam migration with source-policy tests.
- `tests/unit/ba2_gnrl_reader_tests.cpp` - Updates the direct parser-entry test to resolve `host_file_path` explicitly.

## Decisions Made
- Kept the public `archive_reader::open(std::string_view)` signature unchanged while moving its internal state to `detail::host_file_path`.
- Chose `detail::host_file_path` rather than raw `std::filesystem::path` for parser entry seams so diagnostics text and resolved path stay coupled.
- Rewrote parser entry doc comments and the reader-state comment to make the diagnostics-only original-text rule explicit for later phases.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Plan 13-04 can now rewire reader reopens and validation setup onto the stored host-path boundary instead of introducing another open-time path contract.
- Parser entry seams already prove the resolved-path contract across TES3, TES4, BA2 GNRL, and BA2 DX10, reducing risk for the validation and extraction follow-on work.

## Known Stubs

None.

## Self-Check: PASSED

- FOUND: `.planning/phases/13-host-path-correctness-boundary/13-03-SUMMARY.md`
- FOUND: `src/archive.cpp`
- FOUND: `src/formats/bsa/tes3_bsa_parser.hpp`
- FOUND: `src/formats/bsa/tes3_bsa_parser.cpp`
- FOUND: `src/formats/bsa/tes4_bsa_parser.hpp`
- FOUND: `src/formats/bsa/tes4_bsa_parser.cpp`
- FOUND: `src/formats/ba2/ba2_gnrl_parser.hpp`
- FOUND: `src/formats/ba2/ba2_gnrl_parser.cpp`
- FOUND: `src/formats/ba2/ba2_dx10_parser.hpp`
- FOUND: `src/formats/ba2/ba2_dx10_parser.cpp`
- FOUND COMMIT: `24136ab`
- FOUND COMMIT: `7950aec`
- FOUND COMMIT: `bcd9a61`
- FOUND COMMIT: `97912e7`
- FOUND COMMIT: `817d0f9`

---
*Phase: 13-host-path-correctness-boundary*
*Completed: 2026-05-13*
