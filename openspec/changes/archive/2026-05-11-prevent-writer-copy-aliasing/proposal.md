## Why

Public writer classes currently rely on compiler-generated copy operations while storing mutable writer state behind `std::shared_ptr`. Copying a writer aliases its entries and options, so mutating the copy can unexpectedly mutate the original and violates the documented independent-writer ownership model.

## What Changes

- **BREAKING**: Make TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writer classes non-copyable so copies cannot share mutable writer state.
- Preserve explicit move support so callers can transfer writer ownership into containers or helper functions without aliasing mutable state.
- Add compile-time regression coverage that proves writer classes are not copy constructible or copy assignable and remain move constructible and move assignable.
- Keep runtime archive behavior, writer options, add-entry APIs, `write_to` semantics, and emitted bytes unchanged.

## Capabilities

### New Capabilities
- `writer-state-ownership`: Public archive writer objects own mutable state exclusively, cannot be copied into aliasing handles, and remain safely movable for ownership transfer.

### Modified Capabilities

## Impact

- Affected public API: `tes3_bsa_writer`, `tes4_bsa_writer`, `ba2_gnrl_writer`, and `ba2_dx10_writer` copy and move special member functions in `include/libbsa/writer.hpp`.
- Affected implementation: writer PIMPL ownership in the four writer translation units under `src/formats/bsa/` and `src/formats/ba2/`.
- Affected tests: writer unit tests gain compile-time special-member assertions.
- No new runtime dependencies, archive format changes, or `TES5Edit/` changes.
