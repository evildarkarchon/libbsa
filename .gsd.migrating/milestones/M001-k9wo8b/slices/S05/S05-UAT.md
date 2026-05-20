# S05: S05 — UAT

**Milestone:** M001-k9wo8b
**Written:** 2026-05-20T04:02:23.506Z

## UAT Type

Automated closeout verification / package-consumer smoke.

## Preconditions

- Run on the supported Windows/MSVC/vcpkg development lane from `J:/libbsa`.
- No local copyrighted game archives, BSArchPro-derived expected manifests, `LIBBSA_GAME_FIXTURES`, or `LIBBSA_BSARCHPRO_EXPECTED` inputs are required.
- `TES5Edit/` is treated as read-only reference material only.

## Steps and Expected Outcomes

1. Configure the default debug static lane with `cmake --preset windows-msvc-debug-static`.
   - Expected: configuration exits 0 and writes build files under `build/windows-msvc-debug-static`.
2. Build the default test target with `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`.
   - Expected: build exits 0 and produces `libbsa_tests`.
3. Run the full default test suite with `ctest --preset windows-msvc-debug-static --output-on-failure`.
   - Expected: all default tests pass; opt-in local game fixture and BSArchPro comparison tests may be skipped because they are advisory.
4. Run package-consumer proof with `ctest --preset windows-msvc-debug-static -L package_consumer --output-on-failure`.
   - Expected: `package_consumer_smoke` and runtime DLL copy pass. The smoke proves the installed/exported `libbsa::libbsa` target and `<libbsa/libbsa.hpp>` can create, open, inspect, validate, and extract TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives without source-tree fixtures.
5. Run focused docs/policy gates with `ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure`, `ctest --preset windows-msvc-debug-static -L docs_policy --output-on-failure`, and `ctest --preset windows-msvc-debug-static -L target_format_policy --output-on-failure`.
   - Expected: all labels pass and enforce final matrix status, package-consumer runtime proof, dependency-light public headers, optional evidence boundaries, and TES5Edit read-only boundaries.
6. Check the TES5Edit boundary with the repository status check used by the slice plan, `git status --short -- TES5Edit`, or an equivalent read-only boundary check.
   - Expected: no TES5Edit status lines or post-status file modifications are reported.
7. Inspect `docs/coverage-audit-matrix.md`.
   - Expected: it is the final M001 support-truth source; COV-GAP-001 and COV-GAP-003 are closed/former gaps, and only COV-GAP-002 and COV-GAP-004 remain deferred with rationale and future ownership.

## Edge Cases

- If local game fixture or BSArchPro comparison tests are skipped, default UAT still passes because those lanes are advisory.
- If release static/shared package-consumer lanes fail before libbsa configuration due local vcpkg/Visual Studio detection, record the environment limitation and rely on the debug static default gate plus CI wiring until the environment is fixed.
- If any docs-policy test fails, treat it as a stale support-claim or evidence-boundary regression and reopen the relevant documentation/policy task.
- If `package_consumer_smoke` starts depending on source-tree fixtures, TES5Edit, `.gsd`, `.planning`, `.audits`, or optional local corpora, UAT fails.

## Not Proven By This UAT

- Exhaustive real-game or BSArchPro corpus compatibility (deferred COV-GAP-002 / R011).
- A complete public warning taxonomy (deferred COV-GAP-004).
- Large-archive performance/stress readiness (deferred R012).
- Publish/release readiness declaration or every release preset passing locally (deferred R013 and environment-dependent release lane proof).
- A new ergonomic facade or major public API redesign (deferred R010).
