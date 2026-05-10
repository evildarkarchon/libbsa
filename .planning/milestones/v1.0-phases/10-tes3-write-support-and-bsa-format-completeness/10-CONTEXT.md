# Phase 10: tes3-write-support-and-bsa-format-completeness - Context

**Gathered:** 2026-05-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 10 adds public write-new support for TES3/Morrowind BSA archives. Consumers add disk-file or copied memory entries with explicit archive-internal paths, libbsa serializes raw TES3 metadata tables, hash-sorted name records, data-section-relative file offsets, and raw payloads, then proves output through byte-level structure checks, committed legal writer fixture evidence, and reader-backed reopen/list/find/contains/extract round trips.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `10-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `10-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public TES3/Morrowind BSA write-new capability for disk-file entries and memory-buffer entries.
- TES3 raw/uncompressed archive serialization using TES3 header, file records, name offsets, name table, hash table, and data section layout.
- TES3 data-section-relative record offsets with reader-visible archive-absolute metadata after reopen.
- TES3 hash calculation and low32/high32 hash sort order for serialized records.
- Archive path validation, duplicate canonical-path rejection, copied-memory ownership, missing disk source errors, empty archive errors, default overwrite rejection, and explicit overwrite success.
- Committed legal TES3 writer fixture data and manifest evidence from repository-owned synthetic bytes.
- Reader-backed pack/reopen/list/find/contains/extract/byte-compare tests for TES3 writer output.
- Byte-level tests for TES3 writer headers, tables, hash order, name offsets, raw offsets, and payload bytes.
- Regression checks proving existing TES3 reader, TES4-family BSA writer, public include boundary, and TES5Edit read-only behavior remain intact.

**Out of scope (from SPEC.md):**
- TES3 compression, embedded file-name payload prefixes, or per-entry compression override behavior. TES3 writer output is raw-only for this phase.
- TES3 payload deduplication or shared non-empty payload regions.
- TES4-family or BA2 writer feature expansion beyond regression coverage.
- Compatibility warning APIs, standalone validation APIs, malformed-input hardening framework, and BSArchPro-derived warning catalogs. Phase 11 owns compatibility warnings and hardening.
- CLI, GUI, or broad BSArchPro tool-option parity.
- Mandatory local Morrowind install validation, real-game archive fixtures, or committed copyrighted game bytes.
- In-place mutation of existing archives.
- Parallel packing, bounded-memory performance guarantees beyond existing writer conventions, benchmarks, and thread-safety documentation. Phase 12 owns performance and concurrency.
- Editing, formatting, compiling into, staging, or using `TES5Edit/` as a fixture workspace.

</spec_lock>

<decisions>
## Implementation Decisions

### Public API Shape
- **D-01:** Expose a dedicated single-purpose `tes3_bsa_writer` public writer. Do not add a one-value TES3 target enum and do not start a generic BSA writer redesign in Phase 10.
- **D-02:** Add a minimal `tes3_bsa_writer_options` surface with `overwrite_existing` only. Do not add speculative public knobs for compression, embedded names, deduplication, raw flags, hashes, or future compatibility warnings.
- **D-03:** Mirror existing writer method names and lifecycle: `add_file`, `add_bytes`, and `write_to`.
- **D-04:** Public docs/comments should explicitly state that TES3 writer output is raw/uncompressed and intentionally has no compression, dedupe, or embedded-name controls in Phase 10.

### Disk Source Timing
- **D-05:** Disk-file TES3 entries remain path-backed until `write_to`, matching TES4 and BA2 GNRL writer behavior. Do not snapshot disk file bytes at `add_file`.
- **D-06:** `add_file` should immediately validate the archive path and non-empty host path only. Missing or unreadable disk sources are reported from `write_to` through structured `io_error` results.
- **D-07:** Keep `write_to` logically non-consuming like existing writers. Repeated `write_to` calls may reread path-backed disk sources each time; memory entries remain copied at add time.
- **D-08:** If any path-backed disk source is missing or unreadable during finalization, fail cleanly with no successfully published partial archive and clean temporary output state before returning.

### Fixture Proof Shape
- **D-09:** Commit one representative canonical TES3 writer-produced archive plus manifest as writer-output fixture evidence. Cover broader validation scenarios through runtime writer tests rather than committing a large fixture matrix.
- **D-10:** The committed writer-output fixture must be generated through the new public TES3 writer API, not solely through the existing private/test-only TES3 fixture serializer.
- **D-11:** The writer fixture manifest should record full layout facts: source kind, original path, canonical path, stored hash low/high values, raw TES3 data offset, archive-absolute payload offset, raw/stored sizes, and expected payload bytes in hex.
- **D-12:** Tests should assert byte-level structural facts for header fields, name offsets, name bytes, hash records, raw data offsets, and payload bytes plus reader-backed round trips. Do not require one full archive byte-for-byte golden comparison unless planner finds it necessary.

### Path Byte Policy
- **D-13:** Preserve caller-provided path case in serialized TES3 names and stored hash computation. Canonical lowercase paths are only for validation, lookup-key derivation, and duplicate detection.
- **D-14:** Allow any archive path accepted by the shared archive path validator, including root-level file names if the normalizer accepts them. TES3 has a flat name table and should not inherit TES4 folder-record restrictions.
- **D-15:** TES3 stored hashes are writer-owned. Callers provide paths only; the writer computes `detail::hash_tes3` from the serialized name bytes and sorts records by TES3 low32/high32 order.

### Carry-Forward Decisions
- **D-16:** Preserve dependency-light public headers: no public `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, private parser, private writer, private hash, or private compression types.
- **D-17:** Preserve existing writer validation style: expected caller-data and I/O failures return `libbsa::result` errors; tests assert stable `error_code` values, not diagnostic text.
- **D-18:** Preserve generated/legal fixture policy. All committed Phase 10 writer evidence must be repository-owned synthetic data outside `TES5Edit/`, and `TES5Edit/` must remain untouched.
- **D-19:** Preserve reader-backed validation as the acceptance oracle: produced TES3 archives must reopen through `archive_reader`, list/find/contains entries using normalized path semantics, and extract source-equivalent bytes through sink and byte-vector APIs.

### the agent's Discretion
- The exact separator byte policy is delegated to researcher/planner. Default to the existing writer policy of converting `\` to `/`, serializing that preserved spelling, and hashing the serialized bytes unless TES3 reference tracing proves caller separator bytes must be preserved exactly.
- Researcher/planner may choose exact private file layout, helper boundaries, CMake registration, and Catch2 test organization if public headers stay dependency-light and the decisions above remain intact.
- Planner may choose exact synthetic entry paths and payload bytes for the canonical fixture and runtime tests, provided disk source, memory source, zero-byte, mixed-case, mixed-separator, root-level if valid, hash sorting, data-section-relative offsets, and duplicate/invalid path behavior are covered.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-SPEC.md` - Locked Phase 10 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` - Phase 10 goal, mapped requirement WBSA-04, success criteria, and Phase 11/12 boundaries.
- `.planning/REQUIREMENTS.md` - BSA write requirement WBSA-04 plus public API, fixture, compatibility, validation, performance, and out-of-scope constraints.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, key decisions, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from prior phases.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, write-new scope, streaming goals, and BSArchPro compatibility expectations.

### Prior Phase Decisions
- `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md` - Latest writer API, add-time validation contrast, safe publish, generated fixture, and public dependency boundary decisions.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-CONTEXT.md` - BA2 writer object shape, disk/memory entry behavior, archive-level options, end-to-end writer validation, safe publish, and dedupe carry-forward context.
- `.planning/phases/07-tes4-family-bsa-write-new-support/07-CONTEXT.md` - Closest BSA writer API shape, copied memory entries, path-backed disk entries, host-path output, overwrite defaults, duplicate validation, stable errors, and reader-backed writer validation.
- `.planning/phases/04-tes3-bsa-read-extract/04-CONTEXT.md` - TES3 reader decisions for archive-absolute public offsets, strict duplicate-path behavior, generated fixture proof strategy, and variant-specific reader dispatch.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for TES3 payload offset semantics and archive handling; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for BSA parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for pack/write option behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit, format, stage, compile, or use as fixture workspace.

### Current Codebase State
- `include/libbsa/writer.hpp` - Existing public writer surfaces, Doxygen style, options structs, `add_file`/`add_bytes`/`write_to` naming, and dependency-light boundary to mirror for TES3.
- `include/libbsa/libbsa.hpp` - Umbrella public include that must expose the TES3 writer surface once added.
- `include/libbsa/archive.hpp` - Public archive metadata, `entry_metadata`, `entry_compression::none`, and reader facade used for writer-output validation.
- `src/archive.cpp` - Current variant dispatch to TES3 parser/reader and public list/find/contains/extract routing used as the round-trip oracle.
- `src/formats/bsa/tes3_bsa_parser.hpp` and `src/formats/bsa/tes3_bsa_parser.cpp` - Existing TES3 header/table/name/hash parsing, data-section-relative offset conversion, hash sort validation, path normalization, duplicate-path validation, and raw-only metadata materialization to mirror in writer output.
- `src/formats/bsa/tes3_bsa_reader.hpp` and `src/formats/bsa/tes3_bsa_reader.cpp` - Existing TES3 entries/find/contains/extraction helpers and bounded host-file payload streaming used for reader-backed validation.
- `src/formats/bsa/tes4_bsa_writer.cpp` and `src/formats/bsa/tes4_bsa_writer.hpp` - Existing BSA writer-owned state, memory-copy behavior, path-backed disk source behavior, validation, table serialization, and public `write_to` bridge patterns to adapt carefully.
- `src/detail/archive_path.hpp` and `src/detail/archive_path.cpp` - Shared archive virtual path normalization and validation for canonical keys and duplicate detection.
- `src/detail/bethesda_hash.hpp` and `src/detail/bethesda_hash.cpp` - Existing `hash_tes3` and `tes3_hash_sort_key` helpers required for TES3 stored hash computation and order.
- `src/detail/binary_io.hpp` and `src/detail/binary_io.cpp` - Checked little-endian binary read/write primitives for safe TES3 serialization.
- `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` - Existing test-only TES3 reader fixture serializer and manifest/provenance pattern. Useful as layout reference, but Phase 10 writer-output fixture evidence must prove the public writer.
- `tests/unit/tes3_bsa_reader_tests.cpp` - Existing TES3 metadata, entries, lookup, extraction, stored-hash, offset conversion, malformed, and stable error-code tests to preserve and mirror.
- `tests/unit/tes4_bsa_writer_tests.cpp` - Existing writer public API, copied memory, missing source, invalid path, overwrite, compression, dedupe, and reader-backed BSA writer assertion patterns to adapt for TES3 where in scope.
- `tests/unit/public_include_boundary_tests.cpp` - Public dependency boundary tests that must stay green after adding TES3 writer APIs.
- `tests/CMakeLists.txt` - Catch2, generated fixture, and CTest label wiring for adding TES3 writer tests and fixture generation/validation.
- `CMakeLists.txt` - Library public header file set and private source registration for adding TES3 writer implementation files.

### External Format References
- `https://en.uesp.net/wiki/Morrowind_Mod:BSA_File_Format` - Independent TES3/Morrowind BSA format notes useful for cross-checking TES3 header, offsets, names, and hash tables against TES5Edit tracing.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `include/libbsa/writer.hpp` already provides the public writer pattern Phase 10 should mirror: Doxygen-commented types, options structs, `add_file`, `add_bytes`, `write_to`, `std::string_view` host/archive paths, and `libbsa::result` failures.
- `tes4_bsa_writer` provides the closest BSA write-new implementation pattern for writer-owned state, memory-copy entries, path-backed disk entries, finalization validation, checked serialization, and reader-backed tests.
- `tes3_bsa_parser` already defines the TES3 structure writer output must satisfy: version `0x00000100`, 12-byte fixed header, 8-byte file records, 4-byte name offsets, null-terminated names, 8-byte hash records, data-section-relative raw offsets, and archive-absolute public metadata after reopen.
- `detail::hash_tes3` and `detail::tes3_hash_sort_key` already encode the required hash and low32/high32 sort behavior.
- `generate_tes3_bsa_fixtures.cpp` has a compact known-good TES3 layout builder and manifest style that can guide tests, but production writer behavior should live outside fixture-only generator code.

### Established Patterns
- Public APIs live under `include/libbsa/` and must remain C++20/dependency-light. Private format writers belong under `src/formats/bsa/` or adjacent internal helpers.
- Public methods added or substantially rewritten need Doxygen-compliant comments.
- Memory entries are copied at add time; disk entries for raw archive writers are path-backed and fail during finalization if missing.
- Duplicate canonical archive paths are rejected by finalization, not by last-wins behavior.
- Tests assert stable `error_code` values and public metadata/bytes, not diagnostic string text.
- Generated fixtures and manifests must use repository-owned synthetic data outside `TES5Edit/`.

### Integration Points
- Add `tes3_bsa_writer_options` and `tes3_bsa_writer` to `include/libbsa/writer.hpp`, then ensure `include/libbsa/libbsa.hpp` exposes it through the existing public include path.
- Add private TES3 writer entry state and serializer under `src/formats/bsa/`, registered in `CMakeLists.txt` without exposing private hash/parser types publicly.
- Extend tests with public-boundary compile coverage, TES3 writer unit tests, committed writer fixture generation/validation, byte-level table assertions, reader-backed round trips, and regressions for existing TES3 reader and TES4 BSA writer suites.
- Keep `TES5Edit/` untouched and include `git -C TES5Edit status --short` in final verification as required by `10-SPEC.md`.

</code_context>

<specifics>
## Specific Ideas

- Public writer shape should feel like the existing writers but be visibly smaller: `tes3_bsa_writer_options{ overwrite_existing }`, `tes3_bsa_writer`, `add_file`, `add_bytes`, and `write_to`.
- Disk source timing intentionally follows TES4/BA2 GNRL rather than BA2 DX10: DDS needed add-time analysis/snapshotting, while TES3 raw payloads do not.
- The committed canonical writer fixture should be enough to prove production writer bytes exist in the repo, while runtime tests carry most of the matrix burden.
- Separator handling is delegated: prefer existing slash-normalized serialization/hashing unless reference tracing shows TES3 needs exact caller separator bytes.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 10-tes3-write-support-and-bsa-format-completeness*
*Context gathered: 2026-05-09*
