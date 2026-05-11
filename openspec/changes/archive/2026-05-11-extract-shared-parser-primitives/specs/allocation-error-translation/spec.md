## ADDED Requirements

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
