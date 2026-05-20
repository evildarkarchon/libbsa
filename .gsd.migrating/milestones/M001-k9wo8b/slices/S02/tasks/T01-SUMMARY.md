---
id: T01
parent: S02
milestone: M001-k9wo8b
key_files:
  - docs/public-api-reality-check.md
key_decisions:
  - No broad public facade or speculative helper was introduced; the audit recommends closing consumer friction through docs/examples/policy/package-consumer proof unless later evidence proves a tiny helper is necessary.
duration: 
verification_result: passed
completed_at: 2026-05-20T02:10:01.549Z
blocker_discovered: false
---

# T01: Created a public API reality-check audit that maps libbsa's real reader, validation, extraction, result, and writer surfaces to existing proof and routed follow-up gaps without adding a facade.

**Created a public API reality-check audit that maps libbsa's real reader, validation, extraction, result, and writer surfaces to existing proof and routed follow-up gaps without adding a facade.**

## What Happened

Audited the named public headers, consumer docs, coverage matrix, compatibility evidence, package-consumer smoke source, and public-boundary/docs policy tests. Created `docs/public-api-reality-check.md` with a scope/evidence policy, a flow-by-flow proof table for umbrella include/installed target usage, open/list metadata, `find` versus `contains`, single-entry extraction, `extract_bytes`, bulk extraction, validation reports, compatibility warnings, stable `error_code` branching, TES3/TES4/BA2 GNRL writers, and BA2 DX10 writer lifecycle. The audit applies the existing S02 direction by recommending docs/examples/policy proof over a broad facade and concludes that no tiny public helper is unavoidable in T01. It explicitly routes `COV-GAP-001`, `COV-GAP-003`, and `COV-GAP-004` to downstream validation, package/runtime, and compatibility-warning taxonomy work.

## Verification

Ran the task's required file/content gates against `docs/public-api-reality-check.md`: non-empty file check, `archive_reader` presence, `extract_bytes` presence, and `COV-GAP-003` presence. All required checks passed.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `test -s docs/public-api-reality-check.md` | 0 | ✅ pass | 25ms |
| 2 | `grep -Fq archive_reader docs/public-api-reality-check.md` | 0 | ✅ pass | 9ms |
| 3 | `grep -Fq extract_bytes docs/public-api-reality-check.md` | 0 | ✅ pass | 8ms |
| 4 | `grep -Fq COV-GAP-003 docs/public-api-reality-check.md` | 0 | ✅ pass | 8ms |

## Deviations

None. T01 was scoped as a docs-only audit artifact; docs/test hardening remains routed to later S02 tasks per the slice plan.

## Known Issues

None discovered beyond the already-routed follow-up gaps documented in the audit.

## Files Created/Modified

- `docs/public-api-reality-check.md`
