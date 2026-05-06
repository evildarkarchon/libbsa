# Phase 06: ba2-gnrl-read-and-extract - Context

**Gathered:** 2026-05-05
**Status:** Ready for planning

<domain>
## Phase Boundary

This phase delivers BA2-specific open, inspect, list, lookup, and single-entry extraction support for Fallout 4 and Starfield `BTDX` + `GNRL` archives. It covers FO4 GNRL v1/v7/v8, Starfield GNRL v2/v3, length-prefixed file name tables at `FileTableOffset`, raw/deflate/Starfield v3 raw LZ4-block payload extraction, and generated fixture coverage for the required version and compression matrix. It does not cover BA2 `DX10` texture reconstruction, BA2 writers, bulk extraction orchestration, safe disk extraction policy, or Phase 11 reference-corpus validation.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `06-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `06-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public BA2-specific read/extract API for BA2 archives.
- `BTDX` + `GNRL` parsing for Fallout 4 versions 1, 7, and 8.
- `BTDX` + `GNRL` parsing for Starfield versions 2 and 3.
- BA2 GNRL file name table parsing from `FileTableOffset`.
- Normalized path listing, lookup, and copied metadata inspection for BA2 GNRL entries.
- Raw, deflate, and Starfield v3 raw LZ4-block extraction through caller-owned sinks.
- Generated fixture coverage for supported BA2 GNRL versions, compression routes, name tables, metadata, and malformed inputs.
- `.dds` or other extension payloads stored inside GNRL archives as ordinary files.

**Out of scope (from SPEC.md):**
- BA2 `DX10` texture archive parsing and DDS header reconstruction - this is Phase 7.
- Rejecting GNRL entries by file extension - GNRL archives may legitimately contain `.dds` payloads such as LUTs.
- BA2 writers or archive creation - writer behavior belongs to Phase 10 after read paths are proven.
- Unified family-neutral archive API replacing BSA-specific and BA2-specific APIs - this phase locks BA2 behavior without redesigning existing BSA entry points.
- Safe disk extraction policies such as traversal prevention, overwrite handling, and partial directory cleanup - these are application/tooling concerns or later diagnostics work.
- Bulk extraction orchestration, multi-threaded extraction, and performance benchmarking - these belong to Phase 12 after correctness paths are established.
- BSArchPro corpus comparison as a phase gate - generated fixtures are sufficient for Phase 6; broader reference corpus comparison is Phase 11 validation work.
- Modifying, formatting, staging, or compiling any file under `TES5Edit/` - the submodule remains a read-only behavioral reference.

</spec_lock>

<decisions>
## Implementation Decisions

### BA2 API Shape
- **D-01:** Add a separate public BA2 header and type surface mirroring the BSA reader API: `include/libbsa/ba2.hpp`, `ba2_archive`, `open_ba2`, and `extract_ba2_entry`. Do not put BA2 APIs into `bsa.hpp` and do not introduce a family-neutral archive API in Phase 6.
- **D-02:** Keep `ba2_archive` parallel to `bsa_archive`: expose `summary()`, `paths()`, `contains()`, and `entry()` and rely on `archive_summary` and `entry_metadata` for BA2-specific data.
- **D-03:** Public smoke coverage must include `ba2.hpp`, name `ba2_archive`, and take addresses of `open_ba2` and `extract_ba2_entry` from consumer-style code.
- **D-04:** `ba2_archive` is metadata-only. It owns copied metadata and does not retain or own a `byte_source`; callers pass a `byte_source` again to extraction, matching the existing BSA lifetime model.

### Metadata Exposure
- **D-05:** Use existing `archive_summary` fields for BA2 archive-level metadata: format, version, subtype, file count, file table offset, and Starfield v3 compression method. Do not add public fields for unknown or reserved header words unless research proves they are semantically required.
- **D-06:** Map BA2 hash components into existing `entry_metadata::name_hash` and `entry_metadata::directory_hash` where representable. The exact mapping must be locked with tests or comments near the parser so downstream agents can trace compatibility behavior.
- **D-07:** Preserve all three public size semantics for BA2 entries: `size` is unpacked output bytes, `packed_size` is compressed payload bytes or raw payload size, and `stored_size` is the on-disk byte range extraction validates and reads.
- **D-08:** Expose `entry_metadata::offset` as an archive-absolute payload offset after validation, continuing the Phase 5 TES3 pattern. Do not expose BA2 record-relative offsets directly to consumers.

### Compression Semantics
- **D-09:** Populate `entry_metadata::compression` with the resolved per-entry state (`raw`, `deflate`, or `lz4_block`) so metadata inspection matches extraction behavior.
- **D-10:** For Starfield v3 GNRL, route to raw LZ4-block extraction only when the entry is compressed and archive `compression_method == 3`. Raw entries remain raw even in method-3 archives.
- **D-11:** BA2 record size fields drive decompression framing. Do not require or auto-detect a BSA-style embedded uncompressed-size prefix for BA2 compressed payloads.
- **D-12:** Unsupported or inconsistent BA2 compression metadata must fail with structured `unsupported_format` or `malformed_archive` errors. Do not try fallback codecs and do not write partial bytes.

### Fixture Proof Shape
- **D-13:** Use generated deterministic BA2 fixture builders as the primary Phase 6 acceptance corpus. Do not depend on external real archives, BSArchPro execution, or committed binary BA2 blobs unless implementation discovers a concrete need.
- **D-14:** The generated fixture matrix must cover FO4 GNRL v1/v7/v8, Starfield GNRL v2, Starfield GNRL v3 raw/deflate/LZ4-block routes, file-table name parsing, and a `.dds`-named payload stored in GNRL as ordinary bytes.
- **D-15:** Keep fixture builders test-local first, likely in `tests/ba2_reader_tests.cpp` or a small test helper. Promote to shared helpers later only if writer phases reuse them.
- **D-16:** Explicit malformed coverage in Phase 6 must include core parser failures: truncated headers/records, impossible payload offsets, truncated name tables, mismatched file count/name count, and codec route confusion. Comprehensive fuzz-style hardening remains Phase 11 scope.

### the agent's Discretion
No selected area was left to the agent's discretion. The planner may choose exact helper names, parser file names, and test organization details as long as the decisions above, `06-SPEC.md`, and existing project patterns are satisfied.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Locked Phase Scope
- `.planning/phases/06-ba2-gnrl-read-and-extract/06-SPEC.md` - Locked Phase 6 requirements, boundaries, constraints, acceptance criteria, and interview decisions.
- `.planning/ROADMAP.md` - Phase ordering, Phase 6 goal, dependencies, success criteria, and requirement mapping.
- `.planning/REQUIREMENTS.md` - v1 requirement IDs and traceability, especially `BA2-01` through `BA2-04`, `CMP-03`, `CMP-04`, `BIO-01` through `BIO-05`, and `DPH-02` through `DPH-04`.
- `.planning/PROJECT.md` - Project purpose, core value, public API constraints, streaming-first requirement, and TES5Edit reference context.

### Prior Phase Decisions
- `.planning/phases/01-build-error-and-test-foundation/01-CONTEXT.md` - Carry-forward decisions for C++20 `libbsa::result`, public/private layout, explicit CMake sources, test labels, and TES5Edit boundary.
- `.planning/phases/02-streaming-api-archive-model-detection-and-hashes/02-CONTEXT.md` - Carry-forward decisions for caller-owned source/sink lifetimes, copied archive views, archive path normalization, detection summaries, and hash surface policy.

### Product and Reference Context
- `docs/PRD.md` - Original BA2 GNRL deliverables, including versions 1/2/3/7/8, FO4 CRC32 hash, `PackedSize != 0` compression detection, Starfield v3 `CompressionMethod == 3`, and file name table parsing.
- `AGENTS.md` - Repository instructions, TES5Edit read-only boundary, dependency rules, comment policy, and validation expectations.
- `.planning/research/STACK.md` - Dependency and architecture guidance for libdeflate, LZ4 frame/block separation, DirectXTex boundary, CMake/vcpkg, and public API dependency leakage.

### Existing Code To Inspect
- `include/libbsa/bsa.hpp` - Established public read/extract API shape that BA2 should mirror.
- `include/libbsa/archive.hpp` - Existing `archive_summary`, `entry_metadata`, archive formats, and compression-state fields BA2 should reuse.
- `include/libbsa/archive_view.hpp` - Metadata-only copied lookup view reused by `bsa_archive` and suitable for `ba2_archive`.
- `include/libbsa/compression.hpp` - Public compression routing contract, including Starfield BA2 `compression_method` behavior.
- `src/detect.cpp` - Existing BA2 header detection for FO4/Starfield GNRL/DX10 variants and summary field population.
- `src/compression.cpp` - Existing deflate vs LZ4-frame vs raw LZ4-block route separation.
- `src/bsa_reader.cpp` - Current BSA parser/extractor patterns for bounded reads, metadata-only archive object, range validation, and extraction through `byte_sink`.
- `tests/bsa_reader_tests.cpp` - Fixture-builder and parser/extraction test style to mirror for BA2 GNRL tests.
- `tests/detection_tests.cpp` - Existing BA2 detection fixtures and error expectations.
- `tests/compression_policy_tests.cpp` - Existing route-confusion coverage for Starfield BA2 compression method behavior.
- `tests/public_header_smoke.cpp` - Consumer-style public header smoke test to extend with BA2 API names.
- `CMakeLists.txt` - Explicit source/header/test wiring; new BA2 source/header/test files must be listed explicitly and must not include `TES5Edit/`.

### Read-Only Reference Areas
- `TES5Edit/BSArchPro.dpr` - Behavioral reference entry point for BSArchPro compatibility tracing; read-only.
- `TES5Edit/BSArch/` - Reference area for archive behavior; read-only.
- `TES5Edit/Core/wbBSArchive.pas` - Reference area for archive behavior; read-only.
- `TES5Edit/Core/wbBSA.pas` - Reference area for BSA/path/hash behavior; read-only.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `archive_view`: Already owns copied metadata, normalizes paths, returns deterministic path lists, and provides `contains`/`entry` lookup. `ba2_archive` should wrap it like `bsa_archive` does.
- `archive_summary`: Already carries BA2 detection data including version, subtype, file count, `file_table_offset`, and Starfield v3 `compression_method`.
- `entry_metadata`: Already provides path, size, packed size, stored size, absolute offset, hash fields, and compression state for parser-populated metadata.
- `resolve_payload_codec` and `decompress_payload`: Already route FO4/Starfield BA2 deflate and Starfield raw LZ4-block when supplied correct per-entry state and archive compression method.
- `memory_source` and `memory_sink`: Existing test and public helper path for generated fixtures and extraction smoke coverage.

### Established Patterns
- Public APIs live under `include/libbsa/`; private implementation lives under `src/`; public headers use Doxygen comments and expose only libbsa-owned C++20 types.
- Archive objects are metadata-only wrappers over `archive_view`; payload bytes remain in a caller-owned `byte_source` until extraction.
- Parsers use bounded random-access reads, explicit overflow/range checks, and structured `result<T>` failures instead of exceptions for malformed data.
- Tests use Catch2, generated in-test fixtures, labels such as `unit`, `fixture`, `codec`, and public-header smoke coverage.
- `CMakeLists.txt` uses explicit source lists and must be updated for every new public header, source file, and test target.

### Integration Points
- Add `include/libbsa/ba2.hpp` and a corresponding `src/ba2_reader.cpp` or similarly named implementation file.
- Reuse or factor common bounded-read helpers carefully; keep the smallest correct change and avoid moving BSA code unless needed.
- Extend `tests/public_header_smoke.cpp` to include BA2 API symbols.
- Add BA2 reader fixture tests, likely `tests/ba2_reader_tests.cpp`, and wire them explicitly into `CMakeLists.txt` with `unit;fixture` labels and codec labels where route-confusion tests apply.
- Keep `TES5Edit/` read-only: reference tracing is allowed, but no edits, formatting, source-list inclusion, staging, or submodule pointer changes.

</code_context>

<specifics>
## Specific Ideas

- BA2 API should feel intentionally parallel to BSA API, not like the start of a generic archive abstraction.
- Metadata should be consumer-friendly and consistent with existing libbsa semantics, even when BA2 native record fields use different names.
- Compression metadata should tell consumers what extraction will do, not merely echo raw archive defaults.
- Generated fixtures should make BA2 correctness reviewable in source form and keep Phase 11 responsible for broader real-corpus compatibility comparison.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 06-ba2-gnrl-read-and-extract*
*Context gathered: 2026-05-05*
