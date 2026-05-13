## Why

Compressed BSA entries and BA2 payloads currently materialize both the stored compressed bytes and the full decoded bytes before writing to the caller's sink. This makes extraction memory scale with archive-declared payload sizes even when the public sink API is already capable of chunked output.

## What Changes

- Add bounded compressed extraction behavior for TES4-family BSA, BA2 GNRL, and BA2 DX10 payload readers.
- Preserve exact decoded-size validation for every compressed entry or texture chunk.
- Stream decoded codec output to caller sinks where the selected codec API supports incremental output.
- Keep whole-buffer codec paths explicit where streaming is not practical, and reject unsupported oversized convenience extractions before attempting unbounded allocation.
- Add tests that prove compressed sink extraction no longer requires a full decoded output buffer on supported streaming codec paths and that oversized `extract_bytes` requests fail predictably.

## Capabilities

### New Capabilities
- `compressed-extraction-memory-bounds`: Defines memory-bounded compressed extraction and convenience byte extraction limits for BSA and BA2 readers.

### Modified Capabilities

## Impact

- Affected public API behavior: `archive_reader::extract_bytes` fails with a typed result error when a parser-declared output size cannot be safely materialized in one vector.
- Affected extraction readers: `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, and `src/formats/ba2/ba2_dx10_reader.cpp`.
- Affected compression adapters: `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`, and `src/detail/compression_router.*`.
- Affected shared extraction helpers: `src/detail/payload_stream.*`.
- No new external dependencies are required.
