---
id: S01
milestone: M001
status: ready
---

# S01: Evidence Matrix and Gap Ranking — Context

<!-- Slice-scoped context. Milestone-only sections (acceptance criteria, completion class,
     milestone sequence) do not belong here — those live in the milestone context. -->

## Goal

Produce a blunt, public, human-first coverage/gap matrix for current libbsa archive-family support, seeded from repository proof and reconciled against the existing completed M001 baseline.

## Why this Slice

S01 runs first because downstream public API/error audit, remediation, and final verification work need a shared truth source for what libbsa proves by default versus what remains partial, missing, deferred, or advisory. The active `M001` state is duplicated against completed `M001-k9wo8b` artifacts, so this slice should reconcile the existing public matrix and completed S01 evidence into the active milestone, verify that the support-truth baseline has not drifted, and avoid redoing implementation work already backed by repository proof.

## Scope

### In Scope

- Reconcile the existing completed S01 evidence and `docs/coverage-audit-matrix.md` into active `M001` rather than planning a greenfield audit from scratch.
- Audit all currently implemented archive families named by the roadmap: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- Cover the agreed capability axes: reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof.
- Keep the matrix as a blunt dashboard for maintainers: scan-friendly, direct about status, and explicit about evidence, rationale, and gaps.
- Use `Proven`, `Fixed`, `Partial`, `Missing`, `Deferred`, and `N/A`-style statuses, with green/default support reserved for claims backed by committed or generated legal fixtures, always-on Catch2/CTest tests, package-consumer checks, or docs-policy proof.
- Mark unclear, stale, indirect, or optional-only evidence as non-green until current committed/default proof clearly supports the claim.
- Rank gaps primarily by overclaim risk: places where public docs or implied support claims are stronger than default repository proof. Use data-integrity and consumer-friction risk as secondary tie-breakers.
- Give each meaningful gap a durable identifier plus risk, affected family/axis, evidence deficit, likely owning downstream slice, and proof needed to close it.
- Present optional local game-corpus or BSArchPro-derived evidence separately as advisory context; it may guide future compatibility hardening but must not make a default support cell green.
- Preserve or reuse lightweight policy-test guardrails for the matrix where they already exist or are cheap to maintain.

### Out of Scope

- Fixing production code, archive behavior, generated fixtures, validation behavior, package-consumer behavior, or public docs beyond what is strictly needed to keep the S01 matrix artifact truthful and discoverable.
- Closing every S01-discovered gap in this slice; remediation belongs to S03, S04, or later work based on the ranked list.
- Treating `tests/fixtures/generated/compatibility_matrix.json` as the full M001 support matrix; it is malformed-hardening evidence only.
- Requiring local copyrighted game archives, extracted game payloads, or BSArchPro-derived expected output for default completion.
- Creating a machine-schema-first matrix as the primary deliverable unless it is added later as a small supplement to the human-first document.
- Designing a broad public API facade, adding speculative helper APIs, or changing public API contracts; S02 owns the public API/error contract audit.
- Adding new archive formats, GUI/CLI surfaces, in-place mutation, release-readiness gates, performance/stress campaigns, or platform expansion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- Default green proof requires committed/generated legal fixtures, always-on CTest/Catch2 tests, package-consumer checks, or docs-policy proof that can run without local copyrighted archives.
- Optional game-corpus and BSArchPro-derived checks remain advisory, opt-in, skipped when absent, and separate from default support status.
- The matrix should be public and human-first so future contributors and downstream consumers can see support truth without reading `.gsd/` internals.
- The existing completed `M001-k9wo8b` artifacts are valid baseline inputs, but active `M001` should still verify drift-sensitive evidence before depending on them.
- Gap ranking should optimize for avoiding support overclaims before selecting remediation work.
- Keep public headers dependency-light and C++20-compatible while auditing proof; do not expose libdeflate, LZ4 library types, DirectXTex, DXGI, private Windows internals, or C++23-only public types.
- Keep `TES5Edit/` strictly read-only and outside generated outputs, mutable fixtures, formatting, staging, and build inputs.

## Integration Points

### Consumes

- `.gsd/milestones/M001-k9wo8b/slices/S01/S01-CONTEXT.md` — Provides the completed baseline S01 scope, decisions, and matrix-shape constraints to reconcile.
- `.gsd/milestones/M001-k9wo8b/slices/S01/S01-SUMMARY.md` — Provides the completed S01 outcome, known gap IDs, policy-test strategy, and verification history.
- `docs/coverage-audit-matrix.md` — Existing public support-truth matrix to reuse, drift-check, and keep as the durable S01 artifact.
- `docs/compatibility-evidence.md` — Default-versus-optional evidence policy and public compatibility-warning proof catalog.
- `docs/api-mainpage.md`, `docs/integration-examples.md`, and `docs/target-format-guide.md` — Public support-claim surfaces to compare against the matrix.
- `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, and `include/libbsa/result.hpp` — Public API claim surfaces that define reader, writer, validation, result, and compatibility-warning capabilities represented in the matrix.
- `tests/unit/` — Always-on Catch2 evidence for reader, writer, extraction, round-trip, malformed, validation, compatibility-warning, public-boundary, package-policy, and docs-policy claims.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — Existing policy guardrail for required matrix families, axes, status vocabulary, ranked gaps, advisory-evidence separation, and malformed-submatrix wording.
- `tests/fixtures/generated/` — Legal generated fixtures, manifests, and malformed-hardening submatrix used as default evidence inputs.
- `tests/package-consumer/` — Installed/exported package-consumer proof for public/package-consumer API matrix cells.
- Optional `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` paths — Advisory compatibility evidence only when locally available; never default proof.

### Produces

- `docs/coverage-audit-matrix.md` — The active public support-truth matrix for M001, either accepted as current or updated only to correct drift.
- Ranked `COV-GAP-*` list — Claim-risk-prioritized gaps with affected family/axis, evidence deficit, likely owning slice, and proof needed to close.
- S02 input — Public API, package-consumer, dependency-boundary, validation-semantics, or docs proof gaps that need consumer-story review.
- S03 input — Fixture, writer, extraction, round-trip, or generated-proof gaps selected as likely remediation candidates.
- S04 input — Malformed, validation, result/error, or compatibility-warning inconsistencies observed during the audit.
- S04/final closeout input — Final matrix update targets and explicit deferred-gap rationale.

## Open Questions

- Whether the active `M001` S01 should perform only focused drift verification or a broader independent re-audit before accepting the completed baseline — current thinking: use focused drift checks because the user chose baseline reconciliation.
- Exact verification lane for S01 planning/execution — current thinking: run focused `coverage_audit_matrix`, docs-policy, and package/target-format policy checks when practical, while leaving full default-plus-package proof to final closeout.
- Whether any matrix row has drifted since `M001-k9wo8b` completion — current thinking: treat existing rows as valid unless focused checks or file inspection show public claims have changed.
