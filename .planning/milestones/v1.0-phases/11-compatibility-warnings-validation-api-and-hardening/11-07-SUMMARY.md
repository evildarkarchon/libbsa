---
phase: 11-compatibility-warnings-validation-api-and-hardening
plan: 07
subsystem: build-hardening
tags: [cpp20, cmake-presets, ctest, sanitizer, validation-policy]

requires:
  - phase: 11-04
    provides: Machine-checked compatibility evidence policy and fixture documentation.
  - phase: 11-06
    provides: Consolidated malformed matrix and validation/compression label coverage.
provides:
  - Additive `linux-clang-asan-ubsan` configure, build, and test presets.
  - Fixture policy documentation for the sanitizer malformed/validation/compression command path.
  - Policy tests proving the sanitizer path is additive and default Windows CI remains unchanged.
affects: [phase-11-hardening, validation-policy, cmake-presets, fixture-policy]

tech-stack:
  added: []
  patterns:
    - Sanitizer hardening is opt-in through dedicated CMake presets, not default Windows CI.
    - Policy tests enforce build-path documentation and CI boundary preservation with source text checks.

key-files:
  created:
    - .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-07-SUMMARY.md
  modified:
    - CMakePresets.json
    - tests/fixtures/README.md
    - tests/unit/validation_policy_tests.cpp

key-decisions:
  - "The sanitizer hardening path is additive through `linux-clang-asan-ubsan` presets and is not added to the default Windows MSVC CI workflow."
  - "Fixture policy documents the exact CTest label selection for malformed, validation, and compression sanitizer runs."

patterns-established:
  - "Hardening-only toolchain paths must be machine-checked for documentation and kept out of default Windows static/shared CI unless a later plan explicitly adds a CI lane."

requirements-completed: [COMP-05, COMP-06]

duration: 2 min
completed: 2026-05-10
---

# Phase 11 Plan 07: Sanitizer Hardening Path Summary

**Additive Clang ASan/UBSan preset and documented malformed validation command path without changing Windows CI**

## Performance

- **Duration:** 2 min
- **Started:** 2026-05-10T04:33:11Z
- **Completed:** 2026-05-10T04:34:53Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added a RED `validation_policy` test named `sanitizer validation path is additive and documented`.
- Added `linux-clang-asan-ubsan` configure, build, and test presets with Clang `-fsanitize=address,undefined` and `-fno-omit-frame-pointer` settings.
- Documented the exact sanitizer command path in `tests/fixtures/README.md` while keeping `.github/workflows/ci.yml` unchanged.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add policy tests for sanitizer path and CI preservation** - `dd2c88c` (test)
2. **Task 2: Add additive sanitizer preset and fixture documentation** - `f37e93d` (feat)

_Note: This plan used a RED policy-test commit followed by a GREEN preset/docs commit. No refactor commit was needed._

## Files Created/Modified

- `CMakePresets.json` - Adds the opt-in `linux-clang-asan-ubsan` configure/build/test presets.
- `tests/fixtures/README.md` - Documents the sanitizer malformed/validation/compression command path and Windows CI boundary.
- `tests/unit/validation_policy_tests.cpp` - Adds the machine check for sanitizer preset/docs and CI preservation.

## Verification

- Task 1 acceptance: `rg -n "sanitizer validation path is additive and documented" tests/unit/validation_policy_tests.cpp` - found the new policy test.
- Task 1 acceptance: `rg -n "linux-clang-asan-ubsan|-fsanitize=address,undefined|windows-msvc-debug-static|windows-msvc-debug-shared" tests/unit/validation_policy_tests.cpp` - found required tokens in the new test.
- RED: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "sanitizer validation path|validation_policy" --output-on-failure` - build passed and CTest failed as expected because `CMakePresets.json` did not yet contain `linux-clang-asan-ubsan`.
- Task 2 acceptance: `rg -n "\"name\": \"linux-clang-asan-ubsan\"" CMakePresets.json` - found configure, build, and test preset entries.
- Task 2 acceptance: `rg -n -- "-fsanitize=address,undefined|-fno-omit-frame-pointer|CMAKE_CXX_COMPILER|clang\\+\\+" CMakePresets.json` - found sanitizer compiler/linker settings.
- Task 2 acceptance: `rg -n "ctest --preset linux-clang-asan-ubsan -L \"malformed\\|validation\\|compression\" --output-on-failure" tests/fixtures/README.md` - found the documented command.
- Task 2 acceptance: `rg -n "linux-clang-asan-ubsan" .github/workflows/ci.yml` - returned no matches, proving default CI was not changed.
- Task 2 acceptance: `git status --short TES5Edit` - returned no output.
- Plan-level: `cmake --list-presets=all` - listed the existing Windows presets and new `linux-clang-asan-ubsan` configure/build/test presets.
- Plan-level: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- Plan-level: `ctest --preset windows-msvc-debug-static -R "sanitizer validation path|validation_policy|public_include_boundary" --output-on-failure` - passed, 4/4 tests.
- Plan-level: `git status --short TES5Edit` - returned no output.

## Decisions Made

- Kept the sanitizer path preset-only and documentation-backed instead of adding a GitHub Actions lane, matching the plan's default Windows CI boundary.
- Used source-text policy tests for CI preservation because this plan changes build/test configuration rather than runtime archive behavior.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- The sanitizer configure/build/test path was not executed locally because this session ran on Windows and the new preset is documented for supported non-Windows Clang/GCC-style toolchains. The fallback/static acceptance path was `cmake --list-presets=all`, the focused Windows MSVC build, policy CTest, source-text checks, and `git status --short TES5Edit`.

## Known Stubs

None. Stub scan found no TODO, FIXME, placeholder, empty hardcoded UI data, or "coming soon" markers in touched files.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Phase 11 implementation is complete. The project is ready for Phase 11 verification and Phase 12 planning around performance, concurrency, documentation, and polish.

## Self-Check: PASSED

- Found created/modified files: `CMakePresets.json`, `tests/fixtures/README.md`, `tests/unit/validation_policy_tests.cpp`, and this SUMMARY.
- Found task commits: `dd2c88c` and `f37e93d`.
- Verification commands listed above passed except for the intentional RED failure before the sanitizer preset/docs existed.
- Stub scan found no placeholders that block the plan goal.
- Threat surface scan found no new network endpoints, auth paths, runtime file-access surfaces, or schema changes at trust boundaries.

---
*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Completed: 2026-05-10*
