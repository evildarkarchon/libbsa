---
estimated_steps: 7
estimated_files: 3
skills_used: []
---

# T03: Strengthen package-consumer and policy proof

Why: S02 is only credible if the updated public story is enforced by installed-package compile/link smoke and docs/public-boundary policy tests rather than prose alone. Expected executor skills: `tdd`, `cpp-testing`, and `verify-before-complete`.

Do:
1. Update `tests/package-consumer/main.cpp` so every public example promised by `docs/integration-examples.md` remains compile-checked from `<libbsa/libbsa.hpp>` only. Add representative compile/link coverage for any newly documented helper flow such as `contains` and `extract_bytes`; keep the runtime smoke cheap and default-runnable without local copyrighted fixtures.
2. Extend existing policy tests, preferably `tests/unit/docs_policy_tests.cpp` and `tests/unit/target_format_policy_tests.cpp`, to require the new `docs/public-api-reality-check.md` discovery path, the full core journey vocabulary, the default-versus-optional proof boundary, and the absence of forbidden public dependency leakage or mutable TES5Edit fixture claims.
3. Keep public headers dependency-light and C++20-only. If no public header changed, still run the public include/export boundary labels as regression proof.
4. Preserve COV-GAP routing truthfully: do not mark COV-GAP-003 fully closed unless the package-consumer lane now performs default every-family runtime archive proof; otherwise document it as S05 input.

Done when: package-consumer examples compile/link, runtime smoke remains cheap and deterministic, policy tests fail on missing public API audit/doc-discovery claims, and no test reads `.gsd/`, `.planning/`, `.audits/`, TES5Edit mutable paths, or local copyrighted fixture directories.

## Inputs

- `docs/public-api-reality-check.md`
- `docs/api-mainpage.md`
- `docs/integration-examples.md`
- `docs/compatibility-evidence.md`
- `docs/coverage-audit-matrix.md`
- `include/libbsa/libbsa.hpp`
- `include/libbsa/archive.hpp`
- `include/libbsa/writer.hpp`
- `include/libbsa/validation.hpp`
- `include/libbsa/result.hpp`
- `tests/package-consumer/main.cpp`
- `tests/package-consumer/smoke.cmake`
- `tests/package-consumer/verify-runtime-dll-copy.cmake`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
- `tests/unit/public_include_boundary_tests.cpp`
- `tests/unit/export_surface_policy_tests.cpp`
- `tests/unit/validation_api_tests.cpp`
- `tests/CMakeLists.txt`

## Expected Output

- `tests/package-consumer/main.cpp`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L "public-api|docs_policy|target_format_policy|package_consumer|validation_api|coverage_audit_matrix" --output-on-failure

## Observability Impact

Adds or tightens Catch2/CTest failure messages around public API documentation and package-consumer proof; no runtime library observability change.
