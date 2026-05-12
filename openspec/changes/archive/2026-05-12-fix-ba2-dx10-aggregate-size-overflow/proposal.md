## Why

BA2 DX10 parsing currently accumulates per-chunk decoded and stored sizes into public `entry_metadata` with unchecked `std::uint64_t` addition. The on-disk DX10 schema bounds one texture record to a UInt8 chunk count and UInt32 chunk sizes, so a real archive cannot overflow `std::uint64_t` today; however, the parser should still use checked arithmetic at the public metadata boundary and document that invariant so future internal widening cannot reintroduce wrapped metadata.

## What Changes

- Guard BA2 DX10 raw payload aggregation with checked `std::uint64_t` addition before updating the running total.
- Guard BA2 DX10 stored payload aggregation with checked `std::uint64_t` addition before updating the running total.
- Guard the reconstructed DDS header addition before assigning the public uncompressed entry size.
- Add focused checked-add helper coverage and a parser test documenting why a malformed archive fixture cannot trigger this overflow under current BA2 DX10 field widths.
- Preserve existing public API shape and error model by returning `libbsa::error_code::format_error` for invalid archive metadata.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `archive-record-metadata-validation`: BA2 DX10 metadata materialization must use checked aggregate arithmetic and preserve exact public metadata for representable records.

## Impact

- Affected code: `src/formats/ba2/ba2_dx10_parser.cpp` entry materialization.
- Affected tests/fixtures: parser primitive checked-add tests and BA2 DX10 parser tests documenting the archive-schema aggregate bound.
- Public API: no signature changes; malformed archives that previously produced wrapped metadata will now fail with `format_error`.
- Dependencies: none.
