# Phase 16: Parser and Preparer Seam Extraction - Specification

**Created:** 2026-05-14
**Ambiguity score:** 0.14 (gate: <= 0.20)
**Requirements:** 5 locked

## Goal

The targeted TES4 BSA parser and BA2 DX10 writer preparer hotspots change from monolithic mixed-concern implementations into smaller internal seams with focused regression evidence while preserving existing parser, writer, and fixture behavior.

## Background

The v1.1 hardening roadmap identifies Phase 16 as the structural cleanup slice after Phase 15's reader dispatch cleanup. The current TES4-family parser lives primarily in `src/formats/bsa/tes4_bsa_parser.cpp`, where header validation, metadata table sizing, folder and file record parsing, name parsing, hash validation, embedded-name prefix handling, payload-span validation, and entry materialization are coordinated in one translation unit through file-local helpers. The current BA2 DX10 preparer lives primarily in `src/formats/ba2/ba2_dx10_prepare.cpp`, where DDS source loading, DirectXTex analysis handoff, snapshot temp-file staging, texture chunk planning, raw-byte collection, compression routing, record metadata derivation, entry validation, and sorting remain concentrated in one mixed-concern implementation.

Existing tests already cover large parts of the public behavior through `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/writer_stage_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, and related archive-reader and writer execution tests. The remaining gap is not missing end-user capability; it is that maintainers still have to edit broad compatibility-sensitive parser/preparer files for small changes, and there is no phase-specific proof that the new seams exist and remain behavior-preserving. Phase 16 therefore locks a narrow internal refactor only: both hotspots must be split behind private/internal seams, and regression evidence must catch behavior drift without expanding public APIs or claiming Phase 17 dedupe and temp-lifecycle work.

## Requirements

1. **TES4 parser seam extraction**: The targeted TES4-family BSA parser hotspot is split into smaller internal parser seams while preserving the current parser entrypoints and supported archive behavior.
   - Current: `src/formats/bsa/tes4_bsa_parser.cpp` contains file-local helpers, but `parse_tes4_bsa_archive_impl` still coordinates header validation, table sizing, folder/file table parsing, name parsing, hash checks, embedded-name prefix handling, payload-span validation, and entry materialization inside one broad implementation surface.
   - Target: The TES4 parser keeps its existing internal/public call points, including `parse_tes4_bsa_archive`, `parse_tes4_bsa_archive_file`, and `parse_tes4_bsa_metadata`, but the targeted table parsing and entry-materialization responsibilities are available through smaller private/internal helpers or modules that can be changed independently.
   - Acceptance: A maintainer can identify at least two smaller TES4 parser seams that separate table/header parsing concerns from entry materialization or payload-prefix metadata concerns, and TES4 open/list/lookup/extract/malformed fixture tests still pass with unchanged expected error-code behavior.

2. **TES4 parser regression evidence**: Focused TES4 parser coverage catches behavior drift in the extracted seams.
   - Current: TES4 behavior is covered mostly through public reader tests and generated fixtures, but there is no Phase 16-specific guard proving that the seam extraction preserves key parser invariants while reducing the hotspot surface.
   - Target: The test suite contains focused evidence for the extracted TES4 parser responsibilities, covering success metadata and at least one compatibility-sensitive malformed or boundary case already represented by the current parser behavior.
   - Acceptance: The committed tests fail if the extracted TES4 seams regress metadata sizing/table parsing, folder/file name handling, hash validation, duplicate canonical path rejection, embedded-name prefix sizing, or payload-span-over-metadata rejection for the covered cases.

3. **BA2 DX10 preparer seam extraction**: The BA2 DX10 writer preparer hotspot is split into smaller internal preparation seams while preserving current writer behavior.
   - Current: `src/formats/ba2/ba2_dx10_prepare.cpp` mixes DDS loading and analysis handoff, writer-entry creation, snapshot temp-file staging, subresource collection, chunk raw-byte assembly, compression routing, target validation, metadata derivation, entry sorting, and worker-count chunk preparation in one broad implementation surface.
   - Target: The BA2 DX10 preparer keeps the current internal writer surface, including `ba2_dx10_make_writer_entry`, `ba2_dx10_validate_entries`, `ba2_dx10_prepare_chunk`, and `ba2_dx10_prepare_entries`, but moves snapshot/staging and chunk-preparation responsibilities behind smaller internal helpers or modules that can be tested and changed independently.
   - Acceptance: A maintainer can identify smaller BA2 DX10 preparation seams for snapshot/subresource staging and planned-chunk preparation, and BA2 DX10 add-file/write/round-trip/stage tests still pass for Fallout 4 and Starfield v3 deflate/LZ4 behavior.

4. **BA2 DX10 preparer regression evidence**: Focused BA2 DX10 coverage catches behavior drift in staging and chunk preparation.
   - Current: `writer_stage_tests.cpp` and BA2 DX10 writer tests cover many outcomes, but Phase 16 has no explicit guard that the extracted staging/chunk seams preserve source snapshot immutability, chunk ordering, target compression routing, and pre-publish failure behavior.
   - Target: The test suite contains focused evidence for the extracted BA2 DX10 preparation responsibilities, including snapshot-backed chunk assembly and at least one failure path that must occur before publish changes the destination.
   - Acceptance: The committed tests fail if multi-mip/array/cubemap chunk ordering changes, if source mutation after `add_file` is no longer isolated by snapshots, if Fallout 4 vs Starfield v3 compression routing changes for covered cases, or if truncated/missing snapshot data can publish a modified output.

5. **Scope and seam guardrails**: The phase adds maintainability guardrails without changing product scope or later-phase responsibilities.
   - Current: v1.1 requires no public API expansion, and Phase 17 owns dedupe optimization plus BA2 DX10 temporary-data lifecycle cleanup proof; however, the Phase 16 hotspots are adjacent to public reader/writer APIs, host-path seams, dedupe paths, and temp-staging cleanup concerns.
   - Target: Phase 16 changes stay internal, preserve the existing public headers and existing archive behavior, avoid modifications under `TES5Edit/`, and add either source/policy coverage or equivalent reviewable evidence that the targeted hotspot responsibilities have moved into smaller seams without broad redesign.
   - Acceptance: `include/libbsa/` public API signatures remain unchanged for reader and writer consumers, no files under `TES5Edit/` are modified, no new external dependencies are introduced, Phase 17 dedupe/temp-lifecycle requirements remain open, and a source/policy test or equivalent committed evidence fails if the targeted hotspot cleanup is removed or collapsed back into one monolithic mixed-concern implementation.

## Boundaries

**In scope:**
- Internal seam extraction for the targeted TES4-family BSA parser hotspot in `src/formats/bsa/tes4_bsa_parser.cpp` or adjacent private/internal format files.
- Internal seam extraction for the targeted BA2 DX10 writer preparer/staging hotspot in `src/formats/ba2/ba2_dx10_prepare.cpp` or adjacent private/internal format files.
- Focused runtime regression coverage for TES4 parser behavior that is already supported by current generated fixtures and malformed cases.
- Focused runtime regression coverage for BA2 DX10 preparation behavior, including snapshot-backed staging and planned chunk preparation.
- Source/policy guardrails or equivalent committed evidence that the targeted hotspot responsibilities remain split into smaller maintainable seams.
- Preservation of current public reader/writer APIs, current result/error-code behavior, and current fixture-backed parser/preparer behavior.

**Out of scope:**
- Public API redesign or new public reader/writer types - v1.1 is hardening-only and this phase changes internal seams, not consumer surface area.
- Full TES4 parser rewrite or broad generic parser framework - the phase targets smaller maintainable seams, not a new architecture for all parsers.
- Full BA2 writer redesign or replacement of the DX10 writer pipeline - the phase targets preparer/staging seams, not a writer architecture reset.
- TES4 or BA2 GNRL dedupe optimization - this belongs to Phase 17 writer hotspot hardening.
- BA2 DX10 temporary-data lifecycle cleanup proof or abnormal-termination risk documentation - this belongs to Phase 17 ship-gate scope, although Phase 16 may create seams that make that later work safer.
- New archive family support, new compression formats, or new DDS feature support - v1.1 does not expand product capability.
- Editing, formatting, compiling, staging, or committing files under `TES5Edit/` - the submodule remains read-only reference material.
- Cross-platform portability work - libbsa remains Windows-only.

## Constraints

- Keep the public C++20 API stable; do not expose new public parser/preparer types from `include/libbsa/` for this phase.
- Preserve existing archive compatibility behavior, including TES4 metadata/error semantics and BA2 DX10 Fallout 4 vs Starfield v3 compression routing.
- Preserve the Phase 13 host-file boundary: parser and writer paths must not reintroduce raw narrow-string host-path opens where resolved `detail::host_file_path` or host-file helpers are already used.
- Preserve the Phase 15 reader backend seam; this phase must not add repeated archive-family branching back into public reader operations.
- Keep DirectXTex, libdeflate, and lz4 behind existing internal boundaries; do not add external dependencies.
- Keep all implementation work outside `TES5Edit/`.
- Treat source/policy tests as guardrails for structural invariants, not as a reason to freeze one exact helper name if an equivalent smaller internal seam satisfies the requirement.

## Acceptance Criteria

- [ ] `include/libbsa/` public reader and writer signatures remain unchanged by Phase 16.
- [ ] No files under `TES5Edit/` are modified, staged, or committed.
- [ ] The TES4 parser hotspot is split so table/header parsing and entry materialization or payload-prefix metadata responsibilities are no longer only embedded in one broad `parse_tes4_bsa_archive_impl` flow.
- [ ] Focused TES4 parser regression coverage passes for representative success metadata and compatibility-sensitive malformed/boundary behavior already covered by current fixtures.
- [ ] The BA2 DX10 preparer hotspot is split so snapshot/subresource staging and planned-chunk preparation responsibilities are available through smaller internal seams.
- [ ] Focused BA2 DX10 preparer regression coverage passes for snapshot-backed chunk assembly, multi-mip/array/cubemap ordering, target compression routing, and pre-publish failure behavior for covered cases.
- [ ] Existing TES4 reader, BA2 DX10 writer, writer-stage, and relevant archive-reader/writer execution tests continue to pass with unchanged expected behavior.
- [ ] A source/policy test or equivalent committed evidence guards the extracted seam structure without requiring a public API addition.
- [ ] Phase 17 requirements for dedupe optimization and BA2 DX10 temp-data lifecycle cleanup remain unresolved and are not claimed complete by this phase.

## Ambiguity Report

| Dimension           | Score | Min    | Status | Notes |
|---------------------|-------|--------|--------|-------|
| Goal Clarity        | 0.92  | 0.75   | met    | Goal locks both hotspots and behavior-preserving seam extraction. |
| Boundary Clarity    | 0.83  | 0.70   | met    | Public APIs, full redesigns, dedupe, and temp-lifecycle proof are explicitly excluded. |
| Constraint Clarity  | 0.80  | 0.65   | met    | Compatibility, host-path, dependency, TES5Edit, and Phase 15 constraints are named. |
| Acceptance Criteria | 0.84  | 0.70   | met    | Pass/fail checks cover both hotspots, tests, public API stability, and later-phase exclusions. |
| **Ambiguity**       | 0.14  | <=0.20 | met    | Gate passed after round 2. |

Status: met = met minimum, below = below minimum (planner treats below-minimum dimensions as assumptions)

## Interview Log

| Round | Perspective | Question summary | Decision locked |
|-------|-------------|------------------|-----------------|
| 1 | Researcher | What triggers Phase 16, and which codebase gaps should the spec emphasize for TES4 and BA2 DX10? | The trigger is both maintainability and regression-risk reduction; TES4 is framed as a monolithic parser hotspot, and BA2 DX10 is framed as a mixed-concern preparer hotspot. |
| 1 | Scoring | Scores after current-state grounding | Goal 0.88, boundary 0.68, constraint 0.72, acceptance 0.72, ambiguity 0.23; one more round was needed because boundary clarity was below minimum. |
| 2 | Researcher + Simplifier | What is the minimum viable scope, what evidence is mandatory, and which adjacent work should be cut? | Both hotspots are required; runtime plus policy or equivalent guardrail evidence is required; public API changes, full redesigns, dedupe optimization, and BA2 DX10 temp-lifecycle cleanup proof are excluded. |
| 2 | Gate | Ambiguity reached 0.14. Proceed with SPEC.md? | User selected `Yes - write SPEC.md`, then confirmed after a brief correction that the spec should be written. |

---

*Phase: 16-parser-and-preparer-seam-extraction*
*Spec created: 2026-05-14*
*Next step: /gsd-discuss-phase 16 - implementation decisions (how to build the locked parser/preparer seams above)*
