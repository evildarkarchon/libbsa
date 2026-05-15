---
phase: 15
phase_name: "reader-backend-dispatch-cleanup"
project: "libbsa"
generated: "2026-05-14"
counts:
  decisions: 4
  lessons: 2
  patterns: 4
  surprises: 2
missing_artifacts:
  - "15-UAT.md"
---

# Phase 15 Learnings: reader-backend-dispatch-cleanup

## Decisions

### Select Reader Backend Once At Open Time
`archive_reader` now selects a file-local backend table once during `open()` and reuses it for `entries`, `find`, and payload extraction.

**Rationale:** This removes repeated public-method family dispatch while preserving the existing reader contract.
**Source:** 15-01-SUMMARY.md

---

### Keep `contains()` As A Facade Over `find()`
`contains()` remains implemented as `find()` plus `has_value()`.

**Rationale:** Malformed archive-internal paths must keep their existing `invalid_argument` behavior instead of collapsing to `false`.
**Source:** 15-01-SUMMARY.md

---

### Keep Bulk Orchestration In `extract_entries()`
`extract_entries()` keeps duplicate exact-request coalescing, sink creation, and per-request result mirroring in shared facade code while backend callbacks stay limited to lookup and payload extraction.

**Rationale:** Shared bulk semantics stay centralized while backend-specific logic remains narrow.
**Source:** 15-01-SUMMARY.md

---

### Keep The Dispatch Seam File-Local
The reader-backend dispatch seam stays file-local to `src/archive.cpp` instead of expanding into a wider private dispatch API.

**Rationale:** The phase only needed a localized structural cleanup, not a broader internal abstraction.
**Source:** 15-01-SUMMARY.md

---

## Lessons

### Avoid Over-Assuming Fixture Manifest Shape
The runtime dispatch suite cannot assume every representative fixture exposes `lookup_variants`, `raw_size`, and `stored_size`.

**Context:** BA2 DX10 manifest entries may omit those fields, so the final test helper falls back to canonical path lookup and only compares size metadata when the manifest provides it.
**Source:** 15-01-SUMMARY.md

---

### Keep Final State Assembly Inside `open()` When Access Control Matters
A free helper returning `archive_reader::state` from outside the class hit C++ access control during implementation.

**Context:** Final state assembly stayed inside `archive_reader::open` via a local lambda so the single open-time selection seam could be preserved without widening access.
**Source:** 15-01-SUMMARY.md

---

## Patterns

### Phase-Scoped Source-Policy Tests For Negative Dispatch Invariants
Phase-scoped source-policy tests can enforce "no repeated public-method family dispatch" without freezing one helper or callback name.

**When to use:** Use when structural invariants matter more than exact helper naming and the regression should be caught from repository source alone.
**Source:** 15-01-SUMMARY.md

---

### Representative Cross-Family Runtime Dispatch Coverage
Representative reader regression suites can lock `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries` behavior across TES3, TES4, BA2 GNRL, Starfield BA2 GNRL v3, and BA2 DX10 fixtures.

**When to use:** Use when a shared public facade must preserve both success-path behavior and `not_found` or `invalid_argument` semantics across multiple backends.
**Source:** 15-01-SUMMARY.md

---

### Reuse The Stored Resolved Host Path For Payload Reopens
Payload extraction continues to reopen archive data through the stored resolved `detail::host_file_path` after `open()` succeeds.

**When to use:** Use when later I/O must stay on the resolved host-file boundary rather than reinterpreting raw caller UTF-8 text.
**Source:** 15-VERIFICATION.md

---

### Pair Seam-Specific Quick Loops With Adjacent Contract Coverage
The Phase 15 quick loop runs `reader_backend_dispatch` and `archive_reader_dispatch_policy` together with `bulk_extraction` coverage.

**When to use:** Use when a structural refactor changes dispatch wiring but must also preserve nearby shared orchestration behavior that lives outside the new seam.
**Source:** 15-01-PLAN.md

---

## Surprises

### BA2 DX10 Manifests Omit Common Lookup And Size Fields
BA2 DX10 manifest entries did not always expose `lookup_variants`, `raw_size`, or `stored_size`, which broke the first draft of the runtime dispatch suite.

**Impact:** Test helpers had to become tolerant of manifest-shape differences instead of assuming one metadata layout across all representative fixtures.
**Source:** 15-01-SUMMARY.md

---

### `backend_identity` Remained Stored But Unused Post-Open
Verification found that `backend_identity` is still stored in reader state even though post-open behavior currently dereferences only `backend_table`.

**Impact:** This leaves minor maintainability debt because parallel fields must stay in sync until the unused identity is removed or justified.
**Source:** 15-VERIFICATION.md
