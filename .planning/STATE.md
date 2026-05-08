---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: executing
stopped_at: Completed 05-05-PLAN.md
last_updated: "2026-05-08T23:34:58.461Z"
last_activity: 2026-05-08 -- Phase 05 planning complete
progress:
  total_phases: 12
  completed_phases: 4
  total_plans: 27
  completed_plans: 26
  percent: 96
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-08)

**Core value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.
**Current focus:** Phase 05 — ba2-gnrl-read-extract

## Current Position

Phase: 05 (ba2-gnrl-read-extract) — EXECUTING
Plan: 6 of 6
Status: Ready to execute
Last activity: 2026-05-08 -- Phase 05 planning complete

Progress: [██████████] 100%

## Performance Metrics

**Velocity:**

- Total plans completed: 16
- Average duration: 13.7 min
- Total execution time: 1.5 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 03 | 6 | - | - |

| Phase 01 P01 | 8 min | 2 tasks | 7 files |
| Phase 01 P04 | 5 min | 2 tasks | 5 files |
| Phase 01 P02 | 18 min | 3 tasks | 9 files |
| Phase 01 P03 | 10 min | 2 tasks | 3 files |
| Phase 01 P05 | 35 min | 3 tasks | 6 files |
| Phase 02 P01-05 | 85min | 13 tasks | 34 files |
| Phase 03 P01 | 3min | 2 tasks | 7 files |
| Phase 03 P02 | 8min | 2 tasks | 9 files |
| Phase 03 P03 | 10min | 2 tasks | 10 files |
| Phase 03 P04 | 35min | 2 tasks | 10 files |
| Phase 03 P05 | 8min | 2 tasks | 7 files |
| Phase 03 P06 | 24min | 3 tasks | 5 files |
| Phase 04 P01 | 4 min | 3 tasks | 18 files |
| Phase 04 P02 | 3 min | 2 tasks | 5 files |
| Phase 04 P03 | 6 min | 3 tasks | 10 files |
| Phase 04 P04 | 5 min | 3 tasks | 5 files |
| Phase 04 P05 | 3min | 3 tasks | 7 files |
| Phase 05 P01 | 3min | 2 tasks | 18 files |
| Phase 05 P02 | 5min | 2 tasks | 8 files |
| Phase 05 P03 | 3min | 2 tasks | 7 files |
| Phase 05 P04 | 2min | 2 tasks | 4 files |
| Phase 05 P05 | 7min | 3 tasks | 3 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table. Recent decisions affecting current work:

- [Roadmap]: Fine-grained sequential roadmap uses 12 phases derived from v1 requirements and research ordering.
- [Roadmap]: Read support precedes write support so writers can be validated by reopening, extracting, and comparing output.
- [Roadmap]: `TES5Edit/` remains read-only reference material and must not be edited, formatted, staged, or compiled into libbsa.
- [Phase 01]: Installed-package validation uses a separate consumer CMake project configured with CMAKE_PREFIX_PATH instead of source-tree include paths.
- [Phase 01]: Windows shared-library verification exports symbols automatically for the current source-anchor DLL and supplies runtime DLL paths during tests.
- [Phase 01]: CI validates both windows-msvc-debug-static and windows-msvc-debug-shared presets and fails if TES5Edit has any git status output.
- [Phase 02]: Internal primitives remain under src/detail with no public API expansion.
- [Phase 02]: Compression adapters enforce exact-size decompression and route only by explicit metadata enum.
- [Phase 02]: Bethesda hash compatibility follows TES5Edit LowerByte/CreateHash behavior while keeping TES5Edit read-only.
- [Phase 03]: Expose Phase 3 reader contracts on archive_reader rather than adding separate public reader/view/extractor objects. — Plan 03-01 follows D-01 and gives later parser/extractor tasks a stable public surface.
- [Phase 03]: Keep nlohmann-json test-only via PRIVATE libbsa_tests linkage and out of libbsa runtime/public linkage. — D-29 permits JSON manifests for tests while public/runtime dependency boundaries remain clean.
- [Phase 03]: Fixture generation uses a C++ test tool linked to libbsa internals so archive hashes and compression payloads are produced by the same helpers later parser tests will validate.
- [Phase 03]: Success manifests store both canonical paths and original archive spelling so later tests can verify lookup normalization without trusting host filenames.
- [Phase 03]: Malformed fixture generation remains in the C++ fixture generator so success and malformed archives share hash/compression helper behavior.
- [Phase 03]: Manifest validation is parser-free and uses Python standard json to validate fixture expectations before parser implementation.
- [Phase 03]: TES4-family BSA open metadata reports archive_variant::tes4 for v103/v104/v105 while version/default_compression distinguish subtype behavior. — Avoid speculative public enum expansion before full variant model is needed.
- [Phase 03]: Duplicate canonical path validation remains deferred to Plan 03-05 full entry parsing. — Plan 03-04 intentionally exposes metadata-only open state and does not parse entry names yet.
- [Phase 03]: TES4-family metadata parsing exposes canonical sorted entries while preserving archive spelling in original_path with `/` separators. — Plan 03-05 prepares extraction selectors without host filesystem semantics.
- [Phase 03]: TES4-family lookup is canonical-path based and rejects duplicate normalized keys during open. — Ensures deterministic find/contains semantics for archive consumers.
- [Phase 03]: TES4-family extraction remains path-first on archive_reader and returns not_found for valid missing paths while preserving invalid_argument for malformed archive paths. — Plan 03-06 preserves D-14/D-20 caller semantics.
- [Phase 03]: extract_bytes is a convenience adapter over extract(path, sink), not a separate decoding path. — Keeps embedded-name and compression behavior identical across APIs.
- [Phase 03]: TES4-family reader state stores host path plus metadata, not whole archive bytes. — Verification required bounded open/extract behavior before completing the phase.
- [Phase 04]: TES3 fixture manifests record both raw_tes3_data_offset and archive-absolute payload_offset so later parser work can prove D-01 through D-03 without expanding public metadata. — Keeps raw TES3 offset proof in fixtures while preserving public archive-absolute payload offsets.
- [Phase 04]: TES3 RED tests compare manifest archive_hash values through the current public hash field until the planned Phase 04-02 neutral public rename lands. — Plan 04-01 must compile before the public metadata rename planned for 04-02.
- [Phase 04]: entry_metadata now exposes archive_hash instead of tes4_hash so TES3 and TES4-family entries share format-neutral public metadata. — Phase 04 is the first point where the TES4-specific public field name becomes inaccurate, and this is a pre-v1 API cleanup.
- [Phase 04]: TES3 detection is limited to byte classification of little-endian 0x00000100 and deliberately defers parser/open routing to Plan 04-03. — Plan 04-02 only prepares detector scaffolding; full TES3 parser and open dispatch are scoped to Plan 04-03.
- [Phase 04]: TES3 parser stores archive-absolute payload offsets only; raw TES3 data-section offsets are converted and validated during parse. — Keeps public metadata consistent across archive variants while preserving TES3 compatibility.
- [Phase 04]: TES3 hash ordering is represented with explicit low32/high32 helper functions to avoid comparator ambiguity. — Matches TES3 sorted hash records and makes validation testable.
- [Phase 04]: TES3 listing and lookup use dedicated private helper names while preserving the established archive_reader public facade. — Minimizes TES4-family regression risk while adding variant-aware dispatch.
- [Phase 04]: TES3 extraction uses a dedicated raw-only helper instead of reusing TES4-family compression routing, so malformed non-raw TES3 metadata fails with format_error. — Keeps TES3 read scope raw/uncompressed and prevents accidental codec routing.
- [Phase 04]: Invalid archive-owned TES3 paths map to format_error during open. — Caller path errors remain invalid_argument, but malformed archive bytes should fail closed as format errors.
- [Phase 04]: TES3 duplicate stored-hash validation now runs before standalone stored-vs-computed mismatch validation so collision fixtures exercise the intended branch.
- [Phase 04]: TES3 extraction dispatches to a host-file streaming helper before generic stored-payload buffering, preserving bounded sink-first behavior.
- [Phase 05]: [Phase 05]: BA2 fixtures use committed generated bytes plus rich manifests so later parser tasks have stable generated-only acceptance inputs. — Generated-only fixtures satisfy Phase 5 legal-input constraints and keep parser acceptance stable.
- [Phase 05]: [Phase 05]: Starfield v3 LZ4 fixture coverage uses compression_method 3 with explicit lz4_block routing rather than extension inference. — Phase 5 decisions require explicit metadata-driven compression routing.
- [Phase 05]: BA2 detection runs before BSA fallback for BTDX bytes, preserving byte-driven routing and clean unsupported errors for DX10. — BA2 detection runs before BSA fallback for BTDX bytes, preserving byte-driven routing and clean unsupported errors for DX10.
- [Phase 05]: Starfield BA2 v2/v3 raw header fields are exposed as version-gated std::optional values. — Starfield BA2 v2/v3 raw header fields are exposed as version-gated std::optional values.
- [Phase 05]: BA2 GNRL parser internals remain private and materialize public archive_metadata/entry_metadata only. — Preserves public C++20 API boundaries while adding parser-specific behavior.
- [Phase 05]: BA2 GNRL lookup uses canonical archive path normalization and lower_bound semantics established by prior BSA readers. — Keeps find and contains behavior deterministic and consistent across archive families.
- [Phase 05]: BA2 GNRL extraction routes by parsed entry_metadata::compression — Keeps BA2 raw, deflate, and raw LZ4 block extraction metadata-driven and prevents filename/extension inference.
- [Phase 05]: archive_reader::extract dispatches BA2 entries to extract_ba2_gnrl_payload — Preserves sink-first BA2 extraction before generic TES4-family stored-payload buffering.
- [Phase 05]: Malformed BA2 tests assert only stable error_code values from manifest, never diagnostic message text. — Preserves D-11 while validating fail-closed malformed and unsupported BA2 behavior.
- [Phase 05]: Task 2 malformed hardening was already present from prior Phase 5 parser/extraction work; closeout documented existing fail-closed codec behavior. — Focused verification showed no remaining detector/parser/extractor gaps after adding the malformed manifest tests.

### Pending Todos

None yet.

### Blockers/Concerns

- Starfield BA2 v2/v3 fields and compression behavior need fixture-backed policy before writer phases.
- BA2 DDS reconstruction, cubemaps, mip ordering, and chunk limits need focused validation during DDS phases.
- Public C++20 error/result API should avoid exposing C++23 `std::expected` until the project intentionally raises the language standard.

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| v2 | Optional sample CLI | Tracked in REQUIREMENTS.md v2 | Project initialization |
| v2 | Public fuzzing harnesses and lenient corrupt-archive recovery | Tracked in REQUIREMENTS.md v2 | Project initialization |
| v2 | Stable long-term binary ABI policy | Tracked in REQUIREMENTS.md v2 | Project initialization |

## Session Continuity

Last session: 2026-05-08T23:09:02.447Z
Stopped at: Completed 05-05-PLAN.md
Resume file: None
