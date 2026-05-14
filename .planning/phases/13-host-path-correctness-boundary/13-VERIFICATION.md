---
phase: 13-host-path-correctness-boundary
verified: 2026-05-13T17:06:11.7098704-07:00
status: passed
score: 10/10 must-haves verified
overrides_applied: 0
---

# Phase 13: Host Path Correctness Boundary Verification Report

**Phase Goal:** Consumers and maintainers can trust archive open and validation flows on Windows host paths that contain non-ASCII characters.
**Verified:** 2026-05-13T17:06:11.7098704-07:00
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | The neutral shared `host_file` seam exists, is build-registered, and preserves the expected helper/error behavior used as Phase 13's base boundary. | ✓ VERIFIED | `src/detail/host_file.hpp`, `src/detail/host_file.cpp`, `CMakeLists.txt:105-106`, `tests/unit/host_file_tests.cpp`, `tests/unit/host_file_writer_name_tests.cpp`; `ctest --preset windows-msvc-debug-static -R "host_file|tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer"` passed 25/25. |
| 2 | The shared `host_file_path` contract resolves caller UTF-8 text once and migrated helper/writer-prep callers perform actual I/O through the resolved filesystem path. | ✓ VERIFIED | `src/detail/host_file_path.hpp`, `src/detail/host_file_path.cpp:5-7`, `src/detail/host_file.cpp:76-77,94-96,130-146,180-234`, `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/bsa/tes4_bsa_layout.cpp:57-59,111-148`, `src/formats/ba2/ba2_gnrl_prepare.cpp:52-109`, `src/formats/ba2/ba2_dx10_prepare.cpp`; focused writer/helper tests passed. |
| 3 | `archive_reader::open` still exposes the public UTF-8 `std::string_view` API, but now resolves and stores one shared host-file path value at open time. | ✓ VERIFIED | Public API unchanged in `include/libbsa/archive.hpp:253`; open-time resolution/storage in `src/archive.cpp:30-35,113-205`. |
| 4 | Parser entry points no longer reopen archives through raw narrow host-path text; parser metadata opens use the shared resolved-path boundary. | ✓ VERIFIED | `src/formats/bsa/tes3_bsa_parser.hpp`, `src/formats/bsa/tes4_bsa_parser.hpp:27-29`, `src/formats/ba2/ba2_gnrl_parser.hpp`, `src/formats/ba2/ba2_dx10_parser.hpp`; parser implementations call `detail::open_host_file(...)` in `src/formats/bsa/tes3_bsa_parser.cpp:349-360`, `src/formats/bsa/tes4_bsa_parser.cpp:611-622`, `src/formats/ba2/ba2_gnrl_parser.cpp:429-440`, `src/formats/ba2/ba2_dx10_parser.cpp:611-622`. |
| 5 | After open succeeds, follow-on extraction reopens use the stored resolved host path rather than reinterpreting caller text. | ✓ VERIFIED | `src/archive.cpp:91-104,256-300` passes `state_->host_path` into extraction dispatch; reader seams consume `detail::host_file_path` in `src/formats/bsa/tes3_bsa_reader.hpp`, `src/formats/bsa/tes4_bsa_reader.hpp:23-26`, `src/formats/ba2/ba2_gnrl_reader.hpp`, `src/formats/ba2/ba2_dx10_reader.hpp`; implementations reopen through `detail::open_host_file(...)`. |
| 6 | `validate_archive` now reuses `archive_reader::open` as the single setup path and preserves result-level setup failures versus report-level malformed-archive diagnostics. | ✓ VERIFIED | `include/libbsa/validation.hpp:114-122` keeps the public API unchanged; `src/validation.cpp:172-214` delegates setup to `archive_reader::open(host_path)` and uses `reader.extract(...)` for extractability; `ctest --preset windows-msvc-debug-static -R "validation_api"` passed 6/6. |
| 7 | Consumers can open the representative TES4 and BA2 archives from Windows host paths containing non-ASCII directory and filename segments. | ✓ VERIFIED | `tests/unit/host_path_correctness_boundary_tests.cpp:21-22,100-129,185-190,247-254` builds non-ASCII directory+filename paths and calls `archive_reader::open`; `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary"` passed 5/5. |
| 8 | Consumers can validate those same representative archives from non-ASCII host paths with `validate_entry_extractability = true`. | ✓ VERIFIED | `tests/unit/host_path_correctness_boundary_tests.cpp:192-201,247-254` calls `validate_archive(host_path, options)` with `options.validate_entry_extractability = true`; focused suite passed 5/5. |
| 9 | Each representative archive can still perform one canonical manifest-backed extraction after the non-ASCII-path open succeeds. | ✓ VERIFIED | `tests/unit/host_path_correctness_boundary_tests.cpp:175-212,247-254` checks `extract(...)` and `extract_bytes(...)` against manifest-backed expected bytes for each representative case; focused suite passed 5/5. |
| 10 | Maintainers can run one committed deterministic regression suite that proves the Phase 13 public open/validate/extract story. | ✓ VERIFIED | Suite file `tests/unit/host_path_correctness_boundary_tests.cpp`; registered in `tests/CMakeLists.txt:89-91`; `ctest --preset windows-msvc-debug-static -N -R "host_path_correctness_boundary|host_path_correctness_boundary_smoke"` lists 5 tests; focused execution passed 5/5. |

**Score:** 10/10 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/detail/host_file.hpp` + `src/detail/host_file.cpp` | Neutral shared host-file helper seam | ✓ VERIFIED | Exists, substantive helper surface, used by reader/parser/writer-prep call sites. |
| `src/detail/host_file_path.hpp` + `src/detail/host_file_path.cpp` | Shared internal path contract | ✓ VERIFIED | Exists, resolves UTF-8 input once, consumed by helper overloads and reader state. |
| `src/archive.cpp` | Open-time path resolution/storage and extraction dispatch reuse | ✓ VERIFIED | Stores `detail::host_file_path`, uses helper prefix/size probes, dispatches extraction with stored path. |
| `src/validation.cpp` | Single setup path through `archive_reader::open` | ✓ VERIFIED | Duplicate readability preflight removed; extractability still uses public extraction path. |
| Parser headers/sources (`tes3`, `tes4`, `ba2_gnrl`, `ba2_dx10`) | Parser file-open boundary migrated to resolved path contract | ✓ VERIFIED | Headers accept `detail::host_file_path`; sources reopen with `detail::open_host_file`. |
| Reader headers/sources (`tes3`, `tes4`, `ba2_gnrl`, `ba2_dx10`) | Follow-on extraction boundary migrated to stored path contract | ✓ VERIFIED | Headers accept `detail::host_file_path`; sources reopen with `detail::open_host_file`. |
| `tests/unit/host_path_correctness_boundary_tests.cpp` | Dedicated non-ASCII cross-family regression suite | ✓ VERIFIED | Covers representative matrix, validation with extractability, and canonical extraction. |
| `tests/CMakeLists.txt` | Test registration for focused selectors | ✓ VERIFIED | `libbsa_tests` includes `unit/host_path_correctness_boundary_tests.cpp`. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `archive_reader::open` | `detail::host_file_path` | `detail::resolve_host_file_path(host_path)` | WIRED | `src/archive.cpp:118-121`. |
| `archive_reader::open` | host-file reads | `read_host_file_prefix` + `inspect_host_file_size` | WIRED | `src/archive.cpp:124-145,177-180`. |
| Parser archive-file entry points | shared host-file seam | `detail::open_host_file(host_path, ...)` | WIRED | Present in all four parser `.cpp` files. |
| Opened reader state | extraction dispatch | `state_->host_path` passed into `extract_entry_payload(...)` | WIRED | `src/archive.cpp:272,296`. |
| Extraction dispatch | concrete readers | `extract_tes3_bsa_payload` / `extract_tes4_bsa_payload_from_file` / `extract_ba2_*_payload` | WIRED | `src/archive.cpp:96-104`. |
| `validate_archive` | `archive_reader::open` | direct setup delegation | WIRED | `src/validation.cpp:177-187`. |
| extractability validation | public extraction path | `reader.extract(entry.path, sink)` | WIRED | `src/validation.cpp:139-165`. |
| Non-ASCII copied test path | public API proof | `archive_reader::open`, `validate_archive`, `extract`, `extract_bytes` | WIRED | `tests/unit/host_path_correctness_boundary_tests.cpp:181-211`. |
| Test registration | runnable focused lane | `tests/CMakeLists.txt` + CTest discovery | WIRED | `tests/CMakeLists.txt:89-91`; `ctest -N -R ...` lists 5 tests. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/archive.cpp` | `resolved_host_path` | `detail::resolve_host_file_path(host_path)` | Yes — flows into detection prefix reads, file-size probe, and parser entry points | ✓ FLOWING |
| `src/archive.cpp` + reader sources | `state_->host_path` | Stored in `archive_reader::open`, then passed into concrete extraction helpers | Yes — follow-on extraction reopens the real archive path through `detail::open_host_file` | ✓ FLOWING |
| `src/validation.cpp` | `opened` / `reader.extract(...)` | `archive_reader::open(host_path)` | Yes — validation extractability exercises real payload reads, not a stub path | ✓ FLOWING |
| `tests/unit/host_path_correctness_boundary_tests.cpp` | `host_path` | `utf8_string_from_path(copied_archive)` from actual copied non-ASCII filesystem path | Yes — drives `open`, `validate_archive`, `extract`, and `extract_bytes` against committed fixtures | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Dedicated non-ASCII proof suite runs | `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary" --output-on-failure` | 5/5 tests passed | ✓ PASS |
| Validation contract remains green | `ctest --preset windows-msvc-debug-static -R "validation_api" --output-on-failure` | 6/6 tests passed | ✓ PASS |
| Shared helper / migrated-writer seam still works after rename and contract changes | `ctest --preset windows-msvc-debug-static -R "host_file|tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer" --output-on-failure` | 25/25 tests passed | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
| --- | --- | --- | --- |
| None declared or discovered | `glob scripts/**/probe-*.sh` + phase artifact grep | No probe scripts found; no probe references found in Phase 13 planning artifacts | SKIP |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| HOST-01 | `13-01-PLAN.md`, `13-02-PLAN.md`, `13-03-PLAN.md`, `13-04-PLAN.md`, `13-05-PLAN.md` | Consumer can open supported archives from Windows host paths containing non-ASCII characters. | ✓ SATISFIED | `src/archive.cpp` resolves/stores host paths once; parser and reader seams consume `detail::host_file_path`; `tests/unit/host_path_correctness_boundary_tests.cpp` proves representative TES4/BA2 opens from non-ASCII paths; focused CTest lane passed. |
| HOST-02 | `13-04-PLAN.md`, `13-05-PLAN.md` | Consumer can validate supported archives from Windows host paths containing non-ASCII characters. | ✓ SATISFIED | `src/validation.cpp` delegates setup to `archive_reader::open` and uses `reader.extract(...)`; dedicated suite validates each representative archive with `validate_entry_extractability = true`; `validation_api` and focused host-path suites passed. |
| HOST-03 | `13-05-PLAN.md` | Maintainer can verify non-ASCII host-path open and validate coverage through committed regression tests for representative BSA and BA2 families. | ✓ SATISFIED | `tests/unit/host_path_correctness_boundary_tests.cpp` is committed, registered in `tests/CMakeLists.txt`, discoverable by focused selectors, and passed in this verification run. |

Phase 13 requirement IDs from plan frontmatter were fully accounted for: `HOST-01`, `HOST-02`, `HOST-03`.
No orphaned Phase 13 requirement IDs were found in `.planning/REQUIREMENTS.md`.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `src/detail/host_file_path.hpp`, `src/detail/host_file.cpp`, `src/archive.cpp` | `11-21`, `76-77`, `33-34` | `original_utf8` is stored and documented as diagnostics-only state, but current helper overloads only forward `resolved` and no current diagnostic uses `original_utf8`. | ⚠️ Warning | Dead-state/comment-drift concern. This does not falsify the Phase 13 open/validate goal because actual I/O uses the resolved path and the non-ASCII regression suite passes, but the review's advisory cleanup point is valid. |
| `src/formats/ba2/ba2_gnrl_layout.cpp`, `src/formats/ba2/ba2_gnrl_serialize.cpp`, `src/formats/bsa/tes3_bsa_serialize.cpp`, `src/formats/bsa/tes4_bsa_serialize.cpp` | `162-219`, `94-124`, `71-104`, `71-105` | Writer-side finalize/dedupe paths still use raw `std::ifstream{host_path, ...}` reopen logic. | ⚠️ Warning | Real future-scope issue for writer-side non-ASCII disk sources. It does **not** block Phase 13 because `13-SPEC.md:46-52` explicitly excludes writer-side host-path correctness and the locked goal is read/open/validate only. |

### Human Verification Required

None.

### Gaps Summary

None for the locked Phase 13 goal.

The advisory review's writer-side migration finding does not overturn Phase 13 goal achievement: the phase contract is explicitly limited to archive **open**, **validation**, and **post-open read/extraction** flows, and those are wired, exercised, and passing from non-ASCII Windows host paths in the committed focused regression suite. The `original_utf8` dead-state concern is real cleanup debt, but it is not evidence that open or validation correctness failed.

---

_Verified: 2026-05-13T17:06:11.7098704-07:00_
_Verifier: the agent (gsd-verifier)_
