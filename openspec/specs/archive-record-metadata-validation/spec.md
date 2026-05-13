## Purpose

TBD - Archive record metadata validation requirements synced from active changes.

## Requirements

### Requirement: BA2 GNRL record extensions match filename table paths
The BA2 GNRL parser SHALL preserve each record's 4-byte extension field and MUST reject an entry when that field does not match the extension derived from the corresponding filename table path using the BA2 GNRL extension FourCC rules.

#### Scenario: GNRL record extension mismatch is rejected
- **WHEN** a BA2 GNRL archive record stores extension bytes for `dds` but the corresponding filename table path is `meshes/a.nif`
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the mismatched entry is not materialized in public metadata

#### Scenario: GNRL record extension agrees with filename table
- **WHEN** a BA2 GNRL archive record extension matches the corresponding filename table path extension after applying the format's FourCC padding rules
- **THEN** extension validation does not prevent the parser from materializing the entry

### Requirement: BA2 DX10 record extensions match texture filename table paths
The BA2 DX10 parser SHALL preserve each texture record's 4-byte extension field and MUST reject an entry when that field does not match the extension derived from the corresponding texture filename table path using the BA2 DX10 extension FourCC rules.

#### Scenario: DX10 record extension mismatch is rejected
- **WHEN** a BA2 DX10 texture record stores extension bytes that disagree with the corresponding filename table path extension
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** no texture entry is exposed from the inconsistent record

#### Scenario: DX10 record extension agrees with filename table
- **WHEN** a BA2 DX10 texture record extension matches the corresponding filename table path extension after applying the format's FourCC padding rules
- **THEN** extension validation does not prevent the parser from materializing the texture entry

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

### Requirement: TES4-family BSA folder block offsets match parsed layout
The TES4-family BSA parser SHALL validate each stored folder record offset against the expected folder-block location for that record and MUST reject an archive whose stored offset points to a different in-range location.

#### Scenario: TES4 folder block offset is stale but in range
- **WHEN** a TES4-family BSA folder record stores an offset that is inside the archive but does not equal the expected folder-block location for that record
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not recover by consuming folder blocks only from the current sequential parser position

#### Scenario: TES4 folder block offset matches parsed layout
- **WHEN** each TES4-family BSA folder record stores the expected folder-block offset for the parsed table layout
- **THEN** folder-offset validation does not prevent the parser from materializing entries

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

### Requirement: Archive-declared metadata counts are bounded before parser work
TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 parsers SHALL reject archive-declared metadata counts that exceed libbsa's internal safety policy before reserving metadata containers, building duplicate-detection sets, sorting public entries, or materializing public texture chunk metadata. Count-limit rejections MUST return `libbsa::error_code::format_error`.

#### Scenario: TES3 file count exceeds metadata policy
- **WHEN** a TES3 BSA archive declares a file count greater than libbsa's internal metadata entry limit
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not materialize file records, name offsets, hashes, duplicate sets, or public entries for the declared count

#### Scenario: TES4 folder count exceeds metadata policy
- **WHEN** a TES4-family BSA archive declares a folder count greater than libbsa's internal folder limit
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not reserve folder records or folder blocks for the declared count

#### Scenario: TES4 file count exceeds metadata policy
- **WHEN** a TES4-family BSA archive declares a total file count or per-folder file count that exceeds libbsa's internal metadata entry limit
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not reserve file records, filename entries, duplicate sets, or public entries for the excessive count

#### Scenario: BA2 GNRL file count exceeds metadata policy
- **WHEN** a BA2 GNRL archive declares a file count greater than libbsa's internal metadata entry limit
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not reserve GNRL records, filename entries, duplicate sets, or public entries for the declared count

#### Scenario: BA2 DX10 file count exceeds metadata policy
- **WHEN** a BA2 DX10 archive declares a file count greater than libbsa's internal metadata entry limit
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not reserve texture records, filename entries, duplicate sets, or public entries for the declared count

#### Scenario: BA2 DX10 aggregate chunk count exceeds metadata policy
- **WHEN** a BA2 DX10 archive's parsed texture records declare an aggregate chunk count greater than libbsa's internal texture chunk limit
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not materialize public texture chunk metadata or public entries for the excessive aggregate chunk count

### Requirement: TES3 payload overlap validation avoids quadratic scans
The TES3 BSA parser SHALL validate payload overlaps using ordered non-empty archive-absolute payload spans rather than comparing each entry span against every previously seen entry span. It MUST preserve TES3 stored-hash ordering, duplicate stored-hash, stored-hash/name, canonical-path, data-section-relative offset, and archive-bound validation before reporting payload overlap failures.

#### Scenario: Overlapping non-empty TES3 payload spans are rejected
- **WHEN** a TES3 BSA archive contains two non-empty entries whose data-section-relative payload ranges overlap after conversion to archive-absolute offsets
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** no public entry metadata is materialized from the overlapping payload layout

#### Scenario: Adjacent non-overlapping TES3 payload spans are accepted
- **WHEN** a TES3 BSA archive contains non-empty entries whose payload end offsets exactly match the next payload start offsets without crossing
- **THEN** payload overlap validation does not prevent the parser from materializing entries
- **THEN** public entry metadata exposes the same archive-absolute payload offsets derived from the TES3 data section

#### Scenario: Zero-byte TES3 entries do not occupy payload bytes
- **WHEN** a TES3 BSA archive contains a zero-byte entry at an offset that is also the boundary of another entry's payload range
- **THEN** payload overlap validation does not treat the zero-byte entry as overlapping payload data
- **THEN** the zero-byte entry may still be materialized if all other TES3 metadata validation succeeds

#### Scenario: Hash-order validation keeps precedence over overlap validation
- **WHEN** a TES3 BSA archive contains unsorted stored hash records and also contains payload records that would overlap after offset conversion
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error` for the hash-order violation before payload overlap validation changes the failure precedence

#### Scenario: Large non-overlapping TES3 payload sets avoid prior-span scans
- **WHEN** a TES3 BSA archive contains a large number of valid non-overlapping payload spans in hash-table order that differs from payload-offset order
- **THEN** payload overlap validation orders spans for adjacency checking instead of scanning each span against every previously seen span
- **THEN** the parser can materialize the archive entries without quadratic overlap-validation work
