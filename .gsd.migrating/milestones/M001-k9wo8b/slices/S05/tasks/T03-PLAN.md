---
estimated_steps: 7
estimated_files: 14
skills_used: []
---

# T03: Run integrated verification gate

Expected executor skills/frontmatter: `cmake`, `cpp-testing`, `verify-before-complete`.

Why: S05 is the final assembly slice for M001. After code, docs, and policy tests agree, the milestone needs fresh verification evidence that the default Windows MSVC lane and installed package-consumer path still pass together and that `TES5Edit/` remains untouched.

Do: Run the required closeout gate after the last code or docs edit. Configure the default `windows-msvc-debug-static` preset, build `libbsa_tests`, run the default CTest suite or an equivalent full focused sweep if the local environment requires serialized labels, explicitly run the `package_consumer` label so installed-target runtime proof is visible, and check `git status --short -- TES5Edit` for an empty result. If practical in the local environment, also run release static and release shared package-consumer lanes to exercise package-proof installs; if those heavier lanes are unavailable or fail due an environment limitation unrelated to this slice, record the exact limitation and keep it advisory instead of treating it as default green proof. Do not skip the debug static package-consumer proof.

Done when: Fresh verification output exists after all edits; default debug static build/test/package-consumer proof passes; TES5Edit status is empty; release package-proof attempts or limitations are recorded; and the task summary can truthfully validate R007 while preserving R003, R008, R009, R001, R005, and R006.

Q5 Failure modes: Configure/build failures indicate broken CMake or dependency wiring; CTest failures identify unit, policy, package-consumer, or fixture regressions; non-empty TES5Edit status violates the hard boundary. Treat any of these as blockers until fixed or explicitly escalated.

Q6 Load profile: Full default CTest is broader than the earlier focused checks but still uses committed/generated legal fixtures and tiny package-consumer archives. It is not a large-corpus or performance stress gate.

Q7 Negative tests: The final gate must include missing-path error behavior through package-consumer smoke, docs-policy guards against overclaims, and the TES5Edit no-change check.

## Inputs

- `CMakePresets.json`
- `.github/workflows/ci.yml`
- `tests/CMakeLists.txt`
- `tests/package-consumer/main.cpp`
- `tests/package-consumer/smoke.cmake`
- `docs/coverage-audit-matrix.md`
- `docs/public-api-reality-check.md`
- `docs/compatibility-evidence.md`
- `docs/integration-examples.md`
- `docs/api-mainpage.md`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
- `TES5Edit`

## Expected Output

- Update the implementation and proof artifacts needed for this task.

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static --output-on-failure
ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure
git status --short -- TES5Edit

## Observability Impact

Final verification evidence provides the milestone-level health signal: default CTest status, package-consumer installed-target status, policy-test status, optional release-lane notes, and TES5Edit boundary status.
