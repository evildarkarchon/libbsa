---
phase: 06
slug: dds-boundary-and-ba2-dx10-read-reconstruction
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-08
---

# Phase 06 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 `3.14.0#0` via vcpkg + CTest 4.3.2 |
| **Config file** | `tests/CMakeLists.txt`, `CMakePresets.json` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -R "ba2_dx10|public_include_boundary" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | TBD after Wave 0 test target creation |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static -R "ba2_dx10|public_include_boundary" --output-on-failure`
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`
- **Before `/gsd-verify-work`:** Full static and shared preset suites must be green, and `git -C TES5Edit status --short` must produce no output.
- **Max feedback latency:** TBD after Wave 0 establishes DX10 tests.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 06-W0-01 | TBD | 0 | DDS-01 | T-06-01 | Bounded BA2 DX10 open rejects invalid headers safely | fixture unit | `ctest --preset windows-msvc-debug-static -R ba2_dx10_detector --output-on-failure` | no W0 | pending |
| 06-W0-02 | TBD | 0 | DDS-02 | T-06-01 | Starfield v3 DX10 metadata routes by parsed version/method fields | fixture unit | `ctest --preset windows-msvc-debug-static -R ba2_dx10_detector --output-on-failure` | no W0 | pending |
| 06-W0-03 | TBD | 0 | DDS-03 | T-06-05 | Public texture metadata stays dependency-light and exposes required fields | fixture unit | `ctest --preset windows-msvc-debug-static -R ba2_dx10_metadata --output-on-failure` | no W0 | pending |
| 06-W0-04 | TBD | 0 | DDS-04 | T-06-02 | Extraction writes complete reconstructed DDS bytes without unsafe reads | fixture integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_extract --output-on-failure` | no W0 | pending |
| 06-W0-05 | TBD | 0 | DDS-05 | T-06-03 | Compressed chunks enforce exact decoded sizes and fail closed | fixture integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_compression --output-on-failure` | no W0 | pending |
| 06-W0-06 | TBD | 0 | DDS-06 | T-06-05 | DirectXTex validation remains private and public headers stay clean | unit/integration | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_directxtex|public_include_boundary" --output-on-failure` | partial | pending |
| 06-W0-07 | TBD | 0 | DDS-07 | T-06-04 | Cubemap/array mip and face layout is validated before extraction | fixture integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_layout --output-on-failure` | no W0 | pending |

*Status: pending, green, red, flaky.*

---

## Wave 0 Requirements

- [ ] `include/libbsa/archive.hpp` - add `texture_metadata` and DX10-only optional field for DDS-03.
- [ ] `src/formats/ba2/ba2_dx10_parser.*` - add BA2 DX10 parser for DDS-01, DDS-02, and DDS-03.
- [ ] `src/formats/ba2/ba2_dx10_reader.*` - add DDS reconstruction/extraction path for DDS-04, DDS-05, and DDS-07.
- [ ] `src/texture/dds_layout.*` and `src/texture/directxtex_analyzer.*` - add private texture boundary for DDS-04 and DDS-06.
- [ ] `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` - generate legal DX10 success and malformed fixtures.
- [ ] `tests/unit/ba2_dx10_reader_tests.cpp` - add manifest-backed DX10 open, metadata, extraction, DirectXTex validation, and malformed tests.
- [ ] `CMakeLists.txt` and `tests/CMakeLists.txt` - wire DirectXTex privately, add DX10 sources/tests, and register fixture generator target.

---

## Manual-Only Verifications

All Phase 6 required behaviors have automated verification. Optional local real-game archive checks are out of scope for Phase 6 acceptance and must not be required by default.

---

## Validation Sign-Off

- [ ] All tasks have automated verify commands or Wave 0 dependencies.
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify.
- [ ] Wave 0 covers all missing references.
- [ ] No watch-mode flags.
- [ ] Feedback latency measured after Wave 0.
- [ ] `nyquist_compliant: true` set in frontmatter after Wave 0 and sampling commands are proven.

**Approval:** pending
