---
phase: 05-ba2-gnrl-read-extract
verified: 2026-05-08T23:15:05Z
status: gaps_found
score: 12/13 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Consumers can open, inspect, and list required BA2 GNRL archives without reading the payload region into open-time memory."
    status: failed
    reason: "BA2 open reads from FileTableOffset through end-of-file and appends those bytes to metadata_bytes before parsing names, so valid large BA2 archives can require whole-payload allocation just to open/list. This violates the phase/project bounded-reader constraint despite tiny generated fixtures passing."
    artifacts:
      - path: "src/formats/ba2/ba2_gnrl_parser.cpp"
        issue: "Lines 356-366 read archive_size - FileTableOffset bytes into name_and_payload_bytes and append them to metadata_bytes; this includes payload bytes, not only the filename table."
    missing:
      - "Bound the filename-table read to the interval [FileTableOffset, first_payload_offset) computed from parsed GNRL records, then parse names from only that bounded slice."
      - "Add a regression test proving BA2 open/list does not require retaining or reading payload bytes beyond the filename table."
deferred: []
human_verification: []
---

# Phase 5: BA2 GNRL Read/Extract Verification Report

**Phase Goal:** Consumers can open, inspect, list, query, and extract Fallout 4 BA2 GNRL, Starfield BA2 v2 GNRL, and structurally valid Starfield BA2 v3 GNRL archives through the existing `archive_reader` surface with correct raw, deflate, and Starfield raw-LZ4-block compression routing.  
**Verified:** 2026-05-08T23:15:05Z  
**Status:** gaps_found  
**Re-verification:** No — initial verification

## Goal Achievement

Most BA2 GNRL functional behavior is implemented and tested: byte-driven `BTDX` detection, public BA2 metadata, GNRL record/name parsing, deterministic list/query behavior, raw/deflate/raw-LZ4-block extraction, malformed error-code coverage, BSA regression tests, and TES5Edit cleanliness all verified against actual code and tests.

However, the phase is not fully achieved because `archive_reader::open` for BA2 GNRL currently reads the whole suffix from `FileTableOffset` to EOF into memory before parsing the filename table. For real Fallout 4/Starfield BA2 archives, that suffix includes payload data. This is an observable implementation gap against the bounded-reader constraint and can make valid large archives fail to open/list.

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can open Fallout 4 BA2 GNRL, Starfield BA2 v2 GNRL, and structurally valid Starfield BA2 v3 GNRL archives by bytes, not extension. | ✓ VERIFIED | `archive_reader::open` checks the first four bytes for `BTDX` before BSA fallback and calls `formats::ba2::detect_ba2_format` then `parse_ba2_gnrl_archive_file` (`src/archive.cpp:122-143`). Detector parses `BTDX`, version, subtype, and rejects DX10/unsupported methods (`src/formats/ba2/ba2_format_detector.cpp:35-115`). BA2 detector CTest label passed. |
| 2 | BA2 archive metadata reports type, variant, version, file count, default compression, and BA2 optionals. | ✓ VERIFIED | Public `ba2_archive_metadata` and `archive_metadata::ba2` exist (`include/libbsa/archive.hpp:44-74`). Parser materializes `archive_type::ba2`, variant, version, file count, default compression, and parsed BA2 fields (`src/formats/ba2/ba2_gnrl_parser.cpp:304-310`). Tests assert FO4/SFv2/SFv3 metadata (`tests/unit/ba2_gnrl_reader_tests.cpp:131-174`). |
| 3 | BA2 names are parsed from the `FileTableOffset` UInt16 length-prefixed filename table and listed deterministically. | ✓ VERIFIED | Parser reads UInt16 length-prefixed names (`src/formats/ba2/ba2_gnrl_parser.cpp:171-191`), normalizes original separators (`lines 112-114, 212-214`), sorts entries by canonical path (`lines 246-249`), and tests compare sorted manifest paths (`tests/unit/ba2_gnrl_reader_tests.cpp:193-241`). |
| 4 | `entries`, `find`, and `contains` work for BA2 GNRL paths with normalized archive virtual path semantics. | ✓ VERIFIED | `archive_reader` dispatches BA2 entries/find/contains by `metadata.type == archive_type::ba2` (`src/archive.cpp:185-221`). BA2 lookup normalizes caller paths and lower-bounds sorted entries (`src/formats/ba2/ba2_gnrl_reader.cpp:168-190`). Tests cover lookup variants, missing valid paths, and invalid caller paths (`tests/unit/ba2_gnrl_reader_tests.cpp:243-284`). |
| 5 | BA2 entry metadata reports raw size, stored size, archive-absolute offset, hash, record flags, compression, and no embedded-name state. | ✓ VERIFIED | Parser materializes `entry_metadata` from GNRL records and filename table (`src/formats/ba2/ba2_gnrl_parser.cpp:222-239`). Tests compare `raw_size`, `stored_size`, `payload_offset`, `archive_hash`, `record_flags`, compression, and embedded-name fields against manifests (`tests/unit/ba2_gnrl_reader_tests.cpp:225-234`). |
| 6 | Raw-vs-compressed classification uses `PackedSize` and parsed archive metadata, not filename or extension. | ✓ VERIFIED | Parser skips the BA2 record extension field and uses `PackedSize == 0` for raw otherwise `detected.default_compression` (`src/formats/ba2/ba2_gnrl_parser.cpp:151-159, 194-199`). Grep found no BA2 codec routing by `.dds`, `.mesh`, path, or extension. |
| 7 | Starfield v2/v3 header fields are inspectable through dependency-light public metadata. | ✓ VERIFIED | Header reads `Unknown1`, `Unknown2`, and v3 `CompressionMethod` (`src/formats/ba2/ba2_gnrl_parser.cpp:126-142`); public metadata uses `std::optional<std::uint32_t>` fields and no libdeflate/lz4/DirectXTex/TES5Edit public includes (`include/libbsa/archive.hpp:44-74`; public boundary grep found no forbidden tokens). |
| 8 | Raw BA2 GNRL entries extract byte-for-byte through `extract(path, sink)` and `extract_bytes(path)`. | ✓ VERIFIED | BA2 extraction dispatch occurs before generic BSA payload buffering (`src/archive.cpp:239-244`). Raw entries stream from host file in 64 KiB chunks and detect partial sink writes (`src/formats/ba2/ba2_gnrl_reader.cpp:57-94`). Tests compare both public extraction APIs to manifest bytes including zero-byte raw entries (`tests/unit/ba2_gnrl_reader_tests.cpp:286-322`). |
| 9 | Deflate BA2 GNRL entries extract with exact-size validation. | ✓ VERIFIED | Compressed extraction maps `entry_compression::deflate` to `detail::compression_method::deflate` and calls `detail::decompress_payload_exact` with `entry.raw_size` (`src/formats/ba2/ba2_gnrl_reader.cpp:126-159`). Tests include deflate entries and malformed corrupt/exact-size mismatch cases (`tests/unit/ba2_gnrl_reader_tests.cpp:286-322, 359-398`). |
| 10 | Starfield v3 `CompressionMethod == 3` entries extract through raw LZ4 block, not LZ4 frame. | ✓ VERIFIED | Detector maps v3 method 3 to `entry_compression::lz4_block` (`src/formats/ba2/ba2_format_detector.cpp:100-109`); reader maps that to `detail::compression_method::lz4_block` and explicitly fails `lz4_frame` for BA2 GNRL (`src/formats/ba2/ba2_gnrl_reader.cpp:126-136`). SFv3 test checks the LZ4-block entry extraction (`tests/unit/ba2_gnrl_reader_tests.cpp:340-356`). |
| 11 | Malformed supported BA2 GNRL bytes fail closed and unsupported valid BA2 profiles fail with stable `unsupported`. | ✓ VERIFIED | Detector rejects DX10 and unsupported v3 method as `unsupported` (`src/formats/ba2/ba2_format_detector.cpp:69-70, 110-113`). Parser checks headers, records, sentinel, `FileTableOffset`, filename table, duplicate canonical paths, and payload spans (`src/formats/ba2/ba2_gnrl_parser.cpp:147-168, 263-299`). Manifest-driven malformed tests passed (`tests/unit/ba2_gnrl_reader_tests.cpp:359-398`). |
| 12 | Generated legal fixtures/manifests cover required variants, metadata, lookup variants, compression routes, expected bytes, and malformed cases outside `TES5Edit/`. | ✓ VERIFIED | Generator creates FO4/SFv2/SFv3 success fixtures and malformed cases from synthetic strings with provenance (`tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp:270-303, 345-420, 484-495`). Manifests contain paths, lookup variants, sizes, offsets, hashes, Starfield fields, compression routes, and expected bytes. CMake target is wired (`tests/CMakeLists.txt:73-146`). |
| 13 | Consumers can open, inspect, and list required BA2 GNRL archives without reading the payload region into open-time memory. | ✗ FAILED | `parse_ba2_gnrl_archive_file` reads `archive_size - FileTableOffset` bytes from the filename table offset and appends them to `metadata_bytes` before parsing names (`src/formats/ba2/ba2_gnrl_parser.cpp:356-366`). Because BA2 payloads follow the filename table in supported fixtures, this reads payload bytes during open/list and can allocate multi-GB buffers for valid archives. |

**Score:** 12/13 truths verified

### Deferred Items

None. Phase 12 has broad large-archive performance goals, but this specific gap is in the Phase 5 BA2 open/list implementation path and blocks reliable use of the claimed reader surface for real BA2 GNRL archives.

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` | Synthetic BA2 GNRL fixture generator | ✓ VERIFIED | `gsd-sdk verify.artifacts` passed; source generates BTDX/GNRL success fixtures, malformed fixtures, deflate and LZ4-block payloads, manifests, and provenance. |
| `tests/CMakeLists.txt` | BA2 fixture generator/test wiring | ✓ VERIFIED | Generator executable/custom target and BA2 test source are registered. |
| `include/libbsa/archive.hpp` | Public BA2 metadata surface | ✓ VERIFIED | Defines `ba2_archive_metadata` optionals and `archive_metadata::ba2` without private dependency leakage. |
| `src/formats/ba2/ba2_format_detector.cpp` | Byte-driven BA2 detector | ✓ VERIFIED | Substantive detector routes FO4/SFv2/SFv3 GNRL and rejects DX10/unsupported versions/methods. |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | Checked BA2 GNRL header/record/name parser | ⚠️ PARTIAL | Parser is substantive and wired, but open-time file parser reads the payload region into memory while constructing name metadata. |
| `src/formats/ba2/ba2_gnrl_reader.cpp` | Listing/lookup/extraction helpers | ✓ VERIFIED | Entry copies, normalized lookup, raw streaming, exact-size deflate/LZ4-block decode, and partial sink checks are implemented. |
| `src/archive.cpp` | `archive_reader` BA2 dispatch | ✓ VERIFIED | BA2 open/list/find/contains/extract dispatch is wired by bytes/type; extraction dispatch occurs before generic BSA buffering. |
| `tests/unit/ba2_gnrl_reader_tests.cpp` | Manifest-backed BA2 tests | ✓ VERIFIED | Covers detector, metadata, lookup, extraction, malformed/unsupported behavior, and direct helper partial-sink/LZ4 checks. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `tests/CMakeLists.txt` | `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` | Generator executable/custom target | ✓ WIRED | `gsd-sdk verify.key-links` passed for Plan 05-01. |
| `src/archive.cpp` | `src/formats/ba2/ba2_format_detector.cpp` | `detect_ba2_format` before BSA fallback | ✓ WIRED | `src/archive.cpp:122-146`; `gsd-sdk verify.key-links` passed for Plan 05-02. |
| `src/archive.cpp` | `src/formats/ba2/ba2_gnrl_parser.cpp` | `parse_ba2_gnrl_archive_file` on BA2 open | ✓ WIRED | `src/archive.cpp:131-143`; `gsd-sdk verify.key-links` passed for Plan 05-03. |
| `src/archive.cpp` | `src/formats/ba2/ba2_gnrl_reader.cpp` | BA2 entries/find/contains dispatch | ✓ WIRED | `src/archive.cpp:192-220`; helper names present and tests passed. |
| `src/archive.cpp` | `src/formats/ba2/ba2_gnrl_reader.cpp` | BA2 extraction dispatch | ✓ WIRED | Manual check: `archive_type::ba2` branch calls `extract_ba2_gnrl_payload` at `src/archive.cpp:242-243`. The SDK pattern check missed this due multiline formatting. |
| `src/formats/ba2/ba2_gnrl_reader.cpp` | `src/detail/compression_router.cpp` | `detail::decompress_payload_exact` | ✓ WIRED | `src/formats/ba2/ba2_gnrl_reader.cpp:155`; `gsd-sdk verify.key-links` passed this link. |
| malformed manifest | BA2 malformed tests | `expected_error` manifest-driven assertions | ✓ WIRED | `tests/unit/ba2_gnrl_reader_tests.cpp:359-398`; `gsd-sdk verify.key-links` passed for Plan 05-05. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `archive_reader::metadata()` | `state_->metadata` | BA2 detector + parser header fields | Yes | ✓ FLOWING |
| `archive_reader::entries()` | `state_->entries` | BA2 parser reads records and filename table | Yes, but open reads payload suffix too | ⚠️ FLOWING_WITH_BUFFERING_GAP |
| `archive_reader::find()` / `contains()` | Canonical path search | Parser-sorted `entry_metadata::path` values | Yes | ✓ FLOWING |
| `archive_reader::extract()` raw | Selected host payload span | BA2 helper reopens archive, seeks `payload_offset`, streams 64 KiB chunks | Yes | ✓ FLOWING |
| `archive_reader::extract()` compressed | Stored bytes → exact-size decoded bytes | BA2 helper reads selected stored payload then calls compression router | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused BA2 + public boundary tests | `ctest --preset windows-msvc-debug-static -L "ba2_gnrl|public_include_boundary" --output-on-failure` | 9/9 tests passed | ✓ PASS |
| TES3/TES4/public regression tests | `ctest --preset windows-msvc-debug-static -L "tes3_bsa|tes4_bsa|public-api" --output-on-failure` | 40/40 tests passed | ✓ PASS |
| Full regression gate | Orchestrator evidence: `ctest --preset windows-msvc-debug-static --output-on-failure` | 75/75 tests passed with expected local fixture skip | ✓ PASS |
| TES5Edit read-only boundary | `git -C "TES5Edit" status --short` | No output | ✓ PASS |
| Repo cleanliness | `git status --short` | No output before verification file creation | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| GNRL-01 | 05-01, 05-02, 05-05 | Consumer can read and extract Fallout 4 BA2 GNRL archives. | ⚠️ PARTIAL | Generated FO4 fixture opens/lists/extracts and tests pass. Gap: BA2 open reads payload-region bytes, so real large FO4 GNRL archives may fail to open/list due unbounded allocation. |
| GNRL-02 | 05-01, 05-02, 05-05 | Consumer can read and extract Starfield BA2 v2 GNRL archives. | ⚠️ PARTIAL | Generated SFv2 fixture opens/lists/extracts and tests pass. Same open-time payload-region read gap affects real large SFv2 archives. |
| GNRL-03 | 05-01, 05-02, 05-05 | Consumer can read and extract structurally valid Starfield BA2 v3 GNRL archives. | ⚠️ PARTIAL | Generated SFv3 method-3 fixture opens/lists/extracts via raw LZ4 block. Same open-time payload-region read gap affects real large SFv3 archives. |
| GNRL-04 | 05-01, 05-03, 05-05 | Consumer can parse BA2 filename tables at `FileTableOffset` with length-prefixed names. | ⚠️ PARTIAL | Names are parsed correctly from `FileTableOffset`, but the implementation reads from `FileTableOffset` to EOF instead of bounding the filename-table span. |
| GNRL-05 | 05-01, 05-03, 05-04, 05-05 | Consumer can distinguish raw BA2 entries from compressed entries using `PackedSize` and format metadata. | ✓ SATISFIED | `PackedSize == 0` → raw, nonzero → detector-selected deflate/LZ4-block; manifest tests cover raw, deflate, and lz4_block metadata. |
| GNRL-06 | 05-01, 05-04, 05-05 | Consumer can extract BA2 GNRL entries compressed with deflate. | ✓ SATISFIED | Deflate entries are decoded via `detail::decompress_payload_exact(... deflate ...)`; success and corrupt/size-mismatch tests pass. |
| GNRL-07 | 05-01, 05-04, 05-05 | Consumer can extract Starfield BA2 v3 GNRL entries compressed with raw LZ4 block when `CompressionMethod == 3`. | ✓ SATISFIED | Detector maps method 3 to `entry_compression::lz4_block`; extraction routes to raw LZ4 block and SFv3 test compares manifest bytes. |
| GNRL-08 | 05-01, 05-02, 05-03, 05-05 | Consumer can inspect and preserve Starfield BA2 v2/v3 version-specific header fields in metadata. | ✓ SATISFIED | Public metadata exposes version-gated `starfield_unknown1`, `starfield_unknown2`, and `compression_method`; tests assert manifest values. |

No orphaned Phase 5 requirements were found: GNRL-01 through GNRL-08 are declared in Phase 5 plan frontmatter and mapped to Phase 5 in `.planning/REQUIREMENTS.md`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/formats/ba2/ba2_gnrl_parser.cpp` | 358-365 | Reads `archive_size - FileTableOffset` into `name_and_payload_bytes` and appends to metadata buffer | 🛑 Blocker | Opening/listing a BA2 GNRL archive reads payload bytes into memory before extraction, undermining reliable support for large real FO4/Starfield archives. |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | 266-268 | Only validates parsed version/file_count against detected prefix | ⚠️ Warning | Review WR-02: stale/inconsistent detected Starfield metadata could misroute compression. Not a public-path blocker because detection and parse read the same file in normal `archive_reader::open`, but should be hardened. |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | 201-224 | Narrows record offset to `std::size_t` before `span_fits` | ⚠️ Warning | Review WR-03: malformed 64-bit offsets can be misvalidated on 32-bit builds. Primary MSVC 64-bit tests pass, but portable parser hardening should use `uint64_t` span checks. |

### Human Verification Required

None. The blocking gap and review warnings are observable in code; generated-fixture behavior is covered by automated tests.

### Gaps Summary

Phase 5 successfully implements the main BA2 GNRL read/list/query/extract surface for generated FO4, Starfield v2, and Starfield v3 fixtures and passes focused plus regression tests. The remaining blocker is in BA2 open/list scalability and bounded I/O: `parse_ba2_gnrl_archive_file` reads the entire filename-table-to-EOF suffix, which includes payload bytes, before parsing names. Fixing that requires bounding filename-table reads by the first payload offset and adding a regression proving open/list does not consume payload data.

---

_Verified: 2026-05-08T23:15:05Z_  
_Verifier: the agent (gsd-verifier)_
