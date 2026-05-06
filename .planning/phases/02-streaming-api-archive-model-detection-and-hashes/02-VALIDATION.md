---
phase: 02
slug: streaming-api-archive-model-detection-and-hashes
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-05
---

# Phase 02 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 via CTest |
| **Config file** | `CMakeLists.txt`, `vcpkg.json`, `CMakePresets.json` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit` |
| **Full suite command** | `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~60 seconds after configure |

---

## Sampling Rate

- **After every task commit:** Run `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit`
- **After every plan wave:** Run `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds after configure

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 01 | 1 | BIO-01/BIO-02/BIO-03/BIO-05 | T-02-01 | Bounds-checked source reads and sink writes reject invalid ranges | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_io_tests` | ✅ | ✅ green |
| 02-02-01 | 02 | 2 | BIO-04/BIO-05/DPH-01 | T-02-02 | Malformed/unsupported headers produce structured failures | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_detection_tests` | ✅ | ✅ green |
| 02-03-01 | 03 | 3 | DPH-02/DPH-04/DPH-05 | T-02-03 | Invalid archive paths fail without host path interpretation | golden | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_path_hash_tests` | ✅ | ✅ green |
| 02-04-01 | 04 | 4 | BIO-04/DPH-02/DPH-03/DPH-04 | T-02-04 | Metadata lookup does not read payload bytes or retain borrowed sources | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_archive_view_tests` | ✅ | ✅ green |
| 02-05-01 | 05 | 5 | BIO-05 | T-02-05 | Boundary gates prevent public dependency/TES5Edit leakage | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L smoke` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `tests/io_tests.cpp` — unit coverage for BIO-01/BIO-02/BIO-03.
- [x] `tests/detection_tests.cpp` — unit coverage for BIO-04/DPH-01 detection behavior.
- [x] `tests/path_hash_tests.cpp` — golden-vector coverage for DPH-02/DPH-04/DPH-05.
- [x] `tests/archive_view_tests.cpp` — unit coverage for DPH-02/DPH-03/DPH-04 metadata-only lookup.
- [x] `CMakeLists.txt` — explicit test executable wiring and labels.

---

## Validation Audit 2026-05-05

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

Validated Phase 02 coverage against all PLAN/SUMMARY artifacts and current CTest targets. Existing Wave 0 rows were stale; all referenced test files now exist and passed their automated commands.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 60 seconds after configure
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-05-05
