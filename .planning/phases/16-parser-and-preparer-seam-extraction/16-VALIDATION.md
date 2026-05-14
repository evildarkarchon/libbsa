---
phase: 16
slug: parser-and-preparer-seam-extraction
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-14
---

# Phase 16 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3 via `Catch2::Catch2WithMain`; CTest discovery via `catch_discover_tests`. |
| **Config file** | `tests/CMakeLists.txt`; root build source registration in `CMakeLists.txt`. |
| **Quick run command** | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Hardening lane command** | `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure` |
| **Estimated runtime** | Quick label run expected under 60 seconds; full debug and ASan lanes depend on local MSVC/vcpkg cache state. |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|ba2_dx10_writer|writer-stage|parser_preparer_seam"`
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`
- **Before `/gsd-verify-work`:** Full debug suite plus `cmake --build --preset windows-msvc-asan-static && ctest --preset windows-msvc-asan-static --output-on-failure`
- **Max feedback latency:** 60 seconds for focused label feedback after normal task commits.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 16-01-01 | 01 | 1 | REFA-01 | T-16-01 | TES4 table seam preserves table sizing, folder/file-name offsets, name/hash validation, and duplicate canonical path behavior. | unit + fixture/malformed | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|parser_preparer_seam"` | Wave 0: add `tests/unit/tes4_bsa_parser_seam_tests.cpp`; existing `tests/unit/tes4_bsa_reader_tests.cpp` exists. | pending |
| 16-01-02 | 01 | 1 | REFA-01 | T-16-02 | TES4 payload descriptor seam preserves embedded-name prefix sizing, raw-size calculation, compression interpretation, and payload-span-over-metadata rejection. | unit + malformed | `ctest --preset windows-msvc-debug-static --output-on-failure -L "tes4_bsa|parser_preparer_seam"` | Wave 0: direct seam tests needed; existing public malformed tests exist. | pending |
| 16-02-01 | 02 | 1 | REFA-02 | T-16-03 | BA2 DX10 snapshot builder preserves source snapshot immutability and target DDS format validation. | unit + writer integration | `ctest --preset windows-msvc-debug-static --output-on-failure -L "ba2_dx10_writer|parser_preparer_seam"` | Wave 0: add `tests/unit/ba2_dx10_preparer_seam_tests.cpp`; existing `tests/unit/ba2_dx10_writer_tests.cpp` exists. | pending |
| 16-02-02 | 02 | 1 | REFA-02 | T-16-04 | BA2 DX10 chunk seam preserves streamed snapshot-backed assembly, multi-mip/array/cubemap order, and Fallout 4 vs Starfield v3 compression routing. | unit + writer-stage | `ctest --preset windows-msvc-debug-static --output-on-failure -L "writer-stage|parser_preparer_seam"` | Existing `tests/unit/writer_stage_tests.cpp` covers single/multi/cubemap/truncated; add direct routing/plan-then-assemble seam tests. | pending |
| 16-03-01 | 03 | 1 | REFA-01, REFA-02 | T-16-05 | Structural guardrail fails if responsibilities collapse back into monolithic flows while allowing implementation-chosen helper names. | source-policy unit | `ctest --preset windows-msvc-debug-static --output-on-failure -L "parser_preparer_seam_policy"` | Wave 0: add `tests/unit/parser_preparer_seam_policy_tests.cpp`. | pending |

---

## Wave 0 Requirements

- [ ] `tests/unit/tes4_bsa_parser_seam_tests.cpp` - covers REFA-01 table and payload descriptor seams.
- [ ] `tests/unit/ba2_dx10_preparer_seam_tests.cpp` - covers REFA-02 snapshot builder and chunk plan/assembly/compression seams.
- [ ] `tests/unit/parser_preparer_seam_policy_tests.cpp` - guards role separation without freezing exact helper names.
- [ ] `CMakeLists.txt` - add any new private source `.cpp` files to `libbsa_library_sources`.
- [ ] `tests/CMakeLists.txt` - add new Catch2 test files to `libbsa_tests` and ensure labels are discoverable through CTest.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None | REFA-01, REFA-02 | Phase 16 behavior can be verified through focused source assertions, Catch2 tests, full debug CTest, and MSVC ASan. | All phase behaviors should have automated verification before sign-off. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s for focused label feedback
- [ ] `nyquist_compliant: true` set in frontmatter after Wave 0 and sampling criteria are satisfied

**Approval:** pending
