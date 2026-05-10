---
phase: 06
slug: dds-boundary-and-ba2-dx10-read-reconstruction
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-08
updated: 2026-05-08
audit_state: A-existing-validation-audited
---

# Phase 06 - Validation Strategy

> Per-phase validation contract and Nyquist audit for DDS boundary and BA2 DX10 read/reconstruction coverage.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 `3.14.0#0` via vcpkg + CTest |
| **Config files** | `tests/CMakeLists.txt`, `CMakePresets.json` |
| **Fixture generation** | `cmake --build --preset windows-msvc-debug-static --target generate_ba2_dx10_fixtures` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -R "ba2_dx10|dds_layout|public_include_boundary" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Boundary command** | `git -C TES5Edit status --short` |
| **Audit result** | Targeted Phase 6 suite passed 19/19; full static suite passed 93/93 with the opt-in local game fixture skipped |
| **Estimated runtime** | Targeted Phase 6 suite: about 3 seconds; full static suite: about 17 seconds on this machine |

---

## Sampling Rate

- **After DX10 parser or extraction changes:** Run `ctest --preset windows-msvc-debug-static -R "ba2_dx10|dds_layout|public_include_boundary" --output-on-failure`.
- **After fixture generator changes:** Run `cmake --build --preset windows-msvc-debug-static --target generate_ba2_dx10_fixtures`, then the targeted Phase 6 CTest command.
- **After public header changes:** Run `ctest --preset windows-msvc-debug-static -R "public_include_boundary|ba2_dx10_metadata" --output-on-failure`.
- **Before accepting Phase 6 or related regressions:** Run `ctest --preset windows-msvc-debug-static --output-on-failure` and `git -C TES5Edit status --short`.
- **Max feedback latency:** About 3 seconds for targeted Phase 6 checks; about 17 seconds for full static regression on this machine.

---

## Requirement Coverage Map

| Requirement | Behavior | Test Evidence | Command | Status |
|-------------|----------|---------------|---------|--------|
| DDS-01 | Generated FO4 BA2 DX10 archives open by archive bytes and malformed DX10 headers fail closed. | `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp` | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_detector|ba2_dx10_malformed" --output-on-failure` | COVERED |
| DDS-02 | Generated Starfield v3 BA2 DX10 archives open by bytes and route compression metadata from parsed version/method fields. | `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp` | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_detector|ba2_dx10_compression" --output-on-failure` | COVERED |
| DDS-03 | Public texture metadata exposes dimensions, mip count, DXGI format id, array/cubemap state, and chunk metadata without private dependency types. | `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_metadata|public_include_boundary" --output-on-failure` | COVERED |
| DDS-04 | DX10 extraction reconstructs complete DDS DXT10 bytes through sink and byte-vector APIs. | `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/dds_layout_tests.cpp` | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_extract|dds_layout" --output-on-failure` | COVERED |
| DDS-05 | Raw, deflate, and Starfield raw-LZ4-block DX10 chunks route by parsed metadata with exact-size validation. | `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp` | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_compression|ba2_dx10_malformed" --output-on-failure` | COVERED |
| DDS-06 | DirectXTex validation stays behind an internal boundary and public headers remain dependency-light. | `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_directxtex|ba2_dx10_metadata|public_include_boundary" --output-on-failure` | COVERED |
| DDS-07 | Cubemap and array texture ordering validates mip/face/slice coverage before extraction. | `tests/unit/dds_layout_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp` | `ctest --preset windows-msvc-debug-static -R "dds_layout|ba2_dx10_layout|ba2_dx10_extract" --output-on-failure` | COVERED |
| SPEC-08 | Legal generated DX10 fixtures and manifests cover success and malformed cases outside `TES5Edit/`. | `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, generated `tests/fixtures/generated/archives/ba2_dx10_*`, `tests/CMakeLists.txt` | `cmake --build --preset windows-msvc-debug-static --target generate_ba2_dx10_fixtures` | COVERED |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement IDs | Automated Command | Primary Files | Status |
|---------|------|------|-----------------|-------------------|---------------|--------|
| 06-01-T1 | 06-01 | 1 | DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07, SPEC-08 | `cmake --build --preset windows-msvc-debug-static --target generate_ba2_dx10_fixtures` | `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, generated DX10 fixtures/manifests | COVERED |
| 06-01-T2 | 06-01 | 1 | DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07 | `ctest --preset windows-msvc-debug-static -N -R "ba2_dx10|dds_layout"` | `tests/CMakeLists.txt`, `tests/unit/ba2_dx10_*`, `tests/unit/dds_layout_tests.cpp` | COVERED |
| 06-02-T1 | 06-02 | 2 | DDS-03, DDS-06 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_metadata|public_include_boundary" --output-on-failure` | `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | COVERED |
| 06-02-T2 | 06-02 | 2 | DDS-03, DDS-06 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_metadata|public_include_boundary" --output-on-failure` | `include/libbsa/archive.hpp`, `src/texture/directxtex_analyzer.*`, `CMakeLists.txt` | COVERED |
| 06-02-T3 | 06-02 | 2 | DDS-03, DDS-06 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_metadata|public_include_boundary" --output-on-failure` | `include/libbsa/archive.hpp`, `src/texture/directxtex_analyzer.*` | COVERED |
| 06-03-T1 | 06-03 | 3 | DDS-04, DDS-07 | `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` | `tests/unit/dds_layout_tests.cpp` | COVERED |
| 06-03-T2 | 06-03 | 3 | DDS-04, DDS-07 | `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` | `src/texture/dds_layout.*`, `CMakeLists.txt` | COVERED |
| 06-03-T3 | 06-03 | 3 | DDS-04, DDS-07 | `ctest --preset windows-msvc-debug-static -R dds_layout --output-on-failure` | `src/texture/dds_layout.*` | COVERED |
| 06-04-T1 | 06-04 | 4 | DDS-01, DDS-02, DDS-03, DDS-07 | `ctest --preset windows-msvc-debug-static -R ba2_dx10_detector --output-on-failure` | `tests/unit/ba2_dx10_parser_tests.cpp` | COVERED |
| 06-04-T2 | 06-04 | 4 | DDS-01, DDS-02, DDS-03, DDS-07 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_detector|ba2_dx10_metadata|ba2_dx10_layout" --output-on-failure` | `src/formats/ba2/ba2_dx10_parser.*`, `src/formats/ba2/ba2_dx10_reader.*`, `src/archive.cpp` | COVERED |
| 06-04-T3 | 06-04 | 4 | DDS-01, DDS-02, DDS-03, DDS-07 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_detector|ba2_dx10_metadata|ba2_dx10_layout" --output-on-failure` | `src/formats/ba2/ba2_dx10_parser.cpp`, `src/archive.cpp` | COVERED |
| 06-05-T1 | 06-05 | 5 | DDS-04, DDS-05, DDS-06, DDS-07 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_extract|ba2_dx10_compression|ba2_dx10_directxtex" --output-on-failure` | `tests/unit/ba2_dx10_extraction_tests.cpp` | COVERED |
| 06-05-T2 | 06-05 | 5 | DDS-04, DDS-05, DDS-06, DDS-07 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_extract|ba2_dx10_compression|ba2_dx10_directxtex|ba2_dx10_layout" --output-on-failure` | `src/formats/ba2/ba2_dx10_reader.*`, `src/archive.cpp` | COVERED |
| 06-05-T3 | 06-05 | 5 | DDS-04, DDS-05, DDS-06, DDS-07 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_extract|ba2_dx10_compression|ba2_dx10_directxtex" --output-on-failure` | `src/formats/ba2/ba2_dx10_reader.cpp`, `src/archive.cpp` | COVERED |
| 06-06-T1 | 06-06 | 6 | DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07 | `ctest --preset windows-msvc-debug-static -R ba2_dx10_malformed --output-on-failure` | `tests/unit/ba2_dx10_malformed_tests.cpp` | COVERED |
| 06-06-T2 | 06-06 | 6 | DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10|public_include_boundary" --output-on-failure` | `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp` | COVERED |
| 06-06-T3 | 06-06 | 6 | DDS-01, DDS-02, DDS-03, DDS-04, DDS-05, DDS-06, DDS-07 | `ctest --preset windows-msvc-debug-static --output-on-failure` | Phase 6 test suite and regression surface | COVERED |
| 06-07-T1 | 06-07 | 7 | DDS-01, DDS-02, DDS-03 | `ctest --preset windows-msvc-debug-static -R ba2_dx10_detector --output-on-failure` | `tests/unit/ba2_dx10_parser_tests.cpp` | COVERED |
| 06-07-T2 | 06-07 | 7 | DDS-01, DDS-02, DDS-03 | `ctest --preset windows-msvc-debug-static -R "ba2_dx10_detector|ba2_dx10_metadata|ba2_dx10_layout" --output-on-failure` | `src/formats/ba2/ba2_dx10_parser.cpp` | COVERED |
| 06-08-T1 | 06-08 | 7 | DDS-01, DDS-02, DDS-03, DDS-05 | `ctest --preset windows-msvc-debug-static -R ba2_dx10_malformed --output-on-failure` | `tests/unit/ba2_dx10_malformed_tests.cpp` | COVERED |
| 06-08-T2 | 06-08 | 7 | DDS-01, DDS-02, DDS-03, DDS-05, SPEC-08 | `cmake --build --preset windows-msvc-debug-static --target generate_ba2_dx10_fixtures && ctest --preset windows-msvc-debug-static -R ba2_dx10_malformed --output-on-failure` | `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, generated malformed fixtures/manifests | COVERED |

---

## Generated/Updated Test Files

| Path | Coverage |
|------|----------|
| `tests/unit/ba2_dx10_metadata_tests.cpp` | Public texture metadata and non-DX10 optional texture behavior |
| `tests/unit/dds_layout_tests.cpp` | DDS DXT10 header constants, cubemap/array order, and layout fail-closed cases |
| `tests/unit/ba2_dx10_parser_tests.cpp` | FO4/SF v3 open, metadata, lookup, sparse bounded-open, and validated order |
| `tests/unit/ba2_dx10_extraction_tests.cpp` | DDS reconstruction, sink/vector equivalence, DirectXTex validation, and codec routing |
| `tests/unit/ba2_dx10_malformed_tests.cpp` | Malformed open/extraction errors, duplicate canonical path, unsupported compression, and strict manifest error strings |
| `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` | Legal generated success and malformed BA2 DX10 fixture corpus |
| `tests/fixtures/generated/archives/ba2_dx10_*` | Committed generated fixtures and manifests consumed by CTest |

No additional test files were generated during this validation audit because the audit found no missing automated coverage after Plans 06-07 and 06-08 closed the earlier verifier gaps.

---

## Manual-Only Verifications

None required for Phase 6 acceptance. Optional local real-game archive checks remain out of scope and must not be required by default.

---

## Validation Audit 2026-05-08

| Metric | Count |
|--------|-------|
| Input state | State A |
| Requirements checked | 8 |
| Automated requirements covered | 8 |
| Gaps found | 0 |
| Resolved by this audit | 0 |
| Escalated manual-only | 0 |
| New test files generated by this audit | 0 |

| Command | Result |
|---------|--------|
| `ctest --preset windows-msvc-debug-static -N -R "ba2_dx10|dds_layout|public_include_boundary"` | Listed 19 Phase 6/public-boundary tests |
| `ctest --preset windows-msvc-debug-static -R "ba2_dx10|dds_layout|public_include_boundary" --output-on-failure` | Passed 19/19 |
| `ctest --preset windows-msvc-debug-static --output-on-failure` | Passed 93/93 with one opt-in local game fixture skipped |
| `git -C TES5Edit status --short` | Passed with no output |

---

## Validation Sign-Off

- [x] All Phase 6 task requirements have automated verification commands.
- [x] Sampling continuity: no 3 consecutive tasks lack automated verification.
- [x] Fixture generation, parser, extraction, malformed, sparse, DirectXTex boundary, and public include-boundary checks are automated.
- [x] No watch-mode flags are part of validation commands.
- [x] Feedback latency measured from current CTest runs.
- [x] `nyquist_compliant: true` is set in frontmatter.

**Approval:** Nyquist-compliant
