---
phase: 14-verification-lane-truthfulness
plan: 01
subsystem: testing
tags: [cmake, ctest, presets, msvc, asan]
requires:
  - phase: 13-host-path-correctness-boundary
    provides: Windows-only verification baseline and existing public regression infrastructure
provides:
  - Checked-in Release static/shared preset triads
  - Checked-in MSVC AddressSanitizer static preset triad with real /fsanitize=address instrumentation
  - Shared verification-matrix policy contract for presets, ASan wiring, and CTest-owned package proof
affects: [14-02, 14-03, ci, docs]
tech-stack:
  added: []
  patterns: [shared verification-matrix contract helper, CTest-owned package proof, MSVC ASan runtime DLL propagation]
key-files:
  created: [.planning/phases/14-verification-lane-truthfulness/14-01-SUMMARY.md]
  modified: [CMakeLists.txt, CMakePresets.json, tests/CMakeLists.txt, tests/unit/validation_policy_tests.cpp, tests/package-consumer/CMakeLists.txt, tests/package-consumer/copy-runtime-dlls.cmake, tests/package-consumer/smoke.cmake]
key-decisions:
  - "Release package proof stays inside the existing CTest-owned package_consumer_smoke and package_consumer_runtime_dll_copy path."
  - "The supported MSVC ASan lane keeps real /fsanitize=address instrumentation while disabling STL annotation ODR mismatches against prebuilt dependencies."
patterns-established:
  - "Pattern: model supported verification lanes once in validation_policy_tests.cpp and assert each surface independently against that contract."
  - "Pattern: copy ASan and transitive runtime DLLs into test and package-consumer outputs so Catch2 discovery and downstream smoke remain runnable from checked-in lanes."
requirements-completed: []
duration: 1h 05m
completed: 2026-05-14
---

# Phase 14 Plan 01: Verification Lane Truthfulness Summary

**Runnable Release static/shared preset triads plus a real MSVC ASan hardening lane backed by policy-checked CTest package proof**

## Performance

- **Duration:** 1h 05m
- **Started:** 2026-05-14T02:42:51Z
- **Completed:** 2026-05-14T03:47:35Z
- **Tasks:** 3
- **Files modified:** 7

## Accomplishments
- Added a shared verification-matrix contract helper and failing RED policy checks for Release and ASan lane facts.
- Implemented checked-in Release static/shared and MSVC ASan preset families plus root CMake ASan wiring.
- Kept package-consumer proof inside CTest and made the ASan package-smoke path runnable by copying required runtime DLLs.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED — add a shared verification-matrix contract helper and failing policy assertions for Release and ASan lane facts** - `df58fef` (test)
2. **Task 2: GREEN — implement Release preset families, CTest-owned package proof, and real MSVC ASan wiring** - `beb8c66` (feat)
3. **Task 3: REFACTOR — keep the verification contract helper maintainable and phase-scoped** - `2b32a5a` (refactor)
4. **Verification follow-up: keep the ASan package smoke lane runnable** - `b57ffac` (fix)

**Plan metadata:** pending final docs commit

## Files Created/Modified
- `CMakePresets.json` - defines matching configure/build/test triads for debug, release static/shared, and ASan static lanes.
- `CMakeLists.txt` - adds checked-in MSVC ASan enablement and documents why instrumentation must live in CMake rather than preset names alone.
- `tests/CMakeLists.txt` - keeps package proof in the normal CTest graph and copies ASan runtime DLLs for discovery/test executables.
- `tests/unit/validation_policy_tests.cpp` - centralizes the supported verification matrix contract and asserts presets, CMake, and CTest surfaces independently.
- `tests/package-consumer/CMakeLists.txt` - copies ASan runtime DLLs into the downstream consumer output.
- `tests/package-consumer/copy-runtime-dlls.cmake` - supports optional extra runtime DLL copies without breaking the empty-list no-op contract.
- `tests/package-consumer/smoke.cmake` - mirrors transitive runtime DLLs from the configured prefixes before the downstream consumer ctest run.

## Decisions Made
- Keep Release package-proof ownership on the existing `package_consumer_smoke` and `package_consumer_runtime_dll_copy` tests instead of inventing a second smoke pipeline.
- Keep the ASan lane Windows/MSVC-specific with real `/fsanitize=address` compilation while disabling STL container annotations that would otherwise create LNK2038 ODR mismatches against prebuilt dependencies.
- Treat runtime DLL propagation as part of the supported verification lane contract so Catch2 discovery and package-consumer smoke run from checked-in outputs without caller PATH assumptions.

## TDD Gate Compliance

- PASS: `test(14-01)` (`df58fef`) → `feat(14-01)` (`beb8c66`) → `refactor(14-01)` (`2b32a5a`).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed MSVC ASan STL annotation link mismatches**
- **Found during:** Task 3 verification / plan-level ASan lane verification
- **Issue:** The new ASan lane failed with `LNK2038` `annotate_string` / `annotate_vector` / `annotate_optional` mismatches when instrumented targets linked prebuilt dependencies.
- **Fix:** Kept real `/fsanitize=address` instrumentation enabled, but disabled STL annotation ODR checks for the supported lane and documented why in root CMake.
- **Files modified:** `CMakeLists.txt`
- **Verification:** `cmake --preset windows-msvc-asan-static && cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure`
- **Committed in:** `b57ffac`

**2. [Rule 3 - Blocking] Fixed ASan runtime DLL propagation for discovery and downstream smoke**
- **Found during:** Task 3 verification / plan-level ASan lane verification
- **Issue:** Catch2 pre-test discovery and the package-consumer smoke executable could not run reliably until ASan and transitive runtime DLLs were copied beside built binaries.
- **Fix:** Extended the existing runtime-DLL copy helpers and smoke flow to copy ASan runtime DLLs plus transitive vcpkg runtime DLLs into test and consumer outputs.
- **Files modified:** `tests/CMakeLists.txt`, `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/copy-runtime-dlls.cmake`, `tests/package-consumer/smoke.cmake`
- **Verification:** `ctest --preset windows-msvc-asan-static -R "package_consumer" --output-on-failure` and full ASan lane `ctest --preset windows-msvc-asan-static --output-on-failure`
- **Committed in:** `b57ffac`

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both auto-fixes were required to make the supported ASan lane truthful and runnable. No scope creep beyond the locked verification-lane contract.

## Issues Encountered
- The first ASan implementation passed fast policy checks but failed full-lane verification because prebuilt dependencies were not built with matching STL annotations.
- After the link issue was fixed, the downstream consumer smoke path still needed explicit runtime DLL propagation for ASan and transitive shared dependencies.

## Known Stubs

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 14 plan 02 can now wire CI to the already-runnable Release and ASan preset families.
- Phase 14 plan 03 can align README, fixture policy, and planning summaries against the now-enforced verification matrix contract.
- `VER-01`, `VER-02`, and `VER-03` were advanced substantially here, but they are not marked complete yet because CI and documentation alignment are still assigned to plans 02 and 03.

## Self-Check: PASSED

- Found summary file: `.planning/phases/14-verification-lane-truthfulness/14-01-SUMMARY.md`
- Found commit: `df58fef`
- Found commit: `beb8c66`
- Found commit: `2b32a5a`
- Found commit: `b57ffac`

---
*Phase: 14-verification-lane-truthfulness*
*Completed: 2026-05-14*
