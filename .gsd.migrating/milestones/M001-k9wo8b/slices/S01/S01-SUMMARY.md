---
id: S01
parent: M001-k9wo8b
milestone: M001-k9wo8b
provides:
  - Coverage/gap matrix schema and rows for all current archive families.
  - Ranked gap list with durable `COV-GAP-*` identifiers and downstream routing.
  - Policy-test guardrails for families, axes, optional-evidence separation, malformed-submatrix distinction, public-doc hygiene, and compatibility-evidence discoverability.
requires:
  []
affects:
  - S02
  - S03
  - S04
  - S05
key_files:
  - docs/coverage-audit-matrix.md
  - docs/compatibility-evidence.md
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
  - tests/CMakeLists.txt
key_decisions:
  - D007: Optional local game-corpus and BSArchPro comparison checks remain advisory; default `Proven` status requires reproducible committed/generated or always-on evidence.
  - D008: The S01 matrix contract is enforced with lightweight Catch2 docs-policy tests rather than optional fixture corpora or a machine-schema-first matrix.
patterns_established:
  - Human-first support matrix cells must cite default evidence or expose uncertainty through `Partial`, `Missing`, `Deferred`, `N/A`, or `COV-GAP-*` identifiers.
  - `compatibility_matrix.json` is treated as malformed-hardening evidence only, not as the complete support matrix.
  - Docs-policy tests with focused CTest tags can guard public documentation contracts without introducing runtime behavior changes.
observability_surfaces:
  - No runtime observability was added.
  - Agent-facing diagnostics improved through centralized evidence paths, durable gap IDs, and Catch2 `INFO` failure messages in the `coverage_audit_matrix` docs-policy suite.
drill_down_paths:
  - .gsd/milestones/M001-k9wo8b/slices/S01/tasks/T01-SUMMARY.md
  - .gsd/milestones/M001-k9wo8b/slices/S01/tasks/T02-SUMMARY.md
  - .gsd/exec/c07cef66-d700-455c-aa48-59c62dc27d9c.stdout
duration: ""
verification_result: passed
completed_at: 2026-05-20T01:44:23.615Z
blocker_discovered: false
---

# S01: Coverage Audit Matrix

**Created a public coverage audit matrix and enforced its contract with always-on docs-policy CTest coverage.**

## What Happened

S01 turned broad libbsa support claims into a public, human-first support-truth artifact. T01 added `docs/coverage-audit-matrix.md` and linked it from `docs/compatibility-evidence.md`; the matrix covers TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 across reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof. It uses explicit `Proven`, `Partial`, `Missing`, `Deferred`, and `N/A` statuses, reserves `Proven` for default reproducible evidence, and keeps optional local game/BSArchPro evidence advisory only. It also distinguishes `tests/fixtures/generated/compatibility_matrix.json` as a malformed-hardening submatrix rather than the full support matrix.

T01 found no high-risk executable-proof gaps for the core support axes. Lower-risk or deferred follow-up was captured as `COV-GAP-001` through `COV-GAP-004`: direct variant-specific validation success rows, optional full BSArchPro/game-corpus compatibility comparisons, package-consumer runtime proof for every family, and possible future family-specific warning taxonomy for TES3/BA2 DX10.

T02 added `tests/unit/coverage_audit_matrix_docs_tests.cpp` and registered it in `tests/CMakeLists.txt`. The new Catch2 docs-policy suite locks in the required family names, support axes, status/ranked-gap vocabulary, advisory-evidence separation, malformed-submatrix boundary, absence of internal planning identifiers, and discoverability from the compatibility evidence catalog. During closeout, requirements R001 and R002 were marked validated, decisions D007/D008 were recorded, and durable memories MEM012/MEM013 captured the evidence-policy and malformed-submatrix gotcha for future slices.

## Verification

Fresh closeout verification ran through `gsd_exec` as required (`c07cef66-d700-455c-aa48-59c62dc27d9c`, exit 0, 2067 ms). The script executed the slice-plan checks: `test -s docs/coverage-audit-matrix.md`; required `grep` checks for `TES3 BSA`, `TES4-family BSA`, `BA2 GNRL`, `BA2 DX10`, `Ranked gap list`, and the `coverage-audit-matrix.md` link from `docs/compatibility-evidence.md`; `cmake --preset windows-msvc-debug-static`; `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`; and `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure`. CTest reported 7/7 `coverage_audit_matrix` tests passed, 100% tests passed, with labels `coverage_audit_matrix`, `docs_policy`, and `unit`. Fresh reviewer/security subagent dispatch returned no blocking findings; the security pass explicitly reported no action needed.

## Requirements Advanced

- R006 — S01 documented `COV-GAP-001`, the validation API partial-proof gap for variant-specific success rows, as input to the S04 error/validation stabilization pass.
- R009 — S01 kept TES5Edit as reference-only and based default proof on committed/generated fixtures, always-on tests, package-consumer checks, and public docs rather than mutable TES5Edit data.

## Requirements Validated

- R001 — `docs/coverage-audit-matrix.md` now provides the durable coverage/gap matrix, and closeout verification passed docs smoke checks plus 7/7 focused `coverage_audit_matrix` CTest policy tests.
- R002 — The matrix includes TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10, and the focused policy suite includes a test requiring all four current archive families.

## New Requirements Surfaced

- No new formal requirements surfaced; lower-risk work is captured as `COV-GAP-001` through `COV-GAP-004`.

## Requirements Invalidated or Re-scoped

None.

## Operational Readiness

None.

## Deviations

None.

## Known Limitations

`COV-GAP-001` through `COV-GAP-004` remain open as lower-risk or deferred work: direct variant-specific validation success rows, full optional BSArchPro/game-corpus comparisons, package-consumer runtime proof for every family, and possible future warning taxonomy expansion.

## Follow-ups

S02 should consume API/package/docs proof notes; S03 should consume any fixture or round-trip proof gaps if selected for remediation; S04 should consider `COV-GAP-001` when auditing validation/error behavior; S05 should use the matrix and gap IDs when reporting fixed/deferred status.

## Files Created/Modified

- `docs/coverage-audit-matrix.md` — New public human-first support-truth matrix with status vocabulary, evidence policy, family/axis rows, advisory local evidence section, malformed-submatrix clarification, ranked gaps, and maintenance rule.
- `docs/compatibility-evidence.md` — Linked the coverage audit matrix from the compatibility evidence catalog.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — New Catch2 docs-policy tests enforcing required families, axes, status/ranked gaps, advisory-evidence separation, malformed-submatrix wording, public-doc hygiene, and discoverability.
- `tests/CMakeLists.txt` — Registered the coverage audit matrix docs-policy test source with `libbsa_tests`.
