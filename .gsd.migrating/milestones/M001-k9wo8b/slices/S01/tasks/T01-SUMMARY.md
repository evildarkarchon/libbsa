---
id: T01
parent: S01
milestone: M001-k9wo8b
key_files:
  - docs/coverage-audit-matrix.md
  - docs/compatibility-evidence.md
key_decisions:
  - Classified direct variant-level validation success coverage for some TES4/BA2 Starfield routes as Partial rather than Proven because current validation API tests cover them indirectly through reader/writer tests, representative success cases, and malformed matrix rows.
  - Kept local game and BSArchPro-derived comparison evidence advisory only; absent local copyrighted inputs do not block default green status and present local-only checks do not replace committed default proof.
duration: 
verification_result: passed
completed_at: 2026-05-20T01:31:56.991Z
blocker_discovered: false
---

# T01: Added a public coverage audit matrix that maps current TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 support claims to default evidence and ranked gaps.

**Added a public coverage audit matrix that maps current TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 support claims to default evidence and ranked gaps.**

## What Happened

Created `docs/coverage-audit-matrix.md` as a standalone human-first support-truth artifact. The document defines the evidence policy and status vocabulary, separates default proof from optional local corpus evidence, covers each required archive family across reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, package-consumer API proof, and docs/support-claim proof, and explains why `tests/fixtures/generated/compatibility_matrix.json` is only a malformed-hardening submatrix. It also records a ranked lower-risk/deferred gap list with `COV-GAP-*` IDs and explicitly states that no high-risk executable-proof gap was found for the core axes. Updated `docs/compatibility-evidence.md` with a concise link to the new matrix while preserving the existing compatibility-warning catalog.

## Verification

Ran the task's required shell checks through `gsd_exec`: verified `docs/coverage-audit-matrix.md` exists and contains the required family names and ranked gap section, and verified `docs/compatibility-evidence.md` links to `coverage-audit-matrix.md`. Also ran an additional public-doc safety check that the changed public docs do not expose `.gsd` paths or milestone/slice IDs and that all required axis labels appear in the new matrix.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `test -s docs/coverage-audit-matrix.md && grep -q "TES3 BSA" docs/coverage-audit-matrix.md && grep -q "TES4-family BSA" docs/coverage-audit-matrix.md && grep -q "BA2 GNRL" docs/coverage-audit-matrix.md && grep -q "BA2 DX10" docs/coverage-audit-matrix.md && grep -q "Ranked gap list" docs/coverage-audit-matrix.md && grep -q "coverage-audit-matrix.md" docs/compatibility-evidence.md` | 0 | ✅ pass | 103ms |
| 2 | `grep -En "\.gsd|M[0-9]{3}|S[0-9]{2}" docs/coverage-audit-matrix.md docs/compatibility-evidence.md (expected no matches), plus required axis-label grep checks` | 0 | ✅ pass | 154ms |

## Deviations

None.

## Known Issues

No high-risk executable-proof gaps were found. Lower-risk/deferred gaps are intentionally documented in `docs/coverage-audit-matrix.md` as `COV-GAP-001` through `COV-GAP-004`.

## Files Created/Modified

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
