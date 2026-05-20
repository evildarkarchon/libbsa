# S05: Integrated Confidence Pass

**Goal:** Finalize M001 confidence by closing the installed-package runtime proof gap, updating the coverage matrix with fixed and deferred status, and proving the default build/test/package-consumer path without local copyrighted fixtures.
**Demo:** Default build/test/package-consumer verification passes, the matrix is updated with fixed/deferred status, optional compatibility paths remain documented, and remaining gaps are explicitly handed to later milestones.

## Must-Haves

- `tests/package-consumer/main.cpp` creates writer-produced runtime archives for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 from the installed `libbsa::libbsa` target through `<libbsa/libbsa.hpp>` only, then opens, inspects, validates, and extracts enough payload data to prove package-level runtime usability.
- The package-consumer smoke remains self-contained: no `TES5Edit/`, `.gsd/`, `.planning/`, `.audits/`, `LIBBSA_GAME_FIXTURES`, `LIBBSA_BSARCHPRO_EXPECTED`, or source-tree fixture dependency is introduced.
- `docs/coverage-audit-matrix.md` is the final support-truth source for M001: `COV-GAP-003` is closed or moved to former-gap language, `COV-GAP-001` remains closed, and only `COV-GAP-002` and `COV-GAP-004` remain explicitly deferred with rationale and future ownership.
- Public API and compatibility docs state the final default-versus-optional evidence story without overclaiming real-game or BSArchPro corpus compatibility.
- Docs-policy and target-format policy tests enforce the final matrix status, package-consumer runtime proof, optional local evidence boundary, dependency-light public header story, and TES5Edit read-only boundary.
- Required closeout gate passes on the default Windows MSVC debug static lane: configure, build `libbsa_tests`, full default CTest or an equivalent documented full focused sweep, package-consumer label, and `git status --short -- TES5Edit` showing no TES5Edit changes.
- Release static/shared package-consumer lanes are attempted when practical; if an environment limitation prevents running them locally, the limitation is recorded as advisory rather than silently assumed.

## Proof Level

- This slice proves: Final-assembly integration proof. Real runtime is required for the installed package-consumer executable and default CTest lane. Human/UAT is not required. Optional local game archives and BSArchPro-derived manifests remain advisory and are not completion blockers.

## Integration Closure

Consumes S01 coverage matrix and gap IDs, S02 public API/package-consumer audit, S03 fixture and round-trip proof catalog, and S04 validation/error stabilization proof. Adds the missing package-consumer runtime archive path and final docs-policy guardrails. After this slice, M001 has no remaining default-proof blocker; future work owns deferred `COV-GAP-002` compatibility-corpus comparisons, deferred `COV-GAP-004` warning taxonomy, performance stress gates, and release-readiness declarations.

## Verification

- No production runtime logging or metrics are added. Developer and future-agent diagnostics improve through package-consumer CTest failures, docs-policy failures that name stale gap status, target-format policy failures that localize forbidden fixture dependencies, and the final matrix's fixed/deferred gap IDs.

## Tasks

- [x] **T01: Add installed package runtime archive smoke** `est:2h`
  Expected executor skills/frontmatter: `cpp-testing`, `cmake`, `verify-before-complete`.
  - Files: `tests/package-consumer/main.cpp`
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa
ctest --preset windows-msvc-debug-static -R package_consumer_smoke --output-on-failure

- [x] **T02: Finalize matrix and policy guardrails** `est:2h`
  Expected executor skills/frontmatter: `write-docs`, `cpp-testing`, `verify-before-complete`.
  - Files: `docs/coverage-audit-matrix.md`, `docs/public-api-reality-check.md`, `docs/compatibility-evidence.md`, `docs/integration-examples.md`, `docs/api-mainpage.md`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/coverage_audit_matrix_docs_tests.cpp`, `tests/unit/target_format_policy_tests.cpp`
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure
ctest --preset windows-msvc-debug-static -L docs_policy --output-on-failure
ctest --preset windows-msvc-debug-static -L target_format_policy --output-on-failure
ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure

- [x] **T03: Run integrated verification gate** `est:1h`
  Expected executor skills/frontmatter: `cmake`, `cpp-testing`, `verify-before-complete`.
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static --output-on-failure
ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure
git status --short -- TES5Edit

## Files Likely Touched

- tests/package-consumer/main.cpp
- docs/coverage-audit-matrix.md
- docs/public-api-reality-check.md
- docs/compatibility-evidence.md
- docs/integration-examples.md
- docs/api-mainpage.md
- tests/unit/docs_policy_tests.cpp
- tests/unit/coverage_audit_matrix_docs_tests.cpp
- tests/unit/target_format_policy_tests.cpp
