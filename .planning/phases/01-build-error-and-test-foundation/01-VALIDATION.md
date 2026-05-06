---
phase: 1
slug: build-error-and-test-foundation
status: audited
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-05
updated: 2026-05-05
---

# Phase 1 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.x via vcpkg, orchestrated by CTest |
| **Config file** | `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit` |
| **Full suite command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~10 seconds after configure/build |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit` once the build tree exists.
- **After every plan wave:** Run `cmake --build build/local-vs2026-vcpkg --config Debug` followed by `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`.
- **Before `/gsd-verify-work`:** Full suite must be green.
- **Max feedback latency:** 30 seconds for unit/smoke checks after initial dependency restore.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01-01 | 1 | FND-01 | T-01-01-01 / T-01-01-02 | Avoid dependency/source-list supply-chain drift | smoke | `cmake --list-presets` | yes | green |
| 01-01-02 | 01-01 | 1 | FND-01 | T-01-01-01 | Build scaffold resolves vcpkg dependencies | smoke | `cmake --build build/local-vs2026-vcpkg --config Debug` | yes | green |
| 01-01-03 | 01-01 | 1 | FND-05 | T-01-01-03 | TES5Edit remains read-only and outside build source lists | smoke | PowerShell README/boundary check from plan | yes | green |
| 01-02-01 | 01-02 | 2 | FND-03 | T-01-02-01 / T-01-02-02 | Result/error behavior is specified before implementation | unit-red | Expected-failure PowerShell command in plan | yes | green |
| 01-02-02 | 01-02 | 2 | FND-03 | T-01-02-01 / T-01-02-03 | Public result API returns structured errors without dependency leakage | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit` | yes | green |
| 01-03-01 | 01-03 | 3 | FND-02, FND-04 | T-01-03-01 | Public headers compile without private dependencies or global state | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L smoke` | yes | green |
| 01-03-02 | 01-03 | 3 | VAL-01 | T-01-03-02 | Full foundation suite is green before phase verification | smoke | Full build/test + grep/git gate from plan | yes | green |

*Status: pending, green, red, flaky*

---

## Wave 0 Requirements

- [x] `CMakeLists.txt` - root CMake project with library/test targets and CTest enabled.
- [x] `vcpkg.json` - manifest dependencies for `libdeflate`, `lz4`, `directxtex`, and `catch2`.
- [x] `CMakePresets.json` - primary Windows vcpkg configure preset.
- [x] `include/libbsa/result.hpp` - public result/error API header.
- [x] `src/libbsa.cpp` - minimal source for linkable library target.
- [x] `tests/foundation_tests.cpp` - Catch2 tests for result/error behavior and linkability.
- [x] Public-header smoke translation unit or equivalent compile-only target.

---

## Manual-Only Verifications

None. README wording that documents the `TES5Edit/` read-only boundary is covered by grep-friendly smoke gates and the final boundary check.

---

## Validation Audit 2026-05-05

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

| Requirement | Coverage | Evidence |
|-------------|----------|----------|
| FND-01 | COVERED | `cmake --list-presets`; `cmake --build build/local-vs2026-vcpkg --config Debug`; `vcpkg.json`; `CMakePresets.json`; explicit `CMakeLists.txt` source lists |
| FND-02 | COVERED | `tests/public_header_smoke.cpp`; `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L smoke` |
| FND-03 | COVERED | `tests/foundation_tests.cpp`; `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit` |
| FND-04 | COVERED | `README.md` label policy; full CTest and `unit`/`smoke` label runs |
| FND-05 | COVERED | `git status --short TES5Edit`; no `GLOB` token in `CMakeLists.txt`; boundary text in `README.md` and CMake source-list comments |
| VAL-01 | COVERED | `cmake --build build/local-vs2026-vcpkg --config Debug`; full `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`; final grep/git boundary gate |

---

## Validation Sign-Off

- [x] All tasks have automated verify or Wave 0 dependencies.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 covers all missing references.
- [x] No watch-mode flags.
- [x] Feedback latency < 30s after initial dependency restore.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** audited 2026-05-05
