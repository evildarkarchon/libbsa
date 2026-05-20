# S01: Coverage Audit Matrix

**Goal:** Produce a repo-visible, human-first coverage/gap matrix for current libbsa archive-family support, with default-verification evidence, optional-evidence separation, and ranked claim-risk gaps that downstream S02-S05 can consume.
**Demo:** A durable matrix shows every current archive family against reader, writer, round-trip, malformed, validation, compatibility, API, and docs proof, with ranked gaps instead of vague support claims.

## Must-Haves

- `docs/coverage-audit-matrix.md` exists and covers TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10, with subrows only where target/version/compression behavior changes proof or risk.
- The matrix covers these axes: reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof.
- Every non-green cell has a nearby evidence reference, gap ID, or deferral rationale; `Proven` is used only for default legal/generated fixture, always-on Catch2/CTest, package-consumer, or docs-policy proof.
- Optional local game archive or BSArchPro-derived evidence is clearly advisory and cannot by itself make a cell `Proven`.
- A ranked gap list exists with durable IDs, risk, affected family/axis, evidence source, and likely route to S02, S03, S04, S05, or a later milestone.
- A CTest/Catch2 policy test is registered so future edits fail if the matrix drops required families, axes, optional-evidence separation, malformed-submatrix distinction, or the public documentation link.
- Q3 threat surface: this slice adds no runtime input handling; it must truthfully document untrusted-archive/malformed proof without treating optional copyrighted fixtures as default proof.
- Q4 requirement impact: owns R001/R002 and supports R005-R009; no public API promises or support claims are broadened without proof.
- Q5 failure modes: missing or contradictory evidence must become `Partial`, `Missing`, or `Deferred` with a gap ID, not an inferred green cell.
- Q7 negative proof: the policy test should fail when a current family/required axis is absent, when optional evidence is presented as default proof, or when the malformed-hardening JSON is treated as the full S01 matrix.

## Proof Level

- This slice proves: Contract/documentation proof with always-on policy-test verification. Real runtime is not required beyond configuring/building/running the existing Catch2/CTest unit-test target. Human/UAT is not required.

## Integration Closure

The matrix becomes a public docs artifact linked from the compatibility evidence docs, while the policy test is wired into `libbsa_tests`. Downstream S02 consumes API/docs proof gaps, S03 consumes fixture/round-trip gaps, S04 consumes malformed/validation/error gaps, and S05 consumes fixed/deferred matrix status for integrated confidence reporting. No roadmap reassessment is needed at planning time because current evidence supports keeping the downstream slices as written.

## Verification

- No runtime observability changes. The slice improves agent-facing diagnostics by centralizing evidence paths, gap IDs, and CTest failure messages for missing family/axis/support-claim coverage.

## Tasks

- [x] **T01: Write the human-first coverage matrix** `est:2h`
  Expected executor skills: write-docs, verify-before-complete.
  - Files: `docs/coverage-audit-matrix.md`, `docs/compatibility-evidence.md`
  - Verify: test -s docs/coverage-audit-matrix.md
grep -q "TES3 BSA" docs/coverage-audit-matrix.md
grep -q "TES4-family BSA" docs/coverage-audit-matrix.md
grep -q "BA2 GNRL" docs/coverage-audit-matrix.md
grep -q "BA2 DX10" docs/coverage-audit-matrix.md
grep -q "Ranked gap list" docs/coverage-audit-matrix.md
grep -q "coverage-audit-matrix.md" docs/compatibility-evidence.md

- [x] **T02: Add policy-test coverage for the matrix contract** `est:1h`
  Expected executor skills: test, verify-before-complete.
  - Files: `tests/unit/coverage_audit_matrix_docs_tests.cpp`, `tests/CMakeLists.txt`
  - Verify: cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure

## Files Likely Touched

- docs/coverage-audit-matrix.md
- docs/compatibility-evidence.md
- tests/unit/coverage_audit_matrix_docs_tests.cpp
- tests/CMakeLists.txt
