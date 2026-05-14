---
gsd_state_version: 1.0
milestone: v1.1
milestone_name: Hardening
status: ready_to_plan
stopped_at: Completed 14-03-PLAN.md
last_updated: "2026-05-14T07:13:27.7583141Z"
last_activity: 2026-05-14
progress:
  total_phases: 5
  completed_phases: 3
  total_plans: 8
  completed_plans: 8
  percent: 60
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-14)

**Core value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.
**Current focus:** Phase 14 — verification-lane-truthfulness

## Current Position

Phase: 15
Plan: Not started
Status: Ready to plan
Last activity: 2026-05-14 - Completed quick task 260513-xar: Decode public host paths as UTF-8 on Windows

Progress: [██████████] 100%

Current note: Phase 14 is closing the truthful supported matrix loop across docs and planning for the Windows debug, Release, and MSVC AddressSanitizer lanes.

## Performance Metrics

- Total plans completed: 89
- Current milestone plans completed: 7
- Historical baseline: v1.0 shipped across 12 phases and 75 plans
- Latest execution: 14-03 completed in 9 min across 6 files

## Accumulated Context

### Decisions

- v1.1 remains a hardening-only milestone with no public API expansion.
- Work order is correctness boundary → verification boundary → structural cleanup → hotspot hardening → ship gate.
- Reader, parser, preparer, dedupe, and DX10 staging changes must stay incremental, semantics-preserving, and test-backed.
- Land the neutral `host_file` seam before introducing `host_file_path` so later Phase 13 work does not carry helper-rename debt.
- Preserve caller-owned writer diagnostics while migrating active prepare/layout code to neutral host-file helper names.
- [Phase 13]: host_file helpers now accept host_file_path or resolved std::filesystem::path inputs so raw UTF-8 text stays diagnostics-only once resolved.
- [Phase 13]: Migrated writer call sites resolve disk-source paths once per operation and keep caller-owned diagnostic strings unchanged.
- [Phase 13]: archive_reader::open now resolves caller UTF-8 text once and stores detail::host_file_path in reader state — Detection and size probes now reuse the resolved host-file boundary instead of repeated raw-text conversion.
- [Phase 13]: Parser entry seams now accept detail::host_file_path — TES3, TES4, BA2 GNRL, and BA2 DX10 metadata opens now stay on the shared host_file boundary.
- [Phase 13]: Original UTF-8 host-path text is diagnostics-only across the open/parser seam — Source comments now lock the rule that resolved paths, not caller text, drive later open-time I/O.
- [Phase 13]: [Phase 13]: Reader reopen helpers now consume detail::host_file_path so follow-on extraction stays on the stored resolved host path. — Task 1 moved archive dispatch and concrete reader reopens onto detail::host_file_path and shared host_file helpers.
- [Phase 13]: [Phase 13]: validate_archive now delegates host-path setup to archive_reader::open and preserves direct setup failures vs report-based malformed-archive diagnostics. — Task 2 removed the duplicate readability preflight and kept extractability validation on the public reader path.
- [Phase 13]: [Phase 13]: Source-policy tests now lock reader reopen and validation seams against raw host-path reopen drift. — Task 1 and Task 2 added source-policy assertions so future seam changes cannot silently reintroduce narrow-string host-path opens.
- [Phase 13]: The dedicated host-path regression suite creates non-ASCII filesystem paths natively, then converts them back to explicit UTF-8 before calling the unchanged public reader and validation APIs.
- [Phase 13]: BA2 DX10 canonical extraction expectations are reconstructed from manifest-backed DDS layout metadata plus committed payload bytes, keeping the non-ASCII proof black-box and deterministic.
- [Phase 13]: A smoke policy gate now locks the host-path proof suite to public open, validate, and extract APIs so Phase 13 coverage does not drift into TES3 or writer-side scope.
- [Phase 14]: Release package proof stays inside the existing CTest-owned package_consumer_smoke and package_consumer_runtime_dll_copy path.
- [Phase 14]: The supported MSVC ASan lane keeps real /fsanitize=address instrumentation while disabling STL annotation ODR mismatches against prebuilt dependencies.
- [Phase 14]: Runtime DLL propagation is part of the supported verification lane contract so Catch2 discovery and package-consumer smoke run from checked-in outputs without caller PATH assumptions.
- [Phase 14]: The main Windows CI matrix now uses role-aware matrix.include rows so workflow output names both lane role and concrete preset without changing the preset commands.
- [Phase 14]: The MSVC AddressSanitizer lane stays a separate top-level job that runs the exact windows-msvc-asan-static preset triad instead of becoming a fifth matrix row.
- [Phase 14-verification-lane-truthfulness]: README keeps windows-msvc-debug-static as the quick path, then groups the remaining supported lanes by debug, Release package-proof, and MSVC AddressSanitizer roles. — Plan 14-03 needed one concrete maintainer entry point while still making the supported matrix truthful across docs and tests.
- [Phase 14-verification-lane-truthfulness]: PROJECT.md, ROADMAP.md, and STATE.md stay summary-scoped while 14-CONTEXT.md remains the detailed lane contract. — Phase 14 explicitly locked planning layering so project-wide summaries do not duplicate command-level matrix prose.
- [Phase 14-verification-lane-truthfulness]: validation_policy_tests.cpp must validate README, fixture policy, workflow, presets, and planning summaries independently against the same contract. — Independent repo-surface assertions keep one stale file from validating another and complete the truthfulness loop for VER-03.

### Pending Todos

None yet.

### Blockers/Concerns

- BA2 DX10 temp-data cleanup must be honest about any residual abnormal-termination risk.
- Dedupe optimizations must preserve exact stored-byte equality semantics.

### Quick Tasks Completed

| # | Description | Date | Commit | Status | Directory |
|---|-------------|------|--------|--------|-----------|
| 260513-6nl | Repair Phase 13 execution artifacts so the per-plan SUMMARY.md files are GSD-compliant and truthful | 2026-05-13 | Unavailable | Verified | [260513-6nl-i-suspect-that-phase-13-was-not-executed](./quick/260513-6nl-i-suspect-that-phase-13-was-not-executed/) |
| Phase 13 P02 | 5 min | 3 tasks | 11 files |
| Phase 13 P03 | 6 min | 3 tasks | 11 files |
| Phase 13 P04 | 3 min | 3 tasks | 14 files |
| Phase 14 P01 | 65m | 3 tasks | 7 files |
| Phase 14 P02 | 2 min | 2 tasks | 2 files |
| Phase 14-verification-lane-truthfulness P03 | 9m | 3 tasks | 6 files |
| 260513-xar | Decode public host paths as UTF-8 on Windows | 2026-05-14 | cb1caeb | Verified | [260513-xar-https-github-com-evildarkarchon-libbsa-b](./quick/260513-xar-https-github-com-evildarkarchon-libbsa-b/) |

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| v2 | Optional sample CLI | Tracked in REQUIREMENTS.md future scope | v1.0 close |
| v2 | Public fuzzing harnesses and lenient corrupt-archive recovery | Tracked in REQUIREMENTS.md future scope | v1.0 close |
| v2 | Stable long-term binary ABI policy | Tracked in REQUIREMENTS.md future scope | v1.0 close |

## Session Continuity

Last session: 2026-05-14T05:38:48.683Z
Stopped at: Completed 14-03-PLAN.md
Resume file: None
