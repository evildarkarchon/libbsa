---
phase: 01
slug: foundation-api-boundary-and-test-harness
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-07
---

# Phase 01 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 v3 via vcpkg + CTest |
| **Config file** | `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` and `ctest --preset windows-msvc-debug-shared --output-on-failure` |
| **Estimated runtime** | ~60 seconds after dependencies are installed |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` once the test preset exists.
- **After every plan wave:** Run static and shared preset configure/build/test commands from the active plan verification.
- **Before `/gsd-verify-work`:** Static and shared full suites must be green.
- **Max feedback latency:** 60 seconds for the quick unit label path after initial vcpkg restore.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01 | 1 | FND-01/FND-02 | T-01-01 | Build does not compile `TES5Edit/` | build | `cmake --preset windows-msvc-debug-static && cmake --build --preset windows-msvc-debug-static` | ✅ | ⬜ pending |
| 01-01-02 | 01 | 1 | FND-01/FND-02 | T-01-01 | Package export uses public headers only | build | `cmake --preset windows-msvc-debug-shared && cmake --build --preset windows-msvc-debug-shared` | ✅ | ⬜ pending |
| 01-02-01 | 02 | 2 | FND-03/FND-04/FND-05 | T-01-02 | Public API returns typed errors without dependency leakage | unit | `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` | ✅ | ⬜ pending |
| 01-03-01 | 03 | 3 | FND-06 | T-01-03 | Local game fixture tests are skipped by default | unit | `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` | ✅ | ⬜ pending |
| 01-04-01 | 04 | 1 | FND-07 | T-01-04 | Local fixtures are ignored and `TES5Edit/` is prohibited | docs/grep | `git check-ignore tests/fixtures/local/.keep` | ✅ | ⬜ pending |
| 01-05-01 | 05 | 4 | DOC-04 | T-01-05 | CI builds static/shared without touching `TES5Edit/` | ci/grep | `grep -n "windows-msvc-debug-static\|windows-msvc-debug-shared" .github/workflows/ci.yml` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure does not yet cover phase requirements. Plans create the test and CMake infrastructure before relying on it for full verification. Early tasks use configure/build checks immediately after each relevant file set exists.

---

## Manual-Only Verifications

All Phase 1 behaviors have automated verification through CMake, CTest, grep, or CI workflow file inspection.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or explicit dependency on prior plans that create the command.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 requirements are represented by Plan 01/03 foundation setup.
- [x] No watch-mode flags.
- [x] Feedback latency target < 60s after initial dependency restore.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-05-07
