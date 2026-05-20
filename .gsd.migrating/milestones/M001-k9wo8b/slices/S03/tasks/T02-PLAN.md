---
estimated_steps: 12
estimated_files: 1
skills_used: []
---

# T02: Enforce fixture round-trip evidence and run focused proof

Why: The matrix currently marks round-trip/reopen as Proven for every current family, and S03 should make that claim resistant to drift rather than adding speculative fixtures. This task adds executable docs-policy guardrails and runs the focused generated-fixture/writer/reader/validation proof sweep.

Expected executor skills: tdd, cpp-testing, cmake, verify-before-complete.

Failure modes: if a generator target fails, treat it as fixture provenance or build breakage; if a CTest label is absent, fix the test registration/tagging rather than weakening the proof; if a round-trip or validation check fails, diagnose whether it is a true S03 fixture/round-trip bug or a S04 validation/error-behavior issue before changing production code.

Load profile: bounded synthetic fixtures and unit tests only; no shared services, network calls, or large local corpora. The first 10x breakpoint would be local build/test time, not runtime memory or package API pressure.

Negative tests: docs-policy assertions should fail when required family names, round-trip evidence paths, or advisory-evidence wording disappear. Existing writer/reader/validation suites provide negative/error-path coverage for invalid paths, duplicate entries, malformed archives, decompression failures, and validation failures.

Do:
1. Extend `tests/unit/coverage_audit_matrix_docs_tests.cpp` with focused assertions that the matrix preserves round-trip/reopen evidence for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
2. Assert that the matrix still cites the family writer tests, reader dispatch proof where applicable, validation API proof, and generated fixture/manifests as default evidence.
3. Assert that `docs/compatibility-evidence.md` exposes the new default fixture/round-trip proof sweep and still keeps local game/BSArchPro inputs advisory.
4. Keep tests lightweight string/policy checks; do not parse `.gsd`, `.planning`, `.audits`, build directories, local corpus paths, or `TES5Edit/`.
5. Run the focused configure/build/generate/CTest sweep. If a real implementation bug appears inside the selected fixture/round-trip tranche and can be fixed minimally with durable proof, fix it here; otherwise document the blocker for S04/S05 instead of expanding scope.

Done when: the docs-policy test fails on stale or missing fixture/round-trip evidence, and the focused CMake/CTest sweep passes from committed/generated legal assets.

## Inputs

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `tests/CMakeLists.txt`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/unit/archive_reader_dispatch_tests.cpp`
- `tests/unit/tes3_bsa_writer_tests.cpp`
- `tests/unit/tes4_bsa_writer_tests.cpp`
- `tests/unit/ba2_gnrl_writer_tests.cpp`
- `tests/unit/ba2_dx10_writer_tests.cpp`
- `tests/unit/validation_api_tests.cpp`
- `tests/fixtures/generated/validate_fixture_manifests.py`

## Expected Output

- `tests/unit/coverage_audit_matrix_docs_tests.cpp`

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures generate_tes3_bsa_writer_fixtures generate_tes4_bsa_fixtures generate_ba2_gnrl_fixtures generate_ba2_dx10_fixtures libbsa_tests
ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure
ctest --preset windows-msvc-debug-static -L reader_backend_dispatch --output-on-failure
ctest --preset windows-msvc-debug-static -L tes3_bsa_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure
ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure
ctest --preset windows-msvc-debug-static -R validate_fixture_manifests --output-on-failure

## Observability Impact

Adds Catch2 docs-policy failures that point directly at missing/stale proof tokens and keeps the focused CTest labels as the inspection surface for fixture/round-trip confidence.
