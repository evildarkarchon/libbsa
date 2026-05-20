---
id: T01
parent: S04
milestone: M001-k9wo8b
key_files:
  - tests/unit/validation_api_tests.cpp
key_decisions:
  - Kept proof on the public umbrella API surface and validation report metadata instead of adding private parser assertions or changing public error/warning enums.
  - Did not modify `src/validation.cpp` because existing validation behavior already matched the desired valid-but-risky warning and report/result boundaries.
duration: 
verification_result: passed
completed_at: 2026-05-20T03:06:09.269Z
blocker_discovered: false
---

# T01: Extended public validation API tests to prove expanded generated success fixtures, Starfield BA2 v3 method routes, and expected-variant mismatch warnings.

**Extended public validation API tests to prove expanded generated success fixtures, Starfield BA2 v3 method routes, and expected-variant mismatch warnings.**

## What Happened

Updated `tests/unit/validation_api_tests.cpp` only. The generated success matrix now directly validates TES4 v104, TES4 v105, BA2 GNRL Starfield v2, BA2 GNRL Starfield v3, and BA2 DX10 Starfield v3 through `libbsa::validate_archive` with `validate_entry_extractability = true`, with fixture-specific `INFO` context and metadata assertions for version, default compression route, and Starfield v3 compression method where present. Added compact writer-produced Starfield BA2 v3 route coverage for both GNRL and DX10 archives using method 0 deflate and method 3 raw LZ4 block, again through validation with extractability enabled. Added a focused expected-variant mismatch case proving a structurally valid TES3 archive remains valid while emitting the stable `target_family_mismatch` warning with risky severity. No production changes were needed.

## Verification

Ran the required configure, build, validation API test label, and compatibility warning test selection. All commands passed. The validation API label now reports 8 tests, including the new route and mismatch warning coverage, and the compatibility warning selection remains green.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 793ms |
| 2 | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | 0 | ✅ pass | 5306ms |
| 3 | `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure` | 0 | ✅ pass | 1214ms |
| 4 | `ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure` | 0 | ✅ pass | 271ms |

## Deviations

None.

## Known Issues

None.

## Files Created/Modified

- `tests/unit/validation_api_tests.cpp`
