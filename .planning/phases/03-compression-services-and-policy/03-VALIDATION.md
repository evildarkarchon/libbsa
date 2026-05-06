---
phase: 03
slug: compression-services-and-policy
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-05
updated: 2026-05-06
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
| 03-01-01 | 01 | 1 | CMP-04, CMP-05 | T-03-01-01 | Explicit routing rejects unsupported codec/policy combinations | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_compression_policy_tests` | ✅ | ✅ green |
| 03-02-01 | 02 | 2 | CMP-01, CMP-04 | T-03-02-01 | Deflate failures and exact-size mismatches return structured decompression failures | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_deflate_codec_tests` | ✅ | ✅ green |
| 03-03-01 | 03 | 3 | CMP-02, CMP-03, CMP-04 | T-03-03-01 | LZ4 frame/raw block decoders cannot be interchanged silently | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_lz4_codec_tests` | ✅ | ✅ green |
| 03-04-01 | 04 | 4 | CMP-01, CMP-02, CMP-03, CMP-04, CMP-05 | T-03-04-01 | Public headers remain dependency-free and TES5Edit remains unmodified | smoke/boundary | `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `tests/compression_policy_tests.cpp` — covers CMP-04 and CMP-05
- [x] `tests/deflate_codec_tests.cpp` — covers CMP-01 and CMP-04
- [x] `tests/lz4_codec_tests.cpp` — covers CMP-02, CMP-03, and CMP-04

---

## Validation Audit 2026-05-06

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

| Requirement | Coverage | Test File | Status |
|-------------|----------|-----------|--------|
| CMP-01 | Deflate round-trip, malformed input, exact-size mismatch, empty payload, and full-suite boundary coverage | `tests/deflate_codec_tests.cpp`, `tests/public_header_smoke.cpp` | COVERED |
| CMP-02 | LZ4 frame round-trip, frame magic, cross-route rejection, and public boundary coverage | `tests/lz4_codec_tests.cpp`, `tests/public_header_smoke.cpp` | COVERED |
| CMP-03 | Raw LZ4 block round-trip, exact-size mismatch, cross-route rejection, and Starfield `CompressionMethod == 3` routing coverage | `tests/lz4_codec_tests.cpp`, `tests/compression_policy_tests.cpp`, `tests/public_header_smoke.cpp` | COVERED |
| CMP-04 | Explicit codec routing, unsupported route failures, exact-size decompression failures, and codec-label suite coverage | `tests/compression_policy_tests.cpp`, `tests/deflate_codec_tests.cpp`, `tests/lz4_codec_tests.cpp` | COVERED |
| CMP-05 | Writer policy resolution and dependency-free public compression API smoke coverage | `tests/compression_policy_tests.cpp`, `tests/public_header_smoke.cpp` | COVERED |

**Audit command:** `cmake --build build/local-vs2026-vcpkg --config Debug && ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`

**Result:** Passed 38/38 tests, including 11 codec-labeled tests and `libbsa.public_header_smoke`.

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
