# Roadmap: libbsa

## Milestones

- [x] **v1.0 Complete Library** - Phases 1-12 shipped 2026-05-10.
  Archives: [roadmap](milestones/v1.0-ROADMAP.md), [requirements](milestones/v1.0-REQUIREMENTS.md), [audit](milestones/v1.0-MILESTONE-AUDIT.md), [phase artifacts](milestones/v1.0-phases/).
- [ ] **Next milestone** - Not planned yet. Start with `$gsd-new-milestone` to define fresh requirements and phase scope.

## Phases

<details>
<summary>v1.0 Complete Library (Phases 1-12) - SHIPPED 2026-05-10</summary>

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

No active phases are currently planned. The next milestone should define a new `.planning/REQUIREMENTS.md` and append new phases after Phase 12.

## Progress

| Milestone | Phase Range | Plans | Requirements | Status | Completed |
|-----------|-------------|-------|--------------|--------|-----------|
| v1.0 Complete Library | 1-12 | 75/75 | 81/81 | Shipped | 2026-05-10 |

## Next Scope Candidates

These are not committed roadmap items yet; they were deferred from v1 requirements and need fresh milestone discussion before promotion.

- Optional sample CLI demonstrating library APIs without becoming the primary product.
- Public fuzzing harnesses after the strict parser and validation surfaces have settled.
- Lenient recovery mode for partially corrupt archives.
- Stable long-term binary ABI policy if libbsa is distributed as a binary package.
