# Phase 13: Host Path Correctness Boundary - Research

**Researched:** 2026-05-13 [VERIFIED: system date]
**Domain:** Windows host-filesystem boundary hardening for archive open, validation, and read-side extraction in a C++20 library [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
**Confidence:** HIGH [VERIFIED: codebase inspection + Microsoft Learn filesystem docs]

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Generalize the existing `src/detail/writer_disk_source.*` boundary into a neutrally named shared host-file helper in `src/detail/` rather than adding a parallel read-only helper. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-02:** Rename the helper and its types now so the names match the new shared read/write role, and migrate existing writer call sites in the same phase instead of leaving compatibility shims behind. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-03:** The shared boundary must cover all read-side host-file opens touched by the Phase 13 read/validate flow, including detection-prefix reads, size probes, parser opens, validation setup, and payload extraction opens. Do not limit the cleanup to only the representative test path. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-04:** The helper surface should be layered: small neutral primitives plus focused helpers for common patterns like size inspection, prefix reads, exact reads, and bounded chunk iteration. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-05:** Callers should continue to own stream lifetime locally, but they must obtain read-side streams through the shared helper. The helper should hand back `std::ifstream` streams, not introduce a new custom file wrapper in Phase 13. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-06:** Error reporting stays context-driven. Shared mechanics are centralized, but callers still supply operation-specific diagnostics so public/read-side messages remain stable and phase-appropriate. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-07:** Even though TES3 non-ASCII regression coverage is out of scope, TES3 read-side narrow opens should migrate to the same shared host-file boundary while the refactor is in flight so the repo actually lands on one read-side policy boundary. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-08:** Keep the public API unchanged: callers still pass UTF-8 host paths as `std::string_view`. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-09:** Resolve the UTF-8 host path into the Windows-correct filesystem path once at the public API boundary, not lazily per helper call and not once per later operation. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-10:** Store both forms internally after open succeeds: the original UTF-8 text for diagnostics/traceability and one resolved `std::filesystem::path` for actual host-file I/O. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-11:** Represent those two forms as one small shared internal path value in `src/detail/` instead of widening signatures with separate text/path parameters or keeping subsystem-specific copies. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-12:** After open succeeds, the resolved filesystem path is the only I/O source. The original UTF-8 text is diagnostics-only and must not become a fallback path-opening route. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-13:** Follow-on extraction reads must reopen from the path stored in `archive_reader::state`; they must not re-resolve raw caller text elsewhere. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-14:** Open, validation, and migrated writer call sites should converge on the same shared internal path type. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-15:** Remove `validate_archive`'s duplicate readability preflight and trust the shared host-file boundary plus `archive_reader::open` as the one real setup path. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-16:** Preserve the existing public result/report split. Empty input still fails fast with `invalid_argument`; unreadable or non-openable host paths still fail as direct `io_error` results; readable malformed archive bytes still become `validation_report` diagnostics. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-17:** Unreadable host-path failures should surface the shared helper/open diagnostics directly rather than being reworded through a validation-specific preflight layer. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-18:** If setup fails before parsing any archive bytes, `validate_archive` should return a failed `result<validation_report>` with no partial report object. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-19:** `validate_entry_extractability` must continue to prove payload reads through the opened reader state and the public extraction path. Do not add a validation-only host-path or extraction codepath. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-20:** Phase 13 tests should prove the validation cleanup only through black-box public behavior. Do not add white-box hooks or open-count assertions that couple tests to the exact internal call graph. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-21:** Add one dedicated cross-family Phase 13 regression suite for the non-ASCII host-path proof instead of scattering the main story across existing per-format tests. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-22:** The suite should cover all three existing TES4 BSA fixtures (`v103`, `v104`, `v105`) plus the locked representative BA2 fixtures for Fallout 4 GNRL, Fallout 4 DX10, and Starfield GNRL. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-23:** Copy only the archive-under-test into the non-ASCII temp location. Keep manifests and expected payload data in the normal generated-fixture tree. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-24:** The non-ASCII proof must cover both a non-ASCII directory and a non-ASCII archive filename, not just one path segment. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-25:** Use one stable curated naming token across the suite: `libbsa-Ångström-日本語`. Keep the token deterministic and BMP-only rather than using a broader script matrix or random names. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-26:** For the post-open payload proof, each representative archive only needs one canonical manifest-backed extraction target with expected bytes. Do not rerun every manifest entry from the non-ASCII path in this dedicated suite. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **D-27:** The suite should stay black-box and deterministic: prove `archive_reader::open`, `validate_archive(..., {.validate_entry_extractability = true})`, and the canonical extraction result from the public API surface only. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

### the agent's Discretion
None. The implementation shape is intentionally locked for downstream research and planning. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| HOST-01 | Consumer can open supported archives from Windows host paths containing non-ASCII characters. [VERIFIED: .planning/REQUIREMENTS.md] | Resolve UTF-8 once at `archive_reader::open`, store a shared internal `{utf8_text, filesystem_path}` value, and route detection, size probing, and parser opens through the renamed shared host-file helper. [VERIFIED: src/archive.cpp; VERIFIED: src/detail/writer_disk_source.cpp; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170] |
| HOST-02 | Consumer can validate supported archives from Windows host paths containing non-ASCII characters. [VERIFIED: .planning/REQUIREMENTS.md] | Delete validation's duplicate readability preflight and let `validate_archive` reuse `archive_reader::open` plus the stored resolved path for extractability reads. [VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| HOST-03 | Maintainer can verify non-ASCII host-path open and validate coverage through committed regression tests for representative BSA and BA2 families. [VERIFIED: .planning/REQUIREMENTS.md] | Add one dedicated Catch2 suite that copies only the tested archive into a temp directory and filename containing `libbsa-Ångström-日本語`, then proves open, validate, and one manifest-backed extraction for TES4 v103/v104/v105, FO4 BA2 GNRL, FO4 BA2 DX10, and Starfield BA2 GNRL. [VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/fixtures/generated/archives/*.json; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
</phase_requirements>

## Summary

Phase 13 is a Windows host-filesystem boundary hardening phase, not a public API redesign and not an archive-internal path semantics phase. The codebase already separates archive-internal virtual paths through `detail::archive_path`, but the read side still opens host files via repeated `std::ifstream{std::string{host_path}}` calls in `src/archive.cpp`, `src/validation.cpp`, TES3/TES4 parser files, and BA2 reader/parser files. That is the bug surface this phase must collapse into one shared internal boundary. [VERIFIED: src/detail/archive_path.hpp; VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp; VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp; VERIFIED: src/formats/bsa/tes3_bsa_reader.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp]

The strongest reusable asset is `src/detail/writer_disk_source.*`, which already centralizes host-path opening through `std::filesystem::path`, file sizing, exact reads, prefix reads, and chunk iteration while preserving caller-supplied diagnostics. Microsoft documents that on Windows `std::filesystem::path` stores paths in native `wchar_t` form, accepts UTF-8 input, and can be used wherever `<fstream>` filename arguments are accepted. That means the phase should generalize the existing helper rather than inventing a new conversion layer or changing public signatures. [VERIFIED: src/detail/writer_disk_source.hpp; VERIFIED: src/detail/writer_disk_source.cpp; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170; CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths; CITED: https://learn.microsoft.com/cpp/standard-library/path-class?view=msvc-170#pathpath]

Testing should stay black-box. The repo already uses Catch2, manifest-backed fixtures, temp-path helpers, and committed TES4/BA2 archives. The missing piece is one dedicated cross-family regression suite that copies only the archive under test to a non-ASCII temp directory and non-ASCII filename, then proves open, validate-with-extractability, and one canonical payload extraction per representative archive family. [VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp; VERIFIED: tests/unit/validation_api_tests.cpp; VERIFIED: tests/fixtures/generated/archives/tes4_v103_manifest.json; VERIFIED: tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json; VERIFIED: tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json; VERIFIED: tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json]

**Primary recommendation:** Rename and generalize `writer_disk_source` into the single shared host-file boundary, resolve UTF-8 to `std::filesystem::path` once in `archive_reader::open`, store `{original_utf8, resolved_path}` in reader state, remove validation's duplicate readability preflight, and prove the fix with one dedicated non-ASCII cross-family regression suite. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md; VERIFIED: src/detail/writer_disk_source.cpp; VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp]

## Project Constraints (from AGENTS.md)

- Windows-only scope; do not add Linux/macOS/POSIX portability work. [VERIFIED: AGENTS.md]
- `TES5Edit/` is read-only reference material and must not be edited, formatted, staged, compiled into the project, or used as mutable fixtures. [VERIFIED: AGENTS.md]
- Keep implementation in C++ with reusable library-oriented interfaces, not app-specific tooling. [VERIFIED: AGENTS.md]
- Preserve BSArchPro-discovered behavior unless there is a documented reason to diverge, and record non-obvious compatibility constraints near ported code. [VERIFIED: AGENTS.md]
- Do not add speculative dependencies; prefer the standard library unless a real requirement justifies more. [VERIFIED: AGENTS.md]
- Use `libdeflate`, official `lz4`, `DirectXTex`, and `vcpkg` for the already-approved dependency set. [VERIFIED: AGENTS.md; VERIFIED: vcpkg.json]
- Never delete accurate comments as cleanup; add comments for non-obvious why; add Doxygen comments for public APIs and substantially rewritten methods. [VERIFIED: AGENTS.md]
- Add focused fixture-based tests for parsing, writing, round-tripping, and compatibility behavior; do not keep production code solely for test compatibility. [VERIFIED: AGENTS.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Public UTF-8 host-path acceptance | API / Backend [ASSUMED] | — | `archive_reader::open(std::string_view)` and `validate_archive(std::string_view, ...)` are the public seam where caller text enters the library. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: include/libbsa/validation.hpp] |
| UTF-8 → Windows-native path resolution | API / Backend [ASSUMED] | Shared detail services [ASSUMED] | D-09 locks one-time resolution at the public boundary, while D-11 locks the shared internal path value into `src/detail/`. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Shared host-file open/size/prefix/exact/chunk primitives | Shared detail services [VERIFIED: .planning/codebase/ARCHITECTURE.md] | — | `writer_disk_source` already occupies this layer and is the direct generalization target. [VERIFIED: src/detail/writer_disk_source.hpp; VERIFIED: src/detail/writer_disk_source.cpp] |
| Archive detection and parser dispatch | API / Backend [VERIFIED: .planning/codebase/ARCHITECTURE.md] | Format pipeline [VERIFIED: .planning/codebase/ARCHITECTURE.md] | `src/archive.cpp` performs prefix read + family dispatch, then hands parsing to format modules. [VERIFIED: src/archive.cpp] |
| Parsed metadata and stored resolved host path ownership | API / Backend [ASSUMED] | Shared detail services [ASSUMED] | `archive_reader::state` owns opened archive state today and is the correct place to add the resolved path value without broad public API change. [VERIFIED: src/archive.cpp] |
| Follow-on payload extraction reopen | Format pipeline [VERIFIED: .planning/codebase/ARCHITECTURE.md] | Shared detail services [VERIFIED: .planning/codebase/ARCHITECTURE.md] | Reader modules reopen the host archive today; Phase 13 should keep extraction in the format readers but source their `std::ifstream` instances from the shared helper. [VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/bsa/tes3_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp] |
| Validation setup and extractability proof | API / Backend [VERIFIED: .planning/codebase/ARCHITECTURE.md] | Format pipeline [VERIFIED: .planning/codebase/ARCHITECTURE.md] | `validate_archive` is the public orchestrator; extractability must continue to flow through public extraction on the opened reader. [VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Non-ASCII host-path regression suite | API / Backend tests [ASSUMED] | Shared fixtures [VERIFIED: .planning/codebase/TESTING.md] | The suite proves public behavior and reuses committed manifests/archives from `tests/fixtures/generated/archives/`. [VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/fixtures/generated/archives/*.json] |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++20 + `std::filesystem::path` | C++20 project baseline via presets [VERIFIED: CMakePresets.json] | One-time Windows-correct host-path resolution and stored native path ownership [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170] | Microsoft documents that Windows paths are stored natively in Unicode, `path` accepts UTF-8 input, and `path` objects can be passed to `<fstream>` filename arguments. [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170; CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths] |
| Renamed shared `src/detail/*` host-file helper (generalized from `writer_disk_source`) | Existing internal module pattern, no new dependency [VERIFIED: src/detail/writer_disk_source.hpp; VERIFIED: src/detail/writer_disk_source.cpp] | Centralize open/size/prefix/exact/chunk mechanics for both read-side and writer-side host-file access [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | The codebase already has the right abstraction shape and diagnostics model; Phase 13 is locked to generalize it instead of adding a parallel helper. [VERIFIED: src/detail/writer_disk_source.hpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| `libbsa::result<T>` + caller-supplied diagnostics | Existing public/internal error model [VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp] | Preserve stable `io_error` / `invalid_argument` / `format_error` behavior while sharing file-open mechanics [VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp] | D-06, D-16, and D-17 explicitly lock centralized mechanics with caller-owned message context and stable result/report semantics. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `std::ifstream` path overloads | Standard library behavior documented by Microsoft [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170] | Keep stream lifetime local while opening via resolved `std::filesystem::path` [VERIFIED: src/detail/writer_disk_source.cpp] | Use in shared helper primitives and keep format-specific seeking/streaming logic in existing parser/reader files. [VERIFIED: src/detail/writer_disk_source.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp] |
| Catch2 3 + CTest discovery | Manifest-managed and wired in `tests/CMakeLists.txt` [VERIFIED: vcpkg.json; VERIFIED: tests/CMakeLists.txt] | Dedicated Phase 13 regression suite with tag-based focused execution [VERIFIED: tests/CMakeLists.txt] | Use for the new cross-family non-ASCII suite and for keeping the suite black-box and discoverable through CTest labels. [VERIFIED: tests/CMakeLists.txt; VERIFIED: .planning/codebase/TESTING.md] |
| `nlohmann-json` manifests | Manifest-managed and already used in fixture tests [VERIFIED: vcpkg.json; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp] | Reuse expected bytes and canonical entry names from committed manifests instead of duplicating expectations in the new suite [VERIFIED: tests/fixtures/generated/archives/*.json] | Use for canonical extraction target lookup and byte assertions in the dedicated Phase 13 suite. [VERIFIED: tests/fixtures/generated/archives/tes4_v103_manifest.json; VERIFIED: tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json; VERIFIED: tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json; VERIFIED: tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Shared internal host-file boundary [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | Fix each narrow `std::ifstream{std::string{host_path}}` call separately [VERIFIED: grep over `src/**/*.cpp`] | Repeating conversions would keep Windows correctness non-uniform and violate D-03/D-14. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Public UTF-8 `std::string_view` API [VERIFIED: include/libbsa/archive.hpp; VERIFIED: include/libbsa/validation.hpp] | Public `std::filesystem::path` overloads [ASSUMED] | D-08 and the phase boundary explicitly forbid public API expansion in v1.1. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-SPEC.md] |
| Local `std::ifstream` ownership returned from shared helper [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | New custom file wrapper class [ASSUMED] | D-05 explicitly forbids introducing a new wrapper in Phase 13. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Black-box public regression suite [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | White-box call-count hooks or test-only instrumentation [ASSUMED] | D-20 and D-27 explicitly reject white-box coupling. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |

**Installation:**
```bash
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static --output-on-failure
```
[VERIFIED: README.md; VERIFIED: CMakePresets.json]

**Version verification:** The repo pins dependency selection through committed vcpkg manifest mode and a committed registry baseline rather than ad-hoc local installs. `vcpkg.json` lists `libdeflate`, `lz4`, `directxtex`, `catch2`, and `nlohmann-json`; `vcpkg-configuration.json` commits baseline `12dcccadfe573d0eaa6c67a968413ded7805d256`. [VERIFIED: vcpkg.json; VERIFIED: vcpkg-configuration.json]

## Architecture Patterns

### System Architecture Diagram

```text
Caller UTF-8 host_path (`std::string_view`)
        |
        v
archive_reader::open / validate_archive
        |
        +--> reject empty input early (`invalid_argument`)
        |
        v
resolve once to internal host-file value
{ original_utf8_text, resolved std::filesystem::path }
        |
        +--> shared host-file helper: open / inspect size / read prefix / read exact / iterate chunks
        |          |
        |          +--> uses std::ifstream(path) and caller-supplied diagnostics
        |
        +--> read detection prefix --> format detector --> parser entry point
        |                                   |
        |                                   +--> metadata + entries
        |
        v
archive_reader::state
{ metadata, entries, original utf8 text, resolved filesystem path, family flags }
        |
        +--> entries/find/contains use archive-internal canonical path logic
        |
        +--> extract()/extract_bytes()
        |        |
        |        +--> reader backend reopens from stored resolved filesystem path only
        |
        +--> validate_archive(...extractability=true)
                 |
                 +--> archive_reader::open
                 +--> entries()
                 +--> reader.extract(...) for public black-box proof
```
[VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp; VERIFIED: src/detail/writer_disk_source.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

### Recommended Project Structure
```text
src/
├── detail/
│   ├── archive_path.*          # Existing archive-internal virtual-path normalization
│   ├── <renamed-host-file>.*   # Renamed/generalized shared host-file path + I/O boundary
│   └── payload_stream.*        # Existing shared stream helpers reused by readers
├── archive.cpp                 # Public open state owns original UTF-8 + resolved path
├── validation.cpp              # Validation setup and extractability reuse open path
└── formats/
    ├── bsa/                    # TES3/TES4 parser and reader call sites migrated to shared helper
    └── ba2/                    # BA2 parser and reader call sites migrated to shared helper

tests/
└── unit/
    └── host_path_correctness_boundary_tests.cpp  # New dedicated cross-family non-ASCII suite
```
[VERIFIED: src/detail/archive_path.hpp; VERIFIED: src/detail/writer_disk_source.hpp; VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp; VERIFIED: tests/CMakeLists.txt]

### Pattern 1: Resolve once, store both forms, reopen only from resolved path
**What:** Convert the caller's UTF-8 text to an internal host-file value once during `archive_reader::open`, then persist the original UTF-8 text for diagnostics and the resolved `std::filesystem::path` for every later read-side reopen. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170]

**When to use:** Every archive open that succeeds and every later reader reopen for extraction or validation extractability. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

**Example:**
```cpp
// Source: src/detail/writer_disk_source.cpp + Microsoft Learn filesystem docs
std::filesystem::path resolved{std::string{host_path_utf8}};
std::ifstream input{resolved, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open archive host path"};
}
```
[VERIFIED: src/detail/writer_disk_source.cpp; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170; CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths]

### Pattern 2: Shared helper returns local stream ownership plus focused read primitives
**What:** Keep `std::ifstream` lifetime local to callers, but require callers to obtain streams, size probes, prefixes, exact reads, and bounded chunk iteration through the renamed shared helper. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md; VERIFIED: src/detail/writer_disk_source.hpp]

**When to use:** Detection prefix reads in `src/archive.cpp`, parser file opens, validation setup, and extraction reopen paths. [VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp; VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp]

**Example:**
```cpp
// Source shape to preserve under the Phase 13 neutral naming decision
result<std::uint64_t> inspect_host_file_size(const host_file_path& host_path,
                                             const host_file_context& context);
result<std::vector<std::byte>> read_host_file_prefix(const host_file_path& host_path,
                                                     std::size_t max_bytes,
                                                     const host_file_context& context);
```
[VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-01-PLAN.md; VERIFIED: src/detail/writer_disk_source.hpp]

### Pattern 3: Validation setup reuses open as the only real setup path
**What:** `validate_archive` should stop doing its own host-file readability probe and instead call `archive_reader::open`; only malformed-but-readable bytes should become `validation_report` diagnostics. [VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

**When to use:** Public validation entry only. [VERIFIED: src/validation.cpp]

**Example:**
```cpp
// Source: src/validation.cpp (current flow to preserve, with preflight removed)
auto opened = archive_reader::open(host_path);
if (!opened) {
  if (opened.error().code == error_code::io_error ||
      opened.error().code == error_code::invalid_argument) {
    return opened.error();
  }
  return report_from_open_error(opened.error());
}
```
[VERIFIED: src/validation.cpp]

### Anti-Patterns to Avoid
- **Per-call re-resolution of UTF-8 text:** This violates D-09/D-13 and recreates the exact drift that caused the bug. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **Keeping `std::string host_path` as the only reader-state path:** Current state only stores UTF-8 text, which is insufficient for Windows-correct reopen behavior after a successful open. [VERIFIED: src/archive.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- **Leaving `validate_archive` readability preflight in place:** The current `host_path_can_be_opened()` narrow open would keep setup logic duplicated and Windows-incorrect. [VERIFIED: src/validation.cpp]
- **Fixing only representative-format files:** D-03 and D-07 require the whole read-side boundary, including TES3 read-side opens and existing writer call sites during the rename. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Windows Unicode path conversion | Custom UTF-8/UTF-16 conversion utility stack [ASSUMED] | `std::filesystem::path` at the boundary [CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths] | Microsoft already documents the required conversions and `<fstream>` interoperability; a custom layer adds new failure modes and violates the no-new-dependency / no-new-wrapper intent. [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170; VERIFIED: AGENTS.md] |
| Shared read-side file helper | Second read-only host-file abstraction parallel to `writer_disk_source` [ASSUMED] | Rename and generalize the existing helper in `src/detail/` [VERIFIED: src/detail/writer_disk_source.hpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | D-01/D-02 explicitly lock generalization, not duplication. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Validation setup logic | Separate validation-only readability/open pipeline [ASSUMED] | `archive_reader::open` + result/report split [VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | D-15 through D-19 require one real setup path and no validation-only extraction route. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Test proof | White-box instrumentation or call counters [ASSUMED] | Public API open/validate/extract regression suite [VERIFIED: tests/CMakeLists.txt; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | Black-box proof is the locked acceptance style for this phase. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |

**Key insight:** The codebase already has the right Windows-aware primitive (`writer_disk_source`) and the right public orchestration seam (`archive_reader::open`); the phase is mostly about consolidating all read-side file access behind those two existing patterns instead of adding new concepts. [VERIFIED: src/detail/writer_disk_source.cpp; VERIFIED: src/archive.cpp]

## Common Pitfalls

### Pitfall 1: Fixing only `archive_reader::open` but not follow-on extraction reopens
**What goes wrong:** Open succeeds from a non-ASCII path, but `extract()` or `validate_entry_extractability` fails because reader backends still reopen using narrow `std::string` paths. [VERIFIED: src/archive.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp]
**Why it happens:** Current extraction helpers accept `std::string_view host_path` and open locally inside each reader file. [VERIFIED: src/formats/bsa/tes4_bsa_reader.hpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.hpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.hpp]
**How to avoid:** Change reader-state ownership first, then migrate all reopened read paths to use the stored resolved path through the shared helper. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
**Warning signs:** New open tests pass but extractability or canonical payload tests fail only on non-ASCII temp paths. [VERIFIED: phase acceptance criteria in 13-SPEC.md]

### Pitfall 2: Preserving validation's duplicate preflight
**What goes wrong:** Validation can still fail on non-ASCII paths even after open is fixed, or it can produce drifted diagnostics. [VERIFIED: src/validation.cpp]
**Why it happens:** `host_path_can_be_opened()` currently performs a separate narrow `std::ifstream` probe before `archive_reader::open`. [VERIFIED: src/validation.cpp]
**How to avoid:** Remove the preflight and let `archive_reader::open` own unreadable-path classification. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
**Warning signs:** Two different `io_error` messages or code paths for the same unreadable non-ASCII path. [VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

### Pitfall 3: Accidentally changing archive-internal path semantics while touching host-path code
**What goes wrong:** Lookup, hashing, or separator behavior changes even though the bug is only at the host-filesystem boundary. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-SPEC.md]
**Why it happens:** Host path and archive path are easy to conflate if the new internal host-file type is threaded through entry lookup code. [VERIFIED: src/detail/archive_path.hpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-SPEC.md]
**How to avoid:** Keep `detail::archive_path` untouched and isolate the new internal value to host-file boundary and reader state ownership only. [VERIFIED: src/detail/archive_path.hpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
**Warning signs:** Tests for `find()`, `contains()`, or manifest path normalization start failing on ASCII fixtures. [VERIFIED: tests/unit/archive_path_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp]

### Pitfall 4: Copying manifests and expected payload trees into the non-ASCII temp directory
**What goes wrong:** The dedicated suite stops being focused on the archive-host-path boundary and adds noisy temp-fixture complexity. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
**Why it happens:** Existing tests often read archives and manifests from the same generated tree, so it is tempting to relocate both. [VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp]
**How to avoid:** Copy only the archive-under-test; keep manifests and expected payload bytes in the committed generated fixture tree. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
**Warning signs:** The new suite creates duplicated manifest files under temp or needs cleanup for more than the copied archive. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

## Code Examples

Verified patterns from official sources and the current codebase:

### Shared Windows-aware open primitive
```cpp
// Source: src/detail/writer_disk_source.cpp
result<std::ifstream> open_disk_source(std::string_view host_path,
                                       const writer_disk_source_context& context) {
  std::ifstream input{std::filesystem::path{std::string{host_path}}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, std::string{context.open_error}};
  }
  return input;
}
```
[VERIFIED: src/detail/writer_disk_source.cpp; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170]

### Validation flow to preserve after removing duplicate preflight
```cpp
// Source: src/validation.cpp
auto opened = archive_reader::open(host_path);
if (!opened) {
  const auto& err = opened.error();
  if (err.code == error_code::io_error || err.code == error_code::invalid_argument) {
    return err;
  }
  return report_from_open_error(err);
}
```
[VERIFIED: src/validation.cpp]

### Existing manifest-backed extraction assertion pattern to reuse
```cpp
// Source: tests/unit/ba2_dx10_extraction_tests.cpp
auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive).string());
REQUIRE(opened.has_value());

auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
```
[VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Repeated narrow `std::ifstream{std::string{host_path}}` opens in read-side code [VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp; VERIFIED: src/formats/bsa/tes3_bsa_parser.cpp; VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp; VERIFIED: src/formats/bsa/tes3_bsa_reader.cpp; VERIFIED: src/formats/bsa/tes4_bsa_reader.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_parser.cpp; VERIFIED: src/formats/ba2/ba2_gnrl_reader.cpp; VERIFIED: src/formats/ba2/ba2_dx10_parser.cpp; VERIFIED: src/formats/ba2/ba2_dx10_reader.cpp] | Shared `std::filesystem::path` boundary with caller-supplied diagnostics, already present on writer-side disk-source access [VERIFIED: src/detail/writer_disk_source.cpp] | Already present in current repo on writer-side only [VERIFIED: src/detail/writer_disk_source.cpp] | Phase 13 should extend this boundary across the read side so non-ASCII Windows paths become uniformly correct. [VERIFIED: .planning/codebase/CONCERNS.md; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Validation-specific host readability preflight [VERIFIED: src/validation.cpp] | Open-as-setup-source validation contract [VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | Current flow partially uses `archive_reader::open` already, but still has duplicate preflight. [VERIFIED: src/validation.cpp] | Removing the preflight eliminates one remaining non-ASCII failure route and preserves locked result/report semantics. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Per-format regression suites on ASCII fixture roots [VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp] | Dedicated cross-family non-ASCII path suite [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | Not yet implemented. [VERIFIED: tests/CMakeLists.txt] | This gives one stable acceptance surface for HOST-01/HOST-02/HOST-03 without scattering host-path proofs across existing suites. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |

**Deprecated/outdated:**
- Direct narrow read-side host-file opens are the outdated pattern for Windows non-ASCII correctness in this repo. [VERIFIED: .planning/codebase/CONCERNS.md; VERIFIED: grep over `src/**/*.cpp`]
- Validation's `host_path_can_be_opened()` preflight is outdated for Phase 13 because it duplicates setup and uses the wrong open boundary. [VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Labeling library responsibilities as `API / Backend` and `Shared detail services` is the best architectural tier mapping for a local C++ library. [ASSUMED] | Architectural Responsibility Map | Low — affects plan wording more than implementation behavior. |
| A2 | A new dedicated suite file named `tests/unit/host_path_correctness_boundary_tests.cpp` is the best fit for the locked Phase 13 regression surface. [ASSUMED] | Recommended Project Structure / Validation Architecture | Low — planner can rename the file while preserving the dedicated-suite decision. |
| A3 | A custom UTF-8/UTF-16 conversion layer would be strictly worse than `std::filesystem::path` for this phase. [ASSUMED] | Don't Hand-Roll | Medium — if an STL-specific bug surfaced, planner would need a narrower workaround. |
| A4 | The ASVS tier labels used below are an acceptable fit for a local host-path hardening phase in a library project. [ASSUMED] | Security Domain | Low — they guide review focus, not public behavior. |

## Open Questions (RESOLVED)

1. **Resolved neutral helper/type naming replacement for `writer_disk_source`.** [RESOLVED]
   - Decision: use the `host_file` naming family for the shared helper module (`src/detail/host_file.*`) and `host_file_context` for caller-supplied diagnostics. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-01-PLAN.md]
   - Decision: use `detail::host_file_path` as the shared internal value declared in `src/detail/host_path.hpp`, pairing original UTF-8 diagnostics text with the resolved `std::filesystem::path`. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-01-PLAN.md]
   - Rationale: this matches the planned file layout, stays neutral across read/write call sites, and satisfies D-01/D-02 without compatibility shims. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-01-PLAN.md]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Configure/build/test presets [VERIFIED: README.md; VERIFIED: CMakePresets.json] | ✓ [VERIFIED: `cmake --version`] | 4.3.2 [VERIFIED: `cmake --version`] | — |
| CTest | Automated regression execution [VERIFIED: README.md; VERIFIED: CMakePresets.json] | ✓ [VERIFIED: `ctest --version`] | 4.3.2 [VERIFIED: `ctest --version`] | Run `libbsa_tests.exe` directly if needed, but preset-based CTest is the standard path. [VERIFIED: tests/CMakeLists.txt; ASSUMED] |
| Python 3 | Test configure path because `tests/CMakeLists.txt` requires `Python3::Interpreter` [VERIFIED: tests/CMakeLists.txt] | ✓ [VERIFIED: `python --version`] | 3.14.5 [VERIFIED: `python --version`] | — |
| vcpkg | Dependency resolution through presets/toolchain [VERIFIED: CMakePresets.json; VERIFIED: vcpkg.json] | ✓ [VERIFIED: `vcpkg version`] | 2026-04-08-e0612b42ce44e55a0e630f2ee9d3c533a63d8bc1 [VERIFIED: `vcpkg version`] | — |
| MSVC `cl` on current shell PATH | Direct compiler invocation from the current PowerShell session [ASSUMED] | ✗ [VERIFIED: `Get-Command cl`] | — | Use the checked-in CMake presets in a developer shell / environment where MSVC is initialized. [VERIFIED: README.md; ASSUMED] |

**Missing dependencies with no fallback:**
- None for research itself. [VERIFIED: current session tool checks]

**Missing dependencies with fallback:**
- `cl` is not on the current shell PATH, but the repo's intended workflow is preset-driven Windows MSVC builds from an initialized environment rather than direct `cl` usage. [VERIFIED: `Get-Command cl`; VERIFIED: README.md; VERIFIED: CMakePresets.json]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 3 via `Catch2::Catch2WithMain`. [VERIFIED: tests/CMakeLists.txt; VERIFIED: .planning/codebase/TESTING.md] |
| Config file | `tests/CMakeLists.txt`. [VERIFIED: tests/CMakeLists.txt] |
| Quick run command | `ctest --preset windows-msvc-debug-static -R "host_file|host_path_correctness_boundary_smoke" --output-on-failure` after the new suite is added. [ASSUMED; VERIFIED: CMakePresets.json] |
| Full suite command | `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: README.md; VERIFIED: CMakePresets.json] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| HOST-01 | Non-ASCII host-path `archive_reader::open` succeeds for TES4 v103/v104/v105, FO4 BA2 GNRL, FO4 BA2 DX10, and Starfield BA2 GNRL. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-SPEC.md] | fixture + public API regression [VERIFIED: .planning/codebase/TESTING.md] | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` [ASSUMED; VERIFIED: CMakePresets.json] | ❌ Wave 0 — add dedicated suite and register it in `tests/CMakeLists.txt`. [VERIFIED: tests/CMakeLists.txt] |
| HOST-02 | `validate_archive(non_ascii_path, {.validate_entry_extractability = true})` succeeds for the same representative set. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-SPEC.md] | fixture + validation regression [VERIFIED: tests/unit/validation_api_tests.cpp] | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` [ASSUMED; VERIFIED: CMakePresets.json] | ❌ Wave 0 — same dedicated suite. [VERIFIED: tests/CMakeLists.txt] |
| HOST-03 | Maintainer has committed regression proof for open, validate, and one canonical extraction per representative family. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | fixture + black-box regression [VERIFIED: .planning/codebase/TESTING.md] | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` and full preset run before phase gate. [ASSUMED; VERIFIED: CMakePresets.json] | ❌ Wave 0 — suite does not exist yet. [VERIFIED: tests/CMakeLists.txt] |

### Sampling Rate
- **Per task commit:** Build if needed, then run `ctest --preset windows-msvc-debug-static -R "host_file|host_path_correctness_boundary_smoke" --output-on-failure` once the suite exists. [ASSUMED; VERIFIED: CMakePresets.json]
- **Per wave merge:** `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: README.md; VERIFIED: CMakePresets.json]
- **Phase gate:** Full suite green before `/gsd-verify-work`. [ASSUMED]

### Wave 0 Gaps
- [ ] `tests/unit/host_path_correctness_boundary_tests.cpp` — dedicated cross-family non-ASCII host-path regression suite. [ASSUMED; VERIFIED: tests/CMakeLists.txt]
- [ ] `tests/CMakeLists.txt` — register the new test source in `libbsa_tests`. [VERIFIED: tests/CMakeLists.txt]
- [ ] Small file-local helpers for non-ASCII temp directory/filename setup, archive copy, and manifest-backed expected payload lookup. [VERIFIED: tests/unit/validation_api_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp]
- [ ] Focused regression tag naming containing `host_path_correctness_boundary`, plus one smoke subset/tag named `host_path_correctness_boundary_smoke`, so task-level feedback stays fast. [ASSUMED]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no [ASSUMED] | Not applicable; this is a local archive library with no identity subsystem. [VERIFIED: .planning/codebase/ARCHITECTURE.md] |
| V3 Session Management | no [ASSUMED] | Not applicable. [VERIFIED: .planning/codebase/ARCHITECTURE.md] |
| V4 Access Control | no [ASSUMED] | Not applicable in the library runtime surface for this phase. [ASSUMED] |
| V5 Input Validation | yes [ASSUMED] | Validate empty path input, preserve bounded size checks, and keep parser/read helpers enforcing offset and length limits after the shared host-file boundary opens the archive. [VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp; VERIFIED: src/detail/writer_disk_source.cpp; VERIFIED: src/detail/parser_primitives.cpp] |
| V6 Cryptography | no [ASSUMED] | Not applicable; Phase 13 is host-path boundary hardening, not a crypto phase. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-SPEC.md] |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Host-path encoding mismatch on Windows causes false `io_error` or inconsistent open behavior. [VERIFIED: .planning/codebase/CONCERNS.md] | Denial of Service [ASSUMED] | Resolve once to `std::filesystem::path` and make all later reads use the stored resolved path only. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170] |
| Divergent validation preflight and open logic produce inconsistent diagnostics or behavior. [VERIFIED: src/validation.cpp] | Tampering / Repudiation [ASSUMED] | Remove duplicate preflight and trust `archive_reader::open` as the single setup source. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |
| Re-resolving raw caller text after open can bypass the proven-good stored path or drift across call sites. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] | Tampering [ASSUMED] | Store `{original_utf8, resolved_path}` once and treat original text as diagnostics-only after open succeeds. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md] |

## Sources

### Primary (HIGH confidence)
- `src/archive.cpp` - current open flow, reader state ownership, detection prefix read, size probe, and extraction dispatch. [VERIFIED: src/archive.cpp]
- `src/validation.cpp` - current validation preflight, result/report split, and extractability flow. [VERIFIED: src/validation.cpp]
- `src/detail/writer_disk_source.hpp` and `src/detail/writer_disk_source.cpp` - existing Windows-aware host-file helper to generalize. [VERIFIED: src/detail/writer_disk_source.hpp; VERIFIED: src/detail/writer_disk_source.cpp]
- `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp` - read-side narrow open call sites that Phase 13 must migrate. [VERIFIED: grep over `src/**/*.cpp`]
- `.planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md` and `13-SPEC.md` - locked decisions, scope, and acceptance criteria. [VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-SPEC.md]
- Microsoft Learn `<filesystem>` / `path` / File System Navigation docs - Windows native pathname representation, UTF-8/UTF-16 conversion behavior, and `<fstream>` interoperability for `std::filesystem::path`. [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170; CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths; CITED: https://learn.microsoft.com/cpp/standard-library/path-class?view=msvc-170#pathpath]
- `tests/CMakeLists.txt` and existing unit suites - Catch2/CTest registration and fixture/manfiest testing patterns. [VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: tests/unit/ba2_dx10_extraction_tests.cpp; VERIFIED: tests/unit/validation_api_tests.cpp]

### Secondary (MEDIUM confidence)
- `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/TESTING.md`, `.planning/codebase/CONCERNS.md` - curated codebase analyses that matched direct code inspection during this research. [VERIFIED: .planning/codebase/ARCHITECTURE.md; VERIFIED: .planning/codebase/TESTING.md; VERIFIED: .planning/codebase/CONCERNS.md]

### Tertiary (LOW confidence)
- None. [VERIFIED: this research session]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - the phase reuses existing repo patterns plus Microsoft-documented `std::filesystem::path` behavior. [VERIFIED: src/detail/writer_disk_source.cpp; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170]
- Architecture: HIGH - affected files, state ownership, and validation flow are directly visible in the codebase and heavily constrained by locked context decisions. [VERIFIED: src/archive.cpp; VERIFIED: src/validation.cpp; VERIFIED: .planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md]
- Pitfalls: HIGH - the exact narrow open call sites and validation duplication are directly verified in current source files. [VERIFIED: grep over `src/**/*.cpp`; VERIFIED: src/validation.cpp]

**Research date:** 2026-05-13 [VERIFIED: system date]
**Valid until:** 2026-06-12 for planning in this codebase unless the phase context or affected files change first. [ASSUMED]
