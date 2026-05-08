---
phase: 03-format-detection-and-tes4-family-bsa-read-extract
verified_at: 2026-05-08T09:15:00Z
verified: 2026-05-08T09:15:00Z
status: passed
verdict: passed_bounded_extraction_contract_closed
score: 6/6 must-haves verified
gaps_found: 0
tests_passed: true
tests:
  build: "cmake --build --preset windows-msvc-debug-static passed"
  ctest: "ctest --preset windows-msvc-debug-static --output-on-failure passed: 54 tests passed, one expected local-fixture skip"
  tes5edit_status: "git -C TES5Edit status --short produced no output"
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  previous_score: 5/6
  gaps_closed:
    - "Extraction is bounded and does not require loading the whole archive into memory"
  gaps_remaining: []
  regressions: []
---

# Phase 03: Format Detection and TES4-Family BSA Read/Extract Verification Report

**Phase Goal:** Consumers can open, detect, inspect, query, and extract TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE BSA archives with BSArchPro-compatible behavior.  
**Verified:** 2026-05-08T09:15:00Z  
**Status:** passed  
**Re-verification:** Yes — after bounded parsing/extraction gap closure

## Goal Achievement

Re-verification focused on the previous blocker: `archive_reader` must not retain a whole-archive byte vector, open must read only a detection prefix plus bounded metadata/payload-prefix data, and extraction must reopen/seek/read only the selected entry's stored payload. The previous gap is closed in the codebase.

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can open TES4-family BSA archives and inspect archive type, variant/family, version, flags, file count, paths, compression behavior, and per-entry metadata from archive bytes. | ✓ VERIFIED | `archive_reader::open` reads an 8-byte prefix, detects BSA magic/version, obtains file size, and delegates to `parse_tes4_bsa_archive_file()` (`src/archive.cpp:48-60`, `112-133`). The parser reads the bounded metadata table and payload prefixes needed for metadata (`src/formats/bsa/tes4_bsa_parser.cpp:588-612`). Metadata/listing tests cover archive and entry fields (`tests/unit/tes4_bsa_reader_tests.cpp:141-178`, `273-308`). |
| 2 | Consumer can check archive virtual paths and locate entries using normalized path/hash-compatible lookup semantics. | ✓ VERIFIED | Parser normalizes and rejects duplicate canonical paths (`src/formats/bsa/tes4_bsa_parser.cpp:357-363`); lookup uses `detail::normalize_archive_path` and sorted canonical metadata (`src/formats/bsa/tes4_bsa_reader.cpp:110-132`). Tests cover lookup variants, missing paths, invalid paths, and hash metadata (`tests/unit/tes4_bsa_reader_tests.cpp:318-358`). |
| 3 | Consumer can extract TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE entries whether stored raw, deflate-compressed, or LZ4-frame-compressed. | ✓ VERIFIED | Extraction reads the selected stored payload only (`src/archive.cpp:75-102`, `165-180`), strips compressed size prefix, and routes exact-size decoding through `decompress_payload_exact()` (`src/formats/bsa/tes4_bsa_reader.cpp:88-101`). Tests cover v103/v104/v105 extraction (`tests/unit/tes4_bsa_reader_tests.cpp:360-406`). |
| 4 | Consumer can extract embedded-name entries while preserving payload bytes compatible with BSArchPro output. | ✓ VERIFIED | Parser materializes embedded-name prefix sizes during open (`src/formats/bsa/tes4_bsa_parser.cpp:296-311`, `365-392`); extraction skips the materialized prefix before raw/decompressed output (`src/formats/bsa/tes4_bsa_reader.cpp:70-77`). Tests cover metadata and extracted bytes (`tests/unit/tes4_bsa_reader_tests.cpp:273-308`, `376-406`, `467-486`). |
| 5 | Maintainer can add a future archive version by extending detection/record handling without rewriting unrelated format families. | ✓ VERIFIED | Detector/parser/reader are private BSA sources; public headers remain dependency-light (`include/libbsa/archive.hpp:1-12`, `src/formats/bsa/*.cpp`). Public include boundary tests reject private dependency leakage (`tests/unit/public_include_boundary_tests.cpp:45-70`); unsupported future BSA behavior is tested (`tests/unit/tes4_bsa_reader_tests.cpp:172-178`). |
| 6 | Extraction is bounded and does not require loading the whole archive into memory. | ✓ VERIFIED | Previous blocker closed: `archive_reader::state` stores only metadata, entries, and `host_path` — no archive byte vector (`src/archive.cpp:15-19`). `archive_reader::open` reads only an 8-byte detection prefix (`src/archive.cpp:48-60`) plus bounded metadata table/payload prefixes through `parse_tes4_bsa_archive_file()` (`src/formats/bsa/tes4_bsa_parser.cpp:578-612`). `extract()` reopens the host file, seeks to the selected entry offset, reads only `entry.stored_size`, then passes that selected-entry buffer to the extractor (`src/archive.cpp:75-102`, `165-180`). The regression test deletes the archive after open and expects extraction to fail with `io_error`, proving extraction no longer uses a retained whole-archive buffer (`tests/unit/tes4_bsa_reader_tests.cpp:434-447`). |

**Score:** 6/6 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/archive.hpp` | Public metadata, entry metadata, lookup, extraction, and sink API | ✓ VERIFIED | Defines the Phase 3 reader surface and dependency-light value types (`archive_metadata`, `entry_metadata`, `payload_sink`, `metadata`, `entries`, `find`, `contains`, `extract`, `extract_bytes`). |
| `include/libbsa/result.hpp` | Stable `not_found` error category | ✓ VERIFIED | `error_code::not_found` exists and result misuse throws `std::logic_error`; covered by `tests/unit/result_tests.cpp`. |
| `src/archive.cpp` | Public facade with bounded open/extract state | ✓ VERIFIED | Reader state stores `host_path`, not archive bytes; open reads prefix + file-size and delegates bounded file parsing; extraction reopens/seeks selected payload. |
| `src/formats/bsa/bsa_format_detector.cpp` | Byte-driven BSA magic/version classifier | ✓ VERIFIED | Reads magic/version from bytes and maps `0x67`, `0x68`, `0x69`; no extension-based truth. |
| `src/formats/bsa/tes4_bsa_parser.cpp` | TES4-family table/name parser and bounded metadata materializer | ✓ VERIFIED | File parser reads fixed header, computes checked table size, reads the metadata table, and reads only payload prefixes needed for embedded/compressed metadata. |
| `src/formats/bsa/tes4_bsa_reader.cpp` | Listing, lookup, compression routing, sink writes | ✓ VERIFIED | Lookup normalization, embedded-name skipping, exact-size decompression, chunked sink writes, and short-write failure are implemented. |
| `tests/unit/tes4_bsa_reader_tests.cpp` | Fixture-backed detection, metadata, lookup, extraction, malformed, and bounded-regression tests | ✓ VERIFIED | Includes on-demand extraction regression (`434-447`) plus extraction/compression/malformed coverage. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `archive_reader::open` | BSA detector/parser | `read_detection_prefix()` → `detect_bsa_format()` → `parse_tes4_bsa_archive_file()` | ✓ WIRED | `src/archive.cpp:112-133`. |
| Parser | Public entry metadata | Materialized canonical/original paths, offsets, sizes, hashes, compression, embedded prefix | ✓ WIRED | `src/formats/bsa/tes4_bsa_parser.cpp:339-399`, `546-558`. |
| Lookup APIs | Path normalization | `detail::normalize_archive_path` | ✓ WIRED | `src/formats/bsa/tes4_bsa_reader.cpp:110-132`. |
| Extraction | Host file selected payload | `read_stored_payload(state_->host_path, entry)` | ✓ WIRED | Reopens, seeks, reads exactly selected `stored_size` at `src/archive.cpp:75-102`, called at `176-180`. |
| Extraction | Compression router | `detail::decompress_payload_exact` | ✓ WIRED | `src/formats/bsa/tes4_bsa_reader.cpp:92-101`. |
| Extraction | Public sink | `payload_sink::write` with short-write check | ✓ WIRED | `src/formats/bsa/tes4_bsa_reader.cpp:47-68`. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `archive_reader::metadata()` | `state_->metadata` | Parsed header fields from `parse_tes4_bsa_archive_file()` | Yes | ✓ FLOWING |
| `archive_reader::entries()` | `state_->entries` | Parsed table/name/payload-prefix metadata | Yes | ✓ FLOWING |
| `archive_reader::find()` / `contains()` | Canonical entry search | Parser-sorted `entry_metadata::path` values | Yes | ✓ FLOWING |
| `archive_reader::extract()` | Selected payload bytes | Host file reopened and seek/read only selected entry span | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Full build | `cmake --build --preset windows-msvc-debug-static` | Latest gate evidence: passed | ✓ PASS |
| Full test suite | `ctest --preset windows-msvc-debug-static --output-on-failure` | Latest gate evidence: 54 tests passed and one expected local-fixture skip | ✓ PASS |
| TES5Edit read-only boundary | `git -C TES5Edit status --short` | Latest gate evidence: produced no output | ✓ PASS |
| Bounded extraction regression | `tes4_bsa_extract reads selected payloads from the host archive on demand` | Test removes host archive after open and extraction fails with `io_error`, which would not happen if full bytes were retained | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| FMT-01 | 03-02, 03-03, 03-04 | Byte-driven archive detection | ✓ SATISFIED | Detector reads magic/version; tests include supported versions, unsupported future version, and non-BSA `.bsa` bytes. |
| FMT-02 | 03-01, 03-02, 03-04 | Archive metadata | ✓ SATISFIED | Public metadata and fixture assertions cover type, variant, version, flags, count, compression. |
| FMT-03 | 03-01, 03-02, 03-05 | Stable path listing | ✓ SATISFIED | Parser emits canonical/original paths and sorted entries; tests verify order and metadata. |
| FMT-04 | 03-01, 03-02, 03-05 | Normalized path existence/lookup | ✓ SATISFIED | Lookup uses `normalize_archive_path`; tests cover case/separator variants and invalid inputs. |
| FMT-05 | 03-01, 03-02, 03-05 | Per-entry metadata | ✓ SATISFIED | Tests compare sizes, offsets, hash, record flags, compression, embedded metadata. |
| FMT-06 | 03-01, 03-02, 03-03, 03-04 | Extensible private format boundary | ✓ SATISFIED | Detector/parser/reader split remains private; public boundary tests pass. |
| BSA-01 | 03-02, 03-06 | v103 read/extract | ✓ SATISFIED | Raw/deflate extraction works and now reads selected payload from host file on demand. |
| BSA-02 | 03-02, 03-06 | v104 read/extract | ✓ SATISFIED | Raw/deflate/embedded extraction works with selected-entry buffering. |
| BSA-03 | 03-02, 03-06 | v105 read/extract | ✓ SATISFIED | Raw/LZ4-frame/embedded extraction works with selected-entry buffering. |
| BSA-05 | 03-01, 03-02, 03-05 | Hash-compatible lookup | ✓ SATISFIED | Hash metadata is materialized; lookup tests verify expected records. |
| BSA-06 | 03-01, 03-02, 03-05, 03-06 | Embedded-name extraction | ✓ SATISFIED | Prefix metadata and prefix-skipping extraction are implemented and tested. |
| BSA-07 | 03-01, 03-02, 03-03, 03-06 | Raw/deflate/LZ4-frame routing and malformed failures | ✓ SATISFIED | Compression router is used; corrupt/size-mismatch fixtures fail with `format_error`. |
| BIN-03 | Phase 2 carry-forward / Phase 3 extraction contract | Extraction can stream to caller sinks without loading whole archives into memory | ✓ SATISFIED for Phase 3 scope | No whole-archive resident buffer remains; extraction reads only selected entry stored payload and writes in chunks to the sink. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| — | — | No blocker anti-patterns found | — | `archive_reader::state` no longer stores archive bytes; no `read_archive_bytes` or full-archive extraction buffer path remains under `src/`. |

### Human Verification Required

None. Optional BSArchPro/game-derived compatibility comparisons remain supplemental and are not required for this default Phase 3 gate.

### Gaps Summary

No gaps remain. The prior bounded extraction blocker is closed: opening no longer retains a whole archive byte vector, metadata parsing uses bounded file reads, and extraction reopens/seeks/reads only the selected entry's stored payload before sink/decompression handling.

---

_Verified: 2026-05-08T09:15:00Z_  
_Verifier: the agent (gsd-verifier)_
