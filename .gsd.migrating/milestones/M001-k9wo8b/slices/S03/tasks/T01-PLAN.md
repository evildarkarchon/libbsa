---
estimated_steps: 10
estimated_files: 1
skills_used: []
---

# T01: Document the default fixture round-trip proof sweep

Why: S03 research found no separate high-risk implementation gap in generated fixtures or writer round-trips, but the proof surface is scattered across the coverage matrix, fixture README, writer tests, reader dispatch tests, and validation tests. This task makes the selected S03 tranche explicit and public without claiming optional local corpus evidence.

Expected executor skills: write-docs, verify-before-complete.

Do:
1. Update `docs/compatibility-evidence.md` with a concise public section titled `Default fixture and round-trip proof sweep`.
2. Explain that default proof uses legal generated fixtures, writer-output archives, Catch2/CTest, and manifest validation only; optional `LIBBSA_GAME_FIXTURES` and `LIBBSA_BSARCHPRO_EXPECTED` remain advisory.
3. Name the four current families: TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
4. Cross-reference the existing proof files: `docs/coverage-audit-matrix.md`, `tests/fixtures/README.md`, `tests/unit/archive_reader_dispatch_tests.cpp`, family writer tests, `tests/unit/validation_api_tests.cpp`, and `tests/fixtures/generated/validate_fixture_manifests.py`.
5. List the focused default CMake/CTest sweep commands at a high level without using shell pipes, redirects, local game paths, `.gsd`, `.planning`, `.audits`, or mutable `TES5Edit/` paths.
6. Do not add milestone/slice planning identifiers or imply that COV-GAP-001 is fixed; keep validation-success granularity routed to the validation slice.

Done when: the catalog gives a fresh reader enough information to run the default fixture/round-trip sweep and understand why local game or BSArchPro-derived evidence is optional only.

## Inputs

- `docs/coverage-audit-matrix.md`
- `docs/compatibility-evidence.md`
- `tests/fixtures/README.md`
- `tests/fixtures/generated/validate_fixture_manifests.py`
- `tests/fixtures/generated/archives/tes3_success_manifest.json`
- `tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json`
- `tests/fixtures/generated/archives/tes4_v103_manifest.json`
- `tests/fixtures/generated/archives/tes4_v104_manifest.json`
- `tests/fixtures/generated/archives/tes4_v105_manifest.json`
- `tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json`
- `tests/fixtures/generated/archives/ba2_gnrl_sfv2_manifest.json`
- `tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json`
- `tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json`
- `tests/fixtures/generated/archives/ba2_dx10_sfv3_manifest.json`

## Expected Output

- `docs/compatibility-evidence.md`

## Verification

test -s docs/compatibility-evidence.md
grep -F "Default fixture and round-trip proof sweep" docs/compatibility-evidence.md
grep -F "tests/unit/archive_reader_dispatch_tests.cpp" docs/compatibility-evidence.md
grep -F "tests/unit/validation_api_tests.cpp" docs/compatibility-evidence.md
grep -F "Optional local corpus checks" docs/compatibility-evidence.md

## Observability Impact

Improves agent-facing failure localization by placing the exact default proof sweep and optional-evidence boundary in a public catalog rather than scattering it across tests.
