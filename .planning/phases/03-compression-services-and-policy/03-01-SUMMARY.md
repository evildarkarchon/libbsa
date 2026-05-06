---
phase: 03-compression-services-and-policy
plan: 01
subsystem: compression
tags: [cpp20, cmake, catch2, compression-routing]
requires:
  - phase: 02-streaming-api-archive-model-detection-and-hashes
    provides: archive_format and compression_state metadata contracts
provides:
  - Public compression routing API and writer policy resolution
  - Format-aware codec selection tests for deflate, LZ4 frame, and Starfield raw LZ4 block routes
affects: [compression, readers, writers, archive-routing]
tech-stack:
  added: []
  patterns: [libbsa-owned public routing enums, structured unsupported-format failures]
key-files:
  created: [include/libbsa/compression.hpp, src/compression.cpp, tests/compression_policy_tests.cpp]
  modified: [CMakeLists.txt]
key-decisions:
  - "Compression routing is based on archive format, entry state, and Starfield CompressionMethod rather than filenames or extensions."
  - "Writer policy resolves to archive-native compression_state values while third-party codec details stay out of public headers."
patterns-established:
  - "Public compression APIs expose libbsa-owned enums and result<T> errors only."
  - "Unsupported codec routes fail with error_code::unsupported_format and message 'unsupported compression route'."
requirements-completed: [CMP-04, CMP-05]
duration: 12min
completed: 2026-05-06
---

# Phase 03 Plan 01: Compression Routing Contracts Summary

**Format-aware compression routing with libbsa-owned policy contracts for reader and writer code**

## Performance

- **Duration:** 12 min
- **Started:** 2026-05-06T00:10:00Z
- **Completed:** 2026-05-06T00:22:42Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added `include/libbsa/compression.hpp` with public compression algorithm, policy, request, and resolver declarations.
- Implemented explicit routing in `src/compression.cpp` for none/raw, deflate, Skyrim SE LZ4 frame, and Starfield BA2 v3 raw LZ4 block payloads.
- Added Catch2 policy tests and CMake registration under the `unit;codec` labels.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add failing routing and writer-policy tests** - `6741b82` (test)
2. **Task 2: Implement public compression routing contracts** - `4a6de04` (feat)

_Note: This TDD plan captured RED and GREEN commits separately._

## Files Created/Modified

- `include/libbsa/compression.hpp` - Public compression routing and writer policy contracts.
- `src/compression.cpp` - Format-aware codec and writer-policy resolution implementation.
- `tests/compression_policy_tests.cpp` - Routing, unsupported-route, and writer-policy unit tests.
- `CMakeLists.txt` - Library source/header registration and compression policy test target.

## Decisions Made

- Compression routing is based on archive format, entry state, and Starfield `CompressionMethod` rather than filenames or extensions.
- Writer policy resolves to archive-native `compression_state` values while third-party codec details stay out of public headers.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- RED verification failed as expected because `libbsa/compression.hpp` did not exist before implementation.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## TDD Gate Compliance

- RED gate commit: `6741b82`
- GREEN gate commit: `4a6de04`

## Self-Check: PASSED

- Verified created files exist: `include/libbsa/compression.hpp`, `src/compression.cpp`, `tests/compression_policy_tests.cpp`.
- Verified task commits exist: `6741b82`, `4a6de04`.
- Verified targeted compression policy tests passed.

## Next Phase Readiness

- Compression routing is ready for Plan 03-02 to wire exact-size deflate payload compression and decompression through the dispatcher.
- No blockers identified.

---
*Phase: 03-compression-services-and-policy*
*Completed: 2026-05-06*
