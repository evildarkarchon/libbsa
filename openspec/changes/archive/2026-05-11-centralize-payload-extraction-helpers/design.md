## Context

TES4 BSA, BA2 GNRL, and BA2 DX10 extraction readers each define local helpers for platform-size checks, full sink writes, chunked writes, stream-limit validation, exact host-file reads, and raw payload streaming. `src/detail/payload_stream.*` already contains the internal `payload_source`/`payload_sink` abstractions and `transfer_payload`, but readers still duplicate the extraction-specific mechanics around archive offsets and decoded buffers.

This change keeps the public `libbsa::payload_sink` API stable and moves only internal helper ownership. Format readers remain responsible for archive-family decisions: compression method selection, TES4 embedded-name prefix handling, BA2 raw-vs-compressed routing, BA2 DX10 DDS header reconstruction, texture chunk order, and existing metadata validation.

## Goals / Non-Goals

**Goals:**

- Centralize common extraction I/O helpers in `src/detail/payload_stream.*`.
- Make TES4 BSA, BA2 GNRL, and BA2 DX10 readers use the same helper behavior for partial sink writes, bounded chunking, checked `std::size_t` conversion, platform stream limits, exact reads, and raw-range streaming.
- Preserve archive-family error context by passing caller descriptions into shared helpers.
- Add helper-level regression coverage and keep format-reader extraction tests passing for raw, deflate, LZ4 block, and LZ4 frame payloads.

**Non-Goals:**

- Do not expose new public APIs or alter `libbsa::payload_sink` semantics.
- Do not change compression codec behavior, DDS header reconstruction, texture chunk ordering, archive path normalization, writer behavior, dependencies, CMake presets, or vcpkg configuration.
- Do not introduce cross-platform work beyond the current Windows/MSVC target.
- Do not edit, format, stage, compile, or otherwise mutate `TES5Edit/`.

## Decisions

### Extend `detail::payload_stream` instead of adding a new extraction module

Add internal helpers to `src/detail/payload_stream.hpp` and `.cpp`, such as checked size conversion, exact sink writes, chunked span writes, exact file reads at archive offsets, and raw range streaming. The module already owns bounded payload movement and partial-sink failure behavior, so extending it keeps all extraction payload mechanics in one place.

Alternatives considered:

- Add `detail::extraction_io`: rejected because it would split closely related payload movement behavior across two internal modules.
- Keep helper copies in each reader: rejected because the reported issue is duplicated logic drifting across format readers.

### Keep format routing in reader modules

Readers should call shared helpers only after format-specific decisions are made. TES4 BSA still computes consumer payload offsets after embedded-name prefixes and reads its compressed-size prefix. BA2 GNRL still rejects embedded-name metadata and routes raw entries away from decompression. BA2 DX10 still writes the reconstructed DDS header before iterating parser-ordered chunks.

Alternatives considered:

- Create one generic extractor for all formats: rejected because the formats differ in compression metadata, embedded payload prefixes, and DDS chunk reconstruction rules.

### Use description-driven diagnostics

Shared helpers should accept a concise `std::string_view description` from callers and append stable suffixes for platform limit, seek, read, truncation, and partial-sink failures. This preserves useful diagnostics like `TES4 BSA compressed payload` or `BA2 DX10 chunk payload` without keeping duplicate control flow.

Alternatives considered:

- Standardize one generic message for every helper failure: rejected because it would make fixture and integration failures harder to diagnose.

### Treat partial sink writes as hard extraction failures

The existing behavior returns `io_error` when a sink accepts fewer bytes than requested. The shared write helpers must keep this strict behavior for both decoded buffers and streamed archive ranges, so extraction never reports success after silently truncating output.

Alternatives considered:

- Retry partial sink writes until the buffer is consumed: rejected because the public sink contract returns accepted bytes, and existing code treats partial acceptance as an error rather than a backpressure protocol.

### Test helper contracts directly and retain reader coverage

Expand `tests/unit/payload_stream_tests.cpp` to cover checked size conversion, chunked span writing, exact file reads, stream-limit rejection, truncated archive ranges, and raw range streaming. Existing TES4 BSA, BA2 GNRL, and BA2 DX10 extraction tests continue proving format routing and decoded output behavior.

Alternatives considered:

- Only rely on existing reader tests: rejected because the central helper contracts should be verified without needing every archive-family fixture to trigger each edge case.

## Risks / Trade-offs

- Error text can churn during refactor. Mitigation: use caller-provided descriptions and keep existing error codes and message meaning stable.
- Over-centralization could hide format compatibility behavior. Mitigation: move only primitive payload I/O mechanics; leave compression and archive layout decisions in format readers.
- Stream helper changes can affect seek/read state. Mitigation: helpers must clear stream state before seeking, seek absolute offsets, read exactly the requested byte count, and distinguish I/O failures from truncated archive spans.
- Tests may miss one reader path if only helper tests are updated. Mitigation: run targeted `payload-stream`, TES4 reader, BA2 GNRL reader, and BA2 DX10 extraction tests after the refactor.

## Migration Plan

1. Extend `detail::payload_stream` with shared checked-size, sink-write, chunked-write, exact-read, and stream-range helpers.
2. Add or expand helper-level tests in `tests/unit/payload_stream_tests.cpp`.
3. Refactor `tes4_bsa_reader.cpp` onto the shared helpers while keeping TES4 compressed-size prefix and embedded-name behavior local.
4. Refactor `ba2_gnrl_reader.cpp` onto the shared helpers while keeping BA2 GNRL compression routing local.
5. Refactor `ba2_dx10_reader.cpp` onto the shared helpers while keeping DDS header reconstruction and texture chunk iteration local.
6. Run focused extraction tests and the normal CTest preset used for the repository.

Rollback is internal: restore the reader-local helpers and remove newly added `payload_stream` helper calls if validation exposes a behavior change.

## Open Questions

- None currently. Exact helper names can be chosen during implementation as long as the shared ownership boundary and result/error contracts are preserved.
