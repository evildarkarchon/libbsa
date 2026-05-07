---
phase: 08-writer-planning-streaming-emit-and-dedup-core
plan: 03
subsystem: writer-planning
tags: [cpp20, writer, tdd, catch2, dedup, layout]

requires:
  - phase: 08-writer-planning-streaming-emit-and-dedup-core
    provides: Deterministic writer planning, post-policy stored payload ownership, and checked layout preview from Plan 02
provides:
  - Opt-in post-policy stored payload deduplication for targets that support shared data regions
  - Structured unsupported-format failure when deduplication is requested for unsupported writer targets
  - Stable numeric data-region IDs and shared archive-absolute offsets for duplicate stored payloads
affects: [phase-08-writer-finalization, phase-08-harness-readback, phase-09-bsa-writers, phase-10-ba2-writers]

tech-stack:
  added: []
  patterns:
    - RED/GREEN TDD for writer dedup semantics
    - Standard-container exact-byte grouping without exposing public digests
    - Post-policy grouping after compression and stored-payload creation

key-files:
  created:
    - .planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-03-SUMMARY.md
  modified:
    - include/libbsa/writer.hpp
    - src/writer.cpp
    - tests/writer_core_tests.cpp

key-decisions:
  - "Writer deduplication groups exact post-policy stored payload bytes only after compression routing succeeds."
  - "Unsupported deduplication requests fail early with unsupported_format instead of silently emitting non-dedup output."
  - "Public writer comments use digest-neutral wording so no public content hash surface is implied."

patterns-established:
  - "Dedup region IDs are assigned in first sorted-entry occurrence order and shared entries reuse the first region offset."
  - "When deduplication is disabled, writer planning preserves one data region per sorted entry even for duplicate stored bytes."

requirements-completed: [WRT-06, WRT-07]

duration: 3min
completed: 2026-05-07
---

# Phase 08 Plan 03: Post-Policy Payload Deduplication Summary

**Opt-in stored-payload deduplication with stable data-region IDs and unsupported-target failure semantics**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-07T00:49:35Z
- **Completed:** 2026-05-07T00:52:25Z
- **Tasks:** 2 completed
- **Files modified:** 4

## Accomplishments

- Added RED Catch2 coverage for enabled dedup sharing, disabled duplicate-region behavior, and unsupported shared-region targets.
- Implemented post-policy exact-byte grouping after stored payload creation so duplicate entries share one data region only when requested and supported.
- Preserved deterministic plan metadata by assigning numeric data-region IDs in first sorted-entry occurrence order and reusing shared archive-absolute offsets.

## TDD Evidence

- **RED:** `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_writer_tests && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` failed because the planner emitted three data regions for duplicate bytes and allowed dedup on a target with shared regions disabled.
- **GREEN:** The same focused build and CTest command passed after implementing dedup grouping and the unsupported-target failure.
- **REFACTOR:** No separate refactor commit was needed; the GREEN implementation remained focused.

## Task Commits

Each TDD gate was committed atomically:

1. **Task 1: RED dedup behavior tests** - `62dd677` (test)
2. **Task 2: GREEN implement post-policy dedup grouping** - `fe98898` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified

- `tests/writer_core_tests.cpp` - Adds three writer planning tests for enabled dedup sharing, disabled duplicate payload distinctness, and unsupported shared-region failure.
- `src/writer.cpp` - Groups exact stored payload bytes when deduplication is enabled, computes payload offsets after grouping, and returns `writer target does not support deduplication` for unsupported targets.
- `include/libbsa/writer.hpp` - Rewords existing public comments from content-hash terminology to digest-neutral wording so the public API does not imply a public content identity field.
- `.planning/phases/08-writer-planning-streaming-emit-and-dedup-core/08-03-SUMMARY.md` - Records execution results and TDD evidence.

## Decisions Made

- Used standard vector scans for exact stored-payload comparison, avoiding any new dependency or public digest field.
- Failed unsupported dedup requests before normalization/compression work because target capability validation is a request-level correctness gate.
- Kept disabled dedup behavior as one region per sorted entry to preserve the option's observable layout effect.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Reworded public comments that matched the no-public-hash acceptance gate**
- **Found during:** Task 2 (GREEN implement post-policy dedup grouping)
- **Issue:** The plan's acceptance grep for `hash|content_hash|payload_hash` matched existing writer public comments, even though no public field exposed such data.
- **Fix:** Rewrote the relevant comments to use digest-neutral wording without changing API behavior.
- **Files modified:** `include/libbsa/writer.hpp`
- **Verification:** `rg -n "hash|content_hash|payload_hash" include/libbsa/writer.hpp src/writer.cpp tests/writer_core_tests.cpp` returned no matches.
- **Committed in:** `fe98898` (Task 2 GREEN commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** Acceptance gate was satisfied without changing the public API or implementation semantics.

## Known Stubs

| File | Line | Stub | Reason |
|------|------|------|--------|
| `src/writer.cpp` | 47 | `writer finalization is not implemented` structured failure | Intentional carry-forward from Plan 01; Plan 04 owns streaming finalization behavior. |
| `include/libbsa/writer.hpp` | 118 | Finalization documentation references later placeholder replacement | Intentional carry-forward until Plan 04 replaces finalization behavior. |

## Issues Encountered

- The no-public-hash acceptance grep included comments, so existing public comments were rewritten to avoid implying any public content identity field.

## User Setup Required

None - no external service configuration required.

## Verification

- `rg -n "deduplicates byte-identical stored payloads when enabled|keeps duplicate payloads distinct when deduplication is disabled|rejects deduplication when target disallows shared regions" tests/writer_core_tests.cpp` - passed.
- `cmake --build build/local-vs2026-vcpkg --config Debug --target libbsa_writer_tests && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_writer_tests` - RED failed before implementation, then GREEN passed 11/11 writer tests.
- `rg -n "writer target does not support deduplication|deduplicate|supports_shared_data_regions|data_region_id" src/writer.cpp` - passed.
- `rg -n "hash|content_hash|payload_hash" include/libbsa/writer.hpp src/writer.cpp tests/writer_core_tests.cpp` - passed with no matches.
- `git log --oneline --grep="^test(08-03)" -1` and `git log --oneline --grep="^feat(08-03)" -1` found the RED and GREEN commits.
- `git status --short TES5Edit` reported no changes.

## TDD Gate Compliance

- **RED gate:** `62dd677 test(08-03): add failing tests for writer deduplication`
- **GREEN gate:** `fe98898 feat(08-03): implement writer payload deduplication`
- **REFACTOR gate:** Not needed.

## Next Phase Readiness

Ready for Plan 08-04 to stream the already-planned table and deduplicated payload regions through caller-owned `byte_sink` implementations.

## Self-Check: PASSED

- Created/modified files exist: `include/libbsa/writer.hpp`, `src/writer.cpp`, `tests/writer_core_tests.cpp`, and this summary.
- Task commits exist: `62dd677` and `fe98898`.

---
*Phase: 08-writer-planning-streaming-emit-and-dedup-core*
*Completed: 2026-05-07*
