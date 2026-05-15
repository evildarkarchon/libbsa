---
phase: 14-verification-lane-truthfulness
verified: 2026-05-13T23:06:43.8048646-07:00
status: passed
score: 8/8 must-haves verified
overrides_applied: 0
---

# Phase 14: Verification Lane Truthfulness Verification Report

**Phase Goal:** Maintainers can rely on the documented hardening lanes because the supported Release and ASan flows are checked in, runnable, and consistently described.
**Verified:** 2026-05-13T23:06:43.8048646-07:00
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Maintainer can configure and run a checked-in Windows MSVC Release preset for libbsa builds and automated tests. | ✓ VERIFIED | `CMakePresets.json:36-59,88-95,121-134` defines full Release static/shared configure-build-test triads. `cmake --preset windows-msvc-release-static` and `cmake --preset windows-msvc-release-shared` both succeeded, and `ctest --preset ... -N` listed real test graphs including `package_consumer_smoke` / `package_consumer_runtime_dll_copy` plus `shared_export_surface` for shared. |
| 2 | Release lanes keep install/export plus downstream package-consumer smoke inside `ctest`, and package smoke is blocking Release proof. | ✓ VERIFIED | `tests/CMakeLists.txt:301-338` registers `package_consumer_smoke`, `package_consumer_runtime_dll_copy`, and `shared_export_surface` in CTest, with an explicit comment at `tests/CMakeLists.txt:312-314` assigning Release-lane ownership. `tests/package-consumer/smoke.cmake:28-95` performs install → downstream configure → downstream build → downstream `ctest`, so the smoke path is substantive, not a stub. |
| 3 | Maintainer can configure and run `windows-msvc-asan-static`, and the repo proves it enables real MSVC AddressSanitizer instrumentation. | ✓ VERIFIED | `CMakePresets.json:62-73,98-100,137-139` wires `LIBBSA_ENABLE_MSVC_ASAN=ON` into a full ASan preset triad. `CMakeLists.txt:15-33` defines `LIBBSA_ENABLE_MSVC_ASAN` and applies `/fsanitize=address` through `libbsa_enable_msvc_asan(...)`. Generated build output confirms flow: `build/windows-msvc-asan-static/libbsa.vcxproj:91,136,182,228` and `tests/libbsa_tests.vcxproj:99,177,256,335` contain `/fsanitize=address`. |
| 4 | Policy tests lock lane names, lane roles, package-smoke ownership, workflow topology, docs, and planning summaries instead of only checking preset presence. | ✓ VERIFIED | `tests/unit/validation_policy_tests.cpp:85-109` centralizes the shared verification contract. Independent tests at `366-511` verify README, fixture docs, planning summaries, preset triads, ASan wiring, Release package proof, main workflow matrix, and separate ASan job. Focused execution passed: `ctest --preset windows-msvc-debug-static -R "validation_policy|package_consumer|shared_export_surface" --output-on-failure` → 16/16 tests passed. |
| 5 | CI runs one main Windows matrix for debug and release lanes plus one separate parallel ASan hardening job. | ✓ VERIFIED | `.github/workflows/ci.yml:8-23` defines one `windows-msvc` matrix with debug static/shared and release static/shared rows and `fail-fast: false`. `.github/workflows/ci.yml:76-130` defines a separate `windows-msvc-asan-static` job with no matrix nesting and no `needs:` dependency. |
| 6 | Main-matrix jobs and the ASan job invoke the same checked-in presets maintainers use locally, and workflow names expose both role and preset. | ✓ VERIFIED | `.github/workflows/ci.yml:9,59-65` names matrix rows as `Windows MSVC ${{ matrix.role }} (${{ matrix.preset }})` and drives configure/build/test through `${{ matrix.preset }}`. `.github/workflows/ci.yml:77,115-121` does the same for the named ASan job using `windows-msvc-asan-static`. |
| 7 | README presents `windows-msvc-debug-static` as the quick path and groups the remaining supported lanes by role. | ✓ VERIFIED | `README.md:11-32` gives the concrete quick-path command block for `windows-msvc-debug-static`, then groups supported lanes under Debug inner-loop, Release package-proof, and MSVC AddressSanitizer hardening roles. |
| 8 | `.planning`, `CMakePresets.json`, CI, and policy tests describe the same Windows-only supported verification matrix without drift. | ✓ VERIFIED | `.planning/PROJECT.md:62,98`, `.planning/ROADMAP.md:69-87`, and `.planning/STATE.md:35,64-71` all describe the same role-based Windows matrix. `tests/fixtures/README.md:166-180` preserves Windows-only + opt-in `requires-game-fixture`. The shared truthfulness gate in `tests/unit/validation_policy_tests.cpp:399-511` independently verifies planning, docs, workflow, presets, and ASan topology against one contract. |

**Score:** 8/8 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `CMakePresets.json` | Full configure/build/test triads for Release static/shared and ASan static | ✓ VERIFIED | Present and substantive at `CMakePresets.json:36-145`; configure/build/test preset names match across all supported lanes. |
| `CMakeLists.txt` | Checked-in MSVC ASan option plus real compile-flag wiring | ✓ VERIFIED | `CMakeLists.txt:15-33` adds `LIBBSA_ENABLE_MSVC_ASAN` and applies `/fsanitize=address` in a dedicated helper; helper is wired into `libbsa` at `110`. |
| `tests/CMakeLists.txt` | CTest-owned package-consumer proof and ASan runtime support | ✓ VERIFIED | `tests/CMakeLists.txt:5-63` wires ASan runtime DLL copying for test binaries; `301-338` registers package-consumer proof and shared export proof in CTest. |
| `tests/package-consumer/smoke.cmake` | Substantive install/export + downstream consumer smoke path | ✓ VERIFIED | `tests/package-consumer/smoke.cmake:28-95` installs the package, configures a downstream consumer, builds it, copies runtime DLLs, and runs downstream `ctest`. |
| `tests/package-consumer/CMakeLists.txt` | Consumer executable using installed package plus runtime-DLL helper | ✓ VERIFIED | `tests/package-consumer/CMakeLists.txt:5-24` finds installed `libbsa`, links it, and copies target/ASan runtime DLLs post-build. |
| `.github/workflows/ci.yml` | Main debug/release matrix plus separate ASan hardening job | ✓ VERIFIED | `ci.yml:8-23,76-130` matches the planned topology and invokes preset triads directly. |
| `README.md` | Quick path + role-grouped supported lane docs | ✓ VERIFIED | `README.md:11-32` documents quick path, Release ownership, ASan role, Windows-only scope, and opt-in local corpus checks. |
| `tests/fixtures/README.md` | Fixture policy aligned with supported lane matrix and Windows-only scope | ✓ VERIFIED | `tests/fixtures/README.md:166-180` names the same supported lanes, keeps Windows-only scope, and preserves `requires-game-fixture` opt-in behavior. |
| `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/STATE.md` | Summary-scoped planning alignment | ✓ VERIFIED | `PROJECT.md:62,98`, `ROADMAP.md:69-87`, and `STATE.md:35,64-71` align on the same Phase 14 matrix story without duplicating command-level detail. |
| `tests/unit/validation_policy_tests.cpp` | Shared contract helper plus independent assertions across repo surfaces | ✓ VERIFIED | `tests/unit/validation_policy_tests.cpp:85-109,366-511` is substantive and exercised by the passing focused policy run. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| Release presets | Release CTest proof | `ctest --preset windows-msvc-release-*` | WIRED | Release presets exist in `CMakePresets.json:121-134`; configured `ctest -N` for both Release lanes lists `package_consumer_smoke` and `package_consumer_runtime_dll_copy`, and shared Release also lists `shared_export_surface`. |
| `LIBBSA_ENABLE_MSVC_ASAN` | Real compiler instrumentation | `libbsa_enable_msvc_asan()` → `/fsanitize=address` | WIRED | `CMakePresets.json:72`, `CMakeLists.txt:15-33`, and generated `build/windows-msvc-asan-static/*.vcxproj` all show the flag flow. |
| Test-support targets | Runnable ASan binaries | `libbsa_link_internal_test_support()` → runtime DLL copy helper | WIRED | `tests/CMakeLists.txt:47-62` and `tests/package-consumer/copy-runtime-dlls.cmake:1-28` copy target and extra ASan runtime DLLs to the executable directories. |
| Workflow matrix | Local preset contract | `cmake --preset` / `cmake --build --preset` / `ctest --preset` | WIRED | `.github/workflows/ci.yml:59-65` uses `${{ matrix.preset }}` directly; no alternate CI-only command path exists. |
| Separate ASan job | Supported ASan lane | hard-coded `windows-msvc-asan-static` triad | WIRED | `.github/workflows/ci.yml:115-121` configures, builds, and tests the exact ASan preset. |
| Shared contract helper | Docs / planning / workflow / preset assertions | independent Catch2 tests | WIRED | `tests/unit/validation_policy_tests.cpp:366-511` reads each surface independently against the same contract helper at `85-109`. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `CMakePresets.json` + Release build trees | `configurePreset` / `configuration` | Release static/shared preset triads | Yes — `ctest --preset windows-msvc-release-static -N` listed 16 real tests; `ctest --preset windows-msvc-release-shared -N` listed 17 real tests including `shared_export_surface`. | ✓ FLOWING |
| `CMakePresets.json` + `CMakeLists.txt` + generated ASan project files | `LIBBSA_ENABLE_MSVC_ASAN` | ASan configure preset cache variable | Yes — generated `.vcxproj` files contain `/fsanitize=address` and ASan annotation-disabling definitions, proving the preset drives actual compiler settings. | ✓ FLOWING |
| `.github/workflows/ci.yml` | `matrix.role` / `matrix.preset` | Workflow matrix rows and separate ASan job | Yes — workflow commands consume the exact preset values and display role + preset in job names, so the human-facing matrix and actual execution path are connected. | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Supported preset families are checked in | `cmake --list-presets && ctest --list-presets` | All five supported configure/test presets listed, including Release static/shared and ASan static | ✓ PASS |
| Truthfulness gate and package proof run from the normal test surface | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "validation_policy|package_consumer|shared_export_surface" --output-on-failure` | 16/16 tests passed, including `package_consumer_smoke` and `package_consumer_runtime_dll_copy` | ✓ PASS |
| Release static lane configures and exposes blocking package-proof tests | `cmake --preset windows-msvc-release-static && ctest --preset windows-msvc-release-static -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|shared_export_surface|validation_policy"` | Configure succeeded; 16 tests listed including package-consumer smoke/runtime proof and validation-policy checks | ✓ PASS |
| Release shared lane configures and exposes export/package proof | `cmake --preset windows-msvc-release-shared && ctest --preset windows-msvc-release-shared -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|shared_export_surface|validation_policy"` | Configure succeeded; 17 tests listed including `shared_export_surface` | ✓ PASS |
| ASan lane configures and exposes real test graph | `cmake --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|validation_policy"` | Configure succeeded; 16 tests listed including package-consumer smoke/runtime proof and validation-policy checks | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
| --- | --- | --- | --- |
| None declared or discovered | `find scripts -path '*/tests/probe-*.sh' -type f` equivalent check | No probe scripts found under `scripts/` | SKIP |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| VER-01 | `14-01-PLAN.md`, `14-02-PLAN.md` | Maintainer can configure and run a checked-in Windows MSVC Release preset for libbsa builds and automated tests. | ✓ SATISFIED | Release preset triads are present in `CMakePresets.json`; Release configure commands succeeded; Release `ctest -N` enumerated validation and package-consumer proof; Release ownership is wired in `tests/CMakeLists.txt:312-314`. |
| VER-02 | `14-01-PLAN.md`, `14-02-PLAN.md` | Maintainer can configure and run a checked-in Windows MSVC ASan preset for libbsa builds and automated tests. | ✓ SATISFIED | ASan preset triad exists in `CMakePresets.json`; `LIBBSA_ENABLE_MSVC_ASAN` plus `/fsanitize=address` is wired in `CMakeLists.txt`; generated ASan `.vcxproj` files contain the actual flag. |
| VER-03 | `14-01-PLAN.md`, `14-02-PLAN.md`, `14-03-PLAN.md` | Maintainer can rely on `.planning`, `CMakePresets.json`, CI, and policy tests to describe the same supported verification lanes. | ✓ SATISFIED | README, fixture policy, workflow, presets, and planning summaries all contain the same role-based lane facts, and `validation_policy_tests.cpp:366-511` independently checks each surface against the shared contract. |

Phase 14 requirement IDs from plan frontmatter were fully accounted for: `VER-01`, `VER-02`, `VER-03`.
No orphaned Phase 14 requirement IDs were found in `.planning/REQUIREMENTS.md`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `tests/package-consumer/verify-runtime-dll-copy.cmake` | `30`, `52` | `"runtime placeholder"` / `"asan runtime placeholder"` test writes | ℹ️ Info | Test-only synthetic DLL placeholder files used to verify the copy helper. Not user-visible, not a stub, and not wired into production behavior. |

### Human Verification Required

None.

### Gaps Summary

None.

Phase 14's locked goal is achieved in the codebase: the Release and ASan lanes are present as real preset families, Release package proof remains inside the supported `ctest` contract, CI runs the same preset topology maintainers run locally, and the shared policy suite independently checks docs, planning, presets, and workflow surfaces against one contract instead of trusting summaries.

---

_Verified: 2026-05-13T23:06:43.8048646-07:00_
_Verifier: the agent (gsd-verifier)_
