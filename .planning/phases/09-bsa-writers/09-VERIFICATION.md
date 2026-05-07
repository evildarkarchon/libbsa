---
phase: 09-bsa-writers
verified: 2026-05-07T07:21:23Z
status: gaps_found
score: 6/8 must-haves verified
overrides_applied: 0
gaps:
  - truth: "Consumer can create TES4-family BSA archives compatible with their target game engines."
    status: failed
    reason: "TES4-family writer serializes TotalFolderNameLength as the whole folder-block size, including file records, so generated archives carry an incompatible header value even though libbsa's own reader/tests accept them."
    artifacts:
      - path: "src/bsa_writer.cpp"
        issue: "Line 452 writes folder_blocks_size to header offset 24; the BSA header field is TotalFolderNameLength and must include only serialized folder-name records."
      - path: "tests/bsa_writer_tests.cpp"
        issue: "Read-after-write and byte-level tests do not assert header offset 24, allowing the compatibility defect to pass."
    missing:
      - "Track total serialized folder-name bytes separately from folder block bytes."
      - "Write the folder-name byte total to the TES4-family header field at offset 24."
      - "Add a byte-level writer test asserting header offset 24 excludes 16-byte file records."
---

# Phase 9: BSA Writers Verification Report

**Phase Goal:** Consumers can create TES4-family and TES3 BSA archives compatible with their target game engines.  
**Verified:** 2026-05-07T07:21:23Z  
**Status:** gaps_found  
**Re-verification:** No — initial verification

## Goal Achievement

Phase 09 is **not goal-complete**. Most writer API, TES3, compression, disk-input, and read-after-write behavior exists and is wired, but the review finding `CR-01` is confirmed in code: TES4-family archives are emitted with an incorrect native header field. Because target-engine compatibility is the central phase goal, this is a blocker even though the current libbsa read-back tests pass.

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Consumer can create TES4-family BSA archives for Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE from memory and disk inputs. | ✗ FAILED | API and implementation exist, but `src/bsa_writer.cpp:452` writes `folder_blocks_size` into the TES4 header `TotalFolderNameLength` field. `folder_blocks_size` includes folder-name records plus 16-byte file records, producing incompatible native headers. |
| 2 | Consumer can create TES3 Morrowind BSA archives with correct hash sorting and data-section-relative offsets. | ✓ VERIFIED | `src/bsa_writer.cpp:507-668` implements `tes3_morrowind`; records are sorted by `hash_tes3_path` then path, table bytes serialize high32/low32 hash order, and file records store data-section-relative offsets while plan entries expose archive-absolute offsets. Tests at `tests/bsa_writer_tests.cpp:334-399` cover sorting, offsets, hash bytes, read-back, and extraction. |
| 3 | Consumer can read back libbsa-written BSA archives and retrieve matching paths, metadata, and payload bytes. | ✓ VERIFIED | Focused spot-check passed: `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_bsa_writer_tests|libbsa.public_header_smoke"` ran 27/27 passing tests. Caveat: read-back is insufficient to prove external TES4 compatibility because libbsa's reader does not validate the bad folder-name-length header. |
| 4 | Maintainer can compare BSA writer sorting, flags, hashes, offsets, compression, and embedded-name decisions against target compatibility fixtures. | ✗ FAILED | Tests cover many byte fields, but they missed the compatibility-critical `TotalFolderNameLength` header field. The confirmed header defect demonstrates the current fixture/test comparison is incomplete for TES4-family native headers. |
| 5 | Public BSA writer API exposes explicit BSA targets, separate memory/disk inputs, BSA-native preview fields, and no private dependency/platform leakage. | ✓ VERIFIED | `include/libbsa/bsa_writer.hpp:20-142` defines explicit targets, separate `bsa_memory_entry`/`bsa_disk_entry`, preview regions/entries, plan/finalize APIs; public leakage grep over `include/libbsa` found no private dependency tokens. |
| 6 | TES4-family compression, embedded-name, and dedup semantics are implemented through semantic policies without raw flag exposure. | ✓ VERIFIED | `src/bsa_writer.cpp:256-321` resolves compression policy, routes codecs via `resolve_payload_codec`/`compress_payload`, writes embedded-name prefixes, and computes XOR size flags; `src/bsa_writer.cpp:410-437` deduplicates exact stored payload bytes. Tests at `tests/bsa_writer_tests.cpp:497-641` cover these behaviors. |
| 7 | Disk-backed BSA inputs read host bytes during planning and stay equivalent to memory-backed inputs. | ✓ VERIFIED | `src/bsa_writer.cpp:696-722` validates archive paths, reads each `host_path` into memory entries, then delegates to `plan_bsa_write`; tests at `tests/bsa_writer_tests.cpp:643-680` cover disk/memory equivalence and no finalization-time re-open. |
| 8 | Public smoke, documentation, validation, and TES5Edit boundary gates exist. | ✓ VERIFIED | `tests/public_header_smoke.cpp:22-49` and `:158-165` create/finalize/reopen/extract all four targets through public headers; README documents BSA-only support at `README.md:168-176`; `09-VALIDATION.md` has `nyquist_compliant: true`; `git status --short TES5Edit` returned empty. |

**Score:** 6/8 must-haves verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `include/libbsa/bsa_writer.hpp` | Public BSA writer contracts | ✓ VERIFIED | Exists, substantive, public API includes explicit target enum, memory/disk inputs, plan preview, and finalize API. |
| `include/libbsa/bsa.hpp` | BSA writer discoverability | ✓ VERIFIED | `#include <libbsa/bsa_writer.hpp>` present at line 6. |
| `src/bsa_writer.cpp` | Native TES3/TES4 BSA writer implementation | ✗ BLOCKER | Substantive and wired, but TES4 header serialization at line 452 is compatibility-wrong. |
| `tests/bsa_writer_tests.cpp` | Writer behavior and read-after-write tests | ⚠️ PARTIAL | 26 writer tests pass, but coverage misses `TotalFolderNameLength`, allowing incompatible TES4 headers. |
| `tests/public_header_smoke.cpp` | Consumer-style public compile/link/runtime coverage | ✓ VERIFIED | Public smoke writes/reopens TES3, v103, v104, and v105 archives. |
| `README.md` | Phase-scoped BSA writer documentation | ✓ VERIFIED | Documents TES3/TES4 writer support and defers BA2/DDS/CLI/GUI/performance/in-place mutation. |
| `.planning/phases/09-bsa-writers/09-VALIDATION.md` | Validation sign-off | ⚠️ PARTIAL | Exists and is marked green, but validation is contradicted by the confirmed missing TES4 header-field assertion. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `include/libbsa/bsa.hpp` | `include/libbsa/bsa_writer.hpp` | public include | ✓ WIRED | SDK key-link verification passed. |
| `CMakeLists.txt` | `src/bsa_writer.cpp` / public header / writer tests | explicit source/test wiring | ✓ WIRED | `CMakeLists.txt:55`, `:76`, and `:182-188` wire source, header, and test target. |
| `src/bsa_writer.cpp` | `src/hash.cpp` | TES4/TES3 hash emission | ✓ WIRED | `hash_tes4_path` and `hash_tes3_path` used in writer implementation. |
| `src/bsa_writer.cpp` | compression services | semantic compression routing | ✓ WIRED | `resolve_write_compression`, `resolve_payload_codec`, and `compress_payload` are called in `plan_tes4_entry`. |
| `plan_bsa_write_from_disk` | `plan_bsa_write` | disk bytes converted to memory entries during planning | ✓ WIRED | Manual check: `src/bsa_writer.cpp:696-722` reads host files into `bsa_memory_entry` values and delegates to `plan_bsa_write`. SDK could not verify this link because the `from` value was a symbol, not a file path. |
| writer tests | `open_bsa` / `extract_bsa_entry` | generated memory-source read-back | ✓ WIRED | Tests use read-after-write helpers at `tests/bsa_writer_tests.cpp:138-147`. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|---|---|---|---|---|
| `src/bsa_writer.cpp` TES4 writer | `plan.table_bytes`, `plan.data_regions`, `plan.entries` | Caller `bsa_memory_entry`/`bsa_disk_entry` payloads and normalized paths | Partially | ⚠️ HOLLOW_COMPAT — data flows and archives read back, but header field at offset 24 is populated from the wrong aggregate (`folder_blocks_size`). |
| `src/bsa_writer.cpp` TES3 writer | `plan.table_bytes`, `plan.data_regions`, `plan.entries` | Caller entries normalized and sorted by TES3 hash | Yes | ✓ FLOWING |
| `tests/public_header_smoke.cpp` | `bsa_writer_smoke_ok` | Public `plan_bsa_write` + `finalize_bsa_write` + `open_bsa` + `extract_bsa_entry` | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Focused writer and public smoke tests pass | `ctest --test-dir build/windows-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_bsa_writer_tests|libbsa.public_header_smoke"` | 27/27 tests passed in 0.49s | ✓ PASS |
| TES5Edit submodule remains untouched | `git status --short TES5Edit` | empty output | ✓ PASS |
| TES4 header compatibility field | Static check of `src/bsa_writer.cpp:370-452` | `folder_blocks_size` is accumulated from folder-name record + file records and written to header offset 24 | ✗ FAIL |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| WRT-01 | 09-01, 09-02, 09-03, 09-04, 09-06 | Consumer can create TES4-family BSA archives for Oblivion, FO3/FNV/Skyrim LE, and Skyrim SE/AE from disk paths or in-memory buffers. | ✗ BLOCKED | Memory/disk APIs and tests exist, but TES4-family output has an incompatible header field (`src/bsa_writer.cpp:452`), so target-engine-compatible creation is not verified. |
| WRT-04 | 09-01, 09-05, 09-06 | Consumer can create TES3 Morrowind BSA archives with correct hash sorting and data-section-relative offsets. | ✓ SATISFIED | TES3 writer implementation and tests cover hash sorting, name/hash table bytes, relative offsets, raw extraction, and unsupported options. |

No orphaned Phase 9 requirement IDs were found in `.planning/REQUIREMENTS.md`; Phase 9 maps only `WRT-01` and `WRT-04`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `src/bsa_writer.cpp` | 452 | Wrong compatibility-critical aggregate used for native header field | 🛑 Blocker | External TES4-family tools/engines can reject or misinterpret written archives. |
| `src/bsa_reader.cpp` | 416-419 | Review CR-02: declared `TotalFileNameLength` is range-checked but not used to bound name parsing | ⚠️ Warning | This does not directly prevent valid Phase 9 writer output from reading back, but it weakens malformed-input validation and can let read-back tests consume bytes outside the declared filename table. Should be fixed or explicitly assigned to Phase 11 hardening. |

### Human Verification Required

None. The blocking TES4 compatibility issue is directly observable in code and does not require human testing.

### Gaps Summary

The phase cannot close because TES4-family writer output is not target-compatible. The public API, disk/memory input path, compression/embed/dedup behavior, TES3 writer, tests, and docs are present, but `CR-01` from `09-REVIEW.md` is confirmed: the writer serializes the wrong value for `TotalFolderNameLength`. Existing read-after-write tests pass because they reopen through libbsa and do not assert this header field, so task completion and green tests do not prove the phase goal.

Fix the header aggregate and add a byte-level regression test. After that, re-run verification; the remaining review finding `CR-02` should be either fixed as adjacent malformed-input hardening or explicitly deferred to Phase 11 with a clear decision.

---

_Verified: 2026-05-07T07:21:23Z_  
_Verifier: the agent (gsd-verifier)_
