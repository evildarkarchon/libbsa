---
phase: 05
slug: ba2-gnrl-read-extract
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-08
updated: 2026-05-08
---

# Phase 05 - Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + CTest |
| **Config file** | `tests/CMakeLists.txt` |
| **Primary test file** | `tests/unit/ba2_gnrl_reader_tests.cpp` |
| **Fixture generator** | `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_bounded_open|ba2_gnrl" --output-on-failure` |
| **Regression command** | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_bounded_open|ba2_gnrl|tes3_bsa|tes4_bsa|public_include_boundary" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~10 seconds focused, ~17 seconds full suite |

## Sampling Rate

- **After every task commit:** Run focused labels from the task verify block.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static -L "ba2_gnrl|public_include_boundary" --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite and TES5Edit clean status must be green.
- **Max feedback latency:** 60 seconds for focused labels.

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | Test Evidence | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|---------------|--------|
| 05-01-01 | 01 | 1 | GNRL-01,GNRL-02,GNRL-03,GNRL-04,GNRL-05,GNRL-06,GNRL-07,GNRL-08 | T-05-01,T-05-02 | Generated legal fixtures only; explicit compression metadata | generator | `cmake --build --preset windows-msvc-debug-static --target generate_ba2_gnrl_fixtures` | `generate_ba2_gnrl_fixtures` target and generated BA2 GNRL manifests exist | COVERED |
| 05-02-01 | 02 | 2 | GNRL-01,GNRL-02,GNRL-03,GNRL-08 | T-05-03,T-05-04 | Byte-driven BTDX routing; unsupported DX10/v3 method handling; public dependency boundary | unit | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_detector --output-on-failure` | `[ba2_gnrl_detector]`, `[public-api]` | COVERED |
| 05-03-01 | 03 | 3 | GNRL-04,GNRL-05,GNRL-08 | T-05-05,T-05-06 | Checked record/name/payload spans; duplicate canonical path rejection; normalized lookup | unit | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_metadata|ba2_gnrl_lookup" --output-on-failure` | `[ba2_gnrl_metadata]`, `[ba2_gnrl_lookup]` | COVERED |
| 05-04-01 | 04 | 4 | GNRL-06,GNRL-07 | T-05-07,T-05-08 | Metadata-driven raw/deflate/raw-LZ4 extraction; exact-size codec routing; partial sink errors | unit | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_extract --output-on-failure` | `[ba2_gnrl_extract]` | COVERED |
| 05-05-01 | 05 | 5 | GNRL-01,GNRL-02,GNRL-03,GNRL-04,GNRL-05,GNRL-06,GNRL-07,GNRL-08 | T-05-09,T-05-10,T-05-11 | Fail-closed malformed BA2 cases; unsupported-vs-malformed error classification; TES5Edit read-only gate | unit/regression | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl_malformed|ba2_gnrl_extract|ba2_gnrl_metadata|ba2_gnrl_lookup" --output-on-failure` | `[ba2_gnrl_malformed]`, full Phase 5 regression summaries | COVERED |
| 05-06-01 | 06 | 6 | GNRL-01,GNRL-02,GNRL-03,GNRL-04 | T-05-06-01,T-05-06-02,T-05-06-03 | Open/list reads only metadata and bounded filename-table bytes; payload bytes remain extraction-only | unit/regression | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_bounded_open --output-on-failure` | `[ba2_gnrl_bounded_open]` sparse 8 GiB payload-offset regression | COVERED |

## Requirement Coverage Matrix

| Requirement | Coverage | Evidence |
|-------------|----------|----------|
| GNRL-01 | COVERED | FO4/SFv2/SFv3 BA2 GNRL detector/open tests and bounded-open regression |
| GNRL-02 | COVERED | Byte-driven BA2 metadata detection and Starfield version-gated fields |
| GNRL-03 | COVERED | Unsupported DX10 and v3 compression method tests assert stable `unsupported` errors |
| GNRL-04 | COVERED | Manifest-backed entry metadata and bounded filename-table parsing tests |
| GNRL-05 | COVERED | Normalized find/contains lookup tests and duplicate canonical malformed cases |
| GNRL-06 | COVERED | Raw, deflate, zero-byte, and raw LZ4-block extraction byte comparisons |
| GNRL-07 | COVERED | Exact-size decompression and corrupt/mismatched compressed payload failure tests |
| GNRL-08 | COVERED | Generated-only fixture corpus, malformed manifest, and TES5Edit clean-status gate |

## Wave 0 Requirements

- [x] `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` - generated BA2 fixture source.
- [x] `tests/unit/ba2_gnrl_reader_tests.cpp` - BA2 detector, metadata, lookup, extraction, malformed, and bounded-open tests.
- [x] `src/formats/ba2/` - private BA2 parser/reader implementation directory.

## Manual-Only Verifications

All Phase 5 behaviors have automated verification. No manual-only gaps remain.

## Validation Audit 2026-05-08

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
| Requirements covered | 8 |
| Automated test labels verified | 6 |

### Commands Run

| Command | Result |
|---------|--------|
| `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "ba2_gnrl_bounded_open|ba2_gnrl|tes3_bsa|tes4_bsa|public_include_boundary" --output-on-failure` | Passed, 40/40 tests |
| `ctest --preset windows-msvc-debug-static --output-on-failure` | Passed, 92/93 tests plus 1 expected local-fixture skip |
| `git -C TES5Edit status --short` | Passed, no output |

## Validation Sign-Off

- [x] All tasks have automated verify commands.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 covers all initially missing BA2 test/fixture files.
- [x] Plan 06 bounded-open gap closure is represented in the validation map.
- [x] No watch-mode flags.
- [x] Feedback latency target < 60s for focused labels.

**Approval:** Nyquist-compliant after audit on 2026-05-08.
