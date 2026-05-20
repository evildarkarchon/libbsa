---
id: T03
parent: S02
milestone: M001-k9wo8b
key_files:
  - tests/package-consumer/main.cpp
  - tests/unit/docs_policy_tests.cpp
  - tests/unit/target_format_policy_tests.cpp
  - docs/public-api-reality-check.md
  - docs/coverage-audit-matrix.md
key_decisions:
  - Kept COV-GAP-003 open and routed to S05 because the package-consumer lane still provides representative compile/link plus missing-path runtime proof, not every-family installed-package archive runtime proof.
  - Strengthened docs/policy/package-consumer enforcement instead of adding a facade, public helper, or new public dependency surface.
duration: 
verification_result: passed
completed_at: 2026-05-20T02:23:59.429Z
blocker_discovered: false
---

# T03: Strengthened installed-package consumer smoke and policy tests for the audited public API story while keeping COV-GAP-003 routed to S05.

**Strengthened installed-package consumer smoke and policy tests for the audited public API story while keeping COV-GAP-003 routed to S05.**

## What Happened

Updated `tests/package-consumer/main.cpp` so the umbrella-header package consumer now compile-checks the documented `find`, `contains`, streaming `extract`, and bounded `extract_bytes` lookup/extraction flow from `<libbsa/libbsa.hpp>` only. Tightened docs and target-format policy tests to require package-consumer examples to exist in both docs and source, enforce umbrella-header-only use, keep runtime smoke cheap and missing-path based, reject ignored/local fixture dependencies, require the public API audit discovery path, verify the full core consumer journey vocabulary, and preserve the default-versus-optional evidence boundary. Updated the public API audit and coverage matrix to keep COV-GAP-003 open and explicitly route every-family installed-package runtime archive proof to S05 instead of treating the representative package-consumer lane as full closure.

## Verification

Ran the requested configure, build, and labeled CTest verification. `cmake --preset windows-msvc-debug-static` configured successfully, `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` rebuilt the changed tests successfully, and `ctest --preset windows-msvc-debug-static -L "public-api|docs_policy|target_format_policy|package_consumer|validation_api|coverage_audit_matrix" --output-on-failure` passed all selected tests, including the package-consumer smoke gates and tightened policy checks.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 776ms |
| 2 | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | 0 | ✅ pass | 4143ms |
| 3 | `ctest --preset windows-msvc-debug-static -L "public-api|docs_policy|target_format_policy|package_consumer|validation_api|coverage_audit_matrix" --output-on-failure` | 0 | ✅ pass | 7604ms |

## Deviations

None.

## Known Issues

None.

## Files Created/Modified

- `tests/package-consumer/main.cpp`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
- `docs/public-api-reality-check.md`
- `docs/coverage-audit-matrix.md`
