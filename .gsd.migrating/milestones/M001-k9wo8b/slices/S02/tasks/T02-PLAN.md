---
estimated_steps: 7
estimated_files: 5
skills_used: []
---

# T02: Clarify consumer docs and examples

Why: A first package consumer should understand the full public journey and known sharp edges without reverse-engineering headers or mistaking optional compatibility evidence for default support proof. Expected executor skills: `write-docs`, `api-design` as a contract/documentation review lens, and `verify-before-complete`.

Do:
1. Update `docs/api-mainpage.md` to point readers to the full audited public core and the reality-check document without overclaiming unsupported proof.
2. Update `docs/integration-examples.md` so compile-checked examples or prose cover `find`/`contains`, `extract`, `extract_bytes`, bulk extraction, validation reports, compatibility warnings, stable `error_code` branching, writer family finalization, and host filesystem path versus archive virtual path separation.
3. Update `docs/compatibility-evidence.md` to make the public API reality-check document discoverable next to the S01 coverage matrix and to preserve the default-versus-optional evidence boundary.
4. Only touch `docs/target-format-guide.md` or `docs/thread-safety.md` if T01 finds a concrete sharp-edge mismatch; do not broaden support claims or introduce implementation dependency names into public docs.

Done when: the docs explain what to use when, plainly name BA2 DX10 one-shot writer lifecycle, positive `worker_count` rules, stable `result<T>::error().code` versus unstable message text, archive virtual path boundaries, and optional corpus/BSArchPro proof boundaries.

## Inputs

- `docs/public-api-reality-check.md`
- `docs/api-mainpage.md`
- `docs/integration-examples.md`
- `docs/compatibility-evidence.md`
- `docs/target-format-guide.md`
- `docs/thread-safety.md`
- `docs/coverage-audit-matrix.md`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`

## Expected Output

- `docs/api-mainpage.md`
- `docs/integration-examples.md`
- `docs/compatibility-evidence.md`

## Verification

test -s docs/api-mainpage.md
test -s docs/integration-examples.md
test -s docs/compatibility-evidence.md
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L "docs_policy|target_format_policy|coverage_audit_matrix" --output-on-failure

## Observability Impact

Improves failure localization for future agents by linking public docs to the audit, matrix, and proof boundaries; no runtime diagnostics change.
