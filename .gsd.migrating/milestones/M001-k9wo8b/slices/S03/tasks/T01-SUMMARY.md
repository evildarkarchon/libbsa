---
id: T01
parent: S03
milestone: M001-k9wo8b
key_files:
  - docs/compatibility-evidence.md
key_decisions:
  - Kept the default proof sweep scoped to committed generated fixtures, writer-output archives, Catch2/CTest, and manifest validation only.
  - Left per-variant validation-success granularity routed to the coverage audit matrix and validation API tests instead of implying a validation gap closure.
duration: 
verification_result: passed
completed_at: 2026-05-20T02:38:44.617Z
blocker_discovered: false
---

# T01: Documented the default generated-fixture and writer round-trip proof sweep in the compatibility evidence catalog.

**Documented the default generated-fixture and writer round-trip proof sweep in the compatibility evidence catalog.**

## What Happened

Updated `docs/compatibility-evidence.md` with a new `Default fixture and round-trip proof sweep` section. The section explains that default proof is repository-reproducible from committed legal generated fixtures, writer-output archives, Catch2/CTest, and manifest validation, while `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` remain optional advisory inputs only. It names the four current archive families, cross-references the coverage matrix, fixture policy README, archive reader dispatch tests, family writer tests, validation API tests, and fixture manifest validator, then lists focused CMake/CTest commands using repository presets and test labels without local corpus paths or mutable reference paths.

## Verification

Verified the updated catalog is non-empty and contains the required section title, archive reader dispatch test reference, validation API test reference, and optional-local-corpus phrase. Also ran a sanity check confirming the file does not contain milestone/slice/task planning identifiers, COV-GAP-001, or blocked internal artifact path tokens.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `test -s docs/compatibility-evidence.md` | 0 | ✅ pass | 55ms |
| 2 | `grep -F "Default fixture and round-trip proof sweep" docs/compatibility-evidence.md` | 0 | ✅ pass | 50ms |
| 3 | `grep -F "tests/unit/archive_reader_dispatch_tests.cpp" docs/compatibility-evidence.md` | 0 | ✅ pass | 46ms |
| 4 | `grep -F "tests/unit/validation_api_tests.cpp" docs/compatibility-evidence.md` | 0 | ✅ pass | 43ms |
| 5 | `grep -F "Optional local corpus checks" docs/compatibility-evidence.md` | 0 | ✅ pass | 38ms |
| 6 | `if grep -E "M[0-9]{3}|S[0-9]{2}|T[0-9]{2}|COV-GAP-001|\.gsd|\.planning|\.audits" docs/compatibility-evidence.md; then exit 1; fi` | 0 | ✅ pass | 45ms |

## Deviations

None.

## Known Issues

None.

## Files Created/Modified

- `docs/compatibility-evidence.md`
