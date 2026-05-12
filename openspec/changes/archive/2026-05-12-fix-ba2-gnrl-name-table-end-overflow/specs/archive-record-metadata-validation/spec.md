## ADDED Requirements

### Requirement: BA2 GNRL filename table spans fit archive ranges
The BA2 GNRL parser SHALL compute parsed filename table span ends with checked `std::uint64_t` arithmetic before using that span for payload overlap validation or public metadata materialization. It MUST reject an archive if adding `FileTableOffset` and the parsed filename table byte count would overflow `std::uint64_t`.

#### Scenario: Host-file filename table end overflows
- **WHEN** a BA2 GNRL archive declares a `FileTableOffset` high enough that adding the parsed length-prefixed filename table byte count would overflow `std::uint64_t`
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** no entry metadata is materialized from the wrapped filename table span

#### Scenario: Filename table span fits
- **WHEN** a BA2 GNRL archive's parsed filename table byte count can be added to `FileTableOffset` without overflowing `std::uint64_t`
- **THEN** filename table span validation does not prevent the parser from materializing entries
- **THEN** payload overlap validation uses the exact parsed filename table range
