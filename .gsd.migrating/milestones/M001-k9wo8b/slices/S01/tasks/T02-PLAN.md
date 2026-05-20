---
estimated_steps: 14
estimated_files: 2
skills_used: []
---

# T02: Add policy-test coverage for the matrix contract

Expected executor skills: test, verify-before-complete.

Why: A one-time markdown matrix can silently rot. This task wires a focused policy test into the existing Catch2/CTest suite so the S01 artifact keeps covering the agreed families, axes, evidence-policy boundaries, and discoverability link.

Do:
1. Add `tests/unit/coverage_audit_matrix_docs_tests.cpp`, following the lightweight file-reading helper style in `tests/unit/docs_policy_tests.cpp` and using `LIBBSA_SOURCE_DIR`.
2. Test that `docs/coverage-audit-matrix.md` contains all current archive families: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
3. Test that the required axes are present: reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof.
4. Test that the status vocabulary and ranked gap list are present, including at least one `COV-GAP-` token or an explicit no-high-risk-gap statement if the final audit truly finds no high-risk gaps.
5. Test that the matrix distinguishes optional/advisory evidence (`LIBBSA_GAME_FIXTURES`, `LIBBSA_BSARCHPRO_EXPECTED`) from default proof and calls out `tests/fixtures/generated/compatibility_matrix.json` as a malformed-hardening submatrix, not the full matrix.
6. Test that public docs do not leak `.gsd/`, `M001`, or `S01` planning identifiers in the new matrix.
7. Test that `docs/compatibility-evidence.md` links to `coverage-audit-matrix.md`.
8. Register the new test source in `tests/CMakeLists.txt` near the existing docs/policy tests.

Failure Modes (Q5): If the matrix drops a family or axis, the CTest failure should name the missing token. If optional evidence is phrased as default proof, the test should fail and force the docs to separate advisory evidence. If CMake registration is missed, `ctest -R coverage_audit_matrix` should find no tests, which is a verification failure.

Negative Tests (Q7): The test should be structured so deleting a family name, deleting a required axis, removing the compatibility-evidence link, or removing the advisory-evidence language fails locally without requiring copyrighted fixtures.

Done when: the new test is built into `libbsa_tests`, the focused CTest selection runs, and failures would localize matrix drift to a clear missing token or policy sentence.

## Inputs

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `tests/CMakeLists.txt`
- `tests/unit/docs_policy_tests.cpp`
- `tests/fixtures/generated/compatibility_matrix.json`

## Expected Output

- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/CMakeLists.txt`

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure

## Observability Impact

No runtime observability changes. Adds CTest diagnostics that expose missing matrix families, axes, advisory-evidence boundaries, and documentation links to future agents.
