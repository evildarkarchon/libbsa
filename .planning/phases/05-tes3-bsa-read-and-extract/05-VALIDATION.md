---
phase: 05
slug: tes3-bsa-read-and-extract
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-06
---

# Phase 05 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 via CTest |
| **Config file** | `CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` |
| **Full suite command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~60 seconds |

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`
- **After every plan wave:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 60 seconds

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 05-01-01 | 01 | 1 | BSA-04 | T-05-01 | Unknown/truncated TES3 table bytes fail structured | fixture/unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | ✅ | ✅ COVERED |
| 05-01-02 | 01 | 1 | BSA-04 | T-05-01 | TES3 metadata offsets are bounded and absolute | fixture/unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | ✅ | ✅ COVERED |
| 05-02-01 | 02 | 2 | BSA-04 | T-05-02 | Extract reads only validated raw TES3 payload ranges | fixture/unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | ✅ | ✅ COVERED |
| 05-02-02 | 02 | 2 | BSA-04 | T-05-02 | Lookup path normalization resolves TES3 entries | fixture/unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | ✅ | ✅ COVERED |
| 05-03-01 | 03 | 3 | BSA-04 | T-05-03 | Name/hash table corruption is rejected | unit | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | ✅ | ✅ COVERED |
| 05-04-01 | 04 | 4 | BSA-04 | T-05-04 | Public headers keep dependency/TES5Edit boundary clean | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | ✅ | ✅ COVERED |

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements.

## Manual-Only Verifications

All phase behaviors have automated verification.

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 60s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** audited 2026-05-05

## Validation Audit 2026-05-05

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

All Phase 05 requirements map to existing automated Catch2/CTest coverage. No new test files were required during this audit.
