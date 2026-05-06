---
phase: 06-ba2-gnrl-read-and-extract
verified: 2026-05-06T04:03:05Z
status: passed
score: 21/21 must-haves verified
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  previous_score: 20/21
  gaps_closed:
    - "D-16/D-17: Duplicate BA2 names that normalize to the same archive path are rejected before ba2_archive/archive_view construction."
    - "D-16/D-18: FileTableOffset is range-validated against source.size() even when file_count == 0."
    - "D-19: Generated fixture tests cover duplicate normalized names, impossible empty-archive FileTableOffset, and valid empty archives."
  gaps_remaining: []
  regressions: []
---

# Phase 6: BA2 GNRL Read and Extract Verification Report

**Phase Goal:** Consumers can open, inspect, list, look up, and extract Fallout 4 and Starfield general BA2 archives.
**Verified:** 2026-05-06T04:03:05Z
**Status:** passed
**Re-verification:** Yes — after gap closure plan 06-07

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can open, list, inspect, and extract Fallout 4 BA2 GNRL archives across supported v1/v7/v8 variants. | ✓ VERIFIED | Regression sanity: `tests/ba2_reader_tests.cpp` still covers FO4 v1/v7/v8 metadata plus raw and deflate extraction; focused BA2 reader CTest passed 29/29. |
| 2 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v2 archives while preserving additional header fields in metadata where relevant. | ✓ VERIFIED | Regression sanity: Starfield v2 metadata and deflate extraction tests remain in `tests/ba2_reader_tests.cpp`; full CTest passed 89/89. |
| 3 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v3 archives with version-specific compression-method routing. | ✓ VERIFIED | Regression sanity: v3 tests still cover compression method, method-3 LZ4 block, raw method-3, default deflate, and codec confusion; full CTest passed 89/89. |
| 4 | Consumer can resolve BA2 entry names from length-prefixed file name tables located at `FileTableOffset`. | ✓ VERIFIED | `read_name_table` reads uint16-prefixed names from `file_table_offset`; focused tests include `open_ba2 associates length-prefixed names from FileTableOffset`. |
| 5 | Maintainer can validate BA2 general metadata, hash, file-table, deflate, and raw LZ4 behavior against fixtures. | ✓ VERIFIED | `tests/ba2_reader_tests.cpp` contains deterministic fixture builders and 29 BA2 reader tests; full CTest passed 89/89. |
| 6 | D-01: Separate BA2 public header/API exists without adding BA2 symbols to `bsa.hpp` or a family-neutral archive API. | ✓ VERIFIED | Prior verified item sanity-checked by full CTest/public smoke: `libbsa.public_header_smoke` passed. |
| 7 | D-02: `ba2_archive` exposes `summary()`, `paths()`, `contains()`, and `entry()` parallel to `bsa_archive`. | ✓ VERIFIED | `src/ba2_reader.cpp:294-317` forwards `ba2_archive` methods to `archive_view`; full CTest passed. |
| 8 | D-03: Consumer-style smoke covers `ba2.hpp`, `ba2_archive`, `open_ba2`, and `extract_ba2_entry`. | ✓ VERIFIED | `libbsa.public_header_smoke` passed in full CTest. |
| 9 | D-04: `ba2_archive` is metadata-only and extraction requires caller-owned `byte_source`. | ✓ VERIFIED | `ba2_archive` stores `archive_view`; `extract_ba2_entry(const ba2_archive&, const byte_source&, ...)` re-receives source in `src/ba2_reader.cpp:324-357`. |
| 10 | D-05: BA2 archive metadata uses existing `archive_summary` fields without unknown/reserved public header words. | ✓ VERIFIED | Parser populates existing summary fields at `src/ba2_reader.cpp:280-288`; no gap-closure changes added public fields. |
| 11 | D-06: BA2 hash components map into `entry_metadata::name_hash` and `directory_hash`. | ✓ VERIFIED | `src/ba2_reader.cpp:272-273`; existing metadata assertions still pass. |
| 12 | D-07: Size semantics distinguish unpacked size, packed size, and stored on-disk range. | ✓ VERIFIED | `stored_size_for_record()` and assignments at `src/ba2_reader.cpp:165-167,266-270`; existing size tests still pass. |
| 13 | D-08: `entry_metadata::offset` is archive-absolute payload offset. | ✓ VERIFIED | Parser assigns BA2 record offset directly and validates range at `src/ba2_reader.cpp:265,274-276`; focused tests passed. |
| 14 | D-09: Metadata compression state matches extraction behavior. | ✓ VERIFIED | `compression_for_record()` resolves raw/deflate/lz4_block/unknown; extraction routes via `resolve_payload_codec`; focused tests passed. |
| 15 | D-10: Starfield v3 method-3 compressed entries use raw LZ4 block and raw entries remain raw. | ✓ VERIFIED | `src/ba2_reader.cpp:105-121`; method-3 LZ4 and raw tests passed. |
| 16 | D-11: BA2 deflate extraction uses record PackedSize/Size without BSA embedded-size prefix. | ✓ VERIFIED | Extraction comment/code at `src/ba2_reader.cpp:350-352`; deflate tests named `without BSA prefix` passed. |
| 17 | D-12: Unsupported/inconsistent compression metadata fails structurally without fallback or partial writes. | ✓ VERIFIED | Codec-confusion test `extract_ba2_entry rejects Starfield v3 codec confusion without partial writes` passed. |
| 18 | D-13: Generated deterministic fixtures are the Phase 6 acceptance corpus. | ✓ VERIFIED | Fixture builders remain in `tests/ba2_reader_tests.cpp`; no external archive corpus or TES5Edit mutation needed. |
| 19 | D-14: Fixture matrix covers FO4, Starfield v2/v3, FileTableOffset, `.dds` GNRL payload, deflate, and LZ4 block routes. | ✓ VERIFIED | BA2 focused CTest passed 29/29, including nominal matrix and malformed gap regressions. |
| 20 | D-15: BA2 fixture builders remain test-local. | ✓ VERIFIED | Fixture helpers remain in the anonymous namespace of `tests/ba2_reader_tests.cpp`. |
| 21 | D-16: Malformed BA2 headers, records, name tables, file/name count mismatches, impossible offsets, duplicate normalized names, and codec route confusion fail with structured errors while valid empty archives remain accepted. | ✓ VERIFIED | Prior gap closed: `src/ba2_reader.cpp:197-199` validates `file_table_offset > source.size()` before name parsing, including `file_count == 0`; `src/ba2_reader.cpp:249-264` rejects duplicate normalized paths before `ba2_archive{summary, entries}` and before `archive_view::insert_or_assign` can collapse them; `tests/ba2_reader_tests.cpp:529-566` covers duplicate normalized names, impossible empty-archive FileTableOffset, and valid empty archives. Focused BA2 tests passed 29/29. |

**Score:** 21/21 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/ba2.hpp` | Public BA2 archive API | ✓ VERIFIED | Previously verified; full CTest including public-header smoke passed. |
| `src/ba2_reader.cpp` | BA2 parser/extractor plus parser-side gap closure validation | ✓ VERIFIED | Substantive and wired. `file_table_offset > source.size()` check at lines 197-199 runs independently of `file_count`; duplicate normalized path detection at lines 249-264 rejects before archive construction. |
| `tests/ba2_reader_tests.cpp` | Generated BA2 fixtures and malformed regression tests | ✓ VERIFIED | New generated tests at lines 529-566 cover duplicate normalized names, impossible zero-entry FileTableOffset, and valid empty archives. Focused CTest passed 29/29. |
| `tests/public_header_smoke.cpp` | Consumer-style public API smoke | ✓ VERIFIED | `libbsa.public_header_smoke` passed in full CTest. |
| `CMakeLists.txt` | Explicit BA2 source/header/test wiring | ✓ VERIFIED | Existing wiring still exercised by build/test; `libbsa_ba2_reader_tests` built successfully. |
| `README.md` | Consumer BA2 GNRL support docs | ✓ VERIFIED | Previously verified; gap closure did not alter documentation boundary claims. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `parse_ba2_gnrl` | `byte_source::size` | `file_table_offset > source.size()` before `read_name_table` | ✓ WIRED | `src/ba2_reader.cpp:197-199` validates header metadata independently, so empty archives cannot bypass range validation. |
| `parse_ba2_gnrl` | `archive_view` | duplicate detection before `ba2_archive{summary, entries}` | ✓ WIRED | `src/ba2_reader.cpp:249-264` rejects duplicate normalized names before `src/archive_view.cpp:15` can `insert_or_assign`. |
| `tests/ba2_reader_tests.cpp` | malformed duplicate behavior | generated fixture with case/slash variants | ✓ WIRED | `tests/ba2_reader_tests.cpp:529-545` opens `Meshes/Armor/Iron.NIF` and `meshes\\armor\\iron.nif` fixture and asserts `malformed_archive`. |
| `tests/ba2_reader_tests.cpp` | empty FileTableOffset behavior | generated zero-entry fixture and header mutation | ✓ WIRED | `tests/ba2_reader_tests.cpp:547-566` asserts out-of-range empty archive fails and in-range empty archive opens with empty paths. |
| `extract_ba2_entry` | compression dispatcher | `resolve_payload_codec` + `decompress_payload` | ✓ WIRED | Regression sanity: codec/full tests passed; no gap-closure regression observed. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `src/ba2_reader.cpp` | `file_table_offset` | BA2 header bytes read from caller `byte_source` | Yes | ✓ FLOWING — value is checked against `source.size()` before name table parsing. |
| `src/ba2_reader.cpp` | normalized BA2 names | Length-prefixed name table bytes from `FileTableOffset` | Yes | ✓ FLOWING — raw names normalize through `normalize_archive_path`; duplicates are rejected before metadata publication. |
| `tests/ba2_reader_tests.cpp` | gap fixtures | Test-local deterministic `ba2_gnrl_archive_bytes` and `write_u64` helpers | Yes | ✓ FLOWING — focused tests exercise real `open_ba2` parser behavior. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Relevant commit history exists | `git show --stat --oneline 4f733b4 cdf8db1 515f06f a05ce71` | Shows RED test commit, GREEN parser fix commit, and docs commits | ✓ PASS |
| Focused BA2 reader tests | `cmake --build "build/local-vs2026-vcpkg" --config Debug --target libbsa_ba2_reader_tests && ctest --test-dir "build/local-vs2026-vcpkg" --output-on-failure -C Debug -R libbsa_ba2_reader_tests` | 29/29 tests passed | ✓ PASS |
| Full suite | `ctest --test-dir "build/local-vs2026-vcpkg" --output-on-failure -C Debug` | 89/89 tests passed | ✓ PASS |
| Parser hardening patterns | Grep for `file_table_offset > source.size`, `duplicate BA2 name`, `archive_view::insert_or_assign`, `normalized_paths` in `src/ba2_reader.cpp` | All expected patterns found at lines 197 and 249-264 | ✓ PASS |
| Gap test patterns | Grep for three gap test names, duplicate name variants, `write_u64(bytes, 16U`, and `paths().empty` in `tests/ba2_reader_tests.cpp` | All expected patterns found at lines 529-566 | ✓ PASS |
| TES5Edit read-only boundary | `git status --short TES5Edit` | No output | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| BA2-01 | Plans 06-01 through 06-06 | Consumer can open, list, inspect, and extract Fallout 4 BA2 GNRL archives across supported v1/v7/v8 variants. | ✓ SATISFIED | Regression tests still pass in focused/full CTest. |
| BA2-02 | Plans 06-01 through 06-06 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v2 archives with additional header fields preserved where relevant. | ✓ SATISFIED | Starfield v2 metadata/extraction tests pass. |
| BA2-03 | Plans 06-01 through 06-06 | Consumer can open, list, inspect, and extract Starfield BA2 GNRL v3 archives with correct compression-method routing. | ✓ SATISFIED | Starfield v3 compression routing and codec-confusion tests pass. |
| BA2-04 | Plans 06-01, 06-02, 06-05, 06-06, 06-07 | Consumer can parse BA2 file name tables at `FileTableOffset` and associate length-prefixed names with entries. | ✓ SATISFIED | Gap closed: duplicate normalized names now fail structurally, `FileTableOffset` is range-validated for empty archives, and valid empty archives remain accepted. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| None | - | - | - | No TODO/FIXME/placeholder/stub patterns found in `src/ba2_reader.cpp` or `tests/ba2_reader_tests.cpp` during re-verification scan. |

### Human Verification Required

None. The remaining Phase 6 gap-closure behavior is parser/test behavior and was verified through code inspection plus focused and full test execution.

### Gaps Summary

No follow-up gaps remain for Phase 6. The prior D-16/BA2-04 blocker is closed: duplicate normalized BA2 names fail before `ba2_archive`/`archive_view` construction, `FileTableOffset` bounds validation runs even for zero-entry archives, and generated fixture tests prove both malformed cases while preserving valid empty BA2 archives.

---

_Verified: 2026-05-06T04:03:05Z_
_Verifier: the agent (gsd-verifier)_
