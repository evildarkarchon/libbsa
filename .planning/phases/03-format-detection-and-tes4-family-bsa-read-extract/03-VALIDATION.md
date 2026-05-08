---
phase: 03
slug: format-detection-and-tes4-family-bsa-read-extract
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-05-08
---

# Phase 03 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 `3.14.0#0` via vcpkg and CTest discovery |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "unit|fixture|malformed" --output-on-failure` |
| **Full suite command** | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | TBD after fixture tests are added |

---

## Sampling Rate

- **After every task commit:** Run the relevant `ctest -R` command for the changed behavior plus `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure`.
- **After every plan wave:** Run `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite must be green, generated fixture provenance documented, malformed fixture set passing, public include boundary passing, and `git -C TES5Edit status --short` empty.
- **Max feedback latency:** TBD after the Wave 0 fixture suite exists.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 03-W0-01 | TBD | 0 | FMT-01 | T-03-01 / T-03-02 | Detection rejects non-BSA and malformed bytes without trusting filename extension | fixture + malformed | `ctest --preset windows-msvc-debug-static -R tes4_bsa_detection --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-02 | TBD | 0 | FMT-02 | T-03-01 / — | Archive metadata is parsed from bounded checked fields | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_metadata --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-03 | TBD | 0 | FMT-03 | T-03-03 / — | Public listing returns deterministic canonical paths and source spelling | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_listing --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-04 | TBD | 0 | FMT-04 | T-03-03 / T-03-05 | Lookup normalizes valid input and rejects rooted/traversal-like paths | unit + fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_lookup --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-05 | TBD | 0 | FMT-05 | T-03-01 / T-03-02 | Entry metadata exposes checked sizes, offsets, hashes, flags, and compression | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_entry_metadata --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-06 | TBD | 0 | FMT-06 | — | Public headers remain dependency-light and future/unknown versions classify cleanly | unit + malformed | `ctest --preset windows-msvc-debug-static -R "public_include_boundary|unsupported_future_bsa" --output-on-failure` | ✅ partial / ❌ W0 | ⬜ pending |
| 03-W0-07 | TBD | 0 | BSA-01 | T-03-04 / — | v103 raw and deflate entries extract exact expected bytes | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v103_extract --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-08 | TBD | 0 | BSA-02 | T-03-04 / — | v104 raw and deflate entries extract exact expected bytes | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v104_extract --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-09 | TBD | 0 | BSA-03 | T-03-04 / — | v105 raw and LZ4-frame entries extract exact expected bytes | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v105_extract --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-10 | TBD | 0 | BSA-05 | T-03-03 / — | Hash-compatible lookup routes normalized paths to the expected records | unit + fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_hash_lookup --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-11 | TBD | 0 | BSA-06 | T-03-01 / T-03-04 | Embedded-name prefixes are skipped for extracted payload bytes and exposed in metadata | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_embedded_name --output-on-failure` | ❌ W0 | ⬜ pending |
| 03-W0-12 | TBD | 0 | BSA-07 | T-03-04 / T-03-02 | Raw, deflate, and LZ4-frame routing fail closed on corrupt or size-mismatched payloads | fixture + malformed | `ctest --preset windows-msvc-debug-static -R tes4_bsa_compression_routing --output-on-failure` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/unit/tes4_bsa_reader_tests.cpp` — covers FMT-01 through FMT-05 and BSA-01 through BSA-07.
- [ ] `tests/fixtures/generated/generate_tes4_bsa_fixtures.*` — creates legal v103/v104/v105 success and malformed archives.
- [ ] `tests/fixtures/generated/archives/*.json` — manifests expected metadata, paths, extracted bytes/hashes, and provenance.
- [ ] Test-only `nlohmann-json` manifest dependency and CMake wiring for the test target only.
- [ ] Replace or update `tests/unit/archive_reader_tests.cpp` unsupported-stub assertions.

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

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency measured after fixture suite lands
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
