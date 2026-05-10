# Phase 07: TES4-Family BSA Write-New Support - Context

**Gathered:** 2026-05-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 7 adds public write-new support for TES4-family BSA archives through a dependency-light C++20 writer surface. Consumers must be able to create TES4/Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 archives from host files or memory-buffer entries, then validate output by reopening with `archive_reader`, listing/finding entries, extracting through public APIs, and byte-comparing source payloads.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `07-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `07-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public TES4-family write-new API for creating new BSA archives from disk-file entries and memory-buffer entries.
- Explicit v103, v104, and v105 target profiles.
- TES4-family folder/file hash computation, index sorting, table serialization, and payload offset serialization.
- Archive flag and file flag derivation for supported TES4-family content categories.
- Raw, deflate, and LZ4-frame payload writing through explicit target-profile compression routing.
- Per-file compression overrides relative to the selected archive default.
- Explicit opt-in embedded-name emission with round-trip extraction proof.
- Explicit opt-in identical-payload deduplication with round-trip extraction proof.
- Generated legal writer-output tests that reopen archives with `archive_reader`, extract entries, and byte-compare source payloads.

**Out of scope (from SPEC.md):**
- TES3/Morrowind BSA writing - Phase 10 owns TES3 write support and data-section-relative offset semantics.
- BA2 GNRL or BA2 DX10 writing - Phases 8 and 9 own BA2 writer behavior.
- In-place mutation of existing archives - v1 explicitly prefers write-new flows until full offset and validation behavior is proven.
- Parallel packing, parallel compression, and performance benchmarking - Phase 12 owns performance and concurrency after single-threaded correctness is established.
- Broad BSArchPro UI/CLI option parity - libbsa is a reusable library and this phase locks only the WBSA requirements listed for Phase 7.
- Compatibility warning API or validator framework - Phase 11 owns structured compatibility warnings and hardening APIs.
- Copyrighted game archives or mutable `TES5Edit/` fixtures - writer validation must use repository-owned generated fixtures or caller-provided local-only data.

</spec_lock>

<decisions>
## Implementation Decisions

### Writer API Shape
- **D-01:** Expose TES4-family write-new as a public writer object, not a single one-shot function. The writer object should hold target options, accept entries, and produce a host-path archive when finalized.
- **D-02:** Memory-buffer entries are copied into writer-owned storage when added. Callers may release or mutate their original buffers after `add_bytes`-style calls without invalidating the future write.
- **D-03:** Phase 7 exposes host-path output only. Do not add memory-output or generic sink-output writer destinations in this phase.
- **D-04:** Existing destination archive paths fail by default. If overwrite is supported, it must be an explicit option rather than the default behavior.

### Path Handling
- **D-05:** Disk-file entries require an explicit archive-internal path. Do not infer archive paths from host directory layout as the primary Phase 7 API.
- **D-06:** Preserve caller-provided archive path spelling in serialized folder/file name tables, while using existing archive path normalization only for validation, lookup-key derivation, and duplicate detection.
- **D-07:** Reject duplicate canonical archive paths at write time with a structured error. Do not use last-wins behavior, and do not require incremental add-time duplicate rejection.
- **D-08:** Phase 7 writes file entries only. Empty directory representation is out of scope unless required by reference tracing for files already being written.

### Compression Controls
- **D-09:** Public per-entry compression selection should use a simple state such as `inherit`, `raw`, or `compressed`. The writer maps `compressed` to deflate for v103/v104 and LZ4 frame for v105 using the explicit target profile.
- **D-10:** Archive-wide compression defaults should follow the selected target profile by default, while still allowing callers to choose all-raw or all-compressed archive policy through named options if planner needs those options to satisfy the SPEC.
- **D-11:** Zero-byte entries are stored raw even when the effective policy requests compression.
- **D-12:** Compression failure or an invalid compression request for the selected target profile fails the write with a structured error. Do not silently fall back to raw, and do not add warning-and-continue behavior before the Phase 11 warning API exists.
- **D-13:** Codec compression-level tuning stays internal in Phase 7. The public API exposes per-entry raw/compressed selection, not libdeflate or LZ4 level knobs.
- **D-14:** The writer should set the archive compression flag to match the archive-wide default and use per-file toggle bits only for entries that deviate from that default.
- **D-15:** For non-empty files, honor the effective compression policy. Do not add a hidden size-threshold rule that stores tiny files raw when the policy says compressed.

### Compatibility Knobs
- **D-16:** Callers should not set raw archive flag bits directly in Phase 7. The writer derives required flags and exposes only safe named options for compatibility-sensitive behaviors.
- **D-17:** Derive file category flags as the union of known category bits based on entry extensions across the archive. Researcher/planner must trace BSArchPro/TES5Edit for the exact extension-to-flag mapping before implementation.
- **D-18:** Embedded names are off by default and controlled by one global archive option. If enabled, the writer emits embedded name prefixes only where target-compatible; do not expose per-entry embedded-name opt-in in Phase 7 unless reference tracing proves global behavior is unsafe.
- **D-19:** Opt-in deduplication may share payload offsets only for entries whose final stored bytes are identical after compression policy, embedded-name policy, and size-prefix encoding are applied. Do not dedupe merely because source bytes match if stored encoding differs.

### Carry-Forward Decisions
- **D-20:** Preserve the established dependency-light public boundary: no public libdeflate, lz4, DirectXTex, Windows SDK, `std::expected`, or `TES5Edit/` types.
- **D-21:** Preserve C++20 result/error style for writer failures, reserving exceptions for programmer precondition misuse consistent with the existing `result` behavior.
- **D-22:** Preserve existing archive-reader validation as the writer acceptance oracle: writer tests must reopen produced archives with `archive_reader` and verify public metadata, lookup behavior, extraction bytes, and stable errors.
- **D-23:** Preserve generated legal fixture/test policy. Production writer tests may generate writer-output archives during tests, but committed fixtures and generated manifests must document synthetic provenance and must not use `TES5Edit/` as a mutable workspace.

### the agent's Discretion
- Researcher/planner may choose exact class/function names, source file names, helper boundaries, CMake target organization, and test file organization if the decisions above and `07-SPEC.md` are preserved.
- Researcher/planner should trace TES5Edit/BSArchPro before locking binary layout details: folder/file sorting, folder record offsets, archive flags, file category flags, embedded-name compatibility by version, and compression toggle semantics.
- Planner may choose exact generated writer-output filenames, entry paths, payload bytes, and manifest shape as long as disk-source, memory-source, zero-byte, mixed-case lookup, multi-folder, compression override, embedded-name, and dedupe acceptance cases are covered.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-SPEC.md` - Locked Phase 7 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` - Phase 7 goal, mapped requirements WBSA-01 through WBSA-03 and WBSA-05 through WBSA-10, success criteria, and downstream Phase 8/10 boundaries.
- `.planning/REQUIREMENTS.md` - BSA write requirements plus public API, compression, fixture, compatibility, validation, and out-of-scope constraints.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, key decisions, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from prior phases.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, streaming goals, write-new scope, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/06-dds-boundary-and-ba2-dx10-read-reconstruction/06-CONTEXT.md` - Latest public metadata boundary, generated fixture policy, DirectXTex-private dependency rule, and carry-forward API/test conventions.
- `.planning/phases/05-ba2-gnrl-read-extract/05-CONTEXT.md` - Metadata-driven compression routing, generated fixture manifest policy, stable error-code testing, and archive path behavior carried into writer tests.
- `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md` - Archive-absolute public offsets, strict duplicate-path behavior, generated fixture proof strategy, and variant-specific reader dispatch patterns.
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md` - TES4-family reader surface, canonical/original path semantics, deterministic listings, sink-first extraction, embedded-name extraction, and BSA fixture generator behavior.
- `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md` - Internal binary writer primitives, archive path normalization, TES4 hash helper, explicit compression router, and exact-size codec behavior.

### Reference Material
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for TES4-family BSA parsing/writing-related archive layout and compatibility behavior; use for tracing only, never modify.
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for archive structures and extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for pack/write option behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit, format, stage, compile, or use as fixture workspace.

### Current Codebase State
- `include/libbsa/archive.hpp` - Current public `archive_reader`, metadata, compression enum, and `payload_sink` surface; Phase 7 adds writer API here or in a new public writer header without leaking private types.
- `include/libbsa/libbsa.hpp` - Umbrella public include that must expose the writer surface once added.
- `src/archive.cpp` - Current reader facade and dispatch pattern used as the reopen/extract oracle for writer-output tests.
- `src/formats/bsa/tes4_bsa_parser.hpp` and `src/formats/bsa/tes4_bsa_parser.cpp` - Existing TES4-family header/table/name parsing, compression toggle interpretation, embedded-name metadata, duplicate canonical path validation, and folder offset expectations to mirror in writer serialization.
- `src/formats/bsa/tes4_bsa_reader.hpp` and `src/formats/bsa/tes4_bsa_reader.cpp` - Existing deterministic listing, normalized lookup, embedded-name stripping, compressed payload decoding, and sink-first extraction behavior used to validate produced archives.
- `src/detail/bethesda_hash.hpp` and `src/detail/bethesda_hash.cpp` - Existing TES4-family hash helpers for writer folder/file ordering, manifest proof, and compatibility validation.
- `src/detail/binary_io.hpp` and `src/detail/binary_io.cpp` - Existing checked little-endian binary primitives; writer should reuse or extend writer-side binary helpers rather than hand-rolling unsafe serialization.
- `src/detail/archive_path.hpp` and `src/detail/archive_path.cpp` - Existing archive virtual path normalization and validation rules for canonical keys and duplicate detection.
- `src/detail/compression_router.hpp` and `src/detail/compression_router.cpp` - Existing explicit `compress_payload` and `decompress_payload_exact` routing for `none`, `deflate`, and `lz4_frame` needed by TES4-family writer payload encoding.
- `src/detail/deflate_codec.hpp` and `src/detail/deflate_codec.cpp` - Deflate codec used for v103/v104 compressed payloads.
- `src/detail/lz4_frame_codec.hpp` and `src/detail/lz4_frame_codec.cpp` - LZ4 frame codec used for v105 compressed payloads.
- `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` - Existing test-only TES4-family serializer, manifest shape, compression helper reuse, folder-offset compatibility comment, and generated-fixture provenance pattern to replace with production writer behavior rather than reuse as public API.
- `tests/unit/tes4_bsa_reader_tests.cpp` - Existing manifest-driven TES4-family metadata, lookup, extraction, embedded-name, compression, malformed, and error-code assertion patterns to mirror for writer-output round trips.
- `tests/unit/public_include_boundary_tests.cpp` - Public dependency boundary tests that must stay green after adding the writer API.
- `tests/CMakeLists.txt` - Existing Catch2, fixture generator, custom target, and CTest label wiring for adding writer tests.
- `CMakeLists.txt` - Library public header file set and private source registration; add writer public/private files here without exposing dependency types.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `detail::hash_tes4` already computes TES4-family hashes and is used by the current TES4 fixture generator and parser tests.
- `detail::normalize_archive_path` already provides canonical lowercase `/` validation for duplicate detection and lookup compatibility.
- `detail::compress_payload` already supports `none`, `deflate`, and `lz4_frame`, matching Phase 7 raw/v103-v104/v105 payload needs.
- Existing `tes4_bsa_parser` code documents key on-disk rules the writer must reproduce, including `BSA\0` header shape, fixed header size, folder record sizes, file record size, compression toggle bit, archive embedded-name bit, and TES5Edit-compatible folder offsets that include the file-name table contribution.
- Existing `archive_reader` plus TES4 reader helpers provide the required acceptance oracle for pack/reopen/list/find/contains/extract/byte-compare tests.
- Existing generated fixture tooling has useful manifest/provenance patterns, but production writer behavior should live outside test-only fixture generator code.

### Established Patterns
- Public headers live in `include/libbsa/`; private parsers, codecs, and format helpers live under `src/` or `src/detail/`.
- Public APIs use Doxygen-style `///` comments. New writer classes, options, source-entry methods, and finalization methods need tight public doc comments.
- Tests assert public metadata and stable `error_code` values, not diagnostic message text.
- Generated fixtures and writer-output tests must use repository-owned synthetic data and must not mutate or stage anything under `TES5Edit/`.
- Catch2 tags are turned into CTest labels through `catch_discover_tests(... ADD_TAGS_AS_LABELS)`.

### Integration Points
- Add a public writer surface to `include/libbsa/` and include it from `include/libbsa/libbsa.hpp`.
- Add private TES4-family writer implementation under `src/formats/bsa/` or a comparable writer-specific internal split.
- Extend CMake public header file sets and private source registration for the writer implementation.
- Add writer-output tests that create archives in temp/generated test locations, reopen them through `archive_reader::open`, and assert metadata, path lookup, extraction bytes, embedded-name metadata, compression metadata, and dedupe offsets.
- Reuse existing compression and path/hash helpers, but keep public writer options expressed in libbsa-owned enums and value types.

</code_context>

<specifics>
## Specific Ideas

- The preferred public shape is a safe writer object: configure target profile/options, add explicit archive paths from host files or copied memory bytes, then write to a host path.
- Keep the first writer API intentionally file-output focused. Memory-output and generic stream/sink output are not Phase 7 decisions.
- Per-entry compression selection should be consumer-friendly (`inherit`, `raw`, `compressed`) while codec selection remains target-profile-driven internally.
- Compatibility-sensitive raw flags should not be a public escape hatch in Phase 7. Favor named policy options plus derived bits.
- Deduplication should happen after payload encoding, not before, so shared offsets never depend on reconciling different stored encodings.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 07-tes4-family-bsa-write-new-support*
*Context gathered: 2026-05-08*
