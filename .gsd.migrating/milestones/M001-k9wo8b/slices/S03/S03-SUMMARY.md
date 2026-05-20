---
id: S03
parent: M001-k9wo8b
milestone: M001-k9wo8b
provides:
  - A public default fixture and round-trip proof catalog section for S05 to cite.
  - Executable docs-policy guardrails proving default round-trip/reopen rows stay present for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
  - Fresh closeout verification logs under `.gsd/exec/32ead1c3-4c7a-46d1-a9b4-f710429f00aa.stdout` and `.gsd/exec/s03-closeout-logs-clean/`.
requires:
  - slice: S01
    provides: Coverage audit matrix, ranked gap list, and evidence vocabulary used to select the S03 proof-locking tranche.
affects:
  - S04: receives COV-GAP-001/validation-success granularity as the remaining stabilization follow-up.
  - S05: consumes S03 proof catalog, docs-policy tests, requirement validation notes, and focused verification evidence for integrated confidence.
key_files:
  - docs/compatibility-evidence.md
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
  - .gsd/REQUIREMENTS.md
key_decisions:
  - Default fixture/round-trip proof is scoped to committed/generated legal fixtures, writer-output archives, Catch2/CTest, and manifest validation; local game and BSArchPro comparison paths remain optional advisory inputs only.
  - Docs-policy guardrails assert tracked public documentation and coverage-matrix strings instead of parsing `.gsd`, planning/audit directories, build outputs, local corpora, or `TES5Edit/`.
  - COV-GAP-001 remains routed to S04 because it is validation-success granularity/error behavior work, not fixture/round-trip gap closure.
patterns_established:
  - Public support claims should be stabilized with documentation plus executable docs-policy tests when runtime proof already exists but is scattered.
  - Round-trip/reopen proof for current families is guarded through the `coverage_audit_matrix` label and discoverable through `docs/compatibility-evidence.md`.
  - For final verification in this build tree, serialize separate CTest label/pattern runs to avoid Catch2 `PRE_TEST` discovery races.
observability_surfaces:
  - No runtime observability was added or required.
  - Agent/developer diagnostics improved through explicit proof commands in `docs/compatibility-evidence.md` and Catch2 docs-policy failures that localize stale or missing fixture/round-trip evidence.
drill_down_paths:
  - .gsd/milestones/M001-k9wo8b/slices/S03/tasks/T01-SUMMARY.md
  - .gsd/milestones/M001-k9wo8b/slices/S03/tasks/T02-SUMMARY.md
duration: ""
verification_result: passed
completed_at: 2026-05-20T02:47:17.357Z
blocker_discovered: false
---

# S03: Fixture and Round-trip Gap Closure

**Locked libbsa's default generated-fixture and writer round-trip proof into public compatibility docs, docs-policy tests, and a passing focused CTest sweep across TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.**

## What Happened

S03 selected a bounded, claim-risk-focused tranche from the S01 audit: the current generated-fixture and round-trip proof was already present in the repository, but it was scattered across the coverage matrix, fixture docs, reader dispatch tests, writer tests, validation API tests, and manifest validation. Rather than inventing speculative fixtures or broadening into COV-GAP-001 validation semantics, the slice made that proof public, durable, and resistant to drift.

T01 updated `docs/compatibility-evidence.md` with a public `Default fixture and round-trip proof sweep` section. The catalog now states that default proof comes from committed/generated legal fixtures, writer-output archives, Catch2/CTest, and fixture manifest validation. It names the four current archive families and keeps `LIBBSA_GAME_FIXTURES` / `LIBBSA_BSARCHPRO_EXPECTED` as optional advisory local-corpus inputs only.

T02 extended `tests/unit/coverage_audit_matrix_docs_tests.cpp` with source-documentation policy assertions. The new guardrails require round-trip/reopen evidence to remain Proven for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10; require generated-fixture/manifest evidence wording to remain present; and require the compatibility evidence catalog to stay discoverable. These checks deliberately read tracked public documentation rather than `.gsd`, planning/audit directories, local corpora, build outputs, or mutable `TES5Edit/` paths.

No production library behavior, public API shape, dependency, runtime observability surface, or TES5Edit content changed. Requirement R004 and R005 were validated after fresh closeout verification. COV-GAP-001 remains routed to S04 for validation/error stabilization, and S05 can consume this slice's catalog section, docs-policy tests, requirement updates, and focused verification logs as integrated confidence evidence.

## Verification

Fresh closeout verification passed in gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` with 15/15 checks passing. The verification ran the T01 docs smoke checks (`test -s docs/compatibility-evidence.md` plus required `grep -F` checks for the default proof sweep, reader dispatch reference, validation API reference, and optional local corpus boundary), configured `cmake --preset windows-msvc-debug-static`, built/regenerated `generate_tes3_bsa_fixtures`, `generate_tes3_bsa_writer_fixtures`, `generate_tes4_bsa_fixtures`, `generate_ba2_gnrl_fixtures`, `generate_ba2_dx10_fixtures`, and `libbsa_tests`, then passed the focused CTest proof sweep: `coverage_audit_matrix` (11/11), `reader_backend_dispatch` (2/2), `tes3_bsa_writer` (24/24), `tes4_bsa_writer` (32/32), `ba2_gnrl_writer` (30/30), `ba2_dx10_writer` (45/45), `validation_api` (6/6), and `validate_fixture_manifests` (1/1). All evidence used committed/generated legal fixtures and default repository test paths; no local copyrighted game fixture, BSArchPro expected-output path, or mutable TES5Edit path was required.

## Requirements Advanced

- R001 — Strengthened the already-validated coverage matrix contract by adding docs-policy guardrails for default fixture/round-trip evidence.
- R002 — Kept all four current archive families represented in the default proof sweep and policy tests.
- R009 — Preserved the TES5Edit read-only boundary by relying only on committed/generated legal fixtures and tracked public documentation.

## Requirements Validated

- R004 — Validated via `docs/compatibility-evidence.md`, `tests/unit/coverage_audit_matrix_docs_tests.cpp`, regenerated fixture targets, and closeout gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` passing 15/15 checks across docs smoke, fixture generation/build, coverage matrix, reader dispatch, writer round-trip labels, validation API, and manifest validation.
- R005 — Validated by preserving generated fixture CI proof, writer reopen/round-trip proof, manifest validation, and documented optional local corpus paths as separate advisory evidence in the compatibility evidence catalog and docs-policy tests.

## New Requirements Surfaced

- None.

## Requirements Invalidated or Re-scoped

None.

## Operational Readiness

None.

## Deviations

None from the planned documentation/test scope. Final closeout verification used serialized CTest label/pattern runs to avoid the known Catch2 `PRE_TEST` discovery race in a shared build tree.

## Known Limitations

S03 intentionally did not run optional local game/BSArchPro corpus comparisons, did not claim byte-for-byte writer equality, and did not close COV-GAP-001 validation-success granularity. Those remain advisory or assigned to S04/S05 as planned.

## Follow-ups

S04 should continue with validation/error behavior stabilization for COV-GAP-001. S05 should cite the S03 catalog section, docs-policy tests, requirement validations, and gsd_exec `32ead1c3-4c7a-46d1-a9b4-f710429f00aa` during the integrated default build/test/package-consumer confidence pass.

## Files Created/Modified

- `docs/compatibility-evidence.md` — Added the public default fixture and round-trip proof sweep with default versus optional evidence boundaries.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — Added docs-policy assertions guarding round-trip/reopen and fixture/manifest evidence across current archive families.
- `.gsd/REQUIREMENTS.md` — Updated R004 and R005 to validated through DB-backed requirement updates.
