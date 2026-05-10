<!-- refreshed: 2026-05-10 -->
# Architecture

**Analysis Date:** 2026-05-10

## System Overview

```text
+----------------------------------------------------------------+
|                  Public C++20 Consumer API                     |
|  `include/libbsa/libbsa.hpp`                                   |
+-------------------+---------------------+----------------------+
| `archive_reader`  | writer classes      | `validate_archive`   |
| `include/libbsa/` | `include/libbsa/`   | `include/libbsa/`    |
+---------+---------+----------+----------+----------+-----------+
          |                    |                     |
          v                    v                     v
+----------------------------------------------------------------+
|                  API Implementations / Dispatch                 |
| `src/archive.cpp`, `src/validation.cpp`, writer `.cpp` files    |
+---------+--------------------+---------------------+-----------+
          |                    |                     |
          v                    v                     v
+----------------------------------------------------------------+
|                    Format Implementations                       |
| `src/formats/bsa/`              `src/formats/ba2/`              |
| TES3/TES4 detect, parse, read, write | BA2 GNRL/DX10 parse/write|
+---------+--------------------+---------------------+-----------+
          |                    |                     |
          v                    v                     v
+----------------------------------------------------------------+
|               Shared Internal Services / Texture Boundary       |
| `src/detail/`                  `src/texture/`                   |
| binary I/O, path keys, hashing, compression, workers, publishing|
+---------+--------------------+---------------------+-----------+
          |                    |                     |
          v                    v                     v
+----------------------------------------------------------------+
|              Host Archives, DDS Sources, Generated Fixtures     |
| `tests/fixtures/generated/`, caller host paths, output archives |
+----------------------------------------------------------------+
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| Public archive model | Defines archive families, variants, entry metadata, texture metadata, extraction sinks, bulk extraction contracts, and `archive_reader`. | `include/libbsa/archive.hpp` |
| Public writer model | Defines target profiles, compression policies, write execution options, and TES3/TES4/BA2 writer classes. | `include/libbsa/writer.hpp` |
| Public validation model | Defines validation options, structured reports, diagnostics, compatibility warnings, and `validate_archive`. | `include/libbsa/validation.hpp` |
| Public error contract | Provides the C++20 `result<T>` / `result<void>` and stable `error_code` categories. | `include/libbsa/result.hpp` |
| Reader dispatch | Detects archive bytes, opens host files, selects BSA vs BA2 and DX10 vs GNRL parser/reader paths, and stores immutable reader state. | `src/archive.cpp` |
| Validation dispatch | Reuses `archive_reader::open` as strict parser source of truth, adds public compatibility warnings, and optionally streams extraction into a discard sink. | `src/validation.cpp` |
| BSA format layer | Implements TES3 and TES4-family detection, parsing, listing, lookup, extraction, and write-new flows. | `src/formats/bsa/` |
| BA2 format layer | Implements BA2 GNRL and BA2 DX10 detection, parsing, listing, lookup, extraction, DDS reconstruction, write-new flows, and BA2 publish rollback helpers. | `src/formats/ba2/` |
| Shared detail services | Owns binary readers/writers, archive path normalization, Bethesda hashing, bounded byte-vector helpers, compression routing, codec adapters, payload transfer, atomic file operations, and worker scheduling. | `src/detail/` |
| Texture boundary | Owns DirectXTex usage, DDS metadata translation, BA2 DX10 chunk planning, DDS DXT10 header reconstruction, and chunk ordering validation. | `src/texture/` |
| Build/package target | Builds `libbsa`, links private dependencies, declares public header file set, installs CMake package metadata, and optionally adds tests/benchmarks/docs. | `CMakeLists.txt` |
| Test harness | Builds Catch2 unit/fixture tests, fixture generator tools, package-consumer smoke tests, and manifest validation. | `tests/CMakeLists.txt` |

## Pattern Overview

**Overall:** Layered reusable library with public facade/PIMPL APIs, format-specific private modules, and dependency adapters hidden behind libbsa-owned value types.

**Key Characteristics:**
- Public headers in `include/libbsa/` expose C++20 value types and `result<T>` but do not include libdeflate, lz4, DirectXTex, Catch2, nlohmann-json, or platform SDK headers.
- `archive_reader::open` in `src/archive.cpp:98` is the reader facade. It reads a prefix, classifies the container, dispatches to private parser functions, and stores parsed metadata plus host path in `archive_reader::state`.
- Writer public classes are facade objects with `std::shared_ptr<state>` members declared in `include/libbsa/writer.hpp` and concrete state structs in format writer implementations such as `src/formats/bsa/tes4_bsa_writer.cpp:28`.
- Format code is split by container family and operation: detectors (`*_format_detector.*`), parsers (`*_parser.*`), readers/extractors (`*_reader.*`), and writers (`*_writer.*`) live under `src/formats/bsa/` and `src/formats/ba2/`.
- Shared binary, path, hash, compression, streaming, and worker behavior belongs in `src/detail/`; texture metadata and DDS layout behavior belongs in `src/texture/`.
- Archive-controlled failures return `libbsa::error` through `result<T>`; programmer misuse of successful/error result access throws `std::logic_error` from `include/libbsa/result.hpp`.

## Layers

**Public API Layer:**
- Purpose: Stable consumer surface for opening, reading, extracting, validating, and writing archives.
- Location: `include/libbsa/`
- Contains: `archive_reader`, writer classes, metadata structs, validation reports, sink interfaces, target enums, policy options, and `result<T>`.
- Depends on: C++ standard library headers and `include/libbsa/result.hpp`.
- Used by: Consumers, `tests/package-consumer/main.cpp`, unit tests, benchmarks, and the implementation files under `src/`.

**API Implementation Layer:**
- Purpose: Connect public methods to the right private format implementation while preserving public error and state contracts.
- Location: `src/archive.cpp`, `src/validation.cpp`, `src/formats/**/**/*_writer.cpp`
- Contains: `archive_reader` state and dispatch, `validate_archive`, public writer constructors/add methods/write methods.
- Depends on: Public headers, format modules under `src/formats/`, and detail services under `src/detail/`.
- Used by: The `libbsa` target in `CMakeLists.txt`.

**Format Layer:**
- Purpose: Parse and serialize Bethesda archive byte formats without leaking raw format structures into public headers.
- Location: `src/formats/bsa/`, `src/formats/ba2/`
- Contains: BSA/BA2 format detectors, parser-local record structs, public `entry_metadata` materialization, extraction helpers, and writer finalization pipelines.
- Depends on: Public metadata/result types, `src/detail/` services, and `src/texture/` for BA2 DX10 only.
- Used by: `src/archive.cpp` reader dispatch and public writer method implementations.

**Shared Detail Layer:**
- Purpose: Reusable internal primitives that are format-neutral.
- Location: `src/detail/`
- Contains: `binary_reader`/`binary_writer`, `normalize_archive_path`, Bethesda hash functions, compression router/codecs, bounded byte-vector allocation helpers, payload streaming, atomic file publishing, and indexed worker scheduling.
- Depends on: Public `result<T>` plus external codec libraries in codec adapters only.
- Used by: BSA/BA2 parsers, readers, writers, texture layout code, and `src/archive.cpp`.

**Texture Boundary Layer:**
- Purpose: Keep DirectXTex and DDS/DXGI details out of public headers and format-neutral modules.
- Location: `src/texture/`
- Contains: `analyze_dds_metadata`, `analyze_dds_source`, DDS DXT10 header construction, mip-size calculation, BA2 DX10 chunk planning, and chunk-order validation.
- Depends on: Public texture metadata structs, `src/detail/binary_io.*`, and DirectXTex in `src/texture/directxtex_analyzer.cpp`.
- Used by: `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, and `src/formats/ba2/ba2_dx10_writer.cpp`.

**Validation/Test/Documentation Layer:**
- Purpose: Prove public behavior, fixture provenance, packaging, threading, and compatibility policies.
- Location: `tests/`, `docs/`, `benchmarks/`, `.github/workflows/ci.yml`
- Contains: Catch2 tests, fixture generators, generated legal fixtures, package-consumer smoke project, benchmark runner, Doxygen input, thread-safety docs, and CI gates.
- Depends on: Installed/public `libbsa::libbsa` target for package smoke, and private `src/` includes for internal unit tests.
- Used by: Maintainers and CI.

## Data Flow

### Primary Request Path

1. Caller opens a host archive with `archive_reader::open` (`src/archive.cpp:98`).
2. `archive_reader::open` reads a fixed detection prefix and routes `BTDX` bytes to BA2 detection (`src/archive.cpp:112`) or other bytes to BSA detection (`src/archive.cpp:143`).
3. BA2 dispatch selects DX10 parser (`src/archive.cpp:122`) or GNRL parser (`src/archive.cpp:133`); BSA dispatch selects TES3 parser (`src/archive.cpp:153`) or TES4-family parser (`src/archive.cpp:164`).
4. Format parsers validate table spans, names, payload offsets, compression metadata, duplicate canonical paths, and materialize public `entry_metadata` values. Examples: `src/formats/bsa/tes4_bsa_parser.cpp:483`, `src/formats/ba2/ba2_gnrl_parser.cpp:306`, `src/formats/ba2/ba2_dx10_parser.cpp:460`.
5. The reader stores `archive_metadata`, sorted public entries, host path, and BA2 DX10 flag in `archive_reader::state` (`src/archive.cpp:26`).
6. Listing, lookup, and contains calls dispatch to the matching format reader from `archive_reader::entries` (`src/archive.cpp:182`), `archive_reader::find` (`src/archive.cpp:195`), and `archive_reader::contains` (`src/archive.cpp:209`).
7. Extraction dispatch starts in `archive_reader::extract` (`src/archive.cpp:223`) and streams/decompresses through the matching extractor: TES3 raw (`src/formats/bsa/tes3_bsa_reader.cpp:97`), TES4-family raw/deflate/LZ4-frame (`src/formats/bsa/tes4_bsa_reader.cpp:250`), BA2 GNRL raw/deflate/LZ4-block (`src/formats/ba2/ba2_gnrl_reader.cpp:198`), or BA2 DX10 reconstructed DDS (`src/formats/ba2/ba2_dx10_reader.cpp:193`).
8. Compressed extraction routes through `detail::decompress_payload_exact` (`src/detail/compression_router.cpp:35`) and writes to caller-owned `payload_sink` implementations from `include/libbsa/archive.hpp:166`.

### Writer Finalization Path

1. Caller creates a public writer such as `tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, or `ba2_dx10_writer` from `include/libbsa/writer.hpp`.
2. `add_file` / `add_bytes` methods normalize archive virtual paths and store writer-owned entry state. Examples: `src/formats/bsa/tes4_bsa_writer.cpp:68`, `src/formats/ba2/ba2_gnrl_writer.cpp:71`, `src/formats/ba2/ba2_dx10_writer.cpp:166`.
3. `write_to` validates positive `write_execution_options::worker_count`, then delegates to format finalization: TES3 (`src/formats/bsa/tes3_bsa_writer.cpp:442`), TES4-family (`src/formats/bsa/tes4_bsa_writer.cpp:864`), BA2 GNRL (`src/formats/ba2/ba2_gnrl_writer.cpp:905`), or BA2 DX10 (`src/formats/ba2/ba2_dx10_writer.cpp:829`).
4. Format writers validate entries, prepare/compress payloads, sort deterministic records, assign offsets, and write archive bytes. Parallel-capable preparation uses `detail::run_indexed_work` from `src/detail/parallel_work.cpp:21`.
5. Writers publish through temporary files/directories and no-overwrite or overwrite-safe replacement paths. Shared primitives live in `src/detail/atomic_file_ops.hpp`, and BA2 rollback helper code lives in `src/formats/ba2/ba2_publish.hpp`.

### Validation Flow

1. Caller invokes `validate_archive` (`include/libbsa/validation.hpp:121`, `src/validation.cpp:178`).
2. Validation rejects empty/unreadable host paths as result-level setup failures in `src/validation.cpp:178`.
3. Readable archive bytes are parsed through `archive_reader::open` (`src/validation.cpp:186`) so validation uses the same strict reader as normal consumers.
4. Validation adds target-family and entry-level compatibility warnings in `src/validation.cpp`, including compressed sound payloads and BSA embedded-name risks.
5. Optional extractability validation streams each entry into a discard sink instead of retaining payload bytes (`src/validation.cpp:143`).

### BA2 DX10 Texture Flow

1. BA2 DX10 parser materializes texture metadata and parser-validated chunk metadata in `src/formats/ba2/ba2_dx10_parser.cpp`.
2. BA2 DX10 extraction builds a deterministic DDS DXT10 header from libbsa-owned layout values (`src/formats/ba2/ba2_dx10_reader.cpp:193`, `src/texture/dds_layout.cpp:235`).
3. BA2 DX10 extraction writes the header first, then chunk payloads in validated DDS order using parsed compression metadata only (`src/formats/ba2/ba2_dx10_reader.cpp:193`).
4. BA2 DX10 writer input is DDS host-file based. `ba2_dx10_writer::add_file` snapshots analyzed subresources into writer-owned temporary files (`src/formats/ba2/ba2_dx10_writer.cpp:166`, `src/texture/directxtex_analyzer.cpp:115`).

**State Management:**
- `archive_reader` stores immutable shared state in `std::shared_ptr<const state>` (`include/libbsa/archive.hpp:288`, `src/archive.cpp:26`).
- Public writer classes store mutable writer state in `std::shared_ptr<state>` (`include/libbsa/writer.hpp:221`, `include/libbsa/writer.hpp:272`, `include/libbsa/writer.hpp:337`, `include/libbsa/writer.hpp:386`).
- `write_execution_options` and `bulk_extract_options` are copied into calls; `worker_count == 0` is invalid in `src/archive.cpp:278`, `src/detail/parallel_work.cpp:24`, and writer implementations.
- BA2 DX10 writer state owns a temporary snapshot directory and removes it best-effort in its state destructor (`src/formats/ba2/ba2_dx10_writer.cpp:34`).

## Key Abstractions

**`result<T>` / `error`:**
- Purpose: Stable C++20 fallible return contract for public and private archive operations.
- Examples: `include/libbsa/result.hpp`, `src/validation.cpp`, `src/detail/byte_vector.hpp`
- Pattern: Return `error_code` plus human-readable message for I/O, unsupported, invalid argument, not found, and format failures.

**Archive Metadata Values:**
- Purpose: Public dependency-light archive, entry, BA2, and texture descriptions.
- Examples: `include/libbsa/archive.hpp:91`, `include/libbsa/archive.hpp:126`, `include/libbsa/archive.hpp:143`
- Pattern: Store normalized archive paths, original display paths, offsets, sizes, compression route, and optional texture chunk metadata as libbsa-owned values.

**Reader Facade:**
- Purpose: Stable API for open/list/find/contains/extract/bulk-extract across all supported archive families.
- Examples: `include/libbsa/archive.hpp:240`, `src/archive.cpp`
- Pattern: Public methods dispatch to private format modules using parsed `archive_metadata` and `archive_reader::state`.

**Writer Facades:**
- Purpose: Stable write-new APIs for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Examples: `include/libbsa/writer.hpp:176`, `include/libbsa/writer.hpp:233`, `include/libbsa/writer.hpp:284`, `include/libbsa/writer.hpp:350`
- Pattern: Public writers collect entries and options, then delegate deterministic finalization to private format writers.

**Archive Virtual Path Key:**
- Purpose: Keep archive paths separate from host filesystem paths.
- Examples: `src/detail/archive_path.hpp`, `src/detail/archive_path.cpp`
- Pattern: Normalize to lowercase forward-slash keys and reject rooted/traversal-like paths before lookup, hash, or serialization.

**Compression Router:**
- Purpose: Route explicit archive metadata compression to private codec adapters.
- Examples: `src/detail/compression_router.hpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`
- Pattern: Choose `none`, raw deflate, LZ4 frame, or raw LZ4 block from archive family/version/metadata; never infer codec behavior from file extension.

**Texture Layout Boundary:**
- Purpose: Represent DDS metadata, mip ranges, chunk ordering, and DDS header reconstruction without exposing DirectXTex.
- Examples: `src/texture/directxtex_analyzer.hpp`, `src/texture/dds_layout.hpp`
- Pattern: Translate third-party metadata into `texture_metadata`, then use libbsa-owned layout functions for extraction and writer chunk planning.

## Entry Points

**Library Build Target:**
- Location: `CMakeLists.txt:35`
- Triggers: `cmake --build --preset <preset>`
- Responsibilities: Builds `libbsa`, exposes `libbsa::libbsa`, links private dependencies, publishes public header file set, and installs package config files.

**Umbrella Public Include:**
- Location: `include/libbsa/libbsa.hpp`
- Triggers: Consumer `#include <libbsa/libbsa.hpp>`
- Responsibilities: Includes all public reader, writer, validation, version, and result headers.

**Archive Reader:**
- Location: `include/libbsa/archive.hpp:240`, `src/archive.cpp:98`
- Triggers: `libbsa::archive_reader::open(host_path)`
- Responsibilities: Open supported BSA/BA2 archives, expose metadata/listing/lookup, and extract entries.

**Archive Writers:**
- Location: `include/libbsa/writer.hpp`
- Triggers: Writer construction, `add_file`, `add_bytes`, and `write_to`
- Responsibilities: Create new TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives.

**Validation API:**
- Location: `include/libbsa/validation.hpp:121`, `src/validation.cpp:178`
- Triggers: `libbsa::validate_archive(host_path, options)`
- Responsibilities: Return structured validity, fatal diagnostics, metadata, and compatibility warnings.

**Tests and Fixtures:**
- Location: `tests/CMakeLists.txt:5`, `tests/fixtures/generated/`
- Triggers: `ctest --preset <preset>` and fixture generator targets.
- Responsibilities: Validate public APIs, internal helpers, generated fixture contracts, malformed inputs, package install smoke, and compatibility policies.

**Benchmark Report:**
- Location: `benchmarks/libbsa_benchmarks.cpp`, `CMakeLists.txt:119`
- Triggers: `cmake --build --target libbsa_benchmark_report`
- Responsibilities: Generate synthetic benchmark JSON/Markdown reports without using game archives or `TES5Edit/`.

## Architectural Constraints

- **Threading:** `docs/thread-safety.md` is the canonical policy. Independent readers/writers may be used concurrently; one reader may run concurrent const extraction only with distinct sinks. Parallel extraction and writer preparation are opt-in through positive worker counts and `src/detail/parallel_work.cpp`.
- **Global state:** No archive registry or singleton state exists. A function-local `static std::atomic_uint64_t` in `src/formats/ba2/ba2_dx10_writer.cpp:77` generates unique temporary snapshot directory names; a function-local immutable CRC table in `src/detail/bethesda_hash.cpp` supports hashing.
- **Circular imports:** Not detected among public/private layers. Public headers depend inward only on `include/libbsa/result.hpp` or sibling public headers; private implementation files include public headers, `src/detail/`, `src/formats/`, and `src/texture/`.
- **Reference boundary:** `TES5Edit/` is a git submodule recorded by `.gitmodules` and is read-only reference material only. Do not edit, format, compile, stage, or generate files under `TES5Edit/`.
- **Dependency boundary:** libdeflate, lz4, and DirectXTex are linked privately in `CMakeLists.txt:58` and appear only in private adapter/source files such as `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`, and `src/texture/directxtex_analyzer.cpp`.
- **Path boundary:** Host filesystem paths remain `std::string_view`/`std::filesystem::path` inputs at API and I/O boundaries; archive virtual paths are normalized through `src/detail/archive_path.*`.

## Anti-Patterns

### Format Logic in Public Headers

**What happens:** Not detected in current public headers. Adding parser record structs, raw archive flags, DirectXTex types, libdeflate/lz4 types, or platform headers to `include/libbsa/` would break the current dependency-light API boundary.
**Why it's wrong:** Consumers should not compile against private codec, texture, or format-layout details; the package target in `CMakeLists.txt` installs only stable public headers.
**Do this instead:** Add format-specific structs and helpers under `src/formats/bsa/`, `src/formats/ba2/`, `src/detail/`, or `src/texture/`, then expose only stable libbsa-owned values through `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, or `include/libbsa/validation.hpp`.

### Host Path Semantics for Archive Keys

**What happens:** Not detected in lookup/write paths. Treating archive virtual paths as host filesystem paths would introduce platform-specific separators, rooted paths, and traversal semantics into archive keys.
**Why it's wrong:** Bethesda archive keys are virtual, normalized lookup/hash keys; host path semantics can corrupt hashes, duplicate detection, extraction safety, and writer ordering.
**Do this instead:** Use `detail::normalize_archive_path` from `src/detail/archive_path.cpp` for archive-internal paths, and keep host paths only at file I/O boundaries in reader/writer APIs.

### Extension-Based Compression Guessing

**What happens:** Not detected in extraction paths. Inferring compression from file extension would bypass parsed archive metadata.
**Why it's wrong:** TES4-family BSA, BA2 GNRL, and BA2 DX10 select deflate, LZ4 frame, raw LZ4 block, or raw storage from version/flags/chunk metadata, not path suffixes.
**Do this instead:** Route compression through `entry_metadata::compression`, `texture_chunk_metadata::compression`, and `detail::compression_router` in `src/detail/compression_router.cpp`.

## Error Handling

**Strategy:** Public and private fallible operations return `result<T>` or `result<void>` with stable `error_code` values. Exceptions are reserved for programmer mistakes in `result<T>` access and are translated where archive-controlled sizes could otherwise throw allocation exceptions.

**Patterns:**
- Use `error_code::invalid_argument` for empty paths, invalid archive virtual paths, and invalid worker counts, as in `src/archive.cpp:98`, `src/detail/archive_path.cpp`, and `src/detail/parallel_work.cpp:21`.
- Use `error_code::unsupported` for unsupported magic, subtype, version, or compression method detection in `src/formats/bsa/bsa_format_detector.cpp` and `src/formats/ba2/ba2_format_detector.cpp`.
- Use `error_code::format_error` for malformed supported archive bytes, truncated tables, bad payload spans, decompression size mismatch, duplicate canonical paths, and DDS layout errors in format and texture modules.
- Use `error_code::io_error` for host file open/read/write/publish failures in `src/archive.cpp`, format readers/writers, and `src/detail/atomic_file_ops.hpp`.
- Validation reports archive diagnostics inside `validation_report::errors` while reserving result-level failures for setup problems in `src/validation.cpp:178`.

## Cross-Cutting Concerns

**Logging:** No logging framework is present. Public APIs return structured errors and validation reports; consumers decide how to log messages from `include/libbsa/result.hpp` and `include/libbsa/validation.hpp`.
**Validation:** Parser validation is fail-closed in `src/formats/**/**/*_parser.cpp`; public validation is centralized in `src/validation.cpp`; fixture policy and matrix validation live in `tests/fixtures/README.md` and `tests/fixtures/generated/validate_fixture_manifests.py`.
**Authentication:** Not applicable. libbsa is a local archive library with no network or identity surface.
**Compatibility Evidence:** `docs/compatibility-evidence.md`, `docs/target-format-guide.md`, generated fixtures under `tests/fixtures/generated/`, and policy tests under `tests/unit/` encode compatibility behavior.
**Documentation:** Public API documentation is driven by Doxygen comments in `include/libbsa/` plus docs files under `docs/`; `CMakeLists.txt:16` makes Doxygen optional.

---

*Architecture analysis: 2026-05-10*
