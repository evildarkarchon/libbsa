## Purpose

Specify shared internal payload extraction helpers for archive-derived size validation, exact archive range reads, bounded streaming, and sink write validation.

## Requirements

### Requirement: Shared extraction helpers validate archive-derived sizes and stream spans
libbsa internal extraction readers SHALL use shared payload extraction helpers for repeated archive-derived `std::size_t` conversion and platform stream-limit validation. The helpers MUST reject offsets, byte counts, and offset-plus-size spans that cannot be represented safely by the host stream types used for archive extraction.

#### Scenario: Archive payload size exceeds platform size limits
- **WHEN** an extraction reader asks the shared helper to convert an archive-declared payload size that exceeds `std::size_t`
- **THEN** the helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** the reader does not allocate a wrapped or truncated buffer

#### Scenario: Archive payload span exceeds stream limits
- **WHEN** an extraction reader asks the shared helper to read or stream a payload range whose offset, size, or end position exceeds the platform stream limits
- **THEN** the helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** the reader does not seek to a wrapped or partial archive position

### Requirement: Shared extraction helpers write sink buffers exactly
libbsa internal extraction readers SHALL use shared payload extraction helpers when writing decoded buffers, reconstructed headers, and streamed chunks to a payload sink. The helpers MUST treat any sink write that accepts fewer bytes than requested as a hard extraction failure.

#### Scenario: Sink accepts only part of a payload chunk
- **WHEN** an extraction reader writes a payload buffer through the shared helper and the sink reports fewer bytes accepted than requested
- **THEN** the helper returns a `result` failure with `libbsa::error_code::io_error`
- **THEN** extraction does not report success for partially emitted output

#### Scenario: Decoded payload is larger than one extraction chunk
- **WHEN** an extraction reader writes a decoded payload span larger than the configured extraction chunk size
- **THEN** the shared helper writes the span in bounded chunks
- **THEN** every chunk must be accepted fully before extraction can succeed

### Requirement: Shared extraction helpers read archive ranges exactly
libbsa internal extraction readers SHALL use shared payload extraction helpers for exact host-file reads at archive-controlled offsets and for streaming raw archive ranges. The helpers MUST clear stream state before seeking, seek to the requested absolute offset, read exactly the requested byte count, and distinguish I/O failures from truncated archive spans.

#### Scenario: Archive range is shorter than metadata declares
- **WHEN** an extraction reader asks the shared helper to read or stream a payload range that extends past the available archive bytes
- **THEN** the helper returns a `result` failure with `libbsa::error_code::format_error`
- **THEN** the diagnostic preserves the reader-supplied archive-family description

#### Scenario: Raw payload range is streamed to the sink
- **WHEN** an extraction reader streams an uncompressed TES4 BSA, BA2 GNRL, or BA2 DX10 chunk payload through the shared helper
- **THEN** the helper copies the range using bounded scratch chunks
- **THEN** the helper does not require a whole-payload allocation before writing to the sink

### Requirement: Format readers preserve archive-specific extraction behavior
TES4 BSA, BA2 GNRL, and BA2 DX10 extraction readers SHALL delegate only common payload I/O mechanics to shared helpers. Format-specific compression routing, TES4 embedded-name prefix handling, TES4 compressed-size prefix validation, BA2 GNRL embedded-name rejection, BA2 DX10 DDS header reconstruction, and BA2 DX10 texture chunk ordering MUST remain owned by the format readers.

#### Scenario: TES4 compressed payload is extracted through shared output helpers
- **WHEN** a TES4-family BSA entry stores a compressed payload
- **THEN** the TES4 reader validates the embedded-name offset and compressed-size prefix according to TES4 BSA metadata
- **THEN** the reader writes the decoded bytes through the shared sink helper

#### Scenario: BA2 DX10 payload reconstructs DDS output before chunk extraction
- **WHEN** a BA2 DX10 texture entry is extracted
- **THEN** the BA2 DX10 reader writes the reconstructed DDS header before payload chunks
- **THEN** raw and decompressed texture chunks are emitted through shared sink or stream helpers without changing parser-validated chunk order
