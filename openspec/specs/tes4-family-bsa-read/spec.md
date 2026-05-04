# tes4-family-bsa-read Specification

## Purpose
TBD - created by archiving change m1-foundation-tes4-read-support. Update Purpose after archive.
## Requirements
### Requirement: TES4-family BSA detection
libbsa SHALL detect TES4-family BSA archives by `BSA\0` magic bytes and supported versions `0x67`, `0x68`, and `0x69`. The detection logic SHALL first check for TES3 magic (`0x00000100`), then check for BTDX magic (`BTDX`) and route to the BA2 parser, before falling through to TES4-family version checks.

#### Scenario: Supported BSA version is opened
- **WHEN** a caller opens a `BSA\0` archive with version `0x67`, `0x68`, or `0x69`
- **THEN** libbsa reports the archive as TES4, FO3-family, or SSE-family respectively and makes the read API available.

#### Scenario: Unsupported BSA version is opened
- **WHEN** a caller opens a `BSA\0` archive with any other version
- **THEN** libbsa returns an unsupported-format error.

#### Scenario: TES3 magic is not misidentified as TES4
- **WHEN** a caller opens a file whose first 4 bytes are `0x00000100` (TES3 magic)
- **THEN** libbsa does NOT enter the TES4 detection path and instead routes to TES3 parsing.

#### Scenario: BTDX magic is not misidentified as TES4
- **WHEN** a caller opens a file whose first 4 bytes are `BTDX` (BA2 magic)
- **THEN** libbsa does NOT enter the TES4 detection path and instead routes to the BA2 parser.

### Requirement: TES4-family index parsing
libbsa SHALL parse TES4-family archive headers, archive flags, file flags, folder records, folder names, file records, file names, and version-specific folder offsets into queryable metadata.

#### Scenario: Archive index is listed
- **WHEN** a supported TES4-family archive is opened successfully
- **THEN** the caller can list every stored file path with its raw size, packed size or stored span, folder hash, file hash, compression state, and data offset.

#### Scenario: SSE folder records are parsed
- **WHEN** an SSE-family version `0x69` archive is opened
- **THEN** libbsa reads the SSE folder-record layout with the extra unknown 32-bit field and 64-bit folder offset.

### Requirement: TES4-compatible hashing
libbsa SHALL implement a TES4-family hash function compatible with BSArchPro `CreateHashTES4` for folder names and file name/extension pairs.

#### Scenario: Folder and file hashes are computed
- **WHEN** libbsa hashes a folder path and file name from a known TES4-family fixture
- **THEN** the computed hashes match the hashes stored in that fixture and the corresponding BSArchPro hash output.

#### Scenario: Special extension bits are applied
- **WHEN** libbsa hashes file names with `.kf`, `.nif`, `.dds`, or `.wav` extensions
- **THEN** the computed hash applies the same extension-specific bit markers used by BSArchPro.

### Requirement: Hash-based path lookup
libbsa SHALL provide file existence and metadata lookup by archive-relative path using TES4-family folder and file hashes.

#### Scenario: Existing file is found by path
- **WHEN** a caller looks up an existing path using different ASCII case or either slash style
- **THEN** libbsa normalizes the lookup path, resolves the matching hash-based record, and returns its metadata.

#### Scenario: Missing file is requested
- **WHEN** a caller looks up or extracts a path that is not present in the archive index
- **THEN** libbsa returns a missing-file error.

### Requirement: TES4-family extraction
libbsa SHALL extract raw and compressed TES4-family file entries to byte-identical output.

#### Scenario: Uncompressed entry is extracted
- **WHEN** a caller extracts an uncompressed file from a supported TES4-family archive
- **THEN** libbsa returns exactly the stored payload bytes for that file.

#### Scenario: Deflate-compressed entry is extracted
- **WHEN** a caller extracts a compressed TES4 or FO3-family entry
- **THEN** libbsa reads the uncompressed-size prefix, decompresses the payload with libdeflate, and returns bytes matching the original file.

#### Scenario: LZ4-frame-compressed SSE entry is extracted
- **WHEN** a caller extracts a compressed SSE-family entry
- **THEN** libbsa reads the uncompressed-size prefix, decompresses the payload with LZ4 frame support, and returns bytes matching the original file.

### Requirement: Compression flags follow TES4-family inversion semantics
libbsa SHALL determine per-file compression by XORing archive flag `ARCHIVE_COMPRESS` (`0x0004`) with file size bit `FILE_SIZE_COMPRESS` (`0x40000000`).

#### Scenario: File compression bit overrides archive default
- **WHEN** an archive has a default compression flag and a file size value whose high compression bit differs from that default
- **THEN** libbsa treats the file as compressed only when the XOR rule indicates compression.

### Requirement: Embedded file names are skipped during extraction
libbsa SHALL handle `ARCHIVE_EMBEDNAME` (`0x0100`) by skipping the length-prefixed embedded file name before reading FO3-family and SSE-family file data.

#### Scenario: Embedded-name archive entry is extracted
- **WHEN** a FO3-family or SSE-family archive entry includes an embedded name prefix
- **THEN** libbsa skips the prefix and returns only the extracted file content.

### Requirement: Compatibility tests prove byte identity
Milestone 1 tests SHALL prove that supported TES4-family extraction output is byte-identical to expected fixture payloads or BSArchPro-produced output.

#### Scenario: Fixture archive is extracted
- **WHEN** the test suite extracts all files from a milestone 1 fixture archive
- **THEN** each extracted byte sequence matches the expected bytes for that fixture.
