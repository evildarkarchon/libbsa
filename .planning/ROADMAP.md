# Roadmap: libbsa

## Milestones

- [x] **v1.0 Complete Library** - Phases 1-12 shipped 2026-05-10.
  Archives: [roadmap](milestones/v1.0-ROADMAP.md), [requirements](milestones/v1.0-REQUIREMENTS.md), [audit](milestones/v1.0-MILESTONE-AUDIT.md), [phase artifacts](milestones/v1.0-phases/).
- [ ] **v1.1 Hardening** - Phases 13-17 planned for correctness, verification, targeted seam cleanup, and hotspot hardening without public API expansion.

## Overview

v1.1 hardens the shipped library rather than broadening it. The milestone follows the research-backed order of correctness boundary → verification boundary → structural cleanup → hotspot hardening → ship gate, while keeping all work incremental, semantics-preserving, and test-backed.

## Phases

<details>
<summary>✅ v1.0 Complete Library (Phases 1-12) - SHIPPED 2026-05-10</summary>

- [x] Phase 1: Foundation, API Boundary, and Test Harness (5/5 plans) - completed 2026-05-08
- [x] Phase 2: Binary I/O, Paths, Hashes, and Compression Services (5/5 plans) - completed 2026-05-08
- [x] Phase 3: Format Detection and TES4-Family BSA Read/Extract (6/6 plans) - completed 2026-05-08
- [x] Phase 4: TES3 BSA Read/Extract (5/5 plans) - completed 2026-05-08
- [x] Phase 5: BA2 GNRL Read/Extract (6/6 plans) - completed 2026-05-08
- [x] Phase 6: DDS Boundary and BA2 DX10 Read/Reconstruction (8/8 plans) - completed 2026-05-09
- [x] Phase 7: TES4-Family BSA Write-New Support (6/6 plans) - completed 2026-05-09
- [x] Phase 8: BA2 GNRL Write-New Support (7/7 plans) - completed 2026-05-09
- [x] Phase 9: BA2 DX10 Write-New Support (7/7 plans) - completed 2026-05-09
- [x] Phase 10: TES3 Write Support and BSA Format Completeness (6/6 plans) - completed 2026-05-10
- [x] Phase 11: Compatibility Warnings, Validation API, and Hardening (7/7 plans) - completed 2026-05-10
- [x] Phase 12: Performance, Concurrency, Documentation, and Polish (7/7 plans) - completed 2026-05-10

</details>

### 🚧 v1.1 Hardening (In Progress)

- [x] **Phase 13: Host Path Correctness Boundary** - Verification snapshot recorded 2026-05-13, but the current repository still carries uncommitted Phase 13 implementation and summary-repair work. (completed 2026-05-13)
- [x] **Phase 14: Verification Lane Truthfulness** - Make the supported Windows debug, Release package-proof, and MSVC AddressSanitizer verification lanes real, runnable, and policy-aligned. (completed 2026-05-14)
- [x] **Phase 15: Reader Backend Dispatch Cleanup** - Select reader backend once at open time and keep reader behavior stable across operations. (completed 2026-05-14)
- [x] **Phase 16: Parser and Preparer Seam Extraction** - Break the targeted TES4 parser and BA2 DX10 preparer hotspots into smaller, test-backed helpers. (completed 2026-05-14)
- [ ] **Phase 17: Writer Hotspot Hardening and Ship Gate** - Harden dedupe and BA2 DX10 temp staging under existing semantics and close the milestone with ship-ready evidence.

## Phase Details

### Phase 13: Host Path Correctness Boundary
**Goal**: Consumers and maintainers can trust archive open and validation flows on Windows host paths that contain non-ASCII characters.
**Depends on**: Phase 12
**Requirements**: HOST-01, HOST-02, HOST-03
**Success Criteria** (what must be TRUE):
  1. Consumer can open representative supported BSA and BA2 archives from Windows host paths containing non-ASCII characters.
  2. Consumer can validate representative supported BSA and BA2 archives from Windows host paths containing non-ASCII characters.
  3. Maintainer can run committed regression tests that prove non-ASCII host-path open and validate coverage for representative BSA and BA2 families.
**Plans**: 5 plans

Plans:
**Wave 1**
- [x] 13-01-PLAN.md - Rename and generalize the shared host-file helper into the locked `host_file` family and remove old helper names.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 13-02-PLAN.md - Introduce the shared internal `host_file_path` contract and migrate the shared helper plus writer call sites onto it per D-14.

**Wave 3** *(blocked on Wave 2 completion)*
- [x] 13-03-PLAN.md - Resolve and store host paths once at open time, then migrate parser contracts and parser opens onto that boundary.

**Wave 4** *(blocked on Wave 3 completion)*
- [x] 13-04-PLAN.md - Rewire reader reopens and validation setup onto the single stored host-path boundary while preserving public semantics.

**Wave 5** *(blocked on Wave 4 completion)*
- [x] 13-05-PLAN.md - Prove non-ASCII host-path open, validation, and canonical extraction with one dedicated cross-family regression suite.

### Phase 14: Verification Lane Truthfulness
**Goal**: Maintainers can rely on the documented hardening lanes because the supported Release and ASan flows are checked in, runnable, and consistently described.
**Depends on**: Phase 13
**Requirements**: VER-01, VER-02, VER-03
**Success Criteria** (what must be TRUE):
  1. Maintainer can configure and run a checked-in Windows MSVC Release preset for libbsa builds and automated tests.
  2. Maintainer can configure and run a checked-in Windows MSVC ASan preset for libbsa builds and automated tests.
  3. `.planning`, `CMakePresets.json`, CI, and policy tests all describe the same supported verification lanes without drift.
**Plans**: 3 plans

Plans:
**Wave 1**
- [x] 14-01-PLAN.md - Make Release and ASan lanes real in CMake, presets, CTest ownership, and the shared validation-policy contract.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 14-02-PLAN.md - Reshape GitHub Actions into the supported debug/release matrix plus a separate MSVC AddressSanitizer hardening job.

**Wave 3** *(blocked on Waves 1-2 completion)*
- [x] 14-03-PLAN.md - Align README, fixture policy, and planning summaries with the supported matrix and close the truthfulness gate across every named surface.

### Phase 15: Reader Backend Dispatch Cleanup
**Goal**: Reader behavior is selected once at open time so consumers see unchanged archive operations while maintainers stop repeating family dispatch logic across the public reader surface.
**Depends on**: Phase 14
**Requirements**: DISP-01, DISP-02
**Success Criteria** (what must be TRUE):
  1. Consumer can list, look up, check, extract, and bulk extract entries after one open-time backend selection across supported archive families.
  2. Those reader operations continue to behave the same as before across supported archive families.
  3. Maintainer can adjust reader-backend behavior without duplicating archive-family branching across each public reader operation.
**Plans**: 1 plan

Plans:
**Wave 1**
- [x] 15-01-PLAN.md - Introduce one open-time-selected file-local reader backend seam in `src/archive.cpp` and lock it with dedicated runtime plus policy regression suites.

### Phase 16: Parser and Preparer Seam Extraction
**Goal**: The targeted TES4 parser and BA2 DX10 preparer hotspots become smaller internal seams that are safer to change because behavior is locked down by focused regression coverage.
**Depends on**: Phase 15
**Requirements**: REFA-01, REFA-02
**Success Criteria** (what must be TRUE):
  1. Maintainer can modify the targeted TES4 parser hotspot through smaller internal helpers with focused regression coverage catching behavior drift.
  2. Maintainer can modify the targeted BA2 DX10 preparer and staging hotspot through smaller internal helpers with focused regression coverage catching behavior drift.
  3. Supported parser and preparer fixture behavior remains unchanged while those seams are split into smaller helpers.
**Plans**: 3 plans

Plans:
**Wave 1**
- [x] 16-01-PLAN.md - Extract TES4 raw table and payload descriptor seams with focused TDD regression coverage.

**Wave 2** *(blocked on Wave 1 completion)*
- [x] 16-02-PLAN.md - Extract BA2 DX10 snapshot builder and chunk assembler seams with focused TDD regression coverage.

**Wave 3** *(blocked on Wave 1 and Wave 2 completion)*
- [x] 16-03-PLAN.md - Add role-based seam policy guardrails and run the Phase 16 verification sweep.

### Phase 17: Writer Hotspot Hardening and Ship Gate
**Goal**: The remaining high-cost and fragile writer staging paths are hardened under existing semantics, and the milestone closes with verified ship-ready evidence rather than a broad redesign.
**Depends on**: Phase 16
**Requirements**: DEDU-01, DEDU-02, DX10-01, DX10-02
**Success Criteria** (what must be TRUE):
  1. Consumer can write TES4-family archives with dedupe enabled using faster candidate narrowing while exact stored-byte equality behavior stays unchanged.
  2. Consumer can write BA2 GNRL archives with dedupe enabled using stronger staged identity or digest narrowing while exact stored-byte equality fallback behavior stays unchanged.
  3. Consumer can write BA2 DX10 archives with temporary staging data cleaned up during normal completion and ordinary failure unwinding.
  4. Maintainer can verify and document the remaining BA2 DX10 temporary-data lifecycle behavior, including any residual abnormal-termination risk, before shipping the milestone.
**Plans**: TBD

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 13. Host Path Correctness Boundary | v1.1 | 5/5 | Complete    | 2026-05-14 |
| 14. Verification Lane Truthfulness | v1.1 | 3/3 | Complete    | 2026-05-14 |
| 15. Reader Backend Dispatch Cleanup | v1.1 | 1/1 | Complete    | 2026-05-14 |
| 16. Parser and Preparer Seam Extraction | v1.1 | 3/3 | Complete   | 2026-05-14 |
| 17. Writer Hotspot Hardening and Ship Gate | v1.1 | 0/TBD | Not started | - |

## Next Scope Candidates

These are not committed roadmap items yet; they remain deferred until a later milestone promotes them.

- Optional sample CLI demonstrating library APIs without becoming the primary product.
- Public fuzzing harnesses after the strict parser and validation surfaces have settled.
- Lenient recovery mode for partially corrupt archives.
- Stable long-term binary ABI policy if libbsa is distributed as a binary package.
