---
phase: 02-streaming-api-archive-model-detection-and-hashes
plan: 03
subsystem: path-hash
tags: [cpp20, archive-path, tes3, tes4, fo4, golden]
requires:
  - phase: 02-streaming-api-archive-model-detection-and-hashes
    provides: Result/error foundation and metadata model
provides:
  - Public archive_path normalization
  - Internal/test-visible Bethesda hash primitives
affects: [metadata-lookup, archive-readers, writer-compatibility]
tech-stack:
  added: []
  patterns: [owned virtual paths, golden-vector provenance]
key-files:
  created: [include/libbsa/archive_path.hpp, src/archive_path.cpp, src/hash.hpp, src/hash.cpp, tests/path_hash_tests.cpp]
  modified: [CMakeLists.txt]
key-decisions:
  - "Kept exact hash functions internal/test-visible while exposing path normalization publicly."
patterns-established:
  - "Golden vectors cite TES5Edit provenance inline in test source."
requirements-completed: [DPH-02, DPH-04, DPH-05, BIO-05]
duration: 15min
completed: 2026-05-05
---

# Phase 02 Plan 03: Archive Paths and Hashes Summary

**Owned archive path normalization with TES3, TES4, and FO4 hash vectors traced to TES5Edit behavior**

## Performance

- **Duration:** 15 min
- **Started:** 2026-05-05T23:52:28Z
- **Completed:** 2026-05-05T23:52:28Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- Added public `archive_path` and `normalize_archive_path` APIs.
- Implemented TES3, TES4-family, and FO4/BA2-compatible hash primitives in internal/test-visible files.
- Added golden-vector tests with inline TES5Edit provenance comments.

## Task Commits

1. **Task 1: Specify path normalization and hash golden-vector behavior** - `e69276f` (test)
2. **Task 2: Implement archive_path normalization and internal hashes** - `ab40567` (feat)

## Files Created/Modified
- `include/libbsa/archive_path.hpp` - Public owned path value and normalization API.
- `src/archive_path.cpp` - Validation and normalization.
- `src/hash.hpp` / `src/hash.cpp` - Internal/test-visible hash primitives.
- `tests/path_hash_tests.cpp` - Path and golden-vector coverage.
- `CMakeLists.txt` - Explicit target/source/header wiring and golden label setup.

## Decisions Made
- Used ASCII-only lowercasing to match TES5Edit `LowerByte` instead of locale-aware folding.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Enabled CTest golden label discovery**
- **Found during:** Task 2
- **Issue:** `ctest -L golden` initially found no tests from the path/hash target.
- **Fix:** Added Catch2 tag labels so `[golden]` tests are discoverable by CTest.
- **Files modified:** `CMakeLists.txt`
- **Verification:** `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L golden`
- **Committed in:** `ab40567`

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Known Stubs
None.

## Next Phase Readiness
Metadata lookup can normalize consumer paths and compare stable archive-virtual strings.

## Self-Check: PASSED
- Created files exist.
- Task commits `e69276f` and `ab40567` exist.

---
*Phase: 02-streaming-api-archive-model-detection-and-hashes*
*Completed: 2026-05-05*
