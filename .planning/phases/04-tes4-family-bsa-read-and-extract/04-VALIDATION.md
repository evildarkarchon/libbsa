---
phase: 04
slug: tes4-family-bsa-read-and-extract
phase_slug: tes4-family-bsa-read-and-extract
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-06
updated: 2026-05-05
---

# Phase 04 - Validation Strategy: TES4-Family BSA Read and Extract

Per-phase validation contract for feedback sampling during execution and post-execution Nyquist coverage audit.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.x through CTest |
| **Config file** | `CMakeLists.txt` |
| **Primary target** | `libbsa_bsa_reader_tests` |
| **Fixture labels** | `fixture` through `catch_discover_tests(... ADD_TAGS_AS_LABELS ...)` |
| **Smoke target** | `libbsa.public_header_smoke` |
| **Quick run command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` |
| **Full suite command** | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` |
| **Estimated runtime** | ~4 seconds after build |

---

## Required Validation Dimensions

1. v103/v104/v105 table parsing exposes correct normalized paths and metadata.
2. Per-entry compression state uses archive-default XOR file-size flag behavior.
3. Extraction writes raw, deflate, and LZ4-frame payloads to caller-owned `byte_sink`.
4. Embedded filename prefixes in v104/v105 are skipped before payload handling.
5. Malformed/truncated offsets, sizes, and names return structured failures.
6. Public headers remain dependency-clean and `TES5Edit/` remains untouched.

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`.
- **After every plan wave:** Run `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`.
- **Before `/gsd-verify-work`:** Full suite plus public-header and boundary gates must be green.
- **Max feedback latency:** ~4 seconds for tests after build.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 04-01-01 | 01 | 1 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-01, T-04-02 | Public BSA API exposes lookup/extract contracts without dependency leakage. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `tests/bsa_reader_tests.cpp` | GREEN |
| 04-01-02 | 01 | 1 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-01, T-04-02 | Private constants and source wiring exist outside `TES5Edit/`. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `src/bsa_reader.cpp`, `src/bsa_reader.hpp` | GREEN |
| 04-02-01 | 02 | 2 | BSA-01, BSA-02, BSA-03 | T-04-03, T-04-04 | v103/v104/v105 metadata parsing and XOR compression state are fixture tested. | fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `tests/bsa_reader_tests.cpp` | GREEN |
| 04-02-02 | 02 | 2 | BSA-01, BSA-02, BSA-03 | T-04-03, T-04-04 | Bounded table parser returns structured errors for unsupported or malformed input. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `src/bsa_reader.cpp` | GREEN |
| 04-03-01 | 03 | 3 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-05, T-04-06, T-04-07 | Raw, deflate, LZ4-frame, and embedded-name extraction behaviors are fixture tested. | fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `tests/bsa_reader_tests.cpp` | GREEN |
| 04-03-02 | 03 | 3 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-05, T-04-06, T-04-07 | Extraction validates payload ranges, embedded prefixes, and exact decompression output. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `src/bsa_reader.cpp` | GREEN |
| 04-04-01 | 04 | 4 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-08, T-04-09, T-04-10 | Malformed-input and compatibility regression tests lock stable failure behavior. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `tests/bsa_reader_tests.cpp` | GREEN |
| 04-04-02 | 04 | 4 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-08, T-04-09, T-04-10 | Existing parser/extractor hardening satisfies malformed-input tests without redundant changes. | unit, fixture | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` | yes: `src/bsa_reader.cpp` | GREEN |
| 04-05-01 | 05 | 5 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-11 | Consumer-style smoke coverage includes `libbsa/bsa.hpp` and public BSA lookup. | smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | yes: `tests/public_header_smoke.cpp` | GREEN |
| 04-05-02 | 05 | 5 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-11, T-04-12 | README documents validation commands and public-header/TES5Edit boundaries. | docs, command gate | `rg -n "TES4-family BSA read and extract|ARCHIVE_COMPRESS|FILE_SIZE_COMPRESS|libbsa_bsa_reader_tests" README.md` | yes: `README.md` | GREEN |
| 04-05-03 | 05 | 5 | BSA-01, BSA-02, BSA-03, BSA-05 | T-04-11, T-04-12 | Final full-suite, public-header, CMake, and submodule boundary gates pass. | full suite, command gate | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` | yes: `CMakeLists.txt`, `include/libbsa`, `TES5Edit` | GREEN |

Status: GREEN = command passed during validation audit.

---

## Requirement Coverage Matrix

| Requirement | Validation Status | Evidence |
|-------------|-------------------|----------|
| BSA-01 | COVERED | `open_bsa` v103/v104/v105 metadata tests; public smoke BSA lookup. |
| BSA-02 | COVERED | XOR metadata and extraction inversion tests in `tests/bsa_reader_tests.cpp`. |
| BSA-03 | COVERED | Raw, deflate, LZ4-frame, and embedded-name extraction fixture tests. |
| BSA-05 | COVERED | Unsupported version, truncated table, impossible payload range, truncated embedded-name, public-header token, CMake GLOB, and `TES5Edit` status gates. |

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No Wave 0 test scaffolding is required.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Required Verification Commands

- `cmake --build build/local-vs2026-vcpkg --config Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L fixture`
- `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke`
- `rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex|TES5Edit" include/libbsa`
- `rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt`
- `git status --short TES5Edit`

---

## Validation Audit 2026-05-05

| Metric | Count |
|--------|-------|
| Input state | State A - existing `04-VALIDATION.md` audited |
| Plans audited | 5 |
| Summary files audited | 5 |
| Requirements audited | 4 |
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
| Generated test files | 0 |

No `gsd-nyquist-auditor` subagent was spawned because every requirement mapped to existing green automated coverage.

---

## Validation Sign-Off

- [x] All tasks have automated verification commands.
- [x] Sampling continuity: no 3 consecutive tasks without automated verification.
- [x] Wave 0 covers all missing references; no missing references remain.
- [x] No watch-mode flags.
- [x] Feedback latency is below the 4-second post-build target in this audit run.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-05-05
