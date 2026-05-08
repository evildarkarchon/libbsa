---
phase: 04-tes3-bsa-read-extract
verified: 2026-05-08T12:31:22Z
status: gaps_found
score: 10/12 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Maintainer can validate TES3 listing and extraction behavior with focused fixtures that isolate TES3 hash and offset rules."
    status: failed
    reason: "The claimed tes3_hash_collision malformed fixture overwrites a stored hash while leaving the entry name unchanged, so parser validation fails at stored-hash mismatch before reaching the duplicate/collision branch. The fixture does not isolate collision behavior."
    artifacts:
      - path: "tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp"
        issue: "Lines 321-323 build tes3_hash_collision by copying entries.front().archive_hash into the second hash record, which is caught by src/formats/bsa/tes3_bsa_parser.cpp:218 before duplicate-hash detection at line 226."
      - path: "tests/unit/tes3_bsa_reader_tests.cpp"
        issue: "Lines 316-346 assert only that opening each malformed archive returns the manifest error code, not that the collision-specific duplicate branch is exercised."
    missing:
      - "Replace or supplement tes3_hash_collision with a fixture/test that reaches duplicate stored-hash collision validation rather than the earlier hash-mismatch branch."
  - truth: "TES3 sink extraction streams bounded host-file chunks instead of pre-buffering the selected payload before calling the sink."
    status: failed
    reason: "archive_reader::extract reads the whole stored payload into a std::vector before TES3 variant dispatch, so the TES3 helper receives an already-buffered span instead of streaming from payload_offset/stored_size in bounded chunks."
    artifacts:
      - path: "src/archive.cpp"
        issue: "Lines 77-104 define read_stored_payload with std::vector<std::byte> payload(entry.stored_size); lines 201-207 call it before dispatching to extract_tes3_bsa_payload."
      - path: "src/formats/bsa/tes3_bsa_reader.hpp"
        issue: "extract_tes3_bsa_payload takes std::span<const std::byte> stored_payload, so it cannot own host-file seek/read streaming."
    missing:
      - "Dispatch TES3 extraction before read_stored_payload and have the TES3 helper seek to entry.payload_offset and read/write at most extraction_chunk_size bytes per pass."
deferred: []
human_verification: []
---

# Phase 4: TES3 BSA Read/Extract Verification Report

**Phase Goal:** Consumers can read, list, query, and extract TES3/Morrowind BSA archives while preserving TES3-specific data-section-relative offset behavior.  
**Verified:** 2026-05-08T12:31:22Z  
**Status:** gaps_found  
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can open a TES3/Morrowind BSA and list/query entries through the public `archive_reader` API. | ✓ VERIFIED | `archive_reader::open` dispatches TES3 to `parse_tes3_bsa_archive_file` (`src/archive.cpp:128-137`); `entries/find/contains` dispatch by TES3 variant (`src/archive.cpp:162-184`); focused CTest TES3 metadata/entries/lookup tests passed. |
| 2 | Consumer can extract TES3 entries using data-section-relative offsets converted to archive-absolute public offsets. | ✓ VERIFIED | Parser computes `absolute_payload_offset = data_section_start + raw_offset` and stores only `entry_metadata::payload_offset` (`src/formats/bsa/tes3_bsa_parser.cpp:240-267`); extraction tests compare archive bytes at manifest `payload_offset` (`tests/unit/tes3_bsa_reader_tests.cpp:265-287`). |
| 3 | Maintainer can validate TES3 behavior with focused fixtures that isolate TES3 hash and offset rules. | ✗ FAILED | Offset fixtures are present, but the claimed collision fixture does not isolate collision behavior: generator lines 321-323 create a stored-hash mismatch that parser rejects at line 218 before duplicate detection at line 226. |
| 4 | Generated TES3 fixtures are legal, repository-owned, and include raw-vs-absolute offsets plus stored hash halves. | ✓ VERIFIED | Generator and manifest include synthetic provenance, `raw_tes3_data_offset`, `payload_offset`, `archive_hash`, `hash_low32`, and `hash_high32` (`tes3_success_manifest.json:6-9,15-21,33-39`). |
| 5 | Public metadata is format-neutral and documents archive-absolute payload offsets. | ✓ VERIFIED | `entry_metadata` exposes `archive_hash` and documents `payload_offset` as archive-absolute for every variant (`include/libbsa/archive.hpp:58-75`). |
| 6 | TES3 magic/version `0x00000100` is detected as `archive_variant::tes3`; unrelated bytes are unsupported. | ✓ VERIFIED | Detector checks little-endian `0x00000100` and returns TES3/no compression (`src/formats/bsa/bsa_format_detector.cpp:22-29`); detector tests passed. |
| 7 | TES3 parser validates hashes from parsed archive names, rejects mismatches, duplicate stored hashes, unsorted hash records, invalid paths, and duplicate canonical paths. | ✓ VERIFIED | Validation uses `hash_tes3(names[index])`, `tes3_hash_sort_key`, duplicate hash/path sets, and path normalization (`src/formats/bsa/tes3_bsa_parser.cpp:215-238`); malformed tests passed, with the collision-isolation caveat in truth #3. |
| 8 | TES3 listing is deterministic and preserves original spellings while supporting normalized path lookup. | ✓ VERIFIED | Entries are sorted by canonical path (`src/formats/bsa/tes3_bsa_parser.cpp:270-272`), original separators are normalized for display (`lines 230-231`), and lookup uses `normalize_archive_path` + binary search (`src/formats/bsa/tes3_bsa_reader.cpp:43-58`). |
| 9 | Malformed TES3 spans, overlaps, counts, names, duplicates, and hash/order cases fail closed with stable errors. | ✓ VERIFIED | Parser rejects invalid table/name/payload spans and overlaps (`src/formats/bsa/tes3_bsa_parser.cpp:91-120,163-188,240-256`); `ctest -L tes3_bsa_malformed` passed. |
| 10 | TES3 support does not invoke compression codecs and does not retain whole archive bytes in reader state. | ✓ VERIFIED | Reader state stores metadata, entry vector, and host path only (`src/archive.cpp:17-21`); TES3 helper rejects non-`entry_compression::none` and `tes3_bsa_reader.cpp` has no codec calls (`src/formats/bsa/tes3_bsa_reader.cpp:68-76`). |
| 11 | TES3 extraction is sink-first/bounded from host file. | ✗ FAILED | `archive_reader::extract` calls `read_stored_payload` before TES3 dispatch (`src/archive.cpp:201-207`), and `read_stored_payload` allocates `std::vector<std::byte> payload(entry.stored_size)` (`src/archive.cpp:92`). |
| 12 | TES5Edit remains a read-only reference and public headers do not expose implementation dependencies. | ✓ VERIFIED | `git -C TES5Edit status --short` produced no output; public `archive.hpp` includes only standard library and `libbsa/result.hpp`. |

**Score:** 10/12 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `src/formats/bsa/tes3_bsa_parser.cpp` | TES3 header/table/name/hash parser and strict validation | ✓ VERIFIED | Substantive parser exists; wired from `archive_reader::open`; converts data-section-relative offsets to archive-absolute metadata. |
| `src/formats/bsa/tes3_bsa_reader.cpp` | TES3 entries/find/contains and raw extraction helper | ⚠️ PARTIAL | Listing/lookup/raw-only helper are implemented and wired, but helper receives an already-buffered span, so the sink-first streaming contract is incomplete. |
| `src/archive.cpp` | Variant-aware open/list/query/extract dispatch | ⚠️ PARTIAL | Open/list/query dispatch is correct; extract dispatch happens after whole selected payload is buffered. |
| `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` | Success and malformed TES3 fixture generator | ⚠️ PARTIAL | Generates success/malformed fixtures, but `tes3_hash_collision` does not exercise collision-specific validation. |
| `tests/unit/tes3_bsa_reader_tests.cpp` | Public API tests for TES3 metadata/listing/lookup/extract/malformed behavior | ⚠️ PARTIAL | Tests pass and cover public behavior, but collision test only checks generic `format_error`, not collision branch reachability. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `src/archive.cpp` | `src/formats/bsa/tes3_bsa_parser.cpp` | TES3 open dispatch | ✓ WIRED | `archive_reader::open` calls `parse_tes3_bsa_archive_file` when detector reports TES3 (`src/archive.cpp:128-137`). |
| `src/archive.cpp` | `src/formats/bsa/tes3_bsa_reader.cpp` | entries/find/contains dispatch | ✓ WIRED | TES3 variant dispatch present for entries/find/contains (`src/archive.cpp:162-184`). |
| `src/archive.cpp` | `src/formats/bsa/tes3_bsa_reader.cpp` | extraction dispatch | ⚠️ PARTIAL | `extract_tes3_bsa_payload` is called, but only after `read_stored_payload` pre-buffers the whole selected entry (`src/archive.cpp:201-207`). |
| `src/formats/bsa/tes3_bsa_parser.cpp` | `src/detail/bethesda_hash.cpp` | stored hash validation from parsed names | ✓ WIRED | Parser calls `detail::hash_tes3(names[index])` (`src/formats/bsa/tes3_bsa_parser.cpp:216-220`). |
| `tests/CMakeLists.txt` | `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` | fixture generator target | ✓ WIRED | `generate_tes3_bsa_fixtures_tool` and custom target are registered (`tests/CMakeLists.txt:56-88`). |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `archive_reader::open` | `state.entries` | `parse_tes3_bsa_archive_file` reads TES3 metadata table from host archive | Yes | ✓ FLOWING |
| `archive_reader::entries/find/contains` | `state.entries` | Parser-materialized vector sorted by canonical path | Yes | ✓ FLOWING |
| `archive_reader::extract` | `payload` | `read_stored_payload` seeks `entry.payload_offset` and reads `entry.stored_size` | Yes, but whole selected payload is buffered before sink | ⚠️ FLOWING_WITH_BUFFERING_GAP |
| `tes3_hash_collision` malformed fixture | Stored hash record | Generator overwrites second hash with first entry hash | No collision branch proof; parser rejects mismatch first | ✗ HOLLOW_FIXTURE |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| TES3 metadata/listing/lookup/extract/malformed tests pass | `ctest --preset windows-msvc-debug-static -L "tes3_bsa_metadata|tes3_bsa_entries|tes3_bsa_lookup|tes3_bsa_extract|tes3_bsa_malformed" --output-on-failure` | 8/8 tests passed | ✓ PASS |
| TES4/hash/public-boundary regressions remain green | `ctest --preset windows-msvc-debug-static -L "bethesda_hash|public_include_boundary|tes4_bsa" --output-on-failure` | 21/21 tests passed | ✓ PASS |
| TES5Edit read-only boundary | `git -C "TES5Edit" status --short` | No output | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| BSA-04 | 04-01, 04-02, 04-03, 04-04 | Consumer can read and extract files from TES3/Morrowind BSA archives. | ⚠️ PARTIAL | Public open/list/query/extract behavior works for generated fixtures and tests pass. However, sink extraction pre-buffers the whole selected payload before writing to the sink, violating the Phase 04 sink-first extraction contract. |
| BSA-08 | 04-01, 04-02, 04-03, 04-04 | Consumer can extract TES3 entries using data-section-relative offset semantics. | ✓ SATISFIED | Parser converts `data_section_start + raw_offset` to archive-absolute `payload_offset`; tests verify manifest raw offsets and archive bytes at public offsets. |

No orphaned Phase 4 requirements were found in `.planning/REQUIREMENTS.md`; BSA-04 and BSA-08 are both declared in all Phase 4 plan frontmatter and mapped to Phase 4 in the requirements traceability table.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/archive.cpp` | 92 | `std::vector<std::byte> payload(entry.stored_size)` before TES3 dispatch | 🛑 Blocker | TES3 sink extraction is not truly sink-first/bounded; large selected entries can allocate the whole payload before the caller sink sees data. |
| `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` | 321-323 | Claimed collision fixture only overwrites hash | 🛑 Blocker | Fixture fails at stored-hash mismatch and does not validate duplicate/collision branch. |

### Human Verification Required

None. The remaining gaps are observable in code and test-fixture construction.

### Gaps Summary

Phase 4 implements substantial TES3 read/list/query/extract behavior and passes focused automated tests, but two must-have verification gaps remain. First, the malformed hash-collision fixture does not actually isolate the collision branch it claims to cover. Second, TES3 public sink extraction is wired through a helper only after `archive_reader::extract` pre-buffers the selected payload into memory, so the sink-first/bounded extraction contract is not achieved.

---

_Verified: 2026-05-08T12:31:22Z_  
_Verifier: the agent (gsd-verifier)_
