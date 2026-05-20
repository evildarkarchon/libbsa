---
estimated_steps: 7
estimated_files: 8
skills_used: []
---

# T02: Finalize matrix and policy guardrails

Expected executor skills/frontmatter: `write-docs`, `cpp-testing`, `verify-before-complete`.

Why: Closing the package-consumer runtime gap requires docs and policy tests to move together. Current public docs and docs-policy tests intentionally keep `COV-GAP-003` open; a code-only package-consumer change would leave the matrix stale and fail the matrix-led finish requirement.

Do: Update `docs/coverage-audit-matrix.md` so package-consumer rows for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 cite the installed package-consumer runtime smoke as creating, opening, validating, and extracting writer-produced archives for all four current families. Move `COV-GAP-003` out of the ranked open gap table into former/closed gap language near `COV-GAP-001`, while preserving `COV-GAP-002` as deferred real game/BSArchPro corpus work and `COV-GAP-004` as deferred warning-taxonomy work. Update `docs/public-api-reality-check.md` to say the every-family installed-package runtime proof is closed by S05 without changing the no-facade conclusion. Update `docs/compatibility-evidence.md` to list the package-consumer runtime smoke in the default proof sweep and to name the focused package-consumer CTest command. Add a concise note to `docs/integration-examples.md` and `docs/api-mainpage.md` that the installed package-consumer smoke now runs a writer-produced archive open/extract pass for all four current families; do not imply exhaustive real-game compatibility.

Update policy tests in `tests/unit/docs_policy_tests.cpp`, `tests/unit/coverage_audit_matrix_docs_tests.cpp`, and `tests/unit/target_format_policy_tests.cpp` so they enforce the new final state: `COV-GAP-003` is closed and absent from the ranked open-gap table; `COV-GAP-002` and `COV-GAP-004` remain deferred; package-consumer runtime proof tokens name TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, `package_consumer_smoke`, and installed target usage; forbidden local fixture/env/TES5Edit tokens remain rejected from package-consumer code and smoke CMake. Keep policy tests reading tracked public docs/source files only, not `.gsd/`, `.planning/`, `.audits/`, build outputs, local corpora, or `TES5Edit/`.

Done when: Public docs and policy tests agree that `COV-GAP-003` is closed by default installed-package runtime proof, remaining gaps are explicitly deferred, and support claims still separate default legal proof from optional local advisory evidence.

Q4 Requirement impact: Advances R007 directly, preserves R003's audited public API story and D010's no broad facade decision, preserves R008 by avoiding public header dependency changes, preserves R009 by keeping TES5Edit read-only, and keeps R001/R005/R006 evidence truthful after final matrix edits.

Q7 Negative tests: Policy tests must fail if docs regress to saying `COV-GAP-003` is intentionally unproven, if the ranked open-gap table still contains `COV-GAP-003`, if optional local evidence is treated as default proof, or if package-consumer smoke gains local fixture or TES5Edit dependencies.

## Inputs

- `docs/coverage-audit-matrix.md`
- `docs/public-api-reality-check.md`
- `docs/compatibility-evidence.md`
- `docs/integration-examples.md`
- `docs/api-mainpage.md`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
- `tests/package-consumer/main.cpp`
- `tests/CMakeLists.txt`

## Expected Output

- `docs/coverage-audit-matrix.md`
- `docs/public-api-reality-check.md`
- `docs/compatibility-evidence.md`
- `docs/integration-examples.md`
- `docs/api-mainpage.md`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure
ctest --preset windows-msvc-debug-static -L docs_policy --output-on-failure
ctest --preset windows-msvc-debug-static -L target_format_policy --output-on-failure
ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure

## Observability Impact

Docs-policy and target-format policy failures become the durable diagnostic surface for stale final matrix status, overclaimed package-consumer support, or accidental local fixture dependencies.
