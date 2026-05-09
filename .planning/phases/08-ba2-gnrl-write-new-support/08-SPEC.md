# Phase 8: BA2 GNRL Write-New Support - Specification

**Created:** 2026-05-09
**Ambiguity score:** 0.15 (gate: <= 0.20)
**Requirements:** 8 locked

## Goal

Consumers can create new Fallout 4 BA2 GNRL v1, Starfield BA2 GNRL v2, and Starfield BA2 GNRL v3 archives from disk files or memory buffers, with explicit target profiles, end-of-archive filename tables, version-specific metadata, compression policy, opt-in deduplication, and reader-backed round-trip proof.

## Background

libbsa currently has BA2 GNRL read/extract support through `archive_reader`: byte-driven BA2 detection, GNRL header and record parsing, length-prefixed filename-table parsing, deterministic listing, normalized lookup, raw/deflate/raw-LZ4-block extraction, and public Starfield v2/v3 metadata optionals. Phase 7 added a public TES4-family BSA writer API with disk and memory sources, explicit target profiles, compression overrides, opt-in deduplication, and reader-backed round-trip tests. No public BA2 writer type or production BA2 GNRL serialization path exists today. The generated BA2 GNRL fixture generator currently writes its filename table before payload bytes, while Phase 8 requires writer output whose BA2 filename table is serialized at the end of the archive and proven by reopening and extraction.

## Requirements

1. **Public BA2 GNRL write-new API**: The library exposes a consumer-facing writer for new BA2 GNRL archives from disk files and memory buffers.
   - Current: `include/libbsa/writer.hpp` exposes only `tes4_bsa_writer`; BA2 writer behavior exists only in generated test fixture code and private reader/parser code.
   - Target: Consumers can add host-file entries and copied memory-buffer entries with explicit archive-internal paths, select a BA2 GNRL target profile, and finalize a new archive through public C++20 APIs that return `libbsa::result` values for expected failures.
   - Acceptance: Public include-boundary tests compile against installed libbsa headers and create at least one BA2 GNRL archive from disk-file entries and at least one from memory-buffer entries using only public libbsa APIs.

2. **Target profiles**: The writer supports explicit Fallout 4 v1, Starfield v2, and Starfield v3 BA2 GNRL target profiles.
   - Current: The reader detects these variants and exposes `archive_variant`, version, file count, default compression, and optional BA2 metadata; no writer can emit profile-specific BA2 headers.
   - Target: The write API requires an explicit BA2 GNRL target profile and serializes the correct `BTDX`/`GNRL` header version for Fallout 4 v1, Starfield v2, and Starfield v3 archives.
   - Acceptance: Writer-output tests create one FO4 v1 archive, one Starfield v2 archive, and one Starfield v3 archive; reopening each reports `archive_type::ba2`, the expected public variant, expected version, expected file count, and expected default compression.

3. **End filename table**: The writer serializes BA2 length-prefixed filename tables at the end of the archive.
   - Current: Existing BA2 GNRL reader tests and generated fixtures cover `FileTableOffset`, but generated success fixtures place the filename table before payload bytes.
   - Target: Writer output places stored payload bytes before the length-prefixed filename table, sets `FileTableOffset` to the end-table location, and remains readable by libbsa.
   - Acceptance: A writer-output archive reopens successfully with `FileTableOffset` greater than every payload offset; `entries()` lists every input path from the end filename table, and extraction byte-compares equal to the original source bytes.

4. **GNRL record serialization**: The writer serializes BA2 GNRL records with format-compatible hashes, extension fields, payload offsets, sizes, and sentinels.
   - Current: `ba2_gnrl_parser` reads BA2 GNRL records, validates the `BAADF00D` sentinel, exposes record flags, and uses FO4/BA2 hashes from generated fixtures; no production writer emits these records.
   - Target: Writer output emits one record per entry with the FO4/BA2 path hash, four-byte extension field, directory hash, caller-visible record flags where supported by the target profile, archive-absolute payload offset, packed/raw sizes, and the required `BAADF00D` sentinel.
   - Acceptance: A multi-folder writer-output archive reopens successfully; `entries()` returns all input entries exactly once by canonical path, preserves original archive spelling with `/` separators, exposes expected raw/stored sizes and payload offsets, and has no duplicate canonical paths.

5. **Compression policy and overrides**: The writer supports target defaults plus per-entry raw/compressed overrides for BA2 GNRL payloads.
   - Current: Internal compression routing can compress deflate and raw LZ4 block payloads, and BA2 GNRL extraction already routes compressed entries by parsed metadata; no BA2 writer can select compression.
   - Target: Target-default compression and per-entry raw/compressed overrides are available. Fallout 4 v1, Starfield v2, and Starfield v3 method `0` compressed entries use deflate; Starfield v3 method `3` compressed entries use raw LZ4 block. Raw entries use `PackedSize == 0` and extract without decompression.
   - Acceptance: Writer-output tests create raw and compressed entries for FO4 v1, SF v2, SF v3 method `0`, and SF v3 method `3`; reopening reports the expected `entry_compression`, `raw_size`, `stored_size`, and extraction byte-compares equal to source bytes.

6. **Version-specific BA2 fields**: The writer sets or preserves Starfield BA2 v2/v3 header fields through documented target-profile options.
   - Current: Public `ba2_archive_metadata` exposes Starfield v2 `Unknown1` and `Unknown2`, plus Starfield v3 `CompressionMethod`, only when reading existing archives.
   - Target: Starfield writer profiles document how `Unknown1`, `Unknown2`, and `CompressionMethod` are chosen, including caller-provided values where the public writer options expose them and deterministic defaults where they do not.
   - Acceptance: Writer-output Starfield v2/v3 tests reopen archives and verify the expected `starfield_unknown1`, `starfield_unknown2`, and `compression_method` values through public metadata.

7. **Opt-in payload deduplication**: The writer optionally deduplicates identical BA2 GNRL stored payloads when requested.
   - Current: Phase 7 implements opt-in final-stored-byte deduplication for TES4-family BSA output, but BA2 GNRL writer state does not exist.
   - Target: BA2 GNRL deduplication is disabled by default; when enabled, entries with byte-identical final stored payloads and compatible metadata may share one payload region while preserving distinct records and filename-table entries.
   - Acceptance: Tests create duplicate-content BA2 GNRL entries with deduplication disabled and enabled; disabled output has distinct payload offsets, enabled output shares eligible identical stored payload offsets, and both modes reopen and extract every entry with byte-identical content.

8. **Reader-backed round-trip validation**: Writer output is validated through public reader APIs rather than writer internals.
   - Current: BA2 GNRL reader support is validated with generated fixtures, and Phase 7 validates BSA writer output by reopening and extracting; no BA2 writer-output round-trip tests exist.
   - Target: Phase 8 tests pack, reopen, list, find, contains, extract, and byte-compare BA2 GNRL writer output for disk sources, memory sources, zero-byte payloads, mixed-case lookup variants, raw entries, compressed entries, end filename tables, and opt-in deduplication.
   - Acceptance: CTest labels for BA2 GNRL writer round trips pass for FO4 v1, Starfield v2, Starfield v3 method `0`, and Starfield v3 method `3`; existing BA2 GNRL reader tests, TES4-family writer tests, and public include-boundary tests remain green.

## Boundaries

**In scope:**
- Public BA2 GNRL write-new API for disk-file and memory-buffer entries.
- Explicit Fallout 4 v1, Starfield v2, and Starfield v3 GNRL target profiles.
- BA2 GNRL header, record, payload, and end-of-archive filename-table serialization.
- Version-specific Starfield `Unknown1`, `Unknown2`, and `CompressionMethod` metadata according to documented target-profile options.
- Raw, deflate-compressed, and Starfield raw-LZ4-block-compressed BA2 GNRL payload writing through explicit target/profile policy.
- Per-entry raw/compressed overrides relative to the selected target/default policy.
- Opt-in final-stored-byte payload deduplication for BA2 GNRL entries.
- Reader-backed tests that reopen writer output, inspect metadata, list/find/contains entries, extract entries, and byte-compare source payloads.

**Out of scope:**
- BA2 DX10/DDS texture writing - Phase 9 owns DDS analysis, texture records, mip/chunk planning, and DDS writer validation.
- TES3/Morrowind BSA writing - Phase 10 owns TES3 write support and data-section-relative offset serialization.
- In-place mutation of existing archives - v1 explicitly prefers write-new flows until all format writers and validation behavior are proven.
- Parallel packing, parallel compression, streaming writer performance guarantees, and benchmarks - Phase 12 owns performance and concurrency after single-threaded correctness.
- Compatibility warning APIs and broad malformed-input hardening - Phase 11 owns structured validation and hardening APIs.
- Broad BSArchPro UI/CLI option parity - libbsa remains a reusable library and this phase locks only BA2 GNRL writer requirements.
- Copyrighted game archives or mutable `TES5Edit/` fixtures - required validation uses repository-owned generated data or writer-produced archives outside `TES5Edit/`.
- Public exposure of libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, or private parser/codec types - public headers remain dependency-light.

## Constraints

- Public APIs must remain C++20-compatible and must not expose `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, `TES5Edit/`, or private parser/codec types.
- BA2 writer behavior must use explicit target profiles and metadata-driven compression routing; it must not infer archive version or compression solely from host file extensions.
- Starfield v3 `CompressionMethod == 3` compressed payloads must use raw LZ4 block compression, not the LZ4 frame API.
- Deflate and raw LZ4 block writer output must be accepted by existing exact-size extraction paths after reopening.
- `TES5Edit/` remains read-only: no editing, formatting, compiling, staging, or fixture workspace use.
- Writer validation must use write-new output and generated/legal test data; optional local game archives cannot be mandatory for acceptance.
- Single-threaded correctness is the phase boundary; large-archive bounded-memory writer guarantees beyond existing source reading and finalization behavior are deferred to Phase 12 unless needed for correctness.
- Existing BA2 reader behavior may be adjusted only as required to prove end-of-archive filename-table writer output and must preserve existing generated fixture coverage.

## Acceptance Criteria

- [ ] Public installed-header tests create BA2 GNRL archives from disk-file entries and memory-buffer entries without private headers.
- [ ] Writer-output archives for FO4 v1, Starfield v2, and Starfield v3 reopen successfully and report the expected variant, version, file count, default compression, and BA2 metadata optionals.
- [ ] Writer-output archives serialize `FileTableOffset` to an end-of-archive filename table after payload bytes, and reopened entries are parsed from that table.
- [ ] Multi-folder writer-output archives list all input entries exactly once under normalized canonical paths and preserve original path spelling with `/` separators.
- [ ] BA2 GNRL record metadata reports expected raw size, stored size, payload offset, archive hash, record flags, compression mode, and non-embedded-name state for writer output.
- [ ] Raw and compressed entries round-trip for FO4 v1 deflate, Starfield v2 deflate, Starfield v3 method `0` deflate, and Starfield v3 method `3` raw LZ4 block profiles.
- [ ] Per-entry raw/compressed overrides produce the expected `PackedSize`/compression metadata and extract bytes equal to source bytes.
- [ ] Starfield v2/v3 writer-output metadata exposes expected `Unknown1`, `Unknown2`, and `CompressionMethod` values through public metadata.
- [ ] Deduplication disabled output stores duplicate-content entries at distinct offsets; deduplication enabled output shares eligible identical stored payload offsets while preserving distinct entries.
- [ ] Disk-source, memory-source, zero-byte, mixed-case lookup, and multi-folder cases pass pack/reopen/find/contains/extract/byte-compare tests.
- [ ] Existing BA2 GNRL reader, BA2 DX10 reader, TES4-family writer, public include-boundary, and cross-format regression tests remain green.
- [ ] BA2 DX10 writing, TES3 writing, in-place mutation, parallel packing, and compatibility warning APIs are not introduced by this phase.
- [ ] `git -C TES5Edit status --short` produces no output after phase work.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.94  | 0.75  | PASS   | Public BA2 GNRL writer, required profiles, end filename table, compression, dedupe, and round-trip proof are locked. |
| Boundary Clarity    | 0.80  | 0.70  | PASS   | DX10 writing, TES3 writing, mutation, performance, warning APIs, CLI parity, and dependency leakage are excluded. |
| Constraint Clarity  | 0.78  | 0.65  | PASS   | C++20 public boundary, explicit profile/compression policy, raw-LZ4 method routing, end-table reader proof, and TES5Edit boundary are locked. |
| Acceptance Criteria | 0.82  | 0.70  | PASS   | Pass/fail criteria cover API, profiles, metadata, filename table, compression, dedupe, round-trip, regression, and submodule cleanliness. |
| **Ambiguity**       | 0.15  | <=0.20| PASS   | Gate passed after round 2. |

Status: PASS = meets minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What should the consumer-visible creation surface be? | Mirror the Phase 7 BSA writer shape with a public BA2 GNRL writer for disk files and memory buffers. |
| 1 | Researcher | Which target profiles are pass/fail acceptance targets? | Fallout 4 v1, Starfield v2, and Starfield v3 are all required. |
| 1 | Researcher | What filename-table output shape should Phase 8 lock? | Writer output must use an end-of-archive filename table and prove it through libbsa reader round trips. |
| 2 | Researcher + Simplifier | What is the irreducible core scope? | Single-threaded round-trip writer core: public API, profiles, end table, raw/compressed entries, and reader-backed byte comparison. |
| 2 | Researcher + Simplifier | What compression capability is required? | Target defaults plus per-entry raw/compressed overrides; FO4/SF v2/SF v3 method `0` use deflate and SF v3 method `3` uses raw LZ4 block. |
| 2 | Simplifier | Should BA2 GNRL payload deduplication be part of this phase? | Yes, opt-in deduplication is in scope for BA2 GNRL writer output. |

---

*Phase: 08-ba2-gnrl-write-new-support*
*Spec created: 2026-05-09*
*Next step: /gsd-discuss-phase 8 - implementation decisions only*
