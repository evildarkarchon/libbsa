---
phase: 09
slug: ba2-dx10-write-new-support
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-09
updated: 2026-05-09
last_audited: 2026-05-09
---

# Phase 09 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 through CTest |
| **Config file** | `tests/CMakeLists.txt`, root `CMakeLists.txt`, `CMakePresets.json` |
| **Build command** | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` |
| **Targeted writer command** | `ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure` |
| **Layout command** | `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` |
| **Public boundary command** | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` |
| **Regression command** | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer|dds_layout|ba2_dx10|public_include_boundary" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **TES5Edit boundary command** | `git -C TES5Edit status --short` |
| **Estimated runtime** | ~1 second focused Phase 9 regression once built; ~9 seconds full suite in the current Windows preset |

---

## Sampling Rate

- **After every task commit:** Run the command named in that plan's `<verify>` block.
- **After public header changes:** Run `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure`.
- **After DDS layout or chunk-planning changes:** Run `ctest --preset windows-msvc-debug-static -R "dds_layout|ba2_dx10_writer|ba2_dx10" --output-on-failure`.
- **After compression or archive serialization changes:** Run `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer|compression_router|deflate_codec|lz4_codec" --output-on-failure`.
- **Before `/gsd-verify-work`:** Run the full suite and confirm `git -C TES5Edit status --short` is empty.
- **Max feedback latency:** one task commit.

---

## Requirement Coverage

| Requirement | Behavior | Status | Automated Evidence |
|-------------|----------|--------|--------------------|
| WBA2-06 | Consumer can create new Fallout 4 BA2 DX10/DDS texture archives from DDS files. | COVERED | `tests/unit/public_include_boundary_tests.cpp` proves the public `ba2_dx10_target::fallout4` writer surface; `tests/unit/ba2_dx10_writer_tests.cpp` reopens FO4 deflate writer output through `archive_reader`, validates metadata, and extracts DDS bytes. |
| WBA2-07 | Consumer can create new Starfield BA2 v3 DX10/DDS texture archives from DDS files. | COVERED | `tests/unit/public_include_boundary_tests.cpp` proves the public Starfield target; `tests/unit/ba2_dx10_writer_tests.cpp` reopens Starfield v3 method `3` raw-LZ4 output and method `0` deflate output. |
| WBA2-08 | Writer can analyze DDS input through DirectXTex and generate BA2 texture records from library-owned metadata. | COVERED | `tests/unit/ba2_dx10_writer_tests.cpp` covers the DDS source manifest, malformed/unsupported DDS rejection, `add_file` validation, snapshot semantics, and canonical record metadata. |
| WBA2-09 | Writer can split DDS textures into compatible mip/chunk records with configurable chunk limits. | COVERED | `tests/unit/dds_layout_tests.cpp` covers locked DXGI mip sizes, default/capped chunk planning, arrays, cubemaps, invalid layouts, and Plan 09-07 hostile `UINT32_MAX` block-compressed dimensions. |
| WBA2-10 | Writer can apply per-chunk compression and serialize chunk metadata so extracted DDS output remains valid. | COVERED | `tests/unit/ba2_dx10_writer_tests.cpp` covers FO4 deflate, Starfield v3 raw-LZ4, Starfield method 0 deflate, structural DDS extraction, DX10 dedupe offsets, publish safety, and rollback helper behavior. |
| WBA2-11 | Maintainer can round-trip BA2 writer output by packing, reopening, extracting, and byte-comparing or metadata-validating source files. | COVERED | `tests/unit/ba2_dx10_writer_tests.cpp` uses `archive_reader::open`, `find`, `contains`, `extract(payload_sink&)`, `extract_bytes`, DirectXTex-backed metadata comparison, and source `image_payload_bytes` comparison. |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 09-01 | 09-01 | 1 | WBA2-06, WBA2-07 | T-09-01-01, T-09-01-02, T-09-01-03 | Public DX10 writer API is dependency-light, DDS-host-file-only, and compressed-only with no raw override surface. | compile/unit | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | `tests/unit/public_include_boundary_tests.cpp` | COVERED |
| 09-02 | 09-02 | 1 | WBA2-08, WBA2-11 | T-09-02-01, T-09-02-02, T-09-02-03 | DDS source bytes validate through private DirectXTex analysis and fixture inputs are repository-owned outside `TES5Edit/`. | unit/fixture | `ctest --preset windows-msvc-debug-static -R "BA2 DX10 writer DDS|public_include_boundary" --output-on-failure` | `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json` | COVERED |
| 09-03 | 09-03 | 1 | WBA2-09 | T-09-03-01, T-09-03-02, T-09-03-03 | Locked DXGI format sizing and chunk planning use checked arithmetic and reject gaps, duplicates, unsupported formats, and impossible caps. | unit | `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` | `tests/unit/dds_layout_tests.cpp` | COVERED |
| 09-04 | 09-04 | 2 | WBA2-08 | T-09-04-01, T-09-04-02, T-09-04-03 | `add_file` validates host paths and DDS bytes at add time, snapshots writer-owned DDS bytes, and defers duplicate archive-path validation to write time. | unit/integration | `ctest --preset windows-msvc-debug-static -R ba2_dx10_writer.*add --output-on-failure` | `tests/unit/ba2_dx10_writer_tests.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp` | COVERED |
| 09-05 | 09-05 | 3 | WBA2-06, WBA2-07, WBA2-10, WBA2-11 | T-09-05-01, T-09-05-02, T-09-05-03, T-09-05-04 | FO4 and Starfield DX10 archives serialize compressed chunks by target/options metadata and prove extracted DDS metadata plus payload bytes. | reader-backed integration | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer.*(fo4|starfield|compression)" --output-on-failure` | `tests/unit/ba2_dx10_writer_tests.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp` | COVERED |
| 09-06 | 09-06 | 4 | WBA2-10, WBA2-11 | T-09-06-01, T-09-06-02, T-09-06-03, T-09-06-04 | Optional dedupe keys final stored bytes plus chunk metadata, and publish uses unique temp directories with backup/rollback safety. | unit/regression | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer.*(dedupe|publish|overwrite|temp)" --output-on-failure` | `tests/unit/ba2_dx10_writer_tests.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp` | COVERED |
| 09-07 | 09-07 | 5 | WBA2-09, WBA2-10 | T-09-gap-01, T-09-gap-02, T-09-gap-03 | Hostile block-compressed DDS dimensions cannot wrap before raw-size validation; BC7 overflow fails closed. | unit/regression | `ctest --preset windows-msvc-debug-static -R "dds_layout|ba2_dx10_writer|ba2_dx10|public_include_boundary" --output-on-failure` | `tests/unit/dds_layout_tests.cpp`, `src/texture/dds_layout.cpp` | COVERED |

---

## Generated And Existing Test Files

| File | Purpose | Requirements |
|------|---------|--------------|
| `tests/unit/public_include_boundary_tests.cpp` | Public API contract and dependency-boundary assertions for the BA2 DX10 writer surface. | WBA2-06, WBA2-07 |
| `tests/unit/ba2_dx10_writer_tests.cpp` | DDS fixture manifest validation, add-time validation, FO4/Starfield writer round trips, compression routing, structural DDS extraction, dedupe, and publish safety. | WBA2-06, WBA2-07, WBA2-08, WBA2-10, WBA2-11 |
| `tests/unit/dds_layout_tests.cpp` | Locked-format mip sizing, DX10 chunk planning, array/cubemap repetition, invalid layout rejection, and hostile block-compressed dimension regression coverage. | WBA2-09, WBA2-10 |
| `tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json` | Generated source DDS manifest for locked formats, structural cases, malformed cases, and unsupported cases. | WBA2-08, WBA2-11 |
| `tests/fixtures/generated/source/*.dds` | Generated legal synthetic DDS inputs used by writer and analyzer tests. | WBA2-08, WBA2-11 |

No new test file was generated during this audit because completed Phase 9 artifacts already contain automated coverage for every requirement. The prior verification gap was closed by Plan 09-07 through `tests/unit/dds_layout_tests.cpp` hostile block-compressed dimension tests and the corresponding `src/texture/dds_layout.cpp` arithmetic fix.

---

## Gap Analysis

| Requirement | Existing Test File | Status | Notes |
|-------------|--------------------|--------|-------|
| WBA2-06 | `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp` | COVERED | FO4 public API and deflate writer round-trip are covered. |
| WBA2-07 | `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp` | COVERED | Starfield v3 public API, default method `3`, and method `0` writer round-trips are covered. |
| WBA2-08 | `tests/unit/ba2_dx10_writer_tests.cpp` | COVERED | DDS source manifest, DirectXTex analyzer output, add-time validation errors, and snapshot behavior are covered. |
| WBA2-09 | `tests/unit/dds_layout_tests.cpp` | COVERED | Default/capped chunk planning, arrays/cubemaps, invalid layouts, and hostile BC dimension regression are covered. |
| WBA2-10 | `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/dds_layout_tests.cpp` | COVERED | Compression routing, chunk metadata, structural DDS extraction, dedupe, publish safety, and raw-size validation are covered. |
| WBA2-11 | `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/fixtures/generated/source/*` | COVERED | Reader-backed reopen/list/find/contains/extract validation and source payload comparisons are covered. |

Current audit found no PARTIAL or MISSING requirements. Because no gaps were present, the workflow gap-approval question and `gsd-nyquist-auditor` test-generation step were skipped.

---

## Verification Evidence

| Command | Result |
|---------|--------|
| `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | PASS |
| `ctest --preset windows-msvc-debug-static -R "ba2_dx10_writer|dds_layout|public_include_boundary" --output-on-failure` | PASS, 22/22 tests |
| `ctest --preset windows-msvc-debug-static --output-on-failure` | PASS, 154/155 tests passed with `local game fixtures are opt-in` skipped as expected |
| `git -C TES5Edit status --short` | PASS, no output |

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Optional real-game loading of generated DX10 archives | WBA2-06, WBA2-07 | Game installs and local modding environments are not mandatory CI inputs. Automated reader-backed tests are the required verification. | If local FO4/SF environments are available, pack generated textures, load them in target game/tooling, and record observations separately from mandatory tests. |

---

## Validation Audit 2026-05-09

| Metric | Count |
|--------|-------|
| Requirements audited | 6 |
| Covered | 6 |
| Partial | 0 |
| Missing | 0 |
| Gaps found | 0 |
| Resolved by new tests in this audit | 0 |
| Escalated | 0 |

The existing `09-VALIDATION.md` was a stale Wave 0 draft that marked planned writer tests as pending. This audit reconciled it with completed Plans 09-01 through 09-07, including the hostile block-compressed DDS sizing gap closure that resolved the blocker reported in `09-VERIFICATION.md`.

---

## Validation Sign-Off

- [x] All tasks have automated verify or completed equivalent coverage.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 references are closed by implemented test files.
- [x] No watch-mode flags.
- [x] Feedback latency is one task commit.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** Phase 09 is Nyquist-compliant.
