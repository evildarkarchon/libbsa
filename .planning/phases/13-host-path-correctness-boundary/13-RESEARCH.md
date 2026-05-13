# Phase 13: Host Path Correctness Boundary - Research

**Researched:** 2026-05-13
**Domain:** Windows host-path correctness at the archive open/validate/extract boundary
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

Verbatim copy from `13-CONTEXT.md`. [VERIFIED: `J:/libbsa/.planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md`]

### Locked Decisions
- **D-01:** Generalize the existing `src/detail/writer_disk_source.*` boundary into a neutrally named shared host-file helper in `src/detail/` rather than adding a parallel read-only helper.
- **D-02:** Rename the helper and its types now so the names match the new shared read/write role, and migrate existing writer call sites in the same phase instead of leaving compatibility shims behind.
- **D-03:** The shared boundary must cover all read-side host-file opens touched by the Phase 13 read/validate flow, including detection-prefix reads, size probes, parser opens, validation setup, and payload extraction opens. Do not limit the cleanup to only the representative test path.
- **D-04:** The helper surface should be layered: small neutral primitives plus focused helpers for common patterns like size inspection, prefix reads, exact reads, and bounded chunk iteration.
- **D-05:** Callers should continue to own stream lifetime locally, but they must obtain read-side streams through the shared helper. The helper should hand back `std::ifstream` streams, not introduce a new custom file wrapper in Phase 13.
- **D-06:** Error reporting stays context-driven. Shared mechanics are centralized, but callers still supply operation-specific diagnostics so public/read-side messages remain stable and phase-appropriate.
- **D-07:** Even though TES3 non-ASCII regression coverage is out of scope, TES3 read-side narrow opens should migrate to the same shared host-file boundary while the refactor is in flight so the repo actually lands on one read-side policy boundary.
- **D-08:** Keep the public API unchanged: callers still pass UTF-8 host paths as `std::string_view`.
- **D-09:** Resolve the UTF-8 host path into the Windows-correct filesystem path once at the public API boundary, not lazily per helper call and not once per later operation.
- **D-10:** Store both forms internally after open succeeds: the original UTF-8 text for diagnostics/traceability and one resolved `std::filesystem::path` for actual host-file I/O.
- **D-11:** Represent those two forms as one small shared internal path value in `src/detail/` instead of widening signatures with separate text/path parameters or keeping subsystem-specific copies.
- **D-12:** After open succeeds, the resolved filesystem path is the only I/O source. The original UTF-8 text is diagnostics-only and must not become a fallback path-opening route.
- **D-13:** Follow-on extraction reads must reopen from the path stored in `archive_reader::state`; they must not re-resolve raw caller text elsewhere.
- **D-14:** Open, validation, and migrated writer call sites should converge on the same shared internal path type.
- **D-15:** Remove `validate_archive`'s duplicate readability preflight and trust the shared host-file boundary plus `archive_reader::open` as the one real setup path.
- **D-16:** Preserve the existing public result/report split. Empty input still fails fast with `invalid_argument`; unreadable or non-openable host paths still fail as direct `io_error` results; readable malformed archive bytes still become `validation_report` diagnostics.
- **D-17:** Unreadable host-path failures should surface the shared helper/open diagnostics directly rather than being reworded through a validation-specific preflight layer.
- **D-18:** If setup fails before parsing any archive bytes, `validate_archive` should return a failed `result<validation_report>` with no partial report object.
- **D-19:** `validate_entry_extractability` must continue to prove payload reads through the opened reader state and the public extraction path. Do not add a validation-only host-path or extraction codepath.
- **D-20:** Phase 13 tests should prove the validation cleanup only through black-box public behavior. Do not add white-box hooks or open-count assertions that couple tests to the exact internal call graph.
- **D-21:** Add one dedicated cross-family Phase 13 regression suite for the non-ASCII host-path proof instead of scattering the main story across existing per-format tests.
- **D-22:** The suite should cover all three existing TES4 BSA fixtures (`v103`, `v104`, `v105`) plus the locked representative BA2 fixtures for Fallout 4 GNRL, Fallout 4 DX10, and Starfield GNRL.
- **D-23:** Copy only the archive-under-test into the non-ASCII temp location. Keep manifests and expected payload data in the normal generated-fixture tree.
- **D-24:** The non-ASCII proof must cover both a non-ASCII directory and a non-ASCII archive filename, not just one path segment.
- **D-25:** Use one stable curated naming token across the suite: ``libbsa-Ångström-日本語``. Keep the token deterministic and BMP-only rather than using a broader script matrix or random names.
- **D-26:** For the post-open payload proof, each representative archive only needs one canonical manifest-backed extraction target with expected bytes. Do not rerun every manifest entry from the non-ASCII path in this dedicated suite.
- **D-27:** The suite should stay black-box and deterministic: prove `archive_reader::open`, `validate_archive(..., {.validate_entry_extractability = true})`, and the canonical extraction result from the public API surface only.

### the agent's Discretion
None. The implementation shape is intentionally locked for downstream research and planning.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| HOST-01 | Consumer can open supported archives from Windows host paths containing non-ASCII characters. [VERIFIED: `.planning/REQUIREMENTS.md`] | Resolve the UTF-8 host path once in `archive_reader::open`, store a shared internal path object, and route detection, size probing, and parser opens through one `std::filesystem::path`-based helper. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/detail/writer_disk_source.cpp`; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax] |
| HOST-02 | Consumer can validate supported archives from Windows host paths containing non-ASCII characters. [VERIFIED: `.planning/REQUIREMENTS.md`] | Remove `validate_archive`'s duplicate narrow preflight and let validation setup reuse `archive_reader::open` plus reader-state-based extraction. [VERIFIED: `src/validation.cpp`; VERIFIED: `include/libbsa/validation.hpp`; VERIFIED: `13-CONTEXT.md`] |
| HOST-03 | Maintainer can verify non-ASCII host-path open and validate coverage through committed regression tests for representative BSA and BA2 families. [VERIFIED: `.planning/REQUIREMENTS.md`] | Add one dedicated cross-family Catch2 suite, register it in `tests/CMakeLists.txt`, and drive canonical expected bytes from the committed manifests already used by existing reader tests. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`; VERIFIED: `tests/unit/validation_api_tests.cpp`; VERIFIED: `tests/fixtures/generated/archives/*.json`] |
</phase_requirements>

## Project Constraints (from AGENTS.md)

- Implement Phase 13 in C++20 and keep the library reusable, Windows-only, and free of UI/tool coupling. [VERIFIED: `AGENTS.md`; VERIFIED: `.planning/PROJECT.md`]
- Do not modify, format, stage, compile into libbsa, or otherwise treat `TES5Edit/` as writable code. [VERIFIED: `AGENTS.md`]
- Do not add speculative dependencies; Phase 13 must use the standard library plus already-approved project dependencies only. [VERIFIED: `AGENTS.md`; VERIFIED: `13-SPEC.md`]
- Keep the public API `std::string_view`-based for host paths in this phase; do not add `std::filesystem::path` overloads or other public path types. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `13-SPEC.md`]
- Preserve archive-internal path normalization, hashing, lookup, and separator semantics; the bug is only at the host-filesystem boundary. [VERIFIED: `13-SPEC.md`; VERIFIED: `AGENTS.md`]
- Add focused fixture-based tests and keep them black-box; do not add test-only production shims or white-box hooks. [VERIFIED: `AGENTS.md`; VERIFIED: `13-CONTEXT.md`; VERIFIED: `.planning/codebase/TESTING.md`]
- If Phase 13 rewrites comments or adds public/internal methods, keep accurate comments, add WHY comments where behavior is non-obvious, and add Doxygen-style doc comments for added or substantially rewritten public/internal methods as required by project policy. [VERIFIED: `AGENTS.md`; VERIFIED: `C:/Users/evild/.config/opencode/AGENTS.md`]

## Summary

Phase 13 is a boundary-hardening refactor, not a product-surface change. The live code still opens read-side archive files through repeated narrow-string `std::ifstream{std::string{host_path}}` calls in `src/archive.cpp`, `src/validation.cpp`, all four shipped parser entry points, and all four read-side extraction implementations; `archive_reader::state` stores only the original UTF-8 text today. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/validation.cpp`; VERIFIED: `src/formats/bsa/tes3_bsa_parser.cpp`; VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; VERIFIED: `src/formats/ba2/ba2_gnrl_parser.cpp`; VERIFIED: `src/formats/ba2/ba2_dx10_parser.cpp`; VERIFIED: `src/formats/bsa/tes3_bsa_reader.cpp`; VERIFIED: `src/formats/bsa/tes4_bsa_reader.cpp`; VERIFIED: `src/formats/ba2/ba2_gnrl_reader.cpp`; VERIFIED: `src/formats/ba2/ba2_dx10_reader.cpp`]

The shortest correct plan is to generalize `src/detail/writer_disk_source.*` into a shared `host_file`-style detail boundary, then make `archive_reader::open` the single place that resolves caller UTF-8 into a Windows-correct `std::filesystem::path`, stores both UTF-8 text and resolved path in one internal value, and passes only the resolved path through parser/re-reader/validation code after open succeeds. Microsoft documents that Windows `std::filesystem::path` stores native `wchar_t` paths, supports UTF-8 `char` input, and can be used anywhere `<fstream>` expects a filename argument, which is the key standard-library capability this phase should lean on instead of custom conversion code. [VERIFIED: `src/detail/writer_disk_source.cpp`; VERIFIED: `13-CONTEXT.md`; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax; CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths]

The testing story should stay black-box and cross-family. The repo already has committed representative fixtures, manifest-backed expected bytes, temp-path helpers, Catch2/CTest registration, and extraction assertions; what is missing is one dedicated non-ASCII-path suite that copies only the archive-under-test into a temp directory and filename containing ``libbsa-Ångström-日本語``, then proves open, validate-with-extractability, and one canonical extraction per representative archive. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`; VERIFIED: `tests/unit/validation_api_tests.cpp`; VERIFIED: `tests/unit/ba2_gnrl_reader_tests.cpp`; VERIFIED: `tests/unit/ba2_dx10_extraction_tests.cpp`; VERIFIED: `13-CONTEXT.md`]

**Primary recommendation:** Plan Phase 13 as four sequential edits: rename/generalize the writer helper, add an internal resolved host-path value in reader state, migrate every read-side open/reopen seam onto that value, then add one dedicated cross-family non-ASCII regression suite. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/archive.cpp`; VERIFIED: `src/detail/writer_disk_source.cpp`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Public UTF-8 host-path intake | Public facade (`src/archive.cpp`, `src/validation.cpp`) | Shared detail | `archive_reader::open` and `validate_archive` are the only public entry points that accept caller host-path text, so the one-time UTF-8→filesystem resolution belongs there. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/validation.cpp`; VERIFIED: `include/libbsa/archive.hpp`; VERIFIED: `include/libbsa/validation.hpp`] |
| Windows-correct file open/size/prefix/exact-read primitives | Shared detail (`src/detail/`) | Public facade | The existing writer helper already centralizes path-based open, size inspection, prefix reads, exact reads, and bounded chunk iteration, so Phase 13 should expand that seam rather than duplicate it. [VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `src/detail/writer_disk_source.cpp`; VERIFIED: `13-CONTEXT.md`] |
| Parser entry host-file reads | Format pipeline (`src/formats/*/*_parser.cpp`) | Shared detail | Parsers still own archive grammar and bounded metadata reads, but they should obtain already-open streams or resolved paths from the shared host-file boundary instead of reopening from UTF-8 text. [VERIFIED: `src/formats/bsa/tes3_bsa_parser.hpp`; VERIFIED: `src/formats/bsa/tes4_bsa_parser.hpp`; VERIFIED: `src/formats/ba2/ba2_gnrl_parser.hpp`; VERIFIED: `src/formats/ba2/ba2_dx10_parser.hpp`] |
| Post-open extraction reopens | Format readers (`src/formats/*/*_reader.cpp`) | Shared detail | Follow-on extraction is format-specific, but each reader currently reopens the host archive directly, so the reopen contract must be migrated onto the stored resolved path. [VERIFIED: `src/formats/bsa/tes3_bsa_reader.hpp`; VERIFIED: `src/formats/bsa/tes4_bsa_reader.hpp`; VERIFIED: `src/formats/ba2/ba2_gnrl_reader.hpp`; VERIFIED: `src/formats/ba2/ba2_dx10_reader.hpp`; VERIFIED: `13-CONTEXT.md`] |
| Validation setup/result split | Validation facade (`src/validation.cpp`) | Public facade | Validation owns setup-vs-report semantics, so it must stop doing an extra readability preflight while preserving the existing result/report contract. [VERIFIED: `src/validation.cpp`; VERIFIED: `include/libbsa/validation.hpp`; VERIFIED: `13-CONTEXT.md`] |
| Non-ASCII regression proof | Test harness (`tests/unit`, `tests/CMakeLists.txt`) | Fixture manifests | The proof is consumer-visible behavior, so it belongs in black-box Catch2 suites that reuse committed manifests and expected payload bytes. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `.planning/codebase/TESTING.md`; VERIFIED: `13-CONTEXT.md`] |

## Standard Stack

### Core
| Library / Facility | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++20 `std::filesystem::path` | Project-wide C++20 policy; no new library needed. [VERIFIED: `.planning/PROJECT.md`] | Store the resolved Windows-correct host path after one-time UTF-8 intake. [VERIFIED: `13-CONTEXT.md`] | Microsoft documents that Windows `path` stores native `wchar_t` paths and accepts narrow UTF-8 input, which is the standard-library mechanism Phase 13 needs. [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax; CITED: https://learn.microsoft.com/cpp/standard-library/path-class?view=msvc-170] |
| `std::ifstream` opened from `std::filesystem::path` | C++17+ standard-library capability reused in the existing helper. [VERIFIED: `src/detail/writer_disk_source.cpp`] | Open archive files for detection, metadata parsing, and payload extraction through the resolved path. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/detail/writer_disk_source.cpp`] | Microsoft documents that `path` objects can be used anywhere `<fstream>` expects a filename argument, avoiding custom wide-path wrappers in this phase. [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax; CITED: https://learn.microsoft.com/cpp/standard-library/basic-ifstream-class?view=msvc-170#%60basic_ifstreambasic_ifstream%60] |
| Shared `src/detail/host_file`-style helper generalized from `writer_disk_source` | New internal seam; rename required by locked decision. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/detail/writer_disk_source.hpp`] | Centralize open/inspect/prefix/exact/chunk host-file operations with caller-supplied diagnostics. [VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `src/detail/writer_disk_source.cpp`] | The repo already has the right primitive shape; Phase 13 should expand it instead of adding a second helper or hand-rolling per-call logic again. [VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `13-CONTEXT.md`] |

### Supporting
| Library / Facility | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `libbsa::result<T>` / `error_code` | Existing public/internal error contract. [VERIFIED: `include/libbsa/result.hpp`; VERIFIED: `src/archive.cpp`; VERIFIED: `src/validation.cpp`] | Preserve stable result-level `invalid_argument`/`io_error` behavior while refactoring path plumbing. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/validation.cpp`] | Use for every new helper and migrated seam; do not introduce exceptions for path-open failures. [VERIFIED: `.planning/PROJECT.md`; VERIFIED: `13-CONTEXT.md`] |
| Catch2 3 + CTest | Existing test stack. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `.planning/codebase/TESTING.md`] | Register the dedicated cross-family non-ASCII regression suite and keep it discoverable by tags. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `13-VALIDATION.md`] | Use for the dedicated Phase 13 suite and any renamed helper tests. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `tests/unit/writer_disk_source_tests.cpp`] |
| `nlohmann_json` in tests | Existing test-only dependency. [VERIFIED: `tests/CMakeLists.txt`] | Read committed manifest files so the new suite can assert canonical expected bytes without new fixtures. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`; VERIFIED: `tests/unit/ba2_gnrl_reader_tests.cpp`; VERIFIED: `tests/unit/ba2_dx10_extraction_tests.cpp`] | Use only in the new test suite; Phase 13 does not need any new runtime dependency. [VERIFIED: `13-SPEC.md`] |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Generalizing `writer_disk_source` into a neutral shared helper | Add a second read-only helper beside the existing writer helper | Rejected by locked decisions D-01/D-02 because it would preserve duplicate policy seams and rename debt. [VERIFIED: `13-CONTEXT.md`] |
| Resolving once at open and storing both UTF-8 text plus resolved path | Re-resolve the raw `std::string_view` every time a parser or reader reopens the file | Rejected by D-09 through D-13 because it repeats conversion policy, risks drift, and leaves follow-on extraction dependent on caller text instead of opened state. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/archive.cpp`] |
| Reusing `archive_reader::open` as validation setup | Keep `validation.cpp`'s extra `host_path_can_be_opened()` preflight | Rejected by D-15 through D-18 because it duplicates the boundary and can reword or diverge from shared open diagnostics. [VERIFIED: `src/validation.cpp`; VERIFIED: `13-CONTEXT.md`] |

**Installation:**
```bash
# No new packages. Reuse the existing Windows CMake/vcpkg/Catch2 toolchain.
```

**Version verification:** `cmake --version` and `ctest --version` both report 4.3.2 locally, `VCPKG_ROOT` is set to `C:\vcpkg`, and Phase 13 introduces no new third-party dependency. [VERIFIED: local `cmake --version`; VERIFIED: local `ctest --version`; VERIFIED: local `$env:VCPKG_ROOT`; VERIFIED: `13-SPEC.md`]

## Architecture Patterns

### System Architecture Diagram

```text
caller UTF-8 host_path (std::string_view)
        |
        v
archive_reader::open / validate_archive
        |
        +--> reject empty input -> invalid_argument result
        |
        +--> resolve once to detail::host_file_path
        |        |- original_utf8 (diagnostics only)
        |        `- resolved_fs_path (all I/O after success)
        |
        +--> shared host-file helper
        |        |- open stream
        |        |- read detection prefix
        |        |- inspect archive size
        |        `- hand back std::ifstream or bytes
        |
        +--> detector -> parser entry point
        |                `- parser reads bounded metadata through shared/opened stream
        |
        +--> archive_reader::state stores metadata + entries + host_file_path
        |
        +--> later extract()/extract_bytes()/validate_entry_extractability
                 `- reader reopen from stored resolved_fs_path only
                           `- stream/decompress payload -> caller sink / discard sink
```

The live code already separates public facade, format pipeline, and shared detail layers; Phase 13 should insert the host-path policy boundary into that existing flow rather than inventing a new architecture. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`; VERIFIED: `src/archive.cpp`; VERIFIED: `src/validation.cpp`]

### Recommended Project Structure
```text
src/
├── detail/
│   ├── host_file_path.*      # small internal value storing UTF-8 text + resolved filesystem path
│   ├── host_file.*           # generalized open/inspect/prefix/exact/chunk helper
│   └── writer_publish.*      # unchanged sibling detail service
├── archive.cpp               # one-time host-path resolution + state storage
├── validation.cpp            # setup/result split reusing archive_reader::open
└── formats/
    ├── bsa/*_parser.*        # parser seams migrated off raw host_path text
    ├── bsa/*_reader.*        # extraction reopens migrated to resolved path
    └── ba2/*_parser.* / *_reader.*
tests/
├── unit/host_path_correctness_boundary_tests.cpp  # new dedicated cross-family suite
└── unit/host_file_tests.cpp or renamed helper suite # existing helper tests migrated with the rename
```
The exact new filenames are not locked, but the seam locations are: `src/detail/`, `src/archive.cpp`, `src/validation.cpp`, the parser/reader headers and `.cpp` files listed in `13-CONTEXT.md`, plus `tests/CMakeLists.txt`. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `tests/CMakeLists.txt`]

### Pattern 1: Resolve once, store both forms, reopen only from the resolved path
**What:** Convert the public UTF-8 host path to an internal value containing both the original text and one resolved `std::filesystem::path`, then carry that value through reader state. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/archive.cpp`]
**When to use:** At `archive_reader::open` and any writer call site migrated by the helper rename. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/archive.cpp`; VERIFIED: `src/detail/writer_disk_source.cpp`]
**Example:**
```cpp
// Source: 13-CONTEXT.md D-09..D-14 + Microsoft Learn filesystem/path docs
struct host_file_path {
  std::string original_utf8;
  std::filesystem::path resolved;
};

host_file_path path{std::string{host_path}, std::filesystem::path{std::string{host_path}}};
std::ifstream input{path.resolved, std::ios::binary};
```

### Pattern 2: Keep stream lifetime local, but obtain every read-side stream through the shared helper
**What:** The helper returns `result<std::ifstream>` and focused byte helpers, while callers still own local stream scope and family-specific reads. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/detail/writer_disk_source.cpp`]
**When to use:** Detection prefix reads in `src/archive.cpp`, parser entry-file opens, extraction reopens, and any writer-side file-source operations touched by the rename. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `13-CONTEXT.md`]
**Example:**
```cpp
// Source: src/detail/writer_disk_source.cpp
auto input = detail::open_host_file(path, context);
if (!input) {
  return input.error();
}
return detail::stream_payload_range(input.value(), entry.payload_offset, entry.stored_size, sink, 64U * 1024U,
                                    "payload");
```

### Pattern 3: Validation setup should reuse open-time state instead of reproducing path logic
**What:** `validate_archive` should delegate setup to `archive_reader::open`, then build either a result-level setup error or a `validation_report` from the opened reader. [VERIFIED: `src/validation.cpp`; VERIFIED: `include/libbsa/validation.hpp`; VERIFIED: `13-CONTEXT.md`]
**When to use:** Every host-path validation call, especially `validate_entry_extractability=true`. [VERIFIED: `src/validation.cpp`; VERIFIED: `13-SPEC.md`]
**Example:**
```cpp
// Source: src/validation.cpp
auto opened = archive_reader::open(host_path);
if (!opened) {
  if (opened.error().code == error_code::io_error || opened.error().code == error_code::invalid_argument) {
    return opened.error();
  }
  return report_from_open_error(opened.error());
}
```

### Pattern 4: Use one dedicated cross-family regression suite with manifest-backed canonical extraction targets
**What:** Copy only the archive file into a non-ASCII temp directory and non-ASCII filename, then use the existing committed manifest in place to assert one canonical extraction result. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`; VERIFIED: `tests/unit/ba2_gnrl_reader_tests.cpp`; VERIFIED: `tests/unit/ba2_dx10_extraction_tests.cpp`]
**When to use:** Phase 13 proof only; keep broader extraction matrices in their existing suites. [VERIFIED: `13-CONTEXT.md`] 
**Example:**
```cpp
// Source: tests/unit/*_reader_tests.cpp patterns + 13-CONTEXT.md D-21..D-27
const auto root = std::filesystem::temp_directory_path() / "libbsa-Ångström-日本語";
const auto copied = root / "libbsa-Ångström-日本語.ba2";
std::filesystem::create_directories(root);
std::filesystem::copy_file(source_archive, copied, std::filesystem::copy_options::overwrite_existing);

auto opened = libbsa::archive_reader::open(copied.string());
auto validated = libbsa::validate_archive(copied.string(), {.validate_entry_extractability = true});
auto payload = opened.value().extract_bytes(canonical_entry_path);
```

### Anti-Patterns to Avoid
- **Direct `std::ifstream{std::string{host_path}}` in read-side code:** This is the current bug surface and must be removed from every open/validate/extract seam touched by Phase 13. [VERIFIED: codebase grep `std::ifstream{std::string{host_path}}`; VERIFIED: `13-SPEC.md`]
- **Keeping a validation-only readability probe:** `host_path_can_be_opened()` in `src/validation.cpp` duplicates setup policy and contradicts the locked single-boundary design. [VERIFIED: `src/validation.cpp`; VERIFIED: `13-CONTEXT.md`]
- **Re-resolving raw caller text after open succeeds:** Reader-state extraction must reopen only from the stored resolved path, never from `state_->host_path` text alone. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/archive.cpp`]
- **Introducing a new custom file wrapper type:** D-05 locks the helper to `std::ifstream` ownership, so a new wrapper object would add surface area without solving the actual bug. [VERIFIED: `13-CONTEXT.md`]
- **White-box path-open counting in tests:** D-20 locks proof to black-box public behavior only. [VERIFIED: `13-CONTEXT.md`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Windows UTF-8→native path conversion | A custom Win32 `MultiByteToWideChar` wrapper or ad-hoc wide-string cache | `std::filesystem::path` constructed once from the public UTF-8 text | The standard library already provides the conversion boundary Phase 13 needs, and the existing helper proves the repo already trusts it on writer paths. [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax; VERIFIED: `src/detail/writer_disk_source.cpp`] |
| Shared host-file open/read primitives | A second read-only helper or inline lambdas in each parser/reader | The generalized `writer_disk_source` seam renamed to a neutral host-file helper | The existing helper already has the right primitive set and caller-supplied diagnostics shape. [VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `13-CONTEXT.md`] |
| Validation-specific setup path | Separate validation-only path resolution or extraction path | `archive_reader::open` + public `reader.extract(...)` from opened state | This preserves the existing result/report split and avoids setup drift. [VERIFIED: `src/validation.cpp`; VERIFIED: `13-CONTEXT.md`] |
| New archive fixtures or copied expected payload trees | Handwritten duplicate fixture bytes under a non-ASCII temp root | Existing committed archives/manifests plus `std::filesystem::copy_file` of only the archive-under-test | D-23 and D-26 explicitly lock this to archive-copy-only plus manifest-backed expected bytes. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `tests/fixtures/generated/archives/*.json`] |

**Key insight:** Phase 13 is mostly about deleting duplicated host-file policy, not inventing new I/O abstractions. The standard library plus the existing writer helper seam already cover the hard part. [VERIFIED: `src/detail/writer_disk_source.cpp`; CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths]

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None identified; Phase 13 changes code-level host-file opening and committed test coverage, not persisted archive/database schemas. [VERIFIED: `13-SPEC.md`; VERIFIED: `.planning/PROJECT.md`] | Code edit only; no data migration. [VERIFIED: `13-SPEC.md`] |
| Live service config | None identified; libbsa is a local Windows library with no service/UI-managed host-path configuration surface in this phase. [VERIFIED: `.planning/PROJECT.md`; VERIFIED: `.planning/codebase/ARCHITECTURE.md`] | None. [VERIFIED: `.planning/PROJECT.md`] |
| OS-registered state | None identified in repo-scoped research; this phase renames internal helper symbols, not an installed product identifier or scheduled job. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `.planning/PROJECT.md`] | Rebuild/retest only. [ASSUMED] |
| Secrets/env vars | `VCPKG_ROOT`, `LIBBSA_GAME_FIXTURES`, and `LIBBSA_BSARCHPRO_EXPECTED` exist in the repo/test toolchain, but none are keyed to the helper name or Phase 13 host-path boundary. [VERIFIED: `CMakePresets.json`; VERIFIED: `tests/unit/local_game_fixture_tests.cpp`; VERIFIED: `tests/fixtures/README.md`] | None for Phase 13 logic; keep using existing env vars unchanged. [VERIFIED: `13-SPEC.md`] |
| Build artifacts | Renaming `writer_disk_source.*` and its test file will invalidate existing build outputs and `tests/CMakeLists.txt` source registration, but that is ordinary rebuild fallout rather than a runtime migration. [VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `tests/unit/writer_disk_source_tests.cpp`; VERIFIED: `tests/CMakeLists.txt`] | Code edit + full rebuild/test; no persisted artifact migration. [VERIFIED: `tests/CMakeLists.txt`] |

## Common Pitfalls

### Pitfall 1: Fixing only `archive_reader::open` and leaving parser/reader reopens narrow
**What goes wrong:** Open starts working on non-ASCII paths, but later extraction or parser rereads still fail because downstream code reopens from the old narrow-text path. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/formats/*/*_reader.hpp`; VERIFIED: `13-CONTEXT.md`]
**Why it happens:** The live code spreads host-file opens across `src/archive.cpp`, parser `.cpp` files, extraction `.cpp` files, and `validation.cpp`. [VERIFIED: codebase grep `std::ifstream{std::string{host_path}}`; VERIFIED: `13-CONTEXT.md`]
**How to avoid:** Plan the migration by seam class: prefix/size helpers, parser entry contracts, reader reopen contracts, then validation cleanup. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/archive.cpp`]
**Warning signs:** `archive_reader::open` passes from a non-ASCII temp path but `extract()` or `validate_entry_extractability` still returns `io_error`. [VERIFIED: `13-SPEC.md`]

### Pitfall 2: Removing validation preflight but accidentally changing public failure semantics
**What goes wrong:** `validate_archive` starts returning a partial `validation_report` for unreadable host paths or rewrites setup errors into validation-specific text. [VERIFIED: `src/validation.cpp`; VERIFIED: `13-CONTEXT.md`]
**Why it happens:** The current code already has a split between result-level setup errors and report-level archive diagnostics, so boundary cleanup can accidentally collapse them. [VERIFIED: `include/libbsa/validation.hpp`; VERIFIED: `src/validation.cpp`]
**How to avoid:** Keep the existing result/report contract exactly as D-16 through D-18 describe and route unreadable-host failures straight through shared open diagnostics. [VERIFIED: `13-CONTEXT.md`]
**Warning signs:** Missing-path validation no longer returns `result<validation_report>` failure with `error_code::io_error`. [VERIFIED: `tests/unit/validation_api_tests.cpp`] 

### Pitfall 3: Proving the bug with ASCII temp names or only one Unicode path segment
**What goes wrong:** The dedicated suite passes without actually locking the non-ASCII directory + filename boundary that triggered the phase. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `13-SPEC.md`]
**Why it happens:** Existing tests already use temp paths, so it is easy to reuse ASCII helpers and miss the locked token requirement. [VERIFIED: `.planning/codebase/TESTING.md`; VERIFIED: `tests/unit/*_reader_tests.cpp`] 
**How to avoid:** Put ``libbsa-Ångström-日本語`` in both the parent directory and the copied archive filename in the dedicated suite. [VERIFIED: `13-CONTEXT.md`] 
**Warning signs:** The suite never renames the archive file itself, or the copied path contains Unicode only in a parent directory. [VERIFIED: `13-CONTEXT.md`] 

### Pitfall 4: Forgetting the rename ripple into writer tests and CMake registration
**What goes wrong:** The helper is renamed in `src/detail/` but writer call sites, `writer_disk_source_tests.cpp`, or `tests/CMakeLists.txt` still reference the old name. [VERIFIED: `src/detail/writer_disk_source.hpp`; VERIFIED: `tests/unit/writer_disk_source_tests.cpp`; VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `13-CONTEXT.md`] 
**Why it happens:** Phase 13 is read-side motivated, but D-02 explicitly locks the rename and writer migration into the same phase. [VERIFIED: `13-CONTEXT.md`] 
**How to avoid:** Treat rename propagation as Wave 1 and land it before any reader-state or validation changes. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `.planning/ROADMAP.md`] 
**Warning signs:** Build breaks on missing `writer_disk_source` symbols/includes before read-side tests even run. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `src/formats/bsa/tes4_bsa_prepare.cpp`; VERIFIED: `src/formats/ba2/ba2_gnrl_prepare.cpp`; VERIFIED: `src/formats/ba2/ba2_dx10_prepare.cpp`] 

## Code Examples

Verified patterns from official sources and the live codebase:

### Shared host-file open using `std::filesystem::path`
```cpp
// Source: src/detail/writer_disk_source.cpp
std::ifstream input{std::filesystem::path{std::string{host_path}}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, std::string{context.open_error}};
}
```

### Validation setup preserving result/report semantics
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

### Manifest-backed extraction assertion shape to reuse in the new suite
```cpp
// Source: tests/unit/ba2_dx10_extraction_tests.cpp
auto opened = libbsa::archive_reader::open(archive_path.string());
REQUIRE(opened.has_value());

auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
REQUIRE(bytes.value() == expected_payload_bytes);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Narrow filename opens from `std::string` on Windows | `std::filesystem::path`-based filename handling with native Windows path storage | C++17 filesystem support; current MSVC docs describe it as the standard model. [CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax] | This is the standards-based way to keep UTF-8 public text while opening Unicode Windows paths correctly. [CITED: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths] |
| Repeated host-path conversion/open policy in each call site | One internal host-file boundary plus one resolved path stored in reader state | Phase 13 target state. [VERIFIED: `13-CONTEXT.md`] | Prevents open/validate/extract seams from drifting apart on Windows Unicode behavior. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `13-SPEC.md`] |
| Validation-specific readability preflight | Validation setup delegated to `archive_reader::open` | Phase 13 target state. [VERIFIED: `13-CONTEXT.md`; VERIFIED: `src/validation.cpp`] | Preserves one setup policy and one set of `io_error` diagnostics. [VERIFIED: `13-CONTEXT.md`] |

**Deprecated/outdated:**
- Direct read-side `std::ifstream{std::string{host_path}}` opens are the outdated pattern for this repo and are the exact bug surface Phase 13 is closing. [VERIFIED: codebase grep `std::ifstream{std::string{host_path}}`; VERIFIED: `13-SPEC.md`]
- A separate validation readability probe is outdated once the shared host-file boundary exists, because it duplicates setup policy instead of verifying consumer-visible behavior. [VERIFIED: `src/validation.cpp`; VERIFIED: `13-CONTEXT.md`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Rebuild/retest is the only practical action needed for stale local build artifacts after the helper rename. [ASSUMED] | Runtime State Inventory | Low; a maintainer may need one extra clean-build task, but no user-visible migration should be affected. |
| A2 | If MSVC CLI tools remain absent from the shell, using CI or a Visual Studio developer environment is the intended fallback. [ASSUMED] | Environment Availability | Medium; execution planning may need an explicit environment-setup task before local builds. |

## Open Questions (RESOLVED)

1. **Which single Starfield GNRL fixture should the dedicated suite treat as the representative archive?**
   - Resolution: Use `ba2_gnrl_sfv3.ba2` as the fixed committed representative archive for Phase 13. [VERIFIED: `tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json`]
   - Why this is the locked choice: it satisfies the `13-SPEC.md` requirement for one Starfield BA2 GNRL representative archive while also exercising the harder shipped reopen/extraction path through Starfield v3 `compression_method == 3` and `lz4_block` extraction. [VERIFIED: `13-SPEC.md`; VERIFIED: `tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json`]
   - Planning implication: downstream plans and execution should treat `ba2_gnrl_sfv3.ba2` as fixed scope, not an executor-time decision. [VERIFIED: `13-CONTEXT.md`]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Configure/build/test presets for the phase suite | ✓ [VERIFIED: local `cmake --version`] | 4.3.2 [VERIFIED: local `cmake --version`] | — |
| CTest | Quick-run/full-suite validation commands | ✓ [VERIFIED: local `ctest --version`] | 4.3.2 [VERIFIED: local `ctest --version`] | — |
| `VCPKG_ROOT` toolchain path | Existing Windows presets in `CMakePresets.json` | ✓ [VERIFIED: local `$env:VCPKG_ROOT`; VERIFIED: `CMakePresets.json`] | `C:\vcpkg` path present [VERIFIED: local `$env:VCPKG_ROOT`] | — |
| Python | Existing fixture/manifest validation tooling | ✓ [VERIFIED: local `python --version`] | 3.14.5 [VERIFIED: local `python --version`] | — |
| MSVC CLI tools on PATH (`cl`, `msbuild`, `devenv`) | Local compile/build execution from this shell | ✗ [VERIFIED: local `cl`; VERIFIED: local `Get-Command msbuild`; VERIFIED: local `Get-Command devenv`] | — | Use CI or a developer environment that provides the Windows compiler toolchain. [ASSUMED] |

**Missing dependencies with no fallback:**
- None for research itself. [VERIFIED: local environment probes]

**Missing dependencies with fallback:**
- MSVC CLI tools are not present in this shell, so any local build/ctest execution task should either start from a proper Visual Studio environment or rely on CI. [VERIFIED: local `cl`; VERIFIED: local `Get-Command msbuild`; VERIFIED: local `Get-Command devenv`; ASSUMED fallback]

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Catch2 3 via `Catch2::Catch2WithMain`. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `.planning/codebase/TESTING.md`] |
| Config file | `tests/CMakeLists.txt`. [VERIFIED: `tests/CMakeLists.txt`] |
| Quick run command | `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary" --output-on-failure` after Wave 0 adds the dedicated suite. [VERIFIED: `13-VALIDATION.md`; ASSUMED suite name until implemented] |
| Full suite command | `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: `.planning/codebase/TESTING.md`; VERIFIED: `13-VALIDATION.md`] |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| HOST-01 | `archive_reader::open` succeeds for TES4 v103/v104/v105, FO4 BA2 GNRL, FO4 BA2 DX10, and Starfield BA2 GNRL from a non-ASCII directory + filename. [VERIFIED: `13-SPEC.md`; VERIFIED: `13-CONTEXT.md`] | fixture regression | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` [ASSUMED suite name] | ❌ Wave 0 [VERIFIED: glob `tests/unit/*host_path*`] |
| HOST-02 | `validate_archive(non_ascii_path, {.validate_entry_extractability = true})` returns a successful report for the same representative set. [VERIFIED: `13-SPEC.md`] | fixture regression | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` [ASSUMED suite name] | ❌ Wave 0 [VERIFIED: glob `tests/unit/*host_path*`] |
| HOST-03 | Maintainer can run committed regression coverage proving open, validate, and one canonical extraction per representative archive. [VERIFIED: `.planning/REQUIREMENTS.md`; VERIFIED: `13-CONTEXT.md`] | fixture regression + CTest registration | `ctest --preset windows-msvc-debug-static -R host_path_correctness_boundary --output-on-failure` [ASSUMED suite name] | ❌ Wave 0 [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: glob `tests/unit/*host_path*`] |

### Sampling Rate
- **Per task commit:** `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary|writer-source-io" --output-on-failure` once the suite exists. [VERIFIED: `tests/unit/writer_disk_source_tests.cpp`; VERIFIED: `13-VALIDATION.md`; ASSUMED final tag names]
- **Per wave merge:** `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: `.planning/codebase/TESTING.md`] 
- **Phase gate:** Full suite green before `/gsd-verify-work`. [VERIFIED: `13-VALIDATION.md`] 

### Wave 0 Gaps
- [ ] `tests/unit/host_path_correctness_boundary_tests.cpp` — dedicated cross-family non-ASCII regression suite. [VERIFIED: glob `tests/unit/*host_path*`; VERIFIED: `13-CONTEXT.md`]
- [ ] `tests/CMakeLists.txt` — register the new suite in `libbsa_tests`. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `13-VALIDATION.md`]
- [ ] File-local helpers for non-ASCII temp root creation, archive copy, canonical manifest entry lookup, and expected-byte materialization. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`; VERIFIED: `tests/unit/ba2_gnrl_reader_tests.cpp`; VERIFIED: `tests/unit/ba2_dx10_extraction_tests.cpp`; VERIFIED: `13-CONTEXT.md`]
- [ ] Rename/update `tests/unit/writer_disk_source_tests.cpp` and its `tests/CMakeLists.txt` registration when the helper family is renamed. [VERIFIED: `tests/unit/writer_disk_source_tests.cpp`; VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `13-CONTEXT.md`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | Not applicable; libbsa is a local archive library with no identity subsystem. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`] |
| V3 Session Management | no | Not applicable; there is no session concept in this codebase. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`] |
| V4 Access Control | no | Not applicable to this phase; host-path correctness does not add an authorization layer. [VERIFIED: `.planning/codebase/ARCHITECTURE.md`] |
| V5 Input Validation | yes | Keep parser metadata limits, payload-range checks, and validation setup/result separation intact while refactoring path opens. [VERIFIED: `src/detail/parser_primitives.hpp`; VERIFIED: `src/detail/payload_stream.hpp`; VERIFIED: `src/validation.cpp`] |
| V6 Cryptography | no | No new cryptographic behavior in this phase. [VERIFIED: `.planning/PROJECT.md`] |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Unicode host-path confusion on Windows | Tampering / Denial of Service | Resolve once to `std::filesystem::path` and reopen only from the stored resolved path. [VERIFIED: `13-CONTEXT.md`; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax] |
| Truncated or changed archive between size inspection and later reads | Tampering | Reuse the shared helper’s exact-read/changed-source checks and keep payload-range validation in parser/detail helpers. [VERIFIED: `src/detail/writer_disk_source.cpp`; VERIFIED: `src/detail/payload_stream.hpp`; VERIFIED: `src/detail/parser_primitives.hpp`] |
| Oversized extractability allocation from malformed archive metadata | Denial of Service | Preserve `validation_options.max_extractability_entry_bytes` and parser count/range guards during the refactor. [VERIFIED: `include/libbsa/validation.hpp`; VERIFIED: `src/validation.cpp`; VERIFIED: `src/detail/parser_primitives.hpp`] |
| Validation/setup diagnostic drift | Repudiation | Keep result-level setup errors direct and keep report-level diagnostics only for readable archive bytes. [VERIFIED: `include/libbsa/validation.hpp`; VERIFIED: `src/validation.cpp`; VERIFIED: `13-CONTEXT.md`] |

## Sources

### Primary (HIGH confidence)
- `J:/libbsa/.planning/phases/13-host-path-correctness-boundary/13-CONTEXT.md` - locked implementation decisions, representative set, and regression constraints.
- `J:/libbsa/.planning/phases/13-host-path-correctness-boundary/13-SPEC.md` - goal, scope, acceptance criteria, and out-of-scope boundaries.
- `J:/libbsa/src/archive.cpp` - current open-time resolution/storage and extraction dispatch seams.
- `J:/libbsa/src/validation.cpp` - current validation preflight/setup/result split.
- `J:/libbsa/src/detail/writer_disk_source.hpp` and `J:/libbsa/src/detail/writer_disk_source.cpp` - existing shared host-file helper primitives and diagnostics shape.
- `J:/libbsa/src/formats/bsa/tes3_bsa_parser.*`, `tes4_bsa_parser.*`, `tes3_bsa_reader.*`, `tes4_bsa_reader.*` - current parser/extraction reopen contracts.
- `J:/libbsa/src/formats/ba2/ba2_gnrl_parser.*`, `ba2_dx10_parser.*`, `ba2_gnrl_reader.*`, `ba2_dx10_reader.*` - current BA2 parser/extraction reopen contracts.
- `J:/libbsa/tests/CMakeLists.txt` and representative test files under `tests/unit/` - current Catch2/CTest registration and manifest-backed assertion patterns.
- Microsoft Learn: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax - Windows `std::filesystem::path` storage/conversion behavior and `<fstream>` interoperability.
- Microsoft Learn: https://learn.microsoft.com/cpp/standard-library/file-system-navigation?view=msvc-170#paths - path conversion/composition guidance and Windows Unicode path notes.
- Microsoft Learn: https://learn.microsoft.com/cpp/standard-library/basic-ifstream-class?view=msvc-170#%60basic_ifstreambasic_ifstream%60 - `basic_ifstream` filename-opening behavior.

### Secondary (MEDIUM confidence)
- `J:/libbsa/.planning/codebase/ARCHITECTURE.md` - facade/detail layering and validation flow summary.
- `J:/libbsa/.planning/codebase/TESTING.md` - fixture, manifest, and Catch2 testing patterns.
- `J:/libbsa/.planning/codebase/CONCERNS.md` - non-ASCII host-path bug inventory.
- `J:/libbsa/.planning/phases/13-host-path-correctness-boundary/13-VALIDATION.md` - existing per-phase validation proposal and tag naming direction.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - the phase reuses existing repo seams plus documented standard-library path behavior, with no new runtime dependency choices. [VERIFIED: `src/detail/writer_disk_source.cpp`; CITED: https://learn.microsoft.com/cpp/standard-library/filesystem?view=msvc-170#syntax]
- Architecture: HIGH - the exact migration seams are visible in the live code and are further constrained by locked context decisions. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/validation.cpp`; VERIFIED: `13-CONTEXT.md`]
- Pitfalls: HIGH - each listed pitfall is directly grounded in the current call graph, current tests, or locked scope rules. [VERIFIED: codebase grep; VERIFIED: `13-SPEC.md`; VERIFIED: `13-CONTEXT.md`]

**Research date:** 2026-05-13
**Valid until:** 2026-05-20
