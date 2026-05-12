## Purpose

Specify how libbsa translates archive-controlled allocation failures into typed result errors instead of leaking allocation exceptions.

## Requirements

### Requirement: Parser byte-buffer allocation failures use result errors
Archive parsers SHALL translate byte-buffer allocation failures caused by archive-declared spans into `result<T>` failures with `libbsa::error_code::format_error`. Parser open and listing paths MUST NOT let `std::bad_alloc` or `std::length_error` escape when the failing allocation is derived from archive metadata.

#### Scenario: TES3 BSA declared byte span cannot be allocated
- **WHEN** a TES3 BSA archive declares a byte span that is structurally within the archive but too large for libbsa to allocate
- **THEN** opening the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API

#### Scenario: TES4 BSA declared byte span cannot be allocated
- **WHEN** a TES4 BSA archive declares a byte span that is structurally within the archive but too large for libbsa to allocate
- **THEN** opening the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API

#### Scenario: BA2 GNRL declared byte span cannot be allocated
- **WHEN** a BA2 GNRL archive declares a byte span that is structurally within the archive but too large for libbsa to allocate
- **THEN** opening the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API

#### Scenario: BA2 DX10 declared byte span cannot be allocated
- **WHEN** a BA2 DX10 archive declares a byte span that is structurally within the archive but too large for libbsa to allocate
- **THEN** opening the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API

### Requirement: Shared parser file-read allocations use result errors
Parser file-read helpers that allocate byte buffers for archive-controlled spans SHALL translate allocation failures into `result<T>` failures with `libbsa::error_code::format_error`. TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 parsers MUST use this shared result-based path for bounded metadata and filename-table reads.

#### Scenario: Shared parser read buffer cannot be allocated
- **WHEN** any archive-family parser requests a bounded host-file read for an archive-declared byte span that cannot be allocated
- **THEN** the shared parser helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** the parser open path returns that failure without leaking `std::bad_alloc` or `std::length_error`

### Requirement: Shared parser string allocations use result errors
Parser string materialization helpers that allocate archive path or name strings from archive-controlled byte spans SHALL translate allocation failures into `result<T>` failures with `libbsa::error_code::format_error`. TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 parsers MUST use this shared result-based path when materializing archive metadata strings.

#### Scenario: Shared parser archive string cannot be allocated
- **WHEN** any archive-family parser materializes an archive path or filename from an archive-declared byte span that cannot be allocated as a string
- **THEN** the shared parser helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** parser open, list, find, and metadata paths do not leak `std::bad_alloc` or `std::length_error`

### Requirement: Parser byte-buffer reserve and append failures use result errors
Parser byte-buffer reserve and append operations whose sizes come from archive metadata SHALL return `libbsa::error_code::format_error` through `result<T>` when allocation fails. These paths MUST preserve the existing archive-format validation rules and MUST NOT replace specific malformed-data errors with generic exceptions.

#### Scenario: BA2 DX10 encoded name table growth cannot be allocated
- **WHEN** a BA2 DX10 filename table requires reserving or appending encoded name bytes beyond libbsa's byte-vector allocation capacity
- **THEN** opening the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API

### Requirement: Codec output allocation failures use result errors
Compression and decompression codec helpers SHALL translate output byte-buffer allocation failures into `result<T>` failures. Allocation failures for deflate, LZ4 frame, and raw LZ4 block output buffers MUST use `libbsa::error_code::format_error` rather than escaping allocation exceptions.

#### Scenario: Deflate output allocation cannot be satisfied
- **WHEN** deflate compression or exact-size decompression requires an output buffer that cannot be allocated
- **THEN** the codec helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the helper

#### Scenario: LZ4 frame output allocation cannot be satisfied
- **WHEN** LZ4 frame compression or exact-size decompression requires an output buffer that cannot be allocated
- **THEN** the codec helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the helper

#### Scenario: Raw LZ4 block output allocation cannot be satisfied
- **WHEN** raw LZ4 block compression or exact-size decompression requires an output buffer that cannot be allocated
- **THEN** the codec helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the helper

### Requirement: Parser metadata-container allocation failures use result errors
Archive parsers SHALL translate allocation failures from archive-controlled metadata containers into `result<T>` failures with `libbsa::error_code::format_error`. Parser open, list, find, metadata, and validation paths MUST NOT let `std::bad_alloc` or `std::length_error` escape when the failing allocation is for typed records, names, entries, duplicate-detection sets, payload span tracking, or texture chunk metadata derived from archive metadata.

#### Scenario: Typed metadata vector allocation cannot be satisfied
- **WHEN** a TES3 BSA, TES4-family BSA, BA2 GNRL, or BA2 DX10 parser cannot allocate a typed metadata vector for archive-declared records, names, entries, payload spans, or texture chunks
- **THEN** opening or validating the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API

#### Scenario: Duplicate-detection set allocation cannot be satisfied
- **WHEN** a TES3 BSA, TES4-family BSA, BA2 GNRL, or BA2 DX10 parser cannot allocate duplicate-detection set storage for archive-derived paths or stored hashes
- **THEN** opening or validating the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API

#### Scenario: Texture chunk metadata allocation cannot be satisfied
- **WHEN** a BA2 DX10 parser cannot allocate internal chunk records, ordered chunk metadata, or public texture chunk metadata derived from archive-declared chunk counts
- **THEN** opening or validating the archive returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** no `std::bad_alloc` or `std::length_error` escapes the public API
