# ba2-gnrl-read Specification

## Purpose
TBD - created by syncing change ba2-gnrl-read-support. Update Purpose after archive.

## Requirements
### Requirement: BA2 GNRL format detection
libbsa SHALL detect BA2 GNRL archives by `BTDX` magic followed by a supported version (`0x01`, `0x02`, `0x03`, `0x07`, or `0x08`) and `GNRL` sub-header magic. The detection logic SHALL route BTDX-magic files to the BA2 parser without entering the TES4-family path.

#### Scenario: Supported FO4 BA2 GNRL archive is opened
- **WHEN** a caller opens a `BTDX` archive with version `0x01`, `0x07`, or `0x08` and sub-header magic `GNRL`
- **THEN** libbsa reports the archive as `ArchiveFormat::fo4` and makes the read API available.

#### Scenario: Supported Starfield BA2 GNRL archive is opened
- **WHEN** a caller opens a `BTDX` archive with version `0x02` or `0x03` and sub-header magic `GNRL`
- **THEN** libbsa reports the archive as `ArchiveFormat::starfield` and makes the read API available.

#### Scenario: Unsupported BA2 version is opened
- **WHEN** a caller opens a `BTDX` archive with a version not in `{0x01, 0x02, 0x03, 0x07, 0x08}`
- **THEN** libbsa returns an `unsupported_format` error.

#### Scenario: DX10 sub-header is detected
- **WHEN** a caller opens a `BTDX` archive with sub-header magic `DX10` (not `GNRL`)
- **THEN** libbsa returns an `unsupported_format` error indicating DDS BA2 archives are not yet supported.

### Requirement: BA2 GNRL header and record parsing
libbsa SHALL parse the BTDX outer header (magic, version), the FO4 sub-header (type magic, file count, file table offset), optional Starfield extension fields, and all per-file GNRL records into queryable metadata.

#### Scenario: GNRL file records are parsed
- **WHEN** a supported BA2 GNRL archive is opened successfully
- **THEN** the caller can list every stored file record with its name hash, directory hash, extension, data offset, packed size, and raw size.

#### Scenario: Starfield v2 extra header fields are parsed
- **WHEN** a Starfield v2 BA2 GNRL archive is opened
- **THEN** libbsa reads the additional `Unknown1` and `Unknown2` header fields without error and sets the decompression method to deflate (default for v2).

#### Scenario: Starfield v3 compression method is parsed
- **WHEN** a Starfield v3 BA2 GNRL archive is opened
- **THEN** libbsa reads `Unknown1`, `Unknown2`, and `CompressionMethod`, using LZ4 block only when `CompressionMethod` is `3` and deflate otherwise.

#### Scenario: BAADF00D sentinel is consumed
- **WHEN** the parser reads each GNRL file record
- **THEN** it reads and discards the 4-byte `0xBAADF00D` sentinel trailing each record without treating it as an error.

### Requirement: BA2 file-name table parsing
libbsa SHALL parse the trailing file-name table located at `FileTableOffset`. Each name is a 16-bit length-prefixed string. Path separators SHALL be normalized to backslashes.

#### Scenario: File names are associated with records
- **WHEN** a BA2 GNRL archive with a valid file-name table is opened
- **THEN** each `ArchiveEntry` in the entries list has its `path` field populated with the corresponding name from the table.

#### Scenario: File-name table is beyond EOF
- **WHEN** `FileTableOffset` points beyond the end of the file
- **THEN** libbsa returns a `malformed_archive` error.

#### Scenario: File-name count mismatch
- **WHEN** the file-name table contains fewer entries than `FileCount`
- **THEN** libbsa returns a `malformed_archive` error.

### Requirement: BA2 CRC32-based hash algorithm
libbsa SHALL implement a CRC32-based hash function compatible with BSArchPro `CreateHashFO4` for directory names and file names. The hash SHALL process each ASCII character individually with normalization: lowercase conversion, `/` to `\` substitution, and bytes > 127 skipped.

#### Scenario: Directory and file-name hashes are computed
- **WHEN** libbsa hashes a directory path and file name from a known BA2 fixture
- **THEN** the computed hashes match the hashes stored in that fixture's file records.

#### Scenario: Path normalization applied before hashing
- **WHEN** a path contains uppercase characters or forward slashes
- **THEN** the hash function produces the same result as the lowercase, backslash-normalized equivalent.

#### Scenario: Non-ASCII bytes are skipped
- **WHEN** a path contains bytes with values > 127
- **THEN** those bytes are skipped during hash computation, matching BSArchPro behavior.

### Requirement: BA2 hash-based path lookup
libbsa SHALL provide file existence and metadata lookup by archive-relative path using the CRC32 hash triplet (directory hash, file-name hash, extension literal).

#### Scenario: Existing file is found by path
- **WHEN** a caller looks up an existing BA2 path with different ASCII case or slash style
- **THEN** libbsa normalizes the path, splits into directory/name/extension, computes the hash triplet, and returns the matching record metadata.

#### Scenario: Missing file is requested
- **WHEN** a caller looks up a path not present in the archive index
- **THEN** libbsa returns a `missing_file` error.

### Requirement: BA2 GNRL extraction with transparent decompression
libbsa SHALL extract BA2 GNRL file entries, using deflate or LZ4 block decompression when `PackedSize != 0`.

#### Scenario: Uncompressed BA2 entry is extracted
- **WHEN** a caller extracts a BA2 GNRL entry whose `PackedSize` is zero
- **THEN** libbsa reads `Size` bytes from `Offset` and returns them directly.

#### Scenario: Deflate-compressed FO4 entry is extracted
- **WHEN** a caller extracts a compressed entry from an FO4 BA2 GNRL archive (version 1/7/8)
- **THEN** libbsa reads `PackedSize` bytes from `Offset`, decompresses with libdeflate targeting `Size` bytes, and returns the decompressed content.

#### Scenario: LZ4-block-compressed Starfield entry is extracted
- **WHEN** a caller extracts a compressed entry from a Starfield v3 BA2 GNRL archive with `CompressionMethod = 3`
- **THEN** libbsa reads `PackedSize` bytes from `Offset`, decompresses with `LZ4_decompress_safe()` targeting `Size` bytes, and returns the decompressed content.

#### Scenario: Decompression produces wrong size
- **WHEN** decompression produces a byte count not equal to the expected `Size`
- **THEN** libbsa returns a `decompression_failed` error.

### Requirement: Compatibility tests prove byte identity
Tests SHALL prove that BA2 GNRL extraction output is byte-identical to expected fixture payloads or BSArchPro-produced output.

#### Scenario: FO4 fixture archive is extracted
- **WHEN** the test suite extracts all files from a Fallout 4 GNRL BA2 fixture
- **THEN** each extracted byte sequence matches the expected bytes for that fixture.

#### Scenario: Starfield fixture archive is extracted
- **WHEN** the test suite extracts all files from a Starfield GNRL BA2 fixture
- **THEN** each extracted byte sequence matches the expected bytes for that fixture.
