# Phase 11: Compatibility Validation and Hardening - Specification

**Created:** 2026-05-07
**Ambiguity score:** 0.15 (gate: <= 0.20)
**Requirements:** 7 locked

## Goal

Maintainers can prove every currently supported libbsa read/write target matches reference-compatible metadata and extracted bytes, and malformed or quirk-sensitive inputs fail or report explicitly without crashes, unchecked allocations, or silent corruption.

## Background

The codebase already has generated Catch2 fixture and round-trip coverage for hash behavior, compression routing, BSA readers/writers, BA2 GNRL readers/writers, BA2 DDS readers/writers, and public-header smoke usage. `CMakeLists.txt` registers `fixture`, `roundtrip`, `codec`, `golden`, and `smoke` labels, but there is no dedicated `compat` validation target yet. `README.md` repeatedly defers real game archive corpus checks and BSArchPro byte-for-byte comparisons to this phase. Phase 10 verification also records unresolved BA2 writer compatibility blockers: native BA2 extension fields include the leading dot, multi-mip DDS arrays/cubemaps can reorder payload bytes, and cubemap arrays are accepted even though current native serialization collapses them.

## Requirements

1. **Known BA2 writer blockers**: Phase 11 must close the known BA2 writer compatibility blockers before broader compatibility validation can pass.
   - Current: Phase 10 verification reports wrong native extension bytes, unsafe multi-mip array/cubemap payload ordering, and accepted cubemap arrays that lose cube count.
   - Target: BA2 GNRL/DX10 native extension fields use extension text without the separator, accepted multi-mip array/cubemap inputs preserve source/extracted DDS payload semantics, and unrepresentable cubemap-array inputs are either preserved correctly or rejected structurally during planning.
   - Acceptance: Focused tests fail on the old behaviors and pass only when native extension bytes, multi-mip array/cubemap payload semantics, and cubemap-array behavior are correct or explicitly rejected.

2. **All-target fixture matrix**: Phase 11 must provide a generated or legally committable small-archive fixture matrix for every supported read/write target.
   - Current: Generated fixtures exist across individual test files, but there is no single Phase 11 matrix proving coverage for all supported targets.
   - Target: Fixture coverage includes TES3 BSA, TES4-family BSA v103/v104/v105, Fallout 4 BA2 GNRL v1/v7/v8, Starfield BA2 GNRL v2/v3, Fallout 4 BA2 DDS/DX10 v1/v7/v8, Starfield BA2 DDS/DX10 v3, and libbsa-written BSA/BA2 outputs for those supported writer targets.
   - Acceptance: A maintainer can identify one passing generated or committed-small fixture test for each supported target in the matrix, and missing target coverage fails the compatibility validation suite.

3. **Reference comparison manifest**: Phase 11 must support optional BSArchPro/reference corpus comparison without requiring proprietary archives in the repository.
   - Current: No external corpus manifest or reference comparison path exists; previous phases only document that real archive and BSArchPro comparisons are deferred.
   - Target: A source-controlled manifest format can describe local external archives and expected reference metadata/extracted bytes, while generated fixtures remain the always-available required corpus.
   - Acceptance: When the external corpus is absent, comparison tests skip with a clear notice and generated fixture validation still runs; when a valid manifest is present, comparison tests fail on metadata or extracted payload byte mismatches.

4. **Metadata and payload compatibility proof**: Phase 11 compatibility checks must compare both archive metadata and extracted payload bytes.
   - Current: Several writer tests reopen output through libbsa, but Phase 10 verification shows libbsa read-back can miss native compatibility bytes and metadata issues.
   - Target: Compatibility assertions cover normalized paths, archive identity/version/subtype, compression state, offsets/sizes where observable, texture metadata for DDS entries, and extracted payload bytes for the same corpus entry.
   - Acceptance: A deliberate mismatch in expected metadata or extracted payload bytes causes the relevant compatibility test to fail.

5. **Malformed-input hardening gate**: Phase 11 must add cross-family malformed and truncated archive validation for all supported archive families.
   - Current: Malformed tests exist per feature area, but there is no single hardening gate spanning every supported archive family and writer output path.
   - Target: Negative fixtures cover invalid magic/version/subtype, truncated tables, impossible offsets/sizes, mismatched counts, duplicate normalized names where malformed, codec route confusion, decompression failure, unsupported DDS reconstruction, and writer layout overflow cases across supported families.
   - Acceptance: The malformed-input suite completes without process crashes, unchecked allocations, or partial successful extraction/finalization, and failures return structured libbsa errors.

6. **Public quirk report**: Phase 11 must expose consumer-visible quirk reporting for the named Bethesda compatibility hazards.
   - Current: Public archive metadata and result errors exist, but no public warning or quirk reporting surface exists for compressed sounds, SSE `EMBEDNAME` hazards, or Fallout vanilla zlib tolerance.
   - Target: Consumers can inspect an explicit quirk report or equivalent public diagnostic result for compressed sound entries, SSE `EMBEDNAME` hazard cases, and Fallout vanilla zlib tolerance behavior.
   - Acceptance: Tests construct or load representative cases for all three named quirks and verify the public consumer-facing output identifies each quirk deterministically.

7. **Compatibility test entry point**: Phase 11 must make compatibility validation runnable through CTest without manual test selection.
   - Current: `compat` is documented as a reserved label, but no compatibility suite or target is registered.
   - Target: Maintainers can run a documented CTest command or label that executes the all-target generated fixtures, malformed hardening checks, quirk-report checks, and optional external manifest comparison behavior.
   - Acceptance: `ctest` with the compatibility label or documented filter runs the Phase 11 compatibility suite, reports skips for absent external corpus inputs, and fails on any required generated fixture, malformed-input, quirk-report, or metadata/payload mismatch failure.

## Boundaries

**In scope:**
- Close the known Phase 10 BA2 writer compatibility blockers before declaring Phase 11 compatibility validation successful.
- Generated or legally committable small fixtures for every currently supported archive family and writer target.
- Optional external corpus manifest support that skips cleanly when local proprietary inputs are absent.
- Metadata and extracted-payload byte comparisons for compatibility proof.
- Cross-family malformed/truncated input hardening with structured libbsa failures.
- Public consumer-visible quirk reporting for compressed sounds, SSE `EMBEDNAME` hazards, and Fallout vanilla zlib tolerance.
- CTest integration for a compatibility validation entry point.
- Documentation of the Phase 11 validation command, external corpus setup, skip behavior, and quirk-report semantics.

**Out of scope:**
- Committing proprietary game archives or BSArchPro-generated proprietary outputs to the repository - fixtures must be generated, source-reviewable, or legally committable.
- Editing, formatting, compiling, linking, staging, or updating `TES5Edit/` - it remains read-only reference material.
- Building a productized CLI or GUI archive validation tool - libbsa remains a reusable library and CTest-driven validation is sufficient for this phase.
- Performance benchmarking, multi-threaded extraction/packing, or bulk extraction orchestration - those belong to Phase 12.
- Doxygen API documentation for the entire library - Phase 11 documents validation and quirk behavior only.
- Texture transcoding, resizing, optimization, or broad DDS format expansion beyond compatibility cases justified by the validation corpus.
- True in-place archive mutation - rebuild-style writer outputs remain the supported model.

## Constraints

- `TES5Edit/` must remain unmodified, uncompiled, unlinked, unstaged, and unformatted.
- External corpus support must not make proprietary game archives mandatory for normal contributor test runs.
- Absence of the external BSArchPro/reference corpus must produce a clear skip notice, not a failed generated-fixture validation run.
- Public headers must continue to avoid leaking libdeflate, LZ4, DirectXTex, Windows SDK, Delphi, UI/tooling, or TES5Edit implementation types.
- Compatibility claims for BA2 writers cannot pass while the known extension-byte, multi-mip array/cubemap, or cubemap-array blockers remain open.
- Compatibility proof must include both metadata and extracted payload bytes; extracted bytes alone are not sufficient.

## Acceptance Criteria

- [ ] Known Phase 10 BA2 writer blockers are fixed or structurally rejected with focused regression tests.
- [ ] Generated or legally committable fixture coverage exists for every supported read/write target listed in the all-target matrix.
- [ ] Optional external corpus comparison skips with a clear notice when absent and fails on metadata or extracted payload byte mismatches when present.
- [ ] Compatibility assertions compare metadata and extracted payload bytes, not only libbsa read-back success.
- [ ] Cross-family malformed/truncated archive tests complete without crashes, unchecked allocations, partial writes, or unstructured failures.
- [ ] Public consumer-visible quirk reporting identifies compressed sounds, SSE `EMBEDNAME` hazards, and Fallout vanilla zlib tolerance cases.
- [ ] A documented CTest compatibility entry point runs generated fixtures, malformed hardening, quirk-report checks, and optional external comparison behavior.
- [ ] `git status --short TES5Edit` is empty after Phase 11 work.

## Ambiguity Report

| Dimension           | Score | Min   | Status | Notes |
|---------------------|-------|-------|--------|-------|
| Goal Clarity        | 0.91  | 0.75  | PASS   | Goal locks all supported targets, metadata plus bytes, hardening, and quirk reporting. |
| Boundary Clarity    | 0.84  | 0.70  | PASS   | In-scope and out-of-scope lists separate validation from performance, CLI/GUI, Doxygen, and proprietary corpus handling. |
| Constraint Clarity  | 0.78  | 0.65  | PASS   | External corpus skip behavior, TES5Edit boundary, public-header leakage, and BA2 blocker constraints are explicit. |
| Acceptance Criteria | 0.82  | 0.70  | PASS   | Acceptance criteria are pass/fail and tied to CTest-observable behavior. |
| **Ambiguity**       | 0.15  | <=0.20| PASS   | Gate passed after round 2. |

Status: PASS = met minimum, WARN = below minimum (planner treats as assumption)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | Should Phase 11 include known Phase 10 BA2 writer blockers? | Include the blockers in Phase 11 so compatibility validation cannot pass while they remain open. |
| 1 | Researcher | What evidence counts as sufficient reference compatibility proof? | Compare metadata and extracted payload bytes. |
| 1 | Researcher | What corpus source is the minimum required validation set? | Use generated/committed-small fixtures plus an optional external manifest for BSArchPro/reference comparison. |
| 2 | Researcher + Simplifier | What archive-family matrix is required? | Cover all currently supported read/write targets. |
| 2 | Researcher + Simplifier | What happens when the external corpus is absent? | Required generated fixture validation still runs; external comparison tests skip with a clear notice. |
| 2 | Researcher + Simplifier | What is the minimum consumer-visible quirk result? | Add a public quirk report for compressed sounds, SSE `EMBEDNAME` hazards, and Fallout vanilla zlib tolerance. |

---

*Phase: 11-compatibility-validation-and-hardening*
*Spec created: 2026-05-07*
*Next step: /gsd-discuss-phase 11 - implementation decisions only*
