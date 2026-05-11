## Purpose

Specify shared internal parser primitives for archive-derived arithmetic, bounded reads, allocation-safe string materialization, and display separator normalization.

## Requirements

### Requirement: Shared parser arithmetic validates archive-derived sizes
libbsa parser modules SHALL use shared internal parser primitives for repeated checked size multiplication, checked size addition, and archive span containment when those calculations are driven by archive metadata.

#### Scenario: Parser table size arithmetic overflows
- **WHEN** a parser computes a metadata table byte size from an archive-declared count and element width that cannot fit in the target size type
- **THEN** the shared parser primitive reports the calculation as invalid
- **THEN** the parser returns a `result` failure with `libbsa::error_code::format_error`

#### Scenario: Parser span extends beyond archive bytes
- **WHEN** a parser checks an archive-declared offset and length against the known archive byte length
- **THEN** the shared parser primitive rejects spans whose end would overflow or exceed the archive byte length
- **THEN** the parser returns a `result` failure with `libbsa::error_code::format_error`

### Requirement: Shared parser file reads are bounded and exact
libbsa parser modules SHALL use a shared internal helper for host-file byte reads at archive-controlled offsets. The helper MUST reject stream offsets and byte counts that exceed platform stream limits, seek to the requested absolute offset, read exactly the requested count, and return typed I/O or format errors through `result<T>`.

#### Scenario: Parser read offset exceeds stream limits
- **WHEN** a parser asks the shared helper to read bytes from an archive offset that cannot be represented by the platform stream offset type
- **THEN** the helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no parser attempts a partial or wrapped seek

#### Scenario: Parser read is truncated
- **WHEN** a parser asks the shared helper to read a byte range that is shorter on disk than archive metadata declared
- **THEN** the helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** the parser preserves archive-family context in the diagnostic description

### Requirement: Shared parser strings are allocation-safe
libbsa parser modules SHALL use a shared internal helper to materialize archive byte spans into `std::string` values when the byte count originates from archive metadata. The helper MUST translate `std::bad_alloc` and `std::length_error` into `result<T>` failures with `libbsa::error_code::format_error`.

#### Scenario: Archive string cannot be allocated
- **WHEN** a parser materializes an archive name byte span whose size cannot be allocated as a string
- **THEN** the shared parser primitive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes parser open, list, find, or extract metadata paths

### Requirement: Shared display separator normalization preserves format ownership
libbsa parser modules SHALL use a shared internal helper for normalizing archive display paths to `/` separators after a format module has decided that a stored path should be exposed as an archive display path. The helper MUST NOT replace format-specific parsing, hashing, or on-disk separator compatibility rules.

#### Scenario: Backslash display separators are normalized
- **WHEN** a parser has accepted an archive entry path that is exposed through public metadata
- **THEN** the shared helper converts `\` display separators to `/`
- **THEN** canonical lookup normalization and format-specific hash behavior remain owned by the existing archive path and format modules
