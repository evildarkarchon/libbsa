## 1. Inventory

- [x] 1.1 Identify duplicated BA2 compatibility literals in parser, writer preparation, layout/serialization, and generated-fixture code.
- [x] 1.2 Identify duplicated TES4-family BSA compatibility literals in parser, writer preparation, layout/serialization, and generated-fixture code.
- [x] 1.3 Leave purely local algorithm limits and non-format implementation details out of the constants headers.

## 2. BA2 Constants

- [x] 2.1 Add `src/formats/ba2/ba2_constants.hpp` with typed `inline constexpr` values for BA2 signatures, subtype FourCCs, versions, header sizes, record sizes, sentinel values, Starfield compression methods, and DX10 texture markers.
- [x] 2.2 Preserve or add comments at non-obvious BA2 constants, including the `BAADF00D` record sentinel, DX10 chunk header width, `PackedSize == 0` raw-payload convention where applicable, and cubemap marker values.
- [x] 2.3 Update BA2 GNRL and DX10 parser code to include the BA2 constants header and remove duplicate local compatibility literals.
- [x] 2.4 Update BA2 GNRL and DX10 writer preparation, layout, and serialization code to use the same BA2 constants for shared on-disk values.
- [x] 2.5 Update BA2 GNRL and DX10 generated-fixture tools to use the BA2 constants header instead of independent copies of shared BA2 values.

## 3. TES4-Family BSA Constants

- [x] 3.1 Add `src/formats/bsa/tes4_bsa_constants.hpp` with typed `inline constexpr` values for TES4-family BSA versions, header size, folder/file record sizes, archive flag bits, file flag bits, embedded-name flag, and size-flag compression mask.
- [x] 3.2 Preserve or add comments at non-obvious TES4-family BSA constants, including compression toggle semantics and embedded-name/archive-flag compatibility constraints.
- [x] 3.3 Update TES4-family BSA parser code to include the BSA constants header and remove duplicate local compatibility literals.
- [x] 3.4 Update TES4-family BSA writer preparation, layout, and serialization code to use the same BSA constants for shared on-disk values.
- [x] 3.5 Update TES4-family BSA generated-fixture tools that duplicate shared BSA values to use the BSA constants header.

## 4. Verification

- [x] 4.1 Build the library and fixture-generator targets to verify internal headers and include paths compile.
- [x] 4.2 Run focused BA2 GNRL, BA2 DX10, and TES4-family BSA tests to verify parser, writer, and round-trip behavior is unchanged.
- [x] 4.3 Run or validate generated-fixture targets and confirm regenerated BA2/BSA fixtures continue satisfying existing fixture tests.
- [x] 4.4 Check public headers and install/export configuration to confirm the new constants headers are not exposed as public API.
