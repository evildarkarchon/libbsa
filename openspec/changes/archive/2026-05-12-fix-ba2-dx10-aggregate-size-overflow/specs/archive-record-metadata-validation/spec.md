## ADDED Requirements

### Requirement: BA2 DX10 aggregate payload sizes fit public metadata
The BA2 DX10 parser SHALL materialize aggregate texture payload sizes with checked `std::uint64_t` arithmetic before exposing public entry metadata. It MUST reject an entry if summing chunk raw sizes, summing chunk stored sizes, or adding the reconstructed DDS header size to the raw aggregate would overflow `std::uint64_t`.

#### Scenario: BA2 DX10 archive field bounds prevent encoded UInt64 aggregate overflow
- **WHEN** the parser applies the BA2 DX10 record limits of at most 255 chunks per texture and UInt32 raw and stored chunk sizes
- **THEN** the maximum raw aggregate, stored aggregate, and DDS-header-plus-raw public size fit `std::uint64_t`
- **THEN** malformed overflow coverage is provided through checked-add helper tests rather than impossible archive fixtures

#### Scenario: Internal aggregate overflow is rejected before public metadata exposure
- **WHEN** aggregate materialization is asked to add a chunk size or DDS header size that would overflow `std::uint64_t`
- **THEN** the checked addition fails before updating the running total
- **THEN** the parser reports `libbsa::error_code::format_error` rather than exposing wrapped entry metadata

#### Scenario: Aggregate texture payload sizes fit
- **WHEN** a BA2 DX10 texture record's ordered chunks have raw and stored aggregate sizes that fit `std::uint64_t`
- **THEN** aggregate overflow validation does not prevent the parser from materializing the texture entry
- **THEN** the public entry metadata exposes the exact raw size including the reconstructed DDS header and the exact aggregate stored size
