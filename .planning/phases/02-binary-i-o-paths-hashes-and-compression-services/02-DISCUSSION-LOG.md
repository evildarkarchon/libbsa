# Phase 02: Binary I/O, Paths, Hashes, and Compression Services - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-08
**Phase:** 02-binary-i-o-paths-hashes-and-compression-services
**Areas discussed:** Public surface boundaries, Path normalization rules, Streaming contract shape, Proof and reference standard

---

## Public Surface Boundaries

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| What should be allowed into public headers immediately? | Minimal public only | Minimal public only; Path and stream types; All reusable primitives; You decide |
| Where should a Phase 2 archive path type live? | Internal detail namespace | Internal detail namespace; Public focused header; Public but experimental; You decide |
| Should Phase 2 change `archive_reader::open` or other public signatures? | No public API changes | No public API changes; Add overloads now; Revise open shape; You decide |
| How should dependency-linked services appear at the API boundary? | Private adapters only | Private adapters only; Public codec facade; Test-only adapters; You decide |

**Notes:** User selected the conservative boundary throughout: keep Phase 2 primitives internal and avoid public API churn.

---

## Path Normalization Rules

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| What should the canonical archive path key do with case? | Fold to lowercase | Fold to lowercase; Preserve input case; Family-specific only; You decide |
| How should separators be normalized? | Canonical forward slash | Canonical forward slash; Canonical backslash; Preserve separators; You decide |
| How strict should invalid archive virtual paths be? | Reject obvious bad paths | Reject obvious bad paths; Accept broadly; Strict Bethesda subset; You decide |
| Should normalized paths preserve original spelling? | Canonical only in Phase 2 | Canonical only in Phase 2; Keep original plus key; Family-specific metadata; You decide |

**Notes:** Canonical keys should be lowercase, forward-slash paths with obvious invalid inputs rejected. Original/display spelling can wait for later archive metadata records.

---

## Streaming Contract Shape

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| Should the Phase 2 streaming contract be public or internal? | Internal only now | Internal only now; Public sink only; Public source and sink; You decide |
| What behavior matters most for primitive tests? | Exact bytes and failures | Exact bytes and failures; Performance shape; Callback ergonomics; You decide |
| Should Phase 2 define async or threaded streaming? | No, synchronous only | No, synchronous only; Cancellation-ready hooks; Async-ready API shape; You decide |
| How should chunk sizing be treated? | Implementation detail | Implementation detail; Fixed project constant; Caller-configurable now; You decide |
| Should internal streaming be based on callbacks or object interfaces? | Simple object interfaces | Simple object interfaces; Callbacks/functions; Span-only helpers; You decide |
| What should happen if a sink accepts only part of a chunk? | Treat as error | Treat as error; Retry remainder; Allow partial success; You decide |
| Should a source know the total expected payload size? | Yes for bounded payloads | Yes for bounded payloads; No, stream until EOF; Optional size metadata; You decide |
| Should Phase 2 include file-backed sources/sinks or memory/test doubles only? | Memory plus test doubles | Memory plus test doubles; Add file-backed too; File-backed source only; You decide |

**Notes:** User requested one extra round for streaming, then locked internal synchronous object-style bounded streaming with exact byte/failure proof.

---

## Proof And Reference Standard

| Question | Selected | Options Considered |
|----------|----------|--------------------|
| What should be the proof standard for hash algorithms? | Reference-traced constants | Reference-traced constants; Generated fixtures first; Independent implementations; You decide |
| For compression codecs, what should tests emphasize most? | Round-trip plus corruptions | Round-trip plus corruptions; Known external vectors; Generated archive payloads; You decide |
| How should Phase 2 handle TES5Edit reference use in code comments and tests? | Document non-obvious rules | Document non-obvious rules; Avoid source citations; Detailed trace docs; You decide |
| What fixture policy should Phase 2 use for primitive tests? | Unit vectors first | Unit vectors first; Generated fixtures now; No binary fixtures; You decide |

**Notes:** User chose reference-traced hash constants, codec round-trip/corruption/size-mismatch tests, comments for non-obvious TES5Edit-derived rules, and unit vectors before generated binary fixtures.

---

## Agent Discretion

No major decision was delegated with "You decide." Planner retains normal discretion for exact internal file names, helper names, namespace layout, and test split.

## Deferred Ideas

None.
