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
