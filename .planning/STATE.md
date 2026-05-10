---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: Complete Library
status: Awaiting next milestone
stopped_at: Completed 12-07-PLAN.md
last_updated: "2026-05-10T10:24:16.885Z"
last_activity: 2026-05-10 — Milestone v1.0 completed and archived
progress:
  total_phases: 12
  completed_phases: 12
  total_plans: 75
  completed_plans: 75
  percent: 100
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-10)

**Core value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.
**Current focus:** Planning next milestone

## Current Position

Phase: Milestone v1.0 complete
Plan: —
Status: Awaiting next milestone
Last activity: 2026-05-10 — Milestone v1.0 completed and archived

## Performance Metrics

**Velocity:**

- Total plans completed: 75
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
| Phase 12 P01 | 9min | 3 tasks | 7 files |
| Phase 12 P02 | 6min | 3 tasks | 8 files |
| Phase 12 P03 | 10min | 3 tasks | 6 files |
| Phase 12 P04 | 12.3min | 3 tasks | 7 files |
| Phase 12 P05 | 7min | 3 tasks | 5 files |
| Phase 12 P06 | 5min | 3 tasks | 10 files |
| Phase 12 P07 | 4min | 3 tasks | 7 files |

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
- [Phase 12]: Bulk extraction uses archive_reader::extract_entries with an explicit positive worker_count and request-order result records.
- [Phase 12]: Bulk extraction sink factories must return distinct per-entry sinks and may be called concurrently when worker_count is greater than 1.
- [Phase 12]: Raw extraction bounded-memory proof uses large synthetic TES3, TES4, BA2 GNRL, and generated raw BA2 DX10 archives plus reader source-policy checks.
- [Phase 12]: Writer packing controls are exposed through write_execution_options at write_to time, not through archive compatibility options.
- [Phase 12]: Existing one-argument write_to overloads delegate to default write_execution_options so serial-default behavior remains observable.
- [Phase 12]: worker_count == 0 is rejected as invalid before writer finalization touches output or disk source paths.
- [Phase 12]: BSA writers stream final archive publication through bounded scratch buffers instead of materializing final whole-archive byte vectors.
- [Phase 12]: TES4 BSA writer worker_count prepares independent entries through detail::run_indexed_work before deterministic grouping, sorting, offsets, and publish.
- [Phase 12]: TES4 BSA overwrite publish uses unique temporary directories and safe replace helpers so failed worker-count paths preserve readable destination archives.
- [Phase 12]: BA2 GNRL raw disk payloads stream from caller-owned source files during final output while compressed entries remain bounded to per-entry codec buffers. — This satisfies PERF-02 bounded-memory finalization without changing the codec requirement for compressed payloads.
- [Phase 12]: BA2 DX10 add_file stores writer-owned temp snapshots per subresource instead of retaining whole DDS source bytes in writer state. — This preserves add-time ownership and later source-mutation behavior while keeping long-lived writer memory bounded.
- [Phase 12]: BA2 worker_count parallelism stores prepared results by deterministic entry/chunk index before sorting and offset assignment. — This keeps serial and parallel BA2 outputs compatible and deterministic.
- [Phase ?]: Benchmark evidence uses a custom C++20 runner with no new benchmark dependency. — Plan 12-05 keeps benchmark tooling lightweight and avoids adding a benchmark library dependency.
- [Phase ?]: Benchmark data is generated legal synthetic temp data and never uses TES5Edit or game archives. — Preserves D-26 and D-27 fixture legality and reference-boundary rules.
- [Phase ?]: Benchmark timing is report-only; correctness, report generation, and explicit target wiring are the automated contract. — Default tests must not fail on host-dependent speedup values.
- [Phase 12]: Doxygen remains optional through find_package(Doxygen QUIET); normal configure/build paths continue when the tool is missing. — Plan 12-06 satisfies DOC-01 without adding Doxygen as a required runtime or build dependency.
- [Phase 12]: Public API documentation is generated from include/libbsa plus docs pages only, with src, tests, build output, and TES5Edit excluded. — This enforces the D-21 and D-27 boundary between public API docs, private implementation, and read-only reference material.
- [Phase 12]: docs/thread-safety.md is the canonical D-23 thread-safety reference and public headers point readers to it instead of duplicating every rule inline. — A single canonical document plus policy tests keeps reader, writer, sink, validation, benchmark, and bulk extraction rules aligned.
- [Phase 12]: Docs generation and thread-safety coverage are enforced by default source-text policy tests. — Default CTest coverage catches Doxygen scope drift and D-23 public-type coverage gaps before docs are published.
- [Phase 12]: Consumer examples are compile-checked through the installed package smoke source and mirrored by docs headings. — Keeps documentation snippets tied to the public umbrella include and installed libbsa::libbsa target.
- [Phase 12]: Phase 12 benchmark and documentation evidence remains legal synthetic data only and keeps TES5Edit read-only. — Preserves fixture legality and the project reference boundary through final documentation.
- [Phase 12]: Target-format guidance is machine-checked against required headings, compression route text, and public compatibility_warning_code values parsed from validation.hpp. — Forces future warning-code or format-policy changes to update consumer documentation.

### Pending Todos

None yet.

### Blockers/Concerns

- Starfield BA2 v2/v3 fields and compression behavior need fixture-backed policy before future writer variants.
- BA2 DDS reconstruction, cubemaps, mip ordering, and chunk limits should remain covered during hardening.
- Public C++20 error/result API should avoid exposing C++23 `std::expected` until the project intentionally raises the language standard.
- v2 scope is not yet defined. Start with `$gsd-new-milestone` before adding new phases.

## Milestone Archives

| Milestone | Roadmap | Requirements | Audit | Phase Artifacts |
|-----------|---------|--------------|-------|-----------------|
| v1.0 Complete Library | [v1.0-ROADMAP.md](./milestones/v1.0-ROADMAP.md) | [v1.0-REQUIREMENTS.md](./milestones/v1.0-REQUIREMENTS.md) | [v1.0-MILESTONE-AUDIT.md](./milestones/v1.0-MILESTONE-AUDIT.md) | [v1.0-phases](./milestones/v1.0-phases/) |

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260510-46t | Close gaps and tech_debt from .planning/v1.0-MILESTONE-AUDIT.md | 2026-05-10 | f675b41 | [260510-46t-close-gaps-and-tech-debt-from-planning-v](./quick/260510-46t-close-gaps-and-tech-debt-from-planning-v/) |
| 260510-62q | Update Visual Studio toolchain references to Visual Studio 2026 | 2026-05-10 | uncommitted | [260510-62q-update-visual-studio-2026-docs](./quick/260510-62q-update-visual-studio-2026-docs/) |

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| v2 | Optional sample CLI | Tracked in REQUIREMENTS.md v2 | Project initialization |
| v2 | Public fuzzing harnesses and lenient corrupt-archive recovery | Tracked in REQUIREMENTS.md v2 | Project initialization |
| v2 | Stable long-term binary ABI policy | Tracked in REQUIREMENTS.md v2 | Project initialization |

## Session Continuity

Last session: 2026-05-10T08:59:27.146Z
Stopped at: Completed 12-07-PLAN.md
Resume file: None

## Operator Next Steps

- Start the next milestone with /gsd-new-milestone
