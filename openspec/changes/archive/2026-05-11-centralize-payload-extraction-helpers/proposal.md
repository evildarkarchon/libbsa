## Why

Extraction readers currently repeat sink-write loops, chunked buffer writes, archive-size checks, and host-stream limit validation across TES4 BSA, BA2 GNRL, BA2 DX10, and the existing payload stream detail module. Centralizing this logic now reduces drift risk when fixing partial-sink behavior, chunk sizing, or truncated-stream handling.

## What Changes

- Add shared internal payload extraction helpers in `src/detail/payload_stream.*` for checked archive offsets/sizes, exact host-stream reads, full sink writes, and chunked sink writes.
- Refactor TES4 BSA, BA2 GNRL, and BA2 DX10 readers to delegate common extraction I/O mechanics to the shared helpers.
- Keep format-specific compression routing, BSA embedded-name handling, BA2 DX10 DDS header reconstruction, and archive-family metadata interpretation in their existing readers.
- Add focused regression coverage for partial sink writes, chunked extraction, and invalid stream limit handling through the shared helper path.

## Capabilities

### New Capabilities
- `shared-payload-extraction-helpers`: Internal extraction helpers that provide consistent sink-write, chunked-copy, checked-size, and host-stream-limit behavior across archive readers.

### Modified Capabilities
- None.

## Impact

- Affected implementation files include `src/detail/payload_stream.cpp`, `src/detail/payload_stream.hpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, and `src/formats/ba2/ba2_dx10_reader.cpp`.
- Affected tests include helper-level payload stream tests and reader extraction regression tests that already exercise TES4 BSA, BA2 GNRL, and BA2 DX10 payload reads.
- No public libbsa API, dependency, CMake preset, vcpkg manifest, or TES5Edit submodule change is expected.
