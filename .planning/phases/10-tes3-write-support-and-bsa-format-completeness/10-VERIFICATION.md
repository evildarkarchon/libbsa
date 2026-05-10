---
phase: 10-tes3-write-support-and-bsa-format-completeness
verified: 2026-05-10T02:31:29Z
status: passed
score: 9/9 must-haves verified
overrides_applied: 0
gaps: []
---

# Phase 10: TES3 Write Support and BSA Format Completeness Verification Report

**Phase Goal:** Consumers can create TES3/Morrowind BSA archives from files or memory, completing read/write support for all BSA families.
**Verified:** 2026-05-10T02:31:29Z
**Status:** passed
**Re-verification:** Yes - after `10-06` gap closure.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Consumer can create a TES3/Morrowind BSA archive from disk files or memory buffers. | VERIFIED | `include/libbsa/writer.hpp` exposes `tes3_bsa_writer::add_file`, `add_bytes`, and `write_to`; full suite and TES3 writer tests passed. |
| 2 | Writer can serialize TES3 file indexes and payload offsets using data-section-relative semantics. | VERIFIED | `src/formats/bsa/tes3_bsa_writer.cpp` serializes TES3 tables and payload offsets; `tes3_bsa_writer emits byte-accurate raw TES3 tables in hash order` passed. |
| 3 | Maintainer can pack, reopen, extract, and byte-compare TES3 writer output against source files. | VERIFIED | `tes3_bsa_writer output reopens through reader lookup and extraction APIs` passed, covering reader reopen, lookup, sink extraction, and byte equality. |
| 4 | Public TES3 writer API is dedicated, raw-only, documented, and dependency-light. | VERIFIED | Public boundary tests passed; the writer API exposes TES3 write operations without compression, DirectXTex, or platform-specific public types. |
| 5 | TES3 writer validates sources and avoids publishing partial archives on source/load failures. | VERIFIED | `tes3_bsa_writer reports missing disk sources from write_to` and related source/path validation tests passed. |
| 6 | Accepted archive paths always produce reader-reopenable writer output. | VERIFIED | Gap closure added shared embedded-NUL rejection; `archive_path rejects embedded-NUL virtual paths` and `tes3_bsa_writer rejects archive paths containing NUL bytes` passed. |
| 7 | `overwrite_existing=false` cannot replace existing output. | VERIFIED | `publish_file_without_replace` and TES3 writer publish tests passed, including `tes3_bsa_writer publish helper never replaces an existing destination` and default no-overwrite preservation. |
| 8 | Committed TES3 writer fixture evidence is generated through the public writer API and records D-11 layout facts. | VERIFIED | `tes3_bsa_writer committed fixture manifest records canonical writer evidence` passed; generated fixture and manifest remain committed. |
| 9 | `TES5Edit/` remains read-only/untouched. | VERIFIED | `git -C "TES5Edit" status --short` produced no output. |

**Score:** 9/9 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/writer.hpp` | Public TES3 writer contract | VERIFIED | Doxygen-commented public TES3 writer API is present and covered by public include boundary tests. |
| `src/formats/bsa/tes3_bsa_writer.hpp` | Private TES3 writer entry and serializer declaration | VERIFIED | Private entry/serializer boundary remains internal. |
| `src/formats/bsa/tes3_bsa_writer.cpp` | TES3 writer implementation, serializer, and publish path | VERIFIED | Reader-backed, byte-level, source-validation, embedded-NUL, and publish semantics tests passed. |
| `src/detail/archive_path.cpp` | Shared archive path normalization | VERIFIED | Embedded-NUL paths now fail before format-specific serialization. |
| `src/detail/atomic_file_ops.hpp` | No-replace publish primitive | VERIFIED | Helper-level no-replace tests and TES3 writer publish tests passed. |
| `tests/unit/archive_path_tests.cpp` | Shared path regression coverage | VERIFIED | `archive_path rejects embedded-NUL virtual paths` passed. |
| `tests/unit/tes3_bsa_writer_tests.cpp` | Byte-level, round-trip, validation, fixture, and gap-closure tests | VERIFIED | Focused Phase 10 gate passed 43/43 tests. |
| `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp` | Public-writer fixture generator | VERIFIED | Fixture manifest validation test passed. |
| `tests/fixtures/generated/archives/tes3_writer_canonical.bsa` | Committed synthetic writer archive | VERIFIED | Covered by committed fixture manifest evidence test. |
| `tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json` | Layout/provenance manifest | VERIFIED | Covered by committed fixture manifest evidence test. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| Public writer API | Private serializer | `tes3_bsa_writer::write_to` delegates to `formats::bsa::write_tes3_bsa_archive` | WIRED | Writer tests exercise disk, memory, validation, and publish flows through the public API. |
| Shared path validator | TES3 writer entry creation | `make_entry` normalizes archive paths before preserving names and hash sorting | WIRED | Embedded-NUL rejection is proven at both shared validator and TES3 writer API boundaries. |
| TES3 non-overwrite finalization | No-replace publish helper | `publish_file_without_replace` prevents replacement when the destination exists | WIRED | Helper-level and writer-level publish tests passed. |
| Writer output | Public reader facade | `archive_reader::open` tests | WIRED | TES3 writer output reopens and extracts through public reader APIs. |
| Fixture generator | Public TES3 writer API | `add_file`, `add_bytes`, `write_to` | WIRED | Committed fixture evidence is generated by the public writer API, not TES5Edit. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Build test target | `cmake --build "build/windows-msvc-debug-static" --target libbsa_tests --config Debug` | Built `libbsa_tests` successfully | PASS |
| Focused Phase 10 regression gate | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path\|tes3_bsa_writer\|tes3_bsa_reader\|TES4 BSA writer\|public_include_boundary" --output-on-failure` | 43/43 passed | PASS |
| Full suite | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug --output-on-failure` | 174/174 passed; local game fixture test skipped as expected | PASS |
| TES5Edit cleanliness | `git -C "TES5Edit" status --short` | Empty output | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| WBSA-04 | 10-01 through 10-06 PLAN frontmatter | Consumer can create new TES3/Morrowind BSA archives from disk files or memory buffers. | COMPLETE | Public TES3 writer API, serializer, reader-backed round-trip, fixture evidence, embedded-NUL rejection, and no-replace publish semantics are implemented and verified. |

No additional Phase 10 requirement IDs were found in `.planning/REQUIREMENTS.md` beyond `WBSA-04`.

### Gap Closure

| Gap | Closure Evidence | Status |
|---|---|---|
| Embedded-NUL archive paths could produce self-inconsistent TES3 archives. | `10-06` added shared archive path rejection and regression coverage; focused and full suites passed. | RESOLVED |
| `overwrite_existing=false` lacked final no-replace publish protection. | TES3 writer routes non-overwrite final publish through `publish_file_without_replace`; helper and writer publish tests passed. | RESOLVED |

### Human Verification Required

None. Automated code inspection and CTest coverage are sufficient for this non-visual library phase. Broader game/tool compatibility validation remains scheduled for Phase 11.

## Result

Phase 10 passes verification. The public TES3/Morrowind writer can create archives from disk files or memory buffers, writer output reopens through the public reader, all BSA read/write families are covered through Phase 10, verification gaps from the initial report are resolved, and `TES5Edit/` remains untouched.

---

_Verified: 2026-05-10T02:31:29Z_
_Verifier: Codex_
