## 1. Shared Writer Source I/O

- [x] 1.1 Add an internal writer disk-source helper under `src/detail/` with exact whole-file reads, bounded prefix reads, and chunk iteration APIs.
- [x] 1.2 Route helper allocations through existing byte-vector allocation wrappers and preserve caller-supplied open, inspect, read, allocation, and source-changed diagnostics.
- [x] 1.3 Add the new helper source/header to the CMake target without exposing it through public headers.
- [x] 1.4 Add focused unit tests for exact reads, prefix reads, chunk callbacks, missing sources, short reads, appended-byte detection, and allocation-limit errors where practical.

## 2. TES4 BSA Writer Integration

- [x] 2.1 Replace `tes4_bsa_prepare.cpp` full-source and prefix disk read loops with the shared helper while keeping compressed payload codec boundaries explicit.
- [x] 2.2 Replace `tes4_bsa_layout.cpp` disk payload materialization with the shared helper and avoid duplicate whole-file buffers during disk-backed dedupe where possible.
- [x] 2.3 Verify TES4 raw streaming serialization still uses bounded chunks and reports source mutation with the existing family-specific diagnostic.
- [x] 2.4 Add or update TES4 writer tests for compressed disk entries, DDS prefix probing, dedupe stability, and unchanged archive bytes.

## 3. BA2 Writer Integration

- [x] 3.1 Replace BA2 GNRL compressed-entry disk reads with the shared exact-read helper.
- [x] 3.2 Replace BA2 GNRL raw disk payload hashing with the shared chunk iteration helper.
- [x] 3.3 Replace BA2 DX10 DDS add-time source reads with the shared exact-read helper while preserving DirectXTex analysis behavior.
- [x] 3.4 Replace BA2 DX10 snapshot read appends with bounded chunk/append helper usage and preserve snapshot cleanup semantics.
- [x] 3.5 Add or update BA2 GNRL and BA2 DX10 writer tests for compressed disk entries, raw hashing, DDS analysis, chunk preparation, and unchanged archive bytes.

## 4. Compression Router Allocation Handling

- [x] 4.1 Update `detail::compression_router` no-compression copy paths to use result-based byte-vector allocation helpers.
- [x] 4.2 Add or update compression router tests so allocation/copy failures remain result-based and existing compression behavior is unchanged.

## 5. Verification

- [x] 5.1 Run focused unit tests for writer source I/O, TES4 BSA writer, BA2 GNRL writer, BA2 DX10 writer, writer stages, and compression router.
- [x] 5.2 Run the broader CTest suite or the project-standard preset test command to verify no archive fixture regressions.
- [x] 5.3 Inspect for remaining local byte-at-a-time disk source read loops in the targeted writer preparation/layout files.
