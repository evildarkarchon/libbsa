---
phase: 02-binary-i-o-paths-hashes-and-compression-services
plan: 04
subsystem: compression
tags: [cpp20, lz4, lz4-frame, lz4-block, compression-router, tdd]
requires:
  - phase: 02-03
    provides: Deflate adapter and exact-size codec pattern
provides:
  - Private LZ4 frame adapter
  - Private raw LZ4 block adapter
  - Explicit compression router by method enum
affects: [skyrim-se-bsa, starfield-ba2, compression-routing]
tech-stack:
  added: [lz4-private-link]
  patterns: [separate-lz4-formats, explicit-method-routing]
key-files:
  created: [src/detail/lz4_frame_codec.hpp, src/detail/lz4_frame_codec.cpp, src/detail/lz4_block_codec.hpp, src/detail/lz4_block_codec.cpp, src/detail/compression_router.hpp, src/detail/compression_router.cpp, tests/unit/lz4_codec_tests.cpp, tests/unit/compression_router_tests.cpp]
  modified: [CMakeLists.txt, tests/CMakeLists.txt]
key-decisions:
  - "LZ4 frame and raw block formats are separate adapters with cross-format rejection tests."
  - "Compression routing dispatches only on compression_method enum, never filenames or extensions."
patterns-established:
  - "Router functions compose private codec adapters without exposing dependency headers."
requirements-completed: [BIN-05, BIN-06, BIN-07]
duration: 18min
completed: 2026-05-08
---

# Phase 02 Plan 04: LZ4 and Compression Router Summary

**Separate LZ4 frame/raw-block adapters plus explicit enum-based compression routing with exact-size validation**

## Performance
- **Duration:** 18 min
- **Started:** 2026-05-08T05:32:00Z
- **Completed:** 2026-05-08T05:50:00Z
- **Tasks:** 3
- **Files modified:** 10

## Accomplishments
- Added `compress_lz4_frame`/`decompress_lz4_frame_exact` using `LZ4F_*` APIs.
- Added `compress_lz4_block`/`decompress_lz4_block_exact` using raw `LZ4_*safe*` APIs.
- Added `compression_router` with `none`, `deflate`, `lz4_frame`, `lz4_block`, and unsupported method handling.

## Task Commits
1. **Task 1: RED separate LZ4 codecs** - `554b3d2` (test)
2. **Task 2: GREEN private LZ4 adapters** - `c3f66f8` (feat)
3. **Task 3: RED/GREEN explicit compression router** - `8a831c1` (test), `967d174` (feat)

## Files Created/Modified
- `src/detail/lz4_frame_codec.*` - LZ4 frame compression/decompression.
- `src/detail/lz4_block_codec.*` - Raw LZ4 block compression/decompression.
- `src/detail/compression_router.*` - Explicit compression-method dispatch.
- `tests/unit/lz4_codec_tests.cpp` - Round-trip, malformed, cross-format, and size-mismatch tests.
- `tests/unit/compression_router_tests.cpp` - Router dispatch, none, and unsupported enum tests.
- `CMakeLists.txt`, `tests/CMakeLists.txt` - lz4 link and test/source registration.

## Decisions Made
- Raw LZ4 block sizes are validated against `int` limits before calling one-shot LZ4 APIs.
- LZ4 frame decompression must fully consume the frame and produce exactly the expected byte count.

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
None.

## TDD Gate Compliance
- RED gate commits present: `554b3d2`, `8a831c1`.
- GREEN gate commits present after RED: `c3f66f8`, `967d174`.

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
Format parsers can select compression explicitly from archive metadata without risking frame/block confusion.

## Self-Check: PASSED
- Verified files exist: `src/detail/lz4_frame_codec.hpp`, `src/detail/lz4_block_codec.hpp`, `src/detail/compression_router.hpp`, related tests.
- Verified commits exist: `554b3d2`, `c3f66f8`, `8a831c1`, `967d174`.

---
*Phase: 02-binary-i-o-paths-hashes-and-compression-services*
*Completed: 2026-05-08*
