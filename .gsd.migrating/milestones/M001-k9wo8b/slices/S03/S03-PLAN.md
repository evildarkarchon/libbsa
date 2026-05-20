# S03: Fixture and Round-trip Gap Closure

**Goal:** Lock the current generated-fixture and round-trip evidence story into durable public documentation and docs-policy proof, while confirming the focused writer/reader/validation fixture sweep passes from committed synthetic assets.
**Demo:** The highest-risk generated-fixture or round-trip gaps found by the audit are fixed with durable Catch2/policy/fixture proof across the affected archive families.

## Must-Haves

- Owned/supporting requirements: supports R009 by keeping default proof on committed/generated legal fixtures and advisory local evidence separate; supports already-validated R001/R002 by strengthening the coverage matrix contract; preserves R006 by leaving COV-GAP-001 routed to S04 as validation-success granularity rather than forcing it into fixture/round-trip closure.
- Done means:
- The compatibility evidence catalog includes a public, non-internal "Default fixture and round-trip proof sweep" section that names the generated-fixture, writer-output reopen, reader dispatch, validation, and manifest-validation proof surfaces across TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- A docs-policy Catch2 test enforces that the coverage matrix keeps round-trip/reopen rows proven for all four current archive families and that the compatibility evidence catalog remains discoverable for default fixture/round-trip proof.
- Focused CTest labels for coverage_audit_matrix, reader_backend_dispatch, writer round-trip surfaces, validation_api, and validate_fixture_manifests pass after regenerating the default synthetic fixture targets.
- No production behavior, public API shape, TES5Edit content, local copyrighted fixture requirement, or broad validation/error redesign is introduced.

## Proof Level

- This slice proves: Contract plus integration proof. Real runtime verification is required through CMake/CTest and generated synthetic fixture targets; human/UAT is not required. Threat surface is low because the slice changes documentation and tests only; default proof must not read local game data, .gsd, .planning, .audits, or mutable TES5Edit paths.

## Integration Closure

Consumes the S01 coverage audit matrix and current fixture/writer/validation tests. Introduces no runtime wiring and no public API changes. Leaves COV-GAP-001 for S04 validation/error stabilization and leaves package-consumer every-family runtime proof for S05 if still needed. S05 should be able to cite the added catalog section, docs-policy test, and focused CTest commands as S03 closure evidence.

## Verification

- No runtime observability changes. Agent-facing diagnostics improve through explicit public proof commands and Catch2 INFO-style docs-policy failures that localize stale or missing fixture/round-trip evidence.

## Tasks

- [x] **T01: Document the default fixture round-trip proof sweep** `est:45m`
  Why: S03 research found no separate high-risk implementation gap in generated fixtures or writer round-trips, but the proof surface is scattered across the coverage matrix, fixture README, writer tests, reader dispatch tests, and validation tests. This task makes the selected S03 tranche explicit and public without claiming optional local corpus evidence.
  - Files: `docs/compatibility-evidence.md`
  - Verify: test -s docs/compatibility-evidence.md
grep -F "Default fixture and round-trip proof sweep" docs/compatibility-evidence.md
grep -F "tests/unit/archive_reader_dispatch_tests.cpp" docs/compatibility-evidence.md
grep -F "tests/unit/validation_api_tests.cpp" docs/compatibility-evidence.md
grep -F "Optional local corpus checks" docs/compatibility-evidence.md

- [x] **T02: Enforce fixture round-trip evidence and run focused proof** `est:1h 15m`
  Why: The matrix currently marks round-trip/reopen as Proven for every current family, and S03 should make that claim resistant to drift rather than adding speculative fixtures. This task adds executable docs-policy guardrails and runs the focused generated-fixture/writer/reader/validation proof sweep.
  - Files: `tests/unit/coverage_audit_matrix_docs_tests.cpp`
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures generate_tes3_bsa_writer_fixtures generate_tes4_bsa_fixtures generate_ba2_gnrl_fixtures generate_ba2_dx10_fixtures libbsa_tests
ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure
ctest --preset windows-msvc-debug-static -L reader_backend_dispatch --output-on-failure
ctest --preset windows-msvc-debug-static -L tes3_bsa_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure
ctest --preset windows-msvc-debug-static -R validate_fixture_manifests --output-on-failure

## Files Likely Touched

- docs/compatibility-evidence.md
- tests/unit/coverage_audit_matrix_docs_tests.cpp
