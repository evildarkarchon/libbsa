# Phase 04: tes3-bsa-read-extract - Context

**Gathered:** 2026-05-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 4 adds TES3/Morrowind BSA read support through the existing `archive_reader` API. Consumers must be able to open, list, query, and extract TES3 BSA archives while preserving TES3-specific table, hash, and data-section-relative offset behavior. This phase extends the established reader surface and fixture strategy from Phase 3; it does not add TES3 writing, BA2 support, CLI/tooling, bulk filesystem extraction, compression, or performance/concurrency work.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `04-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `04-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- TES3/Morrowind BSA byte detection for read/open behavior.
- TES3 archive metadata exposed through the existing public metadata shape.
- TES3 entry listing, normalized lookup, and contains behavior through `archive_reader`.
- TES3 raw extraction through `extract(path, sink)` and `extract_bytes(path)`.
- TES3 data-section-relative payload offset semantics.
- Known TES3 read/extract quirks identified during this phase's reference tracing and fixture design.
- Generated legal TES3 success fixture(s), manifests, and focused malformed fixture coverage.
- Tests proving TES3 behavior without local game archives and without mutating `TES5Edit/`.

**Out of scope (from SPEC.md):**
- TES3/Morrowind BSA writing - Phase 10 owns TES3 write-new support.
- TES4-family behavior changes except integration points required to dispatch TES3 safely - Phase 3 behavior is already verified and should not be reworked for scope expansion.
- BA2 GNRL or DDS parsing/extraction - Phases 5 and 6 own BA2 read support.
- Compression handling for TES3 entries - TES3 BSA read/extract scope is raw/uncompressed payloads.
- Filesystem-directory extraction convenience APIs - Phase 4 uses the existing sink and byte-vector extraction APIs only.
- CLI tooling, GUI tooling, progress callbacks, cancellation, parallel extraction, and performance benchmarking - later phases or v2 tooling own those concerns.
- Required real-game archive validation or committed copyrighted data - generated legal fixtures are the required acceptance evidence; local archives may be supplemental only.
- Required BSArchPro-generated golden output files - reference tracing informs compatibility, but generated fixture bytes are sufficient for this phase gate.

</spec_lock>

<decisions>
## Implementation Decisions

### Offset Metadata
- **D-01:** Public `entry_metadata::payload_offset` means archive-absolute byte offset for TES3 entries, consistent with TES4-family metadata and consumer expectations. The raw TES3 record offset remains data-section-relative and is not exposed as a second public field.
- **D-02:** Interpret the SPEC's `payload_offset` acceptance checks as public archive-absolute offset checks. Generated TES3 manifests must separately record the raw data-section-relative offset so tests prove the compatibility rule without expanding public metadata.
- **D-03:** If a TES3 fixture entry has raw offset `0`, public `payload_offset` should equal the computed data section start.
- **D-04:** The reader should convert raw TES3 offsets to absolute offsets during parsing/validation and should not retain raw offsets in runtime reader state after validation.
- **D-05:** Invalid TES3 payload spans, raw-offset plus data-section-base overflow, overlapping metadata/name/hash/data regions, and overlapping payload byte ranges fail during `archive_reader::open` with `error_code::format_error`.
- **D-06:** TES3 zero-byte entries are valid when their offset is within the data section; extraction returns an empty payload.
- **D-07:** Do not require TES3 raw file data to be alphabetically ordered, and do not require payload offsets to be sorted by entry/hash order. Any non-overlapping bounded payload order is acceptable.
- **D-08:** Add/update public documentation so `entry_metadata::payload_offset` is explicitly archive-absolute for all variants.
- **D-09:** Offset tests should read archive bytes at asserted absolute offsets for raw entries, not rely only on manifest equality or extraction success.
- **D-10:** Researcher/planner must trace TES5Edit/BSArchPro for the offset conversion behavior and cite both the trace location and public format docs in a code comment near the non-obvious conversion rule.

### Hash Metadata
- **D-11:** Rename the public `entry_metadata::tes4_hash` field to a neutral `archive_hash` field in Phase 4. This is a pre-v1 API cleanup; do not add a backward-compatibility duplicate unless a concrete consumer need appears.
- **D-12:** For TES3 entries, `archive_hash` exposes the stored hash table value from the archive bytes, not a recomputed replacement. Strict validation separately requires it to match the parsed archive name.
- **D-13:** TES3 fixture manifests should include both the single 64-bit hash value used by public metadata and the low/high 32-bit halves used to make TES3 sort-order expectations explicit.
- **D-14:** When validating or recomputing TES3 hashes, use the parsed archive name bytes/spelling with TES3 lower-byte rules as the source of truth, not the public canonical lookup key.

### Strict Hash Checks
- **D-15:** TES3 open must validate each stored hash against the parsed archive name and fail mismatches with `error_code::format_error`.
- **D-16:** TES3 hash collisions between different names fail open with `error_code::format_error`.
- **D-17:** TES3 hash/order-dependent records must be sorted by TES3 hash order. Unsorted hash table/order records are malformed and fail open.
- **D-18:** Mandatory malformed hash fixtures must cover stored hash mismatch, duplicate hash collision, and unsorted hash/order records.
- **D-19:** Tests for hash validation should assert stable error codes only, not diagnostic message text.
- **D-20:** Stored TES3 names containing uppercase ASCII are allowed if they pass path normalization and hash validation under TES3 lower-byte rules. Preserve archive spelling in `original_path` and normalize lookup independently.
- **D-21:** Stored TES3 names using `/` instead of `\` are allowed if they pass archive path normalization and hash validation. Reject malformed paths or hash mismatches, not safe separator variants.

### Fixture Shape
- **D-22:** Phase 4 success proof should use one rich committed TES3 success archive rather than a broad archive matrix.
- **D-23:** The rich success archive should contain representative Morrowind-like paths across folders/extensions, multiple entries, lookup variants, TES3 hashes, public absolute offsets, raw data-section-relative offset proof, and extraction payload bytes. Do not make the success archive edge-heavy with every tolerated oddity.
- **D-24:** Add a separate TES3-specific generated fixture tool/source while reusing shared helpers where sensible. Do not hand-author opaque fixture bytes without generator provenance.
- **D-25:** Mandatory malformed fixture coverage is the SPEC set plus the strict hash cases: truncated structures, invalid name spans, invalid payload spans, duplicate canonical paths, inconsistent counts/offsets, stored hash mismatch, hash collision, unsorted hash/order records, and a regression case that would fail if raw TES3 offsets were treated as archive-absolute.

### the agent's Discretion
- Planner may choose exact internal parser/source names, detector dispatch mechanics, and helper boundaries if the public API and validation decisions above are preserved.
- Planner may choose exact generated entry names and payload bytes for the representative TES3 fixture, as long as they prove the locked metadata, lookup, hash, and offset decisions.
- Planner may decide the exact wording of diagnostics and comments, but tests should assert stable error codes rather than exact messages.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/04-tes3-bsa-read-extract/04-SPEC.md` — Locked Phase 4 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` — Phase 4 goal, mapped requirements BSA-04 and BSA-08, success criteria, and downstream phase ordering.
- `.planning/REQUIREMENTS.md` — TES3 BSA read/extract requirements plus public API, fixture, compatibility, and validation constraints.

### Project Constraints
- `.planning/PROJECT.md` — Project purpose, key decisions, dependency policy, public API constraints, streaming goal, error model, and TES5Edit boundary.
- `.planning/STATE.md` — Current project state and recent decisions from Phases 1 through 3.
- `AGENTS.md` — Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` — Product goals, supported archive families, streaming goals, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/03-format-detection-and-tes4-family-bsa-read-extract/03-CONTEXT.md` — Public reader surface, path lookup, extraction, fixture, and test-only JSON decisions carried into Phase 4.
- `.planning/phases/02-binary-i-o-paths-hashes-and-compression-services/02-CONTEXT.md` — Internal path normalization, streaming, TES3 hash helper, and primitive-layer decisions carried into Phase 4.
- `.planning/phases/01-foundation-api-boundary-and-test-harness/01-CONTEXT.md` — Public result/error model, fixture policy, dependency boundary, and test label taxonomy carried forward.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` — Read-only behavioral reference for TES3 hash/archive behavior; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` — Read-only behavioral reference for BSA parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` — Read-only BSArchPro application reference for behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` — Read-only reference directory for BSArchPro-related behavior; do not edit or use as fixture workspace.

### External Format References
- `https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format` — Public TES3 BSA section layout, data-section-relative offsets, hash sorting, and hash calculation background.
- `https://mwse.github.io/MWSE/types/tes3archiveOffsetSizeData/` — MWSE-facing offset/size metadata wording for consumer-visible offset expectations.
- `https://docs.rs/bsa3-hash/latest/bsa3_hash/` — Independent TES3 hash representation as two 32-bit halves, useful for fixture manifest expectations.

### Current Codebase State
- `include/libbsa/archive.hpp` — Public `archive_reader`, `entry_metadata`, `payload_sink`, and archive metadata surface; Phase 4 renames `tes4_hash` to `archive_hash` and documents `payload_offset` as archive-absolute.
- `src/archive.cpp` — Current reader dispatch and extraction path; presently calls TES4-only detector/parser/reader helpers and seeks to public absolute `payload_offset`.
- `src/formats/bsa/bsa_format_detector.hpp` — Current BSA detector shape with `detected_bsa_format`; Phase 4 extends detection to TES3.
- `src/formats/bsa/bsa_format_detector.cpp` — Current detector recognizes only `BSA\0` TES4-family versions 103/104/105.
- `src/formats/bsa/tes4_bsa_parser.hpp` — Existing parsed archive result shape for metadata and deterministic entry values; useful analog for a TES3 parser result.
- `src/formats/bsa/tes4_bsa_reader.hpp` — Existing BSA reader helper shape for entries/find/contains/extract; useful analog or shared helper candidate for TES3 lookup/extraction.
- `src/detail/bethesda_hash.hpp` — Existing internal TES3 hash helper declared as `hash_tes3`.
- `src/detail/bethesda_hash.cpp` — Existing TES3 hash implementation and TES5Edit compatibility comment to reuse/verify for TES3 table validation.
- `CMakeLists.txt` — Library source registration and public header file set; Phase 4 adds TES3 parser/reader/detector sources without leaking private headers.
- `tests/CMakeLists.txt` — Existing Catch2/nlohmann-json test wiring and generated fixture tool targets; Phase 4 adds TES3 tests and separate fixture generator target.
- `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` — Existing generated fixture style, manifest patterns, provenance approach, and helper examples.
- `tests/unit/tes4_bsa_reader_tests.cpp` — Existing reader, metadata, lookup, extraction, malformed, and manifest-driven assertion patterns to mirror for TES3.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `libbsa::archive_reader` already exposes the public open/metadata/entries/find/contains/extract/extract_bytes API that TES3 must reuse.
- `entry_metadata` already carries canonical path, original path, sizes, payload offset, compression, record flags, embedded-name fields, and the current `tes4_hash` field that Phase 4 should rename to `archive_hash`.
- `detail::normalize_archive_path` already provides canonical lowercase `/` lookup semantics and invalid-path rejection for `find`, `contains`, and extraction.
- `detail::hash_tes3` already implements TES3 hashing with a TES5Edit compatibility comment and can be used for table validation and fixture generation.
- Existing TES4 reader helpers already implement deterministic entry copies, binary-search lookup over canonical sorted entries, sink-first extraction, partial-sink failure, and `extract_bytes` reuse.
- Existing generated fixture tooling already writes tiny committed BSA archives and JSON manifests parsed by tests through a test-only nlohmann-json dependency.

### Established Patterns
- Public headers stay under `include/libbsa/`; format parsers, codecs, and dependency-bearing code stay under `src/` or `src/detail/`.
- Public APIs use Doxygen-style `///` comments; non-obvious compatibility rules should be documented near implementation/tests.
- Tests assert stable `error_code` values, not diagnostic strings.
- Fixture policy requires committed generated fixtures to document generator/source recipe and legal provenance, and forbids using `TES5Edit/` as a fixture workspace.
- Catch2 tags become CTest labels through `catch_discover_tests(... ADD_TAGS_AS_LABELS)`.

### Integration Points
- Extend BSA detection so TES3 is recognized before TES4-family dispatch rejects non-`BSA\0` bytes.
- Add TES3 parser/reader internals alongside `src/formats/bsa/tes4_bsa_*` without adding TES3-specific public parser classes.
- Update `archive_reader::open`, `entries`, `find`, `contains`, and extraction dispatch so behavior routes by parsed archive variant rather than hard-coded TES4 helpers.
- Update public and test code from `tes4_hash` to `archive_hash`.
- Add TES3 fixture generation and tests to `tests/CMakeLists.txt` using existing `unit`, `fixture`, and `malformed` labels.

</code_context>

<specifics>
## Specific Ideas

- Public offset metadata should be consumer-friendly and archive-absolute; raw TES3 offsets are a fixture/test proof detail, not a runtime public field.
- The success fixture should be representative, not exhaustive: enough Morrowind-like paths and payloads to prove TES3 layout behavior without turning Phase 4 into broad compatibility hardening.
- Hash metadata should become format-neutral now because Phase 4 is the first point where the TES4-specific field name becomes visibly wrong.
- Strict validation is preferred for TES3 structural/hash inconsistencies, while harmless naming conventions such as uppercase stored names or `/` separators are tolerated if path normalization and hash validation pass.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 04-tes3-bsa-read-extract*
*Context gathered: 2026-05-08*
