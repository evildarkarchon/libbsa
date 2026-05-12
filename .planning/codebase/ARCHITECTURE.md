# Architecture

**Analysis Date:** 2026-05-11

## System Overview

```text
┌─────────────────────────────────────────────────────────────┐
│                  Public C++20 API Layer                     │
│ `include/libbsa/archive.hpp` `include/libbsa/writer.hpp`    │
│ `include/libbsa/validation.hpp` `include/libbsa/result.hpp` │
└──────────────┬───────────────────┬──────────────────────────┘
               │                   │
               ▼                   ▼
┌──────────────────────────────┐  ┌────────────────────────────┐
│ Reader / Validation Facades  │  │ Writer Facades             │
│ `src/archive.cpp`            │  │ `src/formats/*/*writer.cpp`│
│ `src/validation.cpp`         │  │                            │
└──────┬────────────┬──────────┘  └──────┬──────────┬──────────┘
       │            │                    │          │
       ▼            ▼                    ▼          ▼
┌──────────────┐ ┌──────────────┐  ┌──────────────┐ ┌────────────┐
│ BSA Formats  │ │ BA2 Formats  │  │ Prepare      │ │ Layout     │
│ `src/formats │ │ `src/formats │  │ `*_prepare`  │ │ `*_layout` │
│ /bsa/`       │ │ /ba2/`       │  │              │ │            │
└──────┬───────┘ └──────┬───────┘  └──────┬───────┘ └─────┬──────┘
       │                │                 │               │
       └────────┬───────┴─────────────────┴───────┬───────┘
                ▼                                  ▼
┌────────────────────────────────┐  ┌───────────────────────────┐
│ Shared Detail Infrastructure   │  │ Texture Integration       │
│ `src/detail/*`                 │  │ `src/texture/*`           │
│ binary I/O, paths, codecs, I/O │  │ DDS layout + DirectXTex   │
└────────────────────────────────┘  └───────────────────────────┘
                │                                  │
                ▼                                  ▼
┌─────────────────────────────────────────────────────────────┐
│ Host filesystem archive files, caller sinks, fixture data    │
│ `tests/fixtures/generated/` `docs/` `TES5Edit/` reference    │
└─────────────────────────────────────────────────────────────┘
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| Public umbrella header | Aggregates stable consumer-facing archive, writer, validation, result, and version headers. | `include/libbsa/libbsa.hpp` |
| Public reader API | Defines archive variants, entry metadata, extraction sinks, bulk extraction requests/results, and `archive_reader`. | `include/libbsa/archive.hpp` |
| Public writer API | Defines target profiles, compression policies, writer options, and writer classes for TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10. | `include/libbsa/writer.hpp` |
| Public result/error model | Provides C++20 `libbsa::result<T>` and stable `error_code` categories. | `include/libbsa/result.hpp` |
| Public validation API | Provides `validate_archive`, validation options, fatal diagnostics, and compatibility warnings. | `include/libbsa/validation.hpp` |
| Reader facade | Detects archive family, delegates parsing, stores immutable opened state, routes listing/lookup/extraction, and schedules bulk extraction. | `src/archive.cpp` |
| Validation facade | Opens archives, collects metadata warnings, optionally proves extractability through a discard sink, and returns `validation_report`. | `src/validation.cpp` |
| BSA format layer | Owns TES3 parsing/reading/writing and TES4-family detection, parsing, reading, writer preparation, layout, and serialization. | `src/formats/bsa/` |
| BA2 format layer | Owns BA2 subtype detection plus GNRL and DX10 parsing, reading, writer preparation, layout, and serialization. | `src/formats/ba2/` |
| Shared detail layer | Owns internal path normalization, binary readers/writers, overflow-safe parser primitives, codec routing, parallel work, payload streaming, hashes, byte-vector helpers, and publish-safe output. | `src/detail/` |
| Texture layer | Owns DDS metadata/layout logic and keeps DirectXTex types out of public headers. | `src/texture/` |
| Build target definition | Defines the `libbsa` target, public header file set, private sources, dependencies, install/export rules, docs, tests, and benchmarks. | `CMakeLists.txt` |

## Pattern Overview

**Overall:** Layered library architecture with dependency-light public API, format-specific modules, and shared private infrastructure.

**Key Characteristics:**
- Keep public headers in `include/libbsa/` small and dependency-light; do not expose `libdeflate`, LZ4, DirectXTex, Windows headers, or raw format parser structures from `src/`.
- Route public operations through façade classes and free functions (`archive_reader::open`, writer `write_to`, `validate_archive`) that delegate to format-specific internals in `src/formats/`.
- Use format-family modules that follow a consistent split: detection/parsing/reading for readers and writer façade + prepare + layout + serialize for writers.
- Use `libbsa::result<T>` for expected I/O, format, and caller-data failures; reserve throwing in `result::value()`/`result::error()` for programmer misuse.
- Store canonical archive paths as normalized `/`-separated lowercase keys, never as host `std::filesystem::path` values inside archive metadata.
- Treat `TES5Edit/` as read-only behavioral reference material; implementation, tests, and generated fixtures live outside `TES5Edit/`.

## Layers

**Public API Layer:**
- Purpose: Define the stable consumer contract for opening, listing, extracting, validating, and writing archives.
- Location: `include/libbsa/`
- Contains: Public enums, metadata value types, `payload_sink`, `bulk_extract_sink_factory`, `archive_reader`, writer classes, validation reports, `result<T>`, DLL export macro.
- Depends on: C++20 standard library headers only plus sibling public headers such as `include/libbsa/result.hpp`.
- Used by: Consumers, tests in `tests/unit/`, implementation files in `src/`, package consumer smoke tests in `tests/package-consumer/`.

**Facade Layer:**
- Purpose: Bridge the stable public API to private format implementations.
- Location: `src/archive.cpp`, `src/validation.cpp`, writer façade sections in `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, and `src/formats/ba2/ba2_dx10_writer.cpp`.
- Contains: PIMPL-like public class state, archive family detection, format dispatch, bulk scheduling, writer state mutation, validation diagnostics.
- Depends on: Public headers, format modules under `src/formats/`, and shared helpers under `src/detail/`.
- Used by: Public API consumers through exported symbols.

**BSA Format Layer:**
- Purpose: Implement TES3/Morrowind and TES4-family BSA rules without leaking parser records to public metadata.
- Location: `src/formats/bsa/`
- Contains: `bsa_format_detector`, `tes3_bsa_parser`, `tes3_bsa_reader`, `tes3_bsa_prepare`, `tes3_bsa_layout`, `tes3_bsa_serialize`, `tes3_bsa_writer`, `tes4_bsa_parser`, `tes4_bsa_reader`, `tes4_bsa_prepare`, `tes4_bsa_layout`, `tes4_bsa_serialize`, `tes4_bsa_writer`.
- Depends on: `src/detail/archive_path.*`, `src/detail/bethesda_hash.*`, `src/detail/binary_io.*`, `src/detail/parser_primitives.*`, `src/detail/compression_router.*`, `src/detail/writer_publish.*`, and `src/texture/directxtex_analyzer.*` for DDS writer validation.
- Used by: `src/archive.cpp`, public BSA writer classes, fixture generators in `tests/fixtures/generated/`, unit tests in `tests/unit/`.

**BA2 Format Layer:**
- Purpose: Implement Fallout 4 and Starfield BA2 GNRL/DX10 rules, including subtype detection, name-table handling, chunk metadata, and compression method routing.
- Location: `src/formats/ba2/`
- Contains: `ba2_format_detector`, GNRL parser/reader/prepare/layout/serialize/writer files, DX10 parser/reader/prepare/layout/serialize/writer files.
- Depends on: Shared detail helpers, `src/texture/dds_layout.*`, and `src/texture/directxtex_analyzer.*` for DX10 DDS metadata and chunk planning.
- Used by: `src/archive.cpp`, BA2 public writer classes, BA2 fixture generators, BA2 tests.

**Shared Detail Layer:**
- Purpose: Centralize reusable internal mechanisms that are not part of the public ABI.
- Location: `src/detail/`
- Contains: `binary_reader`/`binary_writer`, safe vector helpers, overflow-safe span arithmetic, normalized archive path keys, Bethesda hashes, compression routers and concrete codec adapters, payload streaming, parallel work, and atomic writer publication.
- Depends on: Public `result.hpp`, C++ standard library, private dependency adapters for libdeflate and LZ4.
- Used by: All format modules and selected façade code.

**Texture Layer:**
- Purpose: Analyze DDS input, expose libbsa-native texture metadata, and reconstruct/plan DDS DXT10 layout without leaking DirectXTex.
- Location: `src/texture/`
- Contains: `directxtex_analyzer.*` for DirectXTex-backed DDS metadata/source analysis and `dds_layout.*` for format-size math, mip/chunk planning, and DXT10 header generation.
- Depends on: `Microsoft::DirectXTex` privately through `src/texture/directxtex_analyzer.cpp`; `src/texture/dds_layout.cpp` uses libbsa-native numeric DXGI format descriptors.
- Used by: BA2 DX10 parser/reader/writer paths and TES4 BSA DDS compatibility checks.

**Test and Fixture Layer:**
- Purpose: Validate public behavior, private helpers, generated archive fixtures, package exports, policies, and compatibility evidence.
- Location: `tests/`
- Contains: Catch2 unit tests in `tests/unit/`, generated fixture sources/manifests in `tests/fixtures/generated/`, export checks in `tests/export-surface/`, package consumer smoke tests in `tests/package-consumer/`.
- Depends on: `libbsa::libbsa`, Catch2, `nlohmann_json`, Python fixture scripts/tooling through `tests/CMakeLists.txt`.
- Used by: CTest presets and CI-style validation.

## Data Flow

### Primary Request Path

1. Public caller opens a host archive with `archive_reader::open(host_path)` (`src/archive.cpp:98`).
2. Reader reads a bounded prefix for detection (`src/archive.cpp:69`) and dispatches by magic to BA2 detection (`src/archive.cpp:112`) or BSA detection (`src/archive.cpp:143`).
3. Format-specific parser loads and validates metadata: TES3 (`src/formats/bsa/tes3_bsa_parser.cpp`), TES4 (`src/formats/bsa/tes4_bsa_parser.cpp`), BA2 GNRL (`src/formats/ba2/ba2_gnrl_parser.cpp`), or BA2 DX10 (`src/formats/ba2/ba2_dx10_parser.cpp`).
4. Parser materializes sorted public `entry_metadata` records and archive metadata, using normalized archive paths from `src/detail/archive_path.cpp` and stable public compression enums from `include/libbsa/archive.hpp`.
5. `archive_reader` stores `archive_metadata`, `entries`, `host_path`, and BA2 DX10 subtype state in `archive_reader::state` (`src/archive.cpp:26`).
6. `entries()`, `find()`, and `contains()` route to the matching format reader lookup functions (`src/archive.cpp:182`, `src/archive.cpp:195`, `src/archive.cpp:209`).
7. `extract()` resolves the entry and delegates payload delivery to the matching format reader (`src/archive.cpp:223`).
8. Format reader streams raw bytes or decodes compressed data through `src/detail/compression_router.cpp` before writing to caller-owned `payload_sink`.

### Writer Finalization Flow

1. Public writer object stores target/options and writer entries in private `state` (`src/formats/bsa/tes4_bsa_writer.cpp:21`, `src/formats/ba2/ba2_gnrl_writer.cpp:21`, `src/formats/ba2/ba2_dx10_writer.cpp:20`).
2. `add_file`/`add_bytes` validates public archive paths and copies memory payloads into writer-owned state; BA2 DX10 `add_file` also creates snapshot-owned source state (`src/formats/ba2/ba2_dx10_writer.cpp:52`).
3. `write_to` validates `write_execution_options::worker_count` and delegates to format-specific `write_*_archive` (`src/formats/bsa/tes4_bsa_writer.cpp:73`, `src/formats/ba2/ba2_gnrl_writer.cpp:89`, `src/formats/ba2/ba2_dx10_writer.cpp:72`).
4. Format writer validates target options and entries, then runs prepare logic for paths, hashes, compression, DDS inspection, and payload ownership (`src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`).
5. Layout module assigns metadata offsets and optional deduplicated payload offsets (`src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_layout.cpp`, `src/formats/ba2/ba2_dx10_layout.cpp`).
6. Serialization module writes metadata and payloads to a temporary output path (`src/formats/bsa/tes4_bsa_serialize.cpp`, `src/formats/ba2/ba2_gnrl_serialize.cpp`, `src/formats/ba2/ba2_dx10_serialize.cpp`).
7. Shared publish logic reserves an isolated temporary directory and atomically publishes or replaces output with overwrite policy (`src/detail/writer_publish.cpp`, `src/detail/atomic_file_ops.hpp`).

### Bulk Extraction Flow

1. Caller passes `bulk_extract_request` span, sink factory, and `bulk_extract_options` to `archive_reader::extract_entries` (`src/archive.cpp:271`).
2. The reader validates `worker_count` and allocates one `bulk_extract_entry_result` per request (`src/archive.cpp:278`).
3. `detail::run_indexed_work` schedules serial or parallel indexed work using `std::jthread` and first-error capture (`src/detail/parallel_work.cpp:20`).
4. Each work item normalizes lookup through `find`, asks the caller factory for a distinct sink, and calls `extract` (`src/archive.cpp:283`).
5. Per-entry failures are recorded in `bulk_extract_entry_result::failure`; independent sibling entries continue unless scheduler setup fails (`src/archive.cpp:288`).

### Validation Flow

1. Caller invokes `validate_archive(host_path, options)` (`src/validation.cpp:178`).
2. Validation checks setup failures separately, opens through `archive_reader::open`, and converts malformed readable archives into report diagnostics (`src/validation.cpp:186`).
3. Validation appends target-family and entry-level compatibility warnings based only on public metadata (`src/validation.cpp:92`, `src/validation.cpp:108`).
4. Optional extractability validation streams entries to a `discard_payload_sink` without retaining archive-controlled payload bytes (`src/validation.cpp:145`).
5. The returned `validation_report` contains stable fatal diagnostics and warnings from `include/libbsa/validation.hpp`.

**State Management:**
- Public readers and writers hold private shared state via `std::shared_ptr<state>` (`include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `src/archive.cpp`, writer `.cpp` files). Treat public object copies as shared handles to the same underlying state.
- Opened `archive_reader::state` is immutable through public const operations except for local result construction; each extraction opens its own `std::ifstream` or local scratch buffers.
- Writer state is mutable during `add_file`/`add_bytes` and read during `write_to`; caller must not mutate one writer concurrently with other operations on that writer.
- Global mutable state is not used for archive operations; this is documented in `docs/thread-safety.md`.

## Key Abstractions

**`libbsa::result<T>`:**
- Purpose: Expected-like public return channel for I/O, format, unsupported, invalid argument, and not-found failures.
- Examples: `include/libbsa/result.hpp`, `src/archive.cpp`, `src/validation.cpp`, all `src/formats/` modules.
- Pattern: Return `error{error_code::..., "diagnostic"}` for expected failures; use `value()` only after truth checks.

**`archive_reader`:**
- Purpose: Format-neutral opened archive handle with metadata, listing, lookup, extraction, and bulk extraction APIs.
- Examples: `include/libbsa/archive.hpp`, `src/archive.cpp`.
- Pattern: Dispatch by parsed `archive_metadata` and private `is_ba2_dx10` state; do not expose parser-owned records.

**`payload_sink` and `bulk_extract_sink_factory`:**
- Purpose: Caller-owned streaming output boundary for extraction.
- Examples: `include/libbsa/archive.hpp`, `src/archive.cpp`, `src/formats/*/*reader.cpp`.
- Pattern: Every sink `write` must accept the complete span; partial acceptance becomes `error_code::io_error`.

**Writer Classes:**
- Purpose: Write-new archive builders for target profiles without raw flag exposure.
- Examples: `include/libbsa/writer.hpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`.
- Pattern: Store entries in private state, run prepare/layout/serialize on `write_to`, publish through shared writer publication helpers.

**Format Detectors:**
- Purpose: Parse only enough prefix bytes to identify supported format family/version/subtype and default compression route.
- Examples: `src/formats/bsa/bsa_format_detector.*`, `src/formats/ba2/ba2_format_detector.*`, `src/archive.cpp`.
- Pattern: Detection remains separate from full parsing so the facade can select the correct parser.

**Parser Primitives:**
- Purpose: Centralize safe arithmetic, bounded byte reads, archive string conversion, and display separator normalization.
- Examples: `src/detail/parser_primitives.hpp`, `src/detail/parser_primitives.cpp`, `src/detail/binary_io.hpp`, `src/detail/binary_io.cpp`.
- Pattern: Use `add_fits`, `multiply_fits`, and `span_fits*` before offset math or allocation.

**Compression Router:**
- Purpose: Map parsed/writer-selected compression methods to private deflate/LZ4 adapters.
- Examples: `src/detail/compression_router.hpp`, `src/detail/compression_router.cpp`, `src/detail/deflate_codec.*`, `src/detail/lz4_frame_codec.*`, `src/detail/lz4_block_codec.*`.
- Pattern: Choose codec explicitly from archive family/version/metadata; never infer compression solely from extension.

**DDS Texture Metadata/Layout:**
- Purpose: Convert DDS/DirectXTex data into libbsa-native `texture_metadata`, reconstruct DDS DXT10 headers, and plan BA2 DX10 chunk boundaries.
- Examples: `include/libbsa/archive.hpp`, `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.cpp`, `src/formats/ba2/ba2_dx10_*`.
- Pattern: Keep DirectXTex in `.cpp` implementation files and expose only numeric DXGI format values plus libbsa metadata.

**Writer Publish Boundary:**
- Purpose: Prevent partial/corrupt caller-visible archives by writing to a temporary path and publishing after successful serialization.
- Examples: `src/detail/writer_publish.cpp`, `src/detail/atomic_file_ops.hpp`, writer `write_*_archive` functions.
- Pattern: Validate overwrite policy first, reserve isolated temp directories, best-effort cleanup, then publish or replace atomically.

## Entry Points

**Public Reader Open:**
- Location: `src/archive.cpp`
- Triggers: Caller invokes `archive_reader::open` declared in `include/libbsa/archive.hpp`.
- Responsibilities: Validate host path, read detection prefix, detect BSA/BA2, parse metadata, construct reader state.

**Public Metadata/List/Lookup/Extraction:**
- Location: `src/archive.cpp`
- Triggers: Caller invokes `metadata`, `entries`, `find`, `contains`, `extract`, `extract_bytes`, or `extract_entries` on `archive_reader`.
- Responsibilities: Route to format-specific reader helpers and preserve stable public metadata/error semantics.

**Public Writer Finalization:**
- Location: `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Triggers: Caller invokes writer `write_to` declared in `include/libbsa/writer.hpp`.
- Responsibilities: Validate execution controls, prepare entries, assign layout, serialize, and publish output.

**Public Archive Validation:**
- Location: `src/validation.cpp`
- Triggers: Caller invokes `validate_archive` declared in `include/libbsa/validation.hpp`.
- Responsibilities: Open archive, collect stable fatal diagnostics/warnings, optionally extract entries to a discard sink.

**CMake Build Entry:**
- Location: `CMakeLists.txt`
- Triggers: `cmake --preset windows-msvc-debug-static`, `cmake --preset windows-msvc-debug-shared`, or install/package consumers.
- Responsibilities: Configure library target, public header file set, dependency links, tests, benchmarks, Doxygen target, and install/export files.

## Architectural Constraints

- **Threading:** Default public operations are serial. Parallel extraction and writer preparation use explicit `worker_count` options and shared scheduler `src/detail/parallel_work.cpp`. `worker_count == 0` is invalid; `worker_count > 1024` is rejected internally.
- **Global state:** Archive operations use no global mutable state. State lives in reader/writer `state` objects, local `std::ifstream`/`std::ofstream` instances, local scratch vectors, and caller-owned sinks. See `docs/thread-safety.md`.
- **Circular imports:** Not detected in the active C++ architecture. Public headers do not include private `src/` headers; private modules include public headers and sibling private headers.
- **Public dependency boundary:** Public headers must not expose DirectXTex, libdeflate, LZ4, raw parser record structs, or Windows implementation headers. `include/libbsa/export.hpp` is the only public Windows ABI decoration point.
- **Path boundary:** Use `std::filesystem::path` for host I/O and publishing only (`src/detail/writer_publish.cpp`, writer output code). Use `detail::normalize_archive_path` and string keys for archive-internal paths (`src/detail/archive_path.cpp`).
- **Reference boundary:** `TES5Edit/` is read-only behavior reference; do not compile it, use it as source, write fixtures into it, modify it, or stage submodule pointer changes.
- **Error boundary:** Public expected failures use `result<T>` and `error_code`; avoid exceptions for archive bytes, host I/O, compression failures, validation failures, and caller data errors.
- **Memory boundary:** Streaming readers/writers use 64 KiB chunks for raw payload paths (`src/formats/*/*reader.cpp`, `src/formats/bsa/tes4_bsa_serialize.cpp`). Compressed payload decode remains bounded by parser-declared exact sizes and byte-vector allocation helpers.

## Anti-Patterns

### Leaking Private Dependencies Through Public Headers

**What happens:** Public headers include DirectXTex, LZ4, libdeflate, private `src/detail/` headers, or format parser records.
**Why it's wrong:** Public consumers should only depend on stable C++20 libbsa types and must not inherit implementation dependencies or ABI churn.
**Do this instead:** Keep dependency use inside `src/texture/directxtex_analyzer.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, and `src/detail/lz4_block_codec.cpp`; translate to public values in `include/libbsa/archive.hpp`.

### Treating Archive Paths as Host Filesystem Paths

**What happens:** Code stores archive-internal keys as `std::filesystem::path` or applies host separator/case semantics.
**Why it's wrong:** Bethesda virtual paths require explicit normalization independent of Windows host paths and are used for lookup/hash behavior.
**Do this instead:** Normalize with `detail::normalize_archive_path` in `src/detail/archive_path.cpp`, preserve display spelling separately as `entry_metadata::original_path`, and use host filesystem paths only for source/output file access.

### Skipping Prepare/Layout/Serialize Separation in Writers

**What happens:** New writer code computes hashes, compresses payloads, assigns offsets, and writes bytes in one monolithic routine.
**Why it's wrong:** Existing writer modules need independent validation, deterministic layout tests, dedupe behavior, and safe publish semantics.
**Do this instead:** Add writer behavior through the existing split: `*_prepare.*` for validation/materialization, `*_layout.*` for offsets/deduplication, `*_serialize.*` for byte output, and `*_writer.cpp` for public routing.

### Inferring Codec From Filename Extension

**What happens:** Reader or writer code chooses deflate/LZ4/raw based only on `.dds`, `.xwm`, or other archive path extensions.
**Why it's wrong:** Codec route is archive-family/version/metadata-driven; extension-based inference can silently corrupt Starfield and Skyrim SE payloads.
**Do this instead:** Route through explicit metadata and `detail::compression_method` in `src/detail/compression_router.cpp`, using format-specific decisions in parser/prepare modules.

### Writing Directly to Caller Output Paths

**What happens:** Writer serialization creates or truncates the caller's final path before all metadata and payload writes succeed.
**Why it's wrong:** A failed write can leave corrupt archives visible at the destination.
**Do this instead:** Use `detail::publish_writer_output` and helpers in `src/detail/writer_publish.cpp` for validation, temporary output, best-effort cleanup, and final publish.

## Error Handling

**Strategy:** Public and private library operations return `result<T>` for recoverable errors. Stable categories are `unsupported`, `invalid_argument`, `not_found`, `io_error`, and `format_error` from `include/libbsa/result.hpp`.

**Patterns:**
- Validate input early and return `error_code::invalid_argument` for empty paths or invalid worker counts (`src/archive.cpp:99`, `src/archive.cpp:278`, writer `write_to` methods).
- Use `error_code::unsupported` for recognized-but-not-supported format families or target combinations (`src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`).
- Use `error_code::format_error` for malformed archive bytes, overflowed metadata, truncated tables, hash mismatches, invalid sentinels, codec exact-size mismatches, and parser-validated constraints.
- Use `error_code::io_error` for host file open/seek/read/write/publish failures (`src/detail/parser_primitives.cpp`, `src/detail/writer_publish.cpp`, reader/writer modules).
- Convert `std::bad_alloc`/`std::length_error` at archive-controlled allocation boundaries into `format_error` via byte-vector helpers and validation catch blocks (`src/detail/parser_primitives.cpp`, `src/validation.cpp`).
- Do not compare exact diagnostic message text in tests or consumers; stable branching uses `error_code` and validation warning codes.

## Cross-Cutting Concerns

**Logging:** No internal logging framework. Return structured errors and warnings; consumers own logging. Avoid adding `spdlog`, `fmt`, or other logging dependencies.
**Validation:** Structural validation happens during parsing, public policy validation happens in `src/validation.cpp`, and behavior/policy tests live in `tests/unit/`.
**Authentication:** Not applicable; libbsa is a local archive library with no auth subsystem.
**Compression:** Deflate, LZ4 frame, and raw LZ4 block are private codec adapters selected by explicit format metadata through `src/detail/compression_router.cpp`.
**DDS/Texture:** DirectXTex is isolated to `src/texture/directxtex_analyzer.cpp`; public metadata uses libbsa-owned numeric and value types.
**Documentation:** Public APIs use Doxygen-style comments in `include/libbsa/`; design policies and target behavior are documented in `docs/thread-safety.md`, `docs/target-format-guide.md`, `docs/compatibility-evidence.md`, and `docs/integration-examples.md`.
**OpenSpec/GSD workflow:** Project-local OpenSpec skills live in `.claude/skills/openspec-*`; active and archived change artifacts live in `openspec/changes/` and specs in `openspec/specs/`.

---

*Architecture analysis: 2026-05-11*
