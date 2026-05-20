# S04: Error and Validation Stabilization — UAT

**Milestone:** M001-k9wo8b
**Written:** 2026-05-20T03:18:17.901Z

## UAT Type

Automated contract and documentation-policy UAT. Human exploratory UAT is not required for this slice.

## Preconditions

1. Work from `J:/libbsa` on the Windows MSVC/vcpkg development environment.
2. Use the committed/generated legal fixture set; no local game archives, BSArchPro output, or mutable `TES5Edit/` data are required.
3. Do not edit or stage `TES5Edit/`.

## Steps

1. Run `cmake --preset windows-msvc-debug-static`.
   - Expected: Configure/generate succeeds for the Windows MSVC debug static preset.
2. Run `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`.
   - Expected: `libbsa` and `libbsa_tests` build successfully.
3. Run `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure`.
   - Expected: All validation API tests pass. The suite validates the generated success archive routes with extractability enabled, malformed/setup boundaries, writer-produced archive validation, Starfield BA2 v3 method 0/method 3 routes, and expected-variant mismatch warning behavior.
4. Run `ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure`.
   - Expected: All compatibility warning tests pass and every public warning code remains covered.
5. Run `ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure`.
   - Expected: All docs-policy tests pass, including the guards that keep COV-GAP-001 closed and preserve direct validation evidence in the matrix/evidence docs.

## Edge Cases Covered

- Result-level setup errors remain outer `result<T>` failures.
- Malformed archive bytes are represented as `validation_report::errors` through validation paths.
- Expected variant mismatches remain non-fatal compatibility warnings, not validation errors.
- Unsupported/high-risk Starfield compression routes remain branchable by stable public codes where applicable, while writer-produced BA2 v3 method 0 deflate and method 3 raw LZ4 block routes are now proven valid.
- Extractability validation remains bounded by fixture/test design rather than relying on local copyrighted archives.

## Expected Outcome

The slice is acceptable when configure/build succeeds, `validation_api` reports 8/8 passing tests, `compatibility_warning` reports 5/5 passing tests, and `coverage_audit_matrix` reports 11/11 passing tests from the default fixture/docs-policy surface.

## Not Proven By This UAT

- Exhaustive diagnostic wording stability; messages remain human-readable, not exact machine-readable contracts.
- Optional local game-corpus or BSArchPro-derived compatibility checks.
- S05 package-consumer every-family runtime proof, tracked separately as COV-GAP-003.
- Future warning taxonomy expansion for COV-GAP-004.
- Broad parser, compression, or writer redesign outside the evidence-bounded S04 tranche.
