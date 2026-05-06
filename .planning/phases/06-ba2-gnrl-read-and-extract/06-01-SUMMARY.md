---
phase: 06-ba2-gnrl-read-and-extract
plan: 01
subsystem: public-api
tags: [cpp20, ba2, public-headers, cmake, smoke-tests]

requires:
  - phase: 02-streaming-api-archive-model-detection-and-hashes
    provides: archive_summary, entry_metadata, archive_view, caller-owned byte_source and byte_sink contracts
provides:
  - Separate BA2 public API header with metadata-only ba2_archive wrapper
  - Stubbed open_ba2 and extract_ba2_entry symbols returning structured unsupported_format errors
  - Explicit BA2 source/header CMake wiring and public-header smoke coverage
affects: [phase-06-ba2-parser, phase-07-ba2-dds, phase-10-ba2-writer, public-api]

tech-stack:
  added: []
  patterns:
    - Metadata-only archive wrapper backed by archive_view
    - Public-header-only smoke checks for consumer-visible BA2 symbols

key-files:
  created:
    - include/libbsa/ba2.hpp
    - src/ba2_reader.cpp
  modified:
    - CMakeLists.txt
    - tests/public_header_smoke.cpp

key-decisions:
  - "Kept BA2 APIs in a separate ba2.hpp header rather than expanding bsa.hpp or introducing a family-neutral archive API."
  - "Kept ba2_archive metadata-only; payload extraction requires a caller-owned byte_source to be passed again."

patterns-established:
  - "BA2 public archive objects mirror BSA archive inspection methods while preserving a separate family-specific API surface."
  - "Parser/extractor symbols can be linked before behavior implementation by returning structured unsupported_format failures."

requirements-completed: [BA2-01, BA2-02, BA2-03, BA2-04]

duration: 2min
completed: 2026-05-06
---

# Phase 06 Plan 01: BA2 Public Read/Extract Surface Summary

**Separate BA2 public API with metadata-only archive inspection, explicit build wiring, and consumer smoke coverage**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-06T02:23:33Z
- **Completed:** 2026-05-06T02:25:28Z
- **Tasks:** 2/2
- **Files modified:** 4

## Accomplishments

- Added `include/libbsa/ba2.hpp` with `ba2_archive`, `open_ba2`, and `extract_ba2_entry` declarations separate from `bsa.hpp`.
- Implemented `src/ba2_reader.cpp` with `ba2_archive` forwarding through copied `archive_view` metadata and temporary structured unsupported parser/extractor responses.
- Wired BA2 source/header files into explicit CMake source lists and extended the public-header smoke executable to name the BA2 archive type and function symbols.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add BA2 public header and metadata-only wrapper definitions** - `2459786` (feat)
2. **Task 2: Wire BA2 source/header and public smoke coverage** - `300ed63` (feat)

**Plan metadata:** committed separately in the final docs commit.

## Files Created/Modified

- `include/libbsa/ba2.hpp` - Public BA2 API header exposing metadata inspection and read/extract entry points without private dependency leakage.
- `src/ba2_reader.cpp` - BA2 archive wrapper definitions and structured unsupported parser/extractor placeholders for upcoming parser plans.
- `CMakeLists.txt` - Explicitly includes the BA2 source file and public header file set entry.
- `tests/public_header_smoke.cpp` - Consumer-style smoke coverage for `ba2_archive`, `open_ba2`, and `extract_ba2_entry`.

## Decisions Made

- Kept BA2 APIs family-specific in `ba2.hpp`; `bsa.hpp` was not modified and no family-neutral archive abstraction was introduced.
- Preserved the caller-owned I/O lifetime model by storing only copied metadata in `ba2_archive` and requiring extraction to receive a `byte_source` again.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope changes.

## Issues Encountered

None.

## Verification

- `cmake --build build/local-vs2026-vcpkg --config Debug` - passed.
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` - passed.
- `git status --short TES5Edit` - passed with empty output.
- Acceptance criteria for BA2 symbols, CMake wiring, smoke coverage, and public header dependency leakage - passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 06-02 to replace the structured unsupported BA2 parser path with behavior-driven GNRL header/table parsing and generated fixtures.

## Self-Check: PASSED

- Found `include/libbsa/ba2.hpp`, `src/ba2_reader.cpp`, `CMakeLists.txt`, and `tests/public_header_smoke.cpp`.
- Found task commits `2459786` and `300ed63` in git history.
- Confirmed `.planning/STATE.md` and `.planning/ROADMAP.md` were not modified by this executor beyond the pre-existing dirty ROADMAP state.

---
*Phase: 06-ba2-gnrl-read-and-extract*
*Completed: 2026-05-06*
