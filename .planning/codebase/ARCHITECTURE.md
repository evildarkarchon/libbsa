<!-- refreshed: 2026-05-12 -->
# Architecture

**Analysis Date:** 2026-05-12

## System Overview

```text
┌──────────────────────────────────────────────────────────────────────┐
│                         Public API Surface                          │
├──────────────────────┬──────────────────────┬───────────────────────┤
│ archive reader API   │ writer APIs          │ validation API        │
│ `include/libbsa/`    │ `include/libbsa/`    │ `include/libbsa/`     │
└──────────────┬───────┴──────────────┬───────┴──────────────┬────────┘
               │                      │                      │
               ▼                      ▼                      ▼
┌──────────────────────────────────────────────────────────────────────┐
│                    Orchestration / Public Entry Layer               │
│ `src/archive.cpp` `src/validation.cpp` `src/formats/*/*_writer.cpp` │
└──────────────────────────────┬───────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────────┐
│                 Format Pipelines and Shared Detail Services         │
├───────────────────────────────┬──────────────────────────────────────┤
│ Format-specific BSA / BA2     │ Shared detail / texture helpers     │
│ `src/formats/bsa/`            │ `src/detail/` `src/texture/`        │
│ `src/formats/ba2/`            │                                      │
└───────────────────────────────┴──────────────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────────────┐
│                Host filesystem + private Windows libraries          │
│ `std::ifstream` / `std::ofstream` / `std::filesystem`               │
│ `libdeflate` `lz4` `DirectXTex` `bcrypt` `windows.h`                │
└──────────────────────────────────────────────────────────────────────┘
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| Public archive API | Defines stable reader types, metadata, sink interfaces, and extraction contracts | `include/libbsa/archive.hpp` |
| Public writer API | Defines stable write-new entry points and target/profile options | `include/libbsa/writer.hpp` |
| Archive dispatcher | Detects archive family, opens parser path, stores reader state, dispatches extraction | `src/archive.cpp` |
| Validation facade | Reuses `archive_reader` to build structured diagnostics and warnings | `src/validation.cpp` |
| BSA format pipeline | Implements TES3 and TES4-family detect/parse/read/write flows | `src/formats/bsa/` |
| BA2 format pipeline | Implements GNRL and DX10 detect/parse/read/write flows | `src/formats/ba2/` |
| Shared internals | Centralizes path normalization, hashes, binary IO, codecs, payload streaming, publish safety, and worker scheduling | `src/detail/` |
| Texture adapter | Keeps DirectXTex and DDS chunk/header logic private to implementation | `src/texture/` |

## Pattern Overview

**Overall:** Layered library with format-family pipelines and thin public facades.

**Key Characteristics:**
- Public headers in `include/libbsa/` expose stable value types and opaque writer state while keeping codec and DirectXTex details private.
- `src/archive.cpp` and `src/validation.cpp` act as orchestration facades that branch by detected archive family and reuse format-specific modules instead of duplicating parsing logic.
- Every write-new flow follows the same staged pipeline: validate → prepare → assign layout/offsets → serialize to temp output → publish atomically.

## Layers

**Public API layer:**
- Purpose: Provide the reusable C++20 surface consumers link against.
- Location: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`, `include/libbsa/version.hpp`
- Contains: Enums, metadata structs, result/error types, sink interfaces, writer classes.
- Depends on: Standard library plus export/result headers only.
- Used by: Downstream consumers, `benchmarks/libbsa_benchmarks.cpp`, `tests/package-consumer/main.cpp`.

**Facade/orchestration layer:**
- Purpose: Turn public API calls into concrete format operations.
- Location: `src/archive.cpp`, `src/validation.cpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Contains: Reader open/find/extract dispatch, validation report construction, writer state ownership, top-level write pipelines.
- Depends on: Format modules under `src/formats/`, shared helpers under `src/detail/`, and `src/texture/` for DDS-aware write flows.
- Used by: Public headers via linked library symbols.

**Format pipeline layer:**
- Purpose: Encode Bethesda-format-specific rules without leaking them into the public API.
- Location: `src/formats/bsa/`, `src/formats/ba2/`
- Contains: Detectors, parsers, reader helpers, prepare/layout/serialize stages, constants, and internal writer entry structs.
- Depends on: `src/detail/` for binary IO, archive path normalization, hashes, compression routing, source reading, and parallel work.
- Used by: `src/archive.cpp`, `src/validation.cpp`, and the top-level writer `.cpp` files.

**Shared detail services layer:**
- Purpose: Centralize implementation-only services reused across archive families.
- Location: `src/detail/`
- Contains: `archive_path`, `bethesda_hash`, `binary_io`, `parser_primitives`, `payload_stream`, `compression_router`, codecs, `writer_disk_source`, `writer_publish`, `parallel_work`, `atomic_file_ops`.
- Depends on: Standard library, Windows APIs where needed, and private third-party libraries.
- Used by: All format pipelines.

**Texture analysis layer:**
- Purpose: Isolate DDS metadata parsing, BA2 DX10 layout planning, and reconstructed DDS header logic.
- Location: `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.cpp`
- Contains: DirectXTex-backed metadata translation and libbsa-native DDS layout planning.
- Depends on: `DirectXTex` and Windows-only headers.
- Used by: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/bsa/tes4_bsa_prepare.cpp`.

## Data Flow

### Primary Request Path

1. Public open call enters `archive_reader::open`, validates the host path, reads a detection prefix, and chooses BSA or BA2 dispatch (`src/archive.cpp:121`).
2. The selected parser materializes `archive_metadata` plus sorted `entry_metadata` vectors, such as `parse_tes4_bsa_archive_file` (`src/formats/bsa/tes4_bsa_parser.cpp:610`) or `parse_ba2_dx10_archive_file` (`src/formats/ba2/ba2_dx10_parser.cpp:610`).
3. Later lookup and extraction calls use the cached state and route payload decoding through format readers and shared codecs (`src/archive.cpp:246`, `src/formats/bsa/tes4_bsa_reader.cpp:106`, `src/formats/ba2/ba2_dx10_reader.cpp:84`, `src/detail/compression_router.cpp:63`).

### Writer Finalization Flow

1. Public writer objects accumulate staged entries in writer-owned state through `add_file` / `add_bytes`, then call a format-specific top-level write function such as `write_tes4_bsa_archive` (`src/formats/bsa/tes4_bsa_writer.cpp:91`) or `write_ba2_dx10_archive` (`src/formats/ba2/ba2_dx10_writer.cpp:90`).
2. The top-level writer validates input and runs format staging: prepare (`src/formats/bsa/tes4_bsa_prepare.cpp:467`, `src/formats/ba2/ba2_dx10_prepare.cpp:538`), layout (`src/formats/bsa/tes4_bsa_layout.cpp:162`, `src/formats/ba2/ba2_dx10_layout.cpp:48`), then serialization (`src/formats/bsa/tes4_bsa_serialize.cpp:110`, `src/formats/ba2/ba2_dx10_serialize.cpp:95`).
3. The finished bytes are written into a writer-owned temp path and published via the shared publish helper instead of writing directly to the final destination (`src/formats/bsa/tes4_bsa_writer.cpp:135`, `src/formats/ba2/ba2_gnrl_writer.cpp:143`, `src/detail/writer_publish.cpp:60`).

### Validation Flow

1. `validate_archive` checks that the host path is readable and then opens the archive through the same public reader used by consumers (`src/validation.cpp:178`).
2. It collects metadata and entry listings from the opened reader, adds compatibility warnings derived from public metadata, and optionally extracts every entry through a discard sink (`src/validation.cpp:205`, `src/validation.cpp:213`, `src/validation.cpp:215`).
3. Validation returns a structured `validation_report` instead of exposing parser offsets or internal record coordinates (`src/validation.cpp:61`, `include/libbsa/validation.hpp`).

**State Management:**
- `archive_reader` keeps immutable opened state in `std::shared_ptr<archive_reader::state>` so copies share the same parsed metadata and entry vector (`src/archive.cpp:28`).
- Writers keep mutable staged state in private `state` structs behind `std::unique_ptr`, one per writer object (`src/formats/bsa/tes4_bsa_writer.cpp:21`, `src/formats/ba2/ba2_dx10_writer.cpp:20`).
- No global mutable registry or singleton owns archive state; thread-safety rules stay object-scoped and are documented in `docs/thread-safety.md`.

## Key Abstractions

**Canonical archive path:**
- Purpose: Separate archive-internal virtual paths from host filesystem paths.
- Examples: `src/detail/archive_path.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`
- Pattern: Normalize once with `detail::normalize_archive_path`, then store and binary-search canonical lowercase `/` keys.

**Format-specific staged entry pipeline:**
- Purpose: Convert public writer requests into fully prepared archive records before serialization.
- Examples: `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/bsa/tes4_bsa_serialize.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`
- Pattern: Each format owns its own internal `*_writer_entry`, `*_prepared_entry`, and layout structs.

**Payload sink streaming contract:**
- Purpose: Keep extraction synchronous and bounded without forcing the library to own destination storage.
- Examples: `include/libbsa/archive.hpp`, `src/archive.cpp`, `src/detail/payload_stream.cpp`
- Pattern: Reader code streams or decodes into a caller-owned `payload_sink`; `extract_bytes` is the opt-in convenience API that materializes one entry in memory.

**Publish helper:**
- Purpose: Standardize temp-file creation, overwrite policy, and safe publication across all writer families.
- Examples: `src/detail/writer_publish.cpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Pattern: Writers serialize to a temporary sibling path, then atomically publish or replace.

## Entry Points

**Aggregate header include:**
- Location: `include/libbsa/libbsa.hpp`
- Triggers: Consumer includes the full library surface.
- Responsibilities: Re-export `archive`, `writer`, `validation`, `result`, and `version` APIs.

**Archive open/read API:**
- Location: `include/libbsa/archive.hpp`, implementation in `src/archive.cpp`
- Triggers: Consumer calls `archive_reader::open`, `entries`, `find`, `extract`, `extract_bytes`, or `extract_entries`.
- Responsibilities: Detect archive type, cache metadata, normalize paths, and dispatch extraction.

**Validation API:**
- Location: `include/libbsa/validation.hpp`, implementation in `src/validation.cpp`
- Triggers: Consumer calls `validate_archive`.
- Responsibilities: Reuse reader pipeline, return structured diagnostics, and emit compatibility warnings.

**Write-new APIs:**
- Location: `include/libbsa/writer.hpp`, implementations in `src/formats/bsa/*_writer.cpp` and `src/formats/ba2/*_writer.cpp`
- Triggers: Consumer stages entries and calls `write_to`.
- Responsibilities: Own staged source state, select target-compatible pipeline, and publish archive bytes safely.

**Maintainer benchmark tool:**
- Location: `benchmarks/libbsa_benchmarks.cpp`
- Triggers: `libbsa_benchmark_report` target from `CMakeLists.txt`.
- Responsibilities: Exercise public writer/reader APIs and emit benchmark JSON/Markdown reports.

## Architectural Constraints

- **Threading:** Shared worker scheduling is explicit and opt-in through `worker_count`; the common runner is `detail::run_indexed_work` in `src/detail/parallel_work.cpp`, and default behavior stays serial.
- **Global state:** No library-owned global mutable state is present in `src/archive.cpp`, `src/validation.cpp`, `src/detail/parallel_work.cpp`, or the writer implementations.
- **Circular imports:** Not detected; the codebase uses headers within `src/detail/`, `src/formats/`, and `src/texture/` without obvious cyclic include chains.
- **Windows-only internals:** `src/detail/writer_publish.cpp` and `src/formats/ba2/ba2_dx10_prepare.cpp` include Windows headers directly; platform assumptions are implementation-level, not optional.
- **Read-only reference boundary:** `TES5Edit/` is documentation/reference input only and is excluded from implementation flow according to `README.md`, `AGENTS.md`, and `docs/target-format-guide.md`.

## Anti-Patterns

### Repeating variant dispatch in the public facade

**What happens:** `src/archive.cpp` branches on `archive_type`, `archive_variant`, and `is_ba2_dx10` in multiple methods (`open`, `entries`, `find`, `contains`, `extract`).
**Why it's wrong:** Adding a new archive family requires touching several public-facade branches instead of one backend registration point.
**Do this instead:** Extend the existing format modules first and keep dispatch additions localized to `src/archive.cpp` plus the new format pipeline, rather than scattering format conditionals into unrelated code.

### Growing format parsers into monoliths

**What happens:** The most complex archive grammars live in very large single translation units such as `src/formats/bsa/tes4_bsa_parser.cpp` and `src/formats/ba2/ba2_dx10_parser.cpp`.
**Why it's wrong:** These files own header parsing, table parsing, invariant checks, path canonicalization, and payload-span validation in one place, which raises review cost and makes new rule changes easy to miss.
**Do this instead:** Keep new format-specific helpers adjacent to the owning parser file or split focused helper modules under the same format directory rather than pushing more unrelated logic into the existing parser monoliths.

## Error Handling

**Strategy:** Fallible operations return `libbsa::result<T>` with stable `error_code` values instead of throwing for archive, IO, or codec failures.

**Patterns:**
- Public facade methods validate obvious preconditions early and return `invalid_argument`, `io_error`, `unsupported`, `format_error`, or `not_found` as appropriate (`src/archive.cpp`, `src/validation.cpp`).
- Format readers and writers preserve format-specific context in the message string while translating failures into shared public error codes (`src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/detail/writer_publish.cpp`).

## Cross-Cutting Concerns

**Logging:** Not detected in the library implementation; diagnostics flow through `result<T>` and `validation_report` rather than an internal logger.
**Validation:** Structural validation is split between parser invariants in `src/formats/` and public report generation in `src/validation.cpp`.
**Authentication:** Not applicable; this is a local archive library with no identity subsystem.

---

*Architecture analysis: 2026-05-12*
