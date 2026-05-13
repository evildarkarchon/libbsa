## ADDED Requirements

### Requirement: Compressed sink extraction bounds decoded output materialization
libbsa extraction readers SHALL avoid materializing a complete decoded payload before writing to a caller-provided `payload_sink` when the selected codec adapter supports bounded streaming output. When a codec path still requires whole-buffer decoded output, the reader MUST make that materialization boundary explicit and MUST validate the parser-declared decoded size before allocation.

#### Scenario: Streaming codec writes decoded chunks directly to the sink
- **WHEN** a TES4-family BSA compressed payload uses a codec method with a streaming sink adapter
- **THEN** extraction writes decoded bytes to the caller's `payload_sink` in bounded chunks
- **THEN** extraction does not allocate one decoded vector sized to the full entry raw size before writing to the sink

#### Scenario: Whole-buffer codec path remains explicit and prevalidated
- **WHEN** a compressed TES4-family BSA, BA2 GNRL, or BA2 DX10 payload uses a codec method that requires whole-buffer decoded output
- **THEN** extraction validates that the archive-declared decoded size is representable before invoking the codec
- **THEN** extraction returns a typed failure instead of attempting an unchecked decoded-output allocation for an unsupported size

### Requirement: Compressed extraction preserves exact decoded-size validation
Compressed extraction SHALL verify that the total decoded byte count exactly matches the parser-declared raw size for each compressed entry or texture chunk. Extraction MUST report malformed compressed data or decoded-size mismatches with `libbsa::error_code::format_error` and MUST NOT report success for partially decoded output.

#### Scenario: Streaming decoder produces too few bytes
- **WHEN** a compressed payload reaches the end of the compressed stream before producing the parser-declared raw size
- **THEN** extraction fails with `libbsa::error_code::format_error`
- **THEN** extraction does not report success for the entry

#### Scenario: Streaming decoder produces too many bytes
- **WHEN** a compressed payload attempts to produce more bytes than the parser-declared raw size
- **THEN** extraction fails with `libbsa::error_code::format_error`
- **THEN** extraction does not write bytes beyond the declared output size to the caller's sink

#### Scenario: Whole-buffer decoder output size mismatches metadata
- **WHEN** a whole-buffer decompressor returns a byte count that differs from the parser-declared raw size
- **THEN** extraction fails with `libbsa::error_code::format_error`
- **THEN** the reader does not treat the payload as successfully extracted

### Requirement: Convenience byte extraction enforces materialization limits before decoding
`archive_reader::extract_bytes` SHALL reject entries whose parser-declared extracted size cannot be safely materialized as one `std::vector<std::byte>` before it reads or decompresses the entry payload. The failure MUST use a typed libbsa result error and MUST preserve existing invalid-path and not-found behavior.

#### Scenario: Compressed entry is too large for the byte-vector convenience API
- **WHEN** a caller requests `archive_reader::extract_bytes` for a compressed entry whose declared extracted size exceeds the supported byte-vector materialization limit
- **THEN** `extract_bytes` returns a failed result before reading the compressed payload
- **THEN** no codec decompression work is attempted for that request

#### Scenario: Entry fits the byte-vector convenience API
- **WHEN** a caller requests `archive_reader::extract_bytes` for an entry whose declared extracted size fits the supported byte-vector materialization limit
- **THEN** extraction returns the same bytes as `archive_reader::extract` writes to a collecting sink
- **THEN** existing archive path validation and missing-path errors remain unchanged

### Requirement: BA2 DX10 compressed chunks remain chunk-scoped
BA2 DX10 extraction SHALL apply compressed extraction memory bounds independently to each texture chunk after the reconstructed DDS header is emitted. The reader MUST NOT concatenate multiple compressed texture chunks into one decoded texture buffer solely for extraction.

#### Scenario: Texture has multiple compressed chunks
- **WHEN** a BA2 DX10 texture entry contains multiple compressed chunks
- **THEN** extraction processes each compressed chunk through the bounded compressed extraction path independently
- **THEN** extraction preserves the parser-validated texture chunk order and DDS header placement

#### Scenario: One compressed texture chunk is malformed
- **WHEN** one BA2 DX10 compressed texture chunk fails decompression or exact-size validation
- **THEN** extraction fails with `libbsa::error_code::format_error`
- **THEN** later chunks for that texture entry are not extracted after the failure
