# Retrospective

## Milestone: v1.0 - Complete Library

**Shipped:** 2026-05-10  
**Phases:** 12  
**Plans:** 75  
**Requirements:** 81/81 complete

### What Was Built

v1.0 turned libbsa from a project skeleton into a reusable C++20 Bethesda archive library. The release includes dependency-light public APIs, read/extract support for BSA and BA2 families, write-new support for BSA and BA2 families, validation reports, compatibility warnings, generated legal fixtures, bounded-memory workflows, optional parallel execution, benchmark reporting, Doxygen configuration, thread-safety guidance, and compile-checked consumer examples.

### What Worked

- The read-first roadmap paid off: writer phases could prove output by reopening generated archives through the public reader API.
- Generated legal fixtures kept the test suite reproducible without mutating `TES5Edit/` or relying on copyrighted game archives.
- Keeping libdeflate, lz4, and DirectXTex behind internal adapters preserved the public header boundary while still enabling real compression and DDS validation.
- GSD verification artifacts caught stale roadmap, validation, and audit bookkeeping before milestone close.

### What Was Inefficient

- Some validation files lagged behind completed implementation and needed a quick-task reconciliation before milestone close.
- The active `gsd-sdk audit-open` scanner still expects quick tasks to expose a bare `SUMMARY.md`; the quick task used the newer `${quick_id}-SUMMARY.md` shape and needed a compatibility shim.
- The generated milestone entry captured every plan summary one-liner, which was accurate but too large for a living milestone index.

### Patterns Established

- Public writer acceptance should reopen output with `archive_reader` and compare extracted bytes or DDS metadata rather than relying only on byte-table inspection.
- Compatibility and fixture policy should be machine-checked so warning-code, target-format, and fixture-boundary drift fails loudly.
- Bounded-memory claims need source-text policy checks plus large synthetic fixture coverage, not just code inspection.
- Optional local game or BSArchPro-derived comparisons should stay opt-in and skipped unless local fixture environment variables are set.

### Key Lessons

- Archive parsers and writers benefit from explicit target-family routing; extension inference is too fragile for Bethesda formats.
- DDS and compression details belong behind internal boundaries, with libbsa-owned metadata crossing the public API.
- GSD close-out is smoother when verification, roadmap, requirements, and state artifacts are reconciled as soon as each phase finishes.
- Tooling compatibility matters: when the workflow and SDK disagree on artifact names, add a small compatibility artifact rather than hiding the audit result.

### Cost Observations

- Main cost driver was the number of format variants and validation surfaces, not individual implementation size.
- The highest-value checks were reader-backed writer tests, malformed fixture matrices, public header boundary tests, and source-text policy tests.
- The milestone now has enough coverage that future work should prefer targeted v2 requirements over reopening v1 scope broadly.

## Cross-Milestone Trends

| Trend | v1.0 Observation | Next Watch |
|-------|------------------|------------|
| Compatibility evidence | Strong generated fixture and policy-test base | Add real corpus comparisons only through opt-in local fixtures |
| Public API boundary | Dependency-light C++20 headers held through v1.0 | Re-check before ABI or package publishing work |
| Planning artifact drift | Final quick task was needed to reconcile stale validation files | Keep phase verification updated at completion time |
