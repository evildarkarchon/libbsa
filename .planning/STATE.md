---
gsd_state_version: 1.0
milestone: v1.1
milestone_name: Hardening
status: milestone_complete
stopped_at: Completed 17.1-03-PLAN.md
last_updated: "2026-05-15T04:53:54.324Z"
last_activity: 2026-05-15
progress:
  total_phases: 6
  completed_phases: 7
  total_plans: 20
  completed_plans: 20
  percent: 117
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-15)

**Core value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.
**Current focus:** Phase 17.1 — address-tech-debt-in-planning-v1-1-milestone-audit-md

## Current Position

Phase: 17.1
Plan: Not started
Status: Milestone complete
Last activity: 2026-05-15

Progress: [██████████] 100%

Current note: Inserted urgent Phase 17.1 to address tech debt from .planning/v1.1-MILESTONE-AUDIT.md.

## Performance Metrics

- Total plans completed: 104
- Current milestone plans completed: 20
- Historical baseline: v1.0 shipped across 12 phases and 75 plans
- Latest execution: 17.1-03 completed in 25min across 2 tasks and 4 files

## Accumulated Context

### Roadmap Evolution

- Phase 17.1 inserted after Phase 17: Address tech debt in @.planning/v1.1-MILESTONE-AUDIT.md (URGENT)

### Decisions

- v1.1 remained a hardening-only milestone with no public API expansion.
- Work order was correctness boundary → verification boundary → structural cleanup → hotspot hardening → ship gate.
- Reader, parser, preparer, dedupe, and DX10 staging changes stayed incremental, semantics-preserving, and test-backed.
- [Phase 13]: Host-file helpers resolve UTF-8 host paths once into `detail::host_file_path`; original UTF-8 text is diagnostics-only after resolution.
- [Phase 13]: Reader reopen helpers and `validate_archive` now reuse the stored resolved host path instead of repeating raw host-path opens.
- [Phase 14]: The supported Windows verification matrix is role-based: debug inner-loop lanes, Release package proof lanes, and a separate MSVC AddressSanitizer hardening lane.
- [Phase 14]: Runtime DLL propagation is part of the supported verification lane contract so Catch2 discovery and package-consumer smoke run from checked-in outputs without caller PATH assumptions.
- [Phase 15]: `archive_reader` selects a file-local backend table once during open and reuses it for entries, find, extraction, and bulk extraction.
- [Phase 15]: `contains` stays implemented as `find` plus `has_value` so invalid archive-path input preserves invalid_argument behavior instead of collapsing into false.
- [Phase 16]: TES4 parser raw table/payload seams and BA2 DX10 snapshot/chunk seams are private, policy-guarded, and behavior-preserving.
- [Phase 16]: Parser/preparer seam guardrails live in dedicated source-policy Catch2 tests instead of expanding unrelated validation-policy tests.
- [Phase 17]: TES4-family BSA dedupe candidate identity uses stored size plus deterministic final stored-payload fingerprint only as a narrowing filter; `tes4_stored_payloads_equal` remains the sharing authority.
- [Phase 17]: BA2 GNRL prepared entries expose `final_stored_dedupe_hash` as explicit final-stored candidate evidence; it is a filter only, not a correctness authority.
- [Phase 17]: BA2 GNRL layout buckets dedupe candidates by stored size plus `final_stored_dedupe_hash`, then still calls `ba2_gnrl_payloads_equal` before sharing offsets; disk-backed comparisons use resolved host paths so non-ASCII Windows paths stay on the shared host-file seam.
- [Phase 17]: BA2 DX10 `write_to` is consuming after ordinary attempts because writer-owned snapshot files are cleaned promptly instead of retained for retry.
- [Phase 17]: BA2 DX10 snapshot cleanup is best-effort and preserves the primary validation, write, or publish result error.
- [Phase 17]: BA2 DX10 lifecycle documentation states successful completion, ordinary result-returning failure unwinding, destructor safety-net cleanup, and residual abnormal-termination risk.
- [Phase 17]: Official v1.1 ship-gate evidence is committed in `17-VERIFICATION.md`; optional local game-corpus and BSArchPro comparison tests remain advisory.
- [Phase 17.1]: Combined non-ASCII host-path reader regression stays in the existing host-path suite and covers entries/find/contains/extract_entries across a TES3-inclusive representative matrix.
- [Phase 17.1]: `detail::host_file_path` now stores only the resolved native path; diagnostics-only `original_utf8` state was removed rather than consumed by a new diagnostics surface.
- [Phase 17.1]: `archive_reader::state` no longer stores backend identity beside the selected backend table; backend identity remains only as file-local open-time table-selection plumbing.
- [Phase 17.1]: Writer host-path audit debt is closed by a source-policy inventory matrix covering TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 source/output/finalization/dedupe dispositions.
- [Phase 17.1]: BA2 DX10 abnormal-termination cleanup remains accepted residual risk and out of Phase 17.1 scope; `.planning/v1.1-MILESTONE-AUDIT.md` was not edited by this phase.

### Pending Todos

None.

### Blockers/Concerns

None.

### Quick Tasks Completed

| # | Description | Date | Commit | Status | Directory |
|---|-------------|------|--------|--------|-----------|
| Phase 13 | Host Path Correctness Boundary complete | 2026-05-14 | Multiple | Verified | .planning/phases/13-host-path-correctness-boundary/ |
| Phase 14 | Verification Lane Truthfulness complete | 2026-05-14 | Multiple | Verified | .planning/phases/14-verification-lane-truthfulness/ |
| Phase 15 | Reader Backend Dispatch Cleanup complete | 2026-05-14 | Multiple | Verified | .planning/phases/15-reader-backend-dispatch-cleanup/ |
| Phase 16 | Parser and Preparer Seam Extraction complete | 2026-05-14 | Multiple | Verified | .planning/phases/16-parser-and-preparer-seam-extraction/ |
| Phase 17 P01 | TES4 dedupe candidate narrowing | 2026-05-15 | c0564b4 | Verified | .planning/phases/17-writer-hotspot-hardening-and-ship-gate/ |
| Phase 17 P02 | BA2 GNRL staged dedupe identity hardening | 2026-05-15 | f1d59c9 | Verified | .planning/phases/17-writer-hotspot-hardening-and-ship-gate/ |
| Phase 17 P03 | BA2 DX10 snapshot cleanup lifecycle | 2026-05-15 | f9646e9 | Verified | .planning/phases/17-writer-hotspot-hardening-and-ship-gate/ |
| Phase 17 P04 | BA2 DX10 temporary lifecycle documentation | 2026-05-15 | edb3eb0 | Verified | .planning/phases/17-writer-hotspot-hardening-and-ship-gate/ |
| Phase 17 P05 | Debug/ASan/Release ship gate and planning closure | 2026-05-15 | 3db00f7 | Verified | .planning/phases/17-writer-hotspot-hardening-and-ship-gate/ |
| Phase 17.1 P01 | Combined non-ASCII reader-surface regression | 2026-05-15 | 3bee64b | Verified | .planning/phases/17.1-address-tech-debt-in-planning-v1-1-milestone-audit-md/ |
| Phase 17.1 P02 | Dead internal state removal | 2026-05-15 | c0efdc0 | Verified | .planning/phases/17.1-address-tech-debt-in-planning-v1-1-milestone-audit-md/ |
| Phase 17.1 P03 | Writer host-path audit closure | 2026-05-15 | 47f17c5 | Verified | .planning/phases/17.1-address-tech-debt-in-planning-v1-1-milestone-audit-md/ |

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| v2 | Optional sample CLI | Tracked in REQUIREMENTS.md future scope | v1.0 close |
| v2 | Public fuzzing harnesses and lenient corrupt-archive recovery | Tracked in REQUIREMENTS.md future scope | v1.0 close |
| v2 | Stable long-term binary ABI policy | Tracked in REQUIREMENTS.md future scope | v1.0 close |

## Session Continuity

Last session: 2026-05-15T04:53:45.508Z
Stopped at: Completed 17.1-03-PLAN.md
Resume file: None
