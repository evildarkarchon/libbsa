# Phase 08: ba2-gnrl-write-new-support - Context

**Gathered:** 2026-05-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 8 adds public write-new support for BA2 GNRL archives. Consumers must be able to create Fallout 4 v1, Starfield v2, and Starfield v3 general BA2 archives from host files or copied memory buffers through an explicit target profile, profile-driven Starfield metadata, metadata-driven compression routing, end-of-archive filename tables, optional stored-payload deduplication, and reader-backed round-trip tests.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `08-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `08-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public BA2 GNRL write-new API for disk-file and memory-buffer entries.
- Explicit Fallout 4 v1, Starfield v2, and Starfield v3 GNRL target profiles.
- BA2 GNRL header, record, payload, and end-of-archive filename-table serialization.
- Version-specific Starfield `Unknown1`, `Unknown2`, and `CompressionMethod` metadata according to documented target-profile options.
- Raw, deflate-compressed, and Starfield raw-LZ4-block-compressed BA2 GNRL payload writing through explicit target/profile policy.
- Per-entry raw/compressed overrides relative to the selected target/default policy.
- Opt-in final-stored-byte payload deduplication for BA2 GNRL entries.
- Reader-backed tests that reopen writer output, inspect metadata, list/find/contains entries, extract entries, and byte-compare source payloads.

**Out of scope (from SPEC.md):**
- BA2 DX10/DDS texture writing - Phase 9 owns DDS analysis, texture records, mip/chunk planning, and DDS writer validation.
- TES3/Morrowind BSA writing - Phase 10 owns TES3 write support and data-section-relative offset serialization.
- In-place mutation of existing archives - v1 explicitly prefers write-new flows until all format writers and validation behavior are proven.
- Parallel packing, parallel compression, streaming writer performance guarantees, and benchmarks - Phase 12 owns performance and concurrency after single-threaded correctness.
- Compatibility warning APIs and broad malformed-input hardening - Phase 11 owns structured validation and hardening APIs.
- Broad BSArchPro UI/CLI option parity - libbsa remains a reusable library and this phase locks only BA2 GNRL writer requirements.
- Copyrighted game archives or mutable `TES5Edit/` fixtures - required validation uses repository-owned generated data or writer-produced archives outside `TES5Edit/`.
- Public exposure of libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, or private parser/codec types - public headers remain dependency-light.

</spec_lock>

<decisions>
## Implementation Decisions

### Public API Surface
- **D-01:** Expose a dedicated BA2 GNRL writer object, not a generic BA2 writer shell. Phase 9 should design BA2 DX10/DDS writer behavior separately when texture writing is in scope.
- **D-02:** Mirror the Phase 7 writer-object flow: construct with target/options, add entries with explicit archive-internal paths, copy memory-buffer entries into writer-owned state, and finalize to a host-path archive.
- **D-03:** Keep Phase 8 output host-path only through a `write_to`-style operation. Do not add memory-output or generic sink-output writer destinations in this phase.
- **D-04:** Reuse the existing public `archive_compression_policy` and `entry_compression_policy` enums. The selected BA2 target/profile maps `compressed` to the correct internal codec.
- **D-05:** Reject duplicate canonical BA2 paths at write time with a structured error, matching Phase 7. Do not switch to add-time rejection.
- **D-06:** Preserve the existing dependency-light public boundary: no public libdeflate, lz4, DirectXTex, Windows SDK, `std::expected`, TES5Edit, or private parser/codec types.

### Starfield Fields
- **D-07:** Starfield writer options use mixed defaults: deterministic profile defaults are provided, and callers may override Starfield `Unknown1`, `Unknown2`, and `CompressionMethod` where applicable.
- **D-08:** Starfield v3 remains one target profile with `CompressionMethod` controlled by writer options. Do not split v3 into separate deflate and raw-LZ4 target profiles.
- **D-09:** Default Starfield `Unknown1` and `Unknown2` values must be reference-derived. Researcher/planner must trace TES5Edit/BSArchPro, generated fixtures, and external BA2 references before locking constants, and implementation should document the compatibility reason near the defaults.
- **D-10:** Starfield v3 `CompressionMethod` is archive-wide. Per-entry compression overrides choose raw versus compressed only; compressed entries use the archive-level method selected by the target/options.
- **D-11:** Method `3` compressed Starfield v3 entries use raw LZ4 block compression. Method `0` compressed entries use deflate. Unsupported methods remain unsupported unless reference evidence justifies them.

### Record Metadata
- **D-12:** Derive the BA2 GNRL 4-byte extension field from the archive path extension without the dot, pad as needed, and validate unsupported or invalid cases instead of truncating silently.
- **D-13:** Use deterministic safe defaults for the BA2 GNRL unknown/record-flags field, but allow an optional advanced per-entry record-flags override when needed for compatibility evidence.
- **D-14:** The writer computes BA2 name and directory hash fields from archive paths using libbsa hash helpers. Do not expose public hash override knobs.
- **D-15:** The writer owns payload offsets, packed/raw sizes, and the fixed `BAADF00D` sentinel. These are not caller-controlled public fields.
- **D-16:** Existing public `entry_metadata` fields are sufficient for writer-output verification: use `archive_hash`, `record_flags`, sizes, offsets, compression, `path`, and `original_path`. Do not add new BA2-specific public entry metadata in Phase 8 unless implementation proves an acceptance criterion is impossible without it.

### Ordering and Layout
- **D-17:** Prefer reference-compatible record/name-table ordering. Researcher/planner must trace TES5Edit/BSArchPro or known BA2 behavior and prefer hash/reference ordering; if evidence is inconclusive, use a deterministic canonical-path fallback and document the fallback.
- **D-18:** Records and filename-table entries must stay paired by index in the selected order. Public `entries()` may remain sorted by canonical path after reopening.
- **D-19:** Serialize filename-table paths using caller-provided archive spelling with separators normalized to `/`. Canonical lowercase paths are for validation, lookup-key derivation, and duplicate detection only.
- **D-20:** Writer output physically follows the Phase 8 SPEC shape: header and records first, stored payload bytes next, and the length-prefixed filename table at end of archive with `FileTableOffset` pointing to it.
- **D-21:** When deduplication is enabled, entries are encoded in the selected record order; the first eligible byte-identical final stored payload owns the payload bytes, and later duplicates share that offset.
- **D-22:** Ordering tests should assert structure plus reader-backed round trip: `FileTableOffset` after payloads, paired record/name order, payload offsets, dedupe metadata, reopen/list/find/contains, and extract byte equality. Do not require full archive byte-golden tests for Phase 8.

### Carry-Forward Decisions
- **D-23:** Preserve Phase 7 final-stored-byte deduplication semantics. Deduplication is disabled by default and compares final stored payload bytes after compression and all metadata-affecting encoding decisions, not source bytes alone.
- **D-24:** Preserve BA2 reader path semantics from Phase 5 and Phase 6: canonical lowercase `/` lookup keys, `original_path` preservation, duplicate canonical path rejection, and `not_found` for valid missing lookup paths.
- **D-25:** Preserve metadata-driven compression routing from earlier phases. Compression selection must come from explicit target/options, raw-vs-packed state, and Starfield `CompressionMethod`, never file extension or archive path guessing.
- **D-26:** Preserve reader-backed validation as the writer acceptance oracle. Writer tests must reopen produced archives with `archive_reader` and assert public metadata, lookup behavior, extraction bytes, stable errors, and the physical layout facts required by SPEC.
- **D-27:** Preserve generated/legal test policy. Do not mutate, format, compile, stage, or use `TES5Edit/` as a fixture workspace.
- **D-28:** Preserve stable error-code testing. Tests should assert `error_code` values and public metadata, not diagnostic message text.

### the agent's Discretion
- Researcher/planner may choose exact public names such as target enum, options struct, and writer class names, provided the API remains dedicated to BA2 GNRL and satisfies the decisions above.
- Researcher/planner may choose exact private source/header file layout, CMake registration, and Catch2 test organization if public headers remain dependency-light and existing tests stay green.
- Researcher/planner may choose exact synthetic entry paths and payload bytes for writer-output tests, provided disk source, memory source, zero-byte, mixed-case lookup, multi-folder, raw/compressed, Starfield v2/v3 metadata, method `0`, method `3`, end filename table, and dedupe acceptance cases are covered.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/08-ba2-gnrl-write-new-support/08-SPEC.md` - Locked Phase 8 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` - Phase 8 goal, mapped requirements WBA2-01 through WBA2-05, success criteria, and downstream Phase 9/10/11/12 boundaries.
- `.planning/REQUIREMENTS.md` - BA2 write requirements plus public API, compression, compatibility, validation, and out-of-scope constraints.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, key decisions, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from prior phases.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, streaming goals, write-new scope, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md` - Writer API shape, compression policy enums, copied memory entries, host-path output, write-time duplicate rejection, final-stored-byte dedupe, and reader-backed writer validation to carry into Phase 8.
- `.planning/phases/06-dds-boundary-and-ba2-dx10-read-reconstruction/06-CONTEXT.md` - BA2 DX10/DDS boundary, DirectXTex-private rule, BA2 path behavior, metadata-driven compression routing, and Phase 9 handoff.
- `.planning/phases/05-ba2-gnrl-read-extract/05-CONTEXT.md` - BA2 GNRL record/header parsing, Starfield metadata optionals, method `3` raw LZ4 block policy, generated fixture manifest policy, stable error-code tests, and BA2 path behavior.
- `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md` - Archive-absolute public offsets, strict duplicate-path handling, generated fixture proof strategy, and variant-specific reader dispatch patterns.
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md` - Public reader surface, canonical/original path semantics, deterministic listings, sink-first extraction, not_found behavior, and generated fixture manifest policy.
- `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md` - Internal binary writer primitives, archive path normalization, FO4/BA2 hash helper, explicit compression router, and exact-size codec behavior.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for BA2 archive structures, extraction behavior, Starfield fields, and compression-method handling; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for archive parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for pack/write option behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit, format, stage, compile, or use as fixture workspace.

### External Format References
- `https://miere.ru/posts/ba2-archive-format/` - Public BA2 `BTDX`, `GNRL`/`DX10`, header, record, payload, and length-prefixed filename-table notes useful for cross-checking reference tracing.
- `https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html` - Independent BA2/BTDX structure reference, including GNRL vs DX10 split and parser behavior.
- `https://github.com/wrye-bash/wrye-bash/issues/667` - Starfield BA2 discussion noting v2 header unknowns and v3 raw LZ4 block distinction from SSE LZ4 frame behavior.

### Current Codebase State
- `include/libbsa/writer.hpp` - Existing public TES4-family writer shape and compression policy enums to reuse or mirror for BA2 GNRL.
- `include/libbsa/libbsa.hpp` - Umbrella public include that must expose the BA2 GNRL writer surface once added.
- `include/libbsa/archive.hpp` - Public archive metadata, BA2 metadata optionals, entry compression enum, and entry metadata fields used for writer-output verification.
- `src/formats/bsa/tes4_bsa_writer.cpp` - Existing writer-owned entry state, explicit path handling, copied memory entries, duplicate validation, stored-payload encoding, final-stored-byte dedupe, temp-file publishing, and reader-backed compatibility comments to mirror carefully.
- `src/formats/ba2/ba2_gnrl_parser.cpp` - Existing BA2 GNRL header/record/name parser, `BAADF00D` sentinel validation, `PackedSize == 0` raw semantics, path normalization, and bounded filename-table read logic. Note: current host-file parser rejects `FileTableOffset` after the first payload, so Phase 8 likely needs a reader adjustment to accept end-of-archive filename tables.
- `src/formats/ba2/ba2_gnrl_reader.cpp` - Existing BA2 GNRL raw streaming, compressed payload buffering, exact-size deflate/raw-LZ4 extraction, and metadata-driven routing used as the round-trip oracle.
- `src/formats/ba2/ba2_format_detector.cpp` - Existing BA2 byte detection and Starfield v2/v3 metadata classification to keep consistent with writer target profiles.
- `src/detail/archive_path.hpp` and `src/detail/archive_path.cpp` - Existing archive virtual path normalization rules for canonical keys, validation, and duplicate detection.
- `src/detail/bethesda_hash.hpp` and `src/detail/bethesda_hash.cpp` - Existing FO4/BA2 hash helper expected to compute BA2 name/directory hash fields for writer records.
- `src/detail/binary_io.hpp` and `src/detail/binary_io.cpp` - Existing checked little-endian read/write primitives for safe serialization.
- `src/detail/compression_router.hpp` and `src/detail/compression_router.cpp` - Existing explicit compression routing for `none`, `deflate`, `lz4_frame`, and `lz4_block`; BA2 GNRL writer should use `deflate` and `lz4_block`, not LZ4 frame.
- `src/detail/deflate_codec.hpp` and `src/detail/deflate_codec.cpp` - Deflate codec used for FO4 BA2, Starfield v2, and Starfield v3 method `0` compressed payloads.
- `src/detail/lz4_block_codec.hpp` and `src/detail/lz4_block_codec.cpp` - Raw LZ4 block codec used for Starfield v3 method `3` compressed payloads.
- `tests/unit/tes4_bsa_writer_tests.cpp` - Existing writer-output tests for public API, copied memory, invalid paths, overwrite policy, compression overrides, zero-byte raw entries, embedded names, dedupe, reopen/extract, and stable error-code patterns.
- `tests/unit/ba2_gnrl_reader_tests.cpp` - Existing BA2 GNRL reader manifest, metadata, lookup, extraction, malformed, and stable error-code patterns to reuse for writer-output round trips.
- `tests/unit/public_include_boundary_tests.cpp` - Public dependency boundary tests that must stay green after adding BA2 GNRL writer APIs.
- `tests/CMakeLists.txt` - Catch2, fixture generator, and CTest label wiring for adding writer tests.
- `CMakeLists.txt` - Library public header file set and private source registration for adding BA2 writer implementation files.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `archive_compression_policy`, `entry_compression_policy`, and the Phase 7 `tes4_bsa_writer` shape provide the preferred public writer pattern for BA2 GNRL.
- `detail::normalize_archive_path` already provides canonical lowercase `/` path validation for lookup keys and duplicate detection while allowing original spelling to be preserved separately.
- `detail::hash_fo4` or existing FO4/BA2 hash support should compute BA2 record hash fields; callers should not supply raw hashes.
- `detail::compress_payload` already supports deflate and raw LZ4 block through explicit internal metadata, matching BA2 GNRL compressed payload needs.
- Existing BA2 GNRL reader/extractor code is the acceptance oracle for writer output: reopen, inspect metadata, list/find/contains, extract, and compare bytes.

### Established Patterns
- Public headers live in `include/libbsa/`; private parsers, writers, codecs, and format helpers live under `src/` or `src/detail/`.
- Public APIs use Doxygen-style `///` comments. New writer classes, target/options types, entry-add methods, advanced record-flags knobs, and finalization methods need tight public doc comments.
- Tests assert public metadata and stable `error_code` values, not diagnostic message text.
- Generated fixtures and writer-output tests must use repository-owned synthetic data and must not mutate, stage, or use anything under `TES5Edit/` as a workspace.
- Catch2 tags are propagated into CTest labels through existing `catch_discover_tests(... ADD_TAGS_AS_LABELS)` wiring.

### Integration Points
- Add a dedicated BA2 GNRL public writer surface in `include/libbsa/writer.hpp` or a new public writer header included by `include/libbsa/libbsa.hpp`.
- Add private BA2 GNRL writer implementation under `src/formats/ba2/` or an equivalent writer-specific internal split.
- Extend CMake public header file sets and private source registration for BA2 writer files.
- Update BA2 GNRL reader parsing if needed so writer-output archives with end-of-archive filename tables reopen successfully.
- Add writer-output tests that create archives in temporary/generated test locations, reopen through `archive_reader::open`, and assert metadata, path lookup, extraction bytes, Starfield fields, compression metadata, payload offsets, end filename tables, and dedupe offsets.
- Keep public include boundary and installed-package smoke tests green after adding the BA2 GNRL writer API.

</code_context>

<specifics>
## Specific Ideas

- A likely public shape is a dedicated `ba2_gnrl_writer` with `ba2_gnrl_target` values for Fallout 4 v1, Starfield v2, and Starfield v3, plus a writer options struct carrying compression policy, Starfield header overrides, deduplication, overwrite behavior, and any advanced record-flags override mechanism.
- Keep the consumer-facing compression API simple: archive-wide policy plus per-entry `inherit`/`raw`/`compressed`. The selected BA2 target/options decide whether compressed means deflate or raw LZ4 block.
- Treat Starfield raw header defaults as compatibility facts to research and document, not arbitrary constants.
- Treat end-of-archive filename-table support as both writer and reader work: writer must emit it, and `archive_reader` must reopen it.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 08-ba2-gnrl-write-new-support*
*Context gathered: 2026-05-09*
