## Context

TES4 BSA, BA2 GNRL, and BA2 DX10 writer preparation all have disk-source paths that materialize payloads into `std::vector<std::byte>` before compression, deduplication, hashing, or DirectXTex analysis. Some final serialization paths already stream raw disk payloads with fixed-size scratch buffers, but compressed entries and DDS add-time analysis still use duplicated byte-at-a-time `std::ifstream::get` loops.

The current codec and DirectXTex integration points are whole-buffer APIs. This change should improve read behavior and allocation handling around those boundaries without changing archive bytes, writer public APIs, compression method selection, or snapshot semantics.

## Goals / Non-Goals

**Goals:**

- Centralize writer disk-source reads in shared internal helpers with bounded allocation, exact-size reads, prefix reads, and chunk iteration.
- Replace byte-at-a-time vector growth in writer preparation and layout paths with pre-sized reads or fixed-size chunks.
- Preserve existing `result<T>` error handling and family-specific diagnostics for open, inspect, read, and source-mutation failures.
- Keep compression and DDS analysis call sites explicit about when a full source payload must be materialized.
- Add focused tests proving the new helpers are used safely and that writer output remains byte-stable.

**Non-Goals:**

- Streaming deflate, LZ4 frame, or raw LZ4 compression.
- Replacing DirectXTex whole-memory DDS analysis.
- Changing public writer APIs or adding caller-visible source abstractions.
- Changing payload deduplication semantics, archive ordering, or emitted metadata.

## Decisions

1. Add writer-specific source I/O helpers under `src/detail/`.

   The helper should live outside parser primitives because writer reads have different semantics: host files are caller-provided sources, truncation is usually an I/O/source-mutation problem, and some reads intentionally allow short prefixes. Keep the helper internal and link it into writer targets only through existing project build wiring.

   Alternatives considered: extend `parser_primitives` or keep per-format helpers. Extending parser primitives would blur archive-metadata failure semantics with writer host-input semantics; keeping per-format helpers leaves the duplication and allocation behavior unchanged.

2. Use a small error-context object rather than hard-coded messages.

   Shared helpers need to preserve messages such as `TES4 BSA writer failed to open disk source`, `BA2 GNRL disk source changed during finalization`, and `BA2 DX10 writer failed while reading DDS source`. A `writer_source_context` with open, inspect, read, truncated/changed, and allocation descriptions keeps shared logic while allowing format-owned wording at call sites.

   Alternatives considered: generic shared messages or callback-based error creation. Generic messages would regress diagnostics; callbacks are more flexible than needed for this change.

3. Provide three read shapes.

   `read_disk_source_exact` should allocate with `detail::make_byte_vector`, read exactly an expected byte count, and fail if the stream produces fewer bytes or an appended byte is detected when the caller requests stability validation. `read_disk_source_prefix` should allocate at most the requested probe length and allow shorter files. `for_each_disk_source_chunk` should read fixed-size chunks into a scratch buffer and invoke a callback for hashing, final serialization, or disk-backed equality checks.

   Alternatives considered: a single overloaded read-all function. Separate shapes make whole-buffer boundaries visible and reduce the chance that a future writer path accidentally materializes a large file when chunked iteration would suffice.

4. Keep whole-buffer codec boundaries explicit.

   TES4 compressed disk entries and BA2 compressed disk entries still need a full raw payload before calling `detail::compress_payload`. BA2 DX10 DDS add-time analysis still needs a full DDS byte vector before DirectXTex analysis. The improvement is to size and allocate those buffers once, read them in one exact transfer loop, and translate allocation failures through libbsa errors.

   Alternatives considered: introduce streaming compression adapters now. That would be larger than this fix and would change codec ownership decisions beyond the current packing scalability hotspot.

5. Convert compression-router copy paths to allocation-safe helpers.

   `compression_method::none` currently copies spans directly into a vector. Updating that path to use existing byte-vector allocation wrappers keeps the no-compression route consistent with writer-side allocation translation and prevents allocation exceptions from escaping compression-adjacent preparation paths.

## Risks / Trade-offs

- Disk source changes between size inspection and read can still happen -> keep exact-size validation and existing mutation diagnostics where metadata was already assigned.
- Some paths still materialize full payloads -> document the codec/DirectXTex whole-buffer boundary and keep the future streaming compression work separate.
- Shared helper message plumbing can become noisy -> keep the context type minimal and pass only strings needed by current call sites.
- Chunked equality for disk-backed deduplication may be more code than full materialization -> implement only where it avoids duplicated large payload buffering and add regression tests for dedupe behavior.

## Migration Plan

1. Add the shared writer source I/O helper and unit tests around allocation bounds, prefix reads, exact reads, chunk callbacks, and source-size mismatch handling.
2. Replace TES4 BSA preparation and layout local disk-read helpers, preserving raw streaming serialization behavior.
3. Replace BA2 GNRL preparation disk reads and hashing with the shared exact/chunk helpers.
4. Replace BA2 DX10 DDS and snapshot read loops with shared exact/chunk helpers while preserving snapshot file ownership and cleanup behavior.
5. Update compression-router copy allocation handling.
6. Run focused writer, codec, and stage tests; compare existing fixture outputs for unchanged bytes.

## Open Questions

- None.
