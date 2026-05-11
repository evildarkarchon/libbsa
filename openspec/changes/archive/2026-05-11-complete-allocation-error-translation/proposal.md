## Why

Archive-controlled sizes and large caller-provided payloads still reach a few direct `std::vector<std::byte>` constructors, `reserve`, `insert`, and codec output allocations outside `detail::byte_vector` helpers. Those paths can surface `std::bad_alloc` or `std::length_error` instead of returning `libbsa::error_code::format_error` through the public `result<T>` contract.

## What Changes

- Route byte-buffer allocation, reserve, and append operations in the affected parsers and codecs through `detail::make_byte_vector`, `detail::reserve_byte_vector`, and `detail::append_byte_vector`.
- Extend `src/detail/byte_vector.hpp` where needed so byte-buffer resizing or append-like operations can translate allocation failures consistently.
- Add sparse-file or oversized-size regressions for TES3 BSA, TES4 BSA, BA2 GNRL, BA2 DX10, and codec allocation paths where a declared span is structurally valid but cannot be allocated.
- Preserve the existing public API shape, dependencies, and Windows-only support boundary.

## Capabilities

### New Capabilities
- `allocation-error-translation`: Defines how libbsa translates byte-buffer allocation failures caused by archive metadata or caller-provided payload sizes into `result<T>` errors.

### Modified Capabilities
- None.

## Impact

- Affected implementation files include `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`, and `src/detail/byte_vector.hpp`.
- Tests will add focused malformed/sparse allocation regressions near the existing parser and codec coverage.
- No new external dependencies, public headers, archive format behavior, or TES5Edit submodule changes are expected.
