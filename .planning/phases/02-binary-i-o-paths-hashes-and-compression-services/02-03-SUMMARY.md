---
phase: 02-binary-i-o-paths-hashes-and-compression-services
plan: 03
subsystem: compression
tags: [cpp20, libdeflate, deflate, exact-size, tdd]
requires:
  - phase: 02-02
    provides: Internal detail primitive conventions and malformed-test patterns
provides:
  - Private raw deflate compression adapter
  - Exact-size raw deflate decompression validation
affects: [tes4-bsa, fo4-ba2, starfield-ba2, archive-writers]
tech-stack:
  added: [libdeflate-private-link]
  patterns: [private-dependency-adapter, exact-size-codec-validation]
key-files:
  created: [src/detail/deflate_codec.hpp, src/detail/deflate_codec.cpp, tests/unit/deflate_codec_tests.cpp]
  modified: [CMakeLists.txt, tests/CMakeLists.txt]
key-decisions:
  - "Deflate support uses raw libdeflate APIs only; no zlib/gzip wrapper path was added."
  - "Codec output must equal archive metadata expected size exactly."
patterns-established:
  - "Compression adapters expose only libbsa-owned result/vector types in internal headers."
requirements-completed: [BIN-04, BIN-07]
duration: 14min
completed: 2026-05-08
---

# Phase 02 Plan 03: Deflate Codec Summary

**Private libdeflate raw-DEFLATE adapter with exact-size decompression and corrupt-input rejection**

## Performance
- **Duration:** 14 min
- **Started:** 2026-05-08T05:18:00Z
- **Completed:** 2026-05-08T05:32:00Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Added `compress_deflate` and `decompress_deflate_exact` under `libbsa::detail`.
- Validated round-trip, truncated compressed bytes, too-large expected size, and too-small expected size.
- Kept libdeflate includes in private `.cpp` implementation only.

## Task Commits
1. **Task 1: RED exact-size deflate behavior** - `7413334` (test)
2. **Task 2: GREEN private libdeflate adapter** - `b1bcfd2` (feat)

## Files Created/Modified
- `src/detail/deflate_codec.hpp` - Internal raw-deflate API.
- `src/detail/deflate_codec.cpp` - libdeflate RAII adapter and exact-size validation.
- `tests/unit/deflate_codec_tests.cpp` - Round-trip and malformed tests.
- `CMakeLists.txt`, `tests/CMakeLists.txt` - Dependency/source/test registration.

## Decisions Made
- vcpkg on this triplet exports `libdeflate::libdeflate_shared`; CMake uses a target-exists generator expression to prefer `libdeflate::libdeflate_static` when available and otherwise link the exported shared target privately.

## Deviations from Plan

### Auto-fixed Issues
**1. [Rule 3 - Blocking] Adapted libdeflate target name to installed vcpkg package**
- **Found during:** Task 2 (GREEN private libdeflate adapter)
- **Issue:** `libdeflate::libdeflate_static` was not exported by the installed vcpkg package on this triplet.
- **Fix:** Used a private generator-expression link that selects `libdeflate::libdeflate_static` when present, otherwise `libdeflate::libdeflate_shared`.
- **Files modified:** `CMakeLists.txt`
- **Verification:** `ctest --preset windows-msvc-debug-static -R deflate_codec --output-on-failure` passed.
- **Committed in:** `b1bcfd2`

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** Dependency remains private; no public API or behavior scope changed.

## Issues Encountered
The vcpkg target export differed from the plan's static-target assumption; resolved as documented above.

## TDD Gate Compliance
- RED gate commit present: `7413334`.
- GREEN gate commit present after RED: `b1bcfd2`.

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
TES4-family BSA and BA2 phases can use a single exact-size deflate boundary for payload chunks.

## Self-Check: PASSED
- Verified files exist: `src/detail/deflate_codec.hpp`, `src/detail/deflate_codec.cpp`, `tests/unit/deflate_codec_tests.cpp`.
- Verified commits exist: `7413334`, `b1bcfd2`.

---
*Phase: 02-binary-i-o-paths-hashes-and-compression-services*
*Completed: 2026-05-08*
