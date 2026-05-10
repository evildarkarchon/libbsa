---
phase: 11-compatibility-warnings-validation-api-and-hardening
verified: 2026-05-10T05:22:39Z
status: passed
score: 67/67 must-haves verified
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  previous_score: 65/67
  gaps_closed:
    - "COMP-01: Maintainer can compare extracted fixture output against BSArchPro-derived expected bytes or metadata."
  gaps_remaining: []
  regressions: []
---

# Phase 11: Compatibility Warnings, Validation API, and Hardening Verification Report

**Phase Goal:** Consumers and maintainers can validate archives and compatibility quirks with structured warnings/errors while malformed inputs are rejected safely.
**Verified:** 2026-05-10T05:22:39Z
**Status:** passed
**Re-verification:** Yes - after gap closure commit `83bf32e` and clean review commit `6d96286`

## Goal Achievement

Phase 11 now satisfies the roadmap contract and all COMP-01 through COMP-06 requirements. The prior blocker was COMP-01: no executable BSArchPro-derived comparison harness. Commit `83bf32e` adds that harness in `tests/unit/local_game_fixture_tests.cpp`, documents the manifest contract, and policy-checks the fixture documentation. No regressions were found in the previously passing validation API, compatibility warning, malformed hardening, sanitizer, or evidence-catalog surfaces.

## Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Maintainer can compare extracted fixture output against BSArchPro-derived expected bytes or metadata. | VERIFIED | `tests/unit/local_game_fixture_tests.cpp:28` resolves `LIBBSA_BSARCHPRO_EXPECTED` or `LIBBSA_GAME_FIXTURES/bsarchpro_expected.json`; `:219` defines the opt-in comparison test; `:232` opens each listed archive; `:237` compares archive metadata; `:249` compares entry metadata and exact bytes or FNV/size. |
| 2 | Maintainer can verify libbsa-produced archives load or validate as compatible with their target game/archive family. | VERIFIED | `tests/unit/validation_api_tests.cpp:321` validates generated TES3/TES4/BA2 fixtures and `:336` validates writer-produced TES3, TES4, BA2 GNRL, and BA2 DX10 archives. |
| 3 | Consumer can receive structured compatibility warnings for known Bethesda quirks without adopting a logging framework. | VERIFIED | `include/libbsa/validation.hpp:19` declares stable warning codes and `:51` warning records; `tests/unit/compatibility_warning_tests.cpp:108`, `:121`, and `:131` cover target mismatch, embedded-name risk, and compressed sound payload warnings. |
| 4 | Parser can gracefully reject malformed, truncated, oversized, or internally inconsistent archives under sanitizer-backed tests. | VERIFIED | `tests/fixtures/generated/compatibility_matrix.json:2` defines the Phase 11 matrix; rows cover required families/categories; `tests/unit/validation_api_tests.cpp:359` runs malformed matrix rows through validation reports. Full static CTest passed 189/189 with expected local-corpus skips. |
| 5 | Maintainer can trace each non-obvious compatibility rule to reference evidence or fixture coverage. | VERIFIED | `docs/compatibility-evidence.md` catalogs every public warning code; `tests/unit/validation_policy_tests.cpp:111` machine-checks rule/evidence entries against the public enum. |
| 6 | COMP-01: Maintainer can compare extracted fixture output against BSArchPro-derived expected bytes or metadata. | VERIFIED | Same root evidence as truth 1, plus docs in `tests/fixtures/README.md:93` and `docs/compatibility-evidence.md:8`. |
| 7 | COMP-02: Maintainer can verify archives produced by libbsa load or validate as compatible with their target game/archive family. | VERIFIED | Writer-output validation tests passed for TES3, TES4, BA2 GNRL, and BA2 DX10. |
| 8 | COMP-03: Consumer can receive structured compatibility warnings for known Bethesda quirks without requiring a logging framework. | VERIFIED | Public warning API is dependency-light; warning tests assert stable code/severity/path facts, not message text. |
| 9 | COMP-04: Parser can gracefully reject malformed, truncated, oversized, or internally inconsistent archives. | VERIFIED | Malformed matrix, manifest expected-error guards, validation report tests, and full CTest all passed. |
| 10 | COMP-05: Maintainer can run sanitizer-backed malformed-input tests for parser and decompressor hardening. | VERIFIED | `CMakePresets.json:36` exposes `linux-clang-asan-ubsan`; `tests/fixtures/README.md:163` documents the malformed/validation/compression CTest command; policy test passed. |
| 11 | COMP-06: Maintainer can document each non-obvious compatibility rule with reference evidence or fixture coverage. | VERIFIED | Evidence catalog and policy tests passed; all public warning codes have rule/evidence sections. |

**Score:** 67/67 must-haves verified.
**COMP coverage:** 6/6 requirements verified.

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `tests/unit/local_game_fixture_tests.cpp` | Opt-in BSArchPro-derived comparison harness | VERIFIED | Registered in `tests/CMakeLists.txt:21`; discovers manifest by env/local fixture root; opens archives with `archive_reader`; compares archive metadata, entry metadata, exact `bytes_hex`, or streaming FNV/size. |
| `tests/fixtures/README.md` | Fixture policy and BSArchPro manifest schema | VERIFIED | Documents env vars, local-only manifest rules, metadata and payload expectation schema, and TES5Edit read-only boundary. |
| `docs/compatibility-evidence.md` | Compatibility evidence catalog | VERIFIED | Describes generated/writer-output mandatory evidence and the executable optional BSArchPro comparison harness. |
| `tests/unit/validation_policy_tests.cpp` | Machine-checked fixture/evidence policy | VERIFIED | Checks warning-code catalog entries, local fixture policy, BSArchPro manifest documentation, sanitizer path, and CI/TES5Edit boundaries. |
| `include/libbsa/validation.hpp` | Public validation API and structured warnings | VERIFIED | Dependency-light C++20 public contract with stable warning identifiers and validation report shape. |
| `src/validation.cpp` | Strict-open-backed validation implementation | VERIFIED | Uses `archive_reader::open`; setup failures remain result-level; inspectable malformed archives become report diagnostics; warnings derive from parsed metadata/entries. |
| `tests/unit/validation_api_tests.cpp` | Validation success, writer-output, and malformed behavior tests | VERIFIED | Covers generated fixtures, writer output, result-level errors, extractability caps, and malformed matrix rows. |
| `tests/unit/compatibility_warning_tests.cpp` | Compatibility warning scenarios | VERIFIED | Covers all public warning codes. |
| `tests/fixtures/generated/compatibility_matrix.json` | Consolidated malformed hardening matrix | VERIFIED | Covers TES3 BSA, TES4 BSA, BA2 GNRL, BA2 DX10, truncation, duplicate paths, invalid spans, unsupported routes, decompression failures, oversized arithmetic, and DDS chunk layout. |
| `tests/fixtures/generated/validate_fixture_manifests.py` | Manifest and matrix validator | VERIFIED | Validates matrix schema, expected-error categories, and manifest/test evidence references. |
| `CMakePresets.json` | Static/shared and additive sanitizer presets | VERIFIED | Lists Windows static/shared and Linux Clang ASan/UBSan configure/build/test presets. |

`gsd-sdk query verify.artifacts` passed for all 20 declared plan artifacts.

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `tests/CMakeLists.txt` | `tests/unit/local_game_fixture_tests.cpp` | test target source registration | VERIFIED | Source file is in `libbsa_tests`; `catch_discover_tests(... ADD_TAGS_AS_LABELS ...)` exposes Catch2 tags as CTest labels. |
| `tests/unit/local_game_fixture_tests.cpp` | local BSArchPro manifest | `LIBBSA_BSARCHPRO_EXPECTED` / `LIBBSA_GAME_FIXTURES` | VERIFIED | Manifest path resolution and default skip are implemented. |
| `tests/unit/local_game_fixture_tests.cpp` | `archive_reader` extraction APIs | `open`, `find`, `metadata`, `extract_bytes`, `extract` | VERIFIED | Comparison loop opens each archive and verifies metadata plus bytes/hash expectations. |
| `include/libbsa/libbsa.hpp` | `include/libbsa/validation.hpp` | umbrella include | VERIFIED | GSD key-link helper verified the include. |
| `src/validation.cpp` | strict reader path | `archive_reader::open` | VERIFIED | GSD key-link helper verified the strict-open source of truth. |
| `tests/unit/validation_api_tests.cpp` | generated fixtures and writer output | validation API tests | VERIFIED | Generated/writer-output validation paths are present and passing. |
| `include/libbsa/validation.hpp` | `docs/compatibility-evidence.md` | warning code catalog coverage | VERIFIED | Policy test parses public warning codes and requires evidence entries. |
| `tests/fixtures/generated/compatibility_matrix.json` | manifests and test evidence | manifest/test references | VERIFIED | Python and C++ validators resolve references. |
| `CMakePresets.json` | `tests/fixtures/README.md` | documented sanitizer command | VERIFIED | GSD key-link helper verified the preset/docs link. |

The 11-05 GSD key-link helper still cannot expand the wildcard source path `tests/unit/*reader_tests.cpp`; manual `rg` verified the strict `FAIL("unknown ... expected_error")` mappings in TES3, TES4, and BA2 GNRL reader tests and no `invalid_argument` fallback remains.

## Data-Flow Trace

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `tests/unit/local_game_fixture_tests.cpp` | `manifest_path` | `LIBBSA_BSARCHPRO_EXPECTED` or `LIBBSA_GAME_FIXTURES/bsarchpro_expected.json` | Yes, when maintainer supplies opt-in local manifest | FLOWING |
| `tests/unit/local_game_fixture_tests.cpp` | expected archive and entries | JSON `cases[]` / `entries[]` | Yes | FLOWING |
| `tests/unit/local_game_fixture_tests.cpp` | extracted payload comparison | `archive_reader::extract_bytes` or streaming `extract` into FNV sink | Yes | FLOWING |
| `src/validation.cpp` | `validation_report` | `archive_reader::open`, metadata, entries, optional extraction | Yes | FLOWING |
| `src/validation.cpp` | compatibility warnings | parsed archive metadata, entries, caller validation options | Yes | FLOWING |
| `tests/unit/validation_api_tests.cpp` | malformed matrix rows | `compatibility_matrix.json` plus generated manifests/test tokens | Yes | FLOWING |
| `tests/unit/validation_policy_tests.cpp` | warning-code coverage | public header enum plus docs catalog | Yes | FLOWING |

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Static test target builds | `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` | Built `libbsa.lib` and `libbsa_tests.exe` | PASS |
| Focused COMP suite passes | `ctest --preset windows-msvc-debug-static -R "local game fixtures|BSArchPro-derived|validation_policy|validation_api|compatibility_warning|compatibility_matrix|public_include_boundary|package_consumer|validate_fixture_manifests" --output-on-failure` with local fixture env cleared | 18/18 passed; local game and BSArchPro tests skipped as expected without local corpus | PASS |
| Validation policy gates pass | `ctest --preset windows-msvc-debug-static -L validation_policy --output-on-failure` | 2/2 passed | PASS |
| Fixture policy gates pass | `ctest --preset windows-msvc-debug-static -R "local fixture policy|requires-game-fixture label|CTest label taxonomy|CI and presets" --output-on-failure` | 4/4 passed | PASS |
| Full static suite passes | `ctest --preset windows-msvc-debug-static --output-on-failure` | 189/189 passed; two opt-in local-corpus tests skipped | PASS |
| Generated manifest/matrix validator passes | `python tests/fixtures/generated/validate_fixture_manifests.py` | Exit 0 | PASS |
| Static/shared/sanitizer presets are visible | `cmake --list-presets=all` | Windows static/shared and Linux ASan/UBSan configure/build/test presets listed | PASS |
| TES5Edit remains read-only | `git status --short TES5Edit; git diff --stat -- TES5Edit` | No output | PASS |
| Working diff has no whitespace errors | `git diff --check` | No output | PASS |

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| COMP-01 | 11-04 plus gap closure `83bf32e` | Maintainer can compare extracted fixture output against BSArchPro-derived expected bytes/metadata. | SATISFIED | Executable opt-in harness exists, is registered, is labeled `requires-game-fixture`/`compat`, and compares metadata plus bytes or FNV/size. |
| COMP-02 | 11-02, 11-03 | Maintainer can verify libbsa-produced archives load or validate as target-compatible. | SATISFIED | Writer-output validation tests pass for TES3, TES4, BA2 GNRL, and BA2 DX10. |
| COMP-03 | 11-01, 11-03, 11-04 | Consumer can receive structured compatibility warnings without a logging framework. | SATISFIED | Public warning API and warning tests pass; public header remains dependency-light. |
| COMP-04 | 11-02, 11-05, 11-06 | Parser gracefully rejects malformed/truncated/oversized/inconsistent archives. | SATISFIED | Malformed matrix, strict manifest mappings, and validation report tests pass. |
| COMP-05 | 11-06, 11-07 | Maintainer can run sanitizer-backed malformed-input tests. | SATISFIED | Additive ASan/UBSan preset and documented malformed/validation/compression label command are present and policy-tested. |
| COMP-06 | 11-04, 11-07 | Maintainer can document non-obvious compatibility rules with evidence. | SATISFIED | Evidence catalog is machine-checked against public warning codes. |

No orphaned Phase 11 requirements were found. `.planning/REQUIREMENTS.md` maps COMP-01 through COMP-06 to Phase 11, and all six are claimed by phase plans.

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/validation.cpp` | 142 | `return {};` | Info | Benign `result<void>` success value after size checks, not a stub. |
| `tests/unit/validation_policy_tests.cpp` | 47 | `return {};` | Info | Empty trimmed string result in a helper, not user-visible stub behavior. |
| `tests/fixtures/generated/validate_fixture_manifests.py` | 152 | `{}` dict initialization | Info | Manifest cache initialization, not hardcoded output. |

No blocker anti-patterns were found.

## Human Verification Required

None for the phase gate. The BSArchPro-derived corpus comparison is intentionally opt-in and external-data-backed; the default suite verifies the harness exists, is wired, skips safely without local data, and documents the manifest contract.

## Deferred Items

None. No remaining Phase 11 gap needs deferral to Phase 12.

## Gaps Summary

No gaps remain. The previous COMP-01 blocker is closed by the executable opt-in comparison harness and policy/docs updates in `83bf32e`.

## Residual Risks

- No real `LIBBSA_BSARCHPRO_EXPECTED` manifest was present on this host, so the re-verification did not compare against an actual local BSArchPro export. The harness mechanics, wiring, skip behavior, and manifest contract were verified from code and tests.
- The `linux-clang-asan-ubsan` preset was source/policy verified on this Windows host but not executed locally.
- BSArchPro-derived manifests are trusted local oracle data; incorrect local expected metadata or hashes would make the harness compare against an incorrect oracle.

---

_Verified: 2026-05-10T05:22:39Z_
_Verifier: the agent (gsd-verifier)_
