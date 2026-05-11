## Context

`src/detail/byte_vector.hpp` already centralizes translation of byte-buffer allocation failures into `libbsa::error_code::format_error`. Several parser and codec paths still allocate byte vectors directly from archive metadata or caller-provided payload sizes:

- TES3/TES4 BSA and BA2 GNRL/DX10 parser helpers allocate file/name-table byte buffers with `std::vector<std::byte>(count)`.
- BA2 DX10 name-table parsing reserves and appends directly while rebuilding encoded name bytes.
- Deflate and LZ4 compression paths allocate codec output buffers from compression-bound calculations before returning `result<std::vector<std::byte>>`.

Those paths are inside public open/extract/write-style workflows where malformed archives and large caller inputs should return `result<T>` errors, not escape as C++ allocation exceptions.

## Goals / Non-Goals

**Goals:**

- Ensure byte-buffer allocations derived from archive metadata or caller payload sizes use the same `format_error` translation boundary.
- Keep parser and codec behavior compatible with the current archive-format rules; only allocation/error translation changes.
- Add focused regressions that prove oversized but otherwise structurally plausible byte spans return `format_error`.
- Preserve existing public APIs, dependency choices, and Windows-only project support.

**Non-Goals:**

- Do not add new dependencies or expose allocation-limit controls in public headers.
- Do not change non-byte metadata container ownership unless implementation reveals an adjacent allocation path that must be translated to preserve the same public contract.
- Do not edit, stage, or regenerate anything under `TES5Edit/`.
- Do not introduce cross-platform filesystem or portability work.

## Decisions

1. Use `detail::byte_vector` helpers as the byte-buffer allocation boundary.

   Direct byte-buffer construction, reserve, and append operations in the affected files should be replaced by `make_byte_vector`, `reserve_byte_vector`, and `append_byte_vector`. This keeps allocation failure behavior local and consistent without broad `try`/`catch` blocks around parser entry points.

   Alternative considered: catch `std::bad_alloc` and `std::length_error` at public API boundaries. That would be broader but less precise, and it risks translating programmer or unrelated implementation allocation failures that should not be hidden as malformed archive data.

2. Extend `byte_vector.hpp` only for missing byte-buffer operations.

   If implementation needs a grow, resize, or assign operation that cannot be expressed clearly with the existing helpers, add a narrow helper in `src/detail/byte_vector.hpp` with Doxygen comments and the same `format_error` translation behavior. Keep the helper internal and byte-specific.

   Alternative considered: duplicate local safe-allocation wrappers in each parser/codec. That would avoid touching the helper header, but it would make future review harder and drift from the existing project pattern.

3. Keep compression output allocation result-based.

   `compress_deflate`, `compress_lz4_frame`, and `compress_lz4_block` should allocate their output vectors through helper-backed paths before invoking third-party codec APIs. Decompression already uses `make_byte_vector`; compression should match that contract for caller-controlled input sizes.

   Alternative considered: trust codec bound values because they are derived from already-materialized input spans. The input span is still caller-controlled, and the function returns `result<T>`, so allocation failure should stay inside the error contract.

4. Prefer focused regression tests over broad fixture regeneration.

   Tests should mutate existing generated fixtures or create temporary sparse archives in the test body so the declared spans exercise the allocation path without committing giant files. Codec tests should cover helper-routed allocation behavior directly where archive fixtures cannot express a deterministic impossible allocation size.

   Alternative considered: add large generated fixture binaries. That would be expensive, noisy, and unnecessary for proving error translation.

## Risks / Trade-offs

- Oversized sparse-file tests can become slow or memory-sensitive if they actually force multi-gigabyte allocation attempts -> keep test inputs targeted, prefer deterministic helper overflow cases when possible, and assert error codes rather than process-level exceptions.
- Replacing direct `insert` calls can slightly increase boilerplate -> keep descriptions specific enough that failures identify the archive family or codec output being allocated.
- The helper currently only covers `std::vector<std::byte>` -> if implementation finds archive-controlled non-byte vector reservations that can still violate the same public contract, handle them in the narrowest matching scope and document why.

## Migration Plan

1. Update the helper surface if a missing byte-vector operation is needed.
2. Convert the parser and codec byte-vector allocations called out by the proposal.
3. Add regression coverage for TES3 BSA, TES4 BSA, BA2 GNRL, BA2 DX10, and deflate/LZ4 codec output allocation paths.
4. Run focused unit labels first, then the repo’s Windows MSVC debug static test preset and `git diff --check`.

## Open Questions

- None known. The implementation pass should verify the exact set of direct byte-vector allocation sites before editing.
