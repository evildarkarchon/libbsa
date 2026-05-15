---
phase: 16
slug: parser-and-preparer-seam-extraction
status: passed
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-14
last_audited: 2026-05-14
---

# Phase 16 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3 via `Catch2::Catch2WithMain`; CTest discovery via `catch_discover_tests`. |
| **Config file** | `tests/CMakeLists.txt`; root build source registration in `CMakeLists.txt`. |
| **Task smoke command** | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` for TES4/BA2 seam tasks, or `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` for policy tasks. |
| **Affected-suite wave command** | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Hardening lane command** | `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure` |
| **Estimated runtime** | Task smoke commands are expected under 30 seconds after the incremental `libbsa_tests` build has completed; affected-suite, full debug, and ASan lanes are wave/phase gates and may exceed 30 seconds depending on local MSVC/vcpkg cache state. |

---

## Sampling Rate

- **After every task commit:** Run the narrow task smoke command for the touched seam: `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` for Plans 16-01/16-02 runtime seam tasks, or `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` for Plan 16-03 policy guard work.
- **After every plan wave:** Run the affected-suite wave command `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"`; after Plan 16-03 also run the full debug suite.
- **Before `/gsd-verify-work`:** Full debug suite plus `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure`
- **Max feedback latency:** under 30 seconds for focused task smoke feedback after the relevant incremental build; affected-suite, full debug, and ASan commands are intentionally retained as wave/phase gates rather than Nyquist task loops.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 16-01-01 | 01 | 1 | REFA-01 | T-16-01 | TES4 table seam preserves table sizing, folder/file-name offsets, name/hash validation, and duplicate canonical path behavior. | unit + fixture/malformed | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` | Covered by `tests/unit/tes4_bsa_parser_seam_tests.cpp`; existing `tests/unit/tes4_bsa_reader_tests.cpp` remains covered by the affected-suite wave gate. | COVERED |
| 16-01-02 | 01 | 1 | REFA-01 | T-16-02 | TES4 payload descriptor seam preserves embedded-name prefix sizing, raw-size calculation, compression interpretation, and payload-span-over-metadata rejection. | unit + malformed | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` | Covered by `tests/unit/tes4_bsa_parser_seam_tests.cpp`; existing public malformed tests remain covered by the affected-suite wave gate. | COVERED |
| 16-02-01 | 02 | 1 | REFA-02 | T-16-03 | BA2 DX10 snapshot builder preserves source snapshot immutability and target DDS format validation. | unit + writer integration | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` | Covered by `tests/unit/ba2_dx10_preparer_seam_tests.cpp`; existing `tests/unit/ba2_dx10_writer_tests.cpp` remains covered by the affected-suite wave gate. | COVERED |
| 16-02-02 | 02 | 1 | REFA-02 | T-16-04 | BA2 DX10 chunk seam preserves streamed snapshot-backed assembly, multi-mip/array/cubemap order, and Fallout 4 vs Starfield v3 compression routing. | unit + writer-stage | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam"` | Covered by `tests/unit/ba2_dx10_preparer_seam_tests.cpp`; `tests/unit/writer_stage_tests.cpp` remains covered by the affected-suite wave gate. | COVERED |
| 16-03-01 | 03 | 1 | REFA-01, REFA-02 | T-16-05 | Structural guardrail fails if responsibilities collapse back into monolithic flows while allowing implementation-chosen helper names. | source-policy unit | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` | Covered by `tests/unit/parser_preparer_seam_policy_tests.cpp`. | COVERED |

---

## Wave 0 Requirements

- [x] `tests/unit/tes4_bsa_parser_seam_tests.cpp` - covers REFA-01 table and payload descriptor seams.
- [x] `tests/unit/ba2_dx10_preparer_seam_tests.cpp` - covers REFA-02 snapshot builder and chunk plan/assembly/compression seams.
- [x] `tests/unit/parser_preparer_seam_policy_tests.cpp` - guards role separation without freezing exact helper names.
- [x] `CMakeLists.txt` - add any new private source `.cpp` files to `libbsa_library_sources`.
- [x] `tests/CMakeLists.txt` - add new Catch2 test files to `libbsa_tests` and ensure labels are discoverable through CTest.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None | REFA-01, REFA-02 | Phase 16 behavior can be verified through focused source assertions, Catch2 tests, full debug CTest, and MSVC ASan. | All phase behaviors should have automated verification before sign-off. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 30s for focused task smoke feedback after incremental build
- [x] `nyquist_compliant: true` remains set while task smoke commands stay narrow and affected/full/ASan commands remain wave or phase gates

**Approval:** approved by 2026-05-14 Nyquist audit

---

## Validation Audit 2026-05-14

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

| Requirement | Coverage | Evidence |
|-------------|----------|----------|
| REFA-01 | COVERED | `tests/unit/tes4_bsa_parser_seam_tests.cpp` covers TES4 raw table and payload descriptor seams; `tests/unit/parser_preparer_seam_policy_tests.cpp` guards TES4 role collapse. |
| REFA-02 | COVERED | `tests/unit/ba2_dx10_preparer_seam_tests.cpp` covers BA2 DX10 snapshot and chunk assembler seams; `tests/unit/parser_preparer_seam_policy_tests.cpp` guards BA2 role collapse. |

Focused audit command: `cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam|parser_preparer_seam_policy"` passed with 8/8 tests.
