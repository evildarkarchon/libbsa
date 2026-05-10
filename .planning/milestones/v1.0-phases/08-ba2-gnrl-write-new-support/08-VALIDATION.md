---
phase: 08
slug: ba2-gnrl-write-new-support
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-09
updated: 2026-05-09
last_audited: 2026-05-09
---

# Phase 08 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 through CTest |
| **Config file** | `tests/CMakeLists.txt`, root `CMakeLists.txt`, `CMakePresets.json` |
| **Build command** | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` |
| **Targeted writer command** | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` |
| **Reader/layout command** | `ctest --preset windows-msvc-debug-static -R ba2_gnrl_end_table --output-on-failure` |
| **Regression command** | `ctest --preset windows-msvc-debug-static -R "ba2_gnrl_reader|ba2_dx10|tes4_bsa_writer|public_include_boundary" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **TES5Edit boundary command** | `git -C TES5Edit status --short` |
| **Estimated runtime** | ~1 second targeted writer tests once built; ~10 seconds full suite in the current Windows preset |

---

## Sampling Rate

- **After every task commit:** Run the targeted command for files touched by the task; for BA2 GNRL writer changes, run `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure`.
- **After public header changes:** Run `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure`.
- **After parser/layout changes:** Run `ctest --preset windows-msvc-debug-static -R "ba2_gnrl_reader|ba2_gnrl_end_table" --output-on-failure`.
- **After compression routing changes:** Run `ctest --preset windows-msvc-debug-static -R "compression_router|deflate_codec|lz4_codec|ba2_gnrl_reader" --output-on-failure`.
- **Before `/gsd-verify-work`:** Run the full suite and confirm `git -C TES5Edit status --short` is empty.
- **Max feedback latency:** one task commit.

---

## Requirement Coverage

| Requirement | Behavior | Status | Automated Evidence |
|-------------|----------|--------|--------------------|
| WBA2-01 | Consumer can create new Fallout 4 BA2 GNRL archives from disk files or memory buffers. | COVERED | `tests/unit/public_include_boundary_tests.cpp`; `tests/unit/ba2_gnrl_writer_tests.cpp` covers public API, disk source validation, copied memory, FO4 raw round-trip, dedupe, and publish safety. |
| WBA2-02 | Consumer can create new Starfield BA2 GNRL archives with explicit target version and compression method policy. | COVERED | `tests/unit/public_include_boundary_tests.cpp`; `tests/unit/ba2_gnrl_writer_tests.cpp` covers SFv2/SFv3 raw defaults, override metadata, method 0 deflate, and method 3 raw LZ4 block. |
| WBA2-03 | Writer can serialize BA2 filename tables at the end of the archive. | COVERED | `tests/unit/ba2_gnrl_reader_tests.cpp` has `[ba2_gnrl_end_table]`; `tests/unit/ba2_gnrl_writer_tests.cpp` physical-layout helper verifies `FileTableOffset` after payloads and final UInt16 names. |
| WBA2-04 | Writer can compress BA2 GNRL entries with deflate or raw LZ4 block according to target format/version. | COVERED | `tests/unit/ba2_gnrl_writer_tests.cpp` covers all-compressed FO4/SFv2/SFv3 method 0 deflate and SFv3 method 3 LZ4 block extraction. |
| WBA2-05 | Writer can preserve or set version-specific BA2 header fields according to documented target profiles. | COVERED | `tests/unit/ba2_gnrl_writer_tests.cpp` covers default and overridden Starfield unknown fields, v3 compression method metadata, and per-entry record flags. |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 08-01 | 08-01 | 1 | WBA2-01, WBA2-02, WBA2-05 | T-08-01-01, T-08-01-02 | Dependency-light public BA2 GNRL writer API exposes target/options without leaking private dependencies. | compile/unit | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | `tests/unit/public_include_boundary_tests.cpp` | COVERED |
| 08-02 | 08-02 | 1 | WBA2-03 | T-08-02-01, T-08-02-02 | Reader accepts payload-before-filename-table archives and preserves bounded path semantics. | parser regression | `ctest --preset windows-msvc-debug-static -R ba2_gnrl_end_table --output-on-failure` | `tests/unit/ba2_gnrl_reader_tests.cpp` | COVERED |
| 08-03 | 08-03 | 2 | WBA2-01 | T-08-03-01, T-08-03-02, T-08-03-03 | Writer state validates archive paths, duplicate canonical paths, disk sources, and overwrite defaults. | unit/integration | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` | `tests/unit/ba2_gnrl_writer_tests.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp` | COVERED |
| 08-04 | 08-04 | 3 | WBA2-01, WBA2-02, WBA2-03, WBA2-05 | T-08-04-01, T-08-04-02, T-08-04-03 | Raw FO4/SFv2/SFv3 archives reopen with copied-memory payloads, end filename tables, hashes, record flags, and Starfield metadata. | reader-backed integration | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` | `tests/unit/ba2_gnrl_writer_tests.cpp` | COVERED |
| 08-05 | 08-05 | 4 | WBA2-02, WBA2-04, WBA2-05 | T-08-05-01, T-08-05-02, T-08-05-03 | Compression routing uses explicit target/options metadata; deflate and raw LZ4 block payloads round-trip by reader metadata. | unit/integration | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` | `tests/unit/ba2_gnrl_writer_tests.cpp` | COVERED |
| 08-06 | 08-06 | 5 | WBA2-01, WBA2-02, WBA2-03, WBA2-04, WBA2-05 | T-08-06-01, T-08-06-02 | Opt-in final-stored-byte dedupe is verified through reopened offsets and extraction; final regression gates stay green. | unit/regression | `ctest --preset windows-msvc-debug-static --output-on-failure` | `tests/unit/ba2_gnrl_writer_tests.cpp` | COVERED |
| 08-07 | 08-07 | 6 | WBA2-01, WBA2-02, WBA2-03, WBA2-04, WBA2-05 | T-08-07-01, T-08-07-02, T-08-07-03 | Public writer finalization preserves caller-owned temp siblings and rejects unsafe overwrite targets before publish. | unit/regression | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` | `tests/unit/ba2_gnrl_writer_tests.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp` | COVERED |

---

## Generated And Existing Test Files

| File | Purpose | Requirements |
|------|---------|--------------|
| `tests/unit/public_include_boundary_tests.cpp` | Public API and dependency-boundary assertions for the BA2 GNRL writer surface. | WBA2-01, WBA2-02, WBA2-05 |
| `tests/unit/ba2_gnrl_reader_tests.cpp` | BA2 GNRL reader fixtures plus payload-before-final-name-table regression. | WBA2-03 |
| `tests/unit/ba2_gnrl_writer_tests.cpp` | Reader-backed writer validation for input errors, raw output, compression, Starfield metadata, dedupe, and publish safety. | WBA2-01, WBA2-02, WBA2-03, WBA2-04, WBA2-05 |

No new test file was generated during this audit because the Phase 8 execution artifacts already contain automated coverage for every requirement. The audit updated the validation contract to reflect the current tests and verification results.

---

## Gap Analysis

| Requirement | Existing Test File | Status | Notes |
|-------------|--------------------|--------|-------|
| WBA2-01 | `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | COVERED | Includes FO4 creation from disk/memory, validation errors, copied-memory proof, dedupe, and filesystem publish safety. |
| WBA2-02 | `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | COVERED | Includes SFv2/SFv3 targets and explicit v3 compression method policy. |
| WBA2-03 | `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp` | COVERED | Includes reader regression and writer physical layout checks. |
| WBA2-04 | `tests/unit/ba2_gnrl_writer_tests.cpp` | COVERED | Includes deflate and raw LZ4 block round-trip extraction. |
| WBA2-05 | `tests/unit/ba2_gnrl_writer_tests.cpp` | COVERED | Includes default/override Starfield fields, compression method metadata, and record flags. |

Current audit found no PARTIAL or MISSING requirements. Because no gaps were present, the workflow gap-approval question and `gsd-nyquist-auditor` test-generation step were skipped.

---

## Verification Evidence

| Command | Result |
|---------|--------|
| `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | PASS |
| `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` | PASS, 16/16 tests |
| `ctest --preset windows-msvc-debug-static -R "ba2_gnrl_reader|ba2_dx10|tes4_bsa_writer|public_include_boundary" --output-on-failure` | PASS, 27/27 tests |
| `ctest --preset windows-msvc-debug-static --output-on-failure` | PASS, 155/155 tests with `local game fixtures are opt-in` skipped as expected |
| `git -C TES5Edit status --short` | PASS, no output |

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None | None | All Phase 8 requirements have automated verification. | None |

---

## Validation Audit 2026-05-09

| Metric | Count |
|--------|-------|
| Requirements audited | 5 |
| Covered | 5 |
| Partial | 0 |
| Missing | 0 |
| Gaps found | 0 |
| Resolved by new tests in this audit | 0 |
| Escalated | 0 |

The existing `08-VALIDATION.md` was a stale Wave 0 draft that marked the planned writer tests as missing. This audit reconciled it with completed Plans 08-01 through 08-07, including the publish-safety gap closure added after `08-VERIFICATION.md`.

---

## Validation Sign-Off

- [x] All tasks have automated verify or completed equivalent coverage
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 references are closed by implemented test files
- [x] No watch-mode flags
- [x] Feedback latency is one task commit
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** Phase 08 is Nyquist-compliant.
