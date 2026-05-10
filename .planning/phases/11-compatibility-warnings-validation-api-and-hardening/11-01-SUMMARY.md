---
phase: 11-compatibility-warnings-validation-api-and-hardening
plan: 01
subsystem: public-api
tags: [cpp20, validation-api, public-headers, tdd, cmake]

requires:
  - phase: 03-through-10
    provides: Public archive metadata, result/error contracts, writer APIs, and public include-boundary test patterns.
provides:
  - Dependency-light public validation API contract in `include/libbsa/validation.hpp`.
  - Umbrella header exposure for validation types and `validate_archive`.
  - Public include-boundary assertions for stable validation diagnostics and warning identifiers.
  - CMake public header file-set registration for the validation header.
affects: [phase-11-validation-api, public-api, package-consumer, compatibility-warnings]

tech-stack:
  added: []
  patterns:
    - C++20 public API surface using libbsa-owned `result<T>`, `error_code`, and metadata types.
    - TDD RED/GREEN public include-boundary gate for header-only contract introduction.

key-files:
  created:
    - include/libbsa/validation.hpp
  modified:
    - CMakeLists.txt
    - include/libbsa/libbsa.hpp
    - tests/unit/public_include_boundary_tests.cpp

key-decisions:
  - "Plan 11-01 introduces the public validation contract only; validation behavior and the function definition remain for the later implementation plan."
  - "Public compatibility warnings expose stable code and severity values while keeping byte offsets, record indexes, and chunk indexes out of the public report model."

patterns-established:
  - "Validation public headers include only C++ standard headers plus `libbsa/archive.hpp` and `libbsa/result.hpp`."
  - "Public API boundary tests assert stable programmatic identifiers and exact type shapes, not diagnostic message strings."

requirements-completed: [COMP-03]

duration: 4 min
completed: 2026-05-10
---

# Phase 11 Plan 01: Public Validation Contract Summary

**Dependency-light C++20 validation report and compatibility-warning contract exposed through the public umbrella header**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-10T03:50:51Z
- **Completed:** 2026-05-10T03:55:01Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Added RED public include-boundary assertions for `validation_report`, `validation_options`, `validation_diagnostic`, `compatibility_warning`, warning enums, and `validate_archive`.
- Added `include/libbsa/validation.hpp` with Doxygen-documented public validation diagnostics, warnings, options, report, and `validate_archive` declaration.
- Exposed validation through `include/libbsa/libbsa.hpp` and registered the header in the `libbsa` public CMake file set.
- Verified the public header has no forbidden private dependency tokens.

## Task Commits

Each task was committed atomically:

1. **Task 1: RED public validation boundary expectations** - `e2fe659` (test)
2. **Task 2: GREEN dependency-light validation header contract** - `1246f20` (feat)

_Note: This TDD plan produced RED and GREEN commits. No refactor commit was needed._

## Files Created/Modified

- `include/libbsa/validation.hpp` - Public validation diagnostics, warning codes, warning severity, options, report, and `validate_archive` declaration.
- `include/libbsa/libbsa.hpp` - Umbrella include for the validation API.
- `CMakeLists.txt` - Public file-set registration for `validation.hpp`.
- `tests/unit/public_include_boundary_tests.cpp` - Compile-time public API assertions for the validation contract.

## Verification

- `cmake --fresh --preset windows-msvc-debug-static` - passed; refreshed stale build cache from the prior checkout path.
- RED: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - failed as expected before Task 2 because `libbsa::validation_report`, `libbsa::compatibility_warning_code`, and related validation symbols did not exist.
- GREEN: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` - passed.
- `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` - passed, 3/3 tests.
- `rg -n "validate_archive" tests/unit/public_include_boundary_tests.cpp` - found the public API assertion.
- `rg -n "compatibility_warning_code|compatibility_warning_severity|validation_report" tests/unit/public_include_boundary_tests.cpp` - found stable warning/report assertions.
- `rg -n "enum class compatibility_warning_code" include/libbsa/validation.hpp` - found the warning enum.
- `rg -n "compressed_sound_payload|bsa_embedded_name_compatibility_risk|target_family_mismatch" include/libbsa/validation.hpp` - found all initial warning codes.
- `rg -n "struct validation_options|expected_type|expected_variant|validate_entry_extractability" include/libbsa/validation.hpp` - found the locked small option surface.
- `rg -n "result<validation_report> validate_archive" include/libbsa/validation.hpp` - found the public validation entry point declaration.
- `rg -n "#include <libbsa/validation.hpp>" include/libbsa/libbsa.hpp` - found the umbrella include.
- `rg -n "include/libbsa/validation.hpp" CMakeLists.txt` - found the public header file-set entry.
- `rg -n "std::expected|libdeflate|lz4|DirectXTex|DXGI|Windows.h|TES5Edit|bethesda_hash|compression_router" include/libbsa/validation.hpp` - returned no matches.
- `git status --short TES5Edit` - returned no output.

## Decisions Made

- Kept `validate_archive` declaration-only in this plan, matching the plan boundary. The strict-open-backed implementation is planned for the next validation API plan.
- Kept warning records limited to code, severity, message, and optional archive path so public reports do not expose parser offsets, record indexes, or chunk indexes.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Refreshed stale CMake cache before RED verification**
- **Found during:** Task 1 (RED public validation boundary expectations)
- **Issue:** The existing `build/windows-msvc-debug-static` cache still referenced `J:/libbsa-gsd`, so the first build failed before compiling the RED test.
- **Fix:** Ran `cmake --fresh --preset windows-msvc-debug-static` to regenerate the local preset build tree for `J:/libbsa`.
- **Files modified:** None tracked.
- **Verification:** Reran `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`; it then failed for the intended missing validation API symbols.
- **Committed in:** Not applicable; generated build-tree repair only.

---

**Total deviations:** 1 auto-fixed (1 blocking).
**Impact on plan:** Verification environment repair only; no product scope change.

## Issues Encountered

- The local CMake cache pointed at a prior checkout path. It was refreshed with the repo preset before RED/GREEN validation.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Ready for Plan 11-02 to add the strict-open-backed `validate_archive` implementation and validation API behavior tests.

## Self-Check: PASSED

- Found created/modified files: `include/libbsa/validation.hpp`, `include/libbsa/libbsa.hpp`, `CMakeLists.txt`, `tests/unit/public_include_boundary_tests.cpp`, and this SUMMARY.
- Found task commits: `e2fe659` and `1246f20`.
- Verification commands listed above passed except for the intentional RED failure before implementation.

---
*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Completed: 2026-05-10*
