# Phase 04: tes3-bsa-read-extract - Research

**Researched:** 2026-05-08  
**Domain:** TES3/Morrowind BSA read, list, lookup, and raw extraction in a C++20 archive library  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
## Implementation Decisions

### Offset Metadata
- **D-01:** Public `entry_metadata::payload_offset` means archive-absolute byte offset for TES3 entries, consistent with TES4-family metadata and consumer expectations. The raw TES3 record offset remains data-section-relative and is not exposed as a second public field.
- **D-02:** Interpret the SPEC's `payload_offset` acceptance checks as public archive-absolute offset checks. Generated TES3 manifests must separately record the raw data-section-relative offset so tests prove the compatibility rule without expanding public metadata.
- **D-03:** If a TES3 fixture entry has raw offset `0`, public `payload_offset` should equal the computed data section start.
- **D-04:** The reader should convert raw TES3 offsets to absolute offsets during parsing/validation and should not retain raw offsets in runtime reader state after validation.
- **D-05:** Invalid TES3 payload spans, raw-offset plus data-section-base overflow, overlapping metadata/name/hash/data regions, and overlapping payload byte ranges fail during `archive_reader::open` with `error_code::format_error`.
- **D-06:** TES3 zero-byte entries are valid when their offset is within the data section; extraction returns an empty payload.
- **D-07:** Do not require TES3 raw file data to be alphabetically ordered, and do not require payload offsets to be sorted by entry/hash order. Any non-overlapping bounded payload order is acceptable.
- **D-08:** Add/update public documentation so `entry_metadata::payload_offset` is explicitly archive-absolute for all variants.
- **D-09:** Offset tests should read archive bytes at asserted absolute offsets for raw entries, not rely only on manifest equality or extraction success.
- **D-10:** Researcher/planner must trace TES5Edit/BSArchPro for the offset conversion behavior and cite both the trace location and public format docs in a code comment near the non-obvious conversion rule.

### Hash Metadata
- **D-11:** Rename the public `entry_metadata::tes4_hash` field to a neutral `archive_hash` field in Phase 4. This is a pre-v1 API cleanup; do not add a backward-compatibility duplicate unless a concrete consumer need appears.
- **D-12:** For TES3 entries, `archive_hash` exposes the stored hash table value from the archive bytes, not a recomputed replacement. Strict validation separately requires it to match the parsed archive name.
- **D-13:** TES3 fixture manifests should include both the single 64-bit hash value used by public metadata and the low/high 32-bit halves used to make TES3 sort-order expectations explicit.
- **D-14:** When validating or recomputing TES3 hashes, use the parsed archive name bytes/spelling with TES3 lower-byte rules as the source of truth, not the public canonical lookup key.

### Strict Hash Checks
- **D-15:** TES3 open must validate each stored hash against the parsed archive name and fail mismatches with `error_code::format_error`.
- **D-16:** TES3 hash collisions between different names fail open with `error_code::format_error`.
- **D-17:** TES3 hash/order-dependent records must be sorted by TES3 hash order. Unsorted hash table/order records are malformed and fail open.
- **D-18:** Mandatory malformed hash fixtures must cover stored hash mismatch, duplicate hash collision, and unsorted hash/order records.
- **D-19:** Tests for hash validation should assert stable error codes only, not diagnostic message text.
- **D-20:** Stored TES3 names containing uppercase ASCII are allowed if they pass path normalization and hash validation under TES3 lower-byte rules. Preserve archive spelling in `original_path` and normalize lookup independently.
- **D-21:** Stored TES3 names using `/` instead of `\` are allowed if they pass archive path normalization and hash validation. Reject malformed paths or hash mismatches, not safe separator variants.

### Fixture Shape
- **D-22:** Phase 4 success proof should use one rich committed TES3 success archive rather than a broad archive matrix.
- **D-23:** The rich success archive should contain representative Morrowind-like paths across folders/extensions, multiple entries, lookup variants, TES3 hashes, public absolute offsets, raw data-section-relative offset proof, and extraction payload bytes. Do not make the success archive edge-heavy with every tolerated oddity.
- **D-24:** Add a separate TES3-specific generated fixture tool/source while reusing shared helpers where sensible. Do not hand-author opaque fixture bytes without generator provenance.
- **D-25:** Mandatory malformed fixture coverage is the SPEC set plus the strict hash cases: truncated structures, invalid name spans, invalid payload spans, duplicate canonical paths, inconsistent counts/offsets, stored hash mismatch, hash collision, unsorted hash/order records, and a regression case that would fail if raw TES3 offsets were treated as archive-absolute.

### the agent's Discretion
- Planner may choose exact internal parser/source names, detector dispatch mechanics, and helper boundaries if the public API and validation decisions above are preserved.
- Planner may choose exact generated entry names and payload bytes for the representative TES3 fixture, as long as they prove the locked metadata, lookup, hash, and offset decisions.
- Planner may decide the exact wording of diagnostics and comments, but tests should assert stable error codes rather than exact messages.

### Deferred Ideas (OUT OF SCOPE)
## Deferred Ideas

None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| BSA-04 | Consumer can read and extract files from TES3/Morrowind BSA archives. [VERIFIED: `.planning/REQUIREMENTS.md`] | Use byte-driven TES3 detection, a TES3 parser, shared reader lookup helpers, raw payload extraction, and committed generated fixtures. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `include/libbsa/archive.hpp`; VERIFIED: `src/archive.cpp`] |
| BSA-08 | Consumer can extract TES3 entries using data-section-relative offset semantics. [VERIFIED: `.planning/REQUIREMENTS.md`] | Convert raw TES3 record offsets to archive-absolute offsets at parse time: `absolute_payload_offset = data_section_start + raw_record_offset`, then reuse the existing archive-absolute extraction seek path. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1128-1129,2114-2118`; CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format] |
</phase_requirements>

## Summary

Phase 4 should add TES3/Morrowind BSA support as a format-specific parser/reader extension behind the existing `archive_reader` facade, not as a new public API. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `include/libbsa/archive.hpp`] The current implementation opens TES4-family BSA only: `detect_bsa_format` requires `BSA\0` magic and versions 103/104/105, and `archive_reader::open` always dispatches to `parse_tes4_bsa_archive_file`. [VERIFIED: `src/formats/bsa/bsa_format_detector.cpp`; VERIFIED: `src/archive.cpp`]

The key implementation rule is TES3's data-section-relative payload offset model. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format] UESP documents each file record offset as relative to the data section, and TES5Edit reads the hash table, records `fDataOffset := fStream.Position`, and extracts TES3 by seeking to `fDataOffset + FileTES3.Offset`. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1120-1129,2114-2118`] Public metadata should still expose archive-absolute `payload_offset` for all variants, per locked Phase 4 decisions. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

The planner should sequence this phase around fixtures first, then parser/detector integration, then reader dispatch and extraction verification. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`; VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`] No new runtime dependency is needed for TES3 read/extract because TES3 payloads are raw/uncompressed in scope, and the existing C++20/CMake/Catch2/nlohmann-json test stack already supports the required generated fixture and manifest workflow. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `CMakeLists.txt`; VERIFIED: `tests/CMakeLists.txt`]

**Primary recommendation:** Build `tes3_bsa_parser` + `tes3_bsa_reader` beside the TES4-family implementation, extend BSA detection/dispatch to route TES3 magic `0x00000100`, convert TES3 raw offsets to archive-absolute offsets during parse validation, rename public `tes4_hash` to `archive_hash`, and prove everything with one rich generated success fixture plus focused malformed fixtures. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format]

## Project Constraints (from AGENTS.md)

- The implementation language is C++, and public interfaces should be clean, reusable, and portable rather than Delphi/Pascal transliterations. [VERIFIED: `AGENTS.md`]
- `TES5Edit/` is a read-only reference submodule: do not edit, format, stage, commit, compile, vendor, or use it as a fixture workspace. [VERIFIED: `AGENTS.md`]
- Preserve archive-format behavior discovered from BSArchPro unless there is a documented reason to diverge. [VERIFIED: `AGENTS.md`]
- When porting non-obvious reference behavior, trace the reference code first and record compatibility constraints near the new implementation. [VERIFIED: `AGENTS.md`]
- Do not introduce speculative external dependencies; use the standard library until a concrete format, compression, filesystem, testing, or packaging need justifies more. [VERIFIED: `AGENTS.md`]
- Approved dependency policy remains `libdeflate`, official `lz4`, `DirectXTex`, `vcpkg`, Catch2, and current test-only nlohmann-json. [VERIFIED: `AGENTS.md`; VERIFIED: `tests/CMakeLists.txt`]
- Never delete accurate comments as cleanup; add comments for non-obvious compatibility, ownership/lifetime, error-handling, threading, cancellation, and deliberate reference divergences. [VERIFIED: `AGENTS.md`]
- Add Doxygen-compliant C++ doc comments for public APIs and methods added or substantially rewritten. [VERIFIED: `AGENTS.md`]
- Add focused fixture-based tests for archive parsing, writing, round-tripping, and compatibility behavior as those surfaces are implemented. [VERIFIED: `AGENTS.md`]
- Do not use `TES5Edit/` as a mutable test fixture. [VERIFIED: `AGENTS.md`]
- Project-local `.claude/skills/` and `.agents/skills/` directories are absent, so no project-specific skill workflow modifies this phase. [VERIFIED: glob `.claude/skills/**/SKILL.md`; VERIFIED: glob `.agents/skills/**/SKILL.md`]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| TES3 byte detection | Library API / format detection | BSA format parser | `archive_reader::open` already reads a detection prefix and delegates to BSA detector/parser internals. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/formats/bsa/bsa_format_detector.cpp`] |
| TES3 table parsing and validation | BSA format parser | Binary I/O primitives | TES3 header, size/offset table, name offsets, name strings, and hashes are archive-table concerns read from little-endian bytes. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `src/detail/binary_io.hpp`] |
| TES3 path normalization and lookup | Shared reader/helper layer | Format parser | Public lookup must use existing archive virtual path normalization while preserving parsed `original_path`. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; VERIFIED: `src/detail/archive_path.hpp`] |
| TES3 hash validation | Format parser | `detail::hash_tes3` | Stored hashes must be compared with computed TES3 hashes from parsed archive name bytes/spelling, and collisions/order issues fail open. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; VERIFIED: `src/detail/bethesda_hash.cpp`] |
| Data-section-relative offset conversion | Format parser | Extraction seek helper | Raw offsets must be converted to absolute `entry_metadata::payload_offset` before reader state is stored so existing extraction can seek absolute positions. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:2114-2118`] |
| Raw payload extraction | `archive_reader` extraction path | TES3 reader helper | TES3 payloads are uncompressed in scope, so extraction should read exactly the selected entry span and stream to the caller sink. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `src/archive.cpp`] |
| Fixture generation and validation | Test tooling | Unit tests / CTest | Phase 4 acceptance requires legal generated success and malformed TES3 fixtures without local game archives. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `tests/CMakeLists.txt`] |

## Standard Stack

### Core

| Library / Component | Version / Policy | Purpose | Why Standard |
|---------------------|------------------|---------|--------------|
| C++ | C++20 via `target_compile_features(libbsa PUBLIC cxx_std_20)` | Library implementation and public API | Existing project standard; public API must remain C++20-compatible and avoid `std::expected`. [VERIFIED: `CMakeLists.txt`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| CMake | Minimum 3.24; local tool reports 4.3.2 | Build orchestration and target source registration | Existing project uses CMake target sources, install file sets, and CTest test presets. [VERIFIED: `CMakeLists.txt`; VERIFIED: `CMakePresets.json`; VERIFIED: `cmake --version`] |
| vcpkg | Manifest mode via `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake`; local vcpkg executable reports `2026-04-08-e0612b42...` | Dependency acquisition | Existing presets require vcpkg toolchain; local `VCPKG_ROOT=C:\vcpkg` exists even though `vcpkg` is not on PATH. [VERIFIED: `CMakePresets.json`; VERIFIED: shell `& "$env:VCPKG_ROOT\vcpkg.exe" version`] |
| `archive_reader` public facade | Existing public API | Open, metadata, entries, find, contains, extract, and extract_bytes | Phase 4 must extend this API rather than adding TES3-specific public parser/extractor classes. [VERIFIED: `include/libbsa/archive.hpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| `detail::hash_tes3` | Existing internal helper | TES3 hash validation and fixture generation | Existing implementation traces TES5Edit `CreateHashTES3` and applies ASCII-only lower-byte rules. [VERIFIED: `src/detail/bethesda_hash.cpp`; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:705-731`] |
| `detail::binary_reader` | Existing internal helper | Checked little-endian reads and bounded table parsing | Phase 2 established checked binary I/O for truncation/overflow-safe parsers. [VERIFIED: `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md`; VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`] |

### Supporting

| Library / Component | Version / Policy | Purpose | When to Use |
|---------------------|------------------|---------|-------------|
| Catch2 | vcpkg test dependency; tests linked as `Catch2::Catch2WithMain` | Fixture-backed unit tests | Use for TES3 detection, metadata, listing, lookup, extraction, and malformed assertions. [VERIFIED: `tests/CMakeLists.txt`] |
| CTest | Bundled with CMake 4.3.2 locally | Test orchestration and labels | Use `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` for quick gate and a full preset run for phase gate. [VERIFIED: `ctest --version`; VERIFIED: `CMakePresets.json`; VERIFIED: `tests/CMakeLists.txt`] |
| nlohmann-json | Test-only PRIVATE linkage in `libbsa_tests` | Manifest parsing | Reuse existing test-only exception for generated TES3 fixture manifests; do not link into libbsa runtime/public API. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `.planning/STATE.md`] |
| Existing fixture generator pattern | `generate_tes4_bsa_fixtures_tool` pattern | Legal generated archive bytes and manifests | Add a separate TES3 generator target/source while reusing helper idioms such as byte_buffer, manifest JSON, and hex payload encoding. [VERIFIED: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Separate `tes3_bsa_parser` internals | Extend `tes4_bsa_parser` with TES3 branches | Separate internals keep TES3's flat table/hash/relative-offset behavior isolated; branching the TES4 parser risks regressions in Phase 3 behavior. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |
| Shared path lookup helper | Duplicate `find_tes4_bsa_entry` as `find_tes3_bsa_entry` | A neutral shared helper avoids format-named public behavior and supports later BA2 readers; planner may choose exact boundary. [VERIFIED: `src/formats/bsa/tes4_bsa_reader.hpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |
| Generated legal fixtures | Real Morrowind archives or BSArchPro golden files | Real archives/golden outputs may supplement locally, but default CI cannot require copyrighted game data or TES5Edit mutation. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `AGENTS.md`] |
| Existing public `payload_offset` only | Add `tes3_raw_payload_offset` to public metadata | Locked decisions require public archive-absolute `payload_offset` and raw offset only in fixture manifests/tests. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |

**Installation:** No new runtime package should be added for Phase 4. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `CMakeLists.txt`]

```bash
# Existing configure/build/test flow; no new package install required for TES3 raw reads.
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static -L unit --output-on-failure
```

**Version verification:** This phase does not add a new package, so registry version checks are not applicable. [VERIFIED: `CMakeLists.txt`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer host path
  |
  v
archive_reader::open(host_path)
  |
  +--> read detection prefix + archive size
  |
  +--> detect_bsa_format(bytes)
        |
        +--> TES3 magic 0x00000100? ---> parse_tes3_bsa_archive_file
        |                                 |
        |                                 +--> read header: magic, hash_offset_minus_12, file_count
        |                                 +--> read file size/raw offset records
        |                                 +--> read and validate name offsets + names
        |                                 +--> read and validate hash table/order/collisions
        |                                 +--> compute data_section_start
        |                                 +--> convert raw offsets to absolute payload_offset
        |                                 +--> validate payload spans and no overlaps
        |                                 +--> materialize archive_metadata + entry_metadata[]
        |
        +--> BSA\0 v103/v104/v105? ---> parse_tes4_bsa_archive_file (existing)
        |
        +--> otherwise unsupported
  |
  v
archive_reader state: metadata + sorted entries + host_path + variant/family dispatch
  |
  +--> entries() / find() / contains() use canonical archive-path lookup
  |
  +--> extract(path, sink)
        |
        +--> find canonical entry
        +--> seek archive-absolute payload_offset
        +--> read exactly stored_size raw bytes
        +--> write to sink; partial sink write => io_error
```

The diagram reflects the existing `archive_reader` facade and detector/parser split. [VERIFIED: `src/archive.cpp`; VERIFIED: `src/formats/bsa/bsa_format_detector.cpp`; VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`]

### Recommended Project Structure

```text
include/libbsa/
├── archive.hpp                         # Rename tes4_hash -> archive_hash and document archive-absolute payload_offset. [VERIFIED: include/libbsa/archive.hpp]
src/
├── archive.cpp                         # Dispatch opened reader state by detected variant/family. [VERIFIED: src/archive.cpp]
├── formats/bsa/
│   ├── bsa_format_detector.*           # Extend detection for TES3 magic 0x00000100. [VERIFIED: src/formats/bsa/bsa_format_detector.cpp; CITED: UESP]
│   ├── tes3_bsa_parser.*               # New TES3 table/hash/name/payload-span parser. [VERIFIED: 04-CONTEXT]
│   ├── tes3_bsa_reader.*               # New or shared lookup/raw extraction helper boundary. [VERIFIED: 04-CONTEXT]
│   ├── tes4_bsa_parser.*               # Existing TES4 parser; avoid behavior rewrites beyond neutral field rename. [VERIFIED: src/formats/bsa/tes4_bsa_parser.cpp]
│   └── tes4_bsa_reader.*               # Existing reader helpers; consider neutralizing names if shared. [VERIFIED: src/formats/bsa/tes4_bsa_reader.hpp]
tests/
├── fixtures/generated/
│   ├── generate_tes3_bsa_fixtures.cpp   # New generator with manifest/raw-offset/hash proof. [VERIFIED: 04-CONTEXT]
│   └── archives/                       # Committed TES3 success/malformed outputs. [VERIFIED: 04-SPEC]
└── unit/
    └── tes3_bsa_reader_tests.cpp        # New fixture-backed Phase 4 tests. [VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp pattern]
```

### Pattern 1: Parse TES3 Tables as One Metadata Region, Then Convert Offsets

**What:** Read TES3 records in exact section order: 12-byte header, `file_count` size/raw-offset pairs, `file_count` name offsets, zstring names, `file_count` hashes, then raw data. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format]  
**When to use:** During `parse_tes3_bsa_archive_file` before storing any reader state. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]  
**Example:**

```cpp
// Source: UESP TES3 BSA format + TES5Edit/Core/wbBSArchive.pas:1120-1129,2114-2118.
// TES3 stores payload offsets relative to the raw data section, but libbsa exposes
// archive-absolute payload_offset for all variants.
const std::uint64_t absolute_payload_offset = data_section_start + raw_record_offset;
if (absolute_payload_offset < data_section_start || !span_fits(absolute_payload_offset, size, archive_size)) {
  return error{error_code::format_error, "TES3 BSA entry payload span is outside the archive"};
}
```

### Pattern 2: Validate Hashes Against Parsed Archive Names, Not Canonical Lookup Keys

**What:** Compute TES3 hashes from the parsed archive name spelling/bytes after safe separator acceptance, compare with the stored hash, reject mismatches/collisions, then independently normalize to canonical public lookup keys. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; VERIFIED: `src/detail/bethesda_hash.cpp`]  
**When to use:** After parsing all names and hash records and before sorting public entries by canonical path. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]  
**Example:**

```cpp
// Source: TES5Edit/Core/wbBSArchive.pas:705-731 and src/detail/bethesda_hash.cpp.
const auto computed = detail::hash_tes3(parsed_archive_name);
if (stored_hash != computed) {
  return error{error_code::format_error, "TES3 BSA stored hash does not match parsed name"};
}
```

### Pattern 3: Keep Public Extraction Sink-First and Variant-Neutral

**What:** `extract(path, sink)` remains the primary API, and `extract_bytes(path)` remains a bounded convenience wrapper over `extract`. [VERIFIED: `include/libbsa/archive.hpp`; VERIFIED: `src/archive.cpp`]  
**When to use:** For TES3 raw payloads, after entries expose absolute offsets and `entry_compression::none`. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]  
**Example:**

```cpp
// Source: existing archive_reader extraction contract in include/libbsa/archive.hpp and src/archive.cpp.
auto bytes = reader.extract_bytes("Meshes\\Tiny\\Probe.nif");
REQUIRE(bytes.has_value());
REQUIRE(bytes.value() == expected_payload);
```

### Anti-Patterns to Avoid

- **Treating TES3 raw offsets as archive-absolute:** This fails the defining BSA-08 compatibility rule because TES3 offsets are relative to the raw data section. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:2114-2118`]
- **Keeping raw TES3 offsets in public/runtime state:** Locked decisions require archive-absolute public `payload_offset` and no retained raw offset after validation. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]
- **Hashing canonical lowercase `/` lookup keys for validation:** Locked decisions require parsed archive name bytes/spelling with TES3 lower-byte rules as the validation input. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]
- **Adding compression routing for TES3:** TES3 entries are raw/uncompressed in Phase 4, and codec invocation is out of scope. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]
- **Using `std::filesystem::path` for archive-internal names:** Archive virtual paths must use project normalization, not host filesystem semantics. [VERIFIED: `AGENTS.md`; VERIFIED: `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md`]
- **Mutating or generating fixtures under `TES5Edit/`:** The submodule is read-only reference material only. [VERIFIED: `AGENTS.md`; VERIFIED: `git -C TES5Edit status --short`]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Path normalization | Ad-hoc lowercase/slash logic in TES3 parser | `detail::normalize_archive_path` | Existing public lookup semantics already reject rooted/traversal/empty-component paths. [VERIFIED: `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md`; VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`] |
| TES3 hash algorithm | New independent hash implementation in parser/generator | `detail::hash_tes3` | Existing helper traces TES5Edit and should be the single source for parser validation and generated fixtures. [VERIFIED: `src/detail/bethesda_hash.cpp`; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:705-731`] |
| Binary bounds checks | Pointer arithmetic over raw buffers | `detail::binary_reader` plus checked arithmetic helpers | Existing TES4 parser pattern guards truncation and overflow before allocation/reads. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`] |
| JSON manifest parsing | Custom JSON parser | Existing test-only nlohmann-json setup | nlohmann-json is already PRIVATE test linkage and must not leak into libbsa. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `.planning/STATE.md`] |
| Compression/decompression | Any TES3 codec path | No codec for TES3 Phase 4 | TES3 scope is raw/uncompressed payloads only. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| Reference compatibility | Copying TES5Edit source or compiling it | Read-only trace + independent C++ implementation | Project boundary forbids editing, compiling, vendoring, staging, or treating TES5Edit as source. [VERIFIED: `AGENTS.md`] |

**Key insight:** The complex part is not raw extraction itself; it is validating TES3's flat hash-sorted metadata tables while translating data-section-relative offsets into the existing archive-absolute reader model. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

## Common Pitfalls

### Pitfall 1: Offset Base Confusion
**What goes wrong:** Parser exposes or seeks to raw TES3 offsets as if they were archive-absolute offsets. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format]  
**Why it happens:** TES4-family entries store archive-absolute offsets, while TES3 file records store offsets relative to the raw data section. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`]  
**How to avoid:** Compute `data_section_start` after the hash table, validate `data_section_start + raw_offset + size`, store only the absolute value in `entry_metadata::payload_offset`, and add an implementation comment citing UESP and TES5Edit. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1128-1129,2114-2118`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]  
**Warning signs:** A fixture with raw offset `0` reports public `payload_offset == 0` instead of `data_section_start`, or extraction reads metadata bytes. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

### Pitfall 2: Misreading `HashOffset`
**What goes wrong:** Parser treats TES3 `HashOffset` as the direct start of the hash table from file start. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format]  
**Why it happens:** TES3 stores “offset of the hash table in the file, minus the header size (12)”; TES5Edit's writer sets `fHeaderTES3.HashOffset := fDataOffset - 12`. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1371-1379`]  
**How to avoid:** Validate parsed table positions against `hash_table_start == 12 + header.hash_offset` and against the end of the names section. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1593-1616`]  
**Warning signs:** Truncated names/hash fixtures parse successfully, or malformed name spans shift hash reads into name data. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]

### Pitfall 3: Hash Order and Half Ordering
**What goes wrong:** Parser compares serialized 64-bit hash values in the wrong order or ignores sort order. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]  
**Why it happens:** UESP states hashes are sorted first by lower four bytes, then higher four bytes; TES5Edit writes `Hash shr 32` then `Hash and $FFFFFFFF`, while the existing helper returns a single `uint64_t`. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1612-1616`; VERIFIED: `src/detail/bethesda_hash.cpp`]  
**How to avoid:** Fixture manifests should store the public 64-bit hash and the explicit low/high 32-bit halves, and parser/tests should define the comparison key directly. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; CITED: https://docs.rs/bsa3-hash/latest/bsa3_hash/]  
**Warning signs:** A deliberately unsorted hash/order fixture opens successfully. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

### Pitfall 4: Name Offset Validation Gaps
**What goes wrong:** Parser reads names sequentially and ignores name offsets, allowing invalid spans or duplicate offsets to pass. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]  
**Why it happens:** TES5Edit's read path skips name offsets and reads zstrings sequentially, but Phase 4 acceptance explicitly requires invalid name span coverage. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1120-1124`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]  
**How to avoid:** Use the public format layout to validate each name offset points within the names section and resolves to a null-terminated non-empty string before the hash table. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]  
**Warning signs:** A fixture with an out-of-range name offset or missing terminator opens successfully. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]

### Pitfall 5: Over-Reworking Phase 3 TES4 Behavior
**What goes wrong:** Refactoring detector/reader dispatch breaks verified TES4-family behavior. [VERIFIED: `.planning/STATE.md`]  
**Why it happens:** Current `archive_reader` methods are hard-coded to TES4 helpers, so Phase 4 requires dispatch changes in shared code. [VERIFIED: `src/archive.cpp`]  
**How to avoid:** Add variant-aware dispatch with shared neutral helpers where possible and keep all existing TES4 tests green after the `tes4_hash` → `archive_hash` rename. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]  
**Warning signs:** TES4 fixture tests change semantics beyond field rename updates. [VERIFIED: `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md`]

## Code Examples

Verified patterns from project code and reference material:

### TES3 Header/Table Shape

```cpp
// Source: UESP TES3 BSA format and TES5Edit/Core/wbBSArchive.pas:242-251.
struct tes3_header_fields {
  std::uint32_t hash_offset_minus_header; // hash table file offset minus 12 bytes.
  std::uint32_t file_count;
};

struct tes3_file_record {
  std::uint32_t size;
  std::uint32_t raw_data_section_offset;
};
```

### Existing Hash Helper Usage

```cpp
// Source: src/detail/bethesda_hash.cpp and TES5Edit/Core/wbBSArchive.pas:705-731.
const std::uint64_t computed_hash = libbsa::detail::hash_tes3(parsed_archive_name);
```

### Fixture Manifest Should Prove Both Public and Raw Offset Semantics

```json
{
  "path": "meshes/tiny/probe.nif",
  "original_path": "Meshes/Tiny/Probe.nif",
  "raw_tes3_data_offset": 0,
  "payload_offset": 96,
  "hash": "00020336bb500695",
  "hash_low32": "bb500695",
  "hash_high32": "00020336",
  "expected_hex": "70726f62655f6279746573"
}
```

This shape follows the locked requirement to keep raw TES3 offsets in manifests/tests rather than public runtime state. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

## State of the Art

| Old Approach | Current Approach | When Changed / Source | Impact |
|--------------|------------------|------------------------|--------|
| TES3 offsets exposed or treated as raw data-section-relative values | Public `entry_metadata::payload_offset` is archive-absolute for all variants; raw TES3 offsets are fixture proof only | Locked Phase 4 D-01 through D-04. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] | Parser must convert offsets during validation and update public docs. |
| TES4-specific public hash field name `tes4_hash` | Format-neutral `archive_hash` | Locked Phase 4 D-11. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] | Public header and all TES4 tests must be updated consistently. |
| TES4-only detector dispatch | Variant/family dispatch that can route TES3 and TES4-family BSA | Phase 4 scope. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `src/archive.cpp`] | `archive_reader::open`, `entries`, `find`, `contains`, and `extract` need variant-aware or neutral helper routing. |
| Hash helper proven only by unit vectors | Hash helper reused for parser validation and fixture generation | Phase 4 locked decisions. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; VERIFIED: `src/detail/bethesda_hash.cpp`] | Stored TES3 hash mismatches, collisions, and unsorted records become malformed open failures. |

**Deprecated/outdated:**
- The name `tes4_hash` is now outdated because Phase 4 exposes TES3 hash metadata through the same entry metadata shape. [VERIFIED: `include/libbsa/archive.hpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]
- TES4-only method names for lookup/extraction helpers are implementation naming debt if those helpers become shared. [VERIFIED: `src/formats/bsa/tes4_bsa_reader.hpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|

**If this table is empty:** All claims in this research were verified or cited — no user confirmation needed.

## Open Questions (RESOLVED)

1. **Exact TES3 hash sort comparator representation — RESOLVED**
   - What we know: UESP says hashes sort first by lower four bytes, then higher four bytes; TES5Edit writes high 32 bits then low 32 bits from its `UInt64` representation; docs.rs `bsa3_hash` returns a `(u32, u32)` pair with known Morrowind examples. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `TES5Edit/Core/wbBSArchive.pas:1612-1616`; CITED: https://docs.rs/bsa3-hash/latest/bsa3_hash/]
   - What's unclear: Whether the implementation should encode the comparator in terms of serialized table order or `detail::hash_tes3`'s current high/low packing to minimize mistakes. [VERIFIED: `src/detail/bethesda_hash.cpp`]
   - Resolution: Planner should include named helper functions for `tes3_hash_low32`, `tes3_hash_high32`, and `tes3_hash_sort_key` with fixture assertions for at least one docs.rs known vector and the generated fixture names. [CITED: https://docs.rs/bsa3-hash/latest/bsa3_hash/]
2. **Whether to fully neutralize TES4 reader helper names now — RESOLVED**
   - What we know: `archive_reader` currently calls `tes4_bsa_entries`, `find_tes4_bsa_entry`, `contains_tes4_bsa_entry`, and `extract_tes4_bsa_payload` directly. [VERIFIED: `src/archive.cpp`]
   - What's unclear: Exact naming/refactor size belongs to planner discretion. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]
   - Resolution: Planner should prefer small neutral helpers for entry copy/find/contains/raw sink transfer if they reduce duplicate code without changing Phase 3 behavior; avoid a broad helper rename if a TES3-specific helper is smaller. [VERIFIED: `src/formats/bsa/tes4_bsa_reader.hpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]
3. **Exact malformed fixture file organization — RESOLVED**
   - What we know: Phase 4 requires malformed coverage for truncated structures, invalid name spans, invalid payload spans, duplicate canonical paths, inconsistent counts/offsets, stored hash mismatch, hash collision, unsorted records, and raw-offset regression. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]
   - What's unclear: Whether all malformed outputs should live in one `tes3_malformed_manifest.json` or share the existing `malformed_manifest.json`. [VERIFIED: `tests/fixtures/generated/archives/malformed_manifest.json`]
   - Resolution: Use a TES3-specific malformed manifest to avoid mixing TES4 and TES3 case metadata during test iteration. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build libbsa and fixture generators | ✓ | 4.3.2 | — [VERIFIED: `cmake --version`] |
| CTest | Run Catch2 tests through presets/labels | ✓ | 4.3.2 | Direct `libbsa_tests` execution if needed. [VERIFIED: `ctest --version`; VERIFIED: `tests/CMakeLists.txt`] |
| vcpkg executable | Dependency install via preset toolchain | ✓ via `C:\vcpkg\vcpkg.exe`; not on PATH | 2026-04-08-e0612b42... | Use `$env:VCPKG_ROOT\vcpkg.exe`; presets already reference `$env{VCPKG_ROOT}`. [VERIFIED: shell `VCPKG_ROOT`; VERIFIED: `CMakePresets.json`] |
| TES5Edit submodule | Read-only reference tracing | ✓ | clean status | Do not mutate; read-only source references only. [VERIFIED: `git -C TES5Edit status --short`; VERIFIED: `AGENTS.md`] |
| Catch2 | Unit tests | ✓ via vcpkg manifest/configure | vcpkg-managed | Required by existing tests; no alternate test framework recommended. [VERIFIED: `tests/CMakeLists.txt`] |
| nlohmann-json | Test fixture manifests | ✓ via vcpkg manifest/configure | vcpkg-managed | Could hand-parse manifests, but existing test-only dependency is already accepted. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `.planning/STATE.md`] |

**Missing dependencies with no fallback:**
- None identified for planning. [VERIFIED: shell environment checks; VERIFIED: `CMakePresets.json`]

**Missing dependencies with fallback:**
- `vcpkg` is not on PATH, but `$env:VCPKG_ROOT\vcpkg.exe` exists and CMake presets use `$env{VCPKG_ROOT}` directly. [VERIFIED: shell `vcpkg version`; VERIFIED: shell `& "$env:VCPKG_ROOT\vcpkg.exe" version`; VERIFIED: `CMakePresets.json`]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 through `Catch2::Catch2WithMain`, discovered by CTest with tags as labels. [VERIFIED: `tests/CMakeLists.txt`] |
| Config file | `tests/CMakeLists.txt` plus `CMakePresets.json`. [VERIFIED: `tests/CMakeLists.txt`; VERIFIED: `CMakePresets.json`] |
| Quick run command | `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` [VERIFIED: `CMakePresets.json`; VERIFIED: `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md`] |
| Full suite command | `ctest --preset windows-msvc-debug-static --output-on-failure` [VERIFIED: `CMakePresets.json`] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| BSA-04 | Open TES3 archive and expose archive metadata/listing/lookup through `archive_reader` | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_metadata --output-on-failure` | ❌ Wave 0: add `tests/unit/tes3_bsa_reader_tests.cpp` [VERIFIED: `tests/CMakeLists.txt`] |
| BSA-04 | Deterministic TES3 `entries()` metadata matches manifest including canonical/original paths, sizes, absolute offsets, hash, compression, embedded-name fields | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_entries --output-on-failure` | ❌ Wave 0 [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| BSA-04 | `find()` and `contains()` accept case/separator variants, return empty for valid missing, and invalid_argument for invalid paths | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_lookup --output-on-failure` | ❌ Wave 0 [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| BSA-08 | `extract()` and `extract_bytes()` byte-compare payloads using data-section-relative raw offsets converted to absolute offsets | unit + fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_extract --output-on-failure` | ❌ Wave 0 [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| BSA-04/BSA-08 | Malformed TES3 fixtures fail with stable error codes: truncated structures, invalid names, invalid payload spans, duplicates, inconsistent counts/offsets, hash mismatch/collision/unsorted records | unit + malformed fixture | `ctest --preset windows-msvc-debug-static -L tes3_bsa_malformed --output-on-failure` | ❌ Wave 0 [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |
| BSA-04/BSA-08 | Existing TES4-family tests still pass after dispatch and `archive_hash` rename | regression | `ctest --preset windows-msvc-debug-static -L tes4_bsa --output-on-failure` | ✅ Existing `tests/unit/tes4_bsa_reader_tests.cpp` needs field rename edits. [VERIFIED: `tests/unit/tes4_bsa_reader_tests.cpp`] |

### Sampling Rate
- **Per task commit:** `ctest --preset windows-msvc-debug-static -L unit --output-on-failure` after build. [VERIFIED: `CMakePresets.json`]
- **Per wave merge:** `ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: `CMakePresets.json`]
- **Phase gate:** Full suite green, generated fixture outputs committed, public include boundary tests pass, and `git -C TES5Edit status --short` remains empty before `/gsd-verify-work`. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: shell `git -C TES5Edit status --short`]

### Wave 0 Gaps
- [ ] `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` — generates rich TES3 success archive, raw-offset manifest, hash halves, and malformed fixtures. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]
- [ ] `tests/fixtures/generated/archives/tes3_*.bsa` and `tes3_*_manifest.json` — committed generated outputs for default CI. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]
- [ ] `tests/unit/tes3_bsa_reader_tests.cpp` — covers BSA-04 and BSA-08 detection/list/lookup/extract/malformed behavior. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]
- [ ] `tests/CMakeLists.txt` — add TES3 unit source and fixture generator target/custom target. [VERIFIED: `tests/CMakeLists.txt`]
- [ ] Existing tests and public header assertions need `tes4_hash` → `archive_hash` updates. [VERIFIED: `include/libbsa/archive.hpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | No authentication surface exists in Phase 4. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| V3 Session Management | no | No session state exists in Phase 4. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`] |
| V4 Access Control | no | No authorization model exists; archive access is local file input supplied by consumer. [VERIFIED: `.planning/REQUIREMENTS.md`] |
| V5 Input Validation | yes | Checked little-endian reads, checked arithmetic, strict table/name/hash/payload validation, stable `format_error`/`unsupported`/`invalid_argument`. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`; VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`] |
| V6 Cryptography | no | TES3 hash is a format lookup/sort hash, not a security primitive; no cryptographic control is introduced. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `src/detail/bethesda_hash.cpp`] |

### Known Threat Patterns for C++ Binary Archive Parser

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Out-of-bounds reads from truncated tables | Tampering / Denial of Service | Use `detail::binary_reader`, span-fit checks, and return `error_code::format_error`. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`] |
| Integer overflow in count × record-size arithmetic | Tampering / Denial of Service | Use checked multiply/add before allocation and span validation. [VERIFIED: `src/formats/bsa/tes4_bsa_parser.cpp`] |
| Malicious payload offset/size targeting metadata or beyond EOF | Tampering / Denial of Service | Convert TES3 raw offsets with overflow checks and validate non-overlapping bounded spans during open. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |
| Duplicate normalized paths causing ambiguous extraction | Tampering | Reject duplicate canonical paths as `format_error`. [VERIFIED: `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |
| Hash collision or mismatch hiding wrong content | Tampering | Validate stored hash against parsed name, reject collisions and unsorted hash/order records. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`] |
| Partial sink writes reported as success | Tampering / Reliability | Preserve existing sink contract: partial acceptance is `error_code::io_error`. [VERIFIED: `include/libbsa/archive.hpp`; VERIFIED: `src/archive.cpp`] |

## Sources

### Primary (HIGH confidence)
- `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md` — locked Phase 4 implementation decisions, scope, canonical refs, code insights, and deferred items. [VERIFIED]
- `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md` — locked Phase 4 requirements, boundaries, constraints, and acceptance criteria. [VERIFIED]
- `.planning/REQUIREMENTS.md` — BSA-04 and BSA-08 requirement definitions and traceability. [VERIFIED]
- `.planning/ROADMAP.md` — Phase 4 goal, dependencies, success criteria, and downstream ordering. [VERIFIED]
- `.planning/STATE.md` — completed Phase 1–3 decisions and current focus. [VERIFIED]
- `AGENTS.md` — read-only TES5Edit boundary, dependency policy, documentation/comment policy, validation expectations. [VERIFIED]
- `include/libbsa/archive.hpp` — public reader, metadata, `entry_metadata::tes4_hash`, and extraction sink shape. [VERIFIED]
- `src/archive.cpp` — current TES4-only open/reader/extraction dispatch and absolute seek behavior. [VERIFIED]
- `src/formats/bsa/bsa_format_detector.*` — current TES4-family byte detector shape. [VERIFIED]
- `src/formats/bsa/tes4_bsa_parser.*` and `tes4_bsa_reader.*` — existing parser/reader patterns and testable validation style. [VERIFIED]
- `src/detail/bethesda_hash.cpp` — existing TES3 hash helper and TES5Edit compatibility comment. [VERIFIED]
- `tests/CMakeLists.txt`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` — test/fixture patterns. [VERIFIED]
- `TES5Edit/Core/wbBSArchive.pas` — TES3 structs, `CreateHashTES3`, read/write layout, `fDataOffset`, and extraction seek reference. [VERIFIED]

### Secondary (MEDIUM-HIGH confidence)
- https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format — public TES3 BSA layout, magic, `HashOffset` meaning, data-section-relative offsets, hash sorting, and hash calculation. [CITED]
- https://mwse.github.io/MWSE/types/tes3archiveOffsetSizeData/ — consumer-facing offset/size data wording for Morrowind archive metadata. [CITED]
- https://docs.rs/bsa3-hash/latest/bsa3_hash/ and https://docs.rs/bsa3-hash/latest/src/bsa3_hash/lib.rs.html — independent TES3 hash representation as two 32-bit values and known vector. [CITED]

### Tertiary (LOW confidence)
- None. [VERIFIED: all findings above are sourced from project files, read-only reference code, or cited public documentation]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new dependency is required; existing CMake/C++20/Catch2/nlohmann-json/vcpkg setup was verified from project files and environment commands. [VERIFIED: `CMakeLists.txt`; VERIFIED: `tests/CMakeLists.txt`; VERIFIED: shell commands]
- Architecture: HIGH — current reader/detector/parser boundaries are visible in code and Phase 4 decisions lock the TES3 integration direction. [VERIFIED: `src/archive.cpp`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`]
- TES3 format rules: HIGH — data-section-relative offsets and table order are cross-verified between UESP and TES5Edit. [CITED: https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format; VERIFIED: `TES5Edit/Core/wbBSArchive.pas`]
- Pitfalls: HIGH — each pitfall maps to locked decisions, acceptance criteria, existing code, or cited format docs. [VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md`; VERIFIED: `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md`]

**Research date:** 2026-05-08  
**Valid until:** 2026-06-07 for project-local architecture; re-check external documentation if TES3 compatibility policy changes.
