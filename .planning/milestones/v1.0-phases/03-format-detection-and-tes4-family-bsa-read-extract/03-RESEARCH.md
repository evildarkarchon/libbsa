# Phase 03: Format Detection and TES4-Family BSA Read/Extract - Research

**Researched:** 2026-05-07  
**Domain:** C++20 Bethesda TES4-family BSA detection, metadata, lookup, and extraction  
**Confidence:** HIGH for project/API constraints and TES5Edit-traced behavior; MEDIUM for community BSA format notes where they supplement TES5Edit

<user_constraints>

## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Public Reader Surface
- **D-01:** Optimize the public reader slice around methods on `archive_reader`: metadata, entry listing, path lookup/contains, and extraction should be callable from the opened reader object rather than split into separate public view/extractor objects in Phase 3.
- **D-02:** Archive-level public metadata should expose only the required Phase 3 fields: archive type/variant, version, archive flags, file count, and supported compression behavior. Avoid broad variant unions or stringly typed metadata maps for now.
- **D-03:** Entry listing should return stable value metadata records, not paths-only lists or live entry handles. Entry records should include the canonical path and the sizes, offset, hash, compression, embedded-name, and format-specific fields needed by `03-SPEC.md` acceptance.
- **D-04:** Direct lookup should return `result<std::optional<entry_metadata>>` or an equivalent shape: invalid lookup strings remain errors, while valid-but-missing archive paths are normal absence.

#### Path Display And Lookup Policy
- **D-05:** Public entry metadata should expose both `path` and `original_path`. `path` is the canonical normalized lookup key; `original_path` is archive-derived display/source spelling.
- **D-06:** If two archive records normalize to the same canonical `path`, treat the archive as invalid and fail with `error_code::format_error` rather than choosing a winner or exposing ambiguous duplicates.
- **D-07:** `reader.entries()` should return entries in canonical path order for deterministic consumer behavior and tests. Preserve record/hash order internally only where parser compatibility needs it.
- **D-08:** If a structurally valid TES4-family archive lacks usable name strings needed to list paths, Phase 3 should fail open/read as `unsupported` rather than expose hash-only partial behavior.
- **D-09:** `original_path` should preserve parsed folder/file casing and spelling but join split archive name components into a predictable single `/`-separated string. Do not use `std::filesystem::path` semantics for archive-internal paths.
- **D-10:** All public path input should be normalized through the existing virtual path semantics so case and `/` vs `\` variants resolve to the same canonical key.
- **D-11:** Invalid lookup strings such as empty, rooted, or traversal-like archive paths should fail with `error_code::invalid_argument`.

#### Extraction Contract
- **D-12:** Public extraction should be sink-first. Expose a synchronous public sink interface or equivalent `payload_sink`-style target as the primary extraction API.
- **D-13:** Provide a bounded memory convenience helper for tests and small entries, such as `extract_bytes(path)`, while keeping sink extraction the primary API.
- **D-14:** Extraction should identify entries by path string first: `extract(path, sink)` or equivalent. Do not require callers to obtain an entry handle before extracting.
- **D-15:** Partial sink acceptance of a requested chunk is an `error_code::io_error` failure. Do not retry partial writes or report success with incomplete output in Phase 3.
- **D-16:** For compressed entries, Phase 3 may use one-entry bounded buffers around exact-size decompression, then write the decompressed bytes to the sink. The boundary is per entry, never whole archive loading.
- **D-17:** Corrupt compressed payloads or exact-size decompression mismatches should fail with `error_code::format_error`.
- **D-18:** Embedded-name entries should expose metadata indicating an embedded-name layout and prefix size. Extraction must strip/skip the embedded-name prefix so consumer-visible bytes match the file payload.
- **D-19:** Do not add public extract-to-filesystem convenience in Phase 3. Host directory creation, overwrite, traversal, and bulk filesystem policy are deferred.
- **D-20:** Add a public `error_code::not_found` or equivalent missing-entry category for valid path strings that are absent from the archive. Do not overload `invalid_argument` for this case.
- **D-21:** Public extraction is synchronous only in Phase 3. Do not retain sink callbacks or introduce async lifetime, progress, or cancellation contracts.
- **D-22:** Public extraction method names should optimize for simple verbs such as `extract(path, sink)` and `extract_bytes(path)`.

#### Fixture Proof Strategy
- **D-23:** Default Phase 3 proof should use committed tiny generated BSA archives plus machine-readable JSON manifests that describe expected metadata, paths, and extracted bytes/hashes.
- **D-24:** Require at least one success fixture archive per supported TES4-family variant: v103, v104, and v105.
- **D-25:** Across the success fixture set, cover raw entries, v103/v104 deflate entries, v105 LZ4-frame entries, mixed path casing/separator lookup inputs, and embedded-name payload behavior.
- **D-26:** Commit the fixture generation code, not just binary archives. Keep generators and generated outputs under test fixture tooling outside `TES5Edit/`, with provenance documented in fixture README/manifest files.
- **D-27:** Require a focused malformed fixture set for default CI: unsupported version, truncated header/table, duplicate canonical path, corrupt compressed payload, size mismatch, and non-BSA bytes with a `.bsa` host filename.
- **D-28:** BSArchPro-derived or game-derived compatibility comparisons are optional supplements only. They must not block default CI and must not require committed copyrighted archives.
- **D-29:** The JSON manifest parser may be implemented with a documented test-only dependency exception. The dependency must not leak into libbsa public headers or runtime library linkage.

### the agent's Discretion

- Planner may choose exact internal parser class names, source file split, and handler registration mechanics as long as future TES3/BA2 format handlers can be added without rewriting TES4-family code.
- Planner may choose the exact JSON library after research, but it must remain test/tool-only and be documented as a dependency-policy exception.
- Planner may choose exact public type names for metadata and sink interfaces, but must preserve the semantics above and keep public headers C++20-compatible and dependency-light.

### Deferred Ideas (OUT OF SCOPE)

None — discussion stayed within phase scope.

</user_constraints>

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FMT-01 | Detect archive format from magic bytes, archive type fields, and version fields rather than file extension. | Use `BSA\0` magic plus versions `0x67`, `0x68`, and `0x69`; reject non-BSA bytes and unsupported versions. [VERIFIED: `.planning/REQUIREMENTS.md` lines 20-27; `03-SPEC.md` lines 17-20; `TES5Edit/Core/wbBSArchive.pas` lines 500-525, 1079-1106] |
| FMT-02 | Expose archive type, variant, version, flags, file count, and supported compression behavior. | Public metadata should map TES4/FO3/SSE variant, header version, `Flags`, `FileCount`, and compression routing (`deflate` for v103/v104, `lz4_frame` for v105). [VERIFIED: `03-CONTEXT.md` lines 44-48; `TES5Edit/Core/wbBSArchive.pas` lines 430-438, 867-875, 1203-1208] |
| FMT-03 | List archive file paths in a stable library-owned representation. | Parse folder names plus file name block into `original_path`, normalize to canonical `path`, and sort public entries by canonical key. [VERIFIED: `03-CONTEXT.md` lines 51-55; `TES5Edit/Core/wbBSArchive.pas` lines 1223-1237, 2481-2488, 2550-2557] |
| FMT-04 | Check path existence using normalized archive virtual path semantics. | Reuse `detail::normalize_archive_path` for public lookup inputs and build a canonical map. [VERIFIED: `03-CONTEXT.md` lines 50-58; `src/detail/archive_path.cpp` lines 24-57] |
| FMT-05 | Retrieve per-entry metadata including sizes, offset, compression, hash values, and format-specific record data. | TES4-family file records provide file hash, flagged stored size, payload offset, and folder record hash/count/offset; derived metadata must expose raw size, stored size, compression method, embedded-name prefix, and archive flags. [VERIFIED: `.planning/REQUIREMENTS.md` lines 26-27; `TES5Edit/Core/wbBSArchive.pas` lines 265-284, 1212-1231, 812-820] |
| FMT-06 | Add future archive versions without rewriting unrelated format families. | Use a detector/handler boundary behind `archive_reader::open` and keep TES4 parser private under `src/`; public headers remain dependency-light. [VERIFIED: `03-SPEC.md` lines 62-65; `CMakeLists.txt` lines 42-62; `tests/unit/public_include_boundary_tests.cpp` lines 22-48] |
| BSA-01 | Read and extract TES4/Oblivion v103 archives. | v103 uses `BSA\0`, version `0x67`, TES4 folder/file records, and zlib/raw-deflate-compatible compressed blocks with a 4-byte uncompressed-size prefix. [VERIFIED: `.planning/REQUIREMENTS.md` lines 42-48; `TES5Edit/Core/wbBSArchive.pas` lines 522-525, 1203-1237, 2121-2149; CITED: https://en.uesp.net/wiki/Oblivion_Mod:BSA_File_Format] |
| BSA-02 | Read and extract FO3/FNV/Skyrim LE v104 archives. | v104 is the same TES4-family parser branch as `baFO3`, version `0x68`, with deflate routing and embedded-name behavior when archive flag `0x0100` is set. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 522-525, 1090-1096, 1203-1237, 1863-1869, 2131-2138] |
| BSA-03 | Read and extract Skyrim SE/AE v105 archives. | v105 is `baSSE`, version `0x69`, and switches TES4-family compressed payloads to LZ4 frame routing. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 522-525, 1090-1096, 1203-1205, 1790-1812] |
| BSA-05 | Locate TES4-family BSA entries by path using hash-compatible behavior. | TES5Edit locates by folder hash then file hash and assumes hash-sorted tables; libbsa can additionally use canonical path map for public semantics while preserving hash metadata. [VERIFIED: `.planning/REQUIREMENTS.md` lines 45-48; `TES5Edit/Core/wbBSArchive.pas` lines 918-949; `src/detail/bethesda_hash.cpp` lines 79-130] |
| BSA-06 | Extract embedded-name BSA entries while preserving BSArchPro-compatible payload bytes. | For FO3/SSE with `ARCHIVE_EMBEDNAME`, extraction skips the length-prefixed embedded filename before reading/decompressing payload bytes. [VERIFIED: `03-CONTEXT.md` lines 64-66; `TES5Edit/Core/wbBSArchive.pas` lines 1863-1869, 2131-2138] |
| BSA-07 | Extract raw, deflate-compressed, or LZ4-frame-compressed entries according to archive version and flags. | Compute compression as archive default XOR file size bit `0x40000000`; route `none`, `deflate`, or `lz4_frame` into Phase 2 exact-size decompression. [VERIFIED: `.planning/REQUIREMENTS.md` lines 31-37, 42-48; `TES5Edit/Core/wbBSArchive.pas` lines 812-820, 1790-1812, 2121-2149; `src/detail/compression_router.cpp` lines 32-48] |

</phase_requirements>

## Summary

Phase 3 should be planned as a private TES4-family parser plus a small public `archive_reader` expansion, not as a broad archive-framework rewrite. [VERIFIED: `03-CONTEXT.md` lines 44-85; `03-SPEC.md` lines 62-65] The public surface should remain `archive_reader`-centric with value metadata, canonical path listing, normalized lookup, synchronous sink extraction, and a bounded `extract_bytes` helper. [VERIFIED: `03-CONTEXT.md` lines 44-70]

The parser needs to read BSA bytes from the host path, validate `BSA\0` magic, accept only versions `0x67`, `0x68`, and `0x69`, parse the TES4-family header/folder/file/name tables, build a duplicate-rejecting canonical path index, and keep record/hash order as internal compatibility metadata. [VERIFIED: `03-SPEC.md` lines 17-65; `TES5Edit/Core/wbBSArchive.pas` lines 1079-1106, 1203-1237] Extraction needs to compute compression from archive flag `0x0004` XOR file size flag `0x40000000`, skip embedded names for FO3/SSE when flag `0x0100` is set, read the 4-byte uncompressed-size prefix for compressed entries, then route v103/v104 to deflate and v105 to LZ4 frame using Phase 2 exact-size adapters. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 812-820, 1203-1205, 1790-1812, 2121-2149; `src/detail/compression_router.cpp` lines 32-48]

**Primary recommendation:** Plan four implementation seams: public API values/sink/error, private detector+TES4 parser, extraction engine using Phase 2 codecs/stream semantics, and fixture/manifest tests with a small test-only JSON dependency. [VERIFIED: `03-CONTEXT.md` lines 72-85; `vcpkg.io` package page for `nlohmann-json` 3.12.0#2; Context7 `/nlohmann/json` parse examples]

## Project Constraints (from AGENTS.md)

- `TES5Edit/` is a read-only reference submodule and must not be edited, formatted, staged, compiled into libbsa, used as vendored source, or used as a mutable fixture workspace. [VERIFIED: `AGENTS.md` lines 16-29, 59-63]
- Implementation work must be C++ and must prefer clear, portable C++ interfaces over direct Delphi/Pascal transliteration. [VERIFIED: `AGENTS.md` lines 31-37]
- Non-obvious compatibility constraints discovered while tracing BSArchPro/TES5Edit must be recorded near the new implementation. [VERIFIED: `AGENTS.md` lines 35-37, 51-57]
- Do not introduce speculative dependencies; use `libdeflate`, official `lz4`, `DirectXTex`, and `vcpkg` according to project dependency policy. [VERIFIED: `AGENTS.md` lines 39-49]
- Public APIs and newly added or substantially rewritten methods require Doxygen-compliant comments; comments explaining accurate behavior must not be deleted as cleanup. [VERIFIED: `AGENTS.md` lines 51-57]
- Tests should focus on archive parsing, extraction, compatibility behavior, and fixture-based byte/metadata proof; `TES5Edit/` must not be a mutable fixture. [VERIFIED: `AGENTS.md` lines 59-63]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Format detection | Library API / Parser | Host filesystem input | `archive_reader::open` owns byte-driven classification after reading the host path; host filename extension is not format truth. [VERIFIED: `03-SPEC.md` lines 17-20, 88-96] |
| Archive metadata | Library API | Private parser | Public value metadata must expose only Phase 3 fields parsed from TES4-family headers. [VERIFIED: `03-CONTEXT.md` lines 44-48] |
| Entry listing and lookup | Library API | Private parser/hash utilities | Public listing uses canonical virtual paths; private parser preserves folder/file record hashes and record order for compatibility. [VERIFIED: `03-CONTEXT.md` lines 50-58; `TES5Edit/Core/wbBSArchive.pas` lines 918-949] |
| Extraction | Library API | Compression adapters / payload sink | Public `extract(path, sink)` owns caller contract; private code reads payload ranges and routes decompression through Phase 2 adapters. [VERIFIED: `03-CONTEXT.md` lines 59-70; `src/detail/compression_router.cpp` lines 32-48] |
| Fixture generation and manifests | Tests / Tooling | Library parser | Default CI proof must use committed generated fixtures and manifests rather than copyrighted game archives. [VERIFIED: `03-CONTEXT.md` lines 72-80; `tests/fixtures/README.md` lines 7-33] |

## Standard Stack

### Core

| Library / Component | Version | Purpose | Why Standard |
|---------------------|---------|---------|--------------|
| C++20 public API | C++20 mode | Public value types, reader facade, result/error handling | Project and installed-boundary tests require C++20 and reject C++23-only `std::expected`. [VERIFIED: `AGENTS.md` lines 103-113; `tests/unit/public_include_boundary_tests.cpp` lines 12-25] |
| CMake | 3.24 minimum; 4.3.2 available locally | Build, target sources, tests, install/export | Existing project uses CMake target file sets and CTest; local `cmake --version` returned 4.3.2. [VERIFIED: `CMakeLists.txt` lines 1-98; environment probe 2026-05-07] |
| vcpkg manifest mode | Baseline `12dcccadfe573d0eaa6c67a968413ded7805d256` | Dependency acquisition | Existing `vcpkg.json` declares libdeflate, lz4, DirectXTex, and Catch2; CMake presets use `$env{VCPKG_ROOT}`. [VERIFIED: `vcpkg.json` lines 1-17; `CMakePresets.json` lines 8-34] |
| Phase 2 binary/path/hash/compression primitives | Current repo state | Safe reads, normalization, hashes, bounded payload transfer, exact-size decompression | Phase 3 should consume these internals instead of reimplementing them. [VERIFIED: `src/detail/binary_io.hpp` lines 12-50; `src/detail/archive_path.cpp` lines 24-57; `src/detail/bethesda_hash.cpp` lines 79-130; `src/detail/compression_router.cpp` lines 32-48] |
| TES5Edit / BSArchPro source | Read-only submodule | Behavioral reference | Project explicitly uses `TES5Edit/Core/wbBSArchive.pas` and `TES5Edit/Core/wbBSA.pas` as compatibility reference while forbidding edits. [VERIFIED: `AGENTS.md` lines 3-29] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| libdeflate | vcpkg `1.25#0`; project manifest currently declares features `compression` and `decompression` | Deflate payload decompression/compression through Phase 2 adapter | Use for v103/v104 compressed BSA entries after parsing the 4-byte uncompressed-size prefix. [VERIFIED: `vcpkg.json` lines 4-14; `https://vcpkg.io/en/package/libdeflate.html`; `src/detail/deflate_codec.cpp` lines 43-57] |
| lz4 | vcpkg `1.10.0#0` | LZ4 frame decompression through Phase 2 adapter | Use only for v105 Skyrim SE/AE BSA compressed payloads in Phase 3. [VERIFIED: `https://vcpkg.io/en/package/lz4.html`; `TES5Edit/Core/wbBSArchive.pas` lines 1203-1205, 1790-1812; `src/detail/lz4_frame_codec.cpp` lines 33-64] |
| Catch2 | vcpkg `3.14.0#0` | Unit, fixture, malformed, and public-boundary tests | Existing test target uses `Catch2::Catch2WithMain` and `catch_discover_tests(... ADD_TAGS_AS_LABELS)`. [VERIFIED: `https://vcpkg.io/en/package/catch2.html`; Context7 `/catchorg/catch2`; `tests/CMakeLists.txt` lines 1-41] |
| nlohmann-json | vcpkg `3.12.0#2` | Test-only fixture manifest parsing | Add only to test target or fixture tooling as the documented D-29 exception; do not link it into runtime public API. [VERIFIED: `03-CONTEXT.md` lines 78-84; `https://vcpkg.io/en/package/nlohmann-json.html`; Context7 `/nlohmann/json`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `archive_reader` methods | Separate public view/extractor objects | Locked out by D-01 for Phase 3; separate objects may be revisited later if API surface grows. [VERIFIED: `03-CONTEXT.md` lines 44-49] |
| Phase 2 `decompress_payload_exact` | Direct libdeflate/lz4 calls in parser | Direct calls duplicate exact-size/error routing and risk public/private dependency leakage. [VERIFIED: `src/detail/compression_router.cpp` lines 32-48; `tests/unit/public_include_boundary_tests.cpp` lines 22-48] |
| nlohmann-json test dependency | Hand-written manifest parser | Hand parser avoids a test dependency but adds needless parsing edge cases; D-29 explicitly allows a documented test-only dependency. [VERIFIED: `03-CONTEXT.md` lines 78-84; Context7 `/nlohmann/json`] |
| Game-derived fixtures | Generated legal fixtures | Game archives cannot be committed or required by default CI; local compatibility fixtures may supplement only. [VERIFIED: `03-CONTEXT.md` lines 72-80; `tests/fixtures/README.md` lines 7-24] |

**Installation:**
```bash
vcpkg add port nlohmann-json
```

**Version verification:** `libdeflate` `1.25#0`, `lz4` `1.10.0#0`, `catch2` `3.14.0#0`, and `nlohmann-json` `3.12.0#2` were verified from vcpkg package pages during research. [VERIFIED: https://vcpkg.io/en/package/libdeflate.html; https://vcpkg.io/en/package/lz4.html; https://vcpkg.io/en/package/catch2.html; https://vcpkg.io/en/package/nlohmann-json.html]

## Architecture Patterns

### System Architecture Diagram

```text
archive_reader::open(host_path)
  -> validate non-empty host path
  -> read bounded header / archive bytes from host file
  -> detect magic/version
      BSA\0 + 0x67 -> TES4/Oblivion v103 handler
      BSA\0 + 0x68 -> FO3/FNV/Skyrim LE v104 handler
      BSA\0 + 0x69 -> Skyrim SE/AE v105 handler
      other magic/version -> unsupported or format_error
  -> parse TES4-family header + folder records + folder names + file records + file names
  -> build entry_metadata values
      original_path = parsed folder + '/' + file spelling
      path = normalize_archive_path(original_path)
      duplicate canonical path -> format_error
  -> return archive_reader owning parsed metadata and host file access strategy

reader.entries()
  -> return stable value copy sorted by canonical path

reader.find(path) / contains(path)
  -> normalize public path input
      invalid path -> invalid_argument
      valid missing path -> optional none / false
      present path -> value metadata

reader.extract(path, sink)
  -> normalize path
  -> locate entry or not_found
  -> seek payload offset
  -> if embedded-name layout: read length-prefixed embedded name and subtract prefix from stored payload span
  -> if compressed: read uncompressed-size prefix + compressed bytes
      version 103/104 -> compression_method::deflate
      version 105 -> compression_method::lz4_frame
      exact-size mismatch/corrupt stream -> format_error
  -> write exact consumer-visible bytes to sink
      partial sink acceptance -> io_error
```

This diagram is derived from locked Phase 3 scope and TES5Edit parsing/extraction flow. [VERIFIED: `03-SPEC.md` lines 17-65, 99-112; `TES5Edit/Core/wbBSArchive.pas` lines 1079-1237, 2121-2149]

### Recommended Project Structure

```text
include/libbsa/
├── archive.hpp              # archive_reader, archive_metadata, entry_metadata, public sink interface
├── result.hpp               # add not_found error category if chosen
└── libbsa.hpp               # umbrella include remains dependency-light
src/
├── archive.cpp              # public facade delegates to detector/handlers
├── formats/
│   └── bsa/
│       ├── tes4_bsa_reader.hpp/.cpp      # parsed model, lookup, extraction
│       ├── tes4_bsa_parser.hpp/.cpp      # byte parsing and validation
│       └── bsa_format_detector.hpp/.cpp  # magic/version classifier
└── detail/                  # existing Phase 2 binary/path/hash/compression/payload primitives
tests/
├── fixtures/generated/source/             # fixture source payloads and generator input
├── fixtures/generated/archives/           # tiny generated legal .bsa outputs + manifests
├── fixtures/generated/generate_bsa_fixtures.* # generator outside TES5Edit
└── unit/tes4_bsa_reader_tests.cpp          # detection, metadata, lookup, extraction, malformed cases
```

The exact file names are discretionary, but the split should keep public headers minimal and keep parser/codec implementation private. [VERIFIED: `03-CONTEXT.md` lines 81-85; `CMakeLists.txt` lines 42-62; `tests/unit/public_include_boundary_tests.cpp` lines 22-48]

### Pattern 1: Private handler behind public facade

**What:** `archive_reader::open` should perform host path validation, then delegate byte classification to a private detector and construct an `archive_reader` value with a type-erased or variant-backed private implementation. [VERIFIED: `include/libbsa/archive.hpp` lines 15-23; `03-CONTEXT.md` lines 44-49]

**When to use:** Use this for FMT-06 so TES3 and BA2 handlers can later register or branch without changing Phase 3 public methods. [VERIFIED: `03-SPEC.md` lines 62-65]

**Example:**
```cpp
/// Opens a supported archive from a host filesystem path.
static result<archive_reader> open(std::string_view host_path);

// Private implementation sketch; names are discretionary.
result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }
  auto bytes = detail::read_archive_prefix_or_file(host_path);
  if (!bytes) return bytes.error();
  auto kind = formats::detect_archive_format(bytes.value());
  if (!kind) return kind.error();
  return formats::open_tes4_family_bsa(host_path, kind.value());
}
```
Source: public facade and locked API behavior. [VERIFIED: `include/libbsa/archive.hpp` lines 17-23; `03-SPEC.md` lines 17-20]

### Pattern 2: Parse record order, expose canonical order

**What:** Parse folders/files in archive record order for offsets/hashes, but public `entries()` returns a sorted value vector by canonical `path`. [VERIFIED: `03-CONTEXT.md` lines 51-55; `TES5Edit/Core/wbBSArchive.pas` lines 1212-1237]

**When to use:** Use this for deterministic tests and duplicate canonical path validation. [VERIFIED: `03-CONTEXT.md` lines 51-58]

**Example:**
```cpp
auto canonical = detail::normalize_archive_path(original_path);
if (!canonical) return canonical.error();
if (!entries_by_path.emplace(canonical.value().value, entry_index).second) {
  return error{error_code::format_error, "duplicate canonical archive path"};
}
std::sort(public_entries.begin(), public_entries.end(), [](const auto& a, const auto& b) {
  return a.path < b.path;
});
```
Source: existing normalization helper and D-06/D-07. [VERIFIED: `src/detail/archive_path.cpp` lines 24-57; `03-CONTEXT.md` lines 51-58]

### Pattern 3: Derive compression from archive default XOR per-file flag

**What:** `compressed = (archive_flags & 0x0004) XOR (file_size & 0x40000000)`, while raw stored size is `file_size & ~0x40000000`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 812-820]

**When to use:** Use for all v103/v104/v105 TES4-family entries before deciding whether to read the uncompressed-size prefix and route to a codec. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 2121-2149]

**Example:**
```cpp
constexpr std::uint32_t archive_compress = 0x0004;
constexpr std::uint32_t file_size_compress = 0x40000000;
const bool compressed = ((flags & archive_compress) != 0U) ^ ((record_size & file_size_compress) != 0U);
const std::uint32_t stored_size = record_size & ~file_size_compress;
```
Source: TES5Edit `TwbBSFileTES4.Compressed` and `RawSize`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 812-820]

### Pattern 4: Embedded-name prefix is layout metadata, not extracted payload

**What:** For FO3/SSE with `ARCHIVE_EMBEDNAME` (`0x0100`), payload data begins with a length-prefixed embedded name that extraction skips before reading raw or compressed content. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 1863-1869, 2131-2138]

**When to use:** Apply only to `baFO3`/`baSSE` behavior in TES5Edit, not v103 Oblivion, unless a fixture proves otherwise. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 1863-1869, 2131-2138]

**Example:**
```cpp
std::size_t payload_cursor = entry.offset;
std::uint32_t stored_remaining = entry.stored_size;
if (entry.has_embedded_name) {
  auto prefix = read_bstring_at(payload_cursor);
  if (!prefix) return prefix.error();
  payload_cursor += prefix.value().encoded_size;
  stored_remaining -= checked_u32(prefix.value().encoded_size);
}
```
Source: TES5Edit skips `ReadStringLen(False)` plus one byte before extraction. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 2131-2133]

### Anti-Patterns to Avoid

- **Extension-based detection:** Host `.bsa` names are not format truth and tests must include misleading filenames. [VERIFIED: `03-SPEC.md` lines 17-20, 88-96]
- **Public `std::filesystem::path` for archive-internal paths:** Existing project policy requires virtual path semantics independent of host path rules. [VERIFIED: `03-CONTEXT.md` lines 50-58; `src/detail/archive_path.cpp` lines 24-57]
- **Leaking private dependencies in installed headers:** Public include boundary tests already reject `libdeflate`, `lz4`, `DirectXTex`, `TES5Edit`, and `std::expected` tokens. [VERIFIED: `tests/unit/public_include_boundary_tests.cpp` lines 22-48]
- **Choosing a duplicate normalized path winner:** D-06 requires failing the archive as `format_error`. [VERIFIED: `03-CONTEXT.md` lines 51-58]
- **Using LZ4 block API for v105 BSA:** TES5Edit uses `ctLZ4Frame` for `baSSE`; raw LZ4 block is for Starfield BA2 and remains out of Phase 3. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 1203-1205, 1790-1812; `03-SPEC.md` lines 78-86]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Deflate decompression | Custom inflate parser or zlib wrapper guesswork | Existing `detail::decompress_payload_exact(compression_method::deflate, ...)` | Project uses libdeflate and Phase 2 exact-size validation already maps bad streams to `format_error`. [VERIFIED: `AGENTS.md` lines 39-49; `src/detail/compression_router.cpp` lines 32-48; `src/detail/deflate_codec.cpp` lines 43-57] |
| LZ4 frame decompression | Raw LZ4 block calls or custom frame parser | Existing `compression_method::lz4_frame` adapter | LZ4 frame and block APIs are different; Context7 documents `LZ4F_*` frame APIs and `LZ4_decompress_safe` block API separately. [CITED: Context7 `/lz4/lz4`; VERIFIED: `src/detail/lz4_frame_codec.cpp` lines 33-64] |
| Archive virtual path validation | Ad hoc lowercasing or `std::filesystem::path` normalization | Existing `detail::normalize_archive_path` | Existing helper enforces lowercase `/` keys and rejects empty, rooted, repeated separator, `.` and `..` segments. [VERIFIED: `src/detail/archive_path.cpp` lines 24-57; `tests/unit/archive_path_tests.cpp` lines 8-30] |
| TES4 hash compatibility | New hash implementation inside parser | Existing `detail::hash_tes4` | Hash helper is already traced to TES5Edit `CreateHashTES4` behavior. [VERIFIED: `src/detail/bethesda_hash.cpp` lines 79-130; `TES5Edit/Core/wbBSArchive.pas` lines 733-784] |
| Manifest parsing | Fragile handwritten JSON parser | Test-only `nlohmann-json` | D-29 allows a documented test-only dependency, and Context7 shows straightforward file parsing. [VERIFIED: `03-CONTEXT.md` lines 78-84; CITED: Context7 `/nlohmann/json`] |

**Key insight:** Phase 3 complexity is in offset/layout/compatibility validation, not in codecs, paths, or hashes; those already exist and should be reused. [VERIFIED: `03-SPEC.md` lines 11-13; `src/detail/*` files listed in `03-CONTEXT.md` lines 114-123]

## Common Pitfalls

### Pitfall 1: Misinterpreting the file size compression bit as part of stored payload size
**What goes wrong:** Parser seeks too far or extracts too many bytes when bit `0x40000000` is not masked out. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 812-820]  
**Why it happens:** TES4-family `Size` stores both payload span and an inversion flag. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 265-274, 812-820; CITED: https://en.uesp.net/wiki/Oblivion_Mod:BSA_File_Format]  
**How to avoid:** Store both raw record size and `stored_size = size & ~0x40000000`, then compute compression separately. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 812-820]  
**Warning signs:** Manifest stored sizes are off by `0x40000000` or extraction tries to allocate huge buffers. [ASSUMED]

### Pitfall 2: Treating compressed stored bytes as only compressed data
**What goes wrong:** Deflate/LZ4 decoding starts at the 4-byte uncompressed-size prefix and fails. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 2135-2144]  
**Why it happens:** TES4-family compressed blocks include a leading `uint32` uncompressed size before compressed bytes. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 1867-1869, 2135-2144; CITED: https://en.uesp.net/wiki/Oblivion_Mod:BSA_File_Format]  
**How to avoid:** After embedded-name skipping, read expected raw size from the payload stream, decrement stored span by four, then pass remaining bytes to `decompress_payload_exact`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 2135-2144; `src/detail/compression_router.cpp` lines 32-48]  
**Warning signs:** Every compressed fixture fails with `format_error` while raw fixtures pass. [ASSUMED]

### Pitfall 3: Applying embedded-name behavior to the wrong variants
**What goes wrong:** Oblivion payloads lose their first bytes or FO3/SSE payloads expose archive-internal name prefixes. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 1863-1869, 2131-2138]  
**Why it happens:** TES5Edit applies embedded-name write/skip logic only for `baFO3` and `baSSE` with `ARCHIVE_EMBEDNAME`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 1863-1869, 2131-2138]  
**How to avoid:** Encode `has_embedded_name` and `embedded_name_prefix_size` in metadata and cover embedded fixtures. [VERIFIED: `03-CONTEXT.md` lines 64-66, 72-80]  
**Warning signs:** Extracted bytes begin with a path-like string or raw Oblivion payload starts one byte late. [ASSUMED]

### Pitfall 4: LZ4 frame/block confusion
**What goes wrong:** Skyrim SE/AE compressed fixtures fail or produce corrupt output if routed to raw LZ4 block decode. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 1203-1205, 1790-1812]  
**Why it happens:** LZ4 frame payloads are self-describing frame streams while raw LZ4 blocks require external size and use a different API. [CITED: Context7 `/lz4/lz4`]  
**How to avoid:** v105 BSA uses `compression_method::lz4_frame`; reserve `lz4_block` for later BA2/Starfield phases. [VERIFIED: `03-SPEC.md` lines 52-55, 78-86; `src/detail/compression_router.hpp` lines 11-19]  
**Warning signs:** LZ4 decoder reports malformed frame or exact-size mismatch only on v105 compressed entries. [ASSUMED]

### Pitfall 5: Hash lookup without canonical duplicate validation
**What goes wrong:** Two records normalize to the same public path and consumers get nondeterministic lookup/extraction. [VERIFIED: `03-CONTEXT.md` lines 51-58]  
**Why it happens:** Archive record identity and public normalized identity are different layers. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 918-949; `src/detail/archive_path.cpp` lines 24-57]  
**How to avoid:** Build canonical map during open and fail duplicate insertions with `format_error`. [VERIFIED: `03-CONTEXT.md` lines 51-58]  
**Warning signs:** `entries()` contains two records whose `path` values differ only by case or separator. [ASSUMED]

## Code Examples

### TES4-family header constants
```cpp
constexpr std::array<std::byte, 4> bsa_magic{
    std::byte{'B'}, std::byte{'S'}, std::byte{'A'}, std::byte{0}};
constexpr std::uint32_t version_tes4 = 0x67;
constexpr std::uint32_t version_fo3 = 0x68;
constexpr std::uint32_t version_sse = 0x69;
```
Source: TES5Edit constants `MAGIC_BSA`, `HEADER_VERSION_TES4`, `HEADER_VERSION_FO3`, and `HEADER_VERSION_SSE`. [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 500-525]

### Compression routing
```cpp
const auto method = entry.compression == entry_compression::deflate
    ? detail::compression_method::deflate
    : detail::compression_method::lz4_frame;
auto decoded = detail::decompress_payload_exact(method, compressed_payload, expected_raw_size);
if (!decoded) {
  return decoded.error();
}
```
Source: Phase 2 compression router and TES5Edit v105 LZ4 frame switch. [VERIFIED: `src/detail/compression_router.cpp` lines 32-48; `TES5Edit/Core/wbBSArchive.pas` lines 1203-1205]

### Test-only JSON manifest loading
```cpp
#include <fstream>
#include <nlohmann/json.hpp>

std::ifstream manifest{manifest_path};
auto expected = nlohmann::json::parse(manifest);
```
Source: Context7 nlohmann/json file parse examples. [CITED: Context7 `/nlohmann/json`]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `archive_reader::open` returns `unsupported` for non-empty paths | Phase 3 replaces the stub with byte-driven TES4-family opening | Phase 3 scope | Planner must update existing public API tests rather than add parallel APIs. [VERIFIED: `src/archive.cpp` lines 5-12; `03-SPEC.md` lines 7-13] |
| Parser-local codecs | Phase 2 exact-size codec router | Phase 2 complete per context/state | Planner should call existing adapters and add archive-level routing only. [VERIFIED: `03-CONTEXT.md` lines 131-140; `src/detail/compression_router.cpp` lines 32-48] |
| Hash-only or record-order public listing | Canonical path sorted public metadata | Phase 3 locked decision | Planner must separate internal parse order from public order. [VERIFIED: `03-CONTEXT.md` lines 51-58] |
| Required game/BSArchPro fixtures | Committed generated legal fixtures; optional local compatibility tests | Phase 3 locked decision | Default CI must not depend on copyrighted archives or mutable TES5Edit state. [VERIFIED: `03-CONTEXT.md` lines 72-80; `tests/fixtures/README.md` lines 7-24] |

**Deprecated/outdated:**
- Public `std::expected` remains out of scope because public headers must remain C++20-compatible. [VERIFIED: `AGENTS.md` lines 103-113; `tests/unit/public_include_boundary_tests.cpp` lines 22-48]
- Whole-archive extraction loading is not acceptable as the primary extraction model; compressed entries may use one-entry bounded buffers only. [VERIFIED: `03-CONTEXT.md` lines 59-70; `03-SPEC.md` lines 88-96]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Warning signs for compression-bit mistakes and allocation anomalies are inferred from expected failure modes. | Common Pitfalls | Low; tests will reveal exact symptoms. |
| A2 | Warning signs for compressed-prefix mistakes are inferred from expected decode failures. | Common Pitfalls | Low; fixture tests should isolate raw vs compressed behavior. |
| A3 | Warning signs for embedded-name mistakes are inferred from payload layout. | Common Pitfalls | Medium; fixtures must document exact expected bytes. |
| A4 | Warning signs for LZ4 frame/block confusion are inferred from codec behavior. | Common Pitfalls | Low; existing exact-size adapter should fail closed. |
| A5 | Warning signs for duplicate canonical paths are inferred from D-06. | Common Pitfalls | Low; parser can explicitly test duplicates. |

## Open Questions (RESOLVED)

1. **Should Phase 3 add a `not_found` enumerator to `error_code`, or encode missing extract path another way?**
   - What we know: D-20 says add `error_code::not_found` or equivalent for valid missing extraction paths. [VERIFIED: `03-CONTEXT.md` lines 67-70]
   - Resolution: Add `error_code::not_found`. Tests should assert this stable category for valid-but-absent archive entries, while invalid path syntax remains `error_code::invalid_argument`. [VERIFIED: `03-CONTEXT.md` lines 67-70]
2. **What exact fixture generator language should be used?**
   - What we know: Generator code must be committed outside `TES5Edit/`; Python 3.14.4 and Node v25.9.0 are available locally. [VERIFIED: `03-CONTEXT.md` lines 72-80; environment probe 2026-05-07]
   - Resolution: Use a small C++ fixture generator/test tool under test fixture tooling. This keeps fixture bytes reproducible without adding a Python runtime requirement to CI and allows reuse of libbsa compression/hash helpers where appropriate. [VERIFIED: `03-CONTEXT.md` lines 72-80; `CMakeLists.txt` lines 8-40]
3. **How strict should parser validation be for inconsistent folder/file name length fields beyond needed bounds checks?**
   - What we know: Acceptance requires truncated and internally inconsistent archives to fail without out-of-bounds reads. [VERIFIED: `03-SPEC.md` lines 99-112]
   - Resolution: Validate all offsets/count-derived spans against file size, verify parsed names count equals `FileCount`, fail malformed/truncated/internally inconsistent bytes as `error_code::format_error`, and fail structurally valid hash-only or missing-usable-name archives as `error_code::unsupported` per D-08. [VERIFIED: `03-CONTEXT.md` lines 50-58; `03-SPEC.md` lines 99-112]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Configure/build/test | ✓ | 4.3.2 | Project minimum is 3.24. [VERIFIED: environment probe 2026-05-07; `CMakePresets.json` lines 1-7] |
| vcpkg executable on PATH | Dependency install commands | ✗ | — | `VCPKG_ROOT=C:\vcpkg` exists and contains `scripts/buildsystems/vcpkg.cmake`; use explicit path or ensure PATH if invoking `vcpkg`. [VERIFIED: environment probe 2026-05-07; `CMakePresets.json` lines 17-30] |
| vcpkg toolchain file | CMake presets | ✓ | `C:\vcpkg\scripts\buildsystems\vcpkg.cmake` | — [VERIFIED: environment probe 2026-05-07] |
| Git | Submodule boundary checks | ✓ | 2.54.0.windows.1 | — [VERIFIED: environment probe 2026-05-07] |
| Ninja | Optional CMake generator | ✗ | — | Use Visual Studio/MSBuild generator or install Ninja if presets are extended. [VERIFIED: environment probe 2026-05-07] |
| Python | Fixture generator option | ✓ | 3.14.4 | Use C++ generator/test helper if Python is not desired. [VERIFIED: environment probe 2026-05-07] |
| Node | GSD tooling / possible scripts | ✓ | v25.9.0 | — [VERIFIED: environment probe 2026-05-07] |

**Missing dependencies with no fallback:**
- None for planning; vcpkg executable missing from PATH is not blocking while `VCPKG_ROOT` toolchain file exists, but direct `vcpkg add port` commands require using `C:\vcpkg\vcpkg.exe` or updating PATH. [VERIFIED: environment probe 2026-05-07]

**Missing dependencies with fallback:**
- Ninja is missing; use the default Visual Studio/MSBuild generator or add Ninja later only if needed. [VERIFIED: environment probe 2026-05-07]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 `3.14.0#0` via vcpkg and CTest discovery. [VERIFIED: `https://vcpkg.io/en/package/catch2.html`; `tests/CMakeLists.txt` lines 1-41] |
| Config file | `tests/CMakeLists.txt`; no separate Catch2 config file. [VERIFIED: `tests/CMakeLists.txt` lines 1-52] |
| Quick run command | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static -L "unit|fixture|malformed" --output-on-failure` [VERIFIED: `CMakePresets.json` lines 36-65; `tests/CMakeLists.txt` lines 36-41] |
| Full suite command | `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` [VERIFIED: `CMakePresets.json` lines 36-65] |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| FMT-01 | Byte-driven detection for v103/v104/v105 and non-BSA `.bsa` rejection | fixture + malformed | `ctest --preset windows-msvc-debug-static -R tes4_bsa_detection --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 17-20] |
| FMT-02 | Archive metadata fields match manifest | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_metadata --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 22-25] |
| FMT-03 | Stable canonical path listing | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_listing --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 27-30] |
| FMT-04 | Normalized lookup/contains and invalid path errors | unit + fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_lookup --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 32-35; `tests/unit/archive_path_tests.cpp` lines 8-30] |
| FMT-05 | Entry metadata sizes, offsets, hashes, flags, compression | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_entry_metadata --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 37-40] |
| FMT-06 | Public boundary remains clean and unsupported future version is classified | unit + malformed | `ctest --preset windows-msvc-debug-static -R "public_include_boundary|unsupported_future_bsa" --output-on-failure` | ✅ boundary exists; ❌ future-version test [VERIFIED: `tests/unit/public_include_boundary_tests.cpp` lines 22-48; `03-SPEC.md` lines 62-65] |
| BSA-01 | v103 raw and deflate extraction | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v103_extract --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 42-45] |
| BSA-02 | v104 raw and deflate extraction | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v104_extract --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 47-50] |
| BSA-03 | v105 raw and LZ4-frame extraction | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_v105_extract --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 52-55] |
| BSA-05 | Hash-compatible lookup metadata | unit + fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_hash_lookup --output-on-failure` | ❌ Wave 0 [VERIFIED: `TES5Edit/Core/wbBSArchive.pas` lines 918-949] |
| BSA-06 | Embedded-name prefix skipped and metadata exposed | fixture | `ctest --preset windows-msvc-debug-static -R tes4_bsa_embedded_name --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 57-60] |
| BSA-07 | Raw/deflate/LZ4 routing and corrupt/size mismatch failures | fixture + malformed | `ctest --preset windows-msvc-debug-static -R tes4_bsa_compression_routing --output-on-failure` | ❌ Wave 0 [VERIFIED: `03-SPEC.md` lines 42-60, 99-112] |

### Sampling Rate
- **Per task commit:** Run the relevant `ctest -R` command for changed behavior plus `ctest --preset windows-msvc-debug-static -R public_include_boundary --output-on-failure`. [VERIFIED: `tests/unit/public_include_boundary_tests.cpp` lines 22-48]
- **Per wave merge:** Run `cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure`. [VERIFIED: `CMakePresets.json` lines 36-65]
- **Phase gate:** Full suite green, generated fixture provenance documented, malformed fixture set passing, public include boundary passing, and `git -C TES5Edit status --short` empty. [VERIFIED: `03-SPEC.md` lines 99-112; `tests/fixtures/README.md` lines 26-41]

### Wave 0 Gaps
- [ ] `tests/unit/tes4_bsa_reader_tests.cpp` — covers FMT-01 through FMT-05 and BSA-01 through BSA-07. [VERIFIED: current `tests/unit` file list lacks this file]
- [ ] `tests/fixtures/generated/generate_tes4_bsa_fixtures.*` — creates legal v103/v104/v105 success and malformed archives. [VERIFIED: `03-CONTEXT.md` lines 72-80; `tests/fixtures/README.md` lines 7-33]
- [ ] `tests/fixtures/generated/archives/*.json` — manifests expected metadata, paths, extracted bytes/hashes, and provenance. [VERIFIED: `03-CONTEXT.md` lines 72-80]
- [ ] Test-only `nlohmann-json` manifest dependency and CMake wiring for test target only. [VERIFIED: `03-CONTEXT.md` lines 78-84; `https://vcpkg.io/en/package/nlohmann-json.html`]
- [ ] Replace or update `tests/unit/archive_reader_tests.cpp` unsupported-stub assertions. [VERIFIED: `tests/unit/archive_reader_tests.cpp` lines 5-17; `03-CONTEXT.md` lines 149-153]

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | No authentication surface in local archive library phase. [VERIFIED: `03-SPEC.md` lines 67-86] |
| V3 Session Management | no | No session state. [VERIFIED: `03-SPEC.md` lines 67-86] |
| V4 Access Control | no | No authorization boundary; host file access is caller-provided path input only. [VERIFIED: `03-SPEC.md` lines 67-86] |
| V5 Input Validation | yes | Checked binary reads, count/offset validation, normalized virtual path validation, structured errors. [VERIFIED: `src/detail/binary_io.hpp` lines 12-50; `src/detail/archive_path.cpp` lines 24-57; `03-SPEC.md` lines 99-112] |
| V6 Cryptography | no | Compression and hashing here are format compatibility functions, not security cryptography. [VERIFIED: `src/detail/bethesda_hash.cpp` lines 79-148; `src/detail/compression_router.cpp` lines 32-48] |

### Known Threat Patterns for C++ archive parsing

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed/truncated table causes out-of-bounds read | Tampering | Use checked `binary_reader`, validate every count/offset/span against file size, and add malformed fixtures. [VERIFIED: `src/detail/binary_io.hpp` lines 12-50; `03-SPEC.md` lines 99-112] |
| Oversized count or size causes memory exhaustion | Denial of Service | Checked arithmetic, bounded per-entry buffers, reject sizes inconsistent with file length before allocation. [VERIFIED: `03-SPEC.md` lines 88-96, 99-112] |
| Path traversal in public lookup/extraction | Tampering | Normalize and reject rooted/traversal-like archive virtual paths before lookup. [VERIFIED: `src/detail/archive_path.cpp` lines 24-57] |
| Corrupt compressed payload causes partial output | Tampering | Use exact-size decompression adapters and fail `format_error` on mismatches. [VERIFIED: `03-CONTEXT.md` lines 64-66; `src/detail/compression_router.cpp` lines 32-48] |
| Partial sink writes leave ambiguous extraction result | Tampering / DoS | Treat partial sink acceptance as `io_error` and do not retry silently. [VERIFIED: `03-CONTEXT.md` lines 62-64; `src/detail/payload_stream.hpp` lines 22-35] |

## Sources

### Primary (HIGH confidence)
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md` — locked implementation decisions, fixture strategy, and code context. [VERIFIED]
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-SPEC.md` — locked Phase 3 requirements and acceptance criteria. [VERIFIED]
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md` — requirement IDs, phase scope, and prior decisions. [VERIFIED]
- `AGENTS.md` — TES5Edit boundary, dependency policy, comments/docs expectations, validation expectations. [VERIFIED]
- `TES5Edit/Core/wbBSArchive.pas` — magic/version constants, TES4-family parsing, compression flag logic, embedded-name extraction, compression type routing. [VERIFIED]
- Existing code: `include/libbsa/archive.hpp`, `include/libbsa/result.hpp`, `src/detail/archive_path.*`, `src/detail/bethesda_hash.*`, `src/detail/compression_router.*`, `src/detail/payload_stream.*`, tests and CMake files. [VERIFIED]
- Context7 `/lz4/lz4` — LZ4 frame and block API distinction. [CITED]
- Context7 `/catchorg/catch2` — `catch_discover_tests` and `ADD_TAGS_AS_LABELS`. [CITED]
- Context7 `/nlohmann/json` — file parsing examples. [CITED]
- vcpkg package pages for `libdeflate`, `lz4`, `catch2`, and `nlohmann-json` — current package versions and features. [CITED]

### Secondary (MEDIUM confidence)
- UESP Oblivion BSA File Format page — community format structure, hash sorting, flags, compressed block layout; used only to supplement TES5Edit-traced behavior. [CITED: https://en.uesp.net/wiki/Oblivion_Mod:BSA_File_Format]
- libdeflate GitHub README — whole-buffer DEFLATE API design and non-streaming model. [CITED: https://github.com/ebiggers/libdeflate/blob/master/README.md]

### Tertiary (LOW confidence)
- GECK wiki search result for BSA flags was found but not fetched due 403; not used as authoritative research. [VERIFIED: web fetch 403]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — project stack and package versions were verified from repo files and vcpkg package pages. [VERIFIED]
- Architecture: HIGH — public/API decisions are locked in `03-CONTEXT.md`, and parser/extraction behavior is traced to `TES5Edit/Core/wbBSArchive.pas`. [VERIFIED]
- Pitfalls: MEDIUM-HIGH — core pitfalls are traced to TES5Edit and existing Phase 2 code; warning-sign symptoms are marked assumed. [VERIFIED; ASSUMED where noted]

**Research date:** 2026-05-07  
**Valid until:** 2026-06-06 for package versions; TES5Edit-traced format behavior remains valid unless the submodule/reference changes. [ASSUMED]
