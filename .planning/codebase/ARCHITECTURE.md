<!-- refreshed: 2026-05-11 -->
# Architecture

**Analysis Date:** 2026-05-11

## System Overview

```text
┌─────────────────────────────────────────────────────────────┐
│                    Public C++20 API Surface                  │
│  `include/libbsa/archive.hpp` `include/libbsa/writer.hpp`    │
│  `include/libbsa/validation.hpp` `include/libbsa/result.hpp` │
└───────────────┬───────────────────────┬─────────────────────┘
                │                       │
                ▼                       ▼
┌──────────────────────────────┐ ┌────────────────────────────┐
│ Reader / Validation Facades  │ │ Public Writer Facades       │
│ `src/archive.cpp`            │ │ `src/formats/*/*writer.cpp` │
│ `src/validation.cpp`         │ │                            │
└───────────────┬──────────────┘ └──────────────┬─────────────┘
                │                               │
                ▼                               ▼
┌─────────────────────────────────────────────────────────────┐
│                Format-Specific Implementations               │
│ `src/formats/bsa/` TES3 + TES4-family BSA                    │
│ `src/formats/ba2/` BA2 GNRL + BA2 DX10                       │
└───────────────┬───────────────────────┬─────────────────────┘
                │                       │
                ▼                       ▼
┌──────────────────────────────┐ ┌────────────────────────────┐
│ Shared Internal Utilities    │ │ Texture / DDS Boundary      │
│ `src/detail/`                │ │ `src/texture/`              │
│ binary I/O, paths, hashing,  │ │ DirectXTex adapter + BA2    │
│ compression, atomic publish  │ │ DX10 DDS layout planning    │
└───────────────┬──────────────┘ └──────────────┬─────────────┘
                │                               │
                ▼                               ▼
┌─────────────────────────────────────────────────────────────┐
│ Build, Tests, Fixtures, Package Smoke                        │
│ `CMakeLists.txt` `tests/` `benchmarks/` `cmake/`              │
└─────────────────────────────────────────────────────────────┘
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| Public reader API | Defines archive metadata, entry metadata, sinks, bulk extraction contracts, and `archive_reader` methods. | `include/libbsa/archive.hpp` |
| Public writer API | Defines TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 writer classes plus target/options types. | `include/libbsa/writer.hpp` |
| Public validation API | Defines validation reports, diagnostics, compatibility warnings, and `validate_archive`. | `include/libbsa/validation.hpp` |
| Public error model | Provides C++20-compatible `result<T>`, `result<void>`, `error`, and `error_code`. | `include/libbsa/result.hpp` |
| Reader dispatcher | Opens host archive paths, detects BSA vs BA2 bytes, dispatches to parsers, owns immutable reader state, and routes listing/lookup/extraction. | `src/archive.cpp` |
| Validation facade | Opens archives through public APIs, derives warnings from public metadata, and optionally proves extractability through a discard sink. | `src/validation.cpp` |
| BSA detector | Classifies TES3 magic and TES4-family BSA versions before parser dispatch. | `src/formats/bsa/bsa_format_detector.cpp` |
| TES3 BSA parser/reader/writer | Parses, extracts, and writes Morrowind/TES3 BSA archives with raw payloads and TES3 hash ordering. | `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes3_bsa_writer.cpp` |
| TES4-family BSA parser/reader/writer | Parses, extracts, and writes Oblivion/Fallout 3/Skyrim SE BSA variants with deflate or LZ4 frame compression according to archive version. | `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp` |
| BA2 detector | Classifies `BTDX` BA2 files by version, subtype, file count, Starfield header fields, and default compression route. | `src/formats/ba2/ba2_format_detector.cpp` |
| BA2 GNRL parser/reader/writer | Parses, extracts, and writes Fallout 4 and Starfield general BA2 archives with deflate or raw LZ4 block routing. | `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp` |
| BA2 DX10 parser/reader/writer | Parses, reconstructs, extracts, and writes BA2 texture archives using libbsa-owned texture metadata and DDS header reconstruction. | `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp` |
| Internal utilities | Supplies bounded binary readers/writers, archive-path normalization, Bethesda hashes, compression routing, allocation guards, worker scheduling, payload streaming, and atomic publish helpers. | `src/detail/` |
| Texture boundary | Hides DirectXTex and converts DDS state into libbsa-owned metadata, chunk plans, and reconstructed DDS headers. | `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.cpp` |
| Test harness | Builds Catch2 tests, generated fixtures, package-consumer smoke tests, and manifest validation. | `tests/CMakeLists.txt` |
| Build/package surface | Defines the `libbsa` target, public file set, private source list, vcpkg dependencies, install/export package, docs target, benchmarks, and tests. | `CMakeLists.txt`, `cmake/libbsaConfig.cmake.in` |

## Pattern Overview

**Overall:** Public API facade with format-specific parser/reader/writer modules and private dependency adapters.

**Key Characteristics:**
- Public headers under `include/libbsa/` expose stable C++20 value types and classes; private codec, DirectXTex, and format-record details remain in `src/`.
- Reader and validation flows go through public facades in `src/archive.cpp` and `src/validation.cpp`; format modules under `src/formats/` own byte-level archive rules.
- Writers use small public objects with `std::shared_ptr<state>` implementation state, collect caller entries, then finalize through format-owned `write_*_archive` functions.
- Archive-internal paths are normalized through `src/detail/archive_path.cpp`; host filesystem paths appear only at host I/O boundaries.
- Compression is selected from parsed or target metadata and routed through `src/detail/compression_router.cpp`; callers never select raw codec APIs.
- DDS and DirectXTex concerns stay behind `src/texture/`, and public texture metadata uses plain libbsa-owned types from `include/libbsa/archive.hpp`.

## Layers

**Public API Layer:**
- Purpose: Define the reusable C++20 library contract for readers, writers, validation, results, and version constants.
- Location: `include/libbsa/`
- Contains: Public enums, POD metadata, virtual sink interfaces, writer option types, public classes, and Doxygen comments.
- Depends on: C++ standard library and other public `include/libbsa/` headers only.
- Used by: Consumers, tests in `tests/unit/`, package smoke tests in `tests/package-consumer/`, and implementation files in `src/`.

**Facade and Dispatch Layer:**
- Purpose: Convert public calls into format-specific operations without exposing private format types.
- Location: `src/archive.cpp`, `src/validation.cpp`
- Contains: `archive_reader::open`, reader listing/lookup/extraction routing, bulk extraction scheduling, validation report construction.
- Depends on: Public headers, `src/formats/` parsers/readers, and select `src/detail/` helpers.
- Used by: Public API callers and validation code.

**BSA Format Layer:**
- Purpose: Own TES3 and TES4-family BSA byte semantics, metadata materialization, extraction, and write-new serialization.
- Location: `src/formats/bsa/`
- Contains: `bsa_format_detector`, `tes3_bsa_*`, `tes4_bsa_*` parser/reader/writer pairs.
- Depends on: Public libbsa metadata/results, `src/detail/binary_io.*`, `src/detail/archive_path.*`, `src/detail/bethesda_hash.*`, `src/detail/compression_router.*`, `src/detail/atomic_file_ops.hpp`.
- Used by: `src/archive.cpp`, public BSA writer methods implemented in `src/formats/bsa/*_writer.cpp`, and focused tests in `tests/unit/`.

**BA2 Format Layer:**
- Purpose: Own BA2 GNRL and DX10 byte semantics, Starfield header handling, chunk metadata, extraction, and write-new serialization.
- Location: `src/formats/ba2/`
- Contains: `ba2_format_detector`, `ba2_gnrl_*`, `ba2_dx10_*`, and BA2 publish helpers.
- Depends on: Public metadata/results, `src/detail/` helpers, and for DX10 paths `src/texture/` adapters.
- Used by: `src/archive.cpp`, public BA2 writer methods implemented in `src/formats/ba2/*_writer.cpp`, and BA2 tests in `tests/unit/`.

**Shared Detail Layer:**
- Purpose: Provide reusable internal primitives with no public API exposure.
- Location: `src/detail/`
- Contains: `binary_reader`, `binary_writer`, archive path normalization, Bethesda hash functions, compression wrappers/router, payload transfer, byte-vector allocation guards, worker scheduling, atomic publish.
- Depends on: Public result types and third-party libraries only where needed (`libdeflate`, `lz4`, Windows APIs in `atomic_file_ops.hpp`).
- Used by: `src/archive.cpp`, all format modules, texture layout code, and white-box unit tests.

**Texture Layer:**
- Purpose: Isolate DirectXTex and DDS layout rules from public headers and format writers/readers.
- Location: `src/texture/`
- Contains: DDS source analysis, libbsa-owned texture metadata translation, DXT10 header reconstruction, mip/chunk planning, chunk ordering validation.
- Depends on: Public texture metadata in `include/libbsa/archive.hpp`, public result types, `DirectXTex.h` only in `src/texture/directxtex_analyzer.cpp`, and `src/detail/binary_io.*` for header bytes.
- Used by: BA2 DX10 parser, reader, writer, fixture tests, and DX10 writer tests.

**Build/Test/Packaging Layer:**
- Purpose: Build the library, expose installable CMake package config, run tests, generate fixtures, and verify consumer integration.
- Location: `CMakeLists.txt`, `tests/CMakeLists.txt`, `cmake/`, `tests/package-consumer/`, `benchmarks/`
- Contains: CMake target and file set, vcpkg package discovery, Catch2 tests, fixture generators, package-consumer smoke scripts, benchmark tooling.
- Depends on: CMake 4.0+, vcpkg packages from `vcpkg.json`, and Windows MSVC presets from `CMakePresets.json`.
- Used by: Local development, CI, packaging validation, and GSD verification.

## Data Flow

### Primary Archive Open Path

1. Caller invokes `archive_reader::open(host_path)` (`src/archive.cpp:82`).
2. `src/archive.cpp` reads a bounded detection prefix and archive file size (`src/archive.cpp:58`, `src/archive.cpp:72`).
3. BA2 archives are identified by `BTDX` and routed through `formats::ba2::detect_ba2_format` (`src/archive.cpp:109`, `src/formats/ba2/ba2_format_detector.cpp:35`).
4. BA2 DX10 archives call `parse_ba2_dx10_archive_file`; BA2 GNRL archives call `parse_ba2_gnrl_archive_file` (`src/archive.cpp:122`, `src/archive.cpp:133`).
5. Non-BA2 archives call `formats::bsa::detect_bsa_format`, then dispatch to TES3 or TES4-family parsers (`src/archive.cpp:141`, `src/archive.cpp:153`, `src/archive.cpp:164`).
6. Parsers materialize `archive_metadata` and sorted `entry_metadata` vectors while validating spans, names, counts, duplicate canonical paths, hashes, and payload boundaries (`src/formats/bsa/tes3_bsa_parser.cpp:203`, `src/formats/bsa/tes4_bsa_parser.cpp:344`, `src/formats/ba2/ba2_gnrl_parser.cpp:259`, `src/formats/ba2/ba2_dx10_parser.cpp:387`).
7. `archive_reader` stores immutable parsed state in `archive_reader::state` with metadata, entries, host path, and BA2 DX10 flag (`src/archive.cpp:26`).

### Entry Listing, Lookup, and Extraction

1. `archive_reader::entries`, `find`, and `contains` route to the active format reader helpers (`src/archive.cpp:182`, `src/archive.cpp:196`, `src/archive.cpp:210`).
2. Format readers normalize lookup paths through `detail::normalize_archive_path` and binary-search sorted entries (`src/formats/bsa/tes4_bsa_reader.cpp:226`, `src/formats/ba2/ba2_gnrl_reader.cpp:173`, `src/formats/ba2/ba2_dx10_reader.cpp:168`).
3. `archive_reader::extract` looks up the entry first and returns `not_found` when the normalized path is absent (`src/archive.cpp:223`).
4. TES3 raw extraction streams payload bytes from the archive to a caller `payload_sink` (`src/formats/bsa/tes3_bsa_reader.cpp:97`).
5. TES4-family extraction handles embedded-name prefixes, compression size prefixes, deflate/LZ4 frame decode, and chunked sink writes (`src/formats/bsa/tes4_bsa_reader.cpp:177`).
6. BA2 GNRL extraction streams raw payloads or decompresses deflate/raw LZ4 block payloads based on parsed metadata only (`src/formats/ba2/ba2_gnrl_reader.cpp:198`).
7. BA2 DX10 extraction reconstructs a DDS DXT10 header, validates texture metadata presence, then writes decoded chunks in parser-validated DDS order (`src/formats/ba2/ba2_dx10_reader.cpp:193`).
8. `archive_reader::extract_bytes` is a bounded convenience wrapper over `extract` using `vector_payload_sink` (`src/archive.cpp:249`).
9. `archive_reader::extract_entries` runs independent extraction requests through `detail::run_indexed_work`, keeping per-entry failures inside result records (`src/archive.cpp:271`, `src/detail/parallel_work.cpp:20`).

### Public Writer Path

1. Caller constructs a writer from `include/libbsa/writer.hpp`; implementation state lives in the matching format writer source file (`src/formats/bsa/tes3_bsa_writer.cpp:23`, `src/formats/bsa/tes4_bsa_writer.cpp:28`, `src/formats/ba2/ba2_gnrl_writer.cpp:31`, `src/formats/ba2/ba2_dx10_writer.cpp:35`).
2. `add_file` and `add_bytes` normalize archive paths, preserve display path spelling, and copy memory-backed payloads into writer-owned state (`src/formats/bsa/tes4_bsa_writer.cpp:70`, `src/formats/ba2/ba2_gnrl_writer.cpp:71`, `src/formats/bsa/tes3_bsa_writer.cpp:73`).
3. BA2 DX10 `add_file` reads DDS bytes, calls `texture::analyze_dds_source`, and snapshots validated subresources to temp files instead of retaining one long-lived full DDS buffer (`src/formats/ba2/ba2_dx10_writer.cpp:115`, `src/formats/ba2/ba2_dx10_writer.cpp:167`).
4. `write_to` validates `worker_count`, delegates to the format-owned `write_*_archive` function, and keeps target/policy interpretation private (`src/formats/bsa/tes4_bsa_writer.cpp:102`, `src/formats/ba2/ba2_gnrl_writer.cpp:119`, `src/formats/ba2/ba2_dx10_writer.cpp:187`).
5. Format writers validate entries, prepare compressed or raw payloads, assign offsets, write metadata/payload bytes to a temporary file, then publish or replace the output path (`src/formats/bsa/tes4_bsa_writer.cpp:876`, `src/formats/ba2/ba2_gnrl_writer.cpp:905`, `src/formats/ba2/ba2_dx10_writer.cpp:830`).
6. Writers use `src/detail/atomic_file_ops.hpp` for no-replace and atomic-replace semantics where applicable; BA2 writers also use backup/restore helpers in `src/formats/ba2/ba2_publish.hpp`.

### Validation Path

1. Caller invokes `validate_archive(host_path, options)` (`src/validation.cpp:178`).
2. Setup failures such as empty or unreadable host paths remain result-level errors (`src/validation.cpp:179`, `src/validation.cpp:183`).
3. Readable archive bytes are opened through `archive_reader::open`; unsupported or malformed archives become `validation_report::errors` (`src/validation.cpp:187`).
4. Metadata and entries are read through public `archive_reader` APIs, then target-family and entry-level compatibility warnings are derived from public metadata only (`src/validation.cpp:198`, `src/validation.cpp:207`, `src/validation.cpp:93`, `src/validation.cpp:109`).
5. Optional extractability validation bounds entry/chunk sizes, streams to `discard_payload_sink`, and reports extraction failures as fatal diagnostics (`src/validation.cpp:131`, `src/validation.cpp:146`).

**State Management:**
- Public reader and writer objects use hidden `state` structs owned by `std::shared_ptr`; public headers keep private fields opaque (`include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`).
- Parsed archive reader state is immutable after open: metadata, sorted entries, host path, and BA2 DX10 flag live in `archive_reader::state` (`src/archive.cpp:26`).
- Writer state is mutable until `write_to`; independent writer objects are isolated, and write-call worker scheduling is owned by the writer implementation (`docs/thread-safety.md`).
- Result, metadata, validation reports, and entry records are value types and safe to copy after return.

## Key Abstractions

**`libbsa::result<T>`:**
- Purpose: C++20-compatible expected-like return type for public and private recoverable failures.
- Examples: `include/libbsa/result.hpp`, every `result<...>` function under `src/`.
- Pattern: Return `error{error_code, message}` for I/O, format, unsupported, not-found, and invalid-argument failures; reserve exceptions for programmer misuse such as calling `value()` on an error result.

**`archive_reader`:**
- Purpose: Own a successfully opened archive and expose metadata, deterministic entries, lookup, single extraction, byte convenience extraction, and bulk extraction.
- Examples: `include/libbsa/archive.hpp`, `src/archive.cpp`.
- Pattern: Dispatch once by detected bytes, store canonical metadata entries, and route later operations to format readers by parsed metadata.

**`payload_sink` and Bulk Sink Factory:**
- Purpose: Keep extraction synchronous and caller-owned while supporting bounded streaming and opt-in parallel bulk extraction.
- Examples: `include/libbsa/archive.hpp`, `src/archive.cpp`, `src/formats/*/*reader.cpp`.
- Pattern: Treat partial sink writes as `io_error`; bulk extraction creates one sink per entry through `bulk_extract_sink_factory`.

**Format Parser Result Structs:**
- Purpose: Convert raw archive bytes into public `archive_metadata` and sorted `entry_metadata`.
- Examples: `src/formats/bsa/tes3_bsa_parser.hpp`, `src/formats/bsa/tes4_bsa_parser.hpp`, `src/formats/ba2/ba2_gnrl_parser.hpp`, `src/formats/ba2/ba2_dx10_parser.hpp`.
- Pattern: Use internal raw-record structs inside `.cpp` files and return only public metadata values across parser boundaries.

**Public Writer Classes:**
- Purpose: Provide write-new APIs for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Examples: `include/libbsa/writer.hpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`.
- Pattern: Collect normalized entries in object state, then delegate final serialization to format-owned `write_*_archive` functions.

**`detail::binary_reader` / `detail::binary_writer`:**
- Purpose: Read and write little-endian archive fields from bounded spans and owned buffers.
- Examples: `src/detail/binary_io.hpp`, `src/detail/binary_io.cpp`.
- Pattern: Never advance after a failed read; report truncation as `format_error`.

**Archive Path Normalization:**
- Purpose: Convert archive-internal virtual paths to lowercase forward-slash lookup keys.
- Examples: `src/detail/archive_path.hpp`, `src/detail/archive_path.cpp`.
- Pattern: Reject empty, rooted, drive-rooted, NUL, `.` segment, and `..` segment paths; preserve display spelling separately in `entry_metadata::original_path`.

**Compression Router:**
- Purpose: Centralize explicit compression route selection across deflate, LZ4 frame, raw LZ4 block, and raw copies.
- Examples: `src/detail/compression_router.hpp`, `src/detail/compression_router.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`.
- Pattern: Format modules translate parsed metadata into `detail::compression_method`; codec helpers verify exact decoded size.

**DDS Texture Boundary:**
- Purpose: Hide DirectXTex and represent BA2 DX10 texture shape with libbsa-owned metadata.
- Examples: `src/texture/directxtex_analyzer.hpp`, `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.hpp`, `src/texture/dds_layout.cpp`.
- Pattern: Analyze or reconstruct DDS inside `src/texture/`; pass only `texture_metadata`, `dds_texture_layout`, `planned_texture_chunk`, and `logical_texture_segment` to format code.

## Entry Points

**Library Target:**
- Location: `CMakeLists.txt`
- Triggers: `cmake --build --preset windows-msvc-debug-static`, `cmake --build --preset windows-msvc-debug-shared`
- Responsibilities: Define `libbsa`, install/export `libbsa::libbsa`, link private vcpkg dependencies, expose public header file set, and include tests when enabled.

**Umbrella Public Header:**
- Location: `include/libbsa/libbsa.hpp`
- Triggers: Consumer `#include <libbsa/libbsa.hpp>`
- Responsibilities: Include archive, result, validation, version, and writer public headers.

**Archive Reader Open:**
- Location: `src/archive.cpp`
- Triggers: `libbsa::archive_reader::open(host_path)`
- Responsibilities: Detect archive family/version, parse metadata tables, create reader state, and expose format-routed operations.

**Validation API:**
- Location: `src/validation.cpp`
- Triggers: `libbsa::validate_archive(host_path, options)`
- Responsibilities: Return structured validation diagnostics/warnings and optional extractability checks.

**Writer APIs:**
- Location: `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Triggers: Public writer construction, `add_file`, `add_bytes`, and `write_to`
- Responsibilities: Collect entries, enforce writer options, serialize archives, and publish completed output files.

**Test Executable:**
- Location: `tests/CMakeLists.txt`
- Triggers: `ctest --preset windows-msvc-debug-static --output-on-failure`
- Responsibilities: Build `libbsa_tests`, discover Catch2 tests, run fixture manifest validation, and run package-consumer smoke tests.

**Benchmark Tooling:**
- Location: `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`
- Triggers: `cmake --build --preset windows-msvc-debug-static --target libbsa_benchmark_report`
- Responsibilities: Generate report-only synthetic benchmarks that reopen/extract through public APIs.

## Architectural Constraints

- **Threading:** APIs are synchronous by default. Bulk extraction and writer finalization accept positive worker counts and use `src/detail/parallel_work.cpp` with C++20 `std::jthread`. `worker_count == 1` runs serially; `worker_count == 0` returns `invalid_argument`.
- **Global state:** Archive data is object-owned. Module-level constants are mostly `constexpr`. `src/detail/bethesda_hash.cpp` uses a function-local immutable CRC table, and `src/formats/ba2/ba2_dx10_writer.cpp` uses a function-local atomic counter for snapshot temp directory names.
- **Circular imports:** Not detected in the source layout. Public headers do not include private format headers. Format modules include public headers and `src/detail/` helpers; `src/archive.cpp` is the reader dispatch point.
- **Platform:** The project is Windows-only. `src/detail/atomic_file_ops.hpp` contains the Windows `MoveFileExW` publish path and is part of the supported target behavior.
- **Reference boundary:** `TES5Edit/` is read-only reference material. No implementation, build target, test, fixture generator, package consumer, benchmark, or docs-generation workflow may edit, format, stage, compile, or vendor files from `TES5Edit/`.
- **Dependency boundary:** `libdeflate`, `lz4`, and DirectXTex are private implementation dependencies. Public headers under `include/libbsa/` must not expose `libdeflate`, `lz4`, DirectXTex, DXGI, Windows handles, or vcpkg types.
- **Archive paths:** Archive-internal paths are virtual keys normalized by `src/detail/archive_path.cpp`. Use `std::filesystem::path` only for host paths and test/generated output paths.

## Anti-Patterns

### Treating `TES5Edit/` As Source

**What happens:** Code, fixtures, build targets, or generated outputs are added under `TES5Edit/` or the submodule pointer is modified.
**Why it's wrong:** `TES5Edit/` is a read-only compatibility reference, not vendored libbsa implementation or fixture workspace.
**Do this instead:** Trace behavior from `TES5Edit/` when needed, document non-obvious compatibility constraints in libbsa code, and implement under `src/`, `include/libbsa/`, `tests/`, or `docs/`.

### Leaking Private Dependencies Through Public Headers

**What happens:** Public APIs expose `DirectXTex`, DXGI, `libdeflate`, `lz4`, Windows handles, or private parser record types.
**Why it's wrong:** The reusable library contract stays dependency-light and C++20-compatible; implementation dependencies are private details.
**Do this instead:** Translate dependency data into libbsa-owned values such as `texture_metadata` in `include/libbsa/archive.hpp` and keep adapters in `src/texture/` or `src/detail/`.

### Using Host Filesystem Semantics For Archive Keys

**What happens:** Archive lookup, hashing, duplicate detection, or writer entry storage uses `std::filesystem::path` semantics for virtual archive paths.
**Why it's wrong:** Bethesda archive paths use archive-specific separator, case, and hashing rules independent from host filesystem interpretation.
**Do this instead:** Normalize archive-internal paths through `detail::normalize_archive_path` in `src/detail/archive_path.cpp` and preserve original display spelling separately.

### Inferring Codec Behavior From Names Or Extensions

**What happens:** Extraction or writing chooses deflate, LZ4 frame, or raw LZ4 block from file extension or path text.
**Why it's wrong:** Codec routing is archive metadata and target-profile behavior; extension-based routing corrupts valid variants.
**Do this instead:** Route through `src/detail/compression_router.cpp` using parsed `entry_compression` / `texture_chunk_metadata::compression` or writer target options.

## Error Handling

**Strategy:** Recoverable failures use `libbsa::result<T>` and stable `error_code` categories. Public APIs return structured errors for invalid input, I/O failures, unsupported formats, not-found lookups, and malformed archive bytes.

**Patterns:**
- Use `error_code::invalid_argument` for caller setup errors such as empty host paths or `worker_count == 0` (`src/archive.cpp`, `src/formats/*/*writer.cpp`).
- Use `error_code::io_error` for host filesystem failures and sink/source failures (`src/formats/*/*reader.cpp`, `src/formats/*/*writer.cpp`).
- Use `error_code::unsupported` when detected bytes represent a known but unsupported archive shape (`src/formats/bsa/bsa_format_detector.cpp`, `src/formats/ba2/ba2_format_detector.cpp`).
- Use `error_code::format_error` for malformed supported archives, truncated tables, invalid offsets, invalid sentinels, duplicate canonical paths, size mismatches, and codec decode mismatch (`src/formats/`).
- Translate archive-controlled allocation failures through `src/detail/byte_vector.hpp` instead of allowing `std::bad_alloc` or `std::length_error` to escape parser/extraction flows.
- Keep validation report diagnostics separate from setup result errors in `src/validation.cpp`.

## Cross-Cutting Concerns

**Logging:** Not detected. The library returns structured errors and leaves logging to consumers.

**Validation:** Structural validation is embedded in parsers under `src/formats/`; public validation reports are produced by `src/validation.cpp`; fixture manifest validation lives in `tests/fixtures/generated/validate_fixture_manifests.py`.

**Authentication:** Not applicable. libbsa is a local archive library with no authentication surface.

**Compression:** `src/detail/deflate_codec.cpp` wraps libdeflate, `src/detail/lz4_frame_codec.cpp` wraps LZ4 frame APIs, `src/detail/lz4_block_codec.cpp` wraps raw LZ4 block APIs, and `src/detail/compression_router.cpp` is the shared dispatch point.

**Texture Metadata:** `src/texture/directxtex_analyzer.cpp` is the DirectXTex boundary; `src/texture/dds_layout.cpp` owns DXT10 header bytes, mip sizing, chunk planning, and parser chunk-order validation.

**Publishing:** Writers emit to a temporary path first, then publish through `src/detail/atomic_file_ops.hpp` or BA2 backup/restore helpers in `src/formats/ba2/ba2_publish.hpp`.

**Documentation:** Public API documentation is generated through Doxygen configuration in `docs/Doxyfile.in`; thread-safety rules are documented in `docs/thread-safety.md`.

---

*Architecture analysis: 2026-05-11*
