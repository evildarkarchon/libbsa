---
phase: 09-bsa-writers
plan: 05
subsystem: bsa-writer
tags: [cpp20, bsa, tes3, morrowind, tdd, hash-sorting]

requires:
  - phase: 09-bsa-writers
    provides: TES4-family BSA writer API, memory/disk inputs, and native finalization seams from plans 09-01 through 09-04
provides:
  - TES3 Morrowind native BSA writer planning and finalization
  - TES3 hash-sorted records with deterministic path tie handling
  - TES3 reader/test alignment with TES5Edit high32-low32 hash table word order
  - TES3 raw payload read-after-write and unsupported option failure coverage
affects: [bsa-writers, tes3, archive-reader, validation]

tech-stack:
  added: []
  patterns:
    - TDD RED/GREEN for TES3 native writer behavior
    - Checked TES3 table and data-section-relative layout arithmetic
    - TES5Edit-compatible TES3 hash table serialization as high 32 bits then low 32 bits

key-files:
  created:
    - .planning/phases/09-bsa-writers/09-05-SUMMARY.md
  modified:
    - tests/bsa_writer_tests.cpp
    - src/bsa_writer.cpp
    - src/bsa_reader.cpp
    - tests/bsa_reader_tests.cpp

key-decisions:
  - "TES3 hash table bytes now follow TES5Edit save order: high 32 bits first, then low 32 bits, with reader and fixtures updated together."
  - "TES3 planning rejects compression and embedded-name options structurally instead of downgrading to raw output."

patterns-established:
  - "TES3 writer plans expose archive-absolute payload offsets while serialized file records store data-section-relative offsets."
  - "TES3 records are sorted by hash and then normalized path for deterministic tie handling."

requirements-completed: [WRT-04]

duration: 3min
completed: 2026-05-07
---

# Phase 09 Plan 05: TES3 BSA Native Writer Layout Summary

**TES3 Morrowind BSA writer emits hash-sorted native tables, TES5Edit-order hashes, data-section-relative records, and raw payloads that reopen and extract through libbsa.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-07T07:06:44Z
- **Completed:** 2026-05-07T07:09:17Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added failing TES3 writer tests for hash sorting, name offsets, hash table bytes, relative offsets, read-after-write extraction, and unsupported option failures.
- Implemented `bsa_write_target::tes3_morrowind` with checked TES3 header/table/name/hash/data layout and raw payload finalization.
- Updated TES3 reader fixture/parsing behavior to match the resolved TES5Edit high32-low32 hash table byte order.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED TES3 native writer tests** - `b833741` (test)
2. **Task 2: GREEN TES3 native writer implementation** - `a8f108c` (feat)

**Plan metadata:** Pending final metadata commit.

_Note: TDD tasks produced RED and GREEN commits for the single feature._

## Files Created/Modified

- `tests/bsa_writer_tests.cpp` - Adds TES3 native writer RED/GREEN tests for hash order, table bytes, raw round trips, and unsupported options.
- `src/bsa_writer.cpp` - Implements TES3 writer planning/finalization with checked layout arithmetic and native table bytes.
- `src/bsa_reader.cpp` - Parses TES3 hash table entries using TES5Edit high32-low32 word order.
- `tests/bsa_reader_tests.cpp` - Updates generated TES3 fixtures to emit the same reference hash table word order.
- `.planning/phases/09-bsa-writers/09-05-SUMMARY.md` - Records execution outcome and verification.

## Decisions Made

- TES3 hash table serialization follows TES5Edit/BSArchPro save semantics: `Hash shr 32` is written as a little-endian `uint32_t`, followed by `Hash and $FFFFFFFF` as a little-endian `uint32_t`.
- TES3 compression and embedded-name requests are unsupported writer inputs; planning returns structured errors instead of silently emitting a raw archive.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## TDD Gate Compliance

- **RED:** `b833741` added failing TES3 writer tests; focused writer test run failed on absent TES3 writer behavior.
- **GREEN:** `a8f108c` implemented TES3 writer behavior and hash-order reader alignment; focused writer/reader tests passed.
- **REFACTOR:** Not needed.

## Verification

- `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_bsa_writer_tests` — PASS during RED and GREEN runs.
- `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_writer_tests` — PASS, 26/26 tests.
- `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_bsa_writer_tests libbsa_bsa_reader_tests` — PASS.
- `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_bsa_writer_tests|libbsa_bsa_reader_tests"` — PASS, 48/48 tests.
- Acceptance `rg` checks for TES3 test names, implementation markers, hash word-order comments, and unsupported embedded/compression paths — PASS.

## Known Stubs

None.

## Threat Flags

None - this plan adds archive writer table/payload serialization only, matching the plan threat model for caller entries becoming TES3 tables.

## Self-Check: PASSED

- Verified created/modified files exist on disk.
- Verified task commits `b833741` and `a8f108c` exist in git history.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for `09-06`: TES3 writer behavior is in place; remaining Phase 9 work can build on native BSA writer coverage without placeholder TES3 behavior.

---
*Phase: 09-bsa-writers*
*Completed: 2026-05-07*
