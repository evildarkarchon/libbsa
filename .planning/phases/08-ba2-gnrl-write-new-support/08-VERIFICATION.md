---
phase: 08-ba2-gnrl-write-new-support
verified: 2026-05-09T08:32:56Z
status: gaps_found
score: 13/14 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Public BA2 GNRL writer finalization must not delete unrelated caller files or lose an existing destination during failed publish."
    status: failed
    reason: "write_to uses a deterministic output+'.tmp' path, removes it before writing, and removes the destination before rename when overwrite_existing is true. This is observable implementation evidence of a public writer data-loss hazard."
    artifacts:
      - path: "src/formats/ba2/ba2_gnrl_writer.cpp"
        issue: "Lines 578-596 append '.tmp', remove any pre-existing temp path, and remove output_path before publishing the replacement."
    missing:
      - "Allocate a unique temporary file exclusively in the destination directory without deleting pre-existing unrelated temp-name collisions."
      - "Publish replacements without deleting the existing destination until replacement can succeed or be rolled back."
      - "Add reader-backed/public writer tests covering temp-name collision and overwrite publish failure behavior."
deferred:
  - truth: "Parser rejects malformed BA2 GNRL payload offsets that overlap the fixed header or record table."
    addressed_in: "Phase 11"
    evidence: "Phase 11 goal: 'Consumers and maintainers can validate archives and compatibility quirks with structured warnings/errors while malformed inputs are rejected safely.'"
---

# Phase 8: BA2 GNRL Write-New Support Verification Report

**Phase Goal:** Consumers can create Fallout 4 and Starfield BA2 GNRL archives with explicit target profile, version fields, filename tables, and compression policy.  
**Verified:** 2026-05-09T08:32:56Z  
**Status:** gaps_found  
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Dedicated dependency-light BA2 GNRL writer API exists. | ✓ VERIFIED | `include/libbsa/writer.hpp:44-52` defines `ba2_gnrl_target`; `writer.hpp:69-106` defines BA2 writer/entry options; `writer.hpp:154-207` defines `ba2_gnrl_writer`. Public boundary tests assert type and method contracts in `tests/unit/public_include_boundary_tests.cpp:24-101`. |
| 2 | Consumer can add disk-file and copied memory entries and finalize only to a host-path archive. | ✓ VERIFIED | Public API exposes `add_file`, `add_bytes`, and `write_to(std::string_view)` only (`writer.hpp:173-201`). Implementation stores disk host paths and copies memory spans into `memory_bytes` (`ba2_gnrl_writer.cpp:73-110`). |
| 3 | Memory-buffer entries are copied at add time. | ✓ VERIFIED | `ba2_gnrl_writer.cpp:107` assigns the span into writer-owned `memory_bytes`; reader-backed mutation proof is in `ba2_gnrl_writer_tests.cpp:341-368`. |
| 4 | Duplicate canonical paths, invalid archive paths, missing disk sources, and overwrite defaults return structured errors. | ✓ VERIFIED | Validation and duplicate checks are in `ba2_gnrl_writer.cpp:514-538` and overwrite guard at `556-559`; tests assert stable `error_code` values at `ba2_gnrl_writer_tests.cpp:270-328`. |
| 5 | Consumer can create Fallout 4 BA2 GNRL archives and Starfield v2/v3 BA2 GNRL archives from disk files or memory buffers. | ✓ VERIFIED | Target version routing in `ba2_gnrl_writer.cpp:175-195`; raw FO4 and Starfield reader-backed tests at `ba2_gnrl_writer_tests.cpp:341-427`; targeted CTest passed 14/14 BA2 writer tests. |
| 6 | Consumer can select explicit Starfield target version and compression method policy instead of relying on file extensions. | ✓ VERIFIED | `ba2_gnrl_target::starfield_v2`/`starfield_v3` and `starfield_compression_method` are public (`writer.hpp:48-52`, `89-93`); compression method routing uses target/options only (`ba2_gnrl_writer.cpp:247-263`, `287-292`). No-extension-inference test uses `.bin` and expects target-selected compression (`ba2_gnrl_writer_tests.cpp:429-471`). |
| 7 | Writer serializes BA2 filename tables at the end of the archive. | ✓ VERIFIED | Writer emits records, then payload bytes, then UInt16-prefixed names (`ba2_gnrl_writer.cpp:436-458`) and sets `file_table_offset` after payload offset assignment (`572-573`). Physical layout tests read `file_table_offset` and final names at `ba2_gnrl_writer_tests.cpp:95-140`, with assertions in `155-213`. |
| 8 | Version-specific BA2 header fields are preserved/set according to target profiles. | ✓ VERIFIED | Starfield Unknown1/Unknown2 and v3 CompressionMethod are serialized in `ba2_gnrl_writer.cpp:420-434`; override/default tests assert reopened metadata at `ba2_gnrl_writer_tests.cpp:370-427`. |
| 9 | Writer computes BA2 hash/record metadata and preserves per-entry record flags. | ✓ VERIFIED | `detail::hash_fo4` is used for file and directory hashes (`ba2_gnrl_writer.cpp:318-319`); record flags use caller override or zero (`320`); reopened assertions include `archive_hash` and `record_flags` (`ba2_gnrl_writer_tests.cpp:198-207`, `396-427`). |
| 10 | Writer compresses BA2 GNRL entries with deflate or raw LZ4 block according to target format/version. | ✓ VERIFIED | FO4/SFv2 and SFv3 method 0 map to `compression_method::deflate`; SFv3 method 3 maps to `compression_method::lz4_block` (`ba2_gnrl_writer.cpp:247-260`), then calls `detail::compress_payload` (`292`). Compression round-trip tests cover deflate and LZ4 block (`ba2_gnrl_writer_tests.cpp:429-522`). |
| 11 | Per-entry compression overrides choose only raw vs compressed while Starfield v3 compressed entries use archive-wide CompressionMethod. | ✓ VERIFIED | `requested_entry_compression` only resolves raw/compressed state (`ba2_gnrl_writer.cpp:235-245`); method selection is separate and archive-wide (`247-260`). Test at `ba2_gnrl_writer_tests.cpp:473-522` verifies raw override and LZ4 compressed sibling. |
| 12 | Deduplication is disabled by default and enabled only by `deduplicate_payloads`, comparing final stored payload bytes. | ✓ VERIFIED | Dedupe gate and stored-payload key are in `ba2_gnrl_writer.cpp:336-379`, specifically keying `std::map<std::vector<std::byte>, payload_assignment>` after compression routing. Reader-backed offset tests at `ba2_gnrl_writer_tests.cpp:524-617` verify disabled/enabled and raw-vs-compressed distinctness. |
| 13 | Full writer behavior is reader-backed and regression-tested. | ✓ VERIFIED | Writer tests repeatedly reopen output through `archive_reader::open` (`ba2_gnrl_writer_tests.cpp:162`, `229`, `493`, `535`, `563`, `598`). Spot-check command `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` passed 14/14 tests; summary reports full suite 124/124 passed with expected local fixture skip. |
| 14 | Public BA2 GNRL writer finalization must not delete unrelated caller files or lose an existing destination during failed publish. | ✗ FAILED | `ba2_gnrl_writer.cpp:578-596` uses deterministic `output_path + '.tmp'`, removes that path before writing, and removes `output_path` before `rename` when overwrite is true. This can delete unrelated `*.tmp` files and can lose the old archive if publish fails. |

**Score:** 13/14 truths verified

### Deferred Items

Items not yet met but explicitly addressed in later milestone phases.

| # | Item | Addressed In | Evidence |
|---|---|---|---|
| 1 | Parser rejects malformed BA2 GNRL payload offsets that overlap the fixed header or record table. | Phase 11 | Phase 11 goal covers malformed-input hardening: “malformed inputs are rejected safely.” |

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/writer.hpp` | Public BA2 GNRL target/options/writer contract | ✓ VERIFIED | Contains dedicated target enum, options, entry options, and `ba2_gnrl_writer` declarations (`writer.hpp:44-207`). |
| `src/formats/ba2/ba2_gnrl_writer.hpp` | Private writer entry model and write contract | ✓ VERIFIED | Contains `ba2_gnrl_writer_entry` and `write_ba2_gnrl_archive` (`ba2_gnrl_writer.hpp:13-25`). |
| `src/formats/ba2/ba2_gnrl_writer.cpp` | Writer implementation, serialization, compression, dedupe | ⚠️ PARTIAL | Core behavior is substantive and wired, but publish/finalization has a blocker data-loss hazard at `578-596`. |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | Reader support for end filename tables | ✓ VERIFIED | Host-file parser reads exactly `file_count` names from `FileTableOffset` (`426-437`) and supports payload-before-name-table output. Metadata-overlap hardening is deferred. |
| `tests/unit/ba2_gnrl_writer_tests.cpp` | Reader-backed writer validation tests | ✓ VERIFIED | 14 BA2 writer tests cover raw/compressed/profile/dedupe behavior and pass. |
| `tests/unit/ba2_gnrl_reader_tests.cpp` | End filename-table reader regression | ✓ VERIFIED | `[ba2_gnrl_end_table]` regression exists at `295-361`; SDK artifact pattern expected the literal phrase “end filename table,” but equivalent tagged coverage exists. |
| `tests/unit/public_include_boundary_tests.cpp` | Public dependency-boundary assertions | ✓ VERIFIED | BA2 writer API static assertions and forbidden-token checks exist at lines `24-101`. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| Public `ba2_gnrl_writer` | Private writer implementation | `ba2_gnrl_writer::state` and `write_to` call | ✓ WIRED | `writer.hpp` declares private `state`; `ba2_gnrl_writer.cpp:25-29` defines it; `write_to` delegates to `formats::ba2::write_ba2_gnrl_archive` at `113-115`. |
| Writer output | `archive_reader::open` | Reader-backed tests reopen produced host path | ✓ WIRED | Tests call `archive_reader::open(output.string())` after `write_to` at multiple locations. |
| Writer compression options | `detail::compress_payload` | Target/profile + Starfield method routing | ✓ WIRED | `prepare_entries` computes method from target/options and calls `detail::compress_payload` (`247-292`). |
| Stored-payload dedupe | Reopened `payload_offset` metadata | Reader-backed offset assertions | ✓ WIRED | Tests compare reopened `payload_offset` for disabled/enabled/raw-vs-compressed cases (`543`, `574`, `612-613`). |
| Publish/finalization | Host filesystem safety | Temporary file and rename sequence | ✗ NOT_SAFE | Deterministic temp path removal and destination removal are wired into the public writer path (`578-596`). |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `ba2_gnrl_writer.cpp` | `stored_payload` | Disk source or copied memory, optionally through `detail::compress_payload` | Yes | ✓ FLOWING — serialized into payload area and extracted by reader-backed tests. |
| `ba2_gnrl_writer.cpp` | `file_table_offset` | Header/record size plus emitted owned payload bytes | Yes | ✓ FLOWING — serialized in header and consumed by parser/tests. |
| `ba2_gnrl_writer.cpp` | Starfield metadata fields | `ba2_gnrl_writer_options` | Yes | ✓ FLOWING — serialized and reopened as public metadata. |
| `ba2_gnrl_writer.cpp` | Dedupe payload offsets | Stored payload byte map after compression routing | Yes | ✓ FLOWING — reopened `payload_offset` proves shared/distinct behavior. |
| `ba2_gnrl_writer.cpp` | Publish temp/destination paths | `output_host_path` with appended `.tmp` | Unsafe | ✗ HAZARDOUS — data flow can delete pre-existing temp path and existing destination. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| BA2 GNRL writer reader-backed tests pass | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` | 14/14 tests passed in 0.20s | ✓ PASS |
| TES5Edit submodule remains unmodified | `git -C TES5Edit status --short` | No output after combined command | ✓ PASS |
| Full suite | Not rerun during verification; summary evidence says `ctest --preset windows-msvc-debug-static --output-on-failure` passed 124/124 with expected local fixture skip. | Existing evidence only | ✓ PASS (summary-backed) |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| WBA2-01 | 08-01, 08-03, 08-04, 08-06 | Consumer can create new Fallout 4 BA2 GNRL archives from disk files or memory buffers. | ✓ SATISFIED | Public API plus FO4 disk/memory round-trip test at `ba2_gnrl_writer_tests.cpp:341-368`. |
| WBA2-02 | 08-01, 08-04, 08-05, 08-06 | Consumer can create Starfield BA2 GNRL archives with explicit target version and compression method policy. | ✓ SATISFIED | Target enum/options in public header and SFv2/SFv3 metadata/compression tests at `370-471`. |
| WBA2-03 | 08-02, 08-04, 08-06 | Writer can serialize BA2 filename tables at the end of the archive. | ✓ SATISFIED | Serialization writes names after payloads (`ba2_gnrl_writer.cpp:445-458`); physical layout helper verifies final table. |
| WBA2-04 | 08-05, 08-06 | Writer can compress BA2 GNRL entries with deflate or raw LZ4 block according to target format/version. | ✓ SATISFIED | Compression method routing in `ba2_gnrl_writer.cpp:247-292`; tests at `429-522`. |
| WBA2-05 | 08-01, 08-04, 08-05, 08-06 | Writer can preserve or set version-specific BA2 header fields according to documented target profiles. | ✓ SATISFIED | Starfield fields serialized at `420-434`; override/default tests at `370-427`. |

No orphaned Phase 8 requirements were found in `.planning/REQUIREMENTS.md`: WBA2-01 through WBA2-05 all map to Phase 8 and are claimed by at least one plan.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/formats/ba2/ba2_gnrl_writer.cpp` | 557 | Throwing `std::filesystem::exists(output_path)` overload at public API boundary | ⚠️ Warning | Filesystem exceptions can escape instead of returning `result<void>` errors. |
| `src/formats/ba2/ba2_gnrl_writer.cpp` | 578-596 | Deterministic `.tmp` publish path plus unconditional temp removal and destination removal | 🛑 Blocker | Public writer can delete unrelated temp-name collision and can lose existing output if replacement publish fails. |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | 259-286 | Payload overlap validation checks filename table but not fixed header/record table | Deferred | Malformed archive hardening gap; specifically covered by Phase 11, not counted as Phase 8 blocker. |

### Human Verification Required

None. The phase is library/API behavior with reader-backed automated tests; the remaining blocker is directly observable in code.

### Gaps Summary

The BA2 GNRL writer substantially achieves the archive creation, target-profile, metadata, compression, dedupe, and reader-backed validation goals. However, public writer finalization has a blocking data-loss defect: `write_to` deletes a deterministic sibling temp path and, in overwrite mode, deletes the destination before the replacement is safely published. This violates the safety expectations of a reusable write-new library API and must be fixed before the phase can be considered complete.

The parser metadata-overlap issue from code review is real, but it is conservatively deferred to Phase 11 because the roadmap explicitly assigns malformed-input hardening to that phase and the Phase 8 writer does not produce such malformed offsets.

---

_Verified: 2026-05-09T08:32:56Z_  
_Verifier: the agent (gsd-verifier)_
