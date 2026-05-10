---
phase: 04
slug: tes3-bsa-read-extract
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-08
updated: 2026-05-08
---

# Phase 04 - Validation Strategy

> Per-phase validation contract and Nyquist coverage audit for TES3/Morrowind BSA read/extract behavior.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 through `Catch2::Catch2WithMain`, discovered by CTest with tags as labels. |
| **Config file** | `tests/CMakeLists.txt` plus `CMakePresets.json`. |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` |
| **Phase 4 focused command** | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_metadata|tes3_bsa_entries|tes3_bsa_lookup|tes3_bsa_extract|tes3_bsa_malformed|tes3_bsa_detector|tes4_bsa|public_include_boundary|bethesda_hash" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~20 seconds after build on the current Windows MSVC debug-static preset. |

---

## Sampling Rate

- **After every task commit:** Run `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` after build.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`.
- **Before `/gsd-verify-work`:** Full suite must be green, generated fixture outputs committed, public include boundary tests pass, and `git -C TES5Edit status --short` must be empty.
- **Max feedback latency:** 60 seconds for quick unit feedback.

---

## Requirement Coverage Matrix

| Requirement | Covered By | Evidence | Status |
|-------------|------------|----------|--------|
| BSA-04 | TES3 detector, metadata, entries, lookup, malformed, and regression tests. | `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/bethesda_hash_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp` | ✅ green |
| BSA-08 | Manifest-backed raw TES3 offset conversion, archive-absolute payload offsets, extraction byte checks, malformed raw-offset regression, and bounded host-file streaming tests. | `tests/fixtures/generated/archives/tes3_success_manifest.json`, `tests/fixtures/generated/archives/tes3_malformed_manifest.json`, `tests/unit/tes3_bsa_reader_tests.cpp` | ✅ green |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 04-01-01 | 01 | 1 | BSA-04/BSA-08 | T-04-01/T-04-02 | TES3 success and malformed fixtures are generated from repository-owned bytes with raw and archive-absolute offset evidence. | fixture generator + manifest checks | `cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures` | ✅ | ✅ green |
| 04-01-02 | 01 | 1 | BSA-04/BSA-08 | T-04-01/T-04-02 | Public reader tests cover TES3 open, metadata listing, lookup, extraction, stored hashes, and offset conversion through `archive_reader`. | unit + fixture | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_metadata|tes3_bsa_entries|tes3_bsa_lookup|tes3_bsa_extract" --output-on-failure` | ✅ | ✅ green |
| 04-01-03 | 01 | 1 | BSA-04/BSA-08 | T-04-03 | Malformed TES3 fixture cases assert stable error codes for truncation, spans, duplicates, hash mismatch/collision, unsorted hashes, and raw-offset regression. | unit + malformed fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure` | ✅ | ✅ green |
| 04-02-01 | 02 | 2 | BSA-04/BSA-08 | T-04-06/T-04-07 | Public metadata uses format-neutral `archive_hash`, and payload offsets are documented as archive-absolute without exposing private parser details. | regression + public API | `ctest --preset windows-msvc-debug-static -L "tes4_bsa|public_include_boundary" --output-on-failure` | ✅ | ✅ green |
| 04-02-02 | 02 | 2 | BSA-04 | T-04-05/T-04-08 | TES3 magic/version bytes classify as TES3 without misrouting unrelated data or regressing TES4-family detection. | unit + fixture | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_detector|tes4_bsa" --output-on-failure` | ✅ | ✅ green |
| 04-03-01 | 03 | 3 | BSA-04/BSA-08 | T-04-09/T-04-10 | TES3 parser opens generated archives, validates table spans, and materializes archive metadata with archive-absolute payload offsets. | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_metadata --output-on-failure` | ✅ | ✅ green |
| 04-03-02 | 03 | 3 | BSA-04/BSA-08 | T-04-11/T-04-12/T-04-13 | Stored TES3 hashes, names, ordering, duplicate paths, and malformed hash cases are strictly validated. | unit + malformed fixture | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_entries|tes3_bsa_malformed|bethesda_hash" --output-on-failure` | ✅ | ✅ green |
| 04-03-03 | 03 | 3 | BSA-04 | T-04-11 | TES3 `entries()`, `find()`, and `contains()` use normalized lookup semantics while preserving TES4-family behavior. | unit + regression | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_lookup|tes3_bsa_entries|tes4_bsa|public_include_boundary" --output-on-failure` | ✅ | ✅ green |
| 04-04-01 | 04 | 4 | BSA-04/BSA-08 | T-04-14/T-04-16/T-04-17/T-04-18 | TES3 extraction returns exact raw bytes, enforces raw-only metadata, supports zero-byte entries, and reports sink failures. | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_extract --output-on-failure` | ✅ | ✅ green |
| 04-04-02 | 04 | 4 | BSA-04/BSA-08 | T-04-14/T-04-15 | Malformed TES3 spans, overlaps, counts, names, duplicate paths, hashes, and raw-offset regression cases fail closed. | unit + malformed fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure` | ✅ | ✅ green |
| 04-04-03 | 04 | 4 | BSA-04/BSA-08 | T-04-19 | Final Phase 4 quick/full validation and TES5Edit cleanliness gates pass. | full regression + repo boundary | `ctest --preset windows-msvc-debug-static --output-on-failure && pwsh -NoProfile -Command '$status = git -C TES5Edit status --short; if ($status) { throw "TES5Edit has unexpected changes: $status" }'` | ✅ | ✅ green |
| 04-05-01 | 05 | 5 | BSA-04/BSA-08 | T-04-G05-01 | TES3 hash-collision malformed coverage reaches duplicate stored-hash validation instead of only stored-hash mismatch. | generated fixture + malformed test | `ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure` | ✅ | ✅ green |
| 04-05-02 | 05 | 5 | BSA-04/BSA-08 | T-04-G05-02/T-04-G05-03/T-04-G05-04 | TES3 extraction streams bounded chunks directly from the host archive before generic payload buffering, while TES4-family extraction remains unchanged. | unit + regression | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_extract|tes4_bsa" --output-on-failure` | ✅ | ✅ green |
| 04-05-03 | 05 | 5 | BSA-04/BSA-08 | T-04-G05-05 | Gap-closure focused labels, full regression, public boundary, and TES5Edit read-only gate pass. | full regression + repo boundary | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_malformed|tes3_bsa_extract|tes4_bsa|public_include_boundary" --output-on-failure && ctest --preset windows-msvc-debug-static --output-on-failure && pwsh -NoProfile -Command '$status = git -C TES5Edit status --short; if ($status) { throw "TES5Edit has unexpected changes: $status" }'` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Test File Cross-Reference

| Requirement / Behavior | Test File | Labels / Evidence | Status |
|------------------------|-----------|-------------------|--------|
| TES3 detector routing | `tests/unit/tes3_bsa_reader_tests.cpp` | `tes3_bsa_detector` | COVERED |
| TES3 open and metadata | `tests/unit/tes3_bsa_reader_tests.cpp` | `tes3_bsa_metadata` | COVERED |
| TES3 entry listing and stored hash metadata | `tests/unit/tes3_bsa_reader_tests.cpp` | `tes3_bsa_entries` | COVERED |
| TES3 lookup semantics | `tests/unit/tes3_bsa_reader_tests.cpp` | `tes3_bsa_lookup` | COVERED |
| TES3 extraction and raw-offset conversion | `tests/unit/tes3_bsa_reader_tests.cpp` | `tes3_bsa_extract`, manifest `raw_tes3_data_offset` and `payload_offset` assertions | COVERED |
| TES3 malformed fail-closed behavior | `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/fixtures/generated/archives/tes3_malformed_manifest.json` | `tes3_bsa_malformed`, `structural_issue` markers for mismatch and duplicate hash cases | COVERED |
| TES3 hash sort helpers | `tests/unit/bethesda_hash_tests.cpp` | `bethesda_hash` | COVERED |
| TES4-family regression | `tests/unit/tes4_bsa_reader_tests.cpp` | `tes4_bsa_*` labels | COVERED |
| Public include boundary | `tests/unit/public_include_boundary_tests.cpp` | `public-api` | COVERED |

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions | Status |
|----------|-------------|------------|-------------------|--------|
| TES5Edit submodule remains read-only | BSA-04/BSA-08 | Git submodule cleanliness is a repository state check, not a Catch2 assertion. | Run `git -C TES5Edit status --short` and verify it produces no output. | ✅ verified 2026-05-08 |

---

## Validation Audit 2026-05-08

| Metric | Count |
|--------|-------|
| Input state | A - existing `04-VALIDATION.md` audited |
| Requirements audited | 2 |
| Phase tasks audited | 14 |
| Automated coverage rows | 14 |
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |
| Manual-only rows | 1 |

### Commands Run

| Command | Result |
|---------|--------|
| `cmake --build --preset windows-msvc-debug-static` | ✅ passed |
| `ctest --preset windows-msvc-debug-static -L "tes3_bsa_metadata|tes3_bsa_entries|tes3_bsa_lookup|tes3_bsa_extract|tes3_bsa_malformed|tes3_bsa_detector|tes4_bsa|public_include_boundary|bethesda_hash" --output-on-failure` | ✅ 31/31 passed |
| `ctest --preset windows-msvc-debug-static --output-on-failure` | ✅ 92 passed, 1 skipped opt-in local fixture, 0 failed |
| `git -C TES5Edit status --short` | ✅ no output |

---

## Validation Sign-Off

- [x] All tasks have automated verification or explicit manual-only boundary checks
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 requirements are satisfied by committed generator, fixtures, manifests, and test files
- [x] No watch-mode flags
- [x] Feedback latency < 60s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete
