## ADDED Requirements

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
