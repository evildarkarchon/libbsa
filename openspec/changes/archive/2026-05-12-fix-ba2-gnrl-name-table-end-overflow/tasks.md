## 1. Parser Guard

- [x] 1.1 Import or otherwise reference `detail::add_fits_u64` in `src/formats/ba2/ba2_gnrl_parser.cpp`.
- [x] 1.2 Replace the host-file path's unchecked `FileTableOffset + name_table_consumed` calculation with checked `std::uint64_t` addition.
- [x] 1.3 Return `libbsa::error_code::format_error` with a BA2 GNRL filename-table diagnostic when the aggregate end calculation overflows.
- [x] 1.4 Confirm the in-memory parse path continues to use its existing `std::size_t` checked-add guard and remains behaviorally unchanged.

## 2. Regression Coverage

- [x] 2.1 Add BA2 GNRL malformed reader coverage for a high-`FileTableOffset` filename table whose aggregate end is rejected before entry metadata materialization.
- [x] 2.2 Use a sparse host-file fixture when the filesystem supports the required range; otherwise keep the overflow branch covered through checked-add/parser primitive coverage and document the fixture limitation in the test.
- [x] 2.3 Confirm existing successful high-offset/end-table BA2 GNRL tests still prove payload-before-filename-table archives open correctly.

## 3. Verification

- [x] 3.1 Run the BA2 GNRL reader tests that cover malformed metadata and end filename tables.
- [x] 3.2 Run parser primitive tests if new or existing checked-add assertions are part of the coverage.
- [x] 3.3 Run the relevant OpenSpec validation/status command and confirm the change remains apply-ready.
