---
phase: 10-tes3-write-support-and-bsa-format-completeness
verified: 2026-05-10T01:05:00Z
status: gaps_found
score: 7/9 must-haves verified
overrides_applied: 0
gaps:
  - truth: "TES3 writer validates archive paths such that every accepted writer path serializes into a reader-reopenable archive."
    status: failed
    reason: "Embedded NUL bytes are accepted by normalize_archive_path/make_entry, but TES3 writes names as NUL-terminated strings and hashes the full string; reader reparses the truncated name and rejects the archive with a stored-hash mismatch."
    artifacts:
      - path: "src/detail/archive_path.cpp"
        issue: "normalize_archive_path iterates input bytes without rejecting '\\0'."
      - path: "src/formats/bsa/tes3_bsa_writer.cpp"
        issue: "make_entry does not reject NUL before preserving/hashing; write_string_terminated emits embedded NUL into the name table."
    missing:
      - "Reject embedded NUL archive paths, preferably in normalize_archive_path and/or at the TES3 writer boundary."
      - "Add writer coverage proving add_file/add_bytes return invalid_argument for NUL-containing archive paths."
  - truth: "TES3 writer safe publish respects overwrite_existing=false and cannot replace an output that appears during finalization."
    status: failed
    reason: "Code checks destination existence before source preparation/temp writing, but the non-overwrite path renames temp_path to output_path without a final no-replace check; on platforms where rename replaces existing files this can violate the no-overwrite contract. This matches Phase 10 review CR-01."
    artifacts:
      - path: "src/formats/bsa/tes3_bsa_writer.cpp"
        issue: "Lines 389-396 perform the initial non-overwrite check; lines 467-470 publish without re-checking destination existence or using a no-replace primitive."
    missing:
      - "Re-check output_path immediately before the non-overwrite rename and fail if it now exists."
      - "Add a regression test or platform abstraction for no-replace publish semantics."
---

# Phase 10: TES3 Write Support and BSA Format Completeness Verification Report

**Phase Goal:** Consumers can create TES3/Morrowind BSA archives from files or memory, completing read/write support for all BSA families.
**Verified:** 2026-05-10T01:05:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can create a TES3/Morrowind BSA archive from disk files or memory buffers. | ✓ VERIFIED | `include/libbsa/writer.hpp` exposes `tes3_bsa_writer::add_file`, `add_bytes`, and `write_to`; `src/formats/bsa/tes3_bsa_writer.cpp` stores disk entries path-backed and memory entries copied. Tests `tes3_bsa_writer copies memory entries`, `reports missing disk sources`, and round-trip test passed. |
| 2 | Writer can serialize TES3 file indexes and payload offsets using data-section-relative semantics. | ✓ VERIFIED | Serializer writes TES3 header/version, file records, name offsets, hashes, and raw payloads in `src/formats/bsa/tes3_bsa_writer.cpp`; `assign_raw_offsets` comments and code set offsets relative to data section. Test `tes3_bsa_writer emits byte-accurate raw TES3 tables in hash order` passed. |
| 3 | Maintainer can pack, reopen, extract, and byte-compare TES3 writer output against source files. | ✓ VERIFIED | Test `tes3_bsa_writer output reopens through reader lookup and extraction APIs` opens writer output through `archive_reader::open`, checks metadata, lookup variants, `extract_bytes`, sink extraction, and byte equality for disk, memory, and zero-byte payloads. |
| 4 | Public TES3 writer API is dedicated, raw-only, documented, and dependency-light. | ✓ VERIFIED | Public header contains `tes3_bsa_writer_options` with only `overwrite_existing`; comments state raw/uncompressed and no compression/dedupe/embedded-name controls. Public boundary tests passed. |
| 5 | TES3 writer validates sources and avoids publishing partial archives on source/load failures. | ✓ VERIFIED | `write_tes3_bsa_archive` validates entries and prepares all source bytes before reserving a temp publish directory. Missing disk-source test returns `io_error` from `write_to`. |
| 6 | Accepted archive paths always produce reader-reopenable writer output. | ✗ FAILED | Embedded NUL paths are accepted by `normalize_archive_path` and `make_entry`; writer hashes full string but reader parses only up to NUL and rejects hash mismatch. |
| 7 | `overwrite_existing=false` cannot replace existing output. | ✗ FAILED | Normal pre-existing output is tested, but code has no final no-replace check before non-overwrite rename after temp writing; portable race/data-loss gap remains from review CR-01. |
| 8 | Committed TES3 writer fixture evidence is generated through the public writer API and records D-11 layout facts. | ✓ VERIFIED | Generator uses `libbsa::tes3_bsa_writer`; manifest records source kind, paths, hash low/high, raw offset, payload offset, sizes, expected bytes, and synthetic/no-TES5Edit provenance. Fixture generator target and manifest validation tests passed. |
| 9 | `TES5Edit/` remains read-only/untouched. | ✓ VERIFIED | `git -C TES5Edit status --short` produced no output. |

**Score:** 7/9 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/writer.hpp` | Public TES3 writer contract | ✓ VERIFIED | Doxygen-commented `tes3_bsa_writer_options` and `tes3_bsa_writer` with disk/memory/write APIs. |
| `src/formats/bsa/tes3_bsa_writer.hpp` | Private entry and serializer declaration | ✓ VERIFIED | Defines `tes3_writer_entry` and `write_tes3_bsa_archive`. |
| `src/formats/bsa/tes3_bsa_writer.cpp` | TES3 writer implementation/serializer/publish | ⚠️ PARTIAL | Core serializer and round-trip behavior present; NUL path validation and no-overwrite race gaps remain. |
| `tests/unit/tes3_bsa_writer_tests.cpp` | Byte-level, round-trip, validation, fixture tests | ✓ VERIFIED | 11 focused TES3 writer CTest cases passed. Missing NUL/no-replace race regression coverage. |
| `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp` | Public-writer fixture generator | ⚠️ WARNING | Generator exists and builds; review warnings remain: no post-write stream-state checks and missing direct `<cctype>` include for `std::tolower`. |
| `tests/fixtures/generated/archives/tes3_writer_canonical.bsa` | Committed synthetic writer archive | ✓ VERIFIED | Generator target rebuilt without changing git status; manifest validation test passes. |
| `tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json` | Layout/provenance manifest | ✓ VERIFIED | Contains D-11 fields and synthetic/no-TES5Edit provenance. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| Public writer API | Private serializer | `tes3_bsa_writer::write_to` delegates to `formats::bsa::write_tes3_bsa_archive` | ✓ WIRED | `src/formats/bsa/tes3_bsa_writer.cpp:84-85`. |
| Build target | TES3 writer source | Root `CMakeLists.txt` source registration | ✓ WIRED | `CMakeLists.txt` includes `src/formats/bsa/tes3_bsa_writer.cpp`. |
| Writer output | Public reader facade | `archive_reader::open` tests | ✓ WIRED | `tests/unit/tes3_bsa_writer_tests.cpp` opens writer-produced archives and extracts payloads. |
| Fixture generator | Public TES3 writer API | `add_file`, `add_bytes`, `write_to` | ✓ WIRED | Generator main uses `libbsa::tes3_bsa_writer`. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `tes3_bsa_writer.cpp` | `entries` / `prepared_entry::payload` | `add_file` host path late read and `add_bytes` copied memory | Yes | ✓ FLOWING |
| `tes3_bsa_writer.cpp` | raw offsets/hash/name tables | Prepared entries sorted by `detail::tes3_hash_sort_key(detail::hash_tes3(...))` | Yes | ✓ FLOWING |
| `generate_tes3_bsa_writer_fixtures.cpp` | manifest entries | Parses generated archive bytes and cross-references source entries | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused TES3/BSA regression suite | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "tes3_bsa_writer|tes3_bsa_reader|TES4 BSA writer|public_include_boundary" --output-on-failure` | 33/33 passed | ✓ PASS |
| TES3 writer tests | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "tes3_bsa_writer" --output-on-failure` | 11/11 passed | ✓ PASS |
| Fixture generation | `cmake --build "build/windows-msvc-debug-static" --config Debug --target generate_tes3_bsa_writer_fixtures` | Passed; no git diff after generation | ✓ PASS |
| Full suite | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug --output-on-failure` | 166/166 passed, with expected local-fixture skip | ✓ PASS |
| TES5Edit cleanliness | `git -C "TES5Edit" status --short` | Empty output | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| WBSA-04 | 10-01 through 10-05 PLAN frontmatter | Consumer can create new TES3/Morrowind BSA archives from disk files or memory buffers. | ⚠️ PARTIAL | Main disk/memory writer flow is implemented and tested through writer/reader round-trip, but NUL-path validation can produce self-inconsistent unreadable archives and no-overwrite publish has a portable race gap. |

No additional Phase 10 requirement IDs were found in `.planning/REQUIREMENTS.md` beyond `WBSA-04`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/detail/archive_path.cpp` | 33-47 | Missing embedded-NUL rejection | 🛑 Blocker | Writer can accept a path it cannot serialize into a reader-reopenable archive. |
| `src/formats/bsa/tes3_bsa_writer.cpp` | 467-470 | Non-overwrite publish lacks final existence check | 🛑 Blocker | Potential cross-platform data-loss race contrary to `overwrite_existing=false`. |
| `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp` | 127-143 | Writes not checked after output operations | ⚠️ Warning | Fixture generator could silently leave truncated source/manifest on write failure. |
| `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp` | 54-58 | `std::tolower` without direct `<cctype>` include | ⚠️ Warning | Non-portable reliance on transitive include. |

### Human Verification Required

None. Automated code inspection and CTest coverage were sufficient for this phase's observable non-visual library behavior. Game/tool compatibility validation is explicitly scheduled for Phase 11 and not counted as a Phase 10 human gate.

### Gaps Summary

Phase 10 achieves the main happy-path TES3 writer outcome: public disk/memory APIs exist, serializer layout is byte-verified, writer output reopens/extracts through public reader APIs, fixture evidence is committed, full tests pass, and `TES5Edit/` is clean.

However, two blocker gaps remain. First, embedded NUL archive paths pass normalization and writer entry creation but serialize as NUL-terminated names while hashing the full string, producing self-inconsistent archives the reader rejects. Second, the non-overwrite publish path has no final destination existence check immediately before rename, leaving a portable race where `overwrite_existing=false` can still replace a concurrently-created file on platforms with replacing rename semantics.

---

_Verified: 2026-05-10T01:05:00Z_
_Verifier: the agent (gsd-verifier)_
