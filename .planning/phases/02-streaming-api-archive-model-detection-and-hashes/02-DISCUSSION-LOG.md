# Phase 02: streaming-api-archive-model-detection-and-hashes - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-05-05
**Phase:** 02-streaming-api-archive-model-detection-and-hashes
**Areas discussed:** Public API shape, Detection policy, Path/hash surface, Golden vectors

---

## Public API Shape

| Question | Options | User's choice |
|----------|---------|---------------|
| For the random-access source, which public shape should Phase 2 lock? | Abstract base class; Function callbacks; Value wrapper only; You decide | Abstract base class |
| How should Phase 2 express ownership and lifetime for sources, sinks, and archive views? | Caller-owned references; Shared ownership; Move-only ownership; You decide | Caller-owned references |
| For small in-memory helpers, how much should be public in Phase 2? | Memory source/sink only; Add file source too; Tests only; You decide | Memory source/sink only |
| What should archive metadata and lookup APIs return for missing entries? | Boolean plus result lookup; Optional metadata; Pointer/null view; You decide | Boolean plus result lookup |

**Notes:** The chosen API style emphasizes reusable consumer adapters, explicit caller-owned lifetimes, metadata views that own copied metadata, and result-based error handling consistent with Phase 1.

---

## Detection Policy

| Question | Options | User's choice |
|----------|---------|---------------|
| When detection sees a supported magic with an unsupported version, what should it return? | Structured unsupported result; Unknown format; Partial metadata warning; You decide | Structured unsupported result |
| How strict should truncated header detection be? | Fail immediately; Best-effort family; Two-level API; You decide | Fail immediately |
| What should the detection result include on success? | Identity only; Header summary; Full archive summary; You decide | Header summary |
| For ambiguous BA2 inputs, which field should decide GNRL vs DDS? | Subtype field only; Subtype plus extension; Defer DDS detection; You decide | Subtype field only |

**Notes:** Detection should be strict about malformed inputs while still returning useful bounded header metadata for valid samples.

---

## Path/Hash Surface

| Question | Options | User's choice |
|----------|---------|---------------|
| Should archive path normalization be part of the public API? | Public path API; Internal only; Test-visible only; You decide | Public path API |
| What representation should archive_path expose to consumers? | UTF-8 string value; Raw byte string; String view only; You decide | UTF-8 string value |
| Which hash functions should be public versus internal? | Public documented hashes; Public normalization, internal hashes; All internal; You decide | Public normalization, internal hashes |
| How should invalid archive path input be handled? | Result-returning normalization; Always normalize string; Throw on invalid; You decide | Result-returning normalization |

**Notes:** Consumers get stable path normalization and lookup behavior, but exact hash functions stay internal or test-visible until a consumer need proves they should be public.

---

## Golden Vectors

| Question | Options | User's choice |
|----------|---------|---------------|
| Where should Phase 2 golden vectors live? | Tests data source file; Planning artifact only; JSON/CSV fixture file; You decide | Tests data source file |
| How should reference provenance be documented for each vector group? | Inline test comments; Compatibility notes doc; Commit message only; You decide | Inline test comments |
| How broad should the initial golden vector set be? | Representative minimal set; Exhaustive known cases; One smoke vector per family; You decide | Representative minimal set |
| If reference tracing finds uncertain or contradictory hash behavior during Phase 2, what should the executor do? | Stop and document blocker; Choose BSArchPro behavior; Mark TODO vector; You decide | Stop and document blocker |

**Notes:** Golden vectors should be executable test evidence with nearby provenance, not planning-only claims. Uncertain compatibility behavior is a blocker, not something to guess through.

---

## Claude's Discretion

None. The user made explicit choices for all selected gray areas.

## Deferred Ideas

None.
