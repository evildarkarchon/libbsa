---
id: S02
parent: M001-k9wo8b
milestone: M001-k9wo8b
provides:
  - S05 receives an audited and policy-enforced public API/package-consumer story, plus explicit COV-GAP-003 runtime-proof follow-up.
  - S04 receives COV-GAP-001 validation/result behavior routing without S02 overclaiming closure.
  - Future public API work receives a decision that broad facade/helper additions need concrete consumer evidence.
requires:
  - slice: S01
    provides: Coverage/gap matrix and ranked gap identifiers consumed to audit public API proof and route COV-GAP-001/COV-GAP-003/COV-GAP-004.
affects:
  - S04
  - S05
key_files:
  - docs/public-api-reality-check.md
  - docs/coverage-audit-matrix.md
  - docs/api-mainpage.md
  - docs/integration-examples.md
  - docs/compatibility-evidence.md
  - tests/package-consumer/main.cpp
  - tests/unit/docs_policy_tests.cpp
  - tests/unit/target_format_policy_tests.cpp
  - test.cmd
  - grep.cmd
  - .gsd/DECISIONS.md
key_decisions:
  - D010: Do not add a broad S02 public facade or speculative helper API; harden the existing public core with docs, examples, package-consumer smoke, and policy tests instead.
  - Keep COV-GAP-003 open until S05 proves every-family installed-package runtime archive behavior by default.
  - Keep optional local game-corpus and BSArchPro-derived evidence advisory rather than treating it as default support proof.
patterns_established:
  - Public API claims should be tied to durable proof: audit rows, docs-policy tests, target-format-policy tests, package-consumer smoke, or named COV-GAP routing.
  - Package-consumer proof should compile representative examples through `<libbsa/libbsa.hpp>` only and avoid ignored/local fixture dependencies unless a later slice deliberately adds legal default fixtures.
  - Sharp consumer edges should be documented directly instead of hidden behind unproven convenience APIs.
observability_surfaces:
  - `docs/public-api-reality-check.md` provides named proof status and downstream COV-GAP routing for public API claims.
  - Docs-policy and target-format-policy Catch2 tests provide failure signals when public docs/examples overclaim, hide optional evidence boundaries, or regress package-consumer proof.
  - The labeled CTest selection provides a repeatable health check for public API/package-consumer documentation proof.
drill_down_paths:
  - .gsd/milestones/M001-k9wo8b/slices/S02/tasks/T01-SUMMARY.md
  - .gsd/milestones/M001-k9wo8b/slices/S02/tasks/T02-SUMMARY.md
  - .gsd/milestones/M001-k9wo8b/slices/S02/tasks/T03-SUMMARY.md
duration: ""
verification_result: passed
completed_at: 2026-05-20T02:28:47.867Z
blocker_discovered: false
---

# S02: Public API Reality Check

**S02 audited libbsa's existing public API core against real headers, docs, coverage-matrix gaps, and installed-package smoke proof, then hardened the docs and policy tests without adding a facade or new public dependency surface.**

## What Happened

S02 converted the S01 coverage findings into a consumer-facing public API reality check. T01 created `docs/public-api-reality-check.md`, mapping the umbrella include, `archive_reader`, lookup/contains, streaming extraction, `extract_bytes`, bulk extraction, validation reports, compatibility warnings, stable `result<T>::error().code` branching, and the TES3/TES4-family/BA2 GNRL/BA2 DX10 writer families to their current proof status and downstream gap routing. T02 clarified the consumer docs in `docs/api-mainpage.md`, `docs/integration-examples.md`, and `docs/compatibility-evidence.md`, making default proof versus optional local corpus/BSArchPro evidence explicit and documenting sharp edges around host filesystem paths versus archive virtual paths, bounded `extract_bytes`, `worker_count`, writer finalization, BA2 DX10 one-shot lifecycle, and compatibility warning handling. T03 strengthened `tests/package-consumer/main.cpp`, docs-policy tests, and target-format policy tests so the installed-package lane compile-checks representative examples through `<libbsa/libbsa.hpp>` only, keeps runtime proof cheap and fixture-independent, rejects ignored/local fixture dependencies, and enforces the audited public story. COV-GAP-003 remains intentionally open for S05 because S02 proves compile/link plus minimal missing-path runtime smoke, not every-family installed-package archive runtime proof. COV-GAP-001 remains routed to S04 validation/result stabilization, and COV-GAP-004 remains documented as compatibility-warning/evidence taxonomy follow-up rather than overclaimed as closed. Quality gates: Q3 threat surface is unchanged except clearer path/worker/lifecycle guidance; no auth, secrets, remote data, or TES5Edit mutation was introduced. Q4 advances R009-style public API/package-consumer proof while preserving R001/R002 and leaving R006 validation-proof work to S04. Q5 failure visibility improved through named audit gaps and policy/package-consumer CTest failures that localize docs, fixture-boundary, and public-boundary regressions. Q6 load profile is unchanged; docs keep serial defaults and `worker_count` guidance explicit. Q7 negative coverage includes missing/invalid archive paths, no private dependency or C++23 public type leakage, no mutable/ignored fixture dependency in package-consumer smoke, and no hidden planning-path documentation claims. Q8 operational readiness is documentation/test based: health signal is the labeled CTest lane, failure signal is the named Catch2/CTest policy failure plus COV-GAP IDs, recovery is to inspect the audited document and rerun the S02 verification labels, and no runtime monitoring was planned for this docs/API-proof slice.

## Verification

Required slice verification passed before closeout and was reused per the recovery instruction not to rerun completed commands: `cmake --preset windows-msvc-debug-static` exited 0 in gsd_exec `beeb9183-4d6b-454d-9d55-1890646d5c7b`; `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` exited 0 in gsd_exec `7205ab90-c6ce-461c-a6c0-460ae5d15b50` and rebuilt `libbsa_tests`; `ctest --preset windows-msvc-debug-static -L "public-api|docs_policy|target_format_policy|package_consumer|validation_api|coverage_audit_matrix" --output-on-failure` exited 0 in gsd_exec `2fcfb065-8803-4beb-8945-1b0142bc37b0` with 48/48 selected tests passing in 7.55s, including `package_consumer_smoke` and `package_consumer_runtime_dll_copy`. After resuming, a lightweight file-sanity check over the edited S02 docs/tests/examples passed in gsd_exec `e118190c-2945-4b1b-9803-ee8f20e726d9`. Task-level summaries also show T01, T02, and T03 verification passed.

## Requirements Advanced

- R009 — Advanced by documenting and testing the package-consumer public API proof story through umbrella-header examples, installed target smoke, and public-doc policy checks.
- R001 — Preserved by keeping support claims tied to coverage-matrix proof and not overstating archive-family behavior.
- R002 — Preserved by keeping package-consumer/default proof independent of copyrighted local fixtures and mutable TES5Edit data.
- R006 — Identified as supporting validation-proof work still routed to S04 rather than prematurely closed by S02 documentation changes.

## Requirements Validated

None.

## New Requirements Surfaced

None.

## Requirements Invalidated or Re-scoped

None.

## Operational Readiness

None.

## Deviations

T02 added minimal repository-local `test.cmd` and `grep.cmd` shims because the Windows verification gate invoked POSIX-style `test -s` and `grep -Fq` through `cmd.exe`. No public header, facade, or broad helper API was added; no target-format or thread-safety doc edits were needed beyond the files changed.

## Known Limitations

COV-GAP-003 remains open for S05 every-family installed-package runtime archive proof. COV-GAP-001 remains routed to S04 validation/result stabilization. Optional local game-corpus and BSArchPro-derived evidence remains advisory, not default support proof. S02 proves compile/link plus minimal runtime error-path smoke, not full behavioral runtime coverage for every archive family.

## Follow-ups

S05 should add or confirm every-family installed-package runtime archive proof before closing COV-GAP-003. S04 should consume the validation/result behavior routing for COV-GAP-001. Future API additions should require concrete consumer misuse/friction evidence before adding helpers or facades.

## Files Created/Modified

- `docs/public-api-reality-check.md` — New audit document mapping public API flows to proof status, sharp edges, helper decisions, and COV-GAP routing.
- `docs/coverage-audit-matrix.md` — Updated to keep COV-GAP-003 open and routed to S05 for every-family installed-package runtime proof.
- `docs/api-mainpage.md` — Linked the audited public API proof story and evidence boundaries from the high-level API page.
- `docs/integration-examples.md` — Expanded consumer guidance for reader lookup/extraction, `extract_bytes`, bulk extraction, validation, error-code branching, writer finalization, BA2 DX10 lifecycle, and path boundaries.
- `docs/compatibility-evidence.md` — Linked the public API reality-check and preserved default-versus-optional evidence policy.
- `tests/package-consumer/main.cpp` — Strengthened installed-package compile smoke for umbrella-header reader lookup/extraction and missing-path error-path behavior.
- `tests/unit/docs_policy_tests.cpp` — Added policy checks enforcing public API audit discoverability, core journey docs, optional evidence boundaries, and documentation hygiene.
- `tests/unit/target_format_policy_tests.cpp` — Added package-consumer and fixture-boundary policy checks plus worker-count/public guide enforcement.
- `test.cmd` — Added minimal Windows cmd shim for `test -s` verification gate usage.
- `grep.cmd` — Added minimal Windows cmd shim for `grep -Fq` verification gate usage.
- `.gsd/DECISIONS.md` — Regenerated after recording D010 public API facade/helper decision.
