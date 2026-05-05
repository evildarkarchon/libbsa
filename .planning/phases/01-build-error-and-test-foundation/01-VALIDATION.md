---
phase: 1
slug: build-error-and-test-foundation
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-05
---

# Phase 1 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.x via vcpkg, orchestrated by CTest |
| **Config file** | `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json` |
| **Quick run command** | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit` |
| **Full suite command** | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure` |
| **Estimated runtime** | ~10 seconds after configure/build |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit` once the build tree exists.
- **After every plan wave:** Run `cmake --build build/windows-msvc-vcpkg --config Debug` followed by `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite must be green.
- **Max feedback latency:** 30 seconds for unit/smoke checks after initial dependency restore.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01-01 | 1 | FND-01 | T-01-01-01 / T-01-01-02 | Avoid dependency/source-list supply-chain drift | smoke | `cmake --list-presets` | W0 | pending |
| 01-01-02 | 01-01 | 1 | FND-01 | T-01-01-01 | Build scaffold resolves vcpkg dependencies | smoke | `cmake --preset windows-msvc-vcpkg && cmake --build build/windows-msvc-vcpkg --config Debug` | W0 | pending |
| 01-01-03 | 01-01 | 1 | FND-05 | T-01-01-03 | TES5Edit remains read-only and outside build source lists | smoke | PowerShell README/boundary check from plan | W0 | pending |
| 01-02-01 | 01-02 | 2 | FND-03 | T-01-02-01 / T-01-02-02 | Result/error behavior is specified before implementation | unit-red | Expected-failure PowerShell command in plan | W0 | pending |
| 01-02-02 | 01-02 | 2 | FND-03 | T-01-02-01 / T-01-02-03 | Public result API returns structured errors without dependency leakage | unit | `cmake --build build/windows-msvc-vcpkg --config Debug; ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit` | W0 | pending |
| 01-03-01 | 01-03 | 3 | FND-02, FND-04 | T-01-03-01 | Public headers compile without private dependencies or global state | smoke | `cmake --build build/windows-msvc-vcpkg --config Debug; ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L smoke` | W0 | pending |
| 01-03-02 | 01-03 | 3 | VAL-01 | T-01-03-02 | Full foundation suite is green before phase verification | smoke | Full build/test + grep/git gate from plan | W0 | pending |

*Status: pending, green, red, flaky*

---

## Wave 0 Requirements

- [ ] `CMakeLists.txt` - root CMake project with library/test targets and CTest enabled.
- [ ] `vcpkg.json` - manifest dependencies for `libdeflate`, `lz4`, `directxtex`, and `catch2`.
- [ ] `CMakePresets.json` - primary Windows vcpkg configure preset.
- [ ] `include/libbsa/result.hpp` - public result/error API header.
- [ ] `src/libbsa.cpp` - minimal source for linkable library target.
- [ ] `tests/foundation_tests.cpp` - Catch2 tests for result/error behavior and linkability.
- [ ] Public-header smoke translation unit or equivalent compile-only target.

---

## Manual-Only Verifications

All phase behaviors have automated verification except visual inspection of README wording; this should be checked by reading `README.md` and confirming it states the `TES5Edit/` read-only boundary.

---

## Validation Sign-Off

- [ ] All tasks have automated verify or Wave 0 dependencies.
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify.
- [ ] Wave 0 covers all missing references.
- [ ] No watch-mode flags.
- [ ] Feedback latency < 30s after initial dependency restore.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** pending
