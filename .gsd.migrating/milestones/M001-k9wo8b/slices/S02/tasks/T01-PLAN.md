---
estimated_steps: 7
estimated_files: 1
skills_used: []
---

# T01: Write public API reality-check audit

Why: S02 must avoid speculative API churn by first proving which consumer flows are already clear, which are unclear, and which gaps belong to later slices. Expected executor skills: `write-docs`, `design-an-interface`, `api-design` as a public-contract review lens, and `verify-before-complete`.

Do:
1. Compare the public core in `include/libbsa/libbsa.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, and `include/libbsa/result.hpp` against `docs/coverage-audit-matrix.md`, `docs/compatibility-evidence.md`, existing docs, and `tests/package-consumer/main.cpp`.
2. Audit these flows explicitly: umbrella include and installed target usage, open/list metadata, `find` versus `contains`, single-entry `extract`, `extract_bytes`, `extract_entries` bulk extraction, validation reports, compatibility warnings, stable `error_code` branching, TES3 writer, TES4-family writer, BA2 GNRL writer, and BA2 DX10 writer lifecycle.
3. Create `docs/public-api-reality-check.md` as a public, human-readable audit. Include scope/evidence policy, a flow-by-flow proof table, sharp-edge guidance coverage, helper/API gap conclusions, and downstream routing for COV-GAP-001, COV-GAP-003, and COV-GAP-004.
4. Apply the D009 decision: default to docs/examples/policy proof and do not introduce a broad facade. If the audit proves a tiny public helper is unavoidable, document the evidence and either defer it or stop for replanning rather than expanding S02 implicitly.

Done when: the audit document is non-empty, names the real public API types, cites S01 gap IDs where relevant, distinguishes default proof from optional TES5Edit/game-corpus evidence, and makes a concrete recommendation for docs/test/API follow-up.

## Inputs

- `include/libbsa/libbsa.hpp`
- `include/libbsa/archive.hpp`
- `include/libbsa/writer.hpp`
- `include/libbsa/validation.hpp`
- `include/libbsa/result.hpp`
- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `docs/api-mainpage.md`
- `docs/integration-examples.md`
- `docs/target-format-guide.md`
- `docs/thread-safety.md`
- `tests/package-consumer/main.cpp`
- `tests/unit/public_include_boundary_tests.cpp`
- `tests/unit/export_surface_policy_tests.cpp`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
- `tests/unit/validation_api_tests.cpp`

## Expected Output

- `docs/public-api-reality-check.md`

## Verification

test -s docs/public-api-reality-check.md
grep -Fq "archive_reader" docs/public-api-reality-check.md
grep -Fq "extract_bytes" docs/public-api-reality-check.md
grep -Fq "COV-GAP-003" docs/public-api-reality-check.md

## Observability Impact

Creates an agent-readable audit surface with durable gap routing; no runtime diagnostics change.
