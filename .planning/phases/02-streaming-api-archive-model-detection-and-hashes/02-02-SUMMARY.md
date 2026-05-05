---
phase: 02-streaming-api-archive-model-detection-and-hashes
plan: 02
subsystem: archive-detection
tags: [cpp20, bsa, ba2, detection, metadata]
requires:
  - phase: 02-streaming-api-archive-model-detection-and-hashes
    provides: Streaming byte_source contract
provides:
  - Public archive metadata model
  - Bounded archive header detection
affects: [archive-readers, archive-view, compression-routing]
tech-stack:
  added: []
  patterns: [bounded little-endian header reads, structured detection failures]
key-files:
  created: [include/libbsa/archive.hpp, include/libbsa/detect.hpp, src/detect.cpp, tests/detection_tests.cpp]
  modified: [CMakeLists.txt]
key-decisions:
  - "Detection reports only safely-read header fields and does not imply table parsing."
patterns-established:
  - "Recognized magic with unsupported version returns unsupported_format; truncation returns malformed_archive."
requirements-completed: [BIO-04, BIO-05, DPH-01]
duration: 10min
completed: 2026-05-05
---

# Phase 02 Plan 02: Archive Model and Detection Summary

**Strict bounded header detection for TES3, TES4-family, FO4 BA2, and Starfield BA2 identities**

## Performance

- **Duration:** 10 min
- **Started:** 2026-05-05T23:52:28Z
- **Completed:** 2026-05-05T23:52:28Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Added public `archive_format`, `compression_state`, `archive_summary`, and `entry_metadata` types.
- Implemented `detect_archive(const byte_source&)` using bounded header reads.
- Added positive and negative detection tests for supported families, unsupported versions, BA2 subtypes, and truncation.

## Task Commits

1. **Task 1: Specify archive metadata and detection behavior with failing tests** - `c6dc1bc` (test)
2. **Task 2: Implement metadata types and strict bounded detection** - `5e7d04c` (feat)

## Files Created/Modified
- `include/libbsa/archive.hpp` - Public metadata model.
- `include/libbsa/detect.hpp` - Public detection API.
- `src/detect.cpp` - Header detector implementation.
- `tests/detection_tests.cpp` - Detection unit tests.
- `CMakeLists.txt` - Explicit target/source/header wiring.

## Decisions Made
- Stored BA2 subtype as a numeric optional header field instead of exposing a reference implementation type.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected BSA sample magic construction in tests**
- **Found during:** Task 2
- **Issue:** The red test helper constructed `"BSA\0"` through `std::string_view`, which dropped the null byte.
- **Fix:** Appended the little-endian BSA magic as `0x00415342`.
- **Files modified:** `tests/detection_tests.cpp`
- **Verification:** `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_detection_tests`
- **Committed in:** `5e7d04c`

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Known Stubs
None.

## Next Phase Readiness
Path normalization and archive views can use stable metadata identities.

## Self-Check: PASSED
- Created files exist.
- Task commits `c6dc1bc` and `5e7d04c` exist.

---
*Phase: 02-streaming-api-archive-model-detection-and-hashes*
*Completed: 2026-05-05*
