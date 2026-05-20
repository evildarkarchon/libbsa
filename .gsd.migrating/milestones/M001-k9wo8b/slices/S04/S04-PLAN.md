# S04: Error and Validation Stabilization

**Goal:** Close the evidence-bounded validation and public error-behavior gap by proving variant-specific validation success rows, stabilizing warning/result boundaries through public API tests, and updating the public matrix without redesigning libbsa's error model.
**Demo:** High-risk inconsistent result/error/validation behavior discovered by the audit is stabilized and covered by tests, without redesigning the public error model.

## Must-Haves

- `tests/unit/validation_api_tests.cpp` directly validates every current generated success archive route: `tes3_success.bsa`, `tes4_v103.bsa`, `tes4_v104.bsa`, `tes4_v105.bsa`, `ba2_gnrl_fo4.ba2`, `ba2_gnrl_sfv2.ba2`, `ba2_gnrl_sfv3.ba2`, `ba2_dx10_fo4.ba2`, and `ba2_dx10_sfv3.ba2`, with `validate_entry_extractability` enabled.
- Public behavior remains branchable by stable `libbsa::error_code` and `compatibility_warning_code`; tests assert result/report/warning shape and avoid treating exact diagnostic strings as stable contracts except for minimal vague-message guards.
- Writer-produced Starfield BA2 validation proof covers the high-risk compression-method routes that are not fully covered by generated success archives, especially BA2 v3 `CompressionMethod == 0` deflate and `CompressionMethod == 3` raw LZ4 block where public writers can produce them.
- Expected-variant target mismatch is proven as a non-fatal compatibility warning, not a validation error.
- `docs/coverage-audit-matrix.md`, `docs/compatibility-evidence.md`, and `docs/public-api-reality-check.md` accurately reflect that COV-GAP-001 is fixed or no longer open, while COV-GAP-003 remains S05-owned and COV-GAP-004 remains deferred unless concrete evidence appears.
- Docs-policy tests guard the new direct validation evidence without reading `.gsd/`, `.planning/`, `.audits/`, build outputs, optional local corpora, or `TES5Edit/`.
- No public error model, enum shape, dependency surface, or TES5Edit content changes unless a new failing test exposes a real semantic bug that cannot be represented by the existing public categories.

## Proof Level

- This slice proves: Contract plus docs-policy proof. Real runtime verification is required through Catch2/CTest; human UAT is not required. Q3 Threat Surface: no auth or secret exposure is introduced, but validation consumes untrusted archive bytes, so proof must include malformed/report-level diagnostics and bounded extractability behavior. Q4 Requirement Impact: primary R006, supporting R009, and matrix/proof updates for already-validated R001/R002/R004/R005; preserve D007/D008 default-evidence policy. Q6 Load Profile: fixtures are small and generated, but validation extractability must remain streaming/capped rather than whole-archive proof by accident. Q7 Negative Tests: setup failures stay outer-result errors, malformed archives stay report-level errors, unsupported Starfield compression stays `unsupported`, and compatibility mismatches stay warnings.

## Integration Closure

Consumes S01/S03 coverage matrix and proof catalog outputs, plus S02 public API routing. Produces S05-ready evidence that COV-GAP-001 is fixed and leaves COV-GAP-003 package-consumer every-family runtime proof plus COV-GAP-004 warning taxonomy expansion outside S04. No roadmap reassessment is needed because S05 remains the integrated confidence pass.

## Verification

- No runtime logging or metrics are added. Developer and future-agent observability improves through structured `validation_report` assertions, stable public codes, Catch2 `INFO` context naming fixture/archive routes, and docs-policy failures that localize stale validation evidence.

## Tasks

- [x] **T01: Prove validation success and warning contracts** `est:2h`
  Expected executor skills: cpp-testing, tdd, api-design, verify-before-complete.
  - Files: `tests/unit/validation_api_tests.cpp`, `src/validation.cpp`
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure
ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure

- [ ] **T02: Update evidence docs and policy guards** `est:1.5h`
  Expected executor skills: write-docs, cpp-testing, verify-before-complete.
  - Files: `docs/coverage-audit-matrix.md`, `docs/compatibility-evidence.md`, `docs/public-api-reality-check.md`, `tests/unit/coverage_audit_matrix_docs_tests.cpp`
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure
ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure

## Files Likely Touched

- tests/unit/validation_api_tests.cpp
- src/validation.cpp
- docs/coverage-audit-matrix.md
- docs/compatibility-evidence.md
- docs/public-api-reality-check.md
- tests/unit/coverage_audit_matrix_docs_tests.cpp
