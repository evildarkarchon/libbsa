## 1. Shared Parser Primitive Module

- [x] 1.1 Add an internal `detail::parser_primitives` header/source pair and wire it into the libbsa CMake target without changing public headers.
- [x] 1.2 Implement shared checked arithmetic, span containment, bounded host-file reads, allocation-safe archive string materialization, and display separator normalization.
- [x] 1.3 Add concise doc comments for the non-obvious helper contracts, especially allocation translation and exact bounded-read behavior.
- [x] 1.4 Add focused helper tests for arithmetic overflow, span rejection, stream offset/count limits, truncated reads, string allocation translation, and separator normalization.

## 2. Parser Refactor

- [x] 2.1 Refactor `tes3_bsa_parser.cpp` to use the shared primitives while preserving TES3 data-section offset and hash validation behavior.
- [x] 2.2 Refactor `tes4_bsa_parser.cpp` to use the shared primitives while preserving TES4-family folder/name table, embedded-name, and metadata-overlap checks.
- [x] 2.3 Refactor `ba2_gnrl_parser.cpp` to use the shared primitives while preserving BA2 GNRL sentinel, filename table, compression metadata, and payload-span checks.
- [x] 2.4 Refactor `ba2_dx10_parser.cpp` to use the shared primitives while preserving DX10 chunk, filename table, cubemap/mip, and payload-span checks.
- [x] 2.5 Remove parser-local duplicate helper definitions only after their callers have moved to the shared module.

## 3. Regression Coverage and Validation

- [x] 3.1 Add or update parser regression tests proving TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 still reject malformed archive-controlled spans through `result` failures.
- [x] 3.2 Add or update allocation-error coverage proving shared parser file-read and string-materialization failures do not leak `std::bad_alloc` or `std::length_error`.
- [x] 3.3 Verify the parser modules no longer define duplicate `multiply_fits`, `add_fits`, `span_fits`, `read_file_bytes_at`, or byte-to-string helpers.
- [x] 3.4 Run focused helper/parser tests, then `cmake --build --preset windows-msvc-debug-static --target libbsa_tests` and `ctest --preset windows-msvc-debug-static --output-on-failure`.
- [x] 3.5 Confirm `git diff --check` passes and `TES5Edit/` remains untouched.
