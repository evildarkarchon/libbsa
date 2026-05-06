# Phase 6: BA2 GNRL Read and Extract - Specification

**Created:** 2026-05-06
**Ambiguity score:** 0.09 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can open, inspect, list, look up, and extract Fallout 4 and Starfield `GNRL` BA2 archives across supported versions through a BA2-specific public API, including raw, deflate, and Starfield v3 raw LZ4-block payloads.

## Background

The codebase already detects BA2 headers in `src/detect.cpp`, exposes BA2 archive identities through `include/libbsa/archive.hpp`, has FO4/BA2 hash vectors in `tests/path_hash_tests.cpp`, and routes BA2 deflate versus Starfield raw LZ4 block compression in `src/compression.cpp`. The existing read/extract implementation is BSA-specific: `include/libbsa/bsa.hpp` exposes `bsa_archive`, `open_bsa`, and `extract_bsa_entry`, while `src/bsa_reader.cpp` implements TES4-family and TES3 parsing and streaming extraction. No BA2 metadata parser, BA2-specific public read API, BA2 GNRL file table parser, or BA2 GNRL extraction path exists yet.

## Requirements

1. **BA2-specific read API**: Phase 6 provides public BA2 read/extract entry points analogous to the existing BSA API.
   - Current: Public read/extract APIs exist only for BSA-family archives in `include/libbsa/bsa.hpp`.
   - Target: Public headers expose a BA2 metadata object plus `open_ba2` and `extract_ba2_entry`-style entry points for BA2 archives without replacing or unifying the existing BSA API.
   - Acceptance: Public-header smoke coverage can include the BA2 header and compile code that opens a BA2 archive object and extracts one named entry through a caller-owned `byte_sink`.

2. **Fallout 4 GNRL variants**: Phase 6 opens, lists, inspects, looks up, and extracts Fallout 4 `BTDX` + `GNRL` BA2 versions 1, 7, and 8.
   - Current: BA2 detection recognizes FO4 GNRL v1/v7/v8, but there is no full table parser or extraction path.
   - Target: Each supported FO4 GNRL version produces normalized paths, entry metadata, and byte-identical extracted payloads for raw and deflate-compressed entries.
   - Acceptance: Generated fixtures for FO4 GNRL v1, v7, and v8 open successfully, list expected paths, return expected metadata, and extract expected bytes for at least one raw and one deflate entry where the format supports both.

3. **Starfield v2 GNRL**: Phase 6 opens, lists, inspects, looks up, and extracts Starfield BA2 GNRL v2 archives while preserving relevant additional header metadata.
   - Current: Detection reads the extended Starfield v2 header length, but parsed archive metadata does not exist.
   - Target: Starfield v2 GNRL archives parse with their additional header fields accounted for, and relevant fields are preserved in libbsa-owned metadata where the public model has a place for them.
   - Acceptance: A generated Starfield v2 GNRL fixture opens successfully, exposes expected summary data including version and file table offset, lists expected paths, and extracts expected bytes.

4. **Starfield v3 GNRL compression routing**: Phase 6 extracts Starfield BA2 GNRL v3 entries using the correct payload route from entry compression state and archive `CompressionMethod`.
   - Current: `resolve_payload_codec` can route Starfield BA2 `CompressionMethod == 3` to raw LZ4 block and otherwise to deflate when asked, but no BA2 reader supplies entry metadata to it.
   - Target: Starfield v3 GNRL extraction handles raw entries, deflate entries, and `CompressionMethod == 3` raw LZ4-block entries without using the LZ4 frame path.
   - Acceptance: Generated Starfield v3 fixtures prove raw, deflate/default, and `CompressionMethod == 3` LZ4-block entries extract to expected bytes, and a route-confusion test fails if LZ4 frame is used for BA2 raw blocks.

5. **File name table parsing**: Phase 6 parses BA2 file name tables located at `FileTableOffset` and associates each length-prefixed name with the matching entry record.
   - Current: Detection exposes `file_table_offset`, but no code consumes the BA2 name table.
   - Target: BA2 GNRL entry paths come from the length-prefixed file name table, are normalized through libbsa archive-path rules, and support deterministic listing and lookup.
   - Acceptance: A generated fixture with multiple nested names lists the exact normalized paths, `contains` succeeds for equivalent slash/case variants, and malformed/truncated name tables return a structured `malformed_archive` error.

6. **Entry metadata fidelity**: Phase 6 exposes validated BA2 GNRL entry offsets, sizes, hashes, and compression state through libbsa-owned metadata.
   - Current: `entry_metadata` has fields for offsets, sizes, hashes, and compression, but no BA2 reader populates them.
   - Target: BA2 GNRL entries populate absolute payload offsets, unpacked size, packed/stored size, FO4-style hash parts where representable, and entry compression state without exposing raw dependency or TES5Edit types.
   - Acceptance: Fixture tests assert expected metadata values for raw and compressed entries, including absolute offset range validation and packed-versus-unpacked size behavior.

7. **GNRL payload transparency**: Phase 6 treats files stored in `GNRL` archives as ordinary payload bytes regardless of filename extension, including `.dds` LUTs or other DDS files placed in GNRL archives.
   - Current: No BA2 extraction path exists, and Phase 7 separately covers `DX10` texture archive reconstruction.
   - Target: `GNRL` extraction does not reject entries because their archive path ends in `.dds`; payload bytes are extracted according to the GNRL record and compression metadata.
   - Acceptance: A generated GNRL fixture containing a `.dds`-named entry opens and extracts that entry as ordinary bytes without invoking `DX10` texture reconstruction behavior.

8. **Generated fixture acceptance corpus**: Phase 6 acceptance is proven through generated, committed test fixtures or generated fixture builders covering the required BA2 GNRL version and compression matrix.
   - Current: Existing tests cover detection, hashes, compression, and BSA reader fixtures, but not BA2 GNRL opening or extraction.
   - Target: The test suite includes focused BA2 GNRL fixtures for FO4 v1/v7/v8, Starfield v2, Starfield v3, file table parsing, malformed table handling, raw extraction, deflate extraction, and LZ4-block extraction.
   - Acceptance: `ctest` can run the BA2 reader tests locally without external archive corpora, BSArchPro execution, or mutation of `TES5Edit/`.

## Boundaries

**In scope:**
- Public BA2-specific read/extract API for BA2 archives.
- `BTDX` + `GNRL` parsing for Fallout 4 versions 1, 7, and 8.
- `BTDX` + `GNRL` parsing for Starfield versions 2 and 3.
- BA2 GNRL file name table parsing from `FileTableOffset`.
- Normalized path listing, lookup, and copied metadata inspection for BA2 GNRL entries.
- Raw, deflate, and Starfield v3 raw LZ4-block extraction through caller-owned sinks.
- Generated fixture coverage for supported BA2 GNRL versions, compression routes, name tables, metadata, and malformed inputs.
- `.dds` or other extension payloads stored inside GNRL archives as ordinary files.

**Out of scope:**
- BA2 `DX10` texture archive parsing and DDS header reconstruction - this is Phase 7.
- Rejecting GNRL entries by file extension - GNRL archives may legitimately contain `.dds` payloads such as LUTs.
- BA2 writers or archive creation - writer behavior belongs to Phase 10 after read paths are proven.
- Unified family-neutral archive API replacing BSA-specific and BA2-specific APIs - this phase locks BA2 behavior without redesigning existing BSA entry points.
- Safe disk extraction policies such as traversal prevention, overwrite handling, and partial directory cleanup - these are application/tooling concerns or later diagnostics work.
- Bulk extraction orchestration, multi-threaded extraction, and performance benchmarking - these belong to Phase 12 after correctness paths are established.
- BSArchPro corpus comparison as a phase gate - generated fixtures are sufficient for Phase 6; broader reference corpus comparison is Phase 11 validation work.
- Modifying, formatting, staging, or compiling any file under `TES5Edit/` - the submodule remains a read-only behavioral reference.

## Constraints

- Public headers must expose libbsa-owned C++20 types and must not leak libdeflate, LZ4, DirectXTex, platform, Delphi, or TES5Edit implementation details.
- BA2 payload range validation must use bounded random-access reads and must not require loading the whole archive image into memory.
- Starfield BA2 v3 raw LZ4 block handling must use the existing raw LZ4 block route, not the Skyrim SE/AE LZ4 frame route.
- Deflate payloads must use the existing libdeflate-backed codec path and validate the expected unpacked size before returning bytes.
- BA2 file names must be normalized as archive-virtual paths, not host filesystem paths.
- Generated fixtures are the required acceptance corpus for this phase; real archive and BSArchPro comparison gates are deferred to compatibility validation.
- `TES5Edit/` is read-only and must remain unmodified.

## Acceptance Criteria

- [ ] Public BA2 read/extract API compiles from public headers and does not require private headers or dependency headers.
- [ ] Generated FO4 GNRL v1, v7, and v8 fixtures open, list expected normalized paths, expose expected metadata, and extract expected payload bytes.
- [ ] Generated Starfield GNRL v2 fixture opens, preserves relevant summary metadata, lists expected paths, and extracts expected payload bytes.
- [ ] Generated Starfield GNRL v3 fixtures prove raw, deflate/default, and `CompressionMethod == 3` raw LZ4-block extraction routes.
- [ ] BA2 file name table parsing reads length-prefixed names from `FileTableOffset` and maps them to the correct entry records.
- [ ] Path lookup succeeds for normalized slash/case variants of BA2 GNRL names.
- [ ] Entry metadata includes validated absolute payload offsets, unpacked size, packed/stored size, hash data where representable, and compression state.
- [ ] A `.dds`-named file inside a GNRL fixture extracts as ordinary payload bytes without requiring DX10 texture reconstruction.
- [ ] Malformed BA2 GNRL headers, truncated records, impossible offsets, and truncated name tables return structured errors without crashes or unchecked allocations.
- [ ] `TES5Edit/` remains clean and unmodified after Phase 6 work.

## Ambiguity Report

| Dimension          | Score | Min   | Status | Notes |
|--------------------|-------|-------|--------|-------|
| Goal Clarity       | 0.95  | 0.75  | ✓      | BA2-specific GNRL open/list/lookup/extract locked. |
| Boundary Clarity   | 0.93  | 0.70  | ✓      | GNRL `.dds` payloads included; DX10 texture archives, writers, bulk/perf work excluded. |
| Constraint Clarity | 0.88  | 0.65  | ✓      | Compression routes, fixture corpus, streaming reads, and public-header boundaries specified. |
| Acceptance Criteria| 0.88  | 0.70  | ✓      | Version, compression, metadata, name-table, and malformed-input checks are pass/fail. |
| **Ambiguity**      | 0.09  | <=0.20| ✓      | Gate passed after round 3. |

Status: ✓ = met minimum, ⚠ = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Which user-facing shape should Phase 6 deliver? | Add a BA2-specific API analogous to the existing BSA API. |
| 1 | Researcher | Which BA2 GNRL variants are required? | FO4 v1/v7/v8 and Starfield v2/v3 are all required. |
| 1 | Researcher | What validation corpus gates acceptance? | Generated fixtures are sufficient for Phase 6 acceptance. |
| 2 | Researcher + Simplifier | What is the irreducible extraction behavior? | Raw, deflate, and Starfield v3 raw LZ4-block entries must extract correctly. |
| 2 | Researcher + Simplifier | How should DDS be treated? | `.dds` files in GNRL archives must not be restricted; they are ordinary payloads. |
| 2 | Researcher + Simplifier | What metadata must be exposed? | Offsets, hashes, sizes, and compression state/method must be exposed where representable. |
| 3 | Boundary Keeper | What is the exact DDS boundary? | GNRL `.dds` payloads are in scope; BA2 `DX10` texture archive reconstruction is Phase 7. |
| 3 | Boundary Keeper | What is explicitly out of scope? | BA2 writers, DX10 texture reconstruction, safe disk extraction policy, bulk/parallel extraction, and BSArchPro corpus comparison are out of scope. |

---

*Phase: 06-ba2-gnrl-read-and-extract*
*Spec created: 2026-05-06*
*Next step: /gsd-discuss-phase 6 - implementation decisions (how to build the BA2 reader and extraction path)*
