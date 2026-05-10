---
phase: 03
slug: format-detection-and-tes4-family-bsa-read-extract
status: secured
threats_open: 0
asvs_level: 1
created: 2026-05-08
updated: 2026-05-08
---

# Phase 03 Security Audit

## SECURED

**Phase:** 03 — format-detection-and-tes4-family-bsa-read-extract  
**Threats Closed:** 26/26  
**ASVS Level:** 1

## Scope

This audit verifies declared mitigations from all Phase 03 plan `<threat_model>` blocks and executor-reported `## Threat Flags` sections after the bounded parsing/extraction fix. Implementation files were treated as read-only; only this `03-SECURITY.md` artifact was updated.

## Threat Verification

| Threat ID | Category | Component | Disposition | Status | Evidence |
|-----------|----------|-----------|-------------|--------|----------|
| 03-01/T-03-01 | Information Disclosure | public headers | mitigate | CLOSED | `include/libbsa/archive.hpp:1-12` uses standard/libbsa headers only; `tests/unit/public_include_boundary_tests.cpp:45-70` scans installed public headers for private dependency tokens; `ctest` passed public boundary tests. |
| 03-01/T-03-02 | Tampering | path-taking method contracts | mitigate | CLOSED | `include/libbsa/archive.hpp:110-121` declares result-returning `find`, `contains`, and `extract`; implementation normalizes lookup/extraction inputs in `src/formats/bsa/tes4_bsa_reader.cpp:110-132` plus public extraction maps valid missing paths in `src/archive.cpp:169-180`; invalid/missing path tests at `tests/unit/tes4_bsa_reader_tests.cpp:318-358` and `420-431`. |
| 03-01/T-03-03 | Denial of Service | `extract_bytes` contract | mitigate | CLOSED | `include/libbsa/archive.hpp:123-124` documents bounded byte extraction; `src/archive.cpp:26-45` and `183-202` build a per-entry vector sink reserved from parsed `raw_size`; extraction-path tests at `tests/unit/tes4_bsa_reader_tests.cpp:467-501`. |
| 03-01/T-03-04 | Tampering / DoS | `payload_sink` | mitigate | CLOSED | `include/libbsa/archive.hpp:76-86` requires sink accepted count; `src/formats/bsa/tes4_bsa_reader.cpp:47-56` maps partial writes to `io_error`; test evidence `tests/unit/tes4_bsa_reader_tests.cpp:408-418`. |
| 03-01/T-03-05 | Information Disclosure | nlohmann-json dependency | mitigate | CLOSED | `tests/CMakeLists.txt:20-25` links `nlohmann_json::nlohmann_json` only to `libbsa_tests` PRIVATE; `CMakeLists.txt:36-40` shows runtime `libbsa` links only compression dependencies; public boundary scan passed. |
| 03-02/T-03-01 | Tampering | success fixture bytes | mitigate | CLOSED | Generator target and declared byproducts are present in `tests/CMakeLists.txt:39-67`; success archives/manifests exist under `tests/fixtures/generated/archives`; generator emits deterministic success set in `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp:433-439`. |
| 03-02/T-03-02 | Repudiation | fixture provenance | mitigate | CLOSED | Fixture README documents generator command and provenance at `tests/fixtures/README.md:7-21` and `57-72`; generated manifests record synthetic provenance at `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp:399-402`. |
| 03-02/T-03-03 | Information Disclosure | TES5Edit/game archives | mitigate | CLOSED | README forbids game/TES5Edit fixture sources at `tests/fixtures/README.md:47-72`; manifests state no game or TES5Edit bytes copied (`tests/fixtures/generated/archives/tes4_v103_manifest.json:13-14` et al.); final `git -C TES5Edit status --short` gate produced no output. |
| 03-03/T-03-01 | Tampering | malformed fixtures | mitigate | CLOSED | Required malformed case IDs are enumerated in `tests/fixtures/generated/validate_fixture_manifests.py:39-47` and generated in `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp:544-590`; manifest validation checks all IDs at `validate_fixture_manifests.py:86-117`. |
| 03-03/T-03-02 | Denial of Service | truncated/size-mismatch cases | mitigate | CLOSED | Malformed manifest includes `truncated_header`, `truncated_table`, and `size_mismatch` at `tests/fixtures/generated/archives/malformed_manifest.json:19-36` and `58-67`; parser/extraction tests cover malformed open and compression mismatch at `tests/unit/tes4_bsa_reader_tests.cpp:180-242` and `449-465`. |
| 03-03/T-03-03 | Tampering | duplicate canonical path case | mitigate | CLOSED | Duplicate case data appears in `tests/fixtures/generated/archives/malformed_manifest.json:37-47`; generator creates case in `generate_tes4_bsa_fixtures.cpp:442-460`; parser rejects duplicates in `src/formats/bsa/tes4_bsa_parser.cpp:357-363`; test at `tests/unit/tes4_bsa_reader_tests.cpp:310-316`. |
| 03-03/T-03-04 | Information Disclosure | fixture provenance | mitigate | CLOSED | Malformed manifest provenance states synthetic data/no game or TES5Edit bytes at `tests/fixtures/generated/archives/malformed_manifest.json:5-8`; final TES5Edit status gate produced no output. |
| 03-04/T-03-01 | Tampering | detector | mitigate | CLOSED | `src/formats/bsa/bsa_format_detector.cpp:14-40` validates `BSA\0`, maps only versions `0x67/0x68/0x69`, and rejects non-BSA/unsupported versions; tests at `tests/unit/tes4_bsa_reader_tests.cpp:141-178`. |
| 03-04/T-03-02 | Denial of Service | parser skeleton | mitigate | CLOSED | Parser uses checked reads plus `multiply_fits`/`add_fits`/`span_fits` before table reads/allocation in `src/formats/bsa/tes4_bsa_parser.cpp:64-82`, `144-164`, `433-479`, and `506-515`; oversized/malformed tests at `tests/unit/tes4_bsa_reader_tests.cpp:180-270`. |
| 03-04/T-03-03 | Information Disclosure | public boundary | mitigate | CLOSED | Detector/parser files are PRIVATE target sources in `CMakeLists.txt:51-64`; public headers file set is limited to `include/libbsa/*.hpp` at `CMakeLists.txt:42-50`; boundary test scans forbidden implementation tokens at `tests/unit/public_include_boundary_tests.cpp:45-70`. |
| 03-04/T-03-04 | Tampering | compression metadata | mitigate | CLOSED | Detector maps compression by BSA version only in `src/formats/bsa/bsa_format_detector.cpp:32-39`; metadata test verifies expected default compression in `tests/unit/tes4_bsa_reader_tests.cpp:156-169`; no extension-based truth found in open path. |
| 03-05/T-03-01 | Tampering | table parser | mitigate | CLOSED | Table/name parsing validates counts, offsets, spans, null termination, and payload bounds in `src/formats/bsa/tes4_bsa_parser.cpp:194-207`, `209-255`, `257-285`, `296-337`, and `433-479`; metadata/malformed tests passed. |
| 03-05/T-03-02 | Denial of Service | metadata vectors | mitigate | CLOSED | Count/span checks precede large vector construction in `src/formats/bsa/tes4_bsa_parser.cpp:64-82`, `144-164`, `402-430`, and `506-515`; tests mutate oversized counts at `tests/unit/tes4_bsa_reader_tests.cpp:201-242`. |
| 03-05/T-03-03 | Tampering | canonical path map | mitigate | CLOSED | `detail::normalize_archive_path` and duplicate canonical rejection are used in `src/formats/bsa/tes4_bsa_parser.cpp:357-363`; lookup reuses normalization in `src/formats/bsa/tes4_bsa_reader.cpp:110-123`; tests at `tests/unit/tes4_bsa_reader_tests.cpp:310-358`. |
| 03-05/T-03-04 | Tampering | embedded-name metadata | mitigate | CLOSED | Embedded prefix is validated/materialized during parsing in `src/formats/bsa/tes4_bsa_parser.cpp:296-311` and `365-392`; extraction reuses materialized prefix at `src/formats/bsa/tes4_bsa_reader.cpp:70-77`; tests at `tests/unit/tes4_bsa_reader_tests.cpp:273-308` and `467-486`. |
| 03-05/T-03-05 | Tampering | lookup inputs | mitigate | CLOSED | Every public lookup/contains path goes through `detail::normalize_archive_path` in `src/formats/bsa/tes4_bsa_reader.cpp:110-132`; extraction delegates through normalized lookup in `src/archive.cpp:169-180`; tests assert invalid inputs return `invalid_argument` and valid missing paths return absence/`not_found` at `tests/unit/tes4_bsa_reader_tests.cpp:318-358`, `420-431`, and `489-501`. |
| 03-06/T-03-01 | Tampering | payload cursor | mitigate | CLOSED | Extraction reads selected `payload_offset`/`stored_size` only after stream/vector-limit checks in `src/archive.cpp:75-102`; embedded-name prefix is range-checked before subspan in `src/formats/bsa/tes4_bsa_reader.cpp:70-77`; parser prevalidates entry spans at `src/formats/bsa/tes4_bsa_parser.cpp:365-378`. |
| 03-06/T-03-02 | Denial of Service | `extract_bytes` / decompression buffers | mitigate | CLOSED | `extract_bytes` reserves bounded per-entry `raw_size` in `src/archive.cpp:26-45` and `183-202`; compressed extraction compares exact-size prefix to metadata before allocation/decode in `src/formats/bsa/tes4_bsa_reader.cpp:88-101`; tests at `tests/unit/tes4_bsa_reader_tests.cpp:449-501`. |
| 03-06/T-03-03 | Tampering | compression decode | mitigate | CLOSED | Exact-size decompression is routed through `detail::decompress_payload_exact` in `src/formats/bsa/tes4_bsa_reader.cpp:92-101`; corrupt/size-mismatch malformed fixtures are generated in `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp:567-585` and tested in `tests/unit/tes4_bsa_reader_tests.cpp:449-465`. |
| 03-06/T-03-04 | Tampering / DoS | caller sink | mitigate | CLOSED | `write_all` checks every sink write result and maps short writes to `io_error` in `src/formats/bsa/tes4_bsa_reader.cpp:47-56`; chunked writes call it for each chunk at `src/formats/bsa/tes4_bsa_reader.cpp:58-68`; partial sink test at `tests/unit/tes4_bsa_reader_tests.cpp:408-418`. |
| 03-06/T-03-05 | Tampering | extraction path input | mitigate | CLOSED | Extraction calls normalized lookup and maps missing entries to `not_found` in `src/archive.cpp:169-180`; public `extract_bytes` preserves invalid/missing semantics in `src/archive.cpp:188-194`; tests at `tests/unit/tes4_bsa_reader_tests.cpp:420-431` and `489-501`. |

## Threat Flags

Executor `## Threat Flags` sections reported no unregistered Phase 03 threat flags. Plan 03-06 explicitly states: "None. The new extraction trust surfaces were already represented in the plan threat model and covered by span validation, exact-size decompression, and partial sink failure tests."

## Verification Run

- `cmake --build --preset windows-msvc-debug-static` — PASSED.
- `ctest --preset windows-msvc-debug-static --output-on-failure` — PASSED, 54 tests passed with one expected local-fixture skip (`local game fixtures are opt-in`).
- `git -C "TES5Edit" status --short` — PASSED; TES5Edit status output was empty.

## Open Threats

None.
