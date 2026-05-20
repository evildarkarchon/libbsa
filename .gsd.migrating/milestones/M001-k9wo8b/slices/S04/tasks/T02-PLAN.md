---
estimated_steps: 6
estimated_files: 4
skills_used: []
---

# T02: Update evidence docs and policy guards

Expected executor skills: write-docs, cpp-testing, verify-before-complete.

Why: Once T01 provides direct proof, the public support matrix and API audit must stop routing COV-GAP-001 as an open partial-proof gap. The docs must be useful to a fresh reader without relying on `.gsd` planning artifacts, optional local corpora, or TES5Edit output.

Do: Update `docs/coverage-audit-matrix.md` validation API rows for TES4-family BSA, BA2 GNRL, and BA2 DX10 from Partial to Proven only to the extent backed by T01. Cite direct generated fixture rows for TES4 v104/v105, BA2 GNRL Starfield v2/v3, and BA2 DX10 Starfield v3, plus writer-produced Starfield compression-method validation proof where applicable. Update the ranked gap list so COV-GAP-001 is fixed or no longer listed as open; keep COV-GAP-002, COV-GAP-003, and COV-GAP-004 scoped truthfully. Update `docs/compatibility-evidence.md` to mention the direct validation success matrix and Starfield method 0 and method 3 validation proof in the default proof sweep. Update `docs/public-api-reality-check.md` downstream routing so it says the validation API shape required no helper/API redesign and the direct proof now exists. Extend `tests/unit/coverage_audit_matrix_docs_tests.cpp` with focused docs-policy assertions for the direct validation evidence strings and compression-method coverage while preserving the existing ban on internal planning identifiers.

Done when: Public docs and docs-policy tests agree with the new validation proof, the matrix no longer overstates or understates COV-GAP-001, and remaining gaps are still routed to the correct later or deferred work.

Q5 Failure Modes: stale docs should fail `coverage_audit_matrix` with a missing-token message; optional local evidence must not be required to make a row Proven; internal planning IDs must remain absent from public matrix docs.
Q7 Negative Tests: policy tests should reject loss of variant-specific validation evidence, loss of optional-local-corpus boundaries, and accidental reintroduction of open COV-GAP-001 wording after T01 closes it.

## Inputs

- `tests/unit/validation_api_tests.cpp`
- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `docs/public-api-reality-check.md`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`

## Expected Output

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `docs/public-api-reality-check.md`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure
ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure

## Observability Impact

Adds docs-policy failure signals for stale validation proof claims. Future agents can inspect `ctest -R coverage_audit_matrix --output-on-failure` to identify the missing public evidence token.
