# Phase 14: Verification Lane Truthfulness - Research

**Researched:** 2026-05-13
**Domain:** Windows MSVC CMake/CTest verification-lane hardening and repo-surface drift control [VERIFIED: repo files + official docs]
**Confidence:** MEDIUM [VERIFIED: repo files + official docs]

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### ASan Lane Role
- **D-01:** `windows-msvc-asan-static` is an official supported lane, but it is explicitly the `MSVC AddressSanitizer hardening lane`, not the everyday default path.
- **D-02:** CI should show ASan as its own clearly named hardening job rather than burying it as just another row in the main Windows matrix.
- **D-03:** Maintainer guidance should recommend the ASan lane for risky parser, writer, compression, and validation changes before shipping, not for every trivial local edit.
- **D-04:** Docs should explicitly distinguish the ASan hardening lane from the Release package-proof lanes instead of relying on preset names alone.

### Release Package Proof
- **D-05:** Both `windows-msvc-release-static` and `windows-msvc-release-shared` own the full supported package-proof contract.
- **D-06:** The supported Release proof stays inside each Release lane's `ctest` path instead of becoming a separate ad hoc CI-only script flow.
- **D-07:** Release-lane wording and policy checks must call out both parts of the contract: install/export generation and downstream package-consumer smoke.
- **D-08:** A package-consumer smoke failure is a blocking Release-lane failure even when core library tests passed.

### Matrix Communication
- **D-09:** Maintainer-facing docs should use a `quick path + matrix` structure rather than leading with the full matrix immediately.
- **D-10:** The quick day-to-day example remains `windows-msvc-debug-static`.
- **D-11:** After the quick path, the remaining supported lanes should be grouped by role: Debug inner-loop lanes, Release package-proof lanes, and the ASan hardening lane.
- **D-12:** README-level docs should show concise per-role command guidance rather than only preset names or fully repeated commands for every lane.

### Drift Guard Contract
- **D-13:** The truthfulness gate should lock exact contract facts while allowing prose style to vary. Exact facts include lane names, lane roles, Release smoke ownership, Windows-only scope, and the opt-in local-corpus rule.
- **D-14:** Every listed surface should independently name the same contract in its own appropriate form rather than merely pointing elsewhere.
- **D-15:** Policy tests must lock role facts, not just preset presence: Debug stays the quick path, Release lanes own package proof, and ASan is the named hardening lane.
- **D-16:** The same truthfulness gate must also keep `requires-game-fixture` opt-in and prevent mandatory local corpus checks from becoming part of the default supported matrix.

### Planning Surface Updates
- **D-17:** `.planning/PROJECT.md` and `.planning/ROADMAP.md` should carry a role-aware summary of the supported matrix, but not duplicate the full command-level contract.
- **D-18:** `STATE.md` should stay concise and session-oriented, with only a short note about the truthful supported matrix instead of full lane detail.
- **D-19:** `PROJECT.md` must fix the existing hardening-coverage history so the chronology stays truthful: v1.0 history remains accurate, and the supported Release/ASan matrix is described as a v1.1 Phase 14 outcome.
- **D-20:** The most detailed lane-role and drift-guard contract lives in `14-CONTEXT.md`, not in the project-wide planning summaries.

### CI Topology
- **D-21:** CI should use one main Windows matrix for Debug and Release lanes plus one separate ASan hardening job.
- **D-22:** `fail-fast` stays disabled so maintainers can see the full supported matrix result set in one run.
- **D-23:** The separate ASan hardening job should run in parallel with the main matrix rather than waiting for it.
- **D-24:** GitHub Actions job names should expose both lane role and concrete preset, not just one or the other.

### Policy Test Structure
- **D-25:** Phase 14 truthfulness checks stay inside `tests/unit/validation_policy_tests.cpp`, but they should be grouped into a focused verification-matrix section instead of being scattered ad hoc.
- **D-26:** That policy section should use one shared expected-contract table/helper set for supported lanes, lane roles, package-smoke expectations, and exclusions.
- **D-27:** Each surface should be checked independently against the shared contract so one stale file cannot validate another stale file.
- **D-28:** Any future supported-lane, role, smoke, or exclusion change must update the shared expected contract and all affected surfaces in the same PR.

### the agent's Discretion
None. The discussion intentionally locked the user-facing matrix contract, lane roles, CI shape, and drift-guard behavior closely enough that downstream research and planning should not reopen them.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| VER-01 | Maintainer can configure and run a checked-in Windows MSVC Release preset for libbsa builds and automated tests. [VERIFIED: `.planning/REQUIREMENTS.md`] | Add full configure/build/test preset triads for `windows-msvc-release-static` and `windows-msvc-release-shared`, wire both into the main CI matrix, and keep release package proof inside `ctest`. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`, `CMakePresets.json`, `.github/workflows/ci.yml`, `tests/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`] |
| VER-02 | Maintainer can configure and run a checked-in Windows MSVC ASan preset for libbsa builds and automated tests. [VERIFIED: `.planning/REQUIREMENTS.md`] | Add `windows-msvc-asan-static` preset triads plus checked-in CMake logic that enables MSVC `/fsanitize=address`; run it in a separate CI hardening job. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`, Microsoft Learn ASan docs] |
| VER-03 | Maintainer can rely on `.planning`, `CMakePresets.json`, CI, and policy tests to describe the same supported verification lanes. [VERIFIED: `.planning/REQUIREMENTS.md`] | Extend `tests/unit/validation_policy_tests.cpp` with one shared expected contract table/helper and independently validate README, fixture policy, presets, workflow, and planning summaries against it. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`, `tests/unit/validation_policy_tests.cpp`] |
</phase_requirements>

## Project Constraints (from AGENTS.md)

- Windows-only scope; do not add Linux/macOS/POSIX portability work. [VERIFIED: `AGENTS.md`, `CLAUDE.md`, `.planning/PROJECT.md`]
- `TES5Edit/` is read-only and must not be edited, formatted, staged, compiled into libbsa, or used as mutable fixture space. [VERIFIED: `AGENTS.md`, `README.md`, `tests/fixtures/README.md`]
- Keep implementation in C++20 with reusable library boundaries and no public API expansion in this milestone. [VERIFIED: `AGENTS.md`, `.planning/PROJECT.md`]
- Do not add speculative dependencies; stay on CMake, vcpkg, Catch2, CTest, libdeflate, lz4, and DirectXTex unless a documented requirement justifies more. [VERIFIED: `AGENTS.md`, `.planning/PROJECT.md`, `vcpkg.json`]
- Never delete accurate comments as cleanup; add Doxygen-style comments for public APIs or substantially rewritten methods. [VERIFIED: `AGENTS.md`]
- Prefer focused fixture-based and policy tests; do not keep product code around only for test compatibility. [VERIFIED: `AGENTS.md`]

## Summary

Phase 14 is primarily a repo-contract phase, not a product-surface phase: the code already has the important reusable proof pieces for package verification, but the checked-in supported matrix is still debug-only in presets, CI, README, fixture policy, and validation-policy tests. [VERIFIED: `CMakePresets.json`, `.github/workflows/ci.yml`, `README.md`, `tests/fixtures/README.md`, `tests/unit/validation_policy_tests.cpp`, `tests/CMakeLists.txt`]

The biggest reuse win is that release package proof does **not** need a new smoke framework. `tests/CMakeLists.txt` already registers `package_consumer_smoke` and `package_consumer_runtime_dll_copy`, `tests/package-consumer/smoke.cmake` already performs install → downstream configure → downstream build → downstream `ctest`, and the shared-build branch in `tests/CMakeLists.txt` already proves why downstream/package checks matter by compiling test support differently when `BUILD_SHARED_LIBS` is on. [VERIFIED: `tests/CMakeLists.txt`, `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`]

The plan should therefore be contract-first: make `validation_policy_tests.cpp` express the final lane contract in one shared helper/table, then land the matching preset families, CI topology, and maintainer/planning docs in the same wave so the matrix never goes half-truthful. [VERIFIED: `14-CONTEXT.md`, `14-SPEC.md`, `tests/unit/validation_policy_tests.cpp`]

**Primary recommendation:** Reuse the existing package-consumer smoke and export-surface checks, add one shared verification-matrix contract helper in `tests/unit/validation_policy_tests.cpp` first, then update `CMakePresets.json`, `.github/workflows/ci.yml`, `README.md`, `tests/fixtures/README.md`, `.planning/PROJECT.md`, `.planning/ROADMAP.md`, and `.planning/STATE.md` together in one coordinated slice. [VERIFIED: `14-CONTEXT.md`, `14-SPEC.md`, repo codebase]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Supported lane definition | `CMakePresets.json` [VERIFIED: repo file] | Root `CMakeLists.txt` [VERIFIED: repo file] | Presets define the maintainer-facing configure/build/test contract, while root CMake must expose the knobs those presets select. [VERIFIED: `CMakePresets.json`, `CMakeLists.txt`, CMake docs] |
| MSVC ASan enablement | Root `CMakeLists.txt` [VERIFIED: repo file] | `windows-msvc-asan-static` presets [VERIFIED: phase spec] | Microsoft documents that `CMakePresets.json` itself does not have an `addressSanitizerEnabled` behavior, so checked-in CMake logic must apply `/fsanitize=address` and the preset must select that path. [CITED: https://learn.microsoft.com/cpp/build/cmake-presets-vs?view=msvc-170#enable-addresssanitizer-for-windows-and-linux] |
| Release package proof | CTest test graph in `tests/CMakeLists.txt` [VERIFIED: repo file] | `tests/package-consumer/*` [VERIFIED: repo file] | The supported contract says release proof stays inside `ctest`, and the downstream smoke flow already lives there. [VERIFIED: `14-CONTEXT.md`, `tests/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`] |
| CI truthfulness | `.github/workflows/ci.yml` [VERIFIED: repo file] | `CMakePresets.json` [VERIFIED: repo file] | CI must execute the exact supported presets and expose the role split required by the phase. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`, `.github/workflows/ci.yml`] |
| Drift prevention | `tests/unit/validation_policy_tests.cpp` [VERIFIED: repo file] | README / fixture / planning summaries [VERIFIED: repo files] | The repo already treats docs/config as testable contracts; Phase 14 extends that pattern to the lane matrix. [VERIFIED: `tests/unit/validation_policy_tests.cpp`, `.planning/codebase/TESTING.md`] |

## Standard Stack

### Core
| Library / Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | 4.3.2 in CI and local environment. [VERIFIED: `.github/workflows/ci.yml`, `cmake --version`] | Checked-in configure/build/install/export orchestration. [VERIFIED: `CMakeLists.txt`, `CMakePresets.json`] | Presets, install/export generation, and package-consumer smoke already center on CMake; Phase 14 should extend that rather than introduce parallel scripts. [VERIFIED: repo files, CMake docs] |
| CTest | 4.3.2 in local environment; bundled with CMake in repo workflow. [VERIFIED: `ctest --version`, `.github/workflows/ci.yml`] | Supported lane test entrypoint. [VERIFIED: `README.md`, `14-SPEC.md`] | `ctest --preset ...` is already the maintained verification entrypoint and already includes package-consumer smoke. [VERIFIED: `README.md`, `tests/CMakeLists.txt`, `ctest --preset windows-msvc-debug-static -N`] |
| Catch2 | 3.14.0. [VERIFIED: `tests/CMakeLists.txt`, `vcpkg x-package-info catch2`] | Policy tests and repo-surface drift checks. [VERIFIED: `tests/unit/validation_policy_tests.cpp`] | Existing policy suites already read repo files directly with Catch2; that is the right place to keep the truthfulness gate. [VERIFIED: `tests/unit/validation_policy_tests.cpp`, `.planning/codebase/TESTING.md`] |
| GitHub Actions | `windows-latest` workflow with CMake 4.3.2 pin. [VERIFIED: `.github/workflows/ci.yml`] | Supported CI execution for declared lanes. [VERIFIED: `14-SPEC.md`] | The phase explicitly locks one main Windows matrix plus a separate ASan job, so GitHub Actions remains the authoritative automation surface. [VERIFIED: `14-CONTEXT.md`] |

### Supporting
| Library / Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| vcpkg manifest mode | Baseline `12dcccadfe573d0eaa6c67a968413ded7805d256`. [VERIFIED: `vcpkg-configuration.json`] | Dependency restoration during configure. [VERIFIED: `CMakePresets.json`, `vcpkg.json`] | Required for every preset lane because configure runs `vcpkg install`. [VERIFIED: `cmake --preset windows-msvc-debug-static` output] |
| Python | 3.14.5 available locally. [VERIFIED: `python --version`] | `validate_fixture_manifests` CTest entry. [VERIFIED: `tests/CMakeLists.txt`] | Needed because full lane `ctest` includes manifest validation. [VERIFIED: `tests/CMakeLists.txt`, `ctest --preset windows-msvc-debug-static -N`] |
| Package-consumer smoke project | checked in under `tests/package-consumer/`. [VERIFIED: repo files] | Install/export and downstream `find_package(libbsa CONFIG REQUIRED)` proof. [VERIFIED: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`] | Reuse this instead of inventing a new release-only smoke harness. [VERIFIED: repo files] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Extending `validation_policy_tests.cpp` [VERIFIED: repo file] | A new standalone policy executable [ASSUMED] | Adds a second truth gate and breaks the established repo pattern that policy/docs/config checks live in the main Catch2 suite. [VERIFIED: `14-CONTEXT.md`, `.planning/codebase/TESTING.md`] |
| Reusing `package_consumer_smoke` [VERIFIED: repo file] | A CI-only ad hoc install script [ASSUMED] | Violates locked Decision D-06 and makes local proof diverge from CI proof. [VERIFIED: `14-CONTEXT.md`] |
| MSVC `/fsanitize=address` [CITED: https://learn.microsoft.com/cpp/sanitizers/asan-building?view=msvc-170#compiler] | Clang/Linux sanitizer lanes [VERIFIED: out-of-scope spec] | Explicitly out of scope and would make the supported matrix untruthful for this Windows-only repo. [VERIFIED: `14-SPEC.md`, `tests/fixtures/README.md`, `AGENTS.md`] |

**Installation / verification commands:**
```powershell
cmake --list-presets
ctest --list-presets
cmake --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static -N
```

**Version verification:** `cmake --version` returned `4.3.2`, `ctest --version` returned `4.3.2`, and `vcpkg x-package-info` reported Catch2 `3.14.0`, nlohmann-json `3.12.0#2`, libdeflate `1.25`, lz4 `1.10.0`, and DirectXTex `2026-03-31`. [VERIFIED: tool output]

## Architecture Patterns

### System Architecture Diagram

```text
Maintainer command / CI job
        |
        v
CMakePresets.json  ----->  root CMake logic (lane knobs, install/export)
        |                               |
        |                               v
        |                        build tree + CTest graph
        |                               |
        |                               +--> libbsa_tests (policy/docs/source checks)
        |                               +--> package_consumer_smoke
        |                               +--> package_consumer_runtime_dll_copy
        |                               +--> shared_export_surface (shared lanes only)
        |
        v
.github/workflows/ci.yml -----> executes supported presets by role
        |
        v
README.md / tests/fixtures/README.md / .planning summaries
        |
        v
validation_policy_tests.cpp reads each surface independently against one expected contract
```

The critical flow is `preset definition -> build/test execution -> documentation/planning summary -> policy gate`, and Phase 14 is truthful only when all four surfaces agree. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`, repo files]

### Recommended Project Structure
```text
repo root
├── CMakePresets.json                     # supported lane families
├── CMakeLists.txt                        # lane-selectable ASan/install/export behavior
├── .github/workflows/ci.yml              # matrix + separate ASan hardening job
├── README.md                             # quick path + role-grouped matrix
├── tests/fixtures/README.md              # fixture/policy matrix summary
├── tests/CMakeLists.txt                  # CTest ownership of package proof
├── tests/package-consumer/               # downstream smoke proof
├── tests/unit/validation_policy_tests.cpp# shared truthfulness contract
└── .planning/{PROJECT,ROADMAP,STATE}.md  # summary-level planning alignment
```

### Likely Plan Slices
1. **Contract-first tests:** add one shared expected-matrix helper/table in `tests/unit/validation_policy_tests.cpp` and make it fail against the current debug-only repo. [VERIFIED: `14-CONTEXT.md`, current repo files]
2. **Preset/CMake plumbing:** add Release static/shared and ASan static triads; add the checked-in CMake toggle for MSVC ASan. [VERIFIED: `14-SPEC.md`, `CMakePresets.json`, Microsoft Learn ASan docs]
3. **CTest ownership check:** keep release package proof in `ctest`; if needed, use test-preset filtering/labels rather than new scripts. [VERIFIED: `tests/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`, CMake preset docs]
4. **CI topology:** expand the main matrix to Debug + Release and add a separate parallel ASan job with role-aware names. [VERIFIED: `14-CONTEXT.md`, `.github/workflows/ci.yml`]
5. **Docs/planning alignment:** update README, fixture policy, and planning summaries in the same PR as the code/test changes. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`]

### Pattern 1: One shared expected contract for all truthfulness checks
**What:** Keep lane names, roles, package-smoke expectations, and exclusions in one file-local helper/table inside `tests/unit/validation_policy_tests.cpp`, then have each test read its own source file independently. [VERIFIED: `14-CONTEXT.md`, `tests/unit/validation_policy_tests.cpp`]
**When to use:** First. This is the phase's TDD anchor because `workflow.tdd_mode` is enabled and the phase goal is source-policy/build-lane truthfulness. [VERIFIED: `.planning/config.json`, `14-CONTEXT.md`]
**Example:**
```cpp
// Source: 14-CONTEXT.md + tests/unit/validation_policy_tests.cpp
struct expected_lane_contract {
  std::string_view preset;
  std::string_view role;
  bool owns_package_smoke;
};

constexpr std::array k_supported_lanes{
    expected_lane_contract{"windows-msvc-debug-static", "debug quick path", false},
    expected_lane_contract{"windows-msvc-debug-shared", "debug inner-loop lane", false},
    expected_lane_contract{"windows-msvc-release-static", "release package-proof lane", true},
    expected_lane_contract{"windows-msvc-release-shared", "release package-proof lane", true},
    expected_lane_contract{"windows-msvc-asan-static", "MSVC AddressSanitizer hardening lane", false},
};
```

### Pattern 2: Full preset families, not name-only additions
**What:** Each supported lane needs matching `configurePresets`, `buildPresets`, and `testPresets` entries with the same lane name. [VERIFIED: `14-SPEC.md`, current `CMakePresets.json`, CMake preset docs]
**When to use:** For every new lane added by Phase 14. [VERIFIED: `14-SPEC.md`]
**Example:**
```json
// Source: https://github.com/kitware/cmake/blob/master/Help/manual/cmake-presets.7.rst
{
  "name": "windows-msvc-release-static",
  "binaryDir": "${sourceDir}/build/${presetName}",
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Release",
    "LIBBSA_BUILD_TESTS": "ON",
    "BUILD_SHARED_LIBS": "OFF"
  }
}
```

### Pattern 3: Release proof stays inside `ctest`
**What:** Keep install/export and downstream consumer proof in CTest, reusing `package_consumer_smoke` and `package_consumer_runtime_dll_copy` rather than inventing a second pipeline. [VERIFIED: `tests/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`, `14-CONTEXT.md`]
**When to use:** For both release lanes. [VERIFIED: `14-CONTEXT.md`]
**Example:**
```cmake
# Source: tests/CMakeLists.txt
add_test(
  NAME package_consumer_smoke
  COMMAND ${CMAKE_COMMAND}
    -DLIBBSA_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DLIBBSA_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/package-consumer-prefix
    -DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/package-consumer
    -DCONFIG=$<CONFIG>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/package-consumer/smoke.cmake
)
```

### Pattern 4: MSVC ASan must be enabled by checked-in CMake logic
**What:** `CMakePresets.json` can select a lane, but the actual MSVC AddressSanitizer flag must come from CMake/compiler flags, not a preset-only Visual Studio toggle. [CITED: https://learn.microsoft.com/cpp/build/cmake-presets-vs?view=msvc-170#enable-addresssanitizer-for-windows-and-linux]
**When to use:** `windows-msvc-asan-static`. [VERIFIED: `14-SPEC.md`]
**Example:**
```cmake
# Source: Microsoft Learn ASan guidance + repo CMake layout
if(MSVC AND LIBBSA_ENABLE_MSVC_ASAN)
  target_compile_options(libbsa PUBLIC /fsanitize=address)
endif()
```

### Anti-Patterns to Avoid
- **Docs-only lane additions:** Adding lane names to README or `.planning` before presets/CI/tests land recreates the exact drift this phase exists to remove. [VERIFIED: `14-SPEC.md`, `.planning/PROJECT.md`, current repo files]
- **CI-only smoke scripts:** A GitHub-only install/export step breaks D-06 because maintainers cannot run the same release proof locally through `ctest`. [VERIFIED: `14-CONTEXT.md`]
- **ASan by non-Windows sanitizer flags:** `-fsanitize=address`/Linux-only wording would violate the Windows-only/MSVC-only contract. [VERIFIED: `14-SPEC.md`, `tests/fixtures/README.md`, `AGENTS.md`]
- **Scattered policy assertions:** The current file already has ad hoc debug-only checks; Phase 14 should consolidate instead of appending more isolated `find(...)` assertions. [VERIFIED: `tests/unit/validation_policy_tests.cpp`, `14-CONTEXT.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Release package proof | A new shell script or CI-only smoke workflow [ASSUMED] | `package_consumer_smoke` + `package_consumer_runtime_dll_copy` already in `tests/CMakeLists.txt` [VERIFIED: repo files] | The existing flow already installs the built package, configures a downstream consumer, builds it, and runs downstream `ctest`. [VERIFIED: `tests/package-consumer/smoke.cmake`] |
| Truthfulness gate | A second policy test binary [ASSUMED] | Extend `tests/unit/validation_policy_tests.cpp` [VERIFIED: `14-CONTEXT.md`] | The repo already treats docs/config surfaces as Catch2-governed contracts. [VERIFIED: `.planning/codebase/TESTING.md`, `tests/unit/validation_policy_tests.cpp`] |
| ASan activation | Visual Studio-only preset magic [ASSUMED] | Checked-in CMake/compiler flags for MSVC `/fsanitize=address` [CITED: https://learn.microsoft.com/cpp/build/cmake-presets-vs?view=msvc-170#enable-addresssanitizer-for-windows-and-linux] | `CMakePresets.json` itself does not support `addressSanitizerEnabled`. [CITED: same URL] |
| Shared-library proof | Assuming unit tests against source-compiled helpers are enough [ASSUMED] | Keep `package_consumer_smoke` and `shared_export_surface` in the contract [VERIFIED: `tests/CMakeLists.txt`] | In shared builds the tests compile internal sources differently, so downstream package/export proof remains necessary. [VERIFIED: `tests/CMakeLists.txt:5-46,305-317`] |

**Key insight:** Most of Phase 14 is already present as reusable infrastructure; the missing work is truthful selection, naming, and enforcement of that infrastructure across every declared surface. [VERIFIED: repo files]

## Common Pitfalls

### Pitfall 1: Forgetting that package smoke already runs inside `ctest`
**What goes wrong:** A planner invents a new release-only smoke script instead of reusing the existing CTest test. [VERIFIED: `tests/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`]
**Why it happens:** The current spec correctly says Release proof is missing from the supported matrix, but the reusable mechanics already exist under debug lanes. [VERIFIED: `14-SPEC.md`, `ctest --preset windows-msvc-debug-static -N`]
**How to avoid:** Treat this as a lane-ownership/documentation/preset problem, not as missing smoke infrastructure. [VERIFIED: repo files]
**Warning signs:** New scripts outside `tests/package-consumer/` or CI steps that bypass `ctest`. [VERIFIED: `14-CONTEXT.md`]

### Pitfall 2: Making the ASan lane name real without making ASan instrumentation real
**What goes wrong:** `windows-msvc-asan-static` exists in presets/CI/docs, but the build never actually compiles with `/fsanitize=address`. [VERIFIED: `14-SPEC.md`]
**Why it happens:** `CMakePresets.json` can name a lane, but Microsoft documents that preset files do not provide `addressSanitizerEnabled` behavior. [CITED: https://learn.microsoft.com/cpp/build/cmake-presets-vs?view=msvc-170#enable-addresssanitizer-for-windows-and-linux]
**How to avoid:** Add a checked-in CMake switch/flag path and make policy tests look for the actual MSVC sanitizer signal, not just the preset name. [VERIFIED: `14-SPEC.md`, Microsoft Learn ASan docs]
**Warning signs:** No `/fsanitize=address` token in root CMake or generated compile flags. [CITED: https://learn.microsoft.com/cpp/sanitizers/asan-building?view=msvc-170#compiler]

### Pitfall 3: Treating shared Release as just another `BUILD_SHARED_LIBS=ON` row
**What goes wrong:** The lane passes core tests, but downstream package consumption or runtime DLL handling drifts. [VERIFIED: `tests/package-consumer/CMakeLists.txt`, `verify-runtime-dll-copy.cmake`]
**Why it happens:** Shared builds already require special handling in both test support and consumer runtime-DLL copying. [VERIFIED: `tests/CMakeLists.txt:5-46`, `tests/package-consumer/CMakeLists.txt`, `verify-runtime-dll-copy.cmake`]
**How to avoid:** Keep `shared_export_surface` and package-consumer smoke explicitly in mind when planning shared Release verification. [VERIFIED: `tests/CMakeLists.txt:305-317`] 
**Warning signs:** Release shared planning that mentions only configure/build/ctest without export or DLL-copy proof. [VERIFIED: `14-SPEC.md`]

### Pitfall 4: Trusting the stale `STATE.md` warning more than the live repo state
**What goes wrong:** Planning overreacts to a supposedly dirty Phase 13 working tree that is no longer present. [VERIFIED: `.planning/STATE.md`, `git status --short`]
**Why it happens:** `STATE.md` still says uncommitted Phase 13 residue exists, but current `git status --short` for the phase-relevant surfaces is clean. [VERIFIED: `.planning/STATE.md`, `git status --short .planning CMakePresets.json .github/workflows/ci.yml README.md tests/fixtures/README.md tests/unit/validation_policy_tests.cpp`]
**How to avoid:** Treat the Phase 13 repo-dirty note as stale, but still carry forward the advisory writer-side/non-blocking cleanup findings from `13-VERIFICATION.md`. [VERIFIED: `.planning/STATE.md`, `13-VERIFICATION.md`]
**Warning signs:** Plans that insert unnecessary cleanup waves for non-existent uncommitted changes. [VERIFIED: current git status]

## Code Examples

Verified patterns from official sources and current repo layout:

### MSVC ASan flag ownership in checked-in CMake
```cmake
# Source: https://learn.microsoft.com/cpp/build/cmake-presets-vs?view=msvc-170#enable-addresssanitizer-for-windows-and-linux
if(MSVC AND LIBBSA_ENABLE_MSVC_ASAN)
  target_compile_options(libbsa PUBLIC /fsanitize=address)
endif()
```

### CTest-owned package smoke
```cmake
# Source: tests/CMakeLists.txt and tests/package-consumer/smoke.cmake
add_test(
  NAME package_consumer_smoke
  COMMAND ${CMAKE_COMMAND}
    -DLIBBSA_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DLIBBSA_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/package-consumer-prefix
    -DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/package-consumer
    -DCONSUMER_BUILD_DIR=${CMAKE_BINARY_DIR}/package-consumer-build
    -DCONFIG=$<CONFIG>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/package-consumer/smoke.cmake
)
```

### Test-preset filtering is available if release lanes need explicit smoke ownership
```json
// Source: CMake preset docs filter properties
{
  "name": "windows-msvc-release-static",
  "configurePreset": "windows-msvc-release-static",
  "configuration": "Release",
  "output": { "outputOnFailure": true },
  "filter": {
    "include": { "label": ".*" }
  }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Debug-only supported presets and CI lanes. [VERIFIED: current repo files] | Supported matrix should include Debug static/shared, Release static/shared, and MSVC ASan static. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`] | Targeted by Phase 14. [VERIFIED: `.planning/ROADMAP.md`] | Maintainers can trust that supported docs and automation match real runnable lanes. [VERIFIED: `14-SPEC.md`] |
| Policy tests lock sanitizer absence and only debug preset facts. [VERIFIED: `tests/unit/validation_policy_tests.cpp`] | Policy tests should lock lane roles, smoke ownership, Windows-only scope, and opt-in corpus exclusions through one shared contract helper. [VERIFIED: `14-CONTEXT.md`] | Targeted by Phase 14. [VERIFIED: phase docs] | Future lane drift becomes a test failure instead of tribal knowledge. [VERIFIED: `14-SPEC.md`] |
| Package consumer smoke exists but is not described as part of a supported Release contract. [VERIFIED: `tests/CMakeLists.txt`, `README.md`, `.github/workflows/ci.yml`] | Release lanes should explicitly own install/export plus downstream package smoke inside `ctest`. [VERIFIED: `14-CONTEXT.md`, `14-SPEC.md`] | Targeted by Phase 14. [VERIFIED: phase docs] | Release shared/static become trustworthy package-proof lanes, not just optimized unit-test rows. [VERIFIED: phase docs] |

**Deprecated/outdated:**
- `README.md` claiming only `windows-msvc-debug-static` and `windows-msvc-debug-shared` are supported is outdated for the Phase 14 target contract. [VERIFIED: `README.md`, `14-SPEC.md`]
- `tests/fixtures/README.md` saying cross-platform sanitizer profiles are intentionally not part of the supported contract is outdated once the repo adds the locked MSVC ASan lane. [VERIFIED: `tests/fixtures/README.md`, `14-SPEC.md`]
- `.planning/PROJECT.md` wording that v1.0 already delivered sanitizer-oriented presets is historically misleading against the current repo state and must be corrected as a Phase 14 outcome. [VERIFIED: `.planning/PROJECT.md`, `14-CONTEXT.md`, current repo files]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | A separate standalone policy executable would be worse than extending `validation_policy_tests.cpp`. [ASSUMED] | Standard Stack / Don't Hand-Roll | Low — the locked phase contract still allows truthfulness checks elsewhere if the user later prefers it. |
| A2 | A CI-only ad hoc smoke script would be worse than reusing the current CTest smoke flow. [ASSUMED] | Alternatives / Don't Hand-Roll | Low — D-06 already strongly pushes against it. |
| A3 | Shared-build source-compilation differences make downstream package/export proof especially important, not merely nice-to-have. [ASSUMED] | Don't Hand-Roll / Pitfalls | Medium — if the team later changes shared-test architecture, the importance argument changes, but current repo evidence still points this way. |

## Open Questions (RESOLVED)

1. **`package_consumer_smoke` ownership resolution**
   - Resolution: treat `package_consumer_smoke` and `package_consumer_runtime_dll_copy` as **Release-required ownership**, not **Release-exclusive execution**. Both Release lanes must prove install/export plus downstream package consumption as blocking contract facts per D-05 through D-08, while debug or ASan lanes may still run the same CTest entries if the existing preset/test graph naturally includes them. [VERIFIED: `14-CONTEXT.md`, `tests/CMakeLists.txt`, `tests/package-consumer/smoke.cmake`]
   - Why: the locked decisions require Release ownership but do not require test-preset filtering that removes the smoke proof from other lanes. Preserving the existing reusable CTest flow is the smallest truthful change and avoids inventing a second lane-specific smoke mechanism. [VERIFIED: `14-CONTEXT.md`, `14-SPEC.md`, repo files]
   - Planning effect: Phase 14 plans should update policy/tests/docs so Release lanes are explicitly named as package-proof lanes, but they do not need a separate task just to make package smoke impossible to run elsewhere. [VERIFIED: phase docs + repo layout]

2. **Preset reproducibility scope resolution**
   - Resolution: Phase 14 keeps the current implicit generator / architecture / toolset behavior and does **not** expand into preset pinning work. [VERIFIED: current `CMakePresets.json`, successful `cmake --preset windows-msvc-debug-static` probe, `14-SPEC.md` scope]
   - Why: the locked phase goal is truthful supported-lane coverage, not broader command-line reproducibility hardening. Current presets already configure successfully in the supported environment, and no locked decision requires generator/toolset pinning. [VERIFIED: `14-SPEC.md`, `14-CONTEXT.md`, tool probes]
   - Planning effect: preset work should stay focused on lane addition, MSVC ASan wiring, and contract alignment across CI/docs/tests. Generator/toolset pinning can be deferred unless implementation uncovers a concrete blocker. [VERIFIED: phase docs + repo state]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | All preset configure/build/install flows | ✓ [VERIFIED: `cmake --version`] | 4.3.2 [VERIFIED: tool output] | — |
| CTest | All supported lane test flows | ✓ [VERIFIED: `ctest --version`] | 4.3.2 [VERIFIED: tool output] | — |
| Python | `validate_fixture_manifests` CTest entry | ✓ [VERIFIED: `python --version`] | 3.14.5 [VERIFIED: tool output] | — |
| Git | CI/read-only checks and local repo workflows | ✓ [VERIFIED: `git --version`] | 2.54.0.windows.1 [VERIFIED: tool output] | — |
| vcpkg root env | Preset configure step | ✓ [VERIFIED: `$env:VCPKG_ROOT`] | `C:\vcpkg` path set [VERIFIED: tool output] | None for supported preset flow |
| MSVC Build Tools | CMake/vcpkg Windows compilation | ✓ [VERIFIED: `cmake --preset windows-msvc-debug-static` output detected `cl.exe`] | VS 18 / MSVC 14.51.36231 detected during configure [VERIFIED: configure output] | — |
| GitHub Actions runner | Supported CI execution | Not locally probeable [VERIFIED: research limitation] | `windows-latest` declared [VERIFIED: `.github/workflows/ci.yml`] | Local preset execution for pre-push proof [VERIFIED: repo workflow shape] |

**Missing dependencies with no fallback:**
- None detected in this environment for local planning/probing. [VERIFIED: tool output]

**Missing dependencies with fallback:**
- GitHub Actions execution itself is not locally available, but the repo already supports local preset-driven configure/build/test verification. [VERIFIED: `.github/workflows/ci.yml`, `CMakePresets.json`, successful local configure]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 3.14.0 via `Catch2::Catch2WithMain`. [VERIFIED: `tests/CMakeLists.txt`, `vcpkg x-package-info catch2`] |
| Config file | `tests/CMakeLists.txt`. [VERIFIED: repo file] |
| Quick run command | `ctest --preset windows-msvc-debug-static -R "validation_policy|package_consumer|shared_export_surface" --output-on-failure`. [VERIFIED: `tests/CMakeLists.txt`, current preset names] |
| Full suite command | `ctest --preset <supported-preset> --output-on-failure`. [VERIFIED: `14-SPEC.md`, `README.md`] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| VER-01 | Release static/shared presets configure, build, and run their supported test contracts. [VERIFIED: `14-SPEC.md`] | integration + policy | `cmake --preset windows-msvc-release-static && cmake --build --preset windows-msvc-release-static && ctest --preset windows-msvc-release-static --output-on-failure` and the shared equivalent. [VERIFIED: spec target commands] | ❌ Wave 0 [VERIFIED: current `CMakePresets.json`, `ctest --list-presets`] |
| VER-02 | ASan static preset configures, builds, runs tests, and clearly enables MSVC ASan. [VERIFIED: `14-SPEC.md`] | integration + policy | `cmake --preset windows-msvc-asan-static && cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure`. [VERIFIED: spec target commands] | ❌ Wave 0 [VERIFIED: current `CMakePresets.json`, `ctest --list-presets`] |
| VER-03 | README, fixture policy, planning summaries, presets, CI, and policy tests express the same contract. [VERIFIED: `14-SPEC.md`] | policy | `ctest --preset windows-msvc-debug-static -R "validation_policy" --output-on-failure`. [VERIFIED: `tests/unit/validation_policy_tests.cpp`] | ✅ existing file, ❌ current contract coverage [VERIFIED: repo file content] |

### Sampling Rate
- **Per task commit:** run the focused `validation_policy` selector plus whichever preset was modified. [VERIFIED: `.planning/config.json` TDD mode + repo test layout]
- **Per wave merge:** run all supported lanes' `ctest --preset ... --output-on-failure`. [VERIFIED: `14-SPEC.md`]
- **Phase gate:** main CI matrix plus separate ASan hardening job green with no policy drift. [VERIFIED: `14-CONTEXT.md`]

### Wave 0 Gaps
- [ ] `CMakePresets.json` lacks `windows-msvc-release-static`, `windows-msvc-release-shared`, and `windows-msvc-asan-static`. [VERIFIED: current repo file]
- [ ] `.github/workflows/ci.yml` lacks Release lanes and a separate ASan hardening job. [VERIFIED: current repo file]
- [ ] `tests/unit/validation_policy_tests.cpp` currently locks debug-only lanes and sanitizer absence instead of the Phase 14 contract. [VERIFIED: current repo file]
- [ ] README, fixture policy, and planning summaries still describe the pre-Phase-14 matrix. [VERIFIED: `README.md`, `tests/fixtures/README.md`, `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no [VERIFIED: local library repo, no auth subsystem] | — |
| V3 Session Management | no [VERIFIED: local library repo, no session subsystem] | — |
| V4 Access Control | no [VERIFIED: local library repo, no access-control subsystem] | — |
| V5 Input Validation | yes [VERIFIED: policy tests parse repo-controlled text/config surfaces before trusting them] | Catch2 policy tests that read and validate exact contract tokens across repo files. [VERIFIED: `tests/unit/validation_policy_tests.cpp`] |
| V6 Cryptography | no [VERIFIED: this phase changes build/test policy, not cryptography behavior] | — |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Verification-policy drift between docs, presets, and CI | Tampering / Repudiation [VERIFIED: phase problem statement] | One shared expected contract in policy tests plus same-PR updates across all named surfaces. [VERIFIED: `14-CONTEXT.md`, `14-SPEC.md`] |
| False hardening claims without actual ASan instrumentation | Repudiation / Integrity [VERIFIED: phase problem statement] | Check for actual MSVC ASan wiring, not just an `asan` preset name. [VERIFIED: `14-SPEC.md`, Microsoft Learn ASan docs] |
| Shared-package regressions hidden by internal-source test linkage | Tampering [VERIFIED: shared-build test structure] | Keep package-consumer smoke and shared-export proof in the supported matrix. [VERIFIED: `tests/CMakeLists.txt`, `tests/package-consumer/CMakeLists.txt`] |

## Sources

### Primary (HIGH confidence)
- `J:/libbsa/.planning/phases/14-verification-lane-truthfulness/14-CONTEXT.md` - locked lane roles, CI topology, drift-guard structure, and planning-surface scope.
- `J:/libbsa/.planning/phases/14-verification-lane-truthfulness/14-SPEC.md` - locked requirements, acceptance criteria, and in/out-of-scope boundaries.
- `J:/libbsa/CMakePresets.json` - current preset families are debug-only.
- `J:/libbsa/.github/workflows/ci.yml` - current CI matrix is debug-only with CMake 4.3.2 pin and TES5Edit read-only check.
- `J:/libbsa/tests/CMakeLists.txt` - existing package-consumer smoke, runtime DLL copy test, shared-export proof, and Catch2 test registration.
- `J:/libbsa/tests/package-consumer/CMakeLists.txt` and `smoke.cmake` - downstream package proof flow.
- `J:/libbsa/tests/unit/validation_policy_tests.cpp` - existing debug-only truth gate.
- `https://learn.microsoft.com/cpp/build/cmake-presets-vs?view=msvc-170#enable-addresssanitizer-for-windows-and-linux` - CMakePresets + ASan behavior and command-line reproducibility notes.
- `https://learn.microsoft.com/cpp/sanitizers/asan-building?view=msvc-170#compiler` - MSVC `/fsanitize=address` behavior and compatibility details.
- Context7 `/kitware/cmake` - CMake preset structure and test filter properties.

### Secondary (MEDIUM confidence)
- `J:/libbsa/.planning/codebase/TESTING.md`, `STACK.md`, `ARCHITECTURE.md`, `INTEGRATIONS.md`, `CONCERNS.md` - current repo patterns and concern framing.
- Tool probes: `cmake --version`, `ctest --version`, `python --version`, `git --version`, `cmake --preset windows-msvc-debug-static`, `ctest --preset windows-msvc-debug-static -N`, `vcpkg x-package-info ...`.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - all recommended surfaces already exist in-repo and tool/runtime facts were directly probed. [VERIFIED: repo files + tool output]
- Architecture: MEDIUM - the repo shape is clear, and the package-smoke ownership plus preset reproducibility questions were resolved for planning without widening phase scope. [VERIFIED: repo files + Open Questions]
- Pitfalls: HIGH - every listed pitfall is grounded in current repo code/docs or locked phase decisions. [VERIFIED: repo files + phase docs]

**Research date:** 2026-05-13
**Valid until:** 2026-06-12 for repo-surface facts; re-check sooner if presets/CI/policy files change. [VERIFIED: repo state]
