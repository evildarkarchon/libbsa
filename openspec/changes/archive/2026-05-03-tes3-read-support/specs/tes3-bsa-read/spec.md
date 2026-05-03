## ADDED Requirements

### Requirement: TES3 archive detection
libbsa SHALL detect Morrowind TES3 BSA archives by their 4-byte magic value `0x00000100` (little-endian uint32).

#### Scenario: TES3 archive is opened
- **WHEN** a caller opens a file whose first 4 bytes are `0x00`, `0x01`, `0x00`, `0x00`
- **THEN** libbsa reports the archive format as `tes3` and makes the read API available.

#### Scenario: TES3 magic is distinguished from TES4+
- **WHEN** a caller opens a file whose first 4 bytes are `BSA\0` (`0x42`, `0x53`, `0x41`, `0x00`)
- **THEN** libbsa does NOT treat it as TES3 and instead routes to TES4-family detection.

### Requirement: TES3 index parsing
libbsa SHALL parse the TES3 on-disk layout: 12-byte header (magic + HashOffset + FileCount), size/offset table, name offset table, filename records, and hash table, producing a queryable file index.

#### Scenario: TES3 file index is listed
- **WHEN** a valid TES3 archive is opened
- **THEN** the caller can list every stored file path with its raw size, data offset, and 64-bit hash.

#### Scenario: TES3 file count matches header
- **WHEN** a valid TES3 archive is opened
- **THEN** the number of entries in the parsed index equals the `FileCount` field from the header.

#### Scenario: TES3 filenames are read from name table
- **WHEN** a valid TES3 archive is opened
- **THEN** each entry's path is a non-empty null-terminated string read from the filename records section.

### Requirement: TES3-compatible hashing
libbsa SHALL implement a TES3 hash function compatible with BSArchPro `CreateHashTES3`, producing a 64-bit hash by splitting the filename at `floor(len/2)`, XOR-accumulating the first half into the high 32 bits, and XOR-accumulating with rotation the second half into the low 32 bits.

#### Scenario: Hash matches BSArchPro output for known filenames
- **WHEN** libbsa hashes filenames from a known Morrowind BSA fixture
- **THEN** each computed 64-bit hash matches the hash stored in that fixture's hash table.

#### Scenario: Hash lowercasing is ASCII-only
- **WHEN** libbsa hashes a filename containing uppercase ASCII letters A-Z
- **THEN** only those letters are lowercased; all other byte values pass through unchanged.

#### Scenario: Hash table uses swapped dword order
- **WHEN** libbsa reads the hash table from a TES3 archive
- **THEN** each 8-byte hash entry is read as high-dword (bytes 0-3) followed by low-dword (bytes 4-7), combined as `(uint64(high) << 32) | low`.

### Requirement: TES3 path-based file lookup
libbsa SHALL provide file existence and metadata lookup by archive-relative path using the TES3 64-bit hash for matching.

#### Scenario: Existing file is found by path
- **WHEN** a caller looks up a path that exists in the TES3 archive using different ASCII case or either slash style
- **THEN** libbsa normalizes the path, computes the TES3 hash, finds the matching entry, and returns its metadata.

#### Scenario: Missing file returns error
- **WHEN** a caller looks up a path whose TES3 hash does not match any entry
- **THEN** libbsa returns a missing-file error.

### Requirement: TES3 file extraction
libbsa SHALL extract TES3 file entries by seeking to `data_section_offset + entry_offset` and reading `entry_size` raw bytes. TES3 archives have no compression.

#### Scenario: File is extracted from TES3 archive
- **WHEN** a caller extracts a file from a TES3 archive
- **THEN** libbsa returns exactly `entry_size` bytes starting at `data_section_offset + entry_offset` in the archive.

#### Scenario: Extracted bytes match original content
- **WHEN** libbsa extracts a file from a TES3 fixture archive
- **THEN** the extracted bytes are byte-identical to the known original file content.

### Requirement: TES3 data section offset calculation
libbsa SHALL compute the data section offset as the byte position immediately after the hash table: `12 + (8 * file_count) + (4 * file_count) + total_name_bytes + (8 * file_count)`.

#### Scenario: Data offset is computed correctly
- **WHEN** a TES3 archive is parsed
- **THEN** the computed data section offset equals the position immediately following the last hash table entry, and using it with entry offsets produces correct extraction results.

### Requirement: TES3 compatibility tests prove byte identity
Tests SHALL prove that TES3 extraction output is byte-identical to expected fixture payloads.

#### Scenario: Fixture archive is fully extracted
- **WHEN** the test suite extracts all files from a TES3 fixture archive
- **THEN** each extracted byte sequence matches the expected bytes for that fixture.
