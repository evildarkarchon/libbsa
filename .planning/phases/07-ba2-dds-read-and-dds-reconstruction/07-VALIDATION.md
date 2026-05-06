---
phase: 07
slug: ba2-dds-read-and-dds-reconstruction
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-06
audited: 2026-05-06
input_state: reconstructed-from-summaries
---

# Phase 07 — Validation Strategy

> Reconstructed Nyquist validation contract for Phase 7 after execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.x through CTest |
| **Config file** | `CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa.public_header_smoke|libbsa_ba2_dds_reader_tests|libbsa_dds_reconstruction_tests|libbsa_ba2_reader_tests"` |
| **Full suite command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~2 seconds for focused Phase 7 tests; full suite previously verified at 114/114 |

---

## Sampling Rate

- **After every task commit:** Run the focused Phase 7 CTest command.
- **After every plan wave:** Run the full CTest suite.
- **Before `/gsd-verify-work`:** Full suite must be green, public headers must remain dependency-clean, and `TES5Edit/` must remain untouched.
- **Max feedback latency:** ~2 seconds for focused Phase 7 tests.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | BA2-05, BA2-06 | — | Public BA2 texture metadata API compiles from consumer-style code without private dependency leakage. | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | yes: `tests/public_header_smoke.cpp` | green |
| 07-01-02 | 01 | 1 | BA2-05, BA2-06 | — | `ba2_archive::texture_metadata(path)` returns copied metadata by value and fails structurally for missing texture metadata. | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | yes: `tests/public_header_smoke.cpp` | green |
| 07-02-01 | 02 | 1 | BA2-05, BA2-06 | — | Generated BA2 DDS fixtures cover FO4/Starfield, mip, cubemap/array, raw, deflate, LZ4-block, and malformed source-reviewable cases. | fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_dds_reader_tests` | yes: `tests/ba2_dds_fixture_helpers.hpp`, `tests/ba2_dds_fixture_helpers.cpp` | green |
| 07-02-02 | 02 | 1 | BA2-05, BA2-06 | — | Focused BA2 DDS test target is wired with `unit;fixture;codec` labels. | fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_dds_reader_tests` | yes: `tests/ba2_dds_reader_tests.cpp` | green |
| 07-03-01 | 03 | 2 | BA2-05, BA2-06 | — | FO4 DX10 v1/v7/v8 and Starfield DX10 v3 archives open, list, lookup, and expose texture metadata. | unit, fixture, codec | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_dds_reader_tests|libbsa_ba2_reader_tests"` | yes: `tests/ba2_dds_reader_tests.cpp` | green |
| 07-03-02 | 03 | 2 | BA2-05, BA2-06 | — | DX10 parser validates records, names, chunks, duplicates, and preserves GNRL behavior. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_dds_reader_tests|libbsa_ba2_reader_tests"` | yes: `tests/ba2_dds_reader_tests.cpp`, `tests/ba2_reader_tests.cpp` | green |
| 07-04-01 | 04 | 3 | BA2-06 | — | Private DDS reconstruction tests prove one-mip, multi-mip, cubemap, array, and unsupported precondition behavior. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_dds_reconstruction_tests` | yes: `tests/dds_reconstruction_tests.cpp` | green |
| 07-04-02 | 04 | 3 | BA2-06 | — | DirectXTex validation remains private and reconstructed DDS bytes are semantically loadable. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_dds_reconstruction_tests` | yes: `tests/dds_reconstruction_tests.cpp` | green |
| 07-05-01 | 05 | 4 | BA2-05, BA2-06 | — | BA2 DX10 extraction covers FO4 deflate, Starfield LZ4-block, raw chunks, one-mip, multi-mip, cubemap, array, and no-partial-write validation failure. | unit, fixture, codec | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_dds_reader_tests` | yes: `tests/ba2_dds_reader_tests.cpp` | green |
| 07-05-02 | 05 | 4 | BA2-05, BA2-06 | — | Extraction buffers all chunks, reconstructs and validates DDS output, then writes to the sink once. | unit, fixture, codec | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_ba2_dds_reader_tests|libbsa_ba2_reader_tests|libbsa_dds_reconstruction_tests"` | yes: `tests/ba2_dds_reader_tests.cpp`, `tests/dds_reconstruction_tests.cpp` | green |
| 07-06-01 | 06 | 5 | BA2-05, BA2-06 | — | Malformed DX10 records, bad chunk ranges, name-table problems, duplicate names, codec routes, mip mapping, unsupported layouts, and reconstruction failures are covered. | unit, fixture, codec | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_dds_reader_tests` | yes: `tests/ba2_dds_reader_tests.cpp` | green |
| 07-06-02 | 06 | 5 | BA2-05, BA2-06 | — | Malformed extraction failures return structured errors and leave caller sinks empty without codec fallback guessing. | unit, fixture, codec | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_dds_reader_tests` | yes: `tests/ba2_dds_reader_tests.cpp` | green |
| 07-06-03 | 06 | 5 | BA2-05, BA2-06 | — | Documentation states generated fixture scope and validation commands without overclaiming real corpus or BSArchPro byte comparison coverage. | docs, regression | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` | yes: `README.md` | green |

---

## Requirement Coverage

| Requirement | Status | Automated Evidence | Notes |
|-------------|--------|--------------------|-------|
| BA2-05 | COVERED | `tests/public_header_smoke.cpp`, `tests/ba2_dds_reader_tests.cpp`, `tests/ba2_reader_tests.cpp` | Covers FO4 and Starfield BA2 DDS open/list/inspect/extract behavior plus GNRL regression safety. |
| BA2-06 | COVERED | `tests/ba2_dds_reader_tests.cpp`, `tests/dds_reconstruction_tests.cpp`, `tests/ba2_dds_fixture_helpers.*` | Covers complete DDS reconstruction, DirectXTex-backed semantic validation, texture metadata fields, malformed safety, and no-partial-write behavior. |

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Audit 2026-05-06

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

### Audit Commands

| Command | Result |
|---------|--------|
| `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa.public_header_smoke|libbsa_ba2_dds_reader_tests|libbsa_dds_reconstruction_tests|libbsa_ba2_reader_tests"` | 55/55 passed |
| `rg -n "DirectXTex|DXGI_FORMAT|Windows\\.h|libdeflate|TES5Edit" include/libbsa` | no matches |
| `rg -n "lz4\\.h|lz4frame\\.h|LZ4_" include/libbsa` | no matches |
| `git status --short TES5Edit` | no output |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or existing test infrastructure dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 10s for focused Phase 7 tests
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-05-06
