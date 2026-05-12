## ADDED Requirements

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
