---
id: T02
parent: S05
milestone: M001-k9wo8b
key_files:
  - docs/coverage-audit-matrix.md
  - docs/public-api-reality-check.md
  - docs/compatibility-evidence.md
  - docs/integration-examples.md
  - docs/api-mainpage.md
  - tests/unit/docs_policy_tests.cpp
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
  - tests/unit/target_format_policy_tests.cpp
key_decisions:
  - Kept COV-GAP-003 as a closed historical gap ID in public audit docs instead of deleting the identifier, so future readers can trace why it is absent from the ranked open-gap table.
  - Used negative policy tokens for the prior open-gap wording so docs regressions that describe COV-GAP-003 as intentionally unproven fail immediately.
duration: 
verification_result: passed
completed_at: 2026-05-20T03:50:33.426Z
blocker_discovered: false
---

# T02: Closed COV-GAP-003 in public docs and policy tests by documenting and enforcing installed package-consumer archive runtime proof for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.

**Closed COV-GAP-003 in public docs and policy tests by documenting and enforcing installed package-consumer archive runtime proof for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.**

## What Happened

Updated the coverage audit matrix so every current family’s public/package-consumer row cites the installed package-consumer smoke as creating writer-produced archives, opening them through archive_reader, validating them through validate_archive, and extracting them through sink, extract_bytes, and bulk extraction paths. Moved COV-GAP-003 out of the ranked open gap table into closed/former-gap language next to COV-GAP-001, while preserving COV-GAP-002 as deferred real game/BSArchPro corpus work and COV-GAP-004 as deferred warning-taxonomy work. Updated the public API reality check to say S05 closed the every-family installed-package runtime proof without changing the no-facade conclusion. Updated the compatibility evidence catalog, integration examples, and API mainpage to name package_consumer_smoke and installed libbsa::libbsa target usage while explicitly avoiding exhaustive real-game compatibility claims. Updated docs, coverage-matrix, and target-format policy tests so they require the closed COV-GAP-003 state, the package-consumer runtime family tokens, the focused package-consumer CTest command, and continued rejection of ignored/local fixture, optional corpus, and TES5Edit dependencies from package-consumer smoke paths.

## Verification

Ran the required Windows/MSVC debug static configure, build, and focused CTest labels. The policy suites passed for coverage_audit_matrix, docs_policy, target_format_policy, and package_consumer; package_consumer exercised the installed-package smoke path.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 805ms |
| 2 | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | 0 | ✅ pass | 4242ms |
| 3 | `ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure` | 0 | ✅ pass | 1420ms |
| 4 | `ctest --preset windows-msvc-debug-static -L docs_policy --output-on-failure` | 0 | ✅ pass | 864ms |
| 5 | `ctest --preset windows-msvc-debug-static -L target_format_policy --output-on-failure` | 0 | ✅ pass | 6477ms |
| 6 | `ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure` | 0 | ✅ pass | 6091ms |

## Deviations

None.

## Known Issues

None.

## Files Created/Modified

- `docs/coverage-audit-matrix.md`
- `docs/public-api-reality-check.md`
- `docs/compatibility-evidence.md`
- `docs/integration-examples.md`
- `docs/api-mainpage.md`
- `tests/unit/docs_policy_tests.cpp`
- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
- `tests/unit/target_format_policy_tests.cpp`
