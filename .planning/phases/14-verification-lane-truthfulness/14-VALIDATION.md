---
phase: 14
slug: verification-lane-truthfulness
status: ready
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-13
---

# Phase 14 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.14.0 plus CTest preset orchestration |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -R "validation_policy|package_consumer|shared_export_surface" --output-on-failure` |
| **Full suite command** | `ctest --preset <supported-preset> --output-on-failure` |
| **Estimated runtime** | ~600 seconds |

---

## Sampling Rate

- **After every task commit:** Run the focused `validation_policy` selector plus the narrowest affected preset or proof surface.
- **After every plan wave:** Run the full `ctest --preset <supported-preset> --output-on-failure` flow for each lane changed in that wave.
- **Before `/gsd-verify-work`:** `windows-msvc-debug-static`, `windows-msvc-debug-shared`, `windows-msvc-release-static`, `windows-msvc-release-shared`, and `windows-msvc-asan-static` must all be green where defined by the final contract.
- **Max feedback latency:** 120 seconds for focused policy runs.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 14-V-01 | 14-01 task 1 | 1 | VER-03 | T-14-01 | Shared verification-matrix policy keeps preset, CMake, and CTest assertions independent so supported-matrix drift fails locally. | policy | `ctest --preset windows-msvc-debug-static -R "validation_policy" --output-on-failure` | ✅ existing | ✅ green |
| 14-V-02 | 14-01 task 2 | 1 | VER-01 | T-14-03 | Release static/shared lanes keep install/export and downstream package smoke inside the checked-in CTest contract. | integration + policy | `cmake --preset windows-msvc-release-static && ctest --preset windows-msvc-release-static -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|shared_export_surface|validation_policy"` plus the shared equivalent | ✅ existing | ✅ green |
| 14-V-03 | 14-01 task 2 | 1 | VER-02 | T-14-02 | The ASan lane must configure from a checked-in preset and still produce real `/fsanitize=address` instrumentation. | integration + policy | `cmake --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|validation_policy"` and `rg -n "/fsanitize=address" build/windows-msvc-asan-static/*.vcxproj` | ✅ existing | ✅ green |
| 14-V-04 | 14-02 task 1 / 14-03 task 3 | 2 / 3 | VER-03 | T-14-04 / T-14-05 / T-14-06 / T-14-07 | README, fixture policy, workflow, presets, and planning summaries describe the same Windows-only supported matrix without cross-surface drift. | policy | `ctest --preset windows-msvc-debug-static -R "validation_policy" --output-on-failure` | ✅ existing | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Coverage Anchors

- `14-V-01` (`VER-03`) is anchored by `tests/unit/validation_policy_tests.cpp` and its focused `validation_policy` selector over the shared verification-matrix contract.
- `14-V-02` (`VER-01`) is anchored by `tests/unit/validation_policy_tests.cpp`, `tests/CMakeLists.txt`, and `tests/package-consumer/smoke.cmake`, with Release static/shared preset discovery confirming blocking `package_consumer_smoke` coverage stays in `ctest`.
- `14-V-03` (`VER-02`) is anchored by `tests/unit/validation_policy_tests.cpp`, `CMakeLists.txt`, and the generated `build/windows-msvc-asan-static/*.vcxproj` files that still carry `/fsanitize=address`.
- `14-V-04` (`VER-03`) is anchored by `README.md`, `tests/fixtures/README.md`, `.github/workflows/ci.yml`, `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`, and the independent repo-surface assertions in `tests/unit/validation_policy_tests.cpp`.

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements. Catch2, CTest, package-consumer smoke, and export-surface checks are already present; Phase 14 needs truthful lane wiring, not a new test framework.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| GitHub Actions runs the main Windows matrix and the separate MSVC AddressSanitizer hardening job with the same supported contract claimed locally. | VER-01, VER-02, VER-03 | GitHub-hosted workflow execution is not locally runnable from this workspace. | Push the branch or open a PR, then confirm `.github/workflows/ci.yml` produces one Debug/Release Windows matrix plus one separate ASan hardening job and that all supported jobs pass. |

---

## Validation Sign-Off

- [x] All planned verification slices have automated commands or an explicit manual-only reason.
- [x] Sampling continuity: focused policy checks can run after each task without waiting for full multi-lane execution.
- [x] Wave 0 covers all missing validation infrastructure references.
- [x] No watch-mode flags.
- [x] Feedback latency < 120s for the focused policy selector.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-05-13

---

## Validation Audit 2026-05-13

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

Focused audit evidence:

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` succeeded.
- `ctest --preset windows-msvc-debug-static -R "validation_policy|package_consumer|shared_export_surface" --output-on-failure` passed 16/16.
- `cmake --preset windows-msvc-release-static && ctest --preset windows-msvc-release-static -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|shared_export_surface|validation_policy"` discovered 16 blocking Release-lane tests.
- `cmake --preset windows-msvc-release-shared && ctest --preset windows-msvc-release-shared -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|shared_export_surface|validation_policy"` discovered 17 blocking Release-lane tests, including `shared_export_surface`.
- `cmake --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static -N -R "package_consumer_smoke|package_consumer_runtime_dll_copy|validation_policy"` discovered 16 ASan-lane tests.
- `build/windows-msvc-asan-static/libbsa.vcxproj` and `build/windows-msvc-asan-static/tests/libbsa_tests.vcxproj` still contain `/fsanitize=address`.
- `.planning/phases/14-verification-lane-truthfulness/14-VERIFICATION.md` still aligns with the current audit and records the phase-close Release/ASan lane evidence.
