## Why

Archive parsers currently need stronger validation for metadata that game and tool lookup paths consume independently from published entry names. If record extensions or folder-block offsets disagree with the filename tables, `open()` and `validate_archive()` can accept archives whose entries are not resolvable by consumers that trust the stored lookup metadata.

## What Changes

- Preserve and validate BA2 GNRL record extension bytes against the parsed filename extension before publishing an entry.
- Preserve and validate BA2 DX10 record extension bytes against the parsed texture filename extension before publishing an entry.
- Validate TES4-family BSA folder record offsets against the expected folder-block layout, not only archive bounds.
- Add malformed fixture coverage proving these metadata mismatches fail through `result` errors with `libbsa::error_code::format_error`.

## Capabilities

### New Capabilities
- `archive-record-metadata-validation`: Defines parser behavior for archive lookup metadata that must agree with filename-table-derived entry paths.

### Modified Capabilities

## Impact

- Affects BA2 GNRL parsing in `src/formats/ba2/ba2_gnrl_parser.cpp`.
- Affects BA2 DX10 parsing in `src/formats/ba2/ba2_dx10_parser.cpp`.
- Affects TES4-family BSA parsing in `src/formats/bsa/tes4_bsa_parser.cpp`.
- Adds focused malformed archive tests; no public API or dependency changes are expected.
