# Phase 10: BA2 Writers - Specification

**Created:** 2026-05-07
**Ambiguity score:** 0.13 (gate: <= 0.20)
**Requirements:** 9 locked

## Goal

Consumers can create Fallout 4 and Starfield BA2 GNRL and DDS archives from disk or memory inputs, then read back the written archives with matching paths, metadata, compression state, texture metadata, chunk layout, and payload bytes.

## Background

The codebase already reads and extracts BA2 GNRL archives through `open_ba2` and `extract_ba2_entry`, covering Fallout 4 GNRL v1/v7/v8 and Starfield GNRL v2/v3 with FileTableOffset name tables, version-specific headers, deflate payloads, raw payloads, and Starfield v3 method-3 raw LZ4-block routing. It also reads and extracts BA2 DDS / DX10 archives for Fallout 4 v1/v7/v8 and Starfield v3, exposing copied `texture_metadata`, chunk metadata, DXGI format values, mip counts, array/cubemap state, and DDS reconstruction validated behind a private DirectXTex boundary. Phase 8 added generic writer planning, compression routing, optional post-policy deduplication, layout preview, and streaming finalization foundations. Phase 9 added production BSA writer APIs and native BSA serialization. No production BA2 writer API or native BA2 GNRL/DDS serialization exists today.

## Requirements

1. **BA2 writer surface**: Phase 10 exposes BA2 creation behavior for every BA2 variant currently supported by the reader.
   - Current: Public BA2 APIs only open, inspect, and extract existing BA2 archives; `plan_archive_write` emits only the generic Phase 8 harness stream for BA2 targets.
   - Target: Consumers can request new BA2 GNRL archives for Fallout 4 v1/v7/v8 and Starfield v2/v3, and new BA2 DDS / DX10 archives for Fallout 4 v1/v7/v8 and Starfield v3, using public libbsa-owned C++20 types.
   - Acceptance: Public-header smoke coverage creates and finalizes at least one GNRL and one DDS archive for Fallout 4 and Starfield BA2 targets without including private codec, DirectXTex, platform, Delphi/TES5Edit, or test-only harness declarations.

2. **Disk and memory inputs**: Phase 10 supports both filesystem-backed and in-memory BA2 creation inputs.
   - Current: BSA writers support memory-backed and disk-backed entries, but BA2 writing has no production input surface. The generic writer core accepts only memory payloads and does not analyze DDS inputs.
   - Target: BA2 GNRL archives can be built from in-memory payloads and explicit disk-file mappings, and BA2 DDS archives can be built from in-memory DDS bytes and explicit disk DDS files whose archive paths are caller-provided archive-virtual paths.
   - Acceptance: Tests create read-back-equivalent BA2 GNRL archives from memory and temporary disk files, and create read-back-equivalent BA2 DDS archives from memory DDS bytes and temporary disk DDS files, with matching normalized paths, metadata, extracted bytes, and texture metadata.

3. **GNRL native serialization**: Phase 10 writes valid BA2 GNRL headers, record tables, name tables, and payload regions.
   - Current: `src/ba2_reader.cpp` parses GNRL headers, version-specific Starfield header tails, fixed 36-byte records, length-prefixed name tables at `FileTableOffset`, payload offsets, hashes, sizes, packed sizes, and compression metadata; no production writer serializes those structures.
   - Target: Written GNRL archives contain `BTDX` + `GNRL` identity bytes, the requested BA2 version, correct file count, version-specific header size and Starfield fields, fixed-size records, name and directory hashes, payload offsets, `PackedSize`/`Size` values, length-prefixed file name table, and payload bytes arranged so `FileTableOffset` and first payload offset are internally consistent.
   - Acceptance: Byte-level tests inspect emitted GNRL archives for v1, v2, v3, v7, and v8 headers and records, then reopen them through `open_ba2` and verify summary fields, path list, entry lookup, hash fields, offsets, stored sizes, compression state, and extracted payload bytes.

4. **DDS input analysis and DX10 serialization**: Phase 10 writes BA2 DDS / DX10 archives from actual DDS inputs, not caller-supplied texture descriptors.
   - Current: `texture_metadata` and generated DX10 fixture helpers model BA2 DDS metadata, and extraction reconstructs DDS output, but production writer code does not analyze source DDS bytes or emit DX10 texture records.
   - Target: BA2 DDS writing analyzes each DDS input through the internal texture-analysis boundary, derives dimensions, DXGI format, mip count, array/cubemap metadata, and mip payload ranges, then serializes DX10 texture records and chunk records for the requested Fallout 4 or Starfield DDS target.
   - Acceptance: Tests pack one-mip, multi-mip, cubemap, and array DDS inputs into BA2 DDS archives, reopen them through `open_ba2`, verify `texture_metadata` fields and chunk-to-mip summaries, extract each entry through `extract_ba2_entry`, and validate the reconstructed DDS bytes through the private DDS validation helper.

5. **Native compression routes**: Phase 10 supports every BA2 compression route already supported by readers and writer-core policy.
   - Current: BA2 readers and compression services can route raw, deflate, and Starfield v3 raw LZ4-block payloads, but no BA2 writer records native `PackedSize`, `Size`, or `CompressionMethod` choices.
   - Target: GNRL entries and DDS chunks support raw storage, Fallout 4 / Starfield method-0 deflate storage where entries or chunks resolve to compressed output, and Starfield v3 method-3 raw LZ4-block storage; unsupported nonzero Starfield v3 compression methods are rejected with structured errors.
   - Acceptance: Tests write raw, archive-default-compressed, force-compressed, and force-raw GNRL entries and DDS chunks for applicable targets, assert native packed/unpacked sizes and compression metadata, and verify extraction returns original payload or valid DDS bytes without codec fallback.

6. **BA2 deduplication**: Phase 10 implements opt-in deduplication for BA2 layouts where shared stored data is valid.
   - Current: Phase 8 can share identical post-policy stored payload bytes in generic write plans, and Phase 9 applies equivalent behavior to native BSA data regions; no BA2 writer dedup behavior exists.
   - Target: With dedup enabled, byte-identical post-policy GNRL payloads share one stored data region, and byte-identical post-policy DDS chunk payloads share one stored chunk data region where BA2 offsets can legally point to the same bytes; with dedup disabled, duplicates receive distinct payload regions.
   - Acceptance: Tests pack duplicate GNRL entries and duplicate DDS chunks with dedup enabled and disabled, assert shared versus distinct offsets as appropriate, reopen the archive, and verify every duplicate entry extracts to the correct original bytes.

7. **Read-after-write proof**: Phase 10 proves written BA2 archives through libbsa readers before broader external corpus validation.
   - Current: BA2 reader tests use generated fixtures, Phase 8 uses a generated harness stream, and Phase 9 proves BSA writers by reopening emitted BSA archives.
   - Target: Every production BA2 writer path is verified by opening generated BA2 bytes with `open_ba2`, comparing metadata to writer inputs, and extracting entries through `extract_ba2_entry`; DDS outputs must validate as DDS after extraction.
   - Acceptance: Focused BA2 writer tests cover at least one generated archive for each required GNRL and DDS target family, verify summary fields, path lookup, metadata fields, compression state, texture metadata where applicable, and round-trip payload or DDS bytes.

8. **Streaming finalization and layout safety**: Phase 10 preserves the established writer safety model for production BA2 bytes.
   - Current: Phase 8 and Phase 9 plans own table and payload bytes and finalize through caller-owned `byte_sink` objects with checked layout arithmetic and first-failure propagation; BA2 writers do not yet exist.
   - Target: BA2 writer planning validates offset, size, count, file-table, chunk-table, name-length, and payload layout arithmetic before successful finalization, and finalization streams planned BA2 bytes through a caller-owned sink without retaining the sink or caller input files after the call returns.
   - Acceptance: Tests finalize BA2 output to `memory_sink` and to a failing custom sink; successful output size matches the planned/emitted size, impossible layout fixtures fail with structured errors, and injected sink failure returns the original sink error without reporting success.

9. **Writer failures and project boundaries**: Phase 10 rejects invalid BA2 writer inputs and preserves established repository/API boundaries.
   - Current: BA2 readers reject malformed input archives, BSA writers reject invalid writer inputs, and public headers avoid private dependency leakage; BA2-specific writer failure behavior does not exist.
   - Target: BA2 writer paths return structured failures for duplicate normalized archive paths, invalid archive paths, missing disk inputs, malformed or unsupported DDS inputs, unsupported BA2 target/version combinations, unsupported Starfield compression methods, impossible layout arithmetic, and sink write failures; `TES5Edit/` remains read-only.
   - Acceptance: Negative tests assert `result` failures with appropriate `error_code` values for each invalid-input class, public-header leakage grep over `include/libbsa` remains clean, and `git status --short TES5Edit` reports no changes.

## Boundaries

**In scope:**
- Production BA2 GNRL byte serialization for Fallout 4 v1/v7/v8 and Starfield v2/v3 targets.
- Production BA2 DDS / DX10 byte serialization for Fallout 4 v1/v7/v8 and Starfield v3 targets.
- Public BA2 writer APIs using libbsa-owned C++20 types for explicit BA2 targets, memory inputs, disk inputs, writer options, plans, preview metadata, and finalization.
- Disk-file and in-memory input support for BA2 GNRL creation.
- Disk-file and in-memory DDS-byte input support for BA2 DDS creation.
- Internal DDS metadata analysis for dimensions, DXGI format, mip count, array/cubemap state, and mip payload ranges without leaking DirectXTex in public headers.
- BA2-native GNRL records, DX10 texture records, chunk records, length-prefixed file name tables, hashes, version fields, compression method fields, offsets, sizes, and payload regions.
- Raw, deflate, and Starfield v3 method-3 raw LZ4-block payload writing where applicable.
- Opt-in deduplication for identical post-policy GNRL payloads and DDS chunk payloads where BA2 layout permits shared offsets.
- Read-after-write tests through `open_ba2` and `extract_ba2_entry`, including DDS validation after extraction.
- Structured BA2 writer failure tests and public-header boundary checks.

**Out of scope:**
- External corpus comparison against BSArchPro or official Bethesda tools - Phase 11 owns broad compatibility corpus validation after BA2 writers exist.
- Malformed archive hardening beyond writer-input validation needed for this phase - Phase 11 owns the dedicated validation and hardening sweep.
- Texture transcoding, resizing, format conversion, or optimization - libbsa analyzes and packages DDS bytes but does not become a texture conversion tool.
- Console-only swizzled texture or proprietary XMem support - those are explicitly outside the portable PC-focused v1 scope.
- Multi-threaded packing, parallel compression, cancellation, progress reporting, benchmarks, or bulk performance tuning - Phase 12 owns performance workflows.
- Productized CLI or GUI packing workflows - libbsa remains a reusable library, and application tooling is out of scope.
- True in-place mutation of existing archives - the project remains open-read-write-new only.
- ZIP, 7z, or other non-Bethesda archive formats - project scope remains Bethesda BSA/BA2 only.
- Modifying, formatting, staging, compiling, or moving anything under `TES5Edit/` - the submodule remains read-only reference material.

## Constraints

- Public headers must expose only libbsa-owned C++20 types and must not leak libdeflate, LZ4, DirectXTex, Windows SDK, platform, Delphi, UI, or TES5Edit implementation details.
- Writer output must target BA2 formats only: Fallout 4 GNRL v1/v7/v8, Starfield GNRL v2/v3, Fallout 4 DDS/DX10 v1/v7/v8, and Starfield DDS/DX10 v3.
- Disk input behavior must not treat archive virtual paths as host `std::filesystem::path` semantics; archive paths must still normalize through libbsa archive-path rules.
- DDS writing must analyze actual DDS bytes through an internal texture-analysis boundary; public callers must not be required to provide native BA2 chunk descriptors to create supported DDS archives.
- Compression decisions must use archive format, target version, writer policy, and explicit Starfield `CompressionMethod`; codec choice must not be inferred from file extension.
- Starfield v3 `CompressionMethod` support is limited to method 0 and method 3; unsupported nonzero values must fail with structured errors.
- BA2 `FileTableOffset`, record-table length, name-table length, chunk-table length, payload offsets, packed sizes, unpacked sizes, and total archive size arithmetic must be checked and return structured failures on impossible layouts.
- Finalization must use caller-owned `byte_sink` semantics and must not retain sink, host file, or input-buffer lifetimes after public writer calls return.
- Generated read-after-write fixtures are sufficient acceptance proof for Phase 10; broader external corpus validation is intentionally deferred to Phase 11.
- `TES5Edit/` is read-only and must remain unmodified.

## Acceptance Criteria

- [ ] Public-header smoke coverage creates and finalizes one Fallout 4 BA2 GNRL archive, one Starfield BA2 GNRL archive, one Fallout 4 BA2 DDS archive, and one Starfield BA2 DDS archive using only public libbsa headers.
- [ ] In-memory and disk-backed BA2 GNRL input tests produce read-back-equivalent archives with matching normalized paths, hashes, sizes, compression states, and extracted payload bytes.
- [ ] In-memory DDS bytes and disk-backed DDS input tests produce read-back-equivalent BA2 DDS archives with matching normalized paths, texture metadata, chunk summaries, and validated extracted DDS bytes.
- [ ] GNRL writer tests assert emitted `BTDX`/`GNRL` identity, versions v1/v2/v3/v7/v8, header sizes, `FileTableOffset`, record fields, name table bytes, payload offsets, packed sizes, unpacked sizes, and compression method metadata.
- [ ] DDS writer tests assert emitted `BTDX`/`DX10` identity, versions v1/v3/v7/v8, texture records, chunk records, file name table bytes, DXGI format, dimensions, mip count, array/cubemap metadata, chunk-to-mip mapping, payload offsets, packed sizes, and unpacked sizes.
- [ ] Raw, deflate, and Starfield v3 method-3 raw LZ4-block BA2 entries/chunks reopen with expected `compression_state` and extract to the original payload bytes or validated DDS bytes.
- [ ] Unsupported Starfield v3 nonzero compression methods fail with structured errors and do not fall back to deflate or raw LZ4-block accidentally.
- [ ] Dedup-enabled BA2 GNRL entries and BA2 DDS chunks share identical post-policy stored payload offsets where permitted, while dedup-disabled output emits distinct regions for duplicate inputs.
- [ ] Every required BA2 target family is reopened through `open_ba2`, inspected through `ba2_archive`, and extracted through `extract_ba2_entry` with payload bytes or validated DDS bytes matching original inputs.
- [ ] Production BA2 finalization writes through caller-owned `byte_sink`; successful `memory_sink` output size matches the emitted archive size, and a failing sink returns the original structured error.
- [ ] Negative tests cover duplicate normalized paths, invalid archive paths, missing disk inputs, malformed or unsupported DDS inputs, unsupported BA2 target/version combinations, unsupported Starfield compression methods, impossible layout arithmetic, and sink write failures.
- [ ] Focused BA2 writer tests, existing BA2 reader tests, BA2 DDS reader tests, writer-core tests, public-header smoke tests, full CTest, public-header private-token grep, and `git status --short TES5Edit` pass.

## Ambiguity Report

| Dimension           | Score | Min    | Status | Notes |
|---------------------|-------|--------|--------|-------|
| Goal Clarity        | 0.93  | 0.75   | PASS   | Required GNRL/DDS variants, disk/memory inputs, DDS analysis, and read-back proof are locked. |
| Boundary Clarity    | 0.86  | 0.70   | PASS   | External corpus validation, broad hardening, texture transforms, performance, CLI/GUI, in-place mutation, and TES5Edit changes are excluded. |
| Constraint Clarity  | 0.84  | 0.65   | PASS   | Compression routes, Starfield methods, DirectXTex boundary, BA2 layout arithmetic, dedup scope, and public-header boundaries are specified. |
| Acceptance Criteria | 0.80  | 0.70   | PASS   | Pass/fail checks cover variants, input sources, byte layout, DDS metadata, compression, dedup, read-back, failures, and validation gates. |
| **Ambiguity**       | 0.13  | <=0.20 | PASS   | Gate passed after round 3. |

Status: PASS = met minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Which BA2 writer variants are required deliverables? | All reader variants are required: GNRL FO4 v1/v7/v8 and Starfield v2/v3; DDS/DX10 FO4 v1/v7/v8 and Starfield v3. |
| 1 | Researcher | What input shape must Phase 10 guarantee? | Disk and memory inputs are required for GNRL, plus DDS inputs from disk or memory DDS bytes. |
| 1 | Researcher | What compatibility proof counts before Phase 11? | Generated read-back is sufficient: libbsa reopens archives, compares metadata, extracts payloads, and validates DDS output with DirectXTex. |
| 1 | Scoring | Ambiguity after round 1? | Goal 0.86, boundary 0.62, constraint 0.68, acceptance 0.70, ambiguity 0.26. |
| 2 | Researcher + Simplifier | What is the irreducible success condition? | Both GNRL and DDS writers are mandatory; success is read-back equivalent archives for every required target. |
| 2 | Researcher + Simplifier | Should BA2 DDS writing require actual DDS input analysis? | Public APIs must accept DDS bytes/files and infer dimensions, DXGI format, mip count, array/cubemap metadata, and chunk records. |
| 2 | Researcher + Simplifier | Which compression behavior is mandatory? | All native routes are required: raw, FO4/method-0 deflate, and Starfield v3 method-3 raw LZ4-block where applicable. |
| 2 | Scoring | Ambiguity after round 2? | Goal 0.91, boundary 0.69, constraint 0.76, acceptance 0.76, ambiguity 0.21. |
| 3 | Boundary Keeper | Which adjacent work is explicitly out of scope? | External corpus/tool comparison, CLI/GUI packing workflows, performance/multithreading, in-place mutation, texture transcoding/optimization, and broad Phase 11 hardening are deferred. |
| 3 | Boundary Keeper | Should BA2 deduplication be required? | Opt-in dedup is required for identical post-policy GNRL payloads and identical DDS chunk payloads where BA2 layout permits shared offsets. |
| 3 | Boundary Keeper | What should happen with Starfield v3 `CompressionMethod` values? | Support method 0 and method 3; reject unsupported nonzero methods with structured errors. |
| 3 | Gate | Ambiguity gate reached; proceed to write SPEC.md? | User selected "Yes, write SPEC.md". |

---

*Phase: 10-ba2-writers*
*Spec created: 2026-05-07*
*Next step: /gsd-discuss-phase 10 - implementation decisions (how to build the locked BA2 writer requirements)*
