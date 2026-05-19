# Plan: CONCERNS.md Tech Debt Refactor

## Strategy

Address the actionable Tech Debt without widening public API behavior. Keep strict parsing and the warning taxonomy unchanged because those require external compatibility evidence. Focus on reducing coupled parser logic and centralizing reusable safety helpers.

## Wave 1: Centralize safe span overlap policy

Files:

- `src/detail/parser_primitives.hpp`
- `src/detail/parser_primitives.cpp`
- `src/formats/ba2/ba2_dx10_parser.cpp`
- `src/formats/ba2/ba2_gnrl_parser.cpp`
- `src/formats/bsa/tes4_bsa_parser.cpp`
- `tests/unit/parser_primitives_tests.cpp`

Changes:

- Add `detail::spans_overlap_u64()` implemented via checked/saturating end math instead of unchecked `start + length`.
- Replace local parser copies with the central helper.
- Add edge-case tests for UInt64-near-max spans.

Verification:

- Build library/tests.
- Run `parser_primitives` focused tests.

## Wave 2: Split BA2 DX10 fixed-header/record and filename-table readers

Files:

- New `src/formats/ba2/ba2_dx10_records.hpp`
- New `src/formats/ba2/ba2_dx10_records.cpp`
- New `src/formats/ba2/ba2_dx10_names.hpp`
- New `src/formats/ba2/ba2_dx10_names.cpp`
- `src/formats/ba2/ba2_dx10_parser.cpp`
- `CMakeLists.txt`
- `tests/unit/parser_preparer_seam_policy_tests.cpp`
- Optionally `tests/unit/ba2_dx10_parser_tests.cpp` for helper-level behavior.

Changes:

- Move BA2 DX10 header size/header parsing and record/chunk parsing into `ba2_dx10_records`.
- Move count-delimited filename-table parsing for in-memory and file-backed paths into `ba2_dx10_names`.
- Keep public entry materialization in `ba2_dx10_parser.cpp` for now to limit blast radius.
- Add static seam policy evidence so the parser cannot silently collapse back into fixed-table/name-table readers.

Verification:

- Reconfigure if needed so new sources are included.
- Build tests.
- Run BA2 DX10 parser/metadata/extraction tests and seam policy tests.

## Wave 3: Documentation and full regression cleanup

Files:

- `CONCERNS.md` if the implemented debt should be marked narrowed rather than fully resolved.
- Workflow `SUMMARY.md` artifact.

Changes:

- Update concern wording only if appropriate: BA2 DX10 parser debt narrowed by extracted helper seams; GNRL/TES3/TES4 remain future candidates.
- Record strict parser and validation-warning decisions as explicitly unchanged due evidence requirements.

Verification:

- Full build.
- Full CTest where practical in this environment.
- Remnant search for local `spans_overlap_u64` definitions outside `parser_primitives` and for accidental `TES5Edit/` changes.

## Commit plan

Commit working source/test changes after a passing focused verification using:

`refactor(parser): wave 1-2 — split BA2 DX10 parser seams`

Only files changed by this workflow should be staged; pre-existing unrelated working-tree changes must remain untouched.
