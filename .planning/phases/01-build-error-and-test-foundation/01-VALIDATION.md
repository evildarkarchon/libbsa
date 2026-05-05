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
| 01-01-01 | 01 | 1 | FND-01, FND-02 | — | N/A | smoke | `cmake -S . --preset windows-msvc-vcpkg && cmake --build build/windows-msvc-vcpkg --config Debug` | W0 | pending |
| 01-01-02 | 01 | 1 | FND-03, FND-04 | — | N/A | unit | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L unit` | W0 | pending |
| 01-01-03 | 01 | 1 | FND-05, VAL-01 | — | N/A | smoke | `ctest --test-dir build/windows-msvc-vcpkg --output-on-failure -L smoke` | W0 | pending |

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
