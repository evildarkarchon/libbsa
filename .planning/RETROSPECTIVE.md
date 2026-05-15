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

## Milestone: v1.1 - Hardening

**Shipped:** 2026-05-15  
**Phases:** 6  
**Plans:** 20  
**Requirements:** 14/14 complete

### What Was Built

v1.1 hardened the shipped library without expanding public scope. It fixed non-ASCII Windows host-path handling across reader and validation flows, made Release and MSVC ASan verification lanes truthful, collapsed reader dispatch to one open-time seam, extracted TES4 parser and BA2 DX10 preparer hotspots into private seams, hardened TES4 and BA2 GNRL dedupe paths, implemented BA2 DX10 ordinary-path snapshot cleanup, and closed audit follow-up debt through Phase 17.1.

### What Worked

- The correctness-boundary-first sequence kept later reader, parser, validation, and writer hardening on one shared host-file model.
- Source-policy tests were effective for preventing regression of architectural seams without over-constraining exact helper names.
- Focused Debug and MSVC ASan gates gave fast confidence for risky writer and host-path changes before full-suite or Release package checks.
- Phase 17.1 was a useful closure mechanism for converting audit warnings into targeted tests and private-state cleanup.

### What Was Inefficient

- The original v1.1 audit found real but non-blocking debt after the nominal ship gate, which required an inserted closure phase.
- Open artifact audit still reported older quick-task tracking artifacts as missing and required manual acknowledgement at close.
- Some generated milestone archive output needed manual shaping to match the archive template and keep the live roadmap compact.

### Patterns Established

- Resolve host paths once and pass resolved internal contracts through later I/O seams.
- Keep non-authoritative dedupe filters separate from exact final stored-byte equality checks.
- Use policy tests for docs, CI, presets, planning, and source seams when drift would otherwise be silent.
- Record accepted residual risk explicitly when the milestone intentionally avoids stronger guarantees.

### Key Lessons

- Hardening milestones should reserve space for audit-closure work before the final completion command.
- Verification-lane truthfulness must be tested as a cross-surface contract, not just documented in README or presets.
- Archive writer lifecycle guarantees need both implementation tests and public documentation policy checks.
- Moving phase artifacts to milestone archives keeps active planning compact, but downstream commands must use archived paths afterward.

### Cost Observations

- The highest-value work was targeted seam extraction plus policy testing, not broad rewrites.
- The largest context cost came from planning/archive artifacts and GSD workflow installation, not library code alone.
- Future milestones should keep requirements narrow and explicitly decide whether optional local-corpus, fuzzing, recovery, or ABI work is in scope.

## Cross-Milestone Trends

| Trend | v1.0 Observation | Next Watch |
|-------|------------------|------------|
| Compatibility evidence | Strong generated fixture and policy-test base | Add real corpus comparisons only through opt-in local fixtures |
| Public API boundary | Dependency-light C++20 headers held through v1.0 | Re-check before ABI or package publishing work |
| Planning artifact drift | Final quick task was needed to reconcile stale validation files | Keep phase verification updated at completion time |
| Hardening closure | v1.1 converted audit warnings into Phase 17.1 tests and cleanup | Budget explicit audit-closure capacity before milestone close |
| Verification lanes | v1.1 made Debug, Release package-proof, and MSVC ASan lanes first-class | Keep new milestone claims tied to checked-in presets and CTest evidence |
