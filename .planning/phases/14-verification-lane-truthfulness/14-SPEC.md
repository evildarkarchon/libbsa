# Phase 14: Verification Lane Truthfulness - Specification

**Created:** 2026-05-13
**Ambiguity score:** 0.11 (gate: <= 0.20)
**Requirements:** 5 locked

## Goal

Maintainers can run a truthful supported Windows verification matrix that includes Release static, Release shared, and MSVC AddressSanitizer static lanes, plus release-lane package smoke, with all policy and documentation surfaces describing the same contract.

## Background

The current repository only checks in `windows-msvc-debug-static` and `windows-msvc-debug-shared` preset families in `CMakePresets.json`, and `.github/workflows/ci.yml` only configures, builds, and tests those two lanes. `README.md` documents only those debug presets as supported, while `tests/fixtures/README.md` still says sanitizer profiles are intentionally not part of the supported contract. At the same time, `.planning/PROJECT.md` and the v1.1 milestone narrative already talk about stronger hardening coverage, which creates policy drift. `tests/unit/validation_policy_tests.cpp` currently locks the debug-only preset story and the absence of sanitizer lanes, so maintainers cannot trust the documented verification story for Release or ASan coverage today.

## Requirements

1. **Release static lane**: A checked-in `windows-msvc-release-static` preset family becomes part of the supported verification matrix for configure, build, and automated test runs.
   - Current: `CMakePresets.json` only defines `windows-msvc-debug-static` and `windows-msvc-debug-shared`, and CI only runs those two debug lanes.
   - Target: `CMakePresets.json`, CI, and supported documentation all include a runnable `windows-msvc-release-static` configure/build/test preset family as an official lane.
   - Acceptance: Maintainer-facing verification passes only if `cmake --preset windows-msvc-release-static`, `cmake --build --preset windows-msvc-release-static`, and `ctest --preset windows-msvc-release-static --output-on-failure` are checked in, documented as supported, and executed by the supported CI matrix.

2. **Release shared lane**: A checked-in `windows-msvc-release-shared` preset family becomes part of the supported verification matrix for configure, build, and automated test runs.
   - Current: No `windows-msvc-release-shared` preset family or supported-lane claim exists in the checked-in preset, CI, or documentation surfaces.
   - Target: `CMakePresets.json`, CI, and supported documentation include a runnable `windows-msvc-release-shared` configure/build/test preset family as an official lane.
   - Acceptance: Maintainer-facing verification passes only if `cmake --preset windows-msvc-release-shared`, `cmake --build --preset windows-msvc-release-shared`, and `ctest --preset windows-msvc-release-shared --output-on-failure` are checked in, documented as supported, and executed by the supported CI matrix.

3. **ASan static lane**: A checked-in `windows-msvc-asan-static` preset family using MSVC AddressSanitizer becomes part of the supported verification matrix for configure, build, and automated test runs.
   - Current: No ASan preset family exists, CI does not run an ASan lane, `README.md` does not advertise one, and `tests/fixtures/README.md` plus `tests/unit/validation_policy_tests.cpp` still encode sanitizer absence as the supported policy.
   - Target: `CMakePresets.json`, CI, and supported documentation include a runnable `windows-msvc-asan-static` configure/build/test preset family that uses MSVC AddressSanitizer as an official hardening lane.
   - Acceptance: Maintainer-facing verification passes only if `cmake --preset windows-msvc-asan-static`, `cmake --build --preset windows-msvc-asan-static`, and `ctest --preset windows-msvc-asan-static --output-on-failure` are checked in, documented as supported, executed by the supported CI matrix, and the checked-in lane configuration clearly enables MSVC AddressSanitizer rather than a non-Windows sanitizer flow.

4. **Release package smoke**: The supported Release verification story proves install/export and downstream package consumption instead of stopping at raw configure/build/test.
   - Current: The checked-in Windows matrix stops at configure, build, and `ctest`; it does not prove release-lane install/export artifacts or a package-consumer smoke flow.
   - Target: The supported Release verification contract includes install/export generation for the release matrix and a checked-in package-consumer smoke proof for downstream consumption.
   - Acceptance: Maintainer-facing verification passes only if the supported Release lanes generate install/export artifacts and at least one checked-in package-consumer smoke flow succeeds against the produced package/install output as part of the supported verification contract.

5. **Supported-matrix truthfulness**: All declared verification surfaces describe the same supported lanes, package-smoke expectations, and exclusions.
   - Current: `.planning`, `README.md`, `tests/fixtures/README.md`, `CMakePresets.json`, `.github/workflows/ci.yml`, and `tests/unit/validation_policy_tests.cpp` do not currently agree: debug lanes are documented as the only supported presets, sanitizer support is explicitly rejected in some places, and broader hardening claims already exist elsewhere.
   - Target: `.planning`, `README.md`, `tests/fixtures/README.md`, `CMakePresets.json`, `.github/workflows/ci.yml`, and `tests/unit/validation_policy_tests.cpp` all describe the same supported matrix: retained debug lanes, supported `windows-msvc-release-static`, supported `windows-msvc-release-shared`, supported `windows-msvc-asan-static`, release-lane package smoke, Windows-only scope, opt-in `requires-game-fixture` checks, and no extra sanitizer or Linux/WSL lanes.
   - Acceptance: Drift-proof validation passes only if the listed files name the same supported lanes and exclusions, policy tests fail when any one surface diverges, and the supported lane contract continues to state that local corpus checks remain opt-in rather than mandatory.

## Boundaries

**In scope:**
- Checked-in configure/build/test preset families for `windows-msvc-release-static`, `windows-msvc-release-shared`, and `windows-msvc-asan-static`
- Supported CI execution for the retained debug lanes plus the new Release static, Release shared, and ASan static lanes
- Release-lane install/export verification and at least one checked-in package-consumer smoke flow
- Truthfulness alignment across `.planning`, `README.md`, `tests/fixtures/README.md`, `CMakePresets.json`, `.github/workflows/ci.yml`, and `tests/unit/validation_policy_tests.cpp`
- Explicit preservation of Windows-only scope and opt-in `requires-game-fixture` behavior in the supported verification contract

**Out of scope:**
- Linux, WSL, macOS, POSIX, or other cross-platform verification lanes - excluded because libbsa remains Windows-only
- UBSan, TSan, fuzzing, or sanitizer families beyond MSVC AddressSanitizer - excluded to keep Phase 14 limited to the locked supported matrix
- Mandatory BSArchPro-derived or game-corpus checks in the default supported lanes - excluded because supported lanes must remain runnable from committed assets only
- New runtime dependencies, archive-family support, or public API expansion - excluded because Phase 14 is verification hardening, not product-surface growth
- Broader performance, parser, reader-dispatch, dedupe, or DX10 staging work - excluded because those belong to later phases in the v1.1 roadmap

## Constraints

- Windows-only: the supported verification matrix must stay on Windows MSVC and must not reopen cross-platform support.
- The ASan lane must use MSVC AddressSanitizer, not a Linux/Clang sanitizer workflow.
- Supported lanes must be runnable from checked-in presets and repository-controlled automation rather than local unpublished scripts.
- Supported lanes must continue to pass without local copyrighted archives; `requires-game-fixture` checks remain opt-in.
- No new runtime dependencies or public API changes are allowed for this phase.
- Policy tests and maintainer-facing docs must enforce the same supported-matrix contract so lane drift becomes a test failure instead of tribal knowledge.

## Acceptance Criteria

- [ ] `windows-msvc-release-static` configure, build, and test presets are checked in, documented as supported, and executed in CI.
- [ ] `windows-msvc-release-shared` configure, build, and test presets are checked in, documented as supported, and executed in CI.
- [ ] `windows-msvc-asan-static` configure, build, and test presets are checked in, documented as supported, executed in CI, and clearly use MSVC AddressSanitizer.
- [ ] The supported Release verification contract proves install/export generation and a checked-in package-consumer smoke flow against produced package/install output.
- [ ] `.planning`, `README.md`, `tests/fixtures/README.md`, `CMakePresets.json`, `.github/workflows/ci.yml`, and `tests/unit/validation_policy_tests.cpp` all describe the same supported lanes and exclusions.
- [ ] The supported lane contract keeps `requires-game-fixture` checks opt-in and does not add Linux/WSL or extra sanitizer lanes to Phase 14.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.94  | 0.75  | ✓      | Supported matrix and package-smoke outcome are explicit |
| Boundary Clarity    | 0.90  | 0.70  | ✓      | Release shared expansion and explicit exclusions are locked |
| Constraint Clarity  | 0.84  | 0.65  | ✓      | Windows-only, MSVC ASan, opt-in corpus checks, no extra sanitizer families |
| Acceptance Criteria | 0.86  | 0.70  | ✓      | Pass/fail checks cover presets, CI, package smoke, and drift enforcement |
| **Ambiguity**       | 0.11  | <=0.20| ✓      | Gate passed after three rounds |

Status: ✓ = met minimum, ⚠ = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What is the minimum Phase 14 outcome, which lanes become officially supported, and which surfaces must stay truthful? | Phase 14 must make CI run the new lanes, add `windows-msvc-release-static` and `windows-msvc-asan-static`, and treat `README.md`, fixture docs, policy tests, `.planning`, presets, and CI as one truth contract |
| 2 | Researcher + Simplifier | How narrow can the phase stay while still solving the problem? | The phase must also include package smoke, add `windows-msvc-release-shared` as an official lane, and keep local corpus checks opt-in |
| 3 | Boundary Keeper | What exactly do shared Release and package smoke mean, and what still stays out of scope? | `windows-msvc-release-shared` is a full supported lane, package smoke means install/export plus consumer smoke, and Linux/WSL, extra sanitizers, fuzzing, and mandatory local corpus checks stay out of scope |

---

*Phase: 14-verification-lane-truthfulness*
*Spec created: 2026-05-13*
*Next step: /gsd-discuss-phase 14 - implementation decisions (how to build what's specified above)*
