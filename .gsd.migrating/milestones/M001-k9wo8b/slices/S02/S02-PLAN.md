# S02: Public API Reality Check

**Goal:** Audit and harden libbsa's existing public API story against real headers, docs, package-consumer smoke coverage, and the S01 coverage matrix without introducing a broad facade or new dependency surface.
**Demo:** The package-consumer story is audited against the matrix, public headers stay dependency-light C++20, and any immediately justified helper/API/doc gaps are identified or implemented with proof.

## Must-Haves

- Done when `docs/public-api-reality-check.md` records the audited public API flows, proof status, sharp edges, helper/API gap decisions, and downstream routing for COV-GAP-001/COV-GAP-003/COV-GAP-004; consumer docs/examples explain the core journey from `<libbsa/libbsa.hpp>` through reader lookup/extraction, bulk extraction, validation, stable error-code branching, and all writer families; policy/package-consumer tests exercise or enforce the updated story; and the final verification commands pass:
- `cmake --preset windows-msvc-debug-static`
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`
- `ctest --preset windows-msvc-debug-static -L "public-api|docs_policy|target_format_policy|package_consumer|validation_api|coverage_audit_matrix" --output-on-failure`
- Quality gates:
- Q3 Threat Surface: docs/examples influence archive path and host filesystem handling, worker-count usage, validation/report branching, and BA2 DX10 writer lifecycle; no auth, secrets, or remote data exposure is introduced.
- Q4 Requirement Impact: S02 owns the public API/package-consumer proof story that supports R009 and preserves R001/R002; R006 remains a supporting validation-proof gap routed to S04 unless directly closed by executable evidence.
- Q5 Failure Modes: if package-consumer install/configure/build/test fails, executor must report the exact phase and keep docs claims no stronger than passing proof.
- Q6 Load Profile: no runtime scaling surface is changed; `worker_count` guidance remains explicit and serial defaults stay documented.
- Q7 Negative Tests: focused policy/package-consumer checks must include missing/invalid archive paths, no private dependency leakage, no C++23 public type leakage, no mutable TES5Edit fixture dependency, and no hidden planning-path documentation claims.

## Proof Level

- This slice proves: Contract and package-consumer proof: public documentation, public-header policy tests, installed-package compile/link smoke, and minimal runtime error-path smoke. This slice does not need copyrighted local fixtures, BSArchPro-derived outputs, or full every-family package-consumer runtime archive proof; any remaining every-family runtime proof stays explicitly routed to S05 as COV-GAP-003.

## Integration Closure

Consumes S01 `docs/coverage-audit-matrix.md`, compatibility evidence policy, public headers, existing docs, and package-consumer tests. Produces a public API audit document, docs/example clarifications, and focused policy/package-consumer proof for S05. Feeds S04 with any validation/result behavior gap such as COV-GAP-001. Roadmap assumptions remain valid; no reassessment is required.

## Verification

- No runtime observability is added. Agent-facing diagnostics improve through durable public gap IDs, a discoverable audit document, and Catch2/CTest policy failures that localize documentation, package-consumer, and public-boundary regressions.

## Tasks

- [x] **T01: Write public API reality-check audit** `est:1h30m`
  Why: S02 must avoid speculative API churn by first proving which consumer flows are already clear, which are unclear, and which gaps belong to later slices. Expected executor skills: `write-docs`, `design-an-interface`, `api-design` as a public-contract review lens, and `verify-before-complete`.
  - Files: `docs/public-api-reality-check.md`
  - Verify: test -s docs/public-api-reality-check.md
grep -Fq "archive_reader" docs/public-api-reality-check.md
grep -Fq "extract_bytes" docs/public-api-reality-check.md
grep -Fq "COV-GAP-003" docs/public-api-reality-check.md

- [x] **T02: Clarify consumer docs and examples** `est:1h15m`
  Why: A first package consumer should understand the full public journey and known sharp edges without reverse-engineering headers or mistaking optional compatibility evidence for default support proof. Expected executor skills: `write-docs`, `api-design` as a contract/documentation review lens, and `verify-before-complete`.
  - Files: `docs/api-mainpage.md`, `docs/integration-examples.md`, `docs/compatibility-evidence.md`, `docs/target-format-guide.md`, `docs/thread-safety.md`
  - Verify: test -s docs/api-mainpage.md
test -s docs/integration-examples.md
test -s docs/compatibility-evidence.md
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L "docs_policy|target_format_policy|coverage_audit_matrix" --output-on-failure

- [ ] **T03: Strengthen package-consumer and policy proof** `est:1h30m`
  Why: S02 is only credible if the updated public story is enforced by installed-package compile/link smoke and docs/public-boundary policy tests rather than prose alone. Expected executor skills: `tdd`, `cpp-testing`, and `verify-before-complete`.
  - Files: `tests/package-consumer/main.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L "public-api|docs_policy|target_format_policy|package_consumer|validation_api|coverage_audit_matrix" --output-on-failure

## Files Likely Touched

- docs/public-api-reality-check.md
- docs/api-mainpage.md
- docs/integration-examples.md
- docs/compatibility-evidence.md
- docs/target-format-guide.md
- docs/thread-safety.md
- tests/package-consumer/main.cpp
- tests/unit/docs_policy_tests.cpp
- tests/unit/target_format_policy_tests.cpp
