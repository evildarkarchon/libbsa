---
phase: 09-ba2-dx10-write-new-support
plan: 03
subsystem: texture
tags: [cpp20, ba2-dx10, dds, chunk-planning, tdd]

requires:
  - phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
    provides: BA2 DX10 read-side DDS layout validation and extraction oracle
provides:
  - DX10 locked-format mip byte sizing for writer chunk planning
  - BA2-compatible contiguous mip chunk planner with default and byte-cap modes
  - Uniform array/cubemap mip-range repetition for writer serialization
affects: [ba2-dx10-writer, texture-layout, WBA2-09]

tech-stack:
  added: []
  patterns: [descriptor-table-format-sizing, checked-mip-range-planning, tdd-red-green-refactor]

key-files:
  created: []
  modified:
    - src/texture/dds_layout.hpp
    - src/texture/dds_layout.cpp
    - tests/unit/dds_layout_tests.cpp

key-decisions:
  - "DX10 chunk planning uses a libbsa-owned descriptor table for the locked DXGI format set rather than DirectXTex types."
  - "max_decoded_chunk_bytes == 0 selects the reference-derived default policy; nonzero values enforce an archive-wide decoded-byte cap at mip boundaries."

patterns-established:
  - "Locked DDS format sizing routes through one checked mip_size_for_format/mip_range_size path used by both planner and validator."
  - "Array and cubemap chunk plans repeat the same mip-range split per slice/face to preserve DDS order."

requirements-completed: [WBA2-09]

duration: 4 min
completed: 2026-05-09
---

# Phase 09 Plan 03: DX10 Mip Sizing and Chunk Planning Summary

**Checked DDS mip sizing and BA2 DX10 chunk planning for locked writer formats, byte caps, arrays, and cubemaps**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-09T10:18:18Z
- **Completed:** 2026-05-09T10:22:04Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Added TDD coverage for locked DXGI format byte sizes, default chunking, cap-driven chunking, arrays, cubemaps, and impossible-cap failures.
- Added `planned_texture_chunk`, `mip_size_for_format`, `mip_range_size`, and `plan_dx10_chunks` in the private texture layout utility.
- Reused checked mip-size/range math in existing DX10 chunk validation so extraction validation and writer planning share one sizing path.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED: Specify format sizing and chunk planner behavior** - `65e4dd8` (test)
2. **Task 2: GREEN: Implement checked locked-format sizing and planner** - `c3971f6` (feat)
3. **Task 3: REFACTOR: Reuse planner in existing chunk validation safely** - `8270200` (refactor)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `src/texture/dds_layout.hpp` - Declares planner result type and checked mip sizing/planning contracts.
- `src/texture/dds_layout.cpp` - Implements locked-format descriptor sizing, reference-derived default chunking, byte-cap chunking, and shared validator sizing.
- `tests/unit/dds_layout_tests.cpp` - Adds table-driven locked-format tests plus default/capped/array/cubemap planner coverage.

## Decisions Made

- DX10 chunk planning stays DirectXTex-free and uses numeric libbsa-owned DXGI format IDs in `dds_layout`.
- `max_decoded_chunk_bytes == 0` is the default sentinel; nonzero caps greedily group contiguous mips and fail with `format_error` if one mip cannot fit.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- The initial RED cap value was too small for a 1024x1024 BC7 mip. It was corrected during GREEN so the test specified the intended behavior: first mip alone, remaining mips grouped under a representable cap.

## TDD Gate Compliance

- **RED:** `65e4dd8` added failing planner tests; build failed because planner APIs were absent.
- **GREEN:** `c3971f6` implemented planner APIs and tests passed.
- **REFACTOR:** `8270200` extracted range expansion cleanup and tests still passed.

## Known Stubs

None.

## Verification

- `cmake --build --preset windows-msvc-debug-static` — passed after GREEN implementation.
- `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` — passed, 7/7 tests.
- `ctest --preset windows-msvc-debug-static -R "dds_layout|ba2_dx10" --output-on-failure` — passed, 20/20 tests.
- TDD gate commit checks for `test(09-03)`, `feat(09-03)`, and `refactor(09-03)` — passed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 09-04 can use `plan_dx10_chunks` and checked mip sizes when serializing compressed BA2 DX10 texture chunks.

## Self-Check: PASSED

- Verified modified files exist: `src/texture/dds_layout.hpp`, `src/texture/dds_layout.cpp`, `tests/unit/dds_layout_tests.cpp`.
- Verified task commits exist: `65e4dd8`, `c3971f6`, `8270200`.

---
*Phase: 09-ba2-dx10-write-new-support*
*Completed: 2026-05-09*
