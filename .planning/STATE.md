---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: executing
stopped_at: Phase 10 context gathered
last_updated: "2026-05-07T11:55:41.906Z"
last_activity: 2026-05-07
progress:
  total_phases: 12
  completed_phases: 10
  total_plans: 53
  completed_plans: 53
  percent: 100
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-05)

**Core value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.
**Current focus:** Phase 10 — ba2-writers

## Current Position

Phase: 10 (ba2-writers) — EXECUTING
Plan: 2 of 7
Status: Ready to execute
Last activity: 2026-05-07

Progress: [██████████] 100%

## Performance Metrics

**Velocity:**

- Total plans completed: 25
- Average duration: 5min
- Total execution time: 0.82 hours

*Updated after each plan completion*
| Phase 01-build-error-and-test-foundation P01 | 3min | 3 tasks | 6 files |
| Phase 01-build-error-and-test-foundation P02 | 11min | 2 tasks | 3 files |
| Phase 01-build-error-and-test-foundation P03 | 3min | 2 tasks | 3 files |
| Phase 02-streaming-api-archive-model-detection-and-hashes P01-P05 | 48min | 11 tasks | 22 files |
| Phase 03-compression-services-and-policy P01-P04 | 18min | 9 tasks | 18 files |
| Phase 04-tes4-family-bsa-read-and-extract P01-P05 | 17min | 11 tasks | 11 files |
| Phase 05-tes3-bsa-read-and-extract P01-P04 | 21min | 8 tasks | 7 files |
| Phase 06-ba2-gnrl-read-and-extract P07 | 12min | 2 tasks | 3 files |
| Phase 07 P01 | 12min | 2 tasks | 3 files |
| Phase 07 P02 | 3min | 2 tasks | 4 files |
| Phase 07 P03 | 3min | 2 tasks | 2 files |
| Phase 07 P04 | 4min | 2 tasks | 6 files |
| Phase 07 P05 | 3min | 2 tasks | 4 files |
| Phase 07 P06 | 3min | 3 tasks | 3 files |
| Phase 08 P01 | 3min | 2 tasks | 6 files |
| Phase 08 P02 | 4min | 2 tasks | 4 files |
| Phase 08 P03 | 3min | 2 tasks | 4 files |
| Phase 08 P04 | 3min | 2 tasks | 4 files |
| Phase 08 P05 | 11min | 2 tasks | 5 files |
| Phase 08 P06 | 5min | 3 tasks | 3 files |
| Phase 09-bsa-writers P01 | 3min | 2 tasks | 5 files |
| Phase 09-bsa-writers P02 | 3min | 2 tasks | 4 files |
| Phase 09-bsa-writers P03 | 1min | 2 tasks | 3 files |
| Phase 09-bsa-writers P04 | 3min | 2 tasks | 2 files |
| Phase 09-bsa-writers P05 | 3min | 2 tasks | 4 files |
| Phase 09-bsa-writers P06 | 2min | 3 tasks | 4 files |
| Phase 10-ba2-writers P01 | 10min | 2 tasks | 4 files |
| Phase 10-ba2-writers P02 | 3min | 2 tasks | 2 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Roadmap]: Use fine-grained, correctness-first phases derived from v1 requirements and research.
- [Roadmap]: Keep read/extract slices before writer slices so writer compatibility builds on proven parsing, hashes, compression, and DDS behavior.
- [Roadmap]: Keep TES5Edit read-only as a reference; all implementation belongs outside `TES5Edit/`.
- [Phase 01]: Kept the committed Visual Studio 17 2022 preset while documenting that this machine only exposes Visual Studio 18 2026 for local verification.
- [Phase 01]: Used explicit CMake source lists and private imported dependency target resolution to keep TES5Edit/ and dependency details out of the public API.
- [Phase 01]: Implemented a local C++20 result API with std::variant storage rather than exposing C++23 std::expected.
- [Phase 01]: Used std::logic_error for wrong result observer calls as programmer precondition violations.
- [Phase 01]: Used local Visual Studio 18 2026 fallback verification for plan 01-03 because Visual Studio 17 2022 preset was unavailable; committed preset unchanged.
- [Phase 02]: Kept source and sink lifetimes caller-owned while exposing only memory helpers, bounded detection summaries, owned archive paths, and copied metadata views.
- [Phase 02]: Kept Bethesda hash routines internal/test-visible and locked compatibility through TES5Edit-provenance golden vectors.
- [Phase 02]: Preserved the committed Visual Studio 17 2022 preset while documenting local Visual Studio 18 2026 fallback validation commands.
- [Phase 03]: Compression routing is explicit by archive format, entry state, and Starfield CompressionMethod; codec libraries remain behind private implementation wrappers.
- [Phase 04]: TES4-family BSA read/extract uses `bsa_archive` with `archive_view` lookup, bounded parsing, compression dispatcher extraction, and TES5Edit-provenance compatibility tests.
- [Phase 05]: TES3 BSA read/extract reuses `bsa_archive` and `open_bsa` with validated table parsing, absolute `entry_metadata` payload offsets, raw extraction, and public-header-only API exposure.
- [Phase 06]: Kept duplicate normalized BA2 name rejection in parse_ba2_gnrl because duplicate name-table associations are BA2 malformed-input validation, not a generic archive_view policy.
- [Phase 06]: Validated FileTableOffset against byte_source::size even for zero-entry BA2 archives so empty archives remain allowed but impossible metadata is rejected.
- [Phase 08]: Established writer planning as an operation-style public API with in-memory entries and caller-owned finalization sinks. — Matches Phase 8 D-01 through D-04 and existing operation-style archive APIs.
- [Phase 08]: Kept writer planning/finalization behavior behind structured unsupported_format placeholders for later TDD plans. — Prevents false writer compatibility claims while establishing compile-safe seams for subsequent behavior work.
- [Phase 08]: Writer planning now normalizes and sorts entries before layout, preserving deterministic preview order independent of caller input order.
- [Phase 08]: Writer plans own stored payload bytes produced through resolve_write_compression, resolve_payload_codec, and compress_payload.
- [Phase 08]: Writer layout arithmetic failures return malformed_archive / writer layout overflow before any finalization sink can be touched.
- [Phase 08]: Writer deduplication groups exact post-policy stored payload bytes only after compression routing succeeds. — Writer deduplication groups exact post-policy stored payload bytes only after compression routing succeeds.
- [Phase 08]: Unsupported deduplication requests fail early with unsupported_format instead of silently emitting non-dedup output. — Unsupported deduplication requests fail early with unsupported_format instead of silently emitting non-dedup output.
- [Phase 08]: Public writer comments use digest-neutral wording so no public content hash surface is implied. — Public writer comments use digest-neutral wording so no public content hash surface is implied.
- [Phase 08]: Writer finalization streams chunks in frozen plan order — 16-byte LBSW header, entry table, data-region table, then data regions.
- [Phase 08]: Finalization serializes entry table records using the layout sizing contract — Records use fixed numeric fields plus exact normalized path bytes.
- [Phase 08]: Finalization propagates first byte_sink write error unchanged — No compression, normalization, sorting, or dedup decisions happen during finalization.
- [Phase 08]: Generated LBSW harness read-back remains test-only and validates metadata plus raw/deflate extraction through real codec routes.
- [Phase 08]: Starfield writer planning preserves nonzero CompressionMethod routing through lz4_block so unsupported method values cannot silently fall back to deflate.
- [Phase 08]: Public smoke and README document writer-core foundations without claiming complete BSA/BA2 writer compatibility.
- [Phase 09]: BSA writer API seam uses explicit targets, separate memory/disk inputs, and structured unsupported placeholders. — Preserves Phase 9 decisions D-01, D-02, D-08 and keeps later TDD plans behind stable public function names.
- [Phase 09]: TES4-family BSA finalization writes plan-owned native table bytes — Preserves plan/finalize determinism and avoids recomputing native layout during sink emission.
- [Phase 09]: TES4-family FileFlags are computed from known entry extensions without a public override — Keeps public controls semantic and minimal while matching Phase 9 research.
- [Phase 09-bsa-writers]: TES4-family embedded-name prefixes use native backslash archive path bytes. — Matches native BSA embedded-name byte spelling while reader extraction remains length-based.
- [Phase 09-bsa-writers]: BSA deduplication shares exact post-policy stored payload bytes. — Sharing after compression and embedded-name shaping prevents source-byte dedup from reusing incompatible native payload regions.
- [Phase 09-bsa-writers]: Disk-backed BSA inputs validate caller-provided archive virtual paths before opening host files. — Preserves structured archive-path errors ahead of filesystem I/O failures and keeps host/archive path concerns separate.
- [Phase 09-bsa-writers]: TES3 hash table bytes now follow TES5Edit save order: high 32 bits first, then low 32 bits, with reader and fixtures updated together. — Matches resolved Phase 9 research and keeps writer output/read-back aligned with BSArchPro compatibility.
- [Phase 09-bsa-writers]: TES3 planning rejects compression and embedded-name options structurally instead of downgrading to raw output. — TES3 native BSA writer only supports raw payloads and has no safe embedded-name representation.
- [Phase 09-bsa-writers]: Phase 09 public documentation claims production BSA writer support only for TES3 Morrowind and TES4-family v103/v104/v105 targets; BA2 writers, DDS packing, CLI/GUI, corpus comparison, performance/multithreading, and in-place mutation remain deferred or out of scope. — Keeps Phase 09 claims aligned with SPEC boundaries and protects Phase 10/11/12 scope.
- [Phase 09-bsa-writers]: The public-header smoke now validates BSA writer APIs by creating, finalizing, reopening, and extracting all required BSA targets through public headers only. — Ensures consumer-facing coverage proves runtime usability without private headers.
- [Phase 10-ba2-writers]: BA2 writer discovery uses a dedicated public header rather than extending the reader-focused BA2 header. — Matches Phase 10 D-01 and the Phase 9 BSA writer precedent.
- [Phase 10-ba2-writers]: BA2 GNRL and DDS/DX10 planning/finalization entry points return structured unsupported_format placeholders until downstream TDD plans implement native writer behavior. — Prevents false BA2 writer compatibility claims while establishing stable public names for downstream plans.
- [Phase 10-ba2-writers]: Native BA2 GNRL planning sorts normalized archive paths before layout so generated records and name tables are deterministic.
- [Phase 10-ba2-writers]: Raw BA2 GNRL records write native PackedSize as zero while data-region previews retain the actual stored byte count.
- [Phase 10-ba2-writers]: BA2 finalization streams frozen plan table bytes followed by plan-owned payload regions without recomputing layout.
- [Phase 10-ba2-writers]: BA2 GNRL disk planning validates archive-virtual paths and duplicate normalized names before opening host files. — Preserves structured writer-input errors ahead of filesystem errors and satisfies Phase 10 T-10-07 mitigation.
- [Phase 10-ba2-writers]: BA2 DDS writer planning analyzes source DDS bytes through a private DirectXTex adapter and stores only libbsa-owned metadata/chunk bytes in the write plan. — Preserves DirectXTex/private dependency boundaries while ensuring planning owns derived texture metadata and payload bytes before finalization.
- [Phase 10-ba2-writers]: BA2 DX10 chunking follows the resolved target rule: individual chunks above 256x256, then one final chunk for the first 256x256-or-smaller mip and all lower mips. — Matches Phase 10 research decision D-15 and keeps public callers from supplying native chunk descriptors.
- [Phase 10-ba2-writers]: BA2 DDS writer tests verify disk input equivalence through libbsa read-back metadata and extracted DDS bytes. — Matches WRT-03 read-back proof and avoids relying on host file lifetimes after planning.
- [Phase 10-ba2-writers]: Unsupported DX10 formats are rejected before DirectXTex load when the DDS header advertises an unsupported format. — Keeps unsupported DDS inputs as structured unsupported_format failures rather than generic malformed-load failures.
- [Phase 10-ba2-writers]: Native DX10 raw chunk records keep packed_size == size, unlike BA2 GNRL's zero packed-size raw marker. — Prevents confusing two BA2 native size encodings and preserves reader codec routing.
- [Phase 10-ba2-writers]: Phase 10 public documentation claims production BA2 writer support only for the implemented Fallout 4 and Starfield GNRL/DX10 targets; external corpus comparison, broad hardening, texture transforms, CLI/GUI, performance/multithreading, and in-place mutation remain deferred or out of scope. — Keeps Phase 10 claims aligned with implemented support and SPEC boundaries.
- [Phase 10-ba2-writers]: The public-header smoke now validates BA2 writer APIs by creating, finalizing, reopening, and extracting GNRL and DDS archives for both Fallout 4 and Starfield through public headers only. — Proves consumer-facing BA2 writer compile/link/runtime usability without private headers.
- [Phase ?]: [Phase 10-ba2-writers]: BA2 DDS writer support is explicitly scoped to R8G8B8A8_UNORM, BC1_UNORM, BC3_UNORM, BC5_UNORM, and BC7_UNORM until Phase 11 corpus validation justifies broader support. — Keeps README and implementation aligned with tested/reconstructable DDS formats.
- [Phase ?]: [Phase 10-ba2-writers]: DX10 chunks requested as compressed fall back to raw storage when compression is not smaller. — BA2 readers treat PackedSize == Size as the raw marker, so equal-size compressed bytes would corrupt extraction.

### Pending Todos

None yet.

### Blockers/Concerns

- BA2 DDS and Starfield writer behavior require fixture-backed validation before compatibility claims.
- Reference compatibility corpus still needs decisions about what can be committed versus kept external.

## Deferred Items

Items acknowledged and carried forward from previous milestone close:

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| *(none)* | | | |
| Phase 10-ba2-writers P03 | 2min | 2 tasks | 2 files |
| Phase 10-ba2-writers P04 | 6 min | 2 tasks | 5 files |
| Phase 10-ba2-writers P05 | 5 min | 2 tasks | 4 files |
| Phase 10-ba2-writers P06 | 3min | 3 tasks | 4 files |
| Phase 10-ba2-writers P07 | 3min | 3 tasks | 6 files |

## Session Continuity

Last session: 2026-05-07T11:55:21.808Z
Stopped at: Phase 10 context gathered
Stopped at: Completed 10-ba2-writers-02-PLAN.md
Resume file: None
