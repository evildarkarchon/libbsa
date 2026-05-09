---
phase: 07-tes4-family-bsa-write-new-support
verified: 2026-05-09T06:49:05Z
status: passed
score: 5/5 must-haves verified
overrides_applied: 0
---

# Phase 7: TES4-Family BSA Write-New Support Verification Report

**Phase Goal:** Consumers can create new TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE BSA archives from files or memory and prove them by reopening and extracting.
**Verified:** 2026-05-09T06:49:05Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can create TES4/Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE BSA archives from disk files or memory buffers. | ✓ VERIFIED | `include/libbsa/writer.hpp` exposes `tes4_bsa_writer`, target profiles, `add_file`, `add_bytes`, and `write_to`. `tes4_bsa_writer.cpp` maps targets to versions `0x67`, `0x68`, and `0x69`; tests reopen output for all three targets and extract disk, memory, copied-memory, and zero-byte entries. |
| 2 | Writer can generate folder/file indexes sorted by format-compatible hash order and derive archive/file flags from content. | ✓ VERIFIED | `prepare_folders` computes `detail::hash_tes4` for folders/files and sorts by hash; `file_flag_for_extension` derives category flags from extensions, and `archive_flags` are built from named options. Writer tests verify reopened metadata, entries, lookup, contains, and extraction for multi-folder mixed-extension archives. |
| 3 | Consumer can choose archive defaults or per-file compression overrides while the writer avoids known embedded-name compatibility hazards. | ✓ VERIFIED | `archive_compression_policy` and `entry_compression_policy` are public named options. `encode_stored_payload` routes v103/v104 to deflate and v105 to LZ4 frame through `detail::compress_payload`; zero-byte entries stay raw; `embed_file_names` emits only for non-v103 targets. Tests assert compression metadata, extracted bytes, absent-by-default embedded names, v104/v105 prefixes, and v103 no-prefix compatibility. |
| 4 | Consumer can optionally deduplicate identical payloads by content hash/final stored bytes. | ✓ VERIFIED | `tes4_bsa_writer_options::deduplicate_payloads` defaults false. `assign_offsets` deduplicates only when enabled, after final stored payload construction including embedded-name prefixes, raw-size prefixes, and codec bytes. Tests assert distinct offsets by default, shared offsets when enabled for identical final bytes, and distinct offsets for compression or embedded-name mismatches. |
| 5 | Maintainer can pack, reopen, extract, and byte-compare TES4-family BSA writer output against source files. | ✓ VERIFIED | `tests/unit/tes4_bsa_writer_tests.cpp` uses public `archive_reader::open`, `entries`, `contains`, `find`, `extract_bytes`, and streaming `extract` on writer output. Spot checks passed: `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` (16/16) and public boundary tests (2/2). |

**Score:** 5/5 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `include/libbsa/writer.hpp` | Dependency-light public writer API | ✓ VERIFIED | Exposes target/profile enums, options, writer object, result-returning add/write methods, and private state pointer; no forbidden private dependency tokens found in public headers. |
| `include/libbsa/libbsa.hpp` | Umbrella include exports writer API | ✓ VERIFIED | Contains `#include <libbsa/writer.hpp>`. |
| `CMakeLists.txt` | Build/install wiring | ✓ VERIFIED | Contains `include/libbsa/writer.hpp` in headers and `src/formats/bsa/tes4_bsa_writer.cpp` in sources. |
| `src/formats/bsa/tes4_bsa_writer.hpp` | Private writer entry model and write entry point | ✓ VERIFIED | Defines `tes4_writer_entry` and `write_tes4_bsa_archive`. |
| `src/formats/bsa/tes4_bsa_writer.cpp` | Validation, serialization, compression, embedded names, dedupe | ✓ VERIFIED | 715-line substantive implementation with path validation, source reads, hash sorting, checked offset/size arithmetic, compression routing, embedded-name encoding, temporary publish, and opt-in final-stored-byte dedupe. |
| `tests/unit/tes4_bsa_writer_tests.cpp` | Reader-backed writer behavior coverage | ✓ VERIFIED | 16 Catch2 tests covering all target profiles, disk/memory/copy semantics, validation, compression, embedded names, dedupe, metadata, lookup, extraction, and sink streaming. |
| `tests/unit/public_include_boundary_tests.cpp` | Public API boundary smoke | ✓ VERIFIED | Static asserts/requires expressions cover writer API; forbidden-token scan protects public headers. |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `include/libbsa/libbsa.hpp` | `include/libbsa/writer.hpp` | public include | ✓ WIRED | `#include <libbsa/writer.hpp>` present. |
| `include/libbsa/writer.hpp` | `src/formats/bsa/tes4_bsa_writer.cpp` | public method definitions delegate to private writer implementation | ✓ WIRED | `tes4_bsa_writer::write_to` calls `formats::bsa::write_tes4_bsa_archive`. |
| `src/formats/bsa/tes4_bsa_writer.cpp` | `src/detail/archive_path.cpp` | `detail::normalize_archive_path` | ✓ WIRED | Used for add-time archive path validation and canonical duplicate keys. |
| `src/formats/bsa/tes4_bsa_writer.cpp` | `src/detail/bethesda_hash.cpp` | `detail::hash_tes4` | ✓ WIRED | Used for folder/file record hashes and sorted table order. |
| `src/formats/bsa/tes4_bsa_writer.cpp` | `src/detail/compression_router.cpp` | `detail::compress_payload` | ✓ WIRED | Used for deflate/LZ4-frame compressed entries. |
| `tests/unit/tes4_bsa_writer_tests.cpp` | public reader implementation | `archive_reader::open` and extraction APIs | ✓ WIRED | Writer output is reopened and verified through public metadata, lookup, and extraction APIs. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `tes4_bsa_writer` | writer entries | `add_file` stores explicit host paths; `add_bytes` copies caller spans into writer-owned vectors | Yes | ✓ FLOWING |
| `write_tes4_bsa_archive` | raw payload bytes | `read_source_bytes` reads disk sources or memory bytes | Yes | ✓ FLOWING |
| `prepare_folders` / `assign_offsets` | folder/file records and payload metadata | preserved paths, canonical paths, hashes, compression policy, embedded-name option, dedupe option | Yes | ✓ FLOWING |
| `write_archive_bytes` | archive bytes | serialized header/tables/file-name table/stored payloads | Yes | ✓ FLOWING |
| writer tests | verification data | produced archives reopened with `archive_reader` and extracted bytes compared to source vectors/files | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| TES4-family writer behavior passes | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | 16/16 tests passed in 0.81s | ✓ PASS |
| Public writer API remains dependency-light | `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure` | 2/2 tests passed in 0.05s | ✓ PASS |
| TES5Edit reference submodule remains read-only | `git -C "TES5Edit" status --short` | no output | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| WBSA-01 | 07-01, 07-02, 07-03, 07-04, 07-06 | Consumer can create new TES4/Oblivion BSA v103 archives from disk files or memory buffers. | ✓ SATISFIED | `tes4_bsa_target::oblivion` maps to v103; tests write/reopen/extract v103 archives with disk and memory entries. |
| WBSA-02 | 07-01 through 07-06 | Consumer can create new FO3/FNV/Skyrim LE BSA v104 archives from disk files or memory buffers. | ✓ SATISFIED | `tes4_bsa_target::fallout3` maps to v104; tests cover raw, compressed, embedded-name, and dedupe behavior. |
| WBSA-03 | 07-01 through 07-06 | Consumer can create new Skyrim SE/AE BSA v105 archives from disk files or memory buffers. | ✓ SATISFIED | `tes4_bsa_target::skyrim_se` maps to v105 with 24-byte folder records and LZ4-frame compression; tests reopen and extract v105 output. |
| WBSA-05 | 07-03, 07-06 | Writer can generate folder and file indexes sorted by format-compatible hash order. | ✓ SATISFIED | `prepare_folders` computes `detail::hash_tes4` and sorts folders/files by hash; multi-folder tests reopen/list/find/extract entries. |
| WBSA-06 | 07-03, 07-06 | Writer can derive archive flags and file flags from content using compatible behavior. | ✓ SATISFIED | Archive flags are derived from include/compression/embed options; `file_flag_for_extension` derives file flags by extension/version. |
| WBSA-07 | 07-04, 07-06 | Writer can apply per-file compression overrides while respecting target archive defaults. | ✓ SATISFIED | `requested_entry_compression`, target-routed `compress_payload`, and XOR toggle logic are implemented; tests verify default, raw override, compressed override, and zero-byte behavior. |
| WBSA-08 | 07-05, 07-06 | Writer can write embedded file names where appropriate without triggering known compatibility hazards. | ✓ SATISFIED | `embed_file_names` emits prefixes for v104/v105 only and excludes v103; tests verify metadata and extraction bytes. |
| WBSA-09 | 07-06 | Writer can optionally deduplicate identical file payloads by content hash. | ✓ SATISFIED | `deduplicate_payloads` opt-in shares offsets only for byte-identical final stored payloads; tests verify enabled/disabled and mismatch cases. |
| WBSA-10 | 07-01 through 07-06 | Maintainer can round-trip BSA writer output by packing, reopening, extracting, and byte-comparing source files. | ✓ SATISFIED | Writer tests pack outputs, reopen with public reader, and compare `extract_bytes`/sink output to source bytes. |

All requirement IDs declared by the phase and PLAN frontmatter are accounted for. No Phase 7 WBSA requirement is orphaned in `REQUIREMENTS.md`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| None | — | — | — | Grep scan found no TODO/FIXME/placeholder/not-implemented markers. `return {}` occurrences in `tes4_bsa_writer.cpp` are normal `result<void>` success returns, not empty stub implementations. |

### Human Verification Required

None. This phase produces a C++ library writer and has programmatic reader-backed verification; no visual, external-service, or real-time behavior requires manual testing.

### Gaps Summary

No blocking gaps found. The codebase contains a substantive, wired TES4-family BSA writer implementation with public API exposure, target-profile serialization, hash-sorted tables, derived flags, compression overrides, embedded-name safeguards, opt-in final-stored-byte dedupe, reader-backed round-trip tests, and passing behavioral spot checks.

---

_Verified: 2026-05-09T06:49:05Z_
_Verifier: the agent (gsd-verifier)_
