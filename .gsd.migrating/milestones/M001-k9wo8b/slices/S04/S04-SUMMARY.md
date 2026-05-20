---
id: S04
parent: M001-k9wo8b
milestone: M001-k9wo8b
provides:
  - S05-ready proof that COV-GAP-001 is closed across generated success fixtures and writer-produced Starfield BA2 v3 compression routes.
  - Validated R006 structured public error behavior evidence.
  - Updated public docs and policy tests that distinguish fixed validation evidence from deferred package-consumer and warning-taxonomy gaps.
requires:
  - slice: S01
    provides: Coverage/gap matrix and ranked validation/error evidence gaps.
  - slice: S02
    provides: Public API audit context for result/error/validation behavior.
  - slice: S03
    provides: Generated fixture and round-trip evidence used to select and close the validation proof gap.
affects:
  - S05
key_files:
  - tests/unit/validation_api_tests.cpp
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
  - docs/coverage-audit-matrix.md
  - docs/compatibility-evidence.md
  - docs/public-api-reality-check.md
  - include/libbsa/result.hpp
  - include/libbsa/validation.hpp
key_decisions:
  - Kept proof on public `validate_archive`, `result<T>`, `validation_report`, and compatibility-warning behavior rather than private parser internals.
  - Did not change `src/validation.cpp`, public error codes, warning codes, dependencies, or public error-model shape because tests showed the existing model supports the desired contract.
  - Closed COV-GAP-001 through direct validation tests plus docs-policy guards, while leaving COV-GAP-003 and COV-GAP-004 outside S04.
patterns_established:
  - Validation/error stabilization tests should assert stable public codes and report/warning shape, using message fragments only as minimal vague-diagnostic guards.
  - Coverage evidence docs should be guarded by docs-policy tests when a matrix gap is closed, preventing stale public claims from drifting away from executable proof.
observability_surfaces:
  - No runtime logging or metrics were added.
  - Developer/future-agent observability comes from `validation_report` assertions, Catch2 `INFO` fixture context, and docs-policy tests that point to stale validation evidence.
drill_down_paths:
  - .gsd/milestones/M001-k9wo8b/slices/S04/tasks/T01-SUMMARY.md
  - .gsd/milestones/M001-k9wo8b/slices/S04/tasks/T02-SUMMARY.md
duration: ""
verification_result: passed
completed_at: 2026-05-20T03:18:17.900Z
blocker_discovered: false
---

# S04: Error and Validation Stabilization

**S04 closed the high-risk validation evidence gap by proving public validation success/warning behavior across generated and writer-produced routes, updating docs-policy guards, and validating R006 without redesigning libbsa's error model.**

## What Happened

S04 consumed the S01/S03 coverage and evidence gaps and focused on the high-risk COV-GAP-001 validation/error-behavior tranche. T01 extended `tests/unit/validation_api_tests.cpp` to validate every current generated success archive route with `validate_entry_extractability` enabled: TES3, TES4 v103/v104/v105, BA2 GNRL Fallout 4/Starfield v2/Starfield v3, and BA2 DX10 Fallout 4/Starfield v3. It also added public validation coverage for writer-produced Starfield BA2 v3 compression method 0 deflate and method 3 raw LZ4 block routes, plus an expected-variant mismatch case proving the archive remains valid while emitting the stable `target_family_mismatch` compatibility warning at risky severity. No production validation code or public enum shape needed to change because the existing `result<T>`/`validation_report`/warning boundary already supported the intended behavior.

T02 updated `docs/coverage-audit-matrix.md`, `docs/compatibility-evidence.md`, and `docs/public-api-reality-check.md` so the public evidence story matches the new direct validation proof. It added docs-policy assertions in `tests/unit/coverage_audit_matrix_docs_tests.cpp` that keep COV-GAP-001 closed, require the direct fixture and Starfield method-route evidence to remain documented, preserve the optional-local-corpus boundary, and keep the public API routing honest. COV-GAP-003 remains S05-owned package-consumer runtime proof, and COV-GAP-004 remains deferred unless concrete warning-taxonomy evidence appears later.

As closer, I reran the slice-level verification through `gsd_exec` and updated R006 to validated with the fresh evidence. The slice produced no public error-model redesign, no new dependencies, no TES5Edit changes, and no runtime logging/metrics surface; future-agent observability comes from structured public validation assertions and docs-policy failures that localize stale evidence.

## Verification

Fresh closeout verification used `gsd_exec` id `b97534ad-8a07-4c3b-86fc-6b1a2fba4577` and passed all required slice checks:

- `cmake --preset windows-msvc-debug-static` — exit 0; configure completed and generated `build/windows-msvc-debug-static`.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` — exit 0; built `libbsa.lib` and `libbsa_tests.exe`.
- `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure` — exit 0; 11/11 docs-policy tests passed, including direct validation success evidence and COV-GAP-001 closed-policy checks.
- `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure` — exit 0; 8/8 validation API tests passed, including generated fixtures, expected-variant mismatch warning, writer-produced archives, and Starfield BA2 v3 compression routes.
- `ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure` — exit 0; 5/5 compatibility warning/policy tests passed.

The configure/build output still includes non-fatal preexisting environment/build warnings (`CMAKE_TOOLCHAIN_FILE` manually specified but unused, and MSB8029 intermediate/output temporary-directory warnings), but no verification command failed.

## Requirements Advanced

- R001 — Updated the durable coverage/gap matrix and docs-policy guards so validation evidence status remains truthful after COV-GAP-001 closure.
- R004 — Completed a risk-bounded high-risk validation evidence gap with direct tests and documentation proof.
- R009 — S04 work stayed outside `TES5Edit/`; no reference submodule content was edited, formatted, vendored, or used as mutable fixture data.

## Requirements Validated

- R006 — Fresh closeout verification `b97534ad-8a07-4c3b-86fc-6b1a2fba4577` passed configure, build, 11/11 coverage matrix docs-policy tests, 8/8 validation API tests, and 5/5 compatibility warning tests after R006 was updated to validated.

## New Requirements Surfaced

- None.

## Requirements Invalidated or Re-scoped

None.

## Operational Readiness

None.

## Deviations

None.

## Known Limitations

COV-GAP-003 remains S05-owned package-consumer runtime proof, and COV-GAP-004 remains deferred warning-taxonomy work. Closeout build output includes non-fatal preexisting CMake/MSBuild warnings, but all verification commands passed.

## Follow-ups

S05 should include the S04 closeout checks in the integrated confidence pass and preserve the fixed/deferred matrix status: COV-GAP-001 closed, COV-GAP-003 S05-owned, and COV-GAP-004 deferred.

## Files Created/Modified

- `tests/unit/validation_api_tests.cpp` — Added public validation success proof for generated archive routes, writer-produced Starfield BA2 v3 compression method routes, and expected-variant mismatch warning behavior.
- `tests/unit/coverage_audit_matrix_docs_tests.cpp` — Added docs-policy guards for direct validation evidence, COV-GAP-001 closure, optional-local-corpus boundaries, and public API routing.
- `docs/coverage-audit-matrix.md` — Updated validation API behavior rows and ranked gaps so COV-GAP-001 is closed while remaining gaps stay scoped/deferred.
- `docs/compatibility-evidence.md` — Documented direct validation success matrix and Starfield BA2 v3 method 0/method 3 proof in the default evidence sweep.
- `docs/public-api-reality-check.md` — Updated public API reality story to reflect direct validation proof without helper/API redesign.
- `.gsd/REQUIREMENTS.md` — Regenerated by GSD after updating R006 validation status and proof.
