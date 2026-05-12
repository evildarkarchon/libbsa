## Why

BA2 GNRL host-file parsing computes the aggregate filename table end as `FileTableOffset + name_table_consumed` after reading individual length-prefixed names. A malformed archive with a very high `FileTableOffset` can make that sum wrap `std::uint64_t`, which can hide filename-table overlap with payload data and publish metadata derived from invalid archive layout.

## What Changes

- Guard the BA2 GNRL host-file filename table end calculation with checked `std::uint64_t` addition before materializing entries.
- Return `libbsa::error_code::format_error` when the parsed filename table span cannot be represented as a 64-bit archive range.
- Add focused parser coverage for the malformed high-`FileTableOffset` case without changing successful BA2 GNRL parsing behavior.
- Preserve the existing public API shape, archive metadata types, compression routing, and read/extract semantics.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `archive-record-metadata-validation`: BA2 GNRL metadata validation must reject filename table spans whose aggregate end overflows `std::uint64_t`.

## Impact

- Affected code: `src/formats/ba2/ba2_gnrl_parser.cpp` host-file open path after filename table parsing.
- Affected tests: BA2 GNRL reader/parser malformed archive coverage for overflowing aggregate filename table end calculation.
- Public API: no signature changes; malformed archives that previously could wrap the filename table end now fail with `format_error`.
- Dependencies: none.
