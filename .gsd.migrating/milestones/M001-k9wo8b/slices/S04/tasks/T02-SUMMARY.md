---
id: T02
parent: S04
milestone: M001-k9wo8b
key_files:
  - docs/coverage-audit-matrix.md
  - docs/compatibility-evidence.md
  - docs/public-api-reality-check.md
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
key_decisions:
  - Upgraded validation API rows to Proven only where T01 provided direct public validation proof, leaving remaining COV-GAP-002 through COV-GAP-004 scoped as lower-risk/deferred work.
  - Kept the fix in public docs and docs-policy tests only; no validation API helper or error-model redesign was introduced.
duration: 
verification_result: passed
completed_at: 2026-05-20T03:14:01.576Z
blocker_discovered: false
---

# T02: Updated the public validation evidence docs and docs-policy guards so COV-GAP-001 is closed without changing the validation API shape.

**Updated the public validation evidence docs and docs-policy guards so COV-GAP-001 is closed without changing the validation API shape.**

## What Happened

Updated `docs/coverage-audit-matrix.md` so TES4-family BSA, BA2 GNRL, and BA2 DX10 validation API behavior rows are Proven only where T01's public validation tests provide direct evidence. The matrix now cites generated success fixtures for TES4 v103/v104/v105, BA2 GNRL Fallout 4/Starfield v2/Starfield v3, BA2 DX10 Fallout 4/Starfield v3, plus writer-produced Starfield BA2 v3 method 0 deflate and method 3 raw LZ4 block validation routes. The ranked gap list no longer lists COV-GAP-001 as open while preserving COV-GAP-002, COV-GAP-003, and COV-GAP-004 as the remaining lower-risk/deferred routes. Updated `docs/compatibility-evidence.md` to include the direct validation success matrix and Starfield method coverage in the default proof sweep, and updated `docs/public-api-reality-check.md` to state that direct proof now exists and the validation API shape required no helper/API redesign. Extended `tests/unit/coverage_audit_matrix_docs_tests.cpp` with docs-policy assertions for the direct evidence strings, method 0/method 3 coverage, closed COV-GAP-001 language, optional-local-corpus boundaries, and public API routing.

## Verification

Ran the required Windows MSVC Debug static configure, built `libbsa_tests`, ran the `coverage_audit_matrix` docs-policy tests, and reran the `validation_api` label. All commands passed; the coverage audit selection reports 11 passing docs-policy tests and the validation API label reports 8 passing tests.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 927ms |
| 2 | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | 0 | ✅ pass | 3313ms |
| 3 | `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure` | 0 | ✅ pass | 1362ms |
| 4 | `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure` | 0 | ✅ pass | 761ms |

## Deviations

None.

## Known Issues

None.

## Files Created/Modified

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `docs/public-api-reality-check.md`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
