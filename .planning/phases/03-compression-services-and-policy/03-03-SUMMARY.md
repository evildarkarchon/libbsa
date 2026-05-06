---
phase: 03-compression-services-and-policy
plan: 03
subsystem: compression
tags: [cpp20, lz4, catch2, codec]
requires:
  - phase: 03-compression-services-and-policy
    provides: compression payload dispatcher and deflate wrapper pattern
provides:
  - Separate LZ4 frame and raw block payload wrappers
  - Cross-route rejection tests proving frame and block bytes are not interchangeable
affects: [compression, skyrim-se, starfield, extraction]
tech-stack:
  added: []
  patterns: [separate frame/block codec wrappers, safe LZ4 decompression]
key-files:
  created: [src/compression/lz4_frame_codec.hpp, src/compression/lz4_frame_codec.cpp, src/compression/lz4_block_codec.hpp, src/compression/lz4_block_codec.cpp, tests/lz4_codec_tests.cpp]
  modified: [src/compression.cpp, CMakeLists.txt]
key-decisions:
  - "Skyrim SE/AE LZ4 frame payloads and Starfield BA2 raw LZ4 block payloads use separate internal wrappers."
  - "Cross-route failures are treated as decompression failures and locked by tests."
patterns-established:
  - "Use LZ4F_* only for frame payloads and LZ4_* safe block APIs only for raw block payloads."
  - "Require exact output size for LZ4 frame and block decompression."
requirements-completed: [CMP-02, CMP-03, CMP-04]
duration: 2min
completed: 2026-05-06
---

# Phase 03 Plan 03: LZ4 Codecs Summary

**Separate LZ4 frame and raw block dispatchers with cross-route rejection coverage**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-06T00:24:45Z
- **Completed:** 2026-05-06T00:26:26Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Added private LZ4 frame wrappers using `LZ4F_compressFrame` and `LZ4F_decompress`.
- Added private raw LZ4 block wrappers using `LZ4_compress_default` and `LZ4_decompress_safe`.
- Added tests for LZ4 frame magic, block round-trip, exact-size mismatch, and frame/block cross-route rejection.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add failing LZ4 frame/block and cross-route tests** - `e12a504` (test)
2. **Task 2: Implement LZ4 frame and raw block wrappers** - `8c71b80` (feat)

_Note: This TDD plan captured RED and GREEN commits separately._

## Files Created/Modified

- `tests/lz4_codec_tests.cpp` - LZ4 frame, raw block, and cross-route tests.
- `src/compression/lz4_frame_codec.hpp` - Internal LZ4 frame declarations.
- `src/compression/lz4_frame_codec.cpp` - LZ4 frame compression/decompression wrapper.
- `src/compression/lz4_block_codec.hpp` - Internal raw LZ4 block declarations.
- `src/compression/lz4_block_codec.cpp` - Raw LZ4 block compression/decompression wrapper.
- `src/compression.cpp` - Dispatcher wiring for frame and block algorithms.
- `CMakeLists.txt` - LZ4 source and test target registration.

## Decisions Made

- Skyrim SE/AE LZ4 frame payloads and Starfield BA2 raw LZ4 block payloads use separate internal wrappers.
- Cross-route failures are treated as decompression failures and locked by tests.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- RED verification failed as expected because the dispatcher initially returned unsupported-route failures for LZ4 algorithms.

## User Setup Required

None - no external service configuration required.

## Known Stubs

None.

## TDD Gate Compliance

- RED gate commit: `e12a504`
- GREEN gate commit: `8c71b80`

## Self-Check: PASSED

- Verified created files exist: `src/compression/lz4_frame_codec.*`, `src/compression/lz4_block_codec.*`, `tests/lz4_codec_tests.cpp`.
- Verified task commits exist: `e12a504`, `8c71b80`.
- Verified targeted LZ4 codec tests passed.

## Next Phase Readiness

- All compression algorithms are wired for Plan 03-04 public-header smoke coverage and documentation.
- No blockers identified.

---
*Phase: 03-compression-services-and-policy*
*Completed: 2026-05-06*
