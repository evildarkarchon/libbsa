---
id: T02
parent: S03
milestone: M001-k9wo8b
key_files:
  - tests/unit/coverage_audit_matrix_docs_tests.cpp
key_decisions:
  - Kept the new guardrails as source-doc string/policy assertions rather than parsing planning artifacts, local corpora, build outputs, or TES5Edit.
  - Scoped family round-trip proof checks to each archive-family section so stale or missing evidence localizes through Catch2 INFO messages.
duration: 
verification_result: passed
completed_at: 2026-05-20T02:43:18.500Z
blocker_discovered: false
---

# T02: Added docs-policy assertions that lock round-trip fixture evidence across TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10, then verified the focused generated-fixture sweep.

**Added docs-policy assertions that lock round-trip fixture evidence across TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10, then verified the focused generated-fixture sweep.**

## What Happened

Extended `tests/unit/coverage_audit_matrix_docs_tests.cpp` with section-scoped coverage-audit assertions for each current archive family. The new checks require the Round-trip/reopen rows to stay Proven and to retain the family writer tests, reader dispatch proof where applicable, validation API proof, generated fixture manifests, and writer/DDS source manifests. Added a separate matrix policy test for default fixture/manifest evidence wording and a compatibility-evidence catalog test that requires the default fixture and round-trip proof sweep, family list, writer/dispatch/validation/manifest proof files, and advisory-only local game/BSArchPro wording. The implementation stayed as lightweight string policy checks against tracked documentation; it does not parse `.gsd`, planning/audit directories, build outputs, local corpus paths, or `TES5Edit/`. During verification, an initial parallel manifest CTest invocation hit a generated CTest parse error consistent with Catch2 `DISCOVERY_MODE PRE_TEST` discovery races; rerunning the required manifest command by itself passed with no source changes, and the gotcha was captured as MEM019.

## Verification

Ran the required Windows MSVC debug static configure/build/generate/test sweep. Configure succeeded, the requested fixture generator targets and `libbsa_tests` target built successfully, the `coverage_audit_matrix` label passed with 11 tests (including the new docs-policy checks), all focused reader/writer/validation labels passed, and `validate_fixture_manifests` passed on the serialized rerun after the CTest discovery race was isolated.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 787ms |
| 2 | `cmake --build --preset windows-msvc-debug-static --target generate_tes3_bsa_fixtures generate_tes3_bsa_writer_fixtures generate_tes4_bsa_fixtures generate_ba2_gnrl_fixtures generate_ba2_dx10_fixtures libbsa_tests` | 0 | ✅ pass | 18247ms |
| 3 | `ctest --preset windows-msvc-debug-static -L coverage_audit_matrix --output-on-failure` | 0 | ✅ pass | 1663ms |
| 4 | `ctest --preset windows-msvc-debug-static -L reader_backend_dispatch --output-on-failure` | 0 | ✅ pass | 1085ms |
| 5 | `ctest --preset windows-msvc-debug-static -L tes3_bsa_writer --output-on-failure` | 0 | ✅ pass | 1756ms |
| 6 | `ctest --preset windows-msvc-debug-static -L tes4_bsa_writer --output-on-failure` | 0 | ✅ pass | 3295ms |
| 7 | `ctest --preset windows-msvc-debug-static -L ba2_gnrl_writer --output-on-failure` | 0 | ✅ pass | 2503ms |
| 8 | `ctest --preset windows-msvc-debug-static -L ba2_dx10_writer --output-on-failure` | 0 | ✅ pass | 4522ms |
| 9 | `ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure` | 0 | ✅ pass | 1390ms |
| 10 | `ctest --preset windows-msvc-debug-static -R validate_fixture_manifests --output-on-failure` | 0 | ✅ pass after serialized rerun | 110ms |

## Deviations

None from the source/test scope. Verification was adjusted only after discovering that independent parallel CTest invocations can race with Catch2 PRE_TEST discovery in the same build tree; the final required manifest command was rerun serialized and passed.

## Known Issues

Concurrent separate CTest invocations from the same build tree can race while Catch2 PRE_TEST discovery regenerates `libbsa_tests-*_tests-Debug.cmake`; prefer a single CTest invocation or serialized label/pattern runs for final evidence. No libbsa implementation or documentation proof issue remains for this task.

## Files Created/Modified

- `tests/unit/coverage_audit_matrix_docs_tests.cpp`
