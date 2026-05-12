## 1. Parser Overflow Guard

- [x] 1.1 Add or reuse a small checked `std::uint64_t` addition helper suitable for archive-derived parser sizes.
- [x] 1.2 Update `src/formats/ba2/ba2_dx10_parser.cpp` so `materialize_entries` rejects raw payload aggregate overflow with `libbsa::error_code::format_error` before updating the running total.
- [x] 1.3 Update `src/formats/ba2/ba2_dx10_parser.cpp` so `materialize_entries` rejects stored payload aggregate overflow with `libbsa::error_code::format_error` before updating the running total.
- [x] 1.4 Validate the 148-byte reconstructed DDS header addition against the raw aggregate before assigning the public entry size.

## 2. Reachable Arithmetic Coverage

- [x] 2.1 Add focused tests for the checked `std::uint64_t` addition helper.
- [x] 2.2 Add BA2 DX10 parser test coverage documenting that the current UInt8 chunk count and UInt32 chunk sizes cannot encode a `std::uint64_t` aggregate overflow fixture.
- [x] 2.3 Leave generated malformed DX10 fixtures unchanged and document that archive-level aggregate overflow is unreachable under the current BA2 DX10 schema.

## 3. Tests and Verification

- [x] 3.1 Run the parser primitive checked-arithmetic tests.
- [x] 3.2 Confirm no fixture regeneration is required because generated fixture outputs are unchanged.
- [x] 3.3 Run the BA2 DX10 malformed archive tests to confirm existing malformed behavior is unchanged.
- [x] 3.4 Run the existing BA2 DX10 positive parser tests to confirm valid aggregate sizes still materialize exact public metadata.
