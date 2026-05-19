# Summary: CONCERNS.md Tech Debt Refactor

## What changed and why

Addressed the actionable portion of the `CONCERNS.md` Tech Debt section without changing public parsing strictness or validation-warning API behavior.

- Split BA2 DX10 raw parser responsibilities out of `src/formats/ba2/ba2_dx10_parser.cpp`:
  - `src/formats/ba2/ba2_dx10_records.{hpp,cpp}` now owns fixed-header parsing, Starfield v2/v3 header fields, record/chunk table parsing, chunk header-size enforcement, and aggregate chunk-count limits.
  - `src/formats/ba2/ba2_dx10_names.{hpp,cpp}` now owns in-memory filename-table decoding and file-backed count-delimited filename-table reads for sparse archives.
  - `ba2_dx10_parser.cpp` now orchestrates these seams and retains public entry/texture metadata materialization.
- Centralized repeated `spans_overlap_u64()` parser helpers into `src/detail/parser_primitives.{hpp,cpp}` with overflow-safe/saturating end-offset arithmetic.
- Replaced local overlap helpers in BA2 DX10, BA2 GNRL, and TES4 BSA parsing with the central primitive.
- Added seam policy coverage so the BA2 DX10 parser cannot silently collapse header/record and filename-table readers back into the monolithic parser.
- Updated `CONCERNS.md` to mark the BA2 DX10 debt as narrowed and the overlap arithmetic known bug as resolved.

Strict parser behavior and the public compatibility-warning taxonomy were intentionally left unchanged because both require compatibility evidence before relaxing failures or adding public warning codes.

## Files modified

13 source/test/doc files plus 4 new internal helper files:

- `CMakeLists.txt`
- `CONCERNS.md`
- `src/detail/parser_primitives.hpp`
- `src/detail/parser_primitives.cpp`
- `src/formats/ba2/ba2_dx10_parser.cpp`
- `src/formats/ba2/ba2_dx10_records.hpp` (new)
- `src/formats/ba2/ba2_dx10_records.cpp` (new)
- `src/formats/ba2/ba2_dx10_names.hpp` (new)
- `src/formats/ba2/ba2_dx10_names.cpp` (new)
- `src/formats/ba2/ba2_gnrl_parser.cpp`
- `src/formats/bsa/tes4_bsa_parser.cpp`
- `tests/unit/parser_primitives_tests.cpp`
- `tests/unit/parser_preparer_seam_policy_tests.cpp`

Workflow artifacts:

- `.gsd/workflows/refactors/260519-2-let-s-address-the-tech-debt-section-of-c/INVENTORY.md`
- `.gsd/workflows/refactors/260519-2-let-s-address-the-tech-debt-section-of-c/PLAN.md`
- `.gsd/workflows/refactors/260519-2-let-s-address-the-tech-debt-section-of-c/SUMMARY.md`

## Verification

Passed:

- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`
- Focused CTest: `ctest --test-dir build/windows-msvc-debug-static -C Debug --output-on-failure -R "parser_primitives|parser_preparer_seam_policy|ba2_dx10_detector|ba2_dx10_metadata"`
  - 23/23 passed.
- Full CTest: `ctest --test-dir build/windows-msvc-debug-static -C Debug --output-on-failure`
  - 416/416 tests passed, with 2 expected opt-in/local fixture skips.
- Whitespace/static checks:
  - `git diff --check -- ...` on tracked touched files produced no whitespace errors.
  - Node whitespace check for new untracked helper source/header files passed.
- Remnant search:
  - `rg -n "bool spans_overlap_u64" src` now reports only `src/detail/parser_primitives.hpp` and `src/detail/parser_primitives.cpp`.
- TES5Edit boundary:
  - `git status --short TES5Edit` returned no changes.

## Follow-up items

- BA2 DX10 still has candidate split points: stored chunk span validation and public texture metadata/materialization.
- BA2 GNRL, TES3 BSA, and TES4 BSA prepare remain broader modules and should be split using the same seam-first pattern when those areas are next modified.
- Keep strict parser leniency and new compatibility warnings evidence-gated through `docs/compatibility-evidence.md` and generated/local fixture coverage.
