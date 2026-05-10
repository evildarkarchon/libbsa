# Phase 05: BA2 GNRL Read/Extract - Context

**Gathered:** 2026-05-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 5 adds BA2 GNRL read/extract support through the existing `archive_reader` API. Consumers must be able to open, inspect, list, query, and extract Fallout 4 BA2 GNRL, Starfield BA2 v2 GNRL, and structurally valid Starfield BA2 v3 GNRL archives, including Starfield version-specific header metadata and explicit raw/deflate/raw-LZ4-block compression routing. This phase is limited to general BA2 archives; BA2 DX10/DDS texture record parsing, DDS reconstruction, and DirectXTex-backed validation remain Phase 6 scope.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `05-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `05-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- BA2 GNRL byte detection and open routing for Fallout 4, Starfield v2, and structurally valid Starfield v3 archives.
- BA2 GNRL header, record, and length-prefixed filename table parsing.
- Deterministic listing, normalized path lookup, containment checks, and entry metadata for BA2 GNRL archives.
- Public or documented library-owned inspection of Starfield v2 `Unknown1` and `Unknown2`, plus Starfield v3 `CompressionMethod`.
- Extraction of BA2 GNRL raw, deflate-compressed, and Starfield v3 raw-LZ4-block-compressed entries.
- TES5Edit/BSArchPro-relevant Starfield v3 GNRL quirks that affect the required read/extract behaviors.
- Generated legal BA2 GNRL fixtures and manifests sufficient to validate required behavior without copyrighted game archives.
- Regression coverage proving BSA readers, public include boundaries, and the TES5Edit read-only boundary remain intact.

**Out of scope (from SPEC.md):**
- BA2 DX10/DDS texture archive parsing or DDS reconstruction - Phase 6 owns texture records, chunk layout, DDS headers, and DirectXTex validation.
- BA2 write-new support - Phase 8 owns GNRL archive creation, filename table serialization, and compression policy for writers.
- BA2 DX10 write-new support - Phase 9 owns texture archive creation from DDS input.
- Whole-project malformed archive hardening beyond focused BA2 parser validity checks needed for this reader - Phase 11 owns broad compatibility warnings and malformed-input hardening.
- Real game archive fixtures as required acceptance evidence - generated legal fixtures are the required proof; local game fixtures remain optional.
- CLI or GUI tooling - libbsa remains a reusable library only.
- In-place archive mutation - v1 read/write sequencing excludes mutation of existing archives.
- Performance/concurrency guarantees beyond preserving bounded open state and sink-based extraction semantics - Phase 12 owns benchmarks and parallel workflows.
- Public exposure of libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, or private parser types - public headers must remain dependency-light.
- Open-ended internet research for Starfield v3 quirks - Phase 5's quirk scope is bounded to TES5Edit/BSArchPro-relevant reference behavior and generated fixture evidence.

</spec_lock>

<decisions>
## Implementation Decisions

### Metadata Surface
- **D-01:** Expose Starfield BA2 version-specific header fields through a small typed optional public BA2 metadata surface, not through private parser structs and not through a stringly typed metadata map.
- **D-02:** Keep raw field names for currently unknown Starfield values. Use names equivalent to `starfield_unknown1`, `starfield_unknown2`, and `compression_method` so the API does not invent semantics before reference evidence exists.
- **D-03:** BA2 metadata fields must be version-gated optionals. Absence for Fallout 4 or older header shapes must remain distinguishable from a present raw value of zero.
- **D-04:** Do not add BA2-specific entry metadata beyond existing `entry_metadata` fields unless reference tracing proves a required BA2 record value has no public home. Prefer existing `archive_hash`, `record_flags`, sizes, archive-absolute `payload_offset`, `compression`, `path`, and `original_path`.

### Fixture Matrix
- **D-05:** Generate at least one BA2 GNRL success fixture per required variant: Fallout 4 GNRL, Starfield v2 GNRL, and Starfield v3 GNRL.
- **D-06:** Success fixtures should be edge-heavy rather than minimal. Across the three fixtures, deliberately cover mixed path spelling, slash/backslash handling, nested folders, zero-byte raw entries, duplicate-free close path variants, non-default Starfield fields, raw entries, deflate entries, and Starfield v3 raw-LZ4-block entries.
- **D-07:** Use a dedicated BA2 GNRL fixture generator, such as `generate_ba2_gnrl_fixtures.cpp`, while reusing helper patterns where sensible. Keep generator provenance explicit and outside `TES5Edit/`.
- **D-08:** Phase 5 malformed coverage must include focused BA2 parser/extraction failures, not just happy-path archives. Required categories include truncated header/records/name table, invalid payload span, duplicate canonical path, corrupt compressed payload, exact-size mismatch, unsupported BA2 subtype, and unsupported v3 compression method.
- **D-09:** BA2 fixture manifests should be rich metadata manifests. They must record canonical and original paths, lookup variants, raw and stored sizes, archive-absolute payload offsets, archive hash, record flags, compression route, Starfield header fields, and expected extracted bytes.
- **D-10:** The Phase 5 acceptance gate remains generated-only. Optional local real-game BA2 checks may be added later only if skipped by default and non-blocking, but Phase 5 must not require copyrighted archives.
- **D-11:** Tests should assert public metadata, manifest fields needed for compatibility proof, lookup variants, exact extracted bytes, and stable error codes for malformed cases. Do not assert diagnostic message text.

### V3 Method Policy
- **D-12:** For Starfield BA2 v3 compressed GNRL entries, `CompressionMethod == 3` routes to raw LZ4 block via the existing raw-block codec path.
- **D-13:** Other Starfield v3 compression-method values may route to deflate only when TES5Edit/BSArchPro reference evidence shows that value is an observed non-LZ4 path. Do not broadly treat every non-3 value as deflate without evidence.
- **D-14:** A Starfield v3 `CompressionMethod` value unsupported by reference evidence fails `archive_reader::open` with `error_code::unsupported` rather than producing a partially usable reader.
- **D-15:** Compression routing must never infer behavior from file extension, BA2 record extension fields, or archive path. Use parsed archive metadata: raw-vs-packed state plus archive family/version and `CompressionMethod`.

### DX10 Rejection
- **D-16:** Valid BA2 `BTDX` bytes with archive type `DX10` must return `error_code::unsupported` in Phase 5. DX10 is a valid BA2 subtype, but it belongs to Phase 6.
- **D-17:** Phase 5 should inspect only enough fixed header data to classify `BTDX` plus `DX10`, then reject before texture record or chunk parsing. Do not parse minimal texture metadata in Phase 5.
- **D-18:** Generated Phase 5 fixtures must include a tiny DX10 rejection fixture proving clean unsupported routing without committing texture behavior.
- **D-19:** Error classification should distinguish unsupported valid BA2 profiles from malformed supported GNRL bytes. Valid `BTDX` with unsupported subtype/version/method returns `unsupported`; malformed, truncated, or internally inconsistent supported GNRL bytes return `format_error`.

### Carry-Forward Decisions
- **D-20:** Preserve the established public `archive_reader` facade: open once, inspect metadata, list deterministic value entries, find/contains by normalized archive virtual path, and extract by path through a caller-owned `payload_sink`.
- **D-21:** Preserve path semantics from prior phases: `path` is the canonical lowercase `/` lookup key, `original_path` preserves archive-derived spelling, duplicate canonical paths fail open, and archive paths are not `std::filesystem::path` values.
- **D-22:** Preserve sink-first extraction. `extract_bytes(path)` remains a bounded convenience adapter over `extract(path, sink)`, not a separate decoding path.
- **D-23:** Preserve exact-size decompression and explicit codec routing from Phase 2. Deflate and raw LZ4 block outputs must exactly match archive metadata or fail with structured errors.
- **D-24:** Preserve the TES5Edit boundary: trace reference behavior as needed, but do not edit, format, stage, compile, or use `TES5Edit/` as a fixture workspace.

### the agent's Discretion
- Planner may choose exact internal file names, parser structs, helper boundaries, detector dispatch shape, and CMake/test file organization if the decisions above and `05-SPEC.md` are preserved.
- Researcher/planner must trace TES5Edit/BSArchPro for BA2 GNRL header/record layout, filename table parsing, Starfield v2/v3 fields, and observed v3 compression-method behavior before locking implementation details.
- Planner may decide the exact generated fixture filenames and entry payload strings, as long as the generated set proves the required variants, edge-heavy valid cases, focused malformed cases, and rich manifest metadata.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/05-ba2-gnrl-read-extract/05-SPEC.md` - Locked Phase 5 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` - Phase 5 goal, mapped requirements GNRL-01 through GNRL-08, success criteria, and downstream Phase 6/8/9 boundaries.
- `.planning/REQUIREMENTS.md` - BA2 GNRL read requirements plus public API, compression, fixture, compatibility, and validation constraints.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from Phases 1 through 4.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, streaming goals, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md` - Public metadata cleanup, archive-absolute payload offsets, strict duplicate-path handling, fixture proof strategy, and reader dispatch patterns carried into Phase 5.
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md` - Public reader surface, canonical/original path semantics, deterministic listings, sink-first extraction, not_found behavior, and generated fixture manifest policy.
- `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md` - Internal binary/path/hash/compression primitives, explicit compression routing, exact-size decompression, and raw LZ4 block support.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for BA2 archive structures, hash behavior, and Starfield-specific fields; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for archive parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit or use as fixture workspace.

### External Format References
- `https://miere.ru/posts/ba2-archive-format/` - Public BA2 `BTDX`, `GNRL`/`DX10`, filename-table, packed-length, and record-layout notes useful for cross-checking reference tracing.
- `https://github.com/wrye-bash/wrye-bash/issues/667` - Starfield BA2 discussion noting v2 header unknowns, v3 compression-method behavior, and raw LZ4 block distinction from SSE LZ4 frame.
- `https://bethesda-structs.readthedocs.io/en/latest/bethesda_structs.archive.html` - Independent BA2/BTDX structure reference, including GNRL vs DX10 split and DDS reconstruction boundary.

### Current Codebase State
- `include/libbsa/archive.hpp` - Public `archive_reader`, `archive_metadata`, `entry_metadata`, `entry_compression`, and `payload_sink` surface that Phase 5 extends without leaking parser/dependency types.
- `src/archive.cpp` - Current reader open/entries/find/contains/extract dispatch for TES3 and TES4-family BSA variants; Phase 5 adds BA2 detection/parser/reader routing here or an equivalent internal dispatch layer.
- `src/detail/compression_router.hpp` - Internal explicit compression method enum and exact-size decompression API; BA2 parser/extractor should map parsed metadata to this router.
- `src/detail/compression_router.cpp` - Existing routing for `none`, `deflate`, `lz4_frame`, and `lz4_block`; Phase 5 should use `deflate` and `lz4_block`, not `lz4_frame` for Starfield v3.
- `src/detail/lz4_block_codec.hpp` and `src/detail/lz4_block_codec.cpp` - Raw LZ4 block codec used by Starfield BA2 v3 method `3`.
- `src/detail/deflate_codec.hpp` and `src/detail/deflate_codec.cpp` - Exact-size deflate codec used by FO4 BA2 and observed non-LZ4 Starfield paths.
- `src/detail/bethesda_hash.hpp` and `src/detail/bethesda_hash.cpp` - Existing FO4/BA2 hash helper expected to support BA2 metadata and fixture generation.
- `src/detail/archive_path.hpp` and `src/detail/archive_path.cpp` - Existing archive virtual path normalization rules for canonical lookup and duplicate detection.
- `src/formats/bsa/bsa_format_detector.hpp` and `src/formats/bsa/bsa_format_detector.cpp` - Current byte-driven BSA detection shape; useful analog for BA2 detector behavior.
- `src/formats/bsa/tes4_bsa_parser.hpp` and `src/formats/bsa/tes4_bsa_reader.hpp` - Existing parser/reader split for table parsing, deterministic entry values, lookup, and extraction analogs.
- `src/formats/bsa/tes3_bsa_parser.hpp` and `src/formats/bsa/tes3_bsa_reader.hpp` - Current variant-specific parser/reader split and dispatch pattern after Phase 4.
- `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` - Existing generator style, manifest shape, compression helper reuse, malformed fixture approach, and provenance pattern.
- `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` - Dedicated variant generator pattern to mirror for BA2 GNRL.
- `tests/unit/tes4_bsa_reader_tests.cpp` and `tests/unit/tes3_bsa_reader_tests.cpp` - Existing reader, metadata, lookup, extraction, manifest, and malformed assertion patterns to extend for BA2.
- `CMakeLists.txt` - Library source registration and public header file set; BA2 internals must remain private implementation sources.
- `tests/CMakeLists.txt` - Existing Catch2/nlohmann-json test wiring and generated fixture tool targets; Phase 5 adds BA2 generator/tests here.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `libbsa::archive_reader` already exposes the public read surface Phase 5 should reuse: `open`, `metadata`, `entries`, `find`, `contains`, `extract`, and `extract_bytes`.
- `archive_metadata` already has `archive_type::ba2`, `archive_variant::fallout4`, `archive_variant::starfield`, version, flags, file count, and default compression fields, but lacks the typed optional BA2 metadata surface decided above.
- `entry_metadata` already carries canonical/display path data, sizes, archive-absolute payload offset, archive hash, compression mode, record flags, and embedded-name flags. This is sufficient by default for BA2 GNRL entries.
- `detail::normalize_archive_path` already provides lowercase forward-slash archive key semantics and invalid-path rejection for public lookup/extraction inputs.
- `detail::decompress_payload_exact` already routes `none`, `deflate`, and `lz4_block` with exact-size validation needed by BA2 GNRL extraction.
- `detail::hash_fo4`/FO4-BA2 hash support already exists from Phase 2 and should be reused for parser validation and generated fixture manifests.
- Existing generated fixture tooling already writes tiny legal archives plus JSON manifests and uses test-only nlohmann-json without leaking runtime/public dependencies.

### Established Patterns
- Public headers remain under `include/libbsa/`; dependency-bearing and parser-specific code stays under `src/` or `src/detail/`.
- Public APIs use Doxygen-style `///` comments; non-obvious BA2 compatibility rules should be documented near implementation/tests with TES5Edit/BSArchPro reference context.
- Tests assert stable `error_code` values and public metadata, not diagnostic message text.
- Generated fixtures must document legal provenance and must not use `TES5Edit/` as a workspace.
- Catch2 tags become CTest labels through existing `catch_discover_tests(... ADD_TAGS_AS_LABELS)` wiring.

### Integration Points
- Add BA2 byte detection for `BTDX`, `GNRL`, `DX10`, version fields, and Starfield version-specific header fields without relying on host file extension.
- Add BA2 GNRL parser/reader internals alongside existing format-specific BSA internals or under a new `src/formats/ba2/` subtree if planner prefers clearer separation.
- Extend `archive_reader::open` dispatch so BA2 GNRL routes to BA2 parsing and DX10 routes to clean `unsupported` without texture record parsing.
- Extend `entries`, `find`, `contains`, and `extract` dispatch so BA2 uses the same public contracts as TES3/TES4-family readers.
- Add a dedicated generated BA2 GNRL fixture target and manifest-driven unit/integration tests to `tests/CMakeLists.txt`.
- Keep public include boundary tests green after adding the typed optional BA2 metadata surface.

</code_context>

<specifics>
## Specific Ideas

- The metadata surface should be pleasant but conservative: a typed optional BA2 metadata object with raw Starfield field names and version-gated optionals.
- The fixture set should be intentionally edge-heavy while still generated and legal: one success archive per required variant, plus focused BA2 malformed/unsupported fixtures.
- V3 compression support is evidence-bounded: method `3` is raw LZ4 block, observed non-LZ4 methods may be deflate, and unknown methods fail open as unsupported.
- DX10 handling is a boundary proof only: classify by header, return unsupported, and leave all texture metadata/chunk parsing for Phase 6.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 05-ba2-gnrl-read-extract*
*Context gathered: 2026-05-08*
