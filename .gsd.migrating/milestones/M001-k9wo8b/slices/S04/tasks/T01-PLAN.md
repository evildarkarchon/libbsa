---
estimated_steps: 6
estimated_files: 2
skills_used: []
---

# T01: Prove validation success and warning contracts

Expected executor skills: cpp-testing, tdd, api-design, verify-before-complete.

Why: COV-GAP-001 is an evidence gap, not an API redesign request. Existing validation behavior already separates setup/result failures from inspectable archive diagnostics, but `validation_api_tests.cpp` only directly accepts four generated success fixtures and leaves several variant routes indirect. This task turns that into public-contract proof through `validate_archive` and stable codes/warning shapes.

Do: Extend `tests/unit/validation_api_tests.cpp` through public `<libbsa/libbsa.hpp>` calls only. Expand the generated success fixture matrix to include TES4 v104, TES4 v105, BA2 GNRL Starfield v2, BA2 GNRL Starfield v3, and BA2 DX10 Starfield v3, keeping `validate_entry_extractability = true` and adding `INFO` context that names the fixture. Add compact writer-produced validation cases for Starfield BA2 v3 compression-method routes that generated fixtures do not fully enumerate, especially method 0 deflate and method 3 raw LZ4 block for GNRL and DX10 where existing public writers can produce them. Add a focused expected-variant mismatch validation case that proves a structurally valid archive remains `report.valid == true` but carries a `target_family_mismatch` compatibility warning with risky severity. Preserve the current public error categories and warning enum; if a new test fails, make the smallest production fix in `src/validation.cpp` only when the failure is a real semantic mismatch rather than a test assumption.

Done when: `validation_api` tests directly prove the variant matrix, Starfield method routes, setup/result boundary, malformed/report boundary, and expected-variant warning behavior without exact-message contracts or private parser assertions.

Q5 Failure Modes: generated fixture missing means the test should fail with a clear `INFO` fixture name; writer setup I/O errors must stay test failures rather than skipped proof; malformed/unreadable archive bytes must remain report-level diagnostics when the host path can be opened.
Q7 Negative Tests: keep existing setup-error and malformed-matrix tests green; add/keep a mismatch warning assertion proving valid-but-risky conditions are warnings, not fatal errors.

## Inputs

- `include/libbsa/result.hpp`
- `include/libbsa/validation.hpp`
- `include/libbsa/archive.hpp`
- `include/libbsa/writer.hpp`
- `src/validation.cpp`
- `tests/unit/validation_api_tests.cpp`
- `tests/unit/compatibility_warning_tests.cpp`
- `tests/unit/ba2_gnrl_writer_tests.cpp`
- `tests/unit/ba2_dx10_writer_tests.cpp`
- `tests/fixtures/generated/archives/tes4_v104.bsa`
- `tests/fixtures/generated/archives/tes4_v105.bsa`
- `tests/fixtures/generated/archives/ba2_gnrl_sfv2.ba2`
- `tests/fixtures/generated/archives/ba2_gnrl_sfv3.ba2`
- `tests/fixtures/generated/archives/ba2_dx10_sfv3.ba2`
- `tests/fixtures/generated/source/ba2_dx10_bc1_unorm.dds`

## Expected Output

- `tests/unit/validation_api_tests.cpp`

## Verification

cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
ctest --preset windows-msvc-debug-static -L validation_api --output-on-failure
ctest --preset windows-msvc-debug-static -R compatibility_warning --output-on-failure

## Observability Impact

Adds Catch2 assertion context that makes validation failures localize to a specific fixture, writer target, compression method, or warning contract. No runtime observability surface changes.
