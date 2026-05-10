---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: milestone
status: executing
stopped_at: Phase 12 context gathered
last_updated: "2026-05-10T07:38:15.466Z"
last_activity: 2026-05-10 -- Phase 12 planning complete
progress:
  total_phases: 12
  completed_phases: 11
  total_plans: 75
  completed_plans: 68
  percent: 91
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-10)

**Core value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.
**Current focus:** Phase 11 — Compatibility Warnings, Validation API, and Hardening

## Current Position

Phase: 12
Plan: Not started
Status: Ready to execute
Last activity: 2026-05-10 -- Phase 12 planning complete

Progress: [██████████] 100%

## Performance Metrics

**Velocity:**

- Total plans completed: 68
- Average duration: 13.7 min
- Total execution time: 1.5 hours

**Recent Plans:**

| Phase Plan | Duration | Tasks | Files |
|------------|----------|-------|-------|
| Phase 09 P01 | 2 min | 3 tasks | 2 files |
| Phase 09 P02 | 6min | 3 tasks | 23 files |
| Phase 09 P03 | 4 min | 3 tasks | 3 files |
| Phase 09 P04 | 3 min | 3 tasks | 5 files |
| Phase 09 P05 | 5min | 3 tasks | 3 files |
| Phase 09 P06 | 4min | 3 tasks | 3 files |
| Phase 09 P07 | 2min | 2 tasks | 3 files |
| Phase 10 P01 | 3min | 3 tasks | 5 files |
| Phase 10 P02 | 4min | 3 tasks | 3 files |
| Phase 10 P03 | 15min | 3 tasks | 3 files |
| Phase 10 P04 | 3min | 3 tasks | 2 files |
| Phase 10 P05 | 28min | 3 tasks | 5 files |
| Phase 10 P06 | 12min | 2 tasks | 2 files |
| Phase 11 P01 | 4 min | 2 tasks | 4 files |
| Phase 11 P05 | 2 min | 2 tasks | 3 files |
| Phase 11 P02 | 5 min | 2 tasks | 6 files |
| Phase 11 P03 | 4 min | 2 tasks | 3 files |
| Phase 11 P04 | 2 min | 2 tasks | 3 files |
| Phase 11 P06 | 4 min | 2 tasks | 6 files |
| Phase 11 P07 | 2 min | 2 tasks | 3 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table. Recent decisions affecting current work:

- [Roadmap]: Fine-grained sequential roadmap uses 12 phases derived from v1 requirements and research ordering.
- [Roadmap]: Read support precedes write support so writers can be validated by reopening, extracting, and comparing output.
- [Roadmap]: `TES5Edit/` remains read-only reference material and must not be edited, formatted, staged, or compiled into libbsa.
- [Phase 08]: BA2 GNRL writer deduplication remains opt-in through `ba2_gnrl_writer_options::deduplicate_payloads` and is disabled by default.
- [Phase 08]: Dedupe eligibility is based on byte-identical final stored payload vectors after raw/compressed routing, not source bytes alone.
- [Phase 08]: BA2 GNRL writer publish reserves a unique sibling temporary directory instead of using and deleting deterministic `<output>.tmp` files.
- [Phase 08]: Overwrite mode rejects non-regular existing destinations and uses backup+rollback before publishing replacement bytes.
- [Phase 09]: BA2 DX10 public writer is DDS-host-file-only and compressed-only at archive level.
- [Phase 09]: Starfield v3 DX10 writer options use Unknown1=1, Unknown2=0, and CompressionMethod=3 defaults.
- [Phase 09]: DDS source analysis copies libbsa-owned DDS and subresource bytes so later source-file mutation cannot affect output.
- [Phase 09]: DX10 chunk planning uses a libbsa-owned descriptor table and max_decoded_chunk_bytes sentinel semantics.
- [Phase 09]: BA2 DX10 writer compression is selected only by target/options metadata: FO4 deflate, Starfield v3 method 3 raw LZ4 block, and Starfield method 0 deflate.
- [Phase 09]: DX10 texture chunks copy validated DDS subresource bytes in planned BA2 order without resizing, transcoding, mip generation, repair, or image-data transformation.
- [Phase 09]: BA2 DX10 writer dedupe uses stored bytes plus raw size, packed size, and compression route so payload sharing cannot cross incompatible chunk metadata.
- [Phase 09]: BA2 DX10 writer overwrite publish mirrors Phase 8 backup/rollback behavior instead of deleting the existing output before replacement.
- [Phase 09]: DDS block-compressed block counts are rounded after uint64 promotion and use descriptor block dimensions rather than hard-coded 4x4 arithmetic.
- [Phase 10]: TES3 writer is a dedicated raw/uncompressed public writer with only overwrite_existing options. — Matches Phase 10 D-01 through D-04 while keeping public headers dependency-light.
- [Phase ?]: [Phase 10]: TES3 writer prepares and validates every source before reserving a publish temp directory. — Prevents missing disk sources from leaving partial published archives.
- [Phase ?]: [Phase 10]: TES3 writer safe publish uses unique .libbsa-tmp-N sibling directories and overwrite backups. — Preserves caller-owned temp siblings and restores previous output if overwrite publish fails.
- [Phase 10]: TES3 writer byte-level tests compute expected order from `detail::tes3_hash_sort_key(detail::hash_tes3(serialized_name))`, not insertion or alphabetical order.
- [Phase 10]: TES3 serializer arithmetic uses named checked uint32 helpers for table, data-section, payload, and raw-offset values.
- [Phase 10]: TES3 writer acceptance now uses archive_reader::open plus public list/find/contains/extract APIs as the oracle rather than relying only on byte-table inspection. — Reader-backed validation satisfies D-19 and prevents writer-internal-only false positives.
- [Phase 10]: Root-level TES3 writer paths remain in round-trip coverage when accepted by shared archive path normalization because TES3 archives use a flat name table. — Plan 04 kept Readme.txt coverage aligned with D-14 and current shared validator behavior.
- [Phase 10]: Committed TES3 writer fixture evidence is generated only through the public tes3_bsa_writer API. — This satisfies D-09 and D-10 while keeping fixture evidence legal and independent of TES5Edit.
- [Phase 10]: TES3 writer fixture validation uses structural table facts and reader extraction instead of full archive golden equality. — This satisfies D-12 and avoids over-constraining non-semantic archive bytes.
- [Phase 10]: Shared archive path normalization rejects embedded NUL bytes before separator normalization so all callers get stable invalid_argument behavior.
- [Phase 10]: TES3 non-overwrite publish remains routed through publish_file_without_replace after a final destination existence check.
- [Phase 11]: Plan 11-01 introduces the public validation contract only; validation behavior and the function definition remain for the later implementation plan. — Matches the plan boundary and leaves strict-open-backed behavior for Plan 11-02.
- [Phase 11]: Public compatibility warnings expose stable code and severity values while keeping byte offsets, record indexes, and chunk indexes out of the public report model. — Satisfies the Phase 11 public/private boundary and D-09 information-disclosure mitigation.
- [Phase 11]: Unknown malformed manifest expected_error values are treated as test-oracle failures instead of being remapped to invalid_argument. — Matches Phase 11 D-13 and D-17 by making manifest typos fail loudly while preserving stable public error-code assertions.
- [Phase 11]: validate_archive reuses archive_reader::open as the single strict parser source of truth — Prevents the validation facade from becoming a lenient second reader.
- [Phase 11]: validate_archive keeps empty and unreadable host paths as result-level setup failures — Readable unsupported or malformed archive bytes are instead reported as fatal validation diagnostics.
- [Phase 11]: Entry extractability validation remains opt-in and uses public reader APIs — validation_report does not expose archive_reader, entry listings, or extraction bytes.
- [Phase 11]: Target-family mismatch warnings compare caller expectations against parsed archive metadata only. — Prevents validation warnings from depending on host file extensions or private parser guesses.
- [Phase 11]: Entry-level warning records include optional normalized archive paths but no byte offsets, record indexes, or chunk indexes. — Preserves the Phase 11 public/private diagnostic boundary and keeps parser coordinates out of the stable API.
- [Phase 11]: Sound payload warning detection treats compressed entries under sound/ or with .wav, .xwm, or .fuz extensions as advisory compatibility risks. — Matches the locked representative warning scenario without adding public warning filters or logging callbacks.
- [Phase 11]: Public compatibility warning evidence is cataloged by one section per warning code so the policy test can enforce local Rule and Evidence coverage. — Plan 11-04 needs machine-checked catalog entries with local Rule and Evidence text.
- [Phase 11]: Optional local game or BSArchPro-derived checks remain smoke/compare-only, require requires-game-fixture, and skip when LIBBSA_GAME_FIXTURES is unset. — D-14, D-18, and D-19 keep optional corpus data out of default acceptance and committed fixtures.
- [Phase 11]: The consolidated malformed matrix uses manifest evidence for generated archive rows and test evidence for the TES4 oversized arithmetic regression. — This keeps mandatory evidence legal while referencing the existing in-repo arithmetic regression.
- [Phase 11]: Validation API matrix coverage runs each manifest-backed row through strict open or opt-in extractability validation according to the row phase. — This preserves fail-closed open behavior and exercises extraction/decompression failures through public validation reports.
- [Phase 11]: The Python validator rejects unknown matrix families, categories, expected errors, phases, evidence types, and unresolved evidence references. — This prevents malformed coverage claims from drifting away from generated manifests or explicit tests.
- [Phase 11]: The sanitizer hardening path is additive through linux-clang-asan-ubsan presets and is not added to the default Windows MSVC CI workflow. — Plan 11-07 preserves default Windows static/shared CI while adding an opt-in hardening path.
- [Phase 11]: Fixture policy documents the exact CTest label selection for malformed, validation, and compression sanitizer runs. — Plan 11-07 makes the hardening command machine-checkable through validation policy tests.

### Pending Todos

None yet.

### Blockers/Concerns

- Starfield BA2 v2/v3 fields and compression behavior need fixture-backed policy before future writer variants.
- BA2 DDS reconstruction, cubemaps, mip ordering, and chunk limits should remain covered during hardening.
- Public C++20 error/result API should avoid exposing C++23 `std::expected` until the project intentionally raises the language standard.
- Phase 10 verification found two TES3 writer gaps: embedded NUL archive paths can produce self-inconsistent archives, and non-overwrite publish lacks a final no-replace check before rename.

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| v2 | Optional sample CLI | Tracked in REQUIREMENTS.md v2 | Project initialization |
| v2 | Public fuzzing harnesses and lenient corrupt-archive recovery | Tracked in REQUIREMENTS.md v2 | Project initialization |
| v2 | Stable long-term binary ABI policy | Tracked in REQUIREMENTS.md v2 | Project initialization |

## Session Continuity

Last session: 2026-05-10T06:48:39.993Z
Stopped at: Phase 12 context gathered
Resume file: .planning/phases/12-performance-concurrency-documentation-and-polish/12-CONTEXT.md
