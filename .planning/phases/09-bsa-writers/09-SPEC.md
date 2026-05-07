# Phase 9: BSA Writers - Specification

**Created:** 2026-05-07
**Ambiguity score:** 0.14 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can create TES3 Morrowind and TES4-family BSA archives from disk paths or in-memory buffers, then read back the written archives with matching paths, metadata, compression state, hashes, offsets, and payload bytes.

## Background

The codebase already reads TES3 and TES4-family BSA archives through `open_bsa` and `extract_bsa_entry`. TES4-family readers cover Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105, including folder/file tables, compression flag XOR behavior, deflate, LZ4-frame payloads, and embedded-name prefix extraction. TES3 readers cover Morrowind metadata, raw extraction, TES3 path hashes, and data-section-relative offsets. Phase 8 added generic writer foundations in `include/libbsa/writer.hpp` and `src/writer.cpp`: in-memory writer entries, deterministic planning, compression policy routing, optional deduplication, layout preview, streaming finalization to `byte_sink`, and generated `LBSW` harness read-back tests. Phase 8 intentionally does not emit production BSA bytes; Phase 9 closes that gap for BSA formats only.

## Requirements

1. **BSA writer surface**: Phase 9 exposes BSA creation behavior for every BSA variant in the roadmap.
   - Current: `plan_archive_write` and `finalize_archive_write` emit a generic `LBSW` harness stream, while public BSA APIs only open and extract existing archives.
   - Target: Consumers can request new archives for TES3 Morrowind, Oblivion TES4 v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105 using public libbsa-owned C++20 types.
   - Acceptance: Public-header smoke coverage creates at least one archive for each BSA variant without including private codec headers, platform headers, Delphi/TES5Edit types, or test-only harness declarations.

2. **Disk and memory inputs**: Phase 9 supports both filesystem-backed and in-memory archive entries.
   - Current: Phase 8 writer inputs are in-memory `writer_entry` payloads only; no disk-path packing source exists.
   - Target: Consumers can build BSA archives from in-memory buffers and from disk files whose archive paths are derived from caller-provided virtual paths or a root-relative disk mapping.
   - Acceptance: Tests create equivalent BSA archives from in-memory entries and from temporary disk files, then assert `open_bsa` reports the same normalized paths, sizes, compression states, and extracted payload bytes for both inputs.

3. **TES4-family table serialization**: Phase 9 writes valid TES4-family BSA headers and metadata tables.
   - Current: `src/bsa_reader.cpp` parses TES4-family headers, folder records, file records, name tables, hashes, and payload offsets, but no production writer serializes those structures.
   - Target: Written v103/v104/v105 BSA archives contain correct magic, version, flags, folder/file counts, folder records, file records, folder name blocks, file name blocks, folder hashes, file hashes, payload offsets, and table length fields for the target engine.
   - Acceptance: Generated writer tests inspect emitted bytes for each TES4-family variant and then reopen them through `open_bsa`, verifying expected summary fields, normalized paths, hashes, offsets, stored sizes, and file counts.

4. **TES4-family compression and embedded-name behavior**: Phase 9 writes TES4-family payloads with explicit compression and embedded-name decisions.
   - Current: Readers can extract raw, deflate, LZ4-frame, and embedded-name-prefixed BSA payloads; Phase 8 writer planning can resolve compression policy but does not serialize BSA-native compression flags or embedded-name prefixes.
   - Target: Written TES4 v103/v104 archives use deflate when entries resolve to compressed storage, written v105 archives use LZ4-frame compression, archive-default and per-file override flags follow the existing reader's XOR model, and embedded-name behavior is explicit rather than accidental.
   - Acceptance: Tests write raw, archive-default-compressed, force-compressed, and force-raw entries for v103/v104/v105 archives; read-back metadata reports the expected `compression_state`, extraction returns original bytes, and embedded-name-enabled output sets `ARCHIVE_EMBEDNAME` and extracts after skipping the prefix.

5. **TES3 BSA serialization**: Phase 9 writes valid Morrowind TES3 BSA archives.
   - Current: `open_tes3_bsa` reads TES3 file records, name offsets, name blocks, hash tables, and data-section-relative offsets; no writer emits those structures.
   - Target: Written TES3 archives contain the TES3 magic/version field, correct file records, name offset table, null-terminated name block, TES3 hash table, TES3-compatible hash ordering, raw payloads, and data-section-relative offsets.
   - Acceptance: Generated TES3 writer tests emit multiple entries with known hash values, assert serialized record/hash order matches TES3-compatible hash sorting, reopen through `open_bsa`, and verify extracted payload bytes and absolute metadata offsets match the emitted data section.

6. **Round-trip verification**: Phase 9 proves written BSA archives through libbsa read-after-write, not by relying on the Phase 8 harness format.
   - Current: Phase 8 read-after-write tests parse only the test-only `LBSW` harness stream.
   - Target: Every production BSA writer path is verified by opening the generated bytes with `open_bsa`, comparing metadata to the write inputs, and extracting entries through `extract_bsa_entry`.
   - Acceptance: Focused writer tests cover at least one generated archive per required BSA variant and verify path list, entry lookup, metadata fields, compression state, stored size, hash fields, and round-trip payload bytes.

7. **Streaming finalization and layout safety**: Phase 9 preserves the Phase 8 writer safety model for production BSA bytes.
   - Current: Phase 8 plans and finalizes generated harness bytes through caller-owned `byte_sink` implementations with checked layout arithmetic and first-failure propagation.
   - Target: Production BSA writing finalizes through caller-owned sinks without requiring callers to receive a whole archive image, validates offset/size/count arithmetic before or during planning, and returns structured errors for sink write failures.
   - Acceptance: Tests finalize BSA output to `memory_sink` and to a failing custom sink; successful output size matches the planned/emitted size, and injected sink failure returns the original structured sink error without reporting success.

8. **Writer failures and project boundaries**: Phase 9 rejects invalid inputs and preserves established repository/API boundaries.
   - Current: Writer-core tests reject duplicate normalized paths, invalid paths, unsupported targets, unsupported compression routes, impossible layout arithmetic, and sink failure; BSA-specific writer failure behavior does not exist.
   - Target: BSA writer paths return structured failures for duplicate normalized archive paths, invalid archive paths, missing disk inputs, unsupported target/version combinations, unsupported compression requests, impossible BSA table/payload layout, and malformed embedded-name requests; `TES5Edit/` remains read-only.
   - Acceptance: Negative tests assert `result` failures with appropriate `error_code` values for each invalid-input class, public-header leakage grep over `include/libbsa` remains clean, and `git status --short TES5Edit` reports no changes.

## Boundaries

**In scope:**
- Production BSA byte serialization for TES3 Morrowind BSA archives.
- Production BSA byte serialization for TES4-family v103, v104, and v105 archives.
- Disk-file and in-memory input support for BSA creation.
- BSA-native folder/file table layout, name blocks, hashes, flags, counts, offsets, and payload regions.
- TES4-family raw, deflate, and LZ4-frame payload writing according to target version and compression policy.
- Explicit embedded-name writing behavior for TES4-family archives.
- TES3 hash sorting and data-section-relative offset serialization.
- Read-after-write tests through `open_bsa` and `extract_bsa_entry` for every required BSA variant.
- Structured BSA writer failure tests and public-header boundary checks.

**Out of scope:**
- Fallout 4 or Starfield BA2 GNRL/DDS writers - Phase 10 owns BA2 version headers, file tables, compression metadata, DDS analysis, and texture chunking.
- DirectXTex-backed DDS input analysis or texture optimization - Phase 10 owns BA2 DDS packing, and texture transcoding is outside v1 scope.
- Broad external compatibility corpus comparison against BSArchPro or official tools - Phase 11 owns cross-format corpus validation after production writers exist.
- Malformed-input hardening beyond writer-input validation needed for this phase - Phase 11 owns the dedicated validation and hardening sweep.
- Multi-threaded packing, parallel compression, cancellation, progress reporting, benchmarks, or bulk performance tuning - Phase 12 owns performance workflows.
- Productized CLI or GUI packing workflows - libbsa remains a reusable library, and application tooling is out of scope.
- True in-place mutation of existing archives - the project remains open-read-write-new only.
- Modifying, formatting, staging, compiling, or moving anything under `TES5Edit/` - the submodule remains read-only reference material.

## Constraints

- Public headers must expose only libbsa-owned C++20 types and must not leak libdeflate, LZ4, DirectXTex, Windows SDK, platform, Delphi, UI, or TES5Edit implementation details.
- Writer output must target BSA formats only: TES3 Morrowind, Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105.
- Disk input behavior must not treat archive virtual paths as host `std::filesystem::path` semantics; archive paths must still normalize through libbsa archive-path rules.
- TES4-family compression decisions must use archive format, archive-default compression, per-entry policy, and BSA-native flag semantics; codec choice must not be inferred from file extension.
- TES4 v103/v104 compressed payloads use deflate; TES4-family v105 compressed payloads use LZ4-frame compression.
- TES3 payloads are raw, use TES3 hash behavior, and store record offsets relative to the TES3 data section.
- Offset, size, count, name-length, and table-length arithmetic must be checked and return structured failures on impossible layouts.
- Finalization must use caller-owned `byte_sink` semantics and must not retain sink lifetimes after the call returns.
- Generated read-after-write fixtures are sufficient acceptance proof for Phase 9; broader external corpus validation is intentionally deferred to Phase 11.
- `TES5Edit/` is read-only and must remain unmodified.

## Acceptance Criteria

- [ ] Public-header smoke coverage creates BSA writer inputs and writes one TES3, v103, v104, and v105 BSA archive using only public libbsa headers.
- [ ] In-memory and disk-backed input tests produce read-back-equivalent BSA archives with matching normalized paths, sizes, compression states, and extracted payload bytes.
- [ ] TES4-family writer tests assert emitted magic, version, flags, counts, table lengths, folder/file hashes, offsets, and names for v103, v104, and v105 archives.
- [ ] TES4-family raw, archive-default-compressed, force-compressed, and force-raw entries reopen with expected `compression_state` and extract to the original payload bytes.
- [ ] TES4-family embedded-name-enabled output sets the embedded-name archive flag and extracts correctly after the reader skips the embedded prefix.
- [ ] TES3 writer tests assert TES3-compatible hash sorting, name offset table contents, hash table contents, raw payload storage, and data-section-relative offsets.
- [ ] Every generated BSA variant is reopened through `open_bsa`, inspected through `bsa_archive`, and extracted through `extract_bsa_entry` with payload bytes matching original inputs.
- [ ] Production BSA finalization writes through caller-owned `byte_sink`; successful `memory_sink` output size matches the emitted archive size, and a failing sink returns the original structured error.
- [ ] Negative tests cover duplicate normalized paths, invalid archive paths, missing disk inputs, unsupported BSA target/version combinations, unsupported compression requests, impossible layout arithmetic, and malformed embedded-name requests.
- [ ] Focused BSA writer tests, existing BSA reader tests, writer-core tests, public-header smoke tests, full CTest, public-header private-token grep, and `git status --short TES5Edit` pass.

## Ambiguity Report

| Dimension           | Score | Min    | Status | Notes |
|---------------------|-------|--------|--------|-------|
| Goal Clarity        | 0.92  | 0.75   | PASS   | All required BSA variants and disk/memory input expectations are locked. |
| Boundary Clarity    | 0.85  | 0.70   | PASS   | Phase is BSA-only; BA2, broad corpus validation, performance, CLI/GUI, and in-place mutation are excluded. |
| Constraint Clarity  | 0.79  | 0.65   | PASS   | Compression routing, BSA flags, TES3 offsets/hashes, sink ownership, and public-header boundaries are specified. |
| Acceptance Criteria | 0.82  | 0.70   | PASS   | Pass/fail checks cover variant output, disk/memory inputs, read-back, compression, embedded names, TES3 sorting, failures, and validation gates. |
| **Ambiguity**       | 0.14  | <=0.20 | PASS   | Gate passed after round 2. |

Status: PASS = met minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Which Phase 9 archive variants are required deliverables? | All BSA variants are required: TES3 Morrowind, Oblivion v103, FO3/FNV/Skyrim LE v104, and Skyrim SE/AE v105. |
| 1 | Researcher | What input source shape must Phase 9 guarantee? | Both disk paths and in-memory buffers are required for BSA creation. |
| 1 | Researcher | What compatibility proof counts before Phase 11? | Generated read-back is sufficient: libbsa opens generated BSA bytes, compares metadata, and round-trip extracts payloads. |
| 1 | Scoring | Ambiguity after round 1? | Goal 0.88, boundary 0.72, constraint 0.65, acceptance 0.74, ambiguity 0.23. |
| 2 | Researcher + Simplifier | What is the irreducible success condition? | Every required BSA variant must produce valid archives that read back through libbsa with matching metadata and payloads. |
| 2 | Researcher + Simplifier | Which TES4-family compatibility behaviors are mandatory? | Compression plus flags are mandatory: deflate/LZ4 routing, archive default and override flags, folder/file hashes, and explicit embedded-name behavior. |
| 2 | Researcher + Simplifier | Which boundary should Phase 9 preserve? | BSA only: no BA2 writers, DDS packing, external corpus validation, multithreading, CLI/GUI, or in-place mutation. |
| 2 | Gate | Ambiguity gate reached; proceed to write SPEC.md? | User selected "Yes, write SPEC.md". |

---

*Phase: 09-bsa-writers*
*Spec created: 2026-05-07*
*Next step: /gsd-discuss-phase 9 - implementation decisions (how to build the locked BSA writer requirements)*
