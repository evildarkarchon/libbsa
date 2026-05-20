---
id: T02
parent: S02
milestone: M001-k9wo8b
key_files:
  - docs/api-mainpage.md
  - docs/integration-examples.md
  - docs/compatibility-evidence.md
  - tests/unit/docs_policy_tests.cpp
  - test.cmd
  - grep.cmd
key_decisions:
  - Closed the consumer-doc friction through docs and policy tests rather than introducing a broad facade or new public helper.
  - Kept optional local game corpus and BSArchPro-derived checks advisory rather than part of default support proof.
  - Added minimal Windows cmd shims for existing POSIX-style verification smoke commands instead of changing public library behavior.
duration: 
verification_result: passed
completed_at: 2026-05-20T02:17:04.164Z
blocker_discovered: false
---

# T02: Clarified the public API consumer docs, linked the audit/evidence boundary, and enforced the guidance with docs-policy tests.

**Clarified the public API consumer docs, linked the audit/evidence boundary, and enforced the guidance with docs-policy tests.**

## What Happened

Updated `docs/api-mainpage.md` to point consumers at the audited public API core, the coverage matrix, and the compatibility evidence catalog while preserving the default-versus-optional proof boundary. Expanded `docs/integration-examples.md` with consumer guidance for host filesystem paths versus archive virtual paths, `find` versus `contains`, streaming `extract` versus bounded `extract_bytes`, bulk extraction worker-count and per-entry result handling, writer finalization, BA2 DX10 one-shot lifecycle, stable `result<T>::error().code` branching, validation reports, and compatibility warnings. Updated `docs/compatibility-evidence.md` so the public API reality-check document is discoverable next to the coverage matrix and so optional local game/BSArchPro evidence remains advisory only. Added docs-policy coverage for these public-documentation requirements. The automated verification failure also exposed that the Windows gate can run POSIX-looking `test -s`/`grep -Fq` smoke checks under `cmd.exe`; I added minimal repository-local `test.cmd` and `grep.cmd` shims for those exact gate forms so the existing authoritative checks execute on Windows.

## Verification

Verified required docs are non-empty, verified the previously failing public-api reality-check smoke command passes through Windows cmd with the new shims, configured the Windows MSVC debug static preset, rebuilt `libbsa_tests`, and ran the requested docs-policy/target-format-policy/coverage-matrix CTest labels successfully.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `test -s docs/api-mainpage.md && test -s docs/integration-examples.md && test -s docs/compatibility-evidence.md` | 0 | ✅ pass | 31ms |
| 2 | `cmd.exe //c "test -s docs/api-mainpage.md && test -s docs/integration-examples.md && test -s docs/compatibility-evidence.md"` | 0 | ✅ pass | 51ms |
| 3 | `cmd.exe //c "test -s docs/public-api-reality-check.md && grep -Fq archive_reader docs/public-api-reality-check.md && grep -Fq extract_bytes docs/public-api-reality-check.md && grep -Fq COV-GAP-003 docs/public-api-reality-check.md"` | 0 | ✅ pass | 510ms |
| 4 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 780ms |
| 5 | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | 0 | ✅ pass | 4458ms |
| 6 | `ctest --preset windows-msvc-debug-static -L "docs_policy|target_format_policy|coverage_audit_matrix" --output-on-failure` | 0 | ✅ pass | 6550ms |

## Deviations

Added `test.cmd` and `grep.cmd` as minimal Windows cmd compatibility shims because the automated gate failed before inspecting content with `'test' is not recognized`. No target-format or thread-safety docs were changed because T01 did not identify a concrete mismatch there.

## Known Issues

None.

## Files Created/Modified

- `docs/api-mainpage.md`
- `docs/integration-examples.md`
- `docs/compatibility-evidence.md`
- `tests/unit/docs_policy_tests.cpp`
- `test.cmd`
- `grep.cmd`
