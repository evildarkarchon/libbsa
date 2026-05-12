## 1. Regression Tests

- [x] 1.1 Add a BA2 GNRL malformed-open regression that mutates a valid generated fixture's first record extension bytes to disagree with the filename table and expects `libbsa::error_code::format_error`.
- [x] 1.2 Add a BA2 DX10 malformed-open regression that mutates a valid generated fixture's texture record extension bytes to disagree with the filename table and expects `libbsa::error_code::format_error`.
- [x] 1.3 Add or confirm a TES4-family BSA malformed-open regression that mutates an in-range folder record offset to a stale value and expects `libbsa::error_code::format_error`.

## 2. Parser Implementation

- [x] 2.1 Update `src/formats/ba2/ba2_gnrl_parser.cpp` so `gnrl_record` preserves the 4-byte extension field instead of skipping it.
- [x] 2.2 Add BA2 GNRL extension FourCC derivation from the filename table path and reject mismatches before materializing `entry_metadata`.
- [x] 2.3 Update `src/formats/ba2/ba2_dx10_parser.cpp` so `dx10_record` preserves the 4-byte extension field instead of skipping it.
- [x] 2.4 Add BA2 DX10 extension FourCC derivation from the parsed texture filename extension and reject mismatches before materializing `entry_metadata`.
- [x] 2.5 Audit `src/formats/bsa/tes4_bsa_parser.cpp` and add any missing expected-folder-offset checks in both metadata validation and entry materialization paths.

## 3. Verification

- [x] 3.1 Build the Windows MSVC debug static preset with tests enabled.
- [x] 3.2 Run targeted BA2 GNRL, BA2 DX10, and TES4 BSA unit tests that cover malformed parser input.
- [x] 3.3 Run `ctest --preset windows-msvc-debug-static --output-on-failure` and confirm the full configured test suite passes.
