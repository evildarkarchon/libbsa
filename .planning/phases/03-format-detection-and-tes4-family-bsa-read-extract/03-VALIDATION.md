---
phase: 03
slug: format-detection-and-tes4-family-bsa-read-extract
status: audited
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-08
updated: 2026-05-10
---

# Phase 03 - Validation Strategy

Per-phase validation contract for feedback sampling during execution and retroactive Nyquist coverage audit.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 `3.14.0#0` via vcpkg and CTest discovery |
| **Config file** | `tests/CMakeLists.txt` |
| **Primary test file** | `tests/unit/tes4_bsa_reader_tests.cpp` |
| **Boundary tests** | `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/archive_reader_tests.cpp` |
| **Fixture manifest gate** | `validate_fixture_manifests` CTest test |
| **Quick run command** | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -R "tes4_bsa|unsupported_future_bsa|archive_reader exposes compile-only Phase 3|public_include_boundary|validate_fixture_manifests" --output-on-failure` |
| **Full suite command** | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Latest targeted runtime** | 0.59 sec test time after build on 2026-05-10 |

---

## Sampling Rate

- **After every task commit:** Run the relevant `ctest -R` command for the changed behavior plus `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure`.
- **After every plan wave:** Run `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite must be green, generated fixture provenance documented, malformed fixture set passing, public include boundary passing, and `git -C TES5Edit status --short` empty.
- **Max feedback latency:** 0.59 sec for the focused Phase 3 CTest selector after an already-configured build; full build/test latency remains governed by the repository preset.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 03-04-01 | 03-04 | 2 | FMT-01 | T-03-01 / T-03-02 | Detection rejects non-BSA and malformed bytes without trusting filename extension | fixture + malformed | `ctest --preset windows-msvc-debug-static -R tes4_bsa_detection --output-on-failure` | yes | green |
| 03-04-02 | 03-04 | 2 | FMT-02 | T-03-01 / T-03-02 | Archive metadata is parsed from bounded checked fields | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_metadata --output-on-failure` | yes | green |
| 03-05-01 | 03-05 | 3 | FMT-03 | T-03-03 | Public listing returns deterministic canonical paths and source spelling | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_entry_metadata --output-on-failure` | yes | green |
| 03-05-02 | 03-05 | 3 | FMT-04 | T-03-03 / T-03-05 | Lookup normalizes valid input and rejects rooted/traversal-like paths | unit + fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_lookup --output-on-failure` | yes | green |
| 03-05-03 | 03-05 | 3 | FMT-05 | T-03-01 / T-03-02 | Entry metadata exposes checked sizes, offsets, hashes, flags, and compression | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_entry_metadata --output-on-failure` | yes | green |
| 03-01-01 | 03-01 / 03-04 | 1-2 | FMT-06 | T-03-01 | Public headers remain dependency-light and future/unknown versions classify cleanly | unit + malformed | `ctest --preset windows-msvc-debug-static -R "public_include_boundary|unsupported_future_bsa|archive_reader exposes compile-only Phase 3" --output-on-failure` | yes | green |
| 03-06-01 | 03-06 | 4 | BSA-01 | T-03-04 | v103 raw and deflate entries extract exact expected bytes | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v103_extract --output-on-failure` | yes | green |
| 03-06-02 | 03-06 | 4 | BSA-02 | T-03-04 | v104 raw and deflate entries extract exact expected bytes | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v104_extract --output-on-failure` | yes | green |
| 03-06-03 | 03-06 | 4 | BSA-03 | T-03-04 | v105 raw and LZ4-frame entries extract exact expected bytes | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v105_extract --output-on-failure` | yes | green |
| 03-05-04 | 03-05 | 3 | BSA-05 | T-03-03 | Hash-compatible lookup routes normalized paths to the expected records | unit + fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_lookup --output-on-failure` | yes | green |
| 03-05-05 | 03-05 / 03-06 | 3-4 | BSA-06 | T-03-03 / T-03-04 | Embedded-name prefixes are skipped for extracted payload bytes and exposed in metadata | fixture | `ctest --preset windows-msvc-debug-static -R "tes4_bsa_entry_metadata|tes4_bsa_v104_extract|tes4_bsa_v105_extract|tes4_bsa_extract_bytes" --output-on-failure` | yes | green |
| 03-06-04 | 03-06 | 4 | BSA-07 | T-03-04 / T-03-02 | Raw, deflate, and LZ4-frame routing fail closed on corrupt or size-mismatched payloads | fixture + malformed | `ctest --preset windows-msvc-debug-static -R tes4_bsa_compression_routing --output-on-failure` | yes | green |
| 03-06-05 | 03-06 / verification | 4 | BIN-03 | T-03-02 / T-03-04 | Extraction reads only the selected stored payload and reports host-file loss after open | fixture + regression | `ctest --preset windows-msvc-debug-static -R "tes4_bsa_extract reads selected payloads from the host archive on demand" --output-on-failure` | yes | green |

---

## Wave 0 Requirements

- [x] `tests/unit/tes4_bsa_reader_tests.cpp` covers FMT-01 through FMT-05 and BSA-01 through BSA-07.
- [x] `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` creates legal v103/v104/v105 success and malformed archives.
- [x] `tests/fixtures/generated/archives/*.json` manifests expected metadata, paths, extracted bytes/hashes, and provenance.
- [x] Test-only `nlohmann-json` manifest dependency and CMake wiring are scoped to the test target only.
- [x] `tests/unit/archive_reader_tests.cpp` unsupported-stub assertions were replaced with Phase 3 reader contract and missing-file behavior.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| BSArchPro-derived compatibility comparison | COMP-01 supplement / BSA-06 behavior confidence | Optional local/game-derived data cannot be committed or required in default CI | If local compatibility fixtures exist, run the optional `compat`/`requires-game-fixture` tests and compare extracted bytes or metadata against BSArchPro-derived expected outputs. |

---

## Threat References

| Threat Ref | Threat | Required Mitigation |
|------------|--------|---------------------|
| T-03-01 | Malformed/truncated table causes out-of-bounds read | Use checked binary reads and validate every count, offset, and span against archive length. |
| T-03-02 | Oversized count or size causes memory exhaustion | Use checked arithmetic and reject sizes inconsistent with file length before allocation. |
| T-03-03 | Duplicate or ambiguous archive paths corrupt lookup/extraction semantics | Normalize paths, reject traversal/rooted inputs, and fail duplicate canonical archive paths. |
| T-03-04 | Corrupt compressed payload causes partial or incorrect output | Use exact-size decompression adapters and fail `format_error` before reporting extraction success. |
| T-03-05 | Partial sink writes leave ambiguous extraction result | Treat partial sink acceptance as `io_error` and do not silently retry in Phase 3. |

---

## Validation Audit 2026-05-10

| Metric | Count |
|--------|-------|
| Requirements audited | 13 |
| Gaps found | 0 |
| Tests created | 0 |
| Resolved | 0 |
| Escalated | 0 |
| Manual-only | 1 optional supplement |

### Audit Evidence

| Command | Result |
|---------|--------|
| `cmake --build --preset windows-msvc-debug-static` | passed; MSBuild emitted existing MSB8028 shared-intermediate warnings for generated fixture tool projects |
| `ctest --preset windows-msvc-debug-static -R "tes4_bsa|unsupported_future_bsa|archive_reader exposes compile-only Phase 3|public_include_boundary|validate_fixture_manifests" --output-on-failure` | passed, 27/27 tests |
| `ctest --preset windows-msvc-debug-static --output-on-failure` | passed, 245/245 tests with the two opt-in local fixture checks skipped as designed |
| `03-VERIFICATION.md` review | passed, 6/6 must-have truths verified and no remaining gaps |
| `03-SECURITY.md` review | secured, 26/26 declared threats closed |
| `03-REVIEW.md` review | clean, 0 findings |

---

## Validation Sign-Off

- [x] All tasks have automated verification or an explicitly optional manual supplement.
- [x] Sampling continuity: no three consecutive tasks lacked automated verification.
- [x] Wave 0 coverage landed and all MISSING references are now backed by tests.
- [x] No watch-mode flags are part of validation commands.
- [x] Feedback latency recorded for the focused Phase 3 selector bundle.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** Nyquist-compliant after 2026-05-10 audit.
