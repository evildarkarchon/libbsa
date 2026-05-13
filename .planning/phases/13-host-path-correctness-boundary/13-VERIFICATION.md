---
phase: 13-host-path-correctness-boundary
verified: 2026-05-13T10:23:25.1111097Z
status: passed
score: 6/6 must-haves verified
overrides_applied: 0
---

# Phase 13: Host Path Correctness Boundary Verification Report

**Phase Goal:** Consumers and maintainers can trust archive open and validation flows on Windows host paths that contain non-ASCII characters.
**Verified:** 2026-05-13T10:23:25.1111097Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Consumer can open representative supported BSA and BA2 archives from Windows host paths containing non-ASCII characters. | ✓ VERIFIED | `src/archive.cpp:109-191` resolves UTF-8 once with `detail::resolve_host_file_path(...)`, stores `detail::host_file_path` in reader state, and dispatches all representative parser families with that stored value. `tests/unit/host_path_correctness_boundary_tests.cpp:20,80-88,122-135,145-151` copies TES4 v103/v104/v105, FO4 BA2 GNRL, FO4 BA2 DX10, and Starfield BA2 GNRL archives into directory + filename paths containing `libbsa-Ångström-日本語`, then `archive_reader::open(...)` succeeds. `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` passed 2/2. |
| 2 | Consumer can validate representative supported BSA and BA2 archives from Windows host paths containing non-ASCII characters. | ✓ VERIFIED | `src/validation.cpp:172-211` has no duplicate readability preflight and routes setup through `archive_reader::open(host_path)`. `src/validation.cpp:139-166,206-207` still proves extractability through `reader.extract(...)`. `tests/unit/host_path_correctness_boundary_tests.cpp:125-135` runs `validate_archive(..., {.validate_entry_extractability = true})` on the same non-ASCII copied archive paths and asserts valid/no errors. Full and smoke ctest runs both passed. |
| 3 | Maintainer can run committed regression tests that prove non-ASCII host-path open and validate coverage for representative BSA and BA2 families. | ✓ VERIFIED | `tests/unit/host_path_correctness_boundary_tests.cpp:156-169` is a committed dedicated suite with smoke + full coverage. `tests/CMakeLists.txt:48-92,276-281` registers the suite in `libbsa_tests` and exposes it through `catch_discover_tests`. Both `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary_smoke --output-on-failure` and `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` passed. |
| 4 | Existing host-file helper behavior is delivered through one shared `host_file` boundary with no parallel `writer_disk_source` family left in source/test code. | ✓ VERIFIED | `src/detail/host_file.hpp:17-53` and `src/detail/host_file.cpp:66-200` provide the shared helper surface. `tests/unit/host_file_tests.cpp:57-167` covers exact/prefix/chunk/non-ASCII/error behavior under the new family. `glob src/detail/writer_disk_source.*` and `glob tests/unit/writer_disk_source_tests.cpp` returned no files. `rg "writer_disk_source" src tests/CMakeLists.txt tests/unit` returned no matches. |
| 5 | Open-time path ownership resolves UTF-8 once and migrated parser contracts consume stored `detail::host_file_path` rather than raw caller text. | ✓ VERIFIED | `src/detail/host_path.hpp:11-18` defines the shared `{original_utf8,resolved}` value; `src/detail/host_path.cpp:8-19` resolves the public UTF-8 string once into `std::filesystem::path`. `src/archive.cpp:29-34,114-191` stores that value in `archive_reader::state` and passes it into parser entry points. Parser headers and implementations now take `const detail::host_file_path&`: `src/formats/bsa/tes3_bsa_parser.hpp:27-29`, `tes4_bsa_parser.hpp:27-29`, `src/formats/ba2/ba2_gnrl_parser.hpp:27-29`, `ba2_dx10_parser.hpp:27-29`, with helper-based opens in `tes3_bsa_parser.cpp:349-360`, `tes4_bsa_parser.cpp:611-622`, `ba2_gnrl_parser.cpp:429-440`, and `ba2_dx10_parser.cpp:611-622`. |
| 6 | Post-open extraction reopens only from stored resolved host-path state through the shared boundary. | ✓ VERIFIED | `src/archive.cpp:86-100,242-285,351-352` routes extraction through `state_->host_path`. Reader contracts now take `const detail::host_file_path&` in `src/formats/bsa/tes3_bsa_reader.hpp:24-25`, `tes4_bsa_reader.hpp:23-25`, `src/formats/ba2/ba2_gnrl_reader.hpp:24-28`, and `ba2_dx10_reader.hpp:24-29`. Concrete reopens use `detail::open_host_file(...)` in `tes3_bsa_reader.cpp:27-43`, `tes4_bsa_reader.cpp:105-117`, `ba2_gnrl_reader.cpp:17-31,49-58`, and `ba2_dx10_reader.cpp:108-116`. |

**Score:** 6/6 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/detail/host_path.hpp`, `src/detail/host_path.cpp` | Shared host-path value and one-time resolver | ✓ VERIFIED | Defines `detail::host_file_path` with `original_utf8` + `resolved` and `resolve_host_file_path(...)` that converts the public UTF-8 path once before later I/O. |
| `src/detail/host_file.hpp`, `src/detail/host_file.cpp` | Shared host-file open/inspect/read/chunk boundary | ✓ VERIFIED | Neutral helper family exists, is substantive, and opens via `std::ifstream{host_path.resolved,...}` with caller-owned diagnostics. |
| `src/archive.cpp` | One-time resolution, stored-path ownership, detection, parser dispatch, extraction dispatch | ✓ VERIFIED | Reader state stores `detail::host_file_path`; open/extract paths are wired to the shared boundary. |
| `src/formats/bsa/tes3_bsa_parser.hpp/.cpp`, `src/formats/bsa/tes4_bsa_parser.hpp/.cpp` | BSA parser contracts consume shared stored path | ✓ VERIFIED | Headers take `const detail::host_file_path&`; implementations use `detail::open_host_file(...)`. |
| `src/formats/ba2/ba2_gnrl_parser.hpp/.cpp`, `src/formats/ba2/ba2_dx10_parser.hpp/.cpp` | BA2 parser contracts consume shared stored path | ✓ VERIFIED | Headers take `const detail::host_file_path&`; implementations use `detail::open_host_file(...)`. |
| `src/formats/bsa/tes3_bsa_reader.hpp/.cpp`, `src/formats/bsa/tes4_bsa_reader.hpp/.cpp`, `src/formats/ba2/ba2_gnrl_reader.hpp/.cpp`, `src/formats/ba2/ba2_dx10_reader.hpp/.cpp` | Extraction reopens through stored shared path | ✓ VERIFIED | Reader contracts migrated and all touched reopens are helper-based, not raw-string file opens. |
| `src/validation.cpp` | Validation setup unified behind `archive_reader::open` | ✓ VERIFIED | No `host_path_can_be_opened`; result/report split preserved; extractability still uses public extraction. |
| `tests/unit/host_file_tests.cpp` | Regression coverage for shared helper boundary | ✓ VERIFIED | Covers exact, prefix, chunk, non-ASCII, missing-source, change-detection, and allocation behavior. |
| `tests/unit/host_path_correctness_boundary_tests.cpp` | Dedicated representative non-ASCII regression suite | ✓ VERIFIED | Covers smoke and full representative matrix with non-ASCII directory + filename token and manifest-backed extraction checks. |
| `tests/CMakeLists.txt` | Suite registration in shipped test target | ✓ VERIFIED | Adds `unit/host_file_tests.cpp` and `unit/host_path_correctness_boundary_tests.cpp` to `libbsa_tests`; discovery stays on `catch_discover_tests`. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `src/detail/host_path.cpp` | `src/archive.cpp` | `resolve_host_file_path(...)` result stored in `archive_reader::state::host_path` | ✓ VERIFIED | `src/archive.cpp:114-191` resolves once and persists the returned `detail::host_file_path`. |
| `src/archive.cpp` | Parser entry points | `parse_*_archive_file(const detail::host_file_path&, ...)` | ✓ VERIFIED | Archive open dispatch calls the migrated parser signatures for TES3, TES4, BA2 GNRL, and BA2 DX10. |
| Parser implementations | `src/detail/host_file.hpp` | `detail::open_host_file(...)` | ✓ VERIFIED | All touched parser `.cpp` files now obtain their input streams from the shared helper. |
| `src/archive.cpp` | Reader entry points | `extract_*payload(const detail::host_file_path&, ...)` | ✓ VERIFIED | Extraction dispatch carries only `state_->host_path` after open succeeds. |
| Reader implementations | `src/detail/host_file.hpp` | `detail::open_host_file(...)` | ✓ VERIFIED | TES3, TES4, BA2 GNRL, and BA2 DX10 readers reopen through the shared helper. |
| `src/validation.cpp` | `src/archive.cpp` | `archive_reader::open(...)` plus `reader.extract(...)` | ✓ VERIFIED | Validation setup uses the same boundary as normal opens and extractability reuses public extraction. |
| `tests/unit/host_path_correctness_boundary_tests.cpp` | Generated manifests + public API | non-ASCII copy + `validate_archive` + `extract_bytes` | ✓ VERIFIED | Suite reads committed manifests, copies only the archive under test, and asserts canonical payload bytes via the public API. |
| `tests/CMakeLists.txt` | CTest discovery | `catch_discover_tests(libbsa_tests ...)` | ✓ VERIFIED | The dedicated suite is discoverable and runnable by tag/filter. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/archive.cpp` | `resolved_host_path` / `state_->host_path` | `detail::resolve_host_file_path(host_path, ...)` | Yes — stored value feeds detection prefix, file-size inspection, parser dispatch, and later extraction dispatch. | ✓ FLOWING |
| `src/validation.cpp` | `opened` / extractability result | `archive_reader::open(host_path)` then `reader.extract(entry.path, sink)` | Yes — readable archives become parsed metadata + streamed payload checks; setup failures stay result-level. | ✓ FLOWING |
| `tests/unit/host_path_correctness_boundary_tests.cpp` | `copied_archive_utf8` / manifest-backed expected bytes | `copy_archive_to_non_ascii_path(...)` and committed `*_manifest.json` files | Yes — same copied non-ASCII path is used for `open`, `validate_archive`, and `extract_bytes`, then compared to committed expected payload bytes. | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Smoke proof for non-ASCII open/validate/extract | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary_smoke --output-on-failure` | 1/1 test passed | ✓ PASS |
| Full representative non-ASCII matrix | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` | 2/2 tests passed | ✓ PASS |
| Shared boundary + archive/validation regression lane | `ctest --preset windows-msvc-debug-static -R "validation_api|archive_reader|bulk_extraction|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10_extraction|ba2_dx10_parser|host_file" --output-on-failure` | 33/33 tests passed | ✓ PASS |
| Forbidden old path patterns absent | `rg "std::ifstream input\{std::string\{host_path\}|host_path_can_be_opened|writer_disk_source" src tests/CMakeLists.txt tests/unit` | no output | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
| --- | --- | --- | --- |
| No phase probes declared or discovered | — | `scripts/` directory absent; no `probe-*.sh` files found | ? SKIP |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| `HOST-01` | `13-01`, `13-02`, `13-03`, `13-04` | Consumer can open supported archives from Windows host paths containing non-ASCII characters. | ✓ SATISFIED | Shared boundary exists (`src/detail/host_file.*`, `src/detail/host_path.*`); open resolves and stores path once (`src/archive.cpp:109-191`); representative non-ASCII open suite passes for TES4 v103/v104/v105, FO4 BA2 GNRL, FO4 BA2 DX10, and Starfield BA2 GNRL (`tests/unit/host_path_correctness_boundary_tests.cpp:145-169`). |
| `HOST-02` | `13-01`, `13-02`, `13-03`, `13-04` | Consumer can validate supported archives from Windows host paths containing non-ASCII characters. | ✓ SATISFIED | Validation now trusts `archive_reader::open` and reuses public extraction (`src/validation.cpp:172-211`); the dedicated non-ASCII suite runs `validate_archive(..., {.validate_entry_extractability = true})` and passes for the representative matrix (`tests/unit/host_path_correctness_boundary_tests.cpp:125-135,145-169`). |
| `HOST-03` | `13-04` | Maintainer can verify non-ASCII host-path open and validate coverage through committed regression tests for representative BSA and BA2 families. | ✓ SATISFIED | Committed dedicated suite exists, is registered in `tests/CMakeLists.txt`, and passes under both smoke and full filters. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| — | — | No blocker debt markers, placeholder strings, or orphaned old helper files found in phase-owned code. Reviewed `return {}` grep hits were normal `result<void>` success returns, not stubs. | ℹ️ Info | No action required. |

### Human Verification Required

None.

### Gaps Summary

No gaps found. The codebase now routes archive open, validation, parser entry, and post-open extraction through one shared Windows-correct host-file boundary; the old helper family is removed from source and tests; and a committed non-ASCII regression suite proves the representative TES4 and BA2 matrix through the public API surface.

---

_Verified: 2026-05-13T10:23:25.1111097Z_
_Verifier: the agent (gsd-verifier)_
