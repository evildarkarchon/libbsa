# Phase 10: TES3 Write Support and BSA Format Completeness - Specification

**Created:** 2026-05-09
**Ambiguity score:** 0.10 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can create new TES3/Morrowind BSA archives from disk files or memory buffers with BSArchPro-compatible TES3 format layout, then prove the output through byte-level table checks and reader-backed extraction round trips.

## Background

Phase 4 completed TES3/Morrowind BSA read/extract support through `archive_reader`: TES3 byte detection, metadata listing, normalized lookup, strict hash/name validation, raw extraction, and conversion from TES3 data-section-relative file offsets to public archive-absolute `entry_metadata::payload_offset` values. Phase 7 completed TES4-family BSA write-new support through a public writer API, including disk and memory sources, duplicate canonical-path validation, overwrite behavior, round-trip tests, and byte-level table checks. Phase 8 and Phase 9 extended the public writer pattern for BA2 archives. Today there is no public TES3 writer API, no production TES3 BSA serializer, no TES3 writer tests, no committed TES3 writer-output fixtures, and WBSA-04 remains the only incomplete BSA write-support requirement.

## Requirements

1. **Public TES3 write-new API**: The library exposes consumer-facing write-new support for TES3/Morrowind BSA archives from disk files and memory buffers.
   - Current: `include/libbsa/writer.hpp` exposes TES4-family, BA2 GNRL, and BA2 DX10 writer APIs, but no TES3/Morrowind BSA writer capability exists.
   - Target: Consumers can add entries with explicit archive-internal paths from host files or copied caller memory, then write a new TES3 BSA archive through public libbsa APIs without using test fixture generators or private parser APIs.
   - Acceptance: A public-boundary test compiles against installed public headers and creates a TES3 writer, adds at least one disk-file entry and one memory-buffer entry, and calls `write_to` using only public libbsa types.

2. **TES3 format serialization**: The writer serializes valid TES3/Morrowind BSA structure with raw file payloads.
   - Current: `src/formats/bsa/tes3_bsa_parser.cpp` can parse TES3 headers, records, name offsets, name tables, hash records, and raw payload spans, but no production code can emit that structure.
   - Target: Writer output uses the TES3 version/header identity, file record table, name offset table, null-terminated name table, TES3 hash table, and raw data section expected by the existing TES3 reader.
   - Acceptance: A byte-level test inspects a writer-produced TES3 archive and verifies the header version, hash-table offset, file count, file sizes, name offsets, null-terminated names, hash records, and raw payload bytes are structurally valid.

3. **TES3 data-section-relative offsets**: The writer stores TES3 record offsets relative to the start of the data section, while reopened public metadata remains archive-absolute.
   - Current: Phase 4 reader code converts raw TES3 record offsets to archive-absolute metadata, but no writer computes or persists TES3 raw offsets.
   - Target: Each TES3 file record stores the raw data-section-relative offset required by the TES3 format, and reopening the archive through `archive_reader` exposes `payload_offset == data_section_start + raw_record_offset`.
   - Acceptance: Tests compare writer-produced raw record offsets against computed data-section-relative positions and assert reopened `entry_metadata::payload_offset` equals the archive-absolute payload position for every entry.

4. **TES3 hash ordering and path semantics**: The writer computes stored TES3 hashes from archive names and serializes records in TES3 hash sort order while preserving libbsa path behavior.
   - Current: `detail::hash_tes3`, `tes3_hash_sort_key`, TES3 parser validation, and normalized lookup already exist, but only fixture generators produce hash-sorted TES3 output.
   - Target: Writer output computes each stored hash using TES3 rules from the preserved archive path spelling, sorts records by TES3 low32/high32 hash order, preserves original path spelling with `/` separators in reopened metadata, and rejects invalid or duplicate canonical archive paths.
   - Acceptance: Tests add mixed-case and mixed-separator entries, inspect the serialized hash table order, reopen the archive, and verify `entries()`, `find()`, `contains()`, `original_path`, canonical paths, and stored hash metadata match expected TES3 behavior.

5. **Source ownership and write validation**: TES3 writer input and finalization behavior matches the established public writer expectations for disk sources, memory sources, and overwrite handling.
   - Current: TES4 and BA2 writers validate archive paths, copy memory entries into writer-owned state, report missing disk sources as structured errors, reject duplicate canonical paths, and refuse existing outputs unless overwrite is enabled; TES3 has none of this write-new behavior.
   - Target: TES3 writer copies memory payloads at add time, accepts explicit disk sources, rejects invalid archive paths, rejects duplicate canonical archive paths by finalization, reports missing disk sources, requires at least one entry, refuses default overwrite, and honors the explicit overwrite option.
   - Acceptance: Unit tests cover copied-memory mutation after `add_bytes`, missing disk source failure, invalid archive path failure, duplicate canonical path failure, empty archive failure, default overwrite rejection, and overwrite-enabled success with structured `result<void>` outcomes.

6. **Reader-backed round-trip validation**: Writer output is validated through the existing public reader and extraction APIs, not by trusting writer internals.
   - Current: TES3 read/extract tests prove generated reader fixtures, and TES4 writer tests prove TES4 output by reopening archives, but no TES3 writer output is round-tripped.
   - Target: TES3 archives produced from disk and memory sources reopen through `archive_reader::open`, report TES3 metadata, list all entries once, support lookup variants, and extract source-equivalent bytes through both sink and byte-vector extraction APIs.
   - Acceptance: CTest coverage packs a multi-entry TES3 archive, reopens it, verifies `archive_type::bsa`, `archive_variant::tes3`, version, file count, `entry_compression::none`, `entries()`, `find()`, `contains()`, `extract(path, sink)`, and `extract_bytes(path)` for every source entry.

7. **Committed TES3 writer fixtures**: Phase 10 adds legal committed TES3 writer-output fixture evidence and manifests.
   - Current: Committed TES3 generated fixtures exist for reader success and malformed cases, but there is no committed fixture or manifest proving production TES3 writer output.
   - Target: The repository contains generated TES3 writer fixture data and manifest evidence produced from repository-owned synthetic bytes, documenting source entries, canonical/original paths, stored hashes, raw TES3 data offsets, archive-absolute payload offsets, and expected payload bytes.
   - Acceptance: Tests can validate or regenerate the TES3 writer fixture evidence from repository-owned code/data, and no test requires local Morrowind archives, copyrighted game bytes, or writes under `TES5Edit/`.

8. **BSA format completeness regression**: Completing TES3 write support does not regress existing BSA read/write coverage or public dependency boundaries.
   - Current: TES3 read/extract and TES4-family BSA write-new support have focused tests, but the BSA family remains incomplete until TES3 writing is added.
   - Target: WBSA-04 is satisfied while TES3 reader, TES4-family writer, public include boundary, and TES5Edit read-only guarantees remain intact.
   - Acceptance: Phase verification runs TES3 writer tests, TES3 reader regression tests, TES4 BSA writer regression tests, public include boundary tests, and `git -C TES5Edit status --short`; all pass with no TES5Edit changes.

## Boundaries

**In scope:**
- Public TES3/Morrowind BSA write-new capability for disk-file entries and memory-buffer entries.
- TES3 raw/uncompressed archive serialization using TES3 header, file records, name offsets, name table, hash table, and data section layout.
- TES3 data-section-relative record offsets with reader-visible archive-absolute metadata after reopen.
- TES3 hash calculation and low32/high32 hash sort order for serialized records.
- Archive path validation, duplicate canonical-path rejection, copied-memory ownership, missing disk source errors, empty archive errors, default overwrite rejection, and explicit overwrite success.
- Committed legal TES3 writer fixture data and manifest evidence from repository-owned synthetic bytes.
- Reader-backed pack/reopen/list/find/contains/extract/byte-compare tests for TES3 writer output.
- Byte-level tests for TES3 writer headers, tables, hash order, name offsets, raw offsets, and payload bytes.
- Regression checks proving existing TES3 reader, TES4-family BSA writer, public include boundary, and TES5Edit read-only behavior remain intact.

**Out of scope:**
- TES3 compression, embedded file-name payload prefixes, or per-entry compression override behavior - TES3 writer output is raw-only for this phase.
- TES3 payload deduplication or shared non-empty payload regions - the current TES3 reader treats overlapping non-empty payload spans as malformed, and this phase targets canonical one-entry-to-one-payload writer output.
- TES4-family or BA2 writer feature expansion - those writer families are already covered by earlier phases and only need regression coverage here.
- Compatibility warning APIs, standalone validation APIs, malformed-input hardening framework, and BSArchPro-derived warning catalogs - Phase 11 owns compatibility warnings and hardening.
- CLI, GUI, or broad BSArchPro tool-option parity - libbsa is a reusable library, and this phase locks TES3 archive output format parity only.
- Mandatory local Morrowind install validation, real-game archive fixtures, or committed copyrighted game bytes - generated legal fixtures are the required acceptance evidence.
- In-place mutation of existing archives - v1 requires write-new flows until offset and validation behavior is proven across formats.
- Parallel packing, bounded-memory performance guarantees beyond existing writer conventions, benchmarks, and thread-safety documentation - Phase 12 owns performance and concurrency.
- Editing, formatting, compiling into, staging, or using `TES5Edit/` as a fixture workspace - the submodule remains read-only reference material.

## Constraints

- Public APIs must remain C++20-compatible and must not expose `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit symbols, private parser types, or private hash/compression implementation names.
- TES3 writer output must be raw/uncompressed; `entry_metadata::compression` after reopen must be `entry_compression::none` for every TES3 writer-produced entry.
- TES3 archive-internal paths remain normalized archive virtual paths, not host `std::filesystem::path` values.
- TES3 file record offsets must be stored as data-section-relative `uint32_t` values, and writer arithmetic must fail with structured errors when TES3 table or payload sizes cannot be represented safely.
- Generated and committed fixture evidence must be repository-owned synthetic data and must not depend on local game installations for normal CI/test success.
- `TES5Edit/` must remain untouched: no edits, formatting, generated files, fixture workspace use, staging, or submodule pointer changes.

## Acceptance Criteria

- [ ] Public include boundary tests compile a TES3 writer contract that adds disk and memory entries and writes an archive using only public libbsa types.
- [ ] A writer-produced TES3 archive reopens through `archive_reader::open` and reports `archive_type::bsa`, `archive_variant::tes3`, version `0x00000100`, expected file count, and `entry_compression::none`.
- [ ] Writer-produced TES3 file records store data-section-relative raw offsets, and reopened public `payload_offset` values equal `data_section_start + raw_record_offset` for every entry.
- [ ] Serialized TES3 hash records equal `detail::hash_tes3` for the preserved archive names and are sorted by TES3 low32/high32 sort order.
- [ ] `entries()`, `find()`, and `contains()` work for TES3 writer output with canonical paths, original path spelling, mixed-case lookup, separator variants, valid missing paths, and invalid path syntax.
- [ ] `extract(path, sink)` and `extract_bytes(path)` byte-compare every TES3 writer-output entry against the original disk or memory source bytes, including a zero-byte entry.
- [ ] TES3 writer validation tests cover copied-memory ownership, invalid archive paths, duplicate canonical paths, missing disk sources, empty archive finalization, default overwrite rejection, and overwrite-enabled success.
- [ ] Committed TES3 writer fixture data and manifest evidence are generated or validated from repository-owned synthetic bytes with no local game archive dependency.
- [ ] TES3 writer implementation exposes no compression, embedded-name, per-entry compression override, or payload deduplication requirement for TES3 output.
- [ ] TES3 reader regression, TES4 BSA writer regression, public include boundary tests, and `git -C TES5Edit status --short` pass after Phase 10 work.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.94  | 0.75  | met    | Public TES3 write-new API, TES3 format parity, and reader-backed output proof are locked. |
| Boundary Clarity    | 0.88  | 0.70  | met    | Raw-only TES3 writing is in scope; compression, embedded names, dedupe, validation APIs, tooling, performance, and in-place mutation are out. |
| Constraint Clarity  | 0.88  | 0.65  | met    | TES3 data-section-relative offsets, hash sort order, public API boundary, legal fixtures, and TES5Edit read-only constraints are explicit. |
| Acceptance Criteria | 0.88  | 0.70  | met    | Pass/fail public API, byte-layout, round-trip, validation, fixture, regression, and boundary checks are specified. |
| **Ambiguity**       | 0.10  | <=0.20| met    | Gate passed after round 3. |

Status: met = meets minimum, below = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What is the primary user-visible deliverable? | Phase 10 requires a public TES3 writer API. |
| 1 | Researcher | Which writer expectations should TES3 inherit? | Disk sources, copied memory sources, explicit archive paths, result-style failures, duplicate validation, and overwrite behavior are required. |
| 1 | Researcher | What fixture evidence is required? | Reader-backed pack/reopen/list/find/contains/extract/byte-compare validation is required. |
| 2 | Researcher + Simplifier | What is the irreducible scope if optional features are cut? | The user requested BSArchPro parity; this was narrowed in round 3 to TES3 archive format parity rather than tool parity. |
| 2 | Researcher + Simplifier | Should tests prove TES3 table layout beyond round-trip extraction? | Byte-level table proof is required. |
| 2 | Researcher + Simplifier | Should fixture evidence be committed or temporary only? | Committed TES3 writer fixture data and manifest evidence are required. |
| 3 | Boundary Keeper | What does BSArchPro parity mean for this phase? | TES3 format parity is required; broad BSArchPro UI/CLI/tool-option parity is not. |
| 3 | Boundary Keeper | Which adjacent features are explicitly out of scope? | CLI/GUI tooling, compatibility warning/validation APIs, malformed hardening framework, performance/concurrency work, and in-place mutation are excluded. |
| 3 | Boundary Keeper | Should TES3 writer support compression or embedded names? | TES3 writer output is raw-only; compression, embedded names, and per-entry compression overrides are not required. |

---

*Phase: 10-tes3-write-support-and-bsa-format-completeness*
*Spec created: 2026-05-09*
*Next step: /gsd-discuss-phase 10 - implementation decisions only*
