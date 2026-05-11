## 1. Allocation Site Audit

- [x] 1.1 Enumerate direct `std::vector<std::byte>` construction, byte-vector `reserve`, and byte-vector `insert` sites in the listed parser and codec files.
- [x] 1.2 Classify each site as archive-controlled, caller-controlled, fixed-size/local scratch, or already protected by prior validation.
- [x] 1.3 Decide whether `src/detail/byte_vector.hpp` needs an additional byte-vector helper beyond `make_byte_vector`, `reserve_byte_vector`, and `append_byte_vector`.

## 2. Byte-Vector Helper Coverage

- [x] 2.1 Add any missing internal byte-vector helper with Doxygen comments and the same `format_error` translation semantics as the existing helpers.
- [x] 2.2 Ensure helper-level overflow and allocation-failure paths can be exercised deterministically without committing large fixture files.
- [x] 2.3 Preserve existing accurate comments and add short why-comments only where allocation translation is not obvious from the helper call.

## 3. Parser Allocation Translation

- [x] 3.1 Convert TES3 BSA parser byte-buffer reads and copies in `src/formats/bsa/tes3_bsa_parser.cpp` to helper-routed allocation paths.
- [x] 3.2 Convert TES4 BSA parser byte-buffer reads and copies in `src/formats/bsa/tes4_bsa_parser.cpp` to helper-routed allocation paths.
- [x] 3.3 Convert BA2 GNRL parser byte-buffer reads in `src/formats/ba2/ba2_gnrl_parser.cpp` to helper-routed allocation paths.
- [x] 3.4 Convert BA2 DX10 parser byte-buffer reads, encoded filename-table growth, and metadata byte appends in `src/formats/ba2/ba2_dx10_parser.cpp` to helper-routed allocation paths.

## 4. Codec Allocation Translation

- [x] 4.1 Convert deflate compression output allocation in `src/detail/deflate_codec.cpp` to `result`-based byte-vector allocation.
- [x] 4.2 Convert LZ4 frame compression output allocation in `src/detail/lz4_frame_codec.cpp` to `result`-based byte-vector allocation.
- [x] 4.3 Convert raw LZ4 block compression output allocation in `src/detail/lz4_block_codec.cpp` to `result`-based byte-vector allocation.
- [x] 4.4 Confirm decompression paths still use helper-routed exact output allocation and keep their existing size-mismatch errors.

## 5. Regression Tests

- [x] 5.1 Add a TES3 BSA regression proving an oversized but structurally valid declared byte span returns `format_error` without throwing allocation exceptions.
- [x] 5.2 Add a TES4 BSA regression proving an oversized but structurally valid declared byte span returns `format_error` without throwing allocation exceptions.
- [x] 5.3 Add a BA2 GNRL regression proving an oversized but structurally valid declared byte span returns `format_error` without throwing allocation exceptions.
- [x] 5.4 Add a BA2 DX10 regression proving oversized declared filename or metadata byte growth returns `format_error` without throwing allocation exceptions.
- [x] 5.5 Add codec regressions for deflate, LZ4 frame, and raw LZ4 block output allocation translation.

## 6. Validation

- [x] 6.1 Run focused unit tests for the affected parser and codec labels.
- [x] 6.2 Run `ctest --preset windows-msvc-debug-static`.
- [x] 6.3 Run `openspec validate "complete-allocation-error-translation" --strict`.
- [x] 6.4 Run `git diff --check`.
