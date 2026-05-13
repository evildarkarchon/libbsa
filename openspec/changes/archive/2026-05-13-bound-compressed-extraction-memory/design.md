## Context

Raw TES3/TES4/BA2 extraction paths already use `detail::stream_payload_range` to copy archive payload ranges to `payload_sink` in 64 KiB chunks. Compressed TES4-family BSA, BA2 GNRL, and BA2 DX10 paths instead read the full stored payload into a vector, call `detail::decompress_payload_exact`, then write the complete decoded vector to the sink in chunks.

That shape preserves exact decoded-size validation, but it means caller-provided sink extraction still allocates at least one complete decoded payload. `archive_reader::extract_bytes` also reserves its output vector before extraction, but oversized requests can still enter format-specific compressed extraction before the sink reports that the convenience API cannot materialize the output.

## Goals / Non-Goals

**Goals:**
- Add an internal sink-oriented decompression path that preserves exact-size validation while allowing bounded decoded output for codecs that support streaming.
- Use the streaming path for TES4-family BSA LZ4 frame payloads, because the official LZ4 frame API supports incremental decompression.
- Keep deflate and raw LZ4 block whole-buffer boundaries explicit, prevalidated, and tested because the current project adapters use whole-buffer library APIs.
- Reject `archive_reader::extract_bytes` requests that cannot be safely represented as one byte vector before reading or decompressing the entry payload.
- Preserve archive-specific behavior for TES4 embedded names and compressed-size prefixes, BA2 GNRL compression routing, and BA2 DX10 DDS header and chunk ordering.

**Non-Goals:**
- No new public extraction option or configurable memory-limit API is introduced in this change.
- No new compression dependency is introduced.
- Deflate and raw LZ4 block decompression are not reimplemented as custom streaming codecs.
- Writer compression paths are not changed.
- In-place archive mutation and multi-threaded performance work remain out of scope.

## Decisions

1. Add a sink-oriented internal decompression router.

   Introduce an internal helper such as `decompress_payload_exact_to_sink(method, source, expected_size, sink, chunk_size, description)` in `detail/compression_router.*`. The helper owns exact decoded byte counting and uses `detail::write_payload_exact` for sink writes so partial sink acceptance remains an `io_error`.

   Alternative considered: keep calling `decompress_payload_exact` in readers and add reader-local chunking. That would preserve behavior but would not remove the decoded whole-buffer allocation for any codec.

2. Stream LZ4 frame input and output for TES4-family BSA payloads.

   Add an LZ4 frame sink adapter around `LZ4F_decompress` that reads the compressed archive range in bounded input chunks and writes bounded decoded chunks to the caller sink. The adapter tracks total decoded bytes, rejects output beyond the expected size, requires the frame to finish, and requires the compressed range to be consumed exactly.

   Alternative considered: pass a full compressed span to a sink-oriented LZ4 frame function. That would remove only the decoded allocation, but compressed archive bytes could still scale with the stored payload size.

3. Keep deflate and raw LZ4 block fallback materialization explicit.

   `libdeflate` and the current raw LZ4 block adapter are whole-buffer APIs in this codebase. For those methods, the sink-oriented router validates the decoded size, reads the stored payload, calls the existing exact whole-buffer decoder, and writes the resulting buffer in bounded chunks. The helper names and tests should make this boundary visible rather than hiding it inside format readers.

   Alternative considered: replace libdeflate or add another streaming deflate implementation. That would violate the dependency constraints and is not required to improve the paths where streaming is already practical.

4. Preflight `extract_bytes` materialization before payload extraction.

   Add a result-returning helper for the convenience API that checks whether `entry_metadata::raw_size` can be represented as a single `std::vector<std::byte>` before invoking `extract_entry_payload`. Convert `vector_payload_sink` construction into a result-returning path or equivalent preflight so oversized entries fail before format-specific readers read or decompress archive data.

   Alternative considered: rely on the current sink reservation failure. That detects allocation failure, but compressed readers can perform expensive whole-buffer work before the sink's first write observes the error.

5. Keep format readers responsible for format-specific validation.

   TES4 BSA readers still validate embedded-name offsets and the stored compressed-size prefix before routing compressed payload data. BA2 readers still select compression method from parsed metadata and reject unsupported compression families. BA2 DX10 still writes the DDS header before payload chunks and processes texture chunks in parser-validated order.

   Alternative considered: move all compressed extraction validation into the compression router. That would blur archive-family rules with codec mechanics and make malformed-archive diagnostics less specific.

## Risks / Trade-offs

- LZ4 frame streaming can write partial output before a later frame error is detected -> Preserve the existing extraction contract by returning `format_error`; callers that need atomic filesystem output should continue using temp-file sinks outside the library.
- Deflate and raw LZ4 block paths still materialize decoded output -> Keep those paths explicit, prevalidated, and covered by tests; future codec work can replace the fallback behind the same sink-oriented router.
- Streaming decoder progress loops can hang on malformed input -> Treat zero-consume/zero-produce progress as `format_error` and add malformed-frame tests.
- `extract_bytes` preflight prevents impossible vector materialization but does not guarantee system memory availability -> Existing result-based allocation translation remains in place for ordinary allocation failures.

## Migration Plan

1. Add sink-oriented codec/router helpers and focused unit coverage.
2. Switch TES4 BSA, BA2 GNRL, and BA2 DX10 compressed extraction readers to the router while preserving their existing validation order.
3. Add `extract_bytes` preflight before `extract_entry_payload` and keep existing invalid-path and not-found behavior unchanged.
4. Run the relevant reader, writer round-trip, and fixture tests to confirm output bytes and error categories remain stable.

Rollback is straightforward because the change is internal: revert the router call-site replacement and restore the previous `decompress_payload_exact` plus `write_payload_chunks` sequence.

## Open Questions

- No blocking questions. A future change can decide whether callers need a public configurable materialization limit for `extract_bytes`; this change only enforces the existing single-vector boundary earlier and more predictably.
