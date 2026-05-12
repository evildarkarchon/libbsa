## 1. Shared Helper Contracts

- [x] 1.1 Extend `src/detail/payload_stream.hpp` with internal helper declarations for checked payload size conversion, exact sink writes, chunked span writes, exact file reads at archive offsets, and raw range streaming.
- [x] 1.2 Implement the new helpers in `src/detail/payload_stream.cpp` while preserving existing `transfer_payload` behavior and using caller-provided descriptions for diagnostics.
- [x] 1.3 Expand `tests/unit/payload_stream_tests.cpp` to cover size-limit rejection, stream-limit rejection, truncated exact reads, raw range streaming, chunked span writes, and partial sink write failures.

## 2. Format Reader Refactor

- [x] 2.1 Refactor `src/formats/bsa/tes4_bsa_reader.cpp` to use shared helpers for raw payload streaming, compressed payload reads, and decoded buffer output while keeping embedded-name and compressed-size-prefix logic local.
- [x] 2.2 Refactor `src/formats/ba2/ba2_gnrl_reader.cpp` to use shared helpers for raw payload streaming, stored payload reads, and decoded buffer output while keeping BA2 GNRL compression routing local.
- [x] 2.3 Refactor `src/formats/ba2/ba2_dx10_reader.cpp` to use shared helpers for DDS header writes, raw chunk streaming, stored chunk reads, and decoded chunk output while keeping DDS reconstruction and texture chunk ordering local.
- [x] 2.4 Remove duplicated reader-local helper functions and unused includes after the refactor.

## 3. Verification

- [x] 3.1 Run focused unit tests for `[payload-stream]`, TES4 BSA extraction, BA2 GNRL extraction, and BA2 DX10 extraction paths.
- [x] 3.2 Run the repository's normal Windows CTest preset or the closest available local test command and fix any regressions.
- [x] 3.3 Confirm no public headers, dependency manifests, CMake presets, or `TES5Edit/` files changed as part of the implementation.
