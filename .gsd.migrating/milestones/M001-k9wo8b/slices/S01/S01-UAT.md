# S01: Coverage Audit Matrix — UAT

**Milestone:** M001-k9wo8b
**Written:** 2026-05-20T01:44:23.616Z

# UAT: S01 Coverage Audit Matrix

**UAT Type:** Documentation/contract verification with always-on policy-test proof.

## Preconditions

- Work from the libbsa repository root on the supported Windows/MSVC development setup.
- The `windows-msvc-debug-static` CMake preset and dependencies are available.
- Optional local variables such as `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` may be absent; their absence must not block the UAT.

## Steps

1. Open `docs/coverage-audit-matrix.md`.
2. Confirm the status vocabulary defines `Proven`, `Partial`, `Missing`, `Deferred`, and `N/A`, and that `Proven` is tied to default reproducible evidence rather than local copyrighted inputs.
3. Confirm the matrix has sections for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
4. For each family section, confirm the required axes are present: reader/open/list metadata, extraction, writer, round-trip/reopen, malformed handling, validation API behavior, compatibility warnings, public/package-consumer API proof, and docs/support-claim proof.
5. Inspect every non-green or non-applicable cell and verify it has nearby evidence, a gap ID, or an explicit rationale.
6. Confirm the advisory evidence section states that `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` are optional and cannot make a cell `Proven` by themselves.
7. Confirm the document says `tests/fixtures/generated/compatibility_matrix.json` is a malformed-hardening submatrix, not the complete support matrix.
8. Confirm `docs/compatibility-evidence.md` links to `coverage-audit-matrix.md`.
9. Run the focused policy verification:

   ```sh
   cmake --preset windows-msvc-debug-static
   cmake --build --preset windows-msvc-debug-static --target libbsa_tests
   ctest --preset windows-msvc-debug-static -R coverage_audit_matrix --output-on-failure
   ```

## Expected Outcomes

- The matrix is readable as the public support-truth source for current implemented archive families.
- All required families and axes are visible.
- Default green status is not granted by optional local game/BSArchPro evidence.
- `compatibility_matrix.json` is clearly scoped to malformed-hardening evidence only.
- The ranked gap list includes durable `COV-GAP-*` identifiers for remaining lower-risk/deferred work.
- Focused CTest policy verification passes and discovers the `coverage_audit_matrix` docs-policy tests.

## Edge Cases

- If optional local game fixtures are unavailable, the matrix and policy tests should still pass.
- If future edits remove a required family, axis, support vocabulary, advisory-evidence separation, malformed-submatrix language, or compatibility-evidence link, the policy test should fail.
- If a support claim lacks default proof, it should remain `Partial`, `Missing`, or `Deferred` with a gap/rationale rather than becoming `Proven`.

## Not Proven By This UAT

- No new runtime archive parsing, writing, extraction, compression, or validation behavior is proven by S01.
- Full real-game or BSArchPro corpus compatibility is not proven by default CI.
- The lower-risk gaps `COV-GAP-001` through `COV-GAP-004` are documented but not remediated in this slice.
- Package-consumer runtime fixture coverage for every archive family is not added by this slice.
