---
id: S03
milestone: M001
status: ready
---

# S03: Risk Bounded Gap Remediation — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Reconcile and, only if necessary, repair a bounded fixture/round-trip/default-proof tranche so the active M001 matrix has durable remediation evidence without duplicating already-completed baseline work.

## Why this Slice

S03 is where M001 must be more than a documentation audit: selected claim-risk gaps need durable proof, clear fixed status, or explicit deferral. Current repository evidence already appears to have locked down default fixture and round-trip proof through public compatibility docs, docs-policy guardrails, generated fixtures, writer tests, validation API tests, manifest validation, and package-consumer runtime smoke. Active `M001` should therefore accept that baseline only after focused drift checks, repair proof drift if found, and avoid inventing speculative fixtures or expanding into a broad compatibility campaign.

## Scope

### In Scope

- Consume active S01/S02 contexts plus completed `M001-k9wo8b` S03 artifacts to select a bounded remediation or acceptance tranche.
- Reconcile the completed S03 proof-locking work into active `M001`, especially `docs/compatibility-evidence.md` default fixture/round-trip proof sweep and `tests/unit/coverage_audit_matrix_docs_tests.cpp` guardrails.
- Verify that previously closed or accepted fixture, round-trip, validation-success, and package-consumer runtime proof claims still have current repository evidence before treating them as fixed in active `M001`.
- Prefer behavioral round-trip proof: writer output reopens, relevant metadata supports the claim, extracted bytes match expected payloads, and validation accepts or reports as expected.
- Prefer public docs plus policy-test guardrails when runtime proof already exists but is scattered across tests, fixtures, manifests, and docs.
- Repair docs, policy tests, proof catalog wording, or matrix references if behavior is already sound but evidence is scattered, stale, or hard to discover.
- Add focused behavioral tests or generated legal fixtures only if a selected claim-risk gap lacks default proof.
- Use generated legal fixtures, writer-output archives, manifest validation, Catch2/CTest, and package-consumer checks as default proof.
- Use a focused verification sweep for S03: coverage matrix checks, family writer/round-trip labels, validation API, fixture manifest validation, and package/target-format proof where relevant.
- Keep optional local game/BSArchPro evidence advisory and non-blocking.
- Produce a clear handoff of fixed/accepted gaps, evidence paths, focused verification commands, and remaining deferred gaps for S04 closeout.

### Out of Scope

- Running a broad fresh fixture, round-trip, or compatibility campaign across all formats regardless of existing proof.
- Closing every adjacent gap discovered while working the selected tranche.
- Requiring local copyrighted game archives, extracted payloads, or BSArchPro-derived expected output.
- Treating exact archive byte equality as the universal success criterion when behavioral compatibility is the actual claim.
- Redesigning public API or adding convenience helpers; S02 owns public API decisions.
- Broad validation/error model stabilization beyond what is necessary for selected proof; S04 owns systemic error/validation closeout.
- Adding new archive families, GUI/CLI surfaces, in-place mutation, performance/stress work, release-readiness claims, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- S03 is bounded: after selected proof is accepted or repaired, newly discovered adjacent issues should be documented and routed rather than silently expanding scope.
- Default proof must run from committed/generated legal fixtures and repository checks without local copyrighted data.
- Fixture changes require explicit legal synthetic provenance and must not copy game bytes, BSArchPro output, or TES5Edit content.
- Prefer docs plus policy guardrails when runtime proof already exists but is scattered; prefer new runtime tests only when behavior itself lacks proof.
- Byte/layout exactness is required only when the support claim explicitly depends on exact layout, ordering, offsets, hashes, or metadata bytes.
- Previously closed gaps should be treated as fixed/accepted only after focused checks confirm the proof chain still holds in the current repository.
- Fixed/accepted gaps should update or reference the public matrix and compatibility evidence so S04 can close out without rediscovery.
- Public headers remain dependency-light C++20; no speculative dependencies.
- Keep `TES5Edit/` strictly read-only and outside generated outputs, mutable fixtures, formatting, staging, and build inputs.

## Integration Points

### Consumes

- `.gsd/milestones/M001/slices/S01/S01-CONTEXT.md` — Active proof boundaries, non-green evidence rules, optional-evidence policy, and gap-ranking priorities.
- `.gsd/milestones/M001/slices/S02/S02-CONTEXT.md` — Active API/error proof boundaries, package-consumer proof expectations, and routing rules for behavior versus proof drift.
- `.gsd/milestones/M001-k9wo8b/slices/S03/S03-CONTEXT.md` — Completed baseline S03 scope, proof bar, and boundaries for fixture/round-trip closure.
- `.gsd/milestones/M001-k9wo8b/slices/S03/S03-SUMMARY.md` — Completed S03 outcome, files changed, verification evidence, known limitations, and follow-up routing.
- `docs/coverage-audit-matrix.md` — Current public support matrix, round-trip/reopen statuses, COV-GAP statuses, and default/advisory evidence boundaries.
- `docs/compatibility-evidence.md` — Default fixture and round-trip proof sweep, proof commands, package-consumer reference, manifest validation reference, and optional-evidence policy.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — Docs-policy guardrails for matrix and compatibility evidence claims.
- `tests/fixtures/generated/` — Legal generated fixtures and manifests for default archive-family behavior proof.
- `tests/fixtures/generated/validate_fixture_manifests.py` — Manifest consistency validation for generated assets and malformed evidence references.
- `tests/unit/*writer_tests.cpp`, `tests/unit/*reader_tests.cpp`, `tests/unit/archive_reader_dispatch_tests.cpp`, and `tests/unit/validation_api_tests.cpp` — Runtime proof surfaces for writer output, reopen, metadata, extraction, compression routing, and validation behavior.
- `tests/package-consumer/` — Installed-package runtime proof when package/API proof is part of the selected tranche.

### Produces

- Active M001 S03 remediation/reconciliation findings — Which existing fixed gaps are accepted, which proof drift was repaired, and which gaps remain deferred.
- Updated public proof catalog, matrix references, policy tests, generated fixtures, or runtime tests, if focused checks reveal drift or selected gaps require changes.
- Focused verification evidence for S04 — Commands and labels that prove the accepted/fixed tranche without requiring local copyrighted fixtures.
- S04 input — Any validation/result/error or compatibility-warning inconsistencies discovered but intentionally routed out of S03.
- S04/final closeout input — Fixed/accepted gap evidence, remaining deferred gap rationale, and optional compatibility evidence notes.

## Open Questions

- Whether focused drift checks reveal any actual S03 work beyond accepting the completed baseline — current thinking: likely no broad changes, but verify before marking fixed/accepted.
- Which exact focused labels/commands are practical locally — current thinking: `coverage_audit_matrix`, family writer labels, `validation_api`, `validate_fixture_manifests`, and package/target-format proof if package claims are touched.
- Whether any closed gap needs matrix wording updated for active `M001` — current thinking: update only if proof text or status has drifted.
