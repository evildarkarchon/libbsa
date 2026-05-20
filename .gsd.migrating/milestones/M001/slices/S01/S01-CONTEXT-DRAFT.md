---
id: S01
milestone: M001
status: draft
---

# S01: Evidence Matrix and Gap Ranking — Context Draft

## Goal

Produce a blunt, public, human-first coverage/gap matrix for current libbsa archive-family support, seeded from existing repository proof and reconciled against the completed M001-k9wo8b baseline without duplicating implementation work.

## Why this Slice

S01 runs first because downstream API/error audit and remediation work needs a shared truth source for what libbsa proves by default versus what remains partial, missing, deferred, or advisory. The active M001 state is duplicated against a completed M001-k9wo8b baseline, so this slice should reconcile and verify the existing evidence matrix rather than re-create product work from scratch.

## Confirmed Human Decisions So Far

- Treat the existing completed M001-k9wo8b evidence as the starting baseline, reconcile it into active M001, and avoid duplicate implementation.
- The matrix should feel like a blunt dashboard for maintainers: easy to scan, direct about support status, and explicit about evidence and gaps.
- Rank gaps primarily by overclaim risk: places where public docs or implied support claims are stronger than repository default proof.
- When evidence is unclear, stale, indirect, or optional-only, mark the cell non-green until committed/default proof supports the claim.
- Keep optional game-corpus and BSArchPro-derived evidence separate as advisory notes or a dedicated section; it must not turn a default claim green.
- The ranked gap list should include each gap's risk, affected family/axis, evidence deficit, likely owning downstream slice, and proof needed to close.

## Scope

### In Scope

- Audit/reconcile TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 across reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof.
- Seed the active M001 matrix from `docs/coverage-audit-matrix.md` and completed M001-k9wo8b S01 evidence, then challenge for drift where needed.
- Use Proven/Fixed/Partial/Missing/Deferred/N/A-style statuses, with Proven reserved for default reproducible evidence.
- Produce ranked COV-GAP-style entries with downstream owner/proof guidance.
- Preserve discoverability from public docs and policy-test guardrails where they already exist.

### Out of Scope

- Broad code remediation, API redesign, or fixture/writer/validation fixes discovered by the audit.
- Treating `tests/fixtures/generated/compatibility_matrix.json` as the full support matrix; it is malformed-hardening evidence only.
- Requiring optional local game archives or BSArchPro-derived expected data for default completion.
- Editing, formatting, staging, compiling, vendoring, or using `TES5Edit/` as mutable fixture data.

## Constraints

- Default green proof requires committed/generated legal fixtures, always-on CTest/Catch2 tests, package-consumer checks, or docs-policy proof.
- Optional local evidence can inform future compatibility hardening but remains advisory and non-blocking.
- The artifact should be human-first, not machine-schema-first, unless a lightweight policy test guardrail is already present or cheap.
- Gap ranking should optimize for avoiding public overclaims before data-integrity or consumer-friction tie-breakers.

## Integration Points

### Consumes

- `docs/coverage-audit-matrix.md` — Existing public support-truth baseline to reconcile.
- `.gsd/milestones/M001-k9wo8b/slices/S01/` — Completed S01 context and summary for baseline decisions and known gaps.
- `docs/compatibility-evidence.md` — Optional/default evidence boundary and compatibility-warning proof catalog.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — Existing policy guardrail for matrix family/axis/status/evidence vocabulary.
- `tests/package-consumer/` — Installed-package proof inputs for public/package-consumer matrix cells.
- `tests/fixtures/generated/` — Legal generated fixture evidence and malformed-hardening submatrix.

### Produces

- Active M001 S01 context and later plan inputs for an evidence matrix/gap-ranking reconciliation slice.
- A public matrix/ranked gap handoff that downstream S02-S04 can consume without re-litigating proof boundaries.

## Open Questions

- Exact final context wording and whether the active M001 S01 should be framed as reconciliation/acceptance versus fresh audit — current thinking: reconciliation/acceptance with drift checks.
- Whether any new drift-check command should be required in S01 planning before accepting the existing matrix — current thinking: run focused coverage/docs/package policy checks if practical.
