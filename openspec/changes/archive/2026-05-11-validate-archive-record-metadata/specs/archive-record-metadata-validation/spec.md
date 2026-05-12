## ADDED Requirements

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

### Requirement: TES4-family BSA folder block offsets match parsed layout
The TES4-family BSA parser SHALL validate each stored folder record offset against the expected folder-block location for that record and MUST reject an archive whose stored offset points to a different in-range location.

#### Scenario: TES4 folder block offset is stale but in range
- **WHEN** a TES4-family BSA folder record stores an offset that is inside the archive but does not equal the expected folder-block location for that record
- **THEN** opening or validating the archive fails with `libbsa::error_code::format_error`
- **THEN** the parser does not recover by consuming folder blocks only from the current sequential parser position

#### Scenario: TES4 folder block offset matches parsed layout
- **WHEN** each TES4-family BSA folder record stores the expected folder-block offset for the parsed table layout
- **THEN** folder-offset validation does not prevent the parser from materializing entries
