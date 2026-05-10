# Phase 03: Format Detection and TES4-Family BSA Read/Extract - Context

**Gathered:** 2026-05-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 3 replaces the Phase 1 `archive_reader::open` unsupported stub with a usable TES4-family BSA read/extract slice. It must detect TES4/Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 archives from bytes; expose archive metadata, entry metadata, deterministic path listing, normalized lookup, and bounded extraction; and prove raw, deflate, LZ4-frame, and embedded-name behavior with committed legal fixtures. TES3, BA2, writers, filesystem extraction convenience, progress/cancellation, and performance/concurrency work remain out of scope.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**10 requirements are locked.** See `03-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `03-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public TES4-family open/read behavior for `archive_reader::open` replacing the current non-empty-path `unsupported` stub.
- Byte-driven detection for TES4/Oblivion BSA v103, FO3/FNV/Skyrim LE BSA v104, and Skyrim SE/AE BSA v105.
- Public library-owned archive metadata, entry metadata, stable path listing, path existence checks, and entry lookup behavior.
- Extraction of TES4-family BSA entries stored raw, deflate-compressed, or LZ4-frame-compressed according to archive version, flags, and record metadata.
- Embedded-name entry extraction behavior with expected output bytes documented and tested.
- Committed tiny legal generated fixtures for default CI coverage of detection, metadata, lookup, extraction, compression routing, and embedded-name behavior.
- Optional local or BSArchPro-derived compatibility tests may supplement coverage but must not be required for default CI completion.

**Out of scope (from SPEC.md):**
- TES3/Morrowind BSA parsing or extraction - Phase 4 owns TES3 data-section-relative offset behavior.
- BA2 GNRL or BA2 DDS parsing/extraction - Phases 5 and 6 own BA2 records, filename tables, and DDS reconstruction.
- Any write-new BSA or BA2 behavior - writer phases start at Phase 7 after readers are proven.
- In-place archive mutation - v1 explicitly excludes mutating existing archives.
- GUI, CLI tooling, or sample app behavior - libbsa is a reusable library and examples/docs are later scope.
- Whole-archive performance optimization, parallel extraction, and benchmarks - Phase 12 owns performance and concurrency after correctness is established.
- Public exposure of libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, or `std::expected` types - the public boundary remains portable C++20.
- Required committed game-derived archives or writable TES5Edit fixtures - fixture policy forbids copyrighted archives in the repo and treats `TES5Edit/` as read-only.

</spec_lock>

<decisions>
## Implementation Decisions

### Public Reader Surface
- **D-01:** Optimize the public reader slice around methods on `archive_reader`: metadata, entry listing, path lookup/contains, and extraction should be callable from the opened reader object rather than split into separate public view/extractor objects in Phase 3.
- **D-02:** Archive-level public metadata should expose only the required Phase 3 fields: archive type/variant, version, archive flags, file count, and supported compression behavior. Avoid broad variant unions or stringly typed metadata maps for now.
- **D-03:** Entry listing should return stable value metadata records, not paths-only lists or live entry handles. Entry records should include the canonical path and the sizes, offset, hash, compression, embedded-name, and format-specific fields needed by `03-SPEC.md` acceptance.
- **D-04:** Direct lookup should return `result<std::optional<entry_metadata>>` or an equivalent shape: invalid lookup strings remain errors, while valid-but-missing archive paths are normal absence.

### Path Display And Lookup Policy
- **D-05:** Public entry metadata should expose both `path` and `original_path`. `path` is the canonical normalized lookup key; `original_path` is archive-derived display/source spelling.
- **D-06:** If two archive records normalize to the same canonical `path`, treat the archive as invalid and fail with `error_code::format_error` rather than choosing a winner or exposing ambiguous duplicates.
- **D-07:** `reader.entries()` should return entries in canonical path order for deterministic consumer behavior and tests. Preserve record/hash order internally only where parser compatibility needs it.
- **D-08:** If a structurally valid TES4-family archive lacks usable name strings needed to list paths, Phase 3 should fail open/read as `unsupported` rather than expose hash-only partial behavior.
- **D-09:** `original_path` should preserve parsed folder/file casing and spelling but join split archive name components into a predictable single `/`-separated string. Do not use `std::filesystem::path` semantics for archive-internal paths.
- **D-10:** All public path input should be normalized through the existing virtual path semantics so case and `/` vs `\` variants resolve to the same canonical key.
- **D-11:** Invalid lookup strings such as empty, rooted, or traversal-like archive paths should fail with `error_code::invalid_argument`.

### Extraction Contract
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

### Fixture Proof Strategy
- **D-23:** Default Phase 3 proof should use committed tiny generated BSA archives plus machine-readable JSON manifests that describe expected metadata, paths, and extracted bytes/hashes.
- **D-24:** Require at least one success fixture archive per supported TES4-family variant: v103, v104, and v105.
- **D-25:** Across the success fixture set, cover raw entries, v103/v104 deflate entries, v105 LZ4-frame entries, mixed path casing/separator lookup inputs, and embedded-name payload behavior.
- **D-26:** Commit the fixture generation code, not just binary archives. Keep generators and generated outputs under test fixture tooling outside `TES5Edit/`, with provenance documented in fixture README/manifest files.
- **D-27:** Require a focused malformed fixture set for default CI: unsupported version, truncated header/table, duplicate canonical path, corrupt compressed payload, size mismatch, and non-BSA bytes with a `.bsa` host filename.
- **D-28:** BSArchPro-derived or game-derived compatibility comparisons are optional supplements only. They must not block default CI and must not require committed copyrighted archives.
- **D-29:** The JSON manifest parser may be implemented with a documented test-only dependency exception. The dependency must not leak into libbsa public headers or runtime library linkage.

### Agent Discretion
- Planner may choose exact internal parser class names, source file split, and handler registration mechanics as long as future TES3/BA2 format handlers can be added without rewriting TES4-family code.
- Planner may choose the exact JSON library after research, but it must remain test/tool-only and be documented as a dependency-policy exception.
- Planner may choose exact public type names for metadata and sink interfaces, but must preserve the semantics above and keep public headers C++20-compatible and dependency-light.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-SPEC.md` — Locked Phase 3 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` — Phase 3 goal, mapped requirements FMT-01 through FMT-06 and BSA-01/BSA-02/BSA-03/BSA-05/BSA-06/BSA-07, success criteria, and downstream phase ordering.
- `.planning/REQUIREMENTS.md` — Format detection, metadata, TES4-family BSA read/extract, compatibility, and validation requirements.

### Project Constraints
- `.planning/PROJECT.md` — Project purpose, approved dependency policy, public API constraints, streaming goal, error model, and TES5Edit boundary.
- `.planning/STATE.md` — Current project state and recent decisions from Phase 1 and Phase 2.
- `AGENTS.md` — Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` — Product goals, supported archive families, compression split, streaming goals, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md` — Internal path, streaming, hash, codec, routing, exact-size decompression, and public-boundary decisions carried into Phase 3.
- `.planning/phases/01-foundation-api-boundary-and-test-harness/01-CONTEXT.md` — Public facade/result/error decisions, fixture policy, dependency boundary, and test label taxonomy carried forward.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` — Read-only behavioral reference for archive/hash behavior; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` — Read-only behavioral reference for BSA parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` — Read-only BSArchPro application reference for behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` — Read-only reference directory for BSArchPro-related behavior; do not edit or use as fixture workspace.

### Current Codebase State
- `include/libbsa/archive.hpp` — Current public `archive_reader::open(std::string_view)` facade that Phase 3 extends from unsupported stub to real reader behavior.
- `include/libbsa/result.hpp` — Existing public `libbsa::result`, `libbsa::error`, and `libbsa::error_code` boundary; Phase 3 should add `not_found` or equivalent only if needed for missing entries.
- `include/libbsa/libbsa.hpp` — Umbrella public include that must remain dependency-light.
- `src/archive.cpp` — Current unsupported open stub to replace with byte-driven TES4-family opening behavior.
- `src/detail/archive_path.hpp` — Existing internal normalized lowercase forward-slash archive path key and invalid-path rejection.
- `src/detail/bethesda_hash.hpp` — Existing internal TES4-family hash helpers for path/index lookup compatibility.
- `src/detail/compression_router.hpp` — Existing explicit `compression_method` routing and exact-size decompression entry point.
- `src/detail/payload_stream.hpp` — Existing internal synchronous bounded source/sink transfer model and partial-sink failure behavior.
- `CMakeLists.txt` — Library target, public header file set, private source list, and dependency linkage boundaries.
- `tests/CMakeLists.txt` — Catch2/CTest test target, labels via `catch_discover_tests`, and source registration pattern.
- `tests/fixtures/README.md` — Fixture provenance and local-game-data policy for generated fixture additions.
- `tests/unit/archive_reader_tests.cpp` — Existing public open stub tests that Phase 3 must replace or update.
- `tests/unit/public_include_boundary_tests.cpp` — Public dependency boundary tests that must continue to reject private dependency leakage.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `libbsa::result<T>`, `libbsa::result<void>`, `libbsa::error`, and `libbsa::error_code` already exist and should carry open, lookup, and extraction failures.
- `archive_reader::open(std::string_view)` exists as the public facade and currently returns `unsupported` for non-empty paths.
- `libbsa::detail::normalize_archive_path` already provides lowercase `/`-separated canonical keys and rejects rooted/traversal-like archive paths.
- `libbsa::detail::hash_tes4` already provides TES4-family hash behavior for lookup/index compatibility.
- `libbsa::detail::decompress_payload_exact` already routes deflate and LZ4 frame payloads by explicit metadata and validates exact decompressed size.
- `libbsa::detail::payload_sink` and `transfer_payload` already model synchronous bounded writes and partial-sink failure semantics internally.

### Established Patterns
- Public headers live under `include/libbsa/`; dependency-bearing implementation and format helpers should stay under `src/` or `src/detail/`.
- Public APIs use Doxygen-style `///` comments and libbsa-owned C++20 value/result types.
- Tests assert stable error codes rather than exact diagnostic strings.
- Catch2 tags become CTest labels through `catch_discover_tests(... ADD_TAGS_AS_LABELS)`.
- Fixture policy separates committed generated fixtures from ignored local game-derived data and forbids using `TES5Edit/` as a fixture workspace.

### Integration Points
- Add TES4-family parser/detector sources to `target_sources(libbsa PRIVATE ...)` without adding private dependency headers to the public header file set.
- Extend public headers only for reader metadata, entry metadata, sink interface, and missing-entry error behavior needed by Phase 3.
- Update `archive_reader_tests.cpp` from unsupported-stub assertions to byte-driven open/list/query/extract coverage.
- Add fixture, malformed, and public-boundary tests to `tests/CMakeLists.txt` using existing labels such as `unit`, `fixture`, `malformed`, and optional `compat`/`requires-game-fixture` where applicable.
- Keep generated fixture archives and manifests under `tests/fixtures/generated` or adjacent fixture tooling, with provenance and generator instructions documented.

</code_context>

<specifics>
## Specific Ideas

- The public reader should be pleasant for consumers: open once, call methods on `archive_reader`, get typed value metadata, and extract by path.
- `path` means canonical lookup key; `original_path` means archive-derived display/source spelling. Do not let original spelling affect identity.
- Synchronous sink extraction is the primary API, but `extract_bytes(path)` or equivalent is useful for tests and small entries if bounded by entry metadata.
- A test-only JSON dependency is intentionally allowed as a documented exception for fixture manifest parsing, but runtime/public dependency leakage remains forbidden.
- Generated fixtures should prove behavior by archive bytes, not by filename extensions, and should include deliberately misleading host filenames where useful.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 03-Format Detection and TES4-Family BSA Read/Extract*
*Context gathered: 2026-05-08*
