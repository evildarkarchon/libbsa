---
phase: 02-binary-i-o-paths-hashes-and-compression-services
plan: 05
subsystem: hash-boundary
tags: [cpp20, bethesda-hash, tes3, tes4, fo4-ba2, public-boundary, tdd]
requires:
  - phase: 02-04
    provides: Compression adapters and public/private boundary pattern
provides:
  - Internal TES3, TES4-family, and FO4/BA2 hash functions
  - Public header boundary regression tests for Phase 2 private names and dependencies
affects: [tes3-bsa, tes4-bsa, fo4-ba2, lookup]
tech-stack:
  added: []
  patterns: [reference-traced-constants, public-boundary-grep-test]
key-files:
  created: [src/detail/bethesda_hash.hpp, src/detail/bethesda_hash.cpp, tests/unit/bethesda_hash_tests.cpp]
  modified: [CMakeLists.txt, tests/CMakeLists.txt, tests/unit/public_include_boundary_tests.cpp]
key-decisions:
  - "Hash functions remain internal and cite TES5Edit/Core/wbBSArchive.pas reference behavior in comments/tests."
  - "Public boundary tests reject Phase 2 private implementation names outside comment-only lines."
patterns-established:
  - "Compatibility algorithms document non-obvious WHY next to implementation, not in public headers."
requirements-completed: [BIN-08]
duration: 20min
completed: 2026-05-08
---

# Phase 02 Plan 05: Bethesda Hashes and Boundary Summary

**Reference-traced TES3/TES4/FO4 hash primitives with public-header leakage guard and TES5Edit read-only verification**

## Performance
- **Duration:** 20 min
- **Started:** 2026-05-08T05:50:00Z
- **Completed:** 2026-05-08T06:10:00Z
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments
- Added `hash_tes3`, `hash_tes4`, split `hash_tes4`, and `hash_fo4` under `libbsa::detail`.
- Locked expected constants for multiple path/extensions from Phase 2 research trace.
- Extended public include boundary tests to keep private dependency/path/hash/router names out of installed headers.
- Verified `git -C TES5Edit status --short` produced no output.

## Task Commits
1. **Task 1: RED Bethesda hash constants** - `649ab61` (test)
2. **Task 2: GREEN internal Bethesda hash service** - `b8e4ca9` (feat)
3. **Task 3: Public boundary and TES5Edit guard** - `eab555b` (test)

## Files Created/Modified
- `src/detail/bethesda_hash.hpp` - Internal hash function declarations with Doxygen comments.
- `src/detail/bethesda_hash.cpp` - TES5Edit-compatible hash implementations and compatibility comments.
- `tests/unit/bethesda_hash_tests.cpp` - Reference constant tests.
- `tests/unit/public_include_boundary_tests.cpp` - Public header forbidden-token guard.
- `CMakeLists.txt`, `tests/CMakeLists.txt` - Source/test registration.

## Decisions Made
- CRC32 table is generated from the standard polynomial at runtime-static initialization, matching TES5Edit's table values without copying the entire table.
- Boundary tests scan public headers from `LIBBSA_SOURCE_DIR` and intentionally ignore comment-only lines to avoid self-invalidating documentation.

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
None.

## TDD Gate Compliance
- RED gate commit present: `649ab61`.
- GREEN gate commit present after RED: `b8e4ca9`.

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
Archive parser phases can use compatible hash functions and public headers remain clean of private dependencies and TES5Edit types.

## Self-Check: PASSED
- Verified files exist: `src/detail/bethesda_hash.hpp`, `src/detail/bethesda_hash.cpp`, `tests/unit/bethesda_hash_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.
- Verified commits exist: `649ab61`, `b8e4ca9`, `eab555b`.

---
*Phase: 02-binary-i-o-paths-hashes-and-compression-services*
*Completed: 2026-05-08*
