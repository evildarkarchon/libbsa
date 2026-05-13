## 1. Sink-Oriented Codec Routing

- [x] 1.1 Add internal sink-oriented decompression declarations with Doxygen comments for exact-size-to-sink extraction.
- [x] 1.2 Implement an LZ4 frame exact-to-sink adapter that reads compressed input in bounded chunks, writes bounded decoded chunks, guards decoder progress, and validates exact decoded size.
- [x] 1.3 Add compression router support that sends LZ4 frame payloads through the streaming adapter and keeps deflate/raw LZ4 block whole-buffer fallbacks explicit.
- [x] 1.4 Ensure whole-buffer fallback paths validate archive-declared decoded size before codec allocation and write fallback decoded output through existing exact sink helpers.

## 2. Reader Integration

- [x] 2.1 Replace TES4-family BSA compressed extraction with the sink-oriented router after preserving embedded-name and compressed-size-prefix validation.
- [x] 2.2 Replace BA2 GNRL compressed extraction with the sink-oriented router while preserving BA2 compression method validation and embedded-name rejection.
- [x] 2.3 Replace BA2 DX10 compressed chunk extraction with the sink-oriented router while preserving DDS header emission and parser-validated chunk order.
- [x] 2.4 Add `archive_reader::extract_bytes` materialization preflight so unsupported vector-sized outputs fail before payload read or decompression.

## 3. Tests

- [x] 3.1 Add codec/router tests proving LZ4 frame exact-to-sink writes bounded chunks and returns `format_error` for truncated, overproducing, or otherwise malformed frame data.
- [x] 3.2 Add extraction tests proving TES4 LZ4 frame payload extraction writes through bounded sink chunks and still matches `extract_bytes` for normal entries.
- [x] 3.3 Add tests proving deflate and raw LZ4 block fallback paths preserve exact output bytes and error categories for existing TES4 BSA, BA2 GNRL, and BA2 DX10 fixtures.
- [x] 3.4 Add `extract_bytes` tests proving oversized declared output fails before compressed payload decoding and valid entries keep existing invalid-path and not-found behavior.

## 4. Verification

- [x] 4.1 Run the focused reader and codec unit tests for TES4 BSA, BA2 GNRL, BA2 DX10, and compression adapters.
- [x] 4.2 Run the broader CTest suite to verify existing reader, writer, round-trip, and fixture behavior remains stable.
- [x] 4.3 Review changed public/internal comments and doc comments for the new helpers, especially the exact-size and partial-output contracts.
