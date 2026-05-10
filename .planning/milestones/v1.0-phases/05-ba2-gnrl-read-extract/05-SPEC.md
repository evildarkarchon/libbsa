# Phase 5: BA2 GNRL Read/Extract - Specification

**Created:** 2026-05-08
**Ambiguity score:** 0.17 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can open, inspect, list, query, and extract Fallout 4 BA2 GNRL, Starfield BA2 v2 GNRL, and structurally valid Starfield BA2 v3 GNRL archives through the existing `archive_reader` surface with correct raw, deflate, and Starfield raw-LZ4-block compression routing.

## Background

The public API already exposes `archive_type::ba2`, `archive_variant::fallout4`, `archive_variant::starfield`, `entry_compression::lz4_block`, archive-level and entry-level metadata shapes, canonical path lookup, and sink-based extraction. Internal services already provide checked little-endian binary reads, archive path normalization, FO4/BA2 hashing, exact-size deflate decompression, and exact-size raw LZ4 block decompression. No BA2 detector, BA2 GNRL parser, BA2 filename-table parser, BA2 fixture generator, BA2 reader dispatch, or BA2 extraction path exists today. Phase 5 fills that gap for general BA2 archives only; BA2 DX10/DDS texture reconstruction remains a later phase.

## Requirements

1. **BA2 GNRL detection and open**: `archive_reader::open` accepts Fallout 4 BA2 GNRL, Starfield BA2 v2 GNRL, and structurally valid Starfield BA2 v3 GNRL archives based on archive bytes rather than file extension.
   - Current: Detection is implemented only for TES3 and TES4-family BSA bytes; BA2 bytes return unsupported.
   - Target: BA2 GNRL archives with `BTDX` container identity and GNRL archive type are routed to BA2 parsing, with metadata reporting `archive_type::ba2`, `archive_variant::fallout4` or `archive_variant::starfield`, version, file count, flags, and default compression behavior.
   - Acceptance: Generated FO4 GNRL, Starfield v2 GNRL, and Starfield v3 GNRL fixtures all open successfully through `archive_reader::open`; a non-BA2 magic fixture still returns `error_code::unsupported`.

2. **Filename table parsing**: BA2 names are parsed from the length-prefixed filename table at `FileTableOffset`.
   - Current: No BA2 filename-table reader exists; existing BSA readers derive names from BSA-specific folder/file tables or TES3 name tables.
   - Target: BA2 GNRL entries receive canonical `path` values and preserved `original_path` values from the filename table, independent of host filesystem separators.
   - Acceptance: Generated fixtures with multiple differently cased and nested names list the expected canonical paths in deterministic order, and `original_path` preserves archive-derived spelling with `/` separators.

3. **BA2 lookup and metadata**: Consumers can list entries, test containment, find entries by normalized archive path, and inspect BA2 GNRL entry metadata.
   - Current: `entries`, `contains`, and `find` dispatch only to TES3 or TES4-family BSA helpers.
   - Target: The same public reader operations work for BA2 GNRL entries and report raw size, stored size, archive-absolute payload offset, FO4/BA2 hash, compression mode, record flags, and non-embedded-name state.
   - Acceptance: Unit tests prove `entries`, `contains`, and `find` work for generated BA2 GNRL paths with mixed separators/case and return missing valid paths as empty optional rather than errors.

4. **Raw versus compressed classification**: BA2 GNRL records distinguish uncompressed entries from compressed entries using `PackedSize` and format/version metadata.
   - Current: Existing BSA compression decisions are based on BSA version and flags; there is no BA2 `PackedSize` classification.
   - Target: Entries with no packed payload are exposed as `entry_compression::none`; compressed entries are exposed as `entry_compression::deflate` or `entry_compression::lz4_block` according to BA2 version and Starfield v3 `CompressionMethod`.
   - Acceptance: Generated raw, deflate-compressed, and v3 raw-LZ4-block fixtures report the expected `entry_compression`, `raw_size`, `stored_size`, and `payload_offset` for every entry.

5. **Starfield version-specific metadata**: Starfield BA2 v2 and v3 version-specific header fields are inspectable or preserved through library-owned metadata without leaking internal parser structs.
   - Current: Public metadata has only generic archive and entry fields; no Starfield BA2 metadata is parsed or retained.
   - Target: Starfield v2 `Unknown1` and `Unknown2`, and Starfield v3 `CompressionMethod`, are available for consumer inspection or preserved in a documented library-owned metadata surface appropriate for BA2 archives.
   - Acceptance: Generated Starfield v2/v3 fixtures with non-default field values can be opened and their `Unknown1`, `Unknown2`, and `CompressionMethod` values can be verified through public or documented library-owned metadata.

6. **Raw and deflate extraction**: BA2 GNRL entries stored raw or deflate-compressed extract to exactly the bytes described by entry metadata.
   - Current: Raw and deflate extraction paths exist only for BSA variants; no BA2 extraction routing exists.
   - Target: BA2 GNRL extraction reads the selected payload span, routes raw and deflate entries by explicit metadata, validates exact decompressed size, and writes through caller-provided `payload_sink`.
   - Acceptance: Generated FO4 and Starfield BA2 GNRL fixtures extract raw and deflate entries through both `extract(path, sink)` and `extract_bytes(path)`, and extracted bytes exactly match fixture manifests.

7. **Starfield v3 raw LZ4 block extraction**: Starfield BA2 v3 GNRL entries with `CompressionMethod == 3` extract through the raw LZ4 block codec.
   - Current: Raw LZ4 block decompression exists internally, but no archive parser routes BA2 v3 payloads to it.
   - Target: Starfield BA2 v3 GNRL records with `CompressionMethod == 3` use `entry_compression::lz4_block`; other TES5Edit/BSArchPro-relevant v3 GNRL compression behavior is handled according to reference evidence, with exact-size validation.
   - Acceptance: A generated v3 fixture containing at least one raw-LZ4-block entry extracts to expected bytes, and a generated v3 fixture for the non-LZ4 observed compression path extracts through the expected non-LZ4 route.

8. **Legal fixture-backed validation**: Phase 5 support is validated by generated, repository-owned BA2 GNRL fixtures and manifests rather than copyrighted game archives.
   - Current: Generated fixture infrastructure exists for TES4-family BSA and TES3 BSA only; no generated BA2 fixtures or BA2 manifests exist.
   - Target: Maintainers can generate small legal BA2 GNRL fixtures covering FO4, Starfield v2, Starfield v3, filename tables, raw entries, deflate entries, raw-LZ4-block entries, and Starfield version fields.
   - Acceptance: CTest builds and uses generated BA2 GNRL fixtures automatically for Phase 5 unit/integration tests; optional local game fixtures are not required for Phase 5 acceptance.

## Boundaries

**In scope:**
- BA2 GNRL byte detection and open routing for Fallout 4, Starfield v2, and structurally valid Starfield v3 archives.
- BA2 GNRL header, record, and length-prefixed filename table parsing.
- Deterministic listing, normalized path lookup, containment checks, and entry metadata for BA2 GNRL archives.
- Public or documented library-owned inspection of Starfield v2 `Unknown1` and `Unknown2`, plus Starfield v3 `CompressionMethod`.
- Extraction of BA2 GNRL raw, deflate-compressed, and Starfield v3 raw-LZ4-block-compressed entries.
- TES5Edit/BSArchPro-relevant Starfield v3 GNRL quirks that affect the required read/extract behaviors.
- Generated legal BA2 GNRL fixtures and manifests sufficient to validate required behavior without copyrighted game archives.
- Regression coverage proving BSA readers, public include boundaries, and the TES5Edit read-only boundary remain intact.

**Out of scope:**
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

## Constraints

- `TES5Edit/` is read-only reference material and must not be edited, formatted, compiled into libbsa, staged, or used as a mutable fixture workspace.
- Public APIs remain C++20-compatible and must not expose `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, or internal parser/codec types.
- BA2 format detection must be byte-driven and must not rely on host file extensions.
- BA2 archive paths remain archive virtual paths, not `std::filesystem::path` values.
- Compression routing must be selected from parsed BA2 metadata, not inferred from file extension.
- Deflate and raw LZ4 block extraction must validate exact decompressed output size against archive metadata.
- The Starfield v3 raw LZ4 path must use the raw LZ4 block API, not the LZ4 frame API.
- Reader state must not retain whole archive bytes; extraction should preserve the existing sink-first public contract for selected entries.
- Fixture evidence must be generated and legal to commit; optional local game fixture checks cannot be mandatory for acceptance.
- Starfield v3 quirk coverage is bounded to TES5Edit/BSArchPro-relevant behavior affecting GNRL read/extract support.

## Acceptance Criteria

- [ ] `archive_reader::open` succeeds for generated Fallout 4 BA2 GNRL, Starfield BA2 v2 GNRL, and Starfield BA2 v3 GNRL fixtures.
- [ ] BA2 GNRL archives report `archive_type::ba2`, the correct public variant, version, file count, and default compression behavior.
- [ ] BA2 names are parsed from the `FileTableOffset` length-prefixed filename table and listed as deterministic canonical archive paths.
- [ ] `entries`, `find`, and `contains` work for BA2 GNRL paths using normalized archive virtual path semantics.
- [ ] BA2 entry metadata reports raw size, stored size, archive-absolute payload offset, FO4/BA2 hash, record flags, compression mode, and non-embedded-name state.
- [ ] Starfield v2 `Unknown1` and `Unknown2`, and Starfield v3 `CompressionMethod`, are inspectable or preserved through library-owned metadata.
- [ ] Raw BA2 GNRL entries extract byte-for-byte through `extract(path, sink)` and `extract_bytes(path)`.
- [ ] Deflate BA2 GNRL entries extract byte-for-byte with exact decompressed size validation.
- [ ] Starfield v3 BA2 GNRL entries with `CompressionMethod == 3` extract byte-for-byte through raw LZ4 block decompression.
- [ ] Generated BA2 GNRL fixtures and manifests are legal, committed outside `TES5Edit/`, and sufficient for automated CTest validation.
- [ ] Existing TES3/TES4-family BSA reader tests and public include boundary tests remain green after BA2 GNRL support is added.
- [ ] `git -C TES5Edit status --short` produces no output after Phase 5 execution.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.93  | 0.75  | PASS   | All required variants and read/extract outcome locked. |
| Boundary Clarity    | 0.78  | 0.70  | PASS   | DDS, write-new, hardening, real archives, and open-ended quirk hunting excluded. |
| Constraint Clarity  | 0.74  | 0.65  | PASS   | TES5Edit boundary, generated fixtures, public API dependency limits, and metadata-driven compression locked. |
| Acceptance Criteria | 0.83  | 0.70  | PASS   | Pass/fail checks cover open, list, lookup, metadata, extraction, fixtures, regressions, and submodule cleanliness. |
| **Ambiguity**       | 0.17  | <=0.20| PASS   | Gate passed after round 2. |

Status: PASS = met minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Which archive variants are non-negotiable deliverables? | Fallout 4 GNRL, Starfield v2 GNRL, and structurally valid Starfield v3 GNRL are all required. |
| 1 | Researcher | What evidence is enough without copyrighted game archives? | Generated legal fixtures are sufficient required evidence for Phase 5 acceptance. |
| 1 | Researcher | What should v3 support promise for edge cases? | Starfield v3 should cover all observed quirks relevant to BA2 GNRL read/extract behavior. |
| 2 | Researcher + Simplifier | What source bounds observed Starfield v3 quirks? | TES5Edit/BSArchPro reference behavior bounds the required quirk set. |
| 2 | Simplifier | What is the irreducible core if scope must be cut? | Open, list, expose metadata, and extract raw/deflate/v3-LZ4 GNRL entries for all required variants. |
| 2 | Researcher | Which Starfield version fields must be visible or preserved? | Starfield v2 `Unknown1`/`Unknown2` and Starfield v3 `CompressionMethod` must be available through library-owned metadata. |

---

*Phase: 05-ba2-gnrl-read-extract*
*Spec created: 2026-05-08*
*Next step: /gsd-discuss-phase 5 - implementation decisions (how to build what's specified above)*
