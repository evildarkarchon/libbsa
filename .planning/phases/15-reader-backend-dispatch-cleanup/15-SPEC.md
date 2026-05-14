# Phase 15: Reader Backend Dispatch Cleanup - Specification

**Created:** 2026-05-13
**Ambiguity score:** 0.15 (gate: <= 0.20)
**Requirements:** 3 locked

## Goal

`archive_reader` selects one private reader backend at open time and preserves the existing consumer-visible behavior of `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries` across the currently supported reader families.

## Background

The shipped library already exposes one public `archive_reader` surface for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives. In `src/archive.cpp`, `archive_reader::open` already detects the archive bytes, chooses the matching parser path, and stores parsed metadata plus entries in reader state. The remaining public reader operations still repeat archive-family branching on `metadata.variant`, `metadata.type`, and `is_ba2_dx10` across `entries`, `find`, `contains`, `extract`, and the extraction helper that those operations reuse. Existing tests already lock large parts of reader behavior through `tests/unit/archive_reader_tests.cpp`, `tes3_bsa_reader_tests.cpp`, `tes4_bsa_reader_tests.cpp`, `ba2_gnrl_reader_tests.cpp`, `ba2_dx10_extraction_tests.cpp`, and related host-path policy tests, but there is not yet a Phase 15 guard that says this repeated public-facade dispatch must collapse into one open-time backend seam without changing behavior.

## Requirements

1. **One open-time backend seam**: The public reader surface uses one open-time-selected backend seam instead of repeating archive-family dispatch in each operation.
   - Current: `archive_reader::open` already performs archive detection and parser selection once, but `archive_reader::state` stores only metadata, entries, host path, and `is_ba2_dx10`, so later operations in `src/archive.cpp` still branch again on archive family details.
   - Target: After a successful `archive_reader::open`, reader state keeps one private backend dispatch seam or equivalent open-time selection that is reused by `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries`, so those public operations no longer each branch on `metadata.variant`, `metadata.type`, or `is_ba2_dx10`.
   - Acceptance: A focused source/policy test fails if the Phase 15 implementation reintroduces per-operation archive-family branching inside the public reader operations in `src/archive.cpp`, while allowing one centralized open-time/backend seam.

2. **Reader behavior continuity**: Dispatch cleanup preserves the current consumer-visible behavior and error semantics of the full reader surface.
   - Current: Consumers can already list entries, look up canonical paths, check containment, extract payloads, materialize `extract_bytes`, and run bulk extraction across supported families, but those outcomes currently depend on repeated family-specific branches in the public facade.
   - Target: The cleanup keeps the existing public signatures and preserves listing order, lookup results, extraction bytes, bulk extraction results, and current failure codes for the existing reader surface while routing those operations through the open-time-selected backend seam.
   - Acceptance: Focused runtime regression coverage passes for representative `TES3 BSA`, `TES4-family BSA`, `Fallout 4 BA2 GNRL`, `Starfield BA2 GNRL v3`, and `BA2 DX10` archives using the current public reader APIs, with existing expected success cases and current `not_found` / invalid-path failures still observed where they are already part of the public contract.

3. **Committed dispatch guardrails**: The phase adds regression evidence that future backend changes do not require public-facade branch duplication again.
   - Current: Family-specific runtime tests exist, but there is no explicit Phase 15 guard proving that repeated dispatch in `src/archive.cpp` has been consolidated or that the current backend coverage spans the full public reader surface.
   - Target: The repository contains both runtime regression coverage and a repo-reading/source-policy guard that together lock the new dispatch contract: behavior stays unchanged for representative backends, and the public reader facade does not drift back to per-method family branching.
   - Acceptance: The committed test suite includes both a runtime selector for the affected reader families and a source/policy check over `src/archive.cpp` that fails if future changes put archive-family branching back into the public reader operations instead of the chosen backend seam.

## Boundaries

**In scope:**
- Internal dispatch cleanup for the public `archive_reader` surface in `src/archive.cpp`
- One open-time-selected private backend seam reused by `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries`
- Behavior-preserving runtime regression coverage for representative current reader backends and BA2 GNRL variant coverage
- Source/policy guardrails that fail if public reader operations regain repeated family branching
- Preservation of the current public reader API and current reader-visible semantics while this internal cleanup lands

**Out of scope:**
- Parser hotspot extraction or BA2 DX10 preparer refactors - excluded because those belong to Phase 16
- Validation-surface behavior changes - excluded because Phase 15 is locked to the public reader surface, not `validate_archive`
- New archive-family support or reader capability expansion - excluded because v1.1 is hardening-only and this phase is semantics-preserving cleanup
- Public API redesign such as new reader-specific public types, new public methods, or replacing the current `archive_reader` shape - excluded because the user locked this phase to outcome preservation, not API expansion
- Generic plugin, registry, or extensibility frameworks - excluded because the architecture research explicitly recommends a small private seam instead of a broad framework
- Writer, dedupe, DX10 temp-staging, or broader performance work - excluded because those belong to later roadmap phases

## Constraints

- Keep the public `archive_reader` API unchanged in `include/libbsa/archive.hpp`.
- Preserve current reader-visible behavior and stable error-code outcomes; this phase is cleanup, not a behavior redesign.
- Keep the implementation Windows-only and C++20-consistent with the existing project constraints.
- Reuse the Phase 13 host-file boundary and other current internal contracts rather than reopening host-path or validation design.
- Do not require one exact private implementation shape; a private backend object, function table, helper seam, or equivalent is acceptable as long as backend selection happens once at open time and public operations stop repeating family branching.
- Do not introduce a generic plugin or registration framework to solve this phase.

## Acceptance Criteria

- [ ] `include/libbsa/archive.hpp` keeps the existing public `archive_reader` signatures, including `entries`, `find`, `contains`, `extract`, `extract_bytes`, and `extract_entries`.
- [ ] `src/archive.cpp` no longer repeats archive-family branching inside each public reader operation after open succeeds; one centralized open-time/backend seam is used instead.
- [ ] Focused runtime regression tests pass for representative `TES3 BSA`, `TES4-family BSA`, `Fallout 4 BA2 GNRL`, `Starfield BA2 GNRL v3`, and `BA2 DX10` archives through the current public reader APIs.
- [ ] Existing consumer-visible outcomes remain stable for the covered reader surface, including deterministic listing/lookup behavior, payload extraction behavior, and the current `not_found` / invalid-path failures already asserted by the repo.
- [ ] The committed test suite contains both runtime regression coverage and a repo-reading/source-policy guard that fails if public reader operations drift back to repeated archive-family branching.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.91  | 0.75  | ✓      | Goal locks one open-time backend seam plus unchanged public behavior |
| Boundary Clarity    | 0.84  | 0.70  | ✓      | Minimal dispatch cleanup is separated from parser, validation, and writer work |
| Constraint Clarity  | 0.74  | 0.65  | ✓      | API stability, semantics preservation, no generic framework, no fixed private shape |
| Acceptance Criteria | 0.86  | 0.70  | ✓      | Pass/fail checks cover source guardrails, runtime evidence, and unchanged public surface |
| **Ambiguity**       | 0.15  | <=0.20| ✓      | Gate passed after round 2 |

Status: ✓ = met minimum, ⚠ = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What is driving Phase 15, which operations are in scope, and what evidence is mandatory? | The phase is both maintainability and regression-safety work; the full current reader surface including `extract_bytes` is in scope; both runtime regression proof and source/policy guardrails are required. |
| 2 | Researcher + Simplifier | What is the minimum viable version of the phase, and should the spec lock a concrete private shape? | The irreducible core is behavior-preserving dispatch consolidation only; adjacent parser/refactor work stays out of scope; the spec locks the outcome, not one exact private helper/type design. |
| 2 | Gate | Ambiguity reached 0.15. Proceed with SPEC.md? | User selected `Yes - write SPEC.md`. |

---

*Phase: 15-reader-backend-dispatch-cleanup*
*Spec created: 2026-05-13*
*Next step: /gsd-discuss-phase 15 - implementation decisions (how to build what's specified above)*
