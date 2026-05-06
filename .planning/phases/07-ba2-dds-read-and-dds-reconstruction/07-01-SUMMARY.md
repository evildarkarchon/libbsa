---
phase: 07-ba2-dds-read-and-dds-reconstruction
plan: 01
subsystem: api
tags: [ba2, dds, texture-metadata, public-api, dxgi-format]
requires:
  - phase: 06-ba2-gnrl-read-and-extract
    provides: BA2 archive metadata ownership and entry lookup patterns
provides:
  - Public libbsa-owned BA2 texture metadata value types
  - BA2 texture metadata lookup by normalized archive path
  - Public-header smoke coverage for DDS metadata APIs
affects: [phase-07, ba2-dds-reader, dds-reconstruction]
tech-stack:
  added: []
  patterns:
    - Public texture metadata uses libbsa-owned value types instead of platform or DirectXTex types.
    - ba2_archive stores copied texture metadata keyed by normalized archive paths.
key-files:
  created: []
  modified:
    - include/libbsa/ba2.hpp
    - src/ba2_reader.cpp
    - tests/public_header_smoke.cpp
key-decisions:
  - "Kept texture metadata separate from generic entry_metadata so BA2 texture-only fields do not leak into generic archive entries."
  - "Returned texture_metadata by value through result<texture_metadata> to preserve the existing structured error contract."
patterns-established:
  - "BA2 texture metadata lookup mirrors entry lookup: normalize caller path, return copied metadata, fail structurally when absent."
requirements-completed: [BA2-05, BA2-06]
duration: 12min
completed: 2026-05-06
---

# Phase 07 Plan 01: Public BA2 Texture Metadata API Summary

**BA2 DDS texture metadata API with libbsa-owned DXGI format values and public smoke coverage**

## Performance

- **Duration:** 12 min
- **Started:** 2026-05-06T06:14:00Z
- **Completed:** 2026-05-06T06:26:38Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added `dxgi_format`, `texture_chunk_metadata`, `texture_metadata`, and `dxgi_format_name` to the public BA2 API.
- Added `ba2_archive::texture_metadata(path)` returning copied metadata by value through `result<texture_metadata>`.
- Extended the public-header smoke test to prove consumer code can construct and inspect the texture API without private dependency headers.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add public texture metadata smoke expectations** - `b74e896` (test)
2. **Task 2: Implement public texture metadata API** - `221ed26` (feat)

**Plan metadata:** pending docs commit

_Note: TDD tasks used separate RED and GREEN commits._

## Files Created/Modified

- `include/libbsa/ba2.hpp` - Public BA2 texture metadata value types, known DXGI format naming, constructor overload, and lookup API.
- `src/ba2_reader.cpp` - Copied texture metadata storage, normalized lookup behavior, and known DXGI format labels.
- `tests/public_header_smoke.cpp` - Consumer-style smoke coverage for texture metadata construction, successful lookup, and structured missing-texture failure.

## Decisions Made

- Kept texture metadata out of `entry_metadata`, preserving the generic archive metadata surface.
- Used copied metadata in `ba2_archive` so returned texture metadata has no source, parser, or DirectXTex lifetime dependency.

## Deviations from Plan

None - plan executed exactly as written.

**Total deviations:** 0 auto-fixed.
**Impact on plan:** No scope change.

## Issues Encountered

- `ctest` initially passed because the existing smoke executable was stale; rebuilding `libbsa_public_header_smoke` produced the expected RED compile failure before implementation.
- The broad public-header leakage grep would also catch pre-existing public compression enum names if run over all `include/libbsa`; the final check verified the new BA2 public surface has no private dependency names.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The Phase 7 parser and DDS reconstruction plans can now populate and consume public texture metadata without changing the consumer-facing API.

---
*Phase: 07-ba2-dds-read-and-dds-reconstruction*
*Completed: 2026-05-06*
