---
last_mapped: 2026-05-15
last_mapped_commit: ead68b6856b37c3ea3acbf007180951c3cc22698
---
<!-- refreshed: 2026-05-15 -->
# Architecture

**Analysis Date:** 2026-05-15

## System Overview

```text
┌─────────────────────────────────────────────────────────────┐
│                    Public C++20 API                         │
│ `include/libbsa/libbsa.hpp`                                 │
├──────────────────┬──────────────────┬───────────────────────┤
│  archive_reader  │  writer classes  │ validation/reporting   │
│ `archive.hpp`    │ `writer.hpp`     │ `validation.hpp`      │
└────────┬─────────┴────────┬─────────┴──────────┬────────────┘
         │                  │                     │
         ▼                  ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│                 Public-to-internal adapters                  │
│ `src/archive.cpp`, `src/validation.cpp`, writer pimpl files  │
└────────┬──────────────────┬─────────────────────┬───────────┘
         │                  │                     │
         ▼                  ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  Format-specific engines                     │
├──────────────────────────────┬──────────────────────────────┤
│ BSA: TES3 + TES4 family       │ BA2: GNRL + DX10             │
│ `src/formats/bsa/`            │ `src/formats/ba2/`           │
│ detect → parse/read/write     │ detect → parse/read/write    │
└────────┬─────────────────────┴───────────────┬──────────────┘
         │                                     │
         ▼                                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    Shared detail services                    │
│ paths, binary I/O, payload streaming, codecs, publishing     │
│ `src/detail/`                                                │
└────────┬─────────────────────────────────────┬──────────────┘
         │                                     │
         ▼                                     ▼
┌─────────────────────────────────────────────────────────────┐
│             External implementation dependencies             │
│ libdeflate, official LZ4, DirectXTex, Windows bcrypt/API     │
│ linked from `CMakeLists.txt`                                 │
└─────────────────────────────────────────────────────────────┘
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| Umbrella public include | Pulls the stable public API together without exposing internals. | `include/libbsa/libbsa.hpp` |
| Reader API | Defines archive metadata, entry metadata, payload sinks, bulk extraction contracts, and `archive_reader`. | `include/libbsa/archive.hpp` |
| Writer API | Defines target profiles, compression policies, execution options, and TES3/TES4/BA2 writer classes. | `include/libbsa/writer.hpp` |
| Result/error API | Provides C++20-compatible expected-like `libbsa::result<T>` and stable `error_code` categories. | `include/libbsa/result.hpp` |
| Validation API | Exposes structured validation diagnostics and compatibility warnings. | `include/libbsa/validation.hpp` |
| Reader dispatcher | Opens host archives, detects family/subtype, parses metadata, stores resolved host path, and dispatches reader calls through callbacks. | `src/archive.cpp` |
| Validation adapter | Reuses `archive_reader` to build reports and optionally streams every entry to a discard sink. | `src/validation.cpp` |
| TES3 BSA engine | Implements Morrowind BSA detection result usage, parsing, lookup, raw extraction, preparation, layout, serialization, and writing. | `src/formats/bsa/tes3_bsa_*.{hpp,cpp}` |
| TES4-family BSA engine | Implements Oblivion/Fallout/Skyrim BSA parsing, lookup, embedded-name handling, deflate/LZ4-frame extraction, writer preparation/layout/serialization. | `src/formats/bsa/tes4_bsa_*.{hpp,cpp}` |
| BA2 GNRL engine | Implements Fallout 4/Starfield BA2 general-file archives including v2/v3 metadata and deflate/raw-LZ4 block routing. | `src/formats/ba2/ba2_gnrl_*.{hpp,cpp}` |
| BA2 DX10 engine | Implements BA2 texture archives, DDS header reconstruction, chunk validation/order, DirectXTex-backed writer analysis, and chunk serialization. | `src/formats/ba2/ba2_dx10_*.{hpp,cpp}` |
| Texture adapter | Owns DirectXTex integration and libbsa-native DDS layout/chunk metadata translation. | `src/texture/directxtex_analyzer.*`, `src/texture/dds_layout.*` |
| Codec routing | Maps internal compression methods to libdeflate, LZ4 frame, or raw LZ4 block helpers and enforces exact-size decompression. | `src/detail/compression_router.*`, `src/detail/deflate_codec.*`, `src/detail/lz4_*_codec.*` |
| Host path/file seams | Resolves caller UTF-8 host paths once into Windows-native `std::filesystem::path` and centralizes open/read diagnostics. | `src/detail/host_file_path.*`, `src/detail/host_file.*` |
| Safe writer publication | Reserves isolated temp directories, rejects unsafe overwrite targets, and atomically publishes finished archives. | `src/detail/writer_publish.*`, `src/detail/atomic_file_ops.hpp` |
| Test graph | Builds unit tests, fixture generators, package-consumer smoke tests, and shared export checks. | `tests/CMakeLists.txt` |

## Pattern Overview

**Overall:** Layered library with dependency-light public headers, format-specific internal engines, shared safety primitives, and pimpl-backed writer state.

**Key Characteristics:**
- Public headers under `include/libbsa/` expose value types, C++20 `result<T>`, and abstract payload sinks; they do not expose libdeflate, LZ4, DirectXTex, Windows handles, parser records, or writer layout internals.
- `src/archive.cpp` is the main reader dispatch seam: it detects BSA vs BA2, selects one backend table, and all public methods reuse that selected backend.
- Each archive writer follows a staged pipeline: public add methods populate writer-owned entries, `*_prepare` validates and materializes/compresses payload state, `*_layout` assigns offsets/deduplication, `*_serialize` writes bytes, and `detail::publish_writer_output` publishes safely.
- Archive virtual paths are normalized with `detail::normalize_archive_path` in `src/detail/archive_path.cpp`; host filesystem paths are resolved by `detail::resolve_host_file_path` in `src/detail/host_file_path.cpp` and must not be reused as archive keys.
- Format compatibility quirks remain in format-specific files: TES3 relative data offsets stay in `src/formats/bsa/tes3_bsa_*`, TES4 embedded names and version-specific compression stay in `src/formats/bsa/tes4_bsa_*`, BA2 Starfield v3 `CompressionMethod` routing stays in `src/formats/ba2/`.

## Layers

**Public API Layer:**
- Purpose: Stable reusable C++20 interface for consumers.
- Location: `include/libbsa/`
- Contains: `archive_reader`, `payload_sink`, writer classes, target enums, metadata structs, validation reports, `result<T>`.
- Depends on: C++ standard library only plus `LIBBSA_API` export annotations in `include/libbsa/export.hpp`.
- Used by: Downstream consumers, `tests/package-consumer/main.cpp`, unit tests using public API.

**Public Adapter Layer:**
- Purpose: Translate API calls into internal engines while preserving the public error model.
- Location: `src/archive.cpp`, `src/validation.cpp`, writer class implementation files such as `src/formats/bsa/tes4_bsa_writer.cpp`.
- Contains: Reader backend callback table, pimpl `state` definitions, validation report assembly, public writer `add_*` and `write_to` methods.
- Depends on: `src/formats/`, `src/detail/`, and public headers.
- Used by: Public API entry points.

**Format Detection Layer:**
- Purpose: Identify archive family/subtype/version before parsing full metadata.
- Location: `src/formats/bsa/bsa_format_detector.*`, `src/formats/ba2/ba2_format_detector.*`.
- Contains: BSA magic/version detection, BA2 `BTDX` subtype/version detection, Starfield BA2 v2/v3 extra header fields, default compression metadata.
- Depends on: `src/detail/binary_io.*` and public metadata enums.
- Used by: `archive_reader::open` in `src/archive.cpp`.

**Format Parser/Reader Layer:**
- Purpose: Convert archive bytes into deterministic public metadata and extract payload bytes.
- Location: `src/formats/bsa/*_parser.*`, `src/formats/bsa/*_reader.*`, `src/formats/ba2/*_parser.*`, `src/formats/ba2/*_reader.*`.
- Contains: Checked header/table/name parsing, duplicate/path/hash validation, sorted entry metadata, lookup, raw streaming, decompression, DDS reconstruction for DX10.
- Depends on: `src/detail/binary_io.*`, `src/detail/parser_primitives.*`, `src/detail/payload_stream.*`, `src/detail/compression_router.*`, `src/texture/dds_layout.*` for DX10.
- Used by: `archive_reader` backend table in `src/archive.cpp` and validation in `src/validation.cpp`.

**Format Writer Layer:**
- Purpose: Build new archives from caller-owned paths/bytes through target-specific compatibility rules.
- Location: `src/formats/bsa/*_writer.*`, `*_prepare.*`, `*_layout.*`, `*_serialize.*`; `src/formats/ba2/*_writer.*`, `*_prepare.*`, `*_layout.*`, `*_serialize.*`.
- Contains: Add-time entry validation, source ownership/snapshotting, compression routing, hash/sort rules, payload deduplication, offset layout, byte serialization.
- Depends on: Shared detail helpers, codec routing, texture analyzer for BA2 DX10, writer publication helper.
- Used by: Public writer classes in `include/libbsa/writer.hpp`.

**Shared Detail Layer:**
- Purpose: Centralize cross-format safety and platform behavior.
- Location: `src/detail/`.
- Contains: Binary reader/writer, parser bounds checks, archive path normalization, host path resolution, host file reads, bounded payload streaming, byte-vector allocation helpers, parallel work scheduling, compression codecs, safe writer publish routines, Bethesda hashing.
- Depends on: Public result/error types and selected third-party libraries in implementation files.
- Used by: All format engines and adapters.

**Texture Layer:**
- Purpose: Keep DDS and DirectXTex implementation details out of public and format layers.
- Location: `src/texture/`.
- Contains: DDS DXT10 header construction, mip/chunk planning, chunk validation/order, DirectXTex metadata/source analysis.
- Depends on: Public texture metadata values and private DirectXTex implementation.
- Used by: BA2 DX10 parser, reader, preparation, and snapshot builder files under `src/formats/ba2/`.

**Build/Test Layer:**
- Purpose: Build static/shared library variants, docs, benchmarks, unit tests, fixtures, and package-consumer proof.
- Location: `CMakeLists.txt`, `tests/CMakeLists.txt`, `CMakePresets.json`, `cmake/libbsaConfig.cmake.in`.
- Contains: `libbsa` target, install/export package config, vcpkg dependencies, test executable, fixture-generator targets, CTest registrations.
- Depends on: CMake 4.0+, vcpkg packages, Catch2, nlohmann_json for tests, Python for fixture validation.
- Used by: Windows MSVC verification lanes.

## Data Flow

### Primary Archive Open + Extract Path

1. Caller opens an archive through `archive_reader::open(host_path)` (`src/archive.cpp:183`).
2. `archive_reader::open` rejects empty paths, resolves the host path once with `detail::resolve_host_file_path`, and reads a 36-byte detection prefix (`src/archive.cpp:185`, `src/archive.cpp:190`, `src/archive.cpp:149`).
3. BA2 archives are detected by `BTDX`, routed through `formats::ba2::detect_ba2_format`, and parsed as DX10 or GNRL (`src/archive.cpp:217`, `src/archive.cpp:222`, `src/archive.cpp:233`).
4. BSA archives are detected through `formats::bsa::detect_bsa_format`; TES3 routes to `parse_tes3_bsa_archive_file`, all supported TES4-family versions route to `parse_tes4_bsa_archive_file` (`src/archive.cpp:260`, `src/archive.cpp:271`, `src/archive.cpp:285`).
5. The opened reader stores public metadata, sorted `entry_metadata`, the resolved host path, and one backend callback table (`src/archive.cpp:88`, `src/archive.cpp:203`).
6. Caller looks up or extracts by archive path. `find()` uses backend lookup, which normalizes with `detail::normalize_archive_path` and binary-searches sorted entries (`src/archive.cpp:315`, `src/formats/bsa/tes4_bsa_reader.cpp:105`).
7. `extract()` passes the selected backend, resolved host path, entry, and caller sink to `extract_entry_payload` (`src/archive.cpp:338`, `src/archive.cpp:159`).
8. Format reader streams raw payloads via `detail::stream_payload_range` or routes compressed data through `detail::decompress_payload_exact_to_sink` (`src/formats/bsa/tes4_bsa_reader.cpp:66`, `src/formats/bsa/tes4_bsa_reader.cpp:93`).

### Bulk Extraction Path

1. Caller passes `bulk_extract_request` records, a `bulk_extract_sink_factory`, and `bulk_extract_options` to `archive_reader::extract_entries` (`include/libbsa/archive.hpp:301`).
2. `extract_entries` rejects unopened readers and `worker_count == 0` (`src/archive.cpp:394`, `src/archive.cpp:398`).
3. Requests are coalesced by exact caller-supplied path string before lookup; later duplicates mirror the first occurrence result (`src/archive.cpp:403`, `src/archive.cpp:410`).
4. Unique groups run through `detail::run_indexed_work` with the caller-selected worker count (`src/archive.cpp:429`, `src/archive.cpp:473`).
5. Each group performs lookup, calls the sink factory once, extracts into the returned sink, and records per-entry failures without aborting siblings (`src/archive.cpp:435`, `src/archive.cpp:450`, `src/archive.cpp:464`).

### Validation Path

1. Caller invokes `validate_archive(host_path, options)` (`src/validation.cpp:203`).
2. Validation delegates archive structure detection/parsing to `archive_reader::open` (`src/validation.cpp:210`).
3. Unreadable host paths remain result-level failures; readable unsupported/malformed archive bytes become `validation_report::errors` (`src/validation.cpp:213`, `src/validation.cpp:217`).
4. Metadata and entry listings are read through public reader APIs (`src/validation.cpp:225`, `src/validation.cpp:236`).
5. Target-family, embedded-name, and compressed-sound warnings are derived from public metadata only (`src/validation.cpp:97`, `src/validation.cpp:115`).
6. Optional extractability validation streams each entry to `discard_payload_sink` without retaining archive-controlled payload bytes (`src/validation.cpp:161`, `src/validation.cpp:177`).

### Writer Finalization Path

1. Caller constructs one writer class from `include/libbsa/writer.hpp` and stages entries through `add_file` or `add_bytes`.
2. Public writer state lives behind a private `std::unique_ptr<state>` pimpl; TES4 state stores target/options/entries in `src/formats/bsa/tes4_bsa_writer.cpp:23`.
3. `write_to` validates `write_execution_options::worker_count` and calls the format-specific `write_*_archive` function (`src/formats/bsa/tes4_bsa_writer.cpp:88`).
4. The format writer resolves the output host path, validates entries/options, prepares/compresses/sorts records, assigns offsets/dedupe, then serializes to a temporary file (`src/formats/bsa/tes4_bsa_writer.cpp:118`, `src/formats/bsa/tes4_bsa_writer.cpp:140`, `src/formats/bsa/tes4_bsa_writer.cpp:152`, `src/formats/bsa/tes4_bsa_writer.cpp:158`).
5. Publication goes through `detail::publish_writer_output`, which validates overwrite policy, reserves a `.libbsa-tmp-*` directory, runs serialization, publishes atomically/no-overwrite, and cleans temporary state (`src/detail/writer_publish.cpp:68`, `src/detail/writer_publish.cpp:110`, `src/detail/writer_publish.cpp:142`).
6. BA2 DX10 writer is one-shot because add-time DDS snapshots are cleaned after a write attempt (`src/formats/ba2/ba2_dx10_writer.cpp:108`, `src/formats/ba2/ba2_dx10_writer.cpp:116`, `src/formats/ba2/ba2_dx10_writer.cpp:120`).

### BA2 DX10 Texture Path

1. Reader parses BA2 DX10 records and texture chunks into public `texture_metadata`/`texture_chunk_metadata` (`include/libbsa/archive.hpp:71`, `include/libbsa/archive.hpp:98`).
2. Extraction builds a libbsa-owned `dds_texture_layout` and writes a reconstructed DXT10 DDS header first (`src/formats/ba2/ba2_dx10_reader.cpp:121`, `src/formats/ba2/ba2_dx10_reader.cpp:127`).
3. Chunks are streamed raw when `entry_compression::none` or decompressed exactly as deflate/raw-LZ4 block chunks (`src/formats/ba2/ba2_dx10_reader.cpp:150`, `src/formats/ba2/ba2_dx10_reader.cpp:161`).
4. Writer add-time DDS analysis is isolated behind `src/texture/directxtex_analyzer.*`; BA2 DX10 entries keep libbsa-owned snapshots rather than DirectXTex objects (`src/texture/directxtex_analyzer.hpp:45`, `src/formats/ba2/ba2_dx10_prepare.hpp:59`).

**State Management:**
- `archive_reader` holds immutable opened state in `std::shared_ptr<const state>` (`src/archive.cpp:88`, `include/libbsa/archive.hpp:311`).
- Writer objects hold mutable staged state behind `std::unique_ptr<state>` and are move-only (`include/libbsa/writer.hpp:201`, `include/libbsa/writer.hpp:254`).
- No global mutable state is used for readers, writers, validation, or benchmarks per `docs/thread-safety.md:3`.
- Parallel work uses local atomics, mutex, and `std::jthread` in `src/detail/parallel_work.cpp`; it caps worker counts at 1024 (`src/detail/parallel_work.cpp:18`).

## Key Abstractions

**`libbsa::result<T>`:**
- Purpose: C++20-compatible error-return model for expected I/O, validation, compression, and format failures.
- Examples: `include/libbsa/result.hpp`, use in `src/archive.cpp`, `src/validation.cpp`, `src/detail/compression_router.cpp`.
- Pattern: Return `result<T>`/`result<void>` for caller-data and archive-data failures; reserve exceptions for programmer misuse of `value()`/`error()` accessors.

**`archive_reader` + backend callback table:**
- Purpose: Keep the public reader shape common across TES3, TES4-family, BA2 GNRL, and BA2 DX10 while preserving format-specific parsing/extraction.
- Examples: `include/libbsa/archive.hpp`, `src/archive.cpp:48`, `src/archive.cpp:56`.
- Pattern: Select backend once at open time; later `entries`, `find`, and `extract` calls dispatch through function pointers.

**`payload_sink` / `bulk_extract_sink_factory`:**
- Purpose: Stream extraction output without forcing archive-controlled payloads into memory.
- Examples: `include/libbsa/archive.hpp:177`, `include/libbsa/archive.hpp:219`, `src/validation.cpp:18`.
- Pattern: Sinks must report full accepted byte counts; partial writes become `io_error` via payload streaming helpers.

**Writer pimpl state:**
- Purpose: Keep public writer headers stable and dependency-light while each format owns internal staged entry structures.
- Examples: `include/libbsa/writer.hpp:251`, `src/formats/bsa/tes4_bsa_writer.cpp:23`, `src/formats/ba2/ba2_dx10_writer.cpp:22`.
- Pattern: Public classes are move-only, own staged entries, and call format-specific pipeline functions during `write_to`.

**Format prepare/layout/serialize pipeline:**
- Purpose: Separate source validation/materialization, offset assignment/deduplication, and byte emission.
- Examples: `src/formats/bsa/tes4_bsa_prepare.hpp`, `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/bsa/tes4_bsa_serialize.cpp`; analogous BA2 files.
- Pattern: Add new writer functionality by extending the relevant `*_prepare`, `*_layout`, and `*_serialize` stages, not by placing format rules in public classes.

**Texture boundary:**
- Purpose: Hide DirectXTex and DDS implementation details while exposing stable libbsa-native metadata.
- Examples: `src/texture/directxtex_analyzer.hpp`, `src/texture/dds_layout.hpp`, `include/libbsa/archive.hpp:98`.
- Pattern: Convert DirectXTex metadata into `texture_metadata`/`dds_texture_layout`; never expose DirectXTex/DXGI types in public headers.

**Resolved host path:**
- Purpose: Avoid reinterpreting raw caller UTF-8 path strings after open/add validation.
- Examples: `src/detail/host_file_path.hpp`, `src/archive.cpp:190`, `src/formats/bsa/tes4_bsa_prepare.hpp:31`.
- Pattern: Resolve once to `detail::host_file_path` and pass that object through later reads/writes.

## Entry Points

**Library target:**
- Location: `CMakeLists.txt:62`
- Triggers: CMake configure/build.
- Responsibilities: Build `libbsa`, set C++20, set includes, link private dependencies, install/export headers/package files.

**Consumer public include:**
- Location: `include/libbsa/libbsa.hpp`
- Triggers: Downstream `#include <libbsa/libbsa.hpp>`.
- Responsibilities: Include reader, writer, validation, version, and result public surfaces.

**Archive open:**
- Location: `src/archive.cpp:183`
- Triggers: `archive_reader::open`.
- Responsibilities: Resolve host path, detect archive type/version, parse entries, construct immutable reader state.

**Archive extraction:**
- Location: `src/archive.cpp:338`, `src/archive.cpp:389`
- Triggers: `archive_reader::extract`, `archive_reader::extract_bytes`, `archive_reader::extract_entries`.
- Responsibilities: Normalize lookup paths, stream/decompress payloads into caller sinks, optionally parallelize independent groups.

**Validation:**
- Location: `src/validation.cpp:203`
- Triggers: `validate_archive`.
- Responsibilities: Open archive, report fatal diagnostics/warnings, optionally prove extractability through streaming.

**TES3 writer:**
- Location: `src/formats/bsa/tes3_bsa_writer.cpp:74`
- Triggers: `tes3_bsa_writer::write_to`.
- Responsibilities: Validate raw/uncompressed entries, prepare offsets, serialize Morrowind BSA, publish safely.

**TES4 writer:**
- Location: `src/formats/bsa/tes4_bsa_writer.cpp:83`
- Triggers: `tes4_bsa_writer::write_to`.
- Responsibilities: Select target version/compression/embedded-name behavior, prepare folders, assign offsets, serialize, publish safely.

**BA2 GNRL writer:**
- Location: `src/formats/ba2/ba2_gnrl_writer.cpp:101`
- Triggers: `ba2_gnrl_writer::write_to`.
- Responsibilities: Validate Starfield/Fallout options, prepare payloads, assign payload offsets/file table, serialize, publish safely.

**BA2 DX10 writer:**
- Location: `src/formats/ba2/ba2_dx10_writer.cpp:101`
- Triggers: `ba2_dx10_writer::write_to`.
- Responsibilities: Consume snapshot-backed DDS entries, prepare chunks, assign offsets, serialize, publish safely, cleanup snapshots.

**Tests and fixtures:**
- Location: `tests/CMakeLists.txt:65`, `tests/CMakeLists.txt:140`, `tests/CMakeLists.txt:302`
- Triggers: CMake test target builds and CTest discovery.
- Responsibilities: Build `libbsa_tests`, generate legal fixture archives, register Catch2 test cases and package-consumer checks.

## Architectural Constraints

- **Threading:** Default reader/writer calls are serial. Bulk extraction and writer preparation can use caller-selected positive `worker_count`; scheduling is via `detail::run_indexed_work` in `src/detail/parallel_work.cpp`. Sink factories and sinks are caller-owned and must manage their own shared state.
- **Global state:** No global mutable state is used for archive reading, writing, validation, or benchmark report generation (`docs/thread-safety.md:3`). Prefer local state, immutable opened reader state, and writer-owned pimpl state.
- **Public dependencies:** Public headers must not expose libdeflate, LZ4, DirectXTex, Windows handles, or DirectX/DXGI types. Third-party libraries are linked privately in `CMakeLists.txt:101` and declared as install dependencies in `cmake/libbsaConfig.cmake.in:4`.
- **Archive paths vs host paths:** Archive-internal paths are normalized strings through `src/detail/archive_path.cpp`; host paths are resolved once with `src/detail/host_file_path.*`. Do not use `std::filesystem::path` as an archive key.
- **Windows-only platform:** Windows/MSVC/vcpkg is the supported platform contract (`README.md:5`). Do not add portability layers or POSIX behavior unless project scope changes.
- **TES5Edit boundary:** `TES5Edit/` is read-only reference material (`README.md:34`, `AGENTS.md`). Do not edit, format, stage, compile, or use it as a fixture workspace.
- **Safe publication:** Writer finalization must publish via `src/detail/writer_publish.*`; do not write directly to requested output paths from format serializers.
- **Bounded memory:** Extraction should stream whenever possible (`src/detail/payload_stream.hpp`); convenience APIs like `extract_bytes` must keep allocation bounded by parser-derived sizes (`src/archive.cpp:373`).
- **Format routing:** Use explicit archive family/version/subtype/compression metadata. Do not infer codec behavior solely from file extension; BA2 DX10 explicitly routes chunks from parsed metadata (`src/formats/ba2/ba2_dx10_reader.cpp:149`).
- **Circular imports:** No circular include chain is intentionally modeled; format files include shared detail/texture helpers, but public headers do not include internal headers.

## Anti-Patterns

### Exposing Internal Dependency Types in Public Headers

**What happens:** A new API returns or accepts DirectXTex, libdeflate, LZ4, Windows handle, or parser-record types from `include/libbsa/`.
**Why it's wrong:** Public headers are intentionally dependency-light and stable; implementation details stay private behind adapters such as `src/texture/directxtex_analyzer.hpp` and `src/detail/compression_router.hpp`.
**Do this instead:** Translate implementation details into libbsa-owned value types in `include/libbsa/archive.hpp` or keep helpers internal under `src/`.

### Mixing Host Filesystem Paths with Archive Virtual Paths

**What happens:** Code uses `std::filesystem::path` normalization or platform separators for archive entry lookup/hash keys.
**Why it's wrong:** Bethesda archive paths are virtual keys with lowercase `/` canonicalization and no drive/root semantics; host path rules can corrupt archive lookups and hashes.
**Do this instead:** Use `detail::normalize_archive_path` from `src/detail/archive_path.cpp` for archive paths and `detail::resolve_host_file_path` from `src/detail/host_file_path.cpp` for host paths.

### Bypassing the Writer Publication Helper

**What happens:** A format serializer writes directly to the final caller path or handles overwrite policy itself.
**Why it's wrong:** The shared helper centralizes no-overwrite races, reparse-point rejection, atomic replacement, temp-directory cleanup, and consistent diagnostics.
**Do this instead:** Serialize inside the callback passed to `detail::publish_writer_output` as shown in `src/formats/bsa/tes4_bsa_writer.cpp:158`.

### Placing Format-Specific Rules in Public Classes

**What happens:** Public writer methods accumulate TES4/BA2 layout, compression, table, or hash logic.
**Why it's wrong:** Public classes should stage entries and dispatch; format rules belong in `*_prepare`, `*_layout`, and `*_serialize` modules for testable seams.
**Do this instead:** Add validation/materialization in `src/formats/<family>/*_prepare.*`, offset/dedupe rules in `*_layout.*`, and byte emission in `*_serialize.*`.

### Reading or Mutating `TES5Edit/` as Working Source

**What happens:** Implementation edits files under `TES5Edit/`, compiles that submodule into libbsa, or writes tests/generated fixtures there.
**Why it's wrong:** `TES5Edit/` is a read-only behavioral reference boundary and must not be modified or compiled into this project.
**Do this instead:** Trace behavior from `TES5Edit/` when needed, then implement clean C++ under `src/` and legal fixtures under `tests/fixtures/generated/`.

## Error Handling

**Strategy:** Expected archive, I/O, validation, compression, and caller-data failures return `libbsa::result<T>` with stable `error_code` plus diagnostic text. Exceptions are converted at allocation/thread boundaries or reserved for programmer misuse of result accessors.

**Patterns:**
- Return `error{error_code::invalid_argument, ...}` for invalid caller input such as empty host paths or `worker_count == 0` (`src/archive.cpp:185`, `src/archive.cpp:398`).
- Return `error_code::unsupported` for unsupported archive families/versions/subtypes (`src/formats/bsa/bsa_format_detector.cpp:37`, `src/formats/ba2/ba2_format_detector.cpp:55`).
- Return `error_code::format_error` for malformed supported archive bytes, truncated tables, inconsistent sizes, duplicate paths, codec size mismatches, and extractability failures.
- Convert parser allocation and length exceptions to format errors using helpers in `src/detail/parser_primitives.hpp` and `src/detail/byte_vector.hpp`.
- Validation keeps unreadable path/setup failures at result level and readable malformed archive bytes inside `validation_report::errors` (`src/validation.cpp:217`).
- Writer publication prefixes filesystem errors with writer-specific context in `src/detail/writer_publish.cpp`.

## Cross-Cutting Concerns

**Logging:** No logging framework is used. Public APIs return structured errors/warnings; callers own logging and display. Do not add `spdlog`, `fmt`, or global loggers for library internals.

**Validation:** Parsing validates archive structure before public state is exposed. `validate_archive` reuses public reader APIs and can stream all entries to a discard sink without retaining payload bytes (`src/validation.cpp:161`).

**Authentication:** Not applicable; this is a local archive library with no network/authentication layer.

**Compression:** Use `src/detail/compression_router.*` as the only cross-format compression dispatch. Deflate routes to libdeflate, Skyrim SE BSA LZ4 routes to LZ4 frame helpers, and Starfield BA2 v3 method `3` routes to raw LZ4 block helpers.

**Texture/DDS:** Use `src/texture/` for DirectXTex-backed analysis, DDS header reconstruction, mip sizing, chunk planning, and chunk ordering. Public metadata remains numeric and dependency-light.

**File I/O:** Use `src/detail/host_file.*` for host archive reads and `src/detail/writer_publish.*` for output publication. Read and write diagnostics should pass context strings that identify the archive family and operation.

**Security/Safety:** Treat archive bytes as untrusted. Preserve metadata count limits in `src/detail/parser_primitives.hpp`, exact decompression size checks in `src/detail/compression_router.cpp`, no-overwrite/reparse checks in `src/detail/writer_publish.cpp`, and local-only game fixture policy in `tests/fixtures/README.md`.

**OpenSpec/project workflow:** Project skills under `.claude/skills/openspec-*` describe OpenSpec change lifecycle commands. Architecture work should preserve generated project artifacts under `openspec/` and `.planning/`; implementation still belongs outside `TES5Edit/`.

---

*Architecture analysis: 2026-05-15*
