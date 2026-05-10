---
phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction
plan: 03
subsystem: texture
tags: [dds, dxt10, ba2-dx10, texture-layout, tdd]

requires:
  - phase: 06-01
    provides: BA2 DX10 fixture and test-target foundation
  - phase: 06-02
    provides: dependency-light texture metadata and private DirectXTex analyzer boundary
provides:
  - DirectXTex-free DDS DXT10 header construction from libbsa-owned texture layout values
  - Validated mip, array, and cubemap chunk ordering with logical source chunk identity
  - Focused DDS layout tests for header constants and malformed coverage rejection
affects: [06-04-ba2-dx10-parser, 06-05-ba2-dx10-extraction, phase-09-ba2-dx10-write]

tech-stack:
  added: []
  patterns:
    - Internal texture utility under src/texture with libbsa-owned structs and result<T> errors
    - DDS layout validation fails closed before extraction payload reads

key-files:
  created:
    - src/texture/dds_layout.hpp
    - src/texture/dds_layout.cpp
  modified:
    - CMakeLists.txt
    - tests/unit/dds_layout_tests.cpp

key-decisions:
  - "DDS reconstruction emits standard DDS magic/header plus DDS_HEADER_DXT10 from libbsa-owned layout values, without DirectXTex as a runtime extraction gate."
  - "Chunk validation accepts only computed BA2-derived DDS order: array slice ascending, cubemap face order +X/-X/+Y/-Y/+Z/-Z, then ascending mip ranges per face or slice."
  - "Phase 6 DDS layout byte-size validation supports fixture-backed DXGI formats 28 and BC1-format ids 71/72, failing closed for unsupported formats."

patterns-established:
  - "dds_layout: build deterministic DDS DXT10 headers using detail::binary_writer and stable constants."
  - "logical_texture_segment: carry array, face, mip range, and source_chunk_index so extractors do not blindly concatenate archive chunks."

requirements-completed: [DDS-04, DDS-07]

duration: 3min
completed: 2026-05-09
---

# Phase 06 Plan 03: DDS Layout Utility Summary

**DirectXTex-free DDS DXT10 header construction with validated BA2 texture chunk order for mip, array, and cubemap extraction.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-09T01:51:12Z
- **Completed:** 2026-05-09T01:54:39Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments

- Added RED/GREEN/REFACTOR-tested DDS layout coverage for DXT10 header constants, cubemap misc flags, logical segment identity, array slice repetition, and malformed layout rejection.
- Implemented `src/texture/dds_layout.hpp/.cpp` with libbsa-owned texture layout and `logical_texture_segment` types, plus deterministic header construction.
- Validated BA2 chunk metadata in computed DDS order and rejects gaps, duplicates/overlaps within the same face or slice, impossible byte totals, zero chunks, and unsupported DXGI fixture formats.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED: Specify DDS DXT10 header and texture coverage behavior** - `66d7212` (test)
2. **Task 2: GREEN: Implement internal DDS layout utility** - `884f467` (feat)
3. **Task 3: REFACTOR: Document DDS assumptions and layout constraints** - `bba4f21` (refactor)

**Plan metadata:** committed separately after state/roadmap updates.

## Files Created/Modified

- `src/texture/dds_layout.hpp` - Internal DDS layout contract with layout, logical segment, header builder, and chunk validator APIs.
- `src/texture/dds_layout.cpp` - DDS DXT10 header construction, fixture-backed byte-size helpers, and BA2-derived chunk coverage validation.
- `tests/unit/dds_layout_tests.cpp` - TDD tests for header constants, cubemap/array logical ordering, and malformed layout rejection.
- `CMakeLists.txt` - Registers `src/texture/dds_layout.cpp` in the libbsa target.

## Decisions Made

- DDS reconstruction now uses a deterministic always-DXT10 internal header builder rather than depending on DirectXTex during normal extraction.
- Layout validation treats repeated mip ranges as valid across different array slices or cubemap faces, but rejects duplicates/overlaps within the same face or slice.
- The first implementation supports the generated-fixture DXGI format 28 and BC1 test formats 71/72, returning `error_code::format_error` for unsupported formats until broader hardening expands coverage.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## Known Stubs

None.

## TDD Gate Compliance

- **RED:** `66d7212` added failing DDS layout tests.
- **GREEN:** `884f467` implemented the DDS layout utility and made tests pass.
- **REFACTOR:** `bba4f21` documented non-obvious DDS/BA2 constraints with tests still green.

## Verification

- `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` — PASS (4/4 tests passed).
- Acceptance greps for DDS magic, DX10 FourCC, cubemap misc flag, logical segment identity, duplicate same-face/slice coverage, implementation declarations, CMake source registration, DXT10 comments, and absence of `#include <DirectXTex` in `dds_layout.cpp` — PASS.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for 06-04 BA2 DX10 parser work. Parser tasks can call `validate_and_order_chunks` during open to reject metadata-detectable layout problems and preserve `source_chunk_index` for later extraction ordering.

## Self-Check: PASSED

- FOUND: `src/texture/dds_layout.hpp`
- FOUND: `src/texture/dds_layout.cpp`
- FOUND: `tests/unit/dds_layout_tests.cpp`
- FOUND: `66d7212`
- FOUND: `884f467`
- FOUND: `bba4f21`

---
*Phase: 06-dds-boundary-and-ba2-dx10-read-reconstruction*
*Completed: 2026-05-09*
