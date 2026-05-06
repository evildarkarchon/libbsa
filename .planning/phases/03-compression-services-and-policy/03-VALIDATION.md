---
phase: 03
slug: compression-services-and-policy
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-05-05
---

# Phase 03 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 through CTest |
| **Config file** | `CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit` |
| **Full suite command** | `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L unit`
- **After every plan wave:** Run `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 03-01-01 | 01 | 1 | CMP-04, CMP-05 | T-03-01-01 | Explicit routing rejects unsupported codec/policy combinations | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_compression_policy_tests` | ❌ W0 | ⬜ pending |
| 03-02-01 | 02 | 2 | CMP-01, CMP-04 | T-03-02-01 | Deflate failures and exact-size mismatches return structured decompression failures | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_deflate_codec_tests` | ❌ W0 | ⬜ pending |
| 03-03-01 | 03 | 3 | CMP-02, CMP-03, CMP-04 | T-03-03-01 | LZ4 frame/raw block decoders cannot be interchanged silently | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_lz4_codec_tests` | ❌ W0 | ⬜ pending |
| 03-04-01 | 04 | 4 | CMP-01, CMP-02, CMP-03, CMP-04, CMP-05 | T-03-04-01 | Public headers remain dependency-free and TES5Edit remains unmodified | smoke/boundary | `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/compression_policy_tests.cpp` — stubs for CMP-04 and CMP-05
- [ ] `tests/deflate_codec_tests.cpp` — stubs for CMP-01 and CMP-04
- [ ] `tests/lz4_codec_tests.cpp` — stubs for CMP-02, CMP-03, and CMP-04

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 60s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-05-05
