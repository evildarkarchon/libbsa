# Roadmap: libbsa

## Milestones

- [x] **v1.0 Complete Library** - Phases 1-12 shipped 2026-05-10. Archives: [roadmap](milestones/v1.0-ROADMAP.md), [requirements](milestones/v1.0-REQUIREMENTS.md), [audit](milestones/v1.0-MILESTONE-AUDIT.md), [phase artifacts](milestones/v1.0-phases/).
- [x] **v1.1 Hardening** - Phases 13-17.1 shipped 2026-05-15 for host-path correctness, verification-lane truthfulness, reader/parser/preparer seam cleanup, writer hotspot hardening, and audit-debt closure. Archives: [roadmap](milestones/v1.1-ROADMAP.md), [requirements](milestones/v1.1-REQUIREMENTS.md), [audit](milestones/v1.1-MILESTONE-AUDIT.md), [phase artifacts](milestones/v1.1-phases/).

## Current Status

No active milestone is defined. Start the next milestone with `/gsd-new-milestone`, which will create fresh requirements and a new roadmap section.

## Next Scope Candidates

These are not committed roadmap items yet; they remain deferred until a later milestone promotes them.

- Optional sample CLI demonstrating library APIs without becoming the primary product.
- Public fuzzing harnesses after the strict parser and validation surfaces have settled.
- Lenient recovery mode for partially corrupt archives.
- Stable long-term binary ABI policy if libbsa is distributed as a binary package.
