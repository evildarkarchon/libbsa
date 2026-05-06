---
phase: 06-ba2-gnrl-read-and-extract
verified: 2026-05-06T02:54:11Z
status: gaps_found
score: 20/21 must-haves verified
overrides_applied: 0
gaps:
  - truth: "D-16: Malformed BA2 headers, records, name tables, file/name count mismatches, impossible offsets, and codec route confusion fail with structured errors."
    status: partial
    reason: "Most malformed cases are covered, but code review warnings are still observable in code: duplicate normalized names are silently collapsed, and zero-entry archives can accept out-of-range FileTableOffset metadata."
    artifacts:
      - path: "src/ba2_reader.cpp"
        issue: "Lines 220-241 skip FileTableOffset range validation for file_count == 0 because read_name_table performs no read and exact name-table validation only runs when records are non-empty."
      - path: "src/ba2_reader.cpp"
        issue: "Lines 243-265 push normalized names without duplicate detection; src/archive_view.cpp:15 uses insert_or_assign, so duplicate paths overwrite earlier entries."
      - path: "tests/ba2_reader_tests.cpp"
        issue: "No tests cover duplicate normalized BA2 names or zero-entry archives with impossible FileTableOffset."
    missing:
      - "Reject duplicate normalized BA2 paths before constructing ba2_archive."
      - "Validate file_table_offset <= source.size() independently, including file_count == 0 archives."
      - "Add generated fixture tests for both malformed cases."
---

# Phase 6: BA2 GNRL Read and Extract Verification Report

**Phase Goal:** Consumers can open, inspect, list, look up, and extract Fallout 4 and Starfield general BA2 archives.
**Verified:** 2026-05-06T02:54:11Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can open, list, inspect, and extract Fallout 4 BA2 GNRL archives across supported v1/v7/v8 variants. | ✓ VERIFIED | `tests/ba2_reader_tests.cpp` covers FO4 v1/v7/v8 metadata plus raw and deflate extraction; `ctest -R libbsa_ba2_reader_tests` passed 26/26. |
| 2 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v2 archives while preserving additional header fields in metadata where relevant. | ✓ VERIFIED | `open_ba2 lists Starfield v2 GNRL metadata` and `extract_ba2_entry writes Starfield v2 deflate bytes` passed; `src/ba2_reader.cpp` uses 32-byte v2 header size. |
| 3 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v3 archives with version-specific compression-method routing. | ✓ VERIFIED | v3 tests cover `compression_method`, method-3 LZ4 block, raw method-3, default deflate, and codec confusion; `ctest -L codec` passed 16/16. |
| 4 | Consumer can resolve BA2 entry names from length-prefixed file name tables located at `FileTableOffset`. | ✓ VERIFIED | `read_name_table` reads uint16-prefixed names at `file_table_offset`; tests cover FileTableOffset association and normalized lookup. |
| 5 | Maintainer can validate BA2 general metadata, hash, file-table, deflate, and raw LZ4 behavior against fixtures. | ✓ VERIFIED | `tests/ba2_reader_tests.cpp` contains deterministic fixture builders and 26 BA2 tests; full `ctest` passed 86/86. |
| 6 | D-01: Separate BA2 public header/API exists without adding BA2 symbols to `bsa.hpp` or a family-neutral archive API. | ✓ VERIFIED | `include/libbsa/ba2.hpp` declares BA2 APIs; `public_header_smoke.cpp` includes both headers separately; no BA2 declarations were found in `bsa.hpp` during code review scope. |
| 7 | D-02: `ba2_archive` exposes `summary()`, `paths()`, `contains()`, and `entry()` parallel to `bsa_archive`. | ✓ VERIFIED | `include/libbsa/ba2.hpp:24-34`; methods forward to `archive_view` in `src/ba2_reader.cpp:287-305`. |
| 8 | D-03: Consumer-style smoke covers `ba2.hpp`, `ba2_archive`, `open_ba2`, and `extract_ba2_entry`. | ✓ VERIFIED | `tests/public_header_smoke.cpp:4,68,75-76`; `ctest -R libbsa.public_header_smoke` passed. |
| 9 | D-04: `ba2_archive` is metadata-only and extraction requires caller-owned `byte_source`. | ✓ VERIFIED | `ba2_archive` stores only `archive_view view_`; `extract_ba2_entry(const ba2_archive&, const byte_source&, ...)` re-receives source. |
| 10 | D-05: BA2 archive metadata uses existing `archive_summary` fields without unknown/reserved public header words. | ✓ VERIFIED | Parser populates format/version/subtype/file_count/file_table_offset/compression_method only in `src/ba2_reader.cpp:268-276`; no new public BA2-only unknown fields in `archive_summary`. |
| 11 | D-06: BA2 hash components map into `entry_metadata::name_hash` and `directory_hash`. | ✓ VERIFIED | `src/ba2_reader.cpp:260-261`; metadata tests assert both hashes. |
| 12 | D-07: Size semantics distinguish unpacked size, packed size, and stored on-disk range. | ✓ VERIFIED | `stored_size_for_record()` and metadata assignments at `src/ba2_reader.cpp:164-167,253-258`; tests assert raw and compressed size semantics. |
| 13 | D-08: `entry_metadata::offset` is archive-absolute payload offset. | ✓ VERIFIED | Parser assigns BA2 record offset directly and validates against source; tests assert expected absolute offsets. |
| 14 | D-09: Metadata compression state matches extraction behavior. | ✓ VERIFIED | `compression_for_record()` resolves raw/deflate/lz4_block/unknown; extraction uses `metadata.value().compression` via `resolve_payload_codec`. |
| 15 | D-10: Starfield v3 method-3 compressed entries use raw LZ4 block and raw entries remain raw. | ✓ VERIFIED | `src/ba2_reader.cpp:104-120`; tests `CompressionMethod 3 LZ4 block` and `method 3 entries raw` pass. |
| 16 | D-11: BA2 deflate extraction uses record PackedSize/Size without BSA embedded-size prefix. | ✓ VERIFIED | Extraction comment and code at `src/ba2_reader.cpp:338-340`; deflate tests are named `without BSA prefix`. |
| 17 | D-12: Unsupported/inconsistent compression metadata fails structurally without fallback or partial writes. | ✓ VERIFIED | Unsupported v3 method maps to `compression_state::unknown`; codec-confusion test asserts failure and empty sink. |
| 18 | D-13: Generated deterministic fixtures are the Phase 6 acceptance corpus. | ✓ VERIFIED | Fixture builders live in `tests/ba2_reader_tests.cpp`; no binary BA2 fixtures or BSArchPro execution are used. |
| 19 | D-14: Fixture matrix covers FO4, Starfield v2/v3, FileTableOffset, `.dds` GNRL payload, deflate, and LZ4 block routes. | ✓ VERIFIED | Test names and helpers cover all listed routes; BA2 reader and codec CTest filters pass. |
| 20 | D-15: BA2 fixture builders remain test-local. | ✓ VERIFIED | Fixture helpers are in anonymous namespace in `tests/ba2_reader_tests.cpp`; no shared production helper was added. |
| 21 | D-16: Malformed BA2 headers, records, name tables, file/name count mismatches, impossible offsets, and codec route confusion fail with structured errors. | ✗ FAILED | Existing tests cover many cases, but review warnings remain true in code: duplicate normalized names are not rejected before `archive_view::insert_or_assign`, and zero-entry archives skip impossible `FileTableOffset` validation. |

**Score:** 20/21 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/ba2.hpp` | Public BA2 archive API | ✓ VERIFIED | Declares `ba2_archive`, `open_ba2`, `extract_ba2_entry`; no private codec/TES5Edit tokens in public headers. |
| `src/ba2_reader.cpp` | BA2 parser and extractor | ⚠️ PARTIAL | Substantive and wired, but malformed edge-case gaps remain for duplicate names and empty archive `FileTableOffset`. |
| `tests/ba2_reader_tests.cpp` | Generated BA2 fixture/parser/extractor/malformed tests | ⚠️ PARTIAL | 26 passing tests cover required nominal matrix and many malformed cases; missing tests for the two review warning cases. |
| `tests/public_header_smoke.cpp` | Consumer-style public API smoke | ✓ VERIFIED | Names `ba2_archive`, `open_ba2`, and `extract_ba2_entry`; smoke test passed. |
| `CMakeLists.txt` | Explicit BA2 source/header/test wiring | ✓ VERIFIED | Contains `src/ba2_reader.cpp`, `include/libbsa/ba2.hpp`, and `libbsa_ba2_reader_tests`; no non-comment GLOB matches. |
| `README.md` | Consumer BA2 GNRL support docs | ✓ VERIFIED | Documents Phase 06 support, compression routing, generated fixtures, and out-of-scope DX10/writer/bulk extraction boundaries. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `include/libbsa/ba2.hpp` | `archive_view` | `archive_view view_` | ✓ WIRED | `ba2_archive` owns `archive_view`; methods forward in `src/ba2_reader.cpp`. |
| `open_ba2` | BA2 parser | `parse_ba2_gnrl(source)` | ✓ WIRED | `open_ba2` calls parser directly. |
| `parse_ba2_gnrl` | `archive_view` | `success(ba2_archive{summary, entries})` | ✓ WIRED | Parsed metadata becomes copied archive view. |
| `extract_ba2_entry` | compression dispatcher | `resolve_payload_codec` + `decompress_payload` | ✓ WIRED | Extraction uses central codec routing, not direct codec calls. |
| `extract_ba2_entry` | caller sink | `sink.write` after successful decode | ✓ WIRED | Writes occur only after lookup/range/codec/decode success. |
| `tests/public_header_smoke.cpp` | `include/libbsa/ba2.hpp` | include + function pointer checks | ✓ WIRED | Public smoke test passed. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `src/ba2_reader.cpp` | `archive_summary`, `entry_metadata` | Bounded reads from caller `byte_source` header, records, and FileTableOffset name table | Yes | ✓ FLOWING |
| `src/ba2_reader.cpp` | extraction payload bytes | `metadata.offset` + `metadata.stored_size` read from caller `byte_source`, decoded through dispatcher, written to `byte_sink` | Yes | ✓ FLOWING |
| `tests/ba2_reader_tests.cpp` | fixture archives | Test-local deterministic byte builders | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| BA2 reader fixture tests | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_ba2_reader_tests` | 26/26 tests passed | ✓ PASS |
| Codec and Starfield route tests | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -L codec` | 16/16 tests passed | ✓ PASS |
| Public header smoke | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug -R libbsa.public_header_smoke` | 1/1 test passed | ✓ PASS |
| Full suite | `ctest --test-dir build/local-vs2026-vcpkg --output-on-failure -C Debug` | 86/86 tests passed | ✓ PASS |
| Public header boundary | `rg -n "libdeflate|lz4\.h|lz4frame\.h|LZ4|DirectXTex|TES5Edit" include/libbsa` | no output | ✓ PASS |
| CMake explicit wiring | `rg -n "^[^#]*\b(GLOB|GLOB_RECURSE)\b" CMakeLists.txt` | no output | ✓ PASS |
| TES5Edit read-only boundary | `git status --short TES5Edit` | no output | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| BA2-01 | Plans 01, 02, 03, 05, 06 | Consumer can open, list, inspect, and extract Fallout 4 BA2 GNRL archives across supported v1/v7/v8 variants. | ✓ SATISFIED | FO4 v1/v7/v8 metadata, raw extraction, and deflate extraction tests passed. |
| BA2-02 | Plans 01, 02, 03, 05, 06 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v2 archives with additional header fields preserved where relevant. | ✓ SATISFIED | Starfield v2 metadata and deflate extraction tests passed; parser accounts for 32-byte header. |
| BA2-03 | Plans 01, 04, 05, 06 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v3 archives with correct compression-method routing. | ✓ SATISFIED | Starfield v3 compression method, default deflate, raw method-3, LZ4-block method-3, and codec confusion tests passed. |
| BA2-04 | Plans 01, 02, 05, 06 | Consumer can parse BA2 file name tables at `FileTableOffset` and associate length-prefixed names with entries. | ⚠️ PARTIAL | Nominal and mismatch tests pass, but duplicate normalized names can still collapse entries through `archive_view::insert_or_assign`, leaving file-count/name association inconsistent for malformed inputs. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/ba2_reader.cpp` | 220-241 | Missing `file_table_offset <= source.size()` validation for zero-entry archives | 🛑 Blocker | Violates D-16 impossible offset failure; opens malformed metadata. |
| `src/ba2_reader.cpp` + `src/archive_view.cpp` | 243-265 + 15 | Duplicate normalized names not rejected; `insert_or_assign` overwrites | 🛑 Blocker | Can silently drop an entry and make `summary.file_count` inconsistent with listed/lookup metadata. |

### Human Verification Required

None. This phase is library/parser/test behavior and was verifiable through code inspection and local commands.

### Gaps Summary

The main BA2 GNRL goal is substantially implemented: public API, parser, metadata inspection, listing/lookup, FO4/Starfield extraction, codec routing, tests, docs, public-header boundary, CMake wiring, and TES5Edit boundary all exist and are wired. However, the code review warnings are not merely advisory; they expose observable malformed-input gaps against D-16 and partially affect BA2-04 name association consistency. Because D-16 is a must-have, the phase cannot be marked passed until these malformed cases are rejected and tested.

---

_Verified: 2026-05-06T02:54:11Z_
_Verifier: the agent (gsd-verifier)_
