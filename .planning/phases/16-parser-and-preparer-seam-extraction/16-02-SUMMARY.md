---
phase: 16-parser-and-preparer-seam-extraction
plan: 02
subsystem: writer
tags: [ba2-dx10, preparer, snapshot-builder, chunk-assembler, tdd, catch2, ctest]
requires:
  - phase: 16-parser-and-preparer-seam-extraction
    provides: TES4 parser seam extraction from plan 16-01
provides:
  - private BA2 DX10 snapshot builder seam for DDS source load, target validation, and add-time snapshot ownership
  - private BA2 DX10 chunk assembler seam for planned texture chunk collection, streamed snapshot reads, and compression routing
  - focused parser_preparer_seam regression coverage for BA2 DX10 snapshot immutability, chunk ordering, failure paths, and compression methods
affects: [src/formats/ba2, tests/unit, CMakeLists.txt, tests/CMakeLists.txt]
tech-stack:
  added: []
  patterns: [private writer-preparer seams, direct internal seam tests, streamed snapshot chunk assembly]
key-files:
  created: [src/formats/ba2/ba2_dx10_snapshot_builder.hpp, src/formats/ba2/ba2_dx10_snapshot_builder.cpp, src/formats/ba2/ba2_dx10_chunk_assembler.hpp, src/formats/ba2/ba2_dx10_chunk_assembler.cpp, tests/unit/ba2_dx10_preparer_seam_tests.cpp]
  modified: [src/formats/ba2/ba2_dx10_prepare.cpp, CMakeLists.txt, tests/CMakeLists.txt, tests/unit/bounded_memory_policy_tests.cpp]
key-decisions:
  - "BA2 DX10 add-file snapshot staging now lives in a private snapshot-builder seam while ba2_dx10_make_writer_entry remains the stable coordinator entrypoint."
  - "BA2 DX10 planned chunk assembly, streamed snapshot reads, size validation, indexed work placement, and compression routing now live in a private chunk-assembler seam."
  - "Existing bounded-memory snapshot reservation policy follows the moved snapshot-builder implementation instead of remaining tied to ba2_dx10_prepare.cpp."
patterns-established:
  - "Direct BA2 DX10 seam tests may include private snapshot-builder and chunk-assembler headers without changing public include/libbsa headers."
  - "Preparer coordinators delegate source ownership and planned-chunk assembly while preserving existing internal function signatures."
requirements-completed: [REFA-02]
duration: 8m
completed: 2026-05-14
---

# Phase 16 Plan 02: BA2 DX10 Snapshot Builder and Chunk Assembler Seams Summary

**BA2 DX10 write preparation now separates add-time DDS snapshot ownership from planned chunk assembly and compression routing behind private seams with focused regression coverage.**

## Performance

- **Duration:** 8m
- **Started:** 2026-05-14T23:14:49Z
- **Completed:** 2026-05-14T23:22:24Z
- **Tasks:** 3
- **Files modified:** 9

## Accomplishments

- Added `ba2_dx10_snapshot_builder` as the private seam for resolving and reading DDS host files, calling the texture analyzer, validating target formats, creating hardened snapshot directories, and writing writer-owned subresource snapshots.
- Added `ba2_dx10_chunk_assembler` as the private seam for collecting planned snapshot subresources, streaming snapshot bytes into per-chunk buffers, validating raw sizes, routing Fallout 4/Starfield compression, and preserving indexed work result placement.
- Added focused `[parser_preparer_seam]` Catch2 coverage for snapshot immutability, target validation, multi-mip/array/cubemap-array ordering, missing/truncated snapshot failure paths, and Fallout 4 plus Starfield v3 compression routing.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED — add focused BA2 DX10 preparer seam tests** - `a5717e7` (test)
2. **Task 2: GREEN — extract snapshot builder and chunk assembler seams** - `7feb845` (feat)
3. **Task 3: REFACTOR — tighten BA2 DX10 coordinator and preserve staging semantics** - `41caf9e` (refactor)

_Note: Plan metadata is committed separately after state and roadmap updates._

## TDD Gate Compliance

- **RED:** `a5717e7` added focused BA2 DX10 seam tests and failed before implementation because `formats/ba2/ba2_dx10_chunk_assembler.hpp` did not exist.
- **GREEN:** `7feb845` added the snapshot-builder and chunk-assembler seams, rewired `ba2_dx10_prepare.cpp`, updated the bounded-memory policy source target, and made focused plus affected BA2 DX10 tests pass.
- **REFACTOR:** `41caf9e` documented the non-obvious snapshot ownership and bounded-memory staging contracts in the private seam headers while focused tests continued to pass.

## Validation

- **RED command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` failed before implementation with missing private BA2 DX10 seam headers.
- **GREEN focused command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` passed with 6/6 tests.
- **GREEN affected command after policy fix:** `ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_dx10_writer|writer-stage|parser_preparer_seam"` passed with 50/50 tests.
- **REFACTOR focused command:** `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` passed with 6/6 tests.
- **Plan final command:** `ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_dx10_writer|writer-stage|parser_preparer_seam"` passed with 50/50 tests.

## Files Created/Modified

- `src/formats/ba2/ba2_dx10_snapshot_builder.hpp` - declares the private snapshot builder and target-format/snapshot-directory responsibilities.
- `src/formats/ba2/ba2_dx10_snapshot_builder.cpp` - implements DDS host-file loading, DirectXTex adapter handoff, target validation, hardened snapshot directory reservation, and subresource snapshot writing.
- `src/formats/ba2/ba2_dx10_chunk_assembler.hpp` - declares chunk assembly and plan-then-assemble entry preparation from snapshot handles and planned texture chunks.
- `src/formats/ba2/ba2_dx10_chunk_assembler.cpp` - implements streamed snapshot-backed chunk assembly, raw-size validation, compression routing, indexed chunk work, and prepared-entry construction.
- `src/formats/ba2/ba2_dx10_prepare.cpp` - becomes the stable coordinator for writer entry creation, entry validation, chunk delegation, and post-preparation canonical sorting.
- `tests/unit/ba2_dx10_preparer_seam_tests.cpp` - adds direct BA2 DX10 snapshot/chunk seam regression tests with `[parser_preparer_seam]` labels.
- `tests/unit/bounded_memory_policy_tests.cpp` - follows the hardened snapshot reservation policy tokens after they moved into the snapshot builder seam.
- `CMakeLists.txt` - registers the new private BA2 DX10 seam implementation files.
- `tests/CMakeLists.txt` - registers the focused BA2 DX10 preparer seam test file.

## Decisions Made

- Kept the new BA2 DX10 seams private under `src/formats/ba2/` and did not add or modify public headers under `include/libbsa/`.
- Kept `ba2_dx10_prepare.hpp` signatures unchanged while using `ba2_dx10_prepare.cpp` as a narrow coordinator over snapshot and chunk seams.
- Kept Phase 17 dedupe optimization and BA2 DX10 temp lifecycle cleanup proof explicitly out of scope; `DEDU-01`, `DEDU-02`, `DX10-01`, and `DX10-02` remain pending.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Updated bounded-memory policy source target after moving snapshot reservation**
- **Found during:** Task 2 (GREEN — extract snapshot builder and chunk assembler seams)
- **Issue:** The existing bounded-memory policy test still searched `ba2_dx10_prepare.cpp` for hardened snapshot reservation tokens after the implementation moved those responsibilities into `ba2_dx10_snapshot_builder.cpp`.
- **Fix:** Updated the policy test to read both the coordinator and snapshot-builder implementation so the same security invariant follows the extracted seam.
- **Files modified:** `tests/unit/bounded_memory_policy_tests.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_dx10_writer|writer-stage|parser_preparer_seam"` passed with 50/50 tests.
- **Committed in:** `7feb845`

---

**Total deviations:** 1 auto-fixed (1 bug).
**Impact on plan:** The fix was required to keep an existing security/maintenance guard accurate after the planned seam move; no product scope expanded.

## Issues Encountered

- The initial affected-suite run failed only because the existing bounded-memory policy test still expected hardened snapshot directory code in the old coordinator file. The policy was updated to follow the extracted snapshot-builder seam, then the affected suite passed.

## Known Stubs

None.

## Threat Flags

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for 16-03: TES4 parser and BA2 DX10 preparer runtime seams now exist with focused coverage, so the next plan can add role-based seam policy guardrails and run the broader Phase 16 verification sweep.
- Phase 17 dedupe and DX10 temp-lifecycle requirements remain out of scope and pending.

## Self-Check: PASSED

- FOUND: `src/formats/ba2/ba2_dx10_snapshot_builder.hpp`
- FOUND: `src/formats/ba2/ba2_dx10_snapshot_builder.cpp`
- FOUND: `src/formats/ba2/ba2_dx10_chunk_assembler.hpp`
- FOUND: `src/formats/ba2/ba2_dx10_chunk_assembler.cpp`
- FOUND: `tests/unit/ba2_dx10_preparer_seam_tests.cpp`
- FOUND COMMIT: `a5717e7`
- FOUND COMMIT: `7feb845`
- FOUND COMMIT: `41caf9e`

---
*Phase: 16-parser-and-preparer-seam-extraction*
*Completed: 2026-05-14*
