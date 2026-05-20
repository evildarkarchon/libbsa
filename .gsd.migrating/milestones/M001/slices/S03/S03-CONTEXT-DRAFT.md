---
id: S03
milestone: M001
status: draft
---

# S03: Risk Bounded Gap Remediation — Context Draft

## Goal

Reconcile and, only if necessary, repair a bounded fixture/round-trip/default-proof tranche so the active M001 matrix has durable evidence without duplicating already-completed M001-k9wo8b implementation work.

## Why this Slice

S03 is the point where M001 must be more than a documentation audit: selected claim-risk gaps need durable proof or explicit deferral. Current repository evidence already appears to have locked down the default fixture and round-trip proof via public docs, policy tests, generated fixtures, writer tests, validation API tests, and package-consumer runtime smoke, so active S03 should accept that baseline only after focused drift checks and avoid inventing speculative fixtures.

## Confirmed Human Decisions So Far

- Use the completed S03 proof-locking work as the baseline; drift-check docs/tests before doing new remediation.
- Prefer behavioral round-trip proof: writer output reopens, metadata supports the claim, extracted bytes match expected payloads, and validation accepts or diagnoses as expected.
- Stop at the selected tranche; route adjacent findings to S04/S05 or future milestones with gap IDs.
- Treat previously closed gaps as fixed/accepted only after current focused checks confirm the proof chain still holds.
- Prefer public docs plus policy-test guardrails when runtime proof already exists but is scattered.
- Use a focused verification sweep rather than full integrated gate: coverage matrix, writer round-trips, validation API, manifest validation, and package/target proof where relevant.

## Scope

### In Scope

- Consume active S01/S02 contexts plus completed M001-k9wo8b S03 artifacts to select a bounded remediation or acceptance tranche.
- Reconcile existing `docs/compatibility-evidence.md` default fixture/round-trip proof sweep and `tests/unit/coverage_audit_matrix_docs_tests.cpp` guardrails into active M001.
- Verify that closed/accepted fixture, round-trip, validation-success, and package-consumer runtime proof claims still have current repository evidence.
- Repair docs, policy tests, or proof catalog drift if behavior is already sound but evidence is scattered, stale, or hard to discover.
- Add focused behavioral tests or generated fixtures only if a selected claim-risk gap lacks default proof.
- Use generated legal fixtures, writer-output archives, manifest validation, Catch2/CTest, and package-consumer checks as default proof.
- Keep optional local game/BSArchPro evidence advisory and non-blocking.
- Produce a clear handoff of fixed/accepted gaps, evidence paths, focused verification commands, and remaining deferred gaps for S04 closeout.

### Out of Scope

- Running a broad fresh compatibility campaign across all formats regardless of existing proof.
- Closing every adjacent gap discovered while working the selected tranche.
- Requiring local copyrighted game archives, extracted payloads, or BSArchPro-derived expected output.
- Treating exact archive byte equality as the universal success criterion when behavioral compatibility is the claim.
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
- Fixed/accepted gaps should update or reference the public matrix and compatibility evidence so S04 can close out without rediscovery.
- Public headers remain dependency-light C++20; no speculative dependencies.

## Integration Points

### Consumes

- `.gsd/milestones/M001/slices/S01/S01-CONTEXT.md` — Active proof boundaries and gap-ranking rules.
- `.gsd/milestones/M001/slices/S02/S02-CONTEXT.md` — Active API/error proof boundaries and tiny-helper constraints.
- `.gsd/milestones/M001-k9wo8b/slices/S03/S03-CONTEXT.md` — Completed S03 baseline scope and proof bar.
- `.gsd/milestones/M001-k9wo8b/slices/S03/S03-SUMMARY.md` — Completed S03 outcome, files changed, verification evidence, and known limitations.
- `docs/coverage-audit-matrix.md` — Current public support matrix and COV-GAP statuses.
- `docs/compatibility-evidence.md` — Default fixture/round-trip proof sweep and optional-evidence boundaries.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — Docs-policy guardrails for matrix and compatibility evidence claims.
- `tests/fixtures/generated/` and `tests/fixtures/generated/validate_fixture_manifests.py` — Legal generated fixtures and manifest validation.
- `tests/unit/*writer_tests.cpp`, `tests/unit/*reader_tests.cpp`, `tests/unit/archive_reader_dispatch_tests.cpp`, and `tests/unit/validation_api_tests.cpp` — Runtime proof surfaces for writer output, reopen, metadata, extraction, compression routing, and validation.
- `tests/package-consumer/` — Installed-package runtime proof when package/API proof is part of the selected tranche.

### Produces

- Active M001 S03 context and later plan inputs for bounded remediation/reconciliation.
- Accepted or repaired fixed-gap evidence for current fixture/round-trip/default proof claims.
- Updated public proof catalog, matrix references, policy tests, generated fixtures, or runtime tests only if drift or selected gaps require it.
- Focused verification evidence for S04 to cite.
- Explicit routing for remaining deferred or adjacent issues, especially optional corpus compatibility and family-specific warning taxonomy.

## Open Questions

- Whether focused drift checks reveal any actual S03 work beyond accepting the completed baseline — current thinking: likely no broad changes, but verify before marking fixed/accepted.
- Which exact focused labels/commands are practical locally — current thinking: `coverage_audit_matrix`, writer labels, `validation_api`, `validate_fixture_manifests`, and package/target-format proof if package claims are touched.
- Whether any closed gap needs matrix wording updated for active M001 — current thinking: update only if proof text or status has drifted.
