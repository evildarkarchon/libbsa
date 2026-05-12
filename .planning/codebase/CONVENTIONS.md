# Coding Conventions

**Analysis Date:** 2026-05-11

## Naming Patterns

**Files:**
- Use lowercase snake_case for C++ implementation and internal header files: `src/detail/archive_path.cpp`, `src/detail/binary_io.hpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/bsa/tes4_bsa_parser.hpp`.
- Public headers live under `include/libbsa/` with lowercase names that match the public concept: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`.
- Tests use lowercase snake_case plus `_tests.cpp`: `tests/unit/archive_path_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/writer_publish_tests.cpp`.
- Generated fixture tools use `generate_<format>_fixtures.cpp`: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.

**Functions:**
- Use lower_snake_case for free functions and member functions: `archive_reader::open` in `src/archive.cpp`, `normalize_archive_path` in `src/detail/archive_path.cpp`, `write_ba2_gnrl_archive` in `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Use small anonymous-namespace helpers for file-local behavior: `read_detection_prefix` and `archive_file_size` in `src/archive.cpp`, `invalid_path_error` in `src/detail/archive_path.cpp`, `truncated_error` in `src/detail/binary_io.cpp`.
- Public API accessors use noun names without `get_`: `archive_reader::metadata`, `archive_reader::entries`, `ba2_gnrl_writer::target`, `ba2_gnrl_writer::options`.

**Variables:**
- Use lower_snake_case for local variables and fields: `host_path`, `archive_flags`, `file_count`, `worker_count`, `payload_offset` in `include/libbsa/archive.hpp` and `include/libbsa/writer.hpp`.
- Private data members use a trailing underscore: `archive_reader::state_` in `include/libbsa/archive.hpp`, `result<T>::storage_` in `include/libbsa/result.hpp`, `binary_reader::position_` in `src/detail/binary_io.hpp`.
- Constants use lower_snake_case with `constexpr`: `max_worker_count` in `src/detail/parallel_work.cpp`.

**Types:**
- Use lower_snake_case for public enums, structs, and classes: `archive_reader`, `archive_metadata`, `entry_compression`, `validation_report`, `tes4_bsa_writer` in `include/libbsa/archive.hpp`, `include/libbsa/validation.hpp`, and `include/libbsa/writer.hpp`.
- Use scoped enums for stable public choices: `error_code` in `include/libbsa/result.hpp`, `archive_type` and `archive_variant` in `include/libbsa/archive.hpp`, `compatibility_warning_code` in `include/libbsa/validation.hpp`.
- Use `state` PIMPL-style nested structs for public writer/reader implementation state: `archive_reader::state` in `src/archive.cpp`, `ba2_gnrl_writer::state` in `src/formats/ba2/ba2_gnrl_writer.cpp`.

## Code Style

**Formatting:**
- No repository `.clang-format`, `.clang-tidy`, or `.editorconfig` file is present. Match the existing style in nearby files.
- Use two-space indentation in C++, CMake, JSON, and YAML: `CMakeLists.txt`, `tests/CMakeLists.txt`, `.github/workflows/ci.yml`, `include/libbsa/result.hpp`.
- Place opening braces on the same line for namespaces, classes, functions, `if`, `for`, and lambdas: `namespace libbsa {` in `src/archive.cpp`, `class result {` in `include/libbsa/result.hpp`.
- Prefer `std::uint32_t`, `std::uint64_t`, `std::size_t`, and `std::byte` for archive data and binary I/O: `src/detail/binary_io.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`.
- Use explicit unsigned suffixes on archive constants and sizes: `36U` in `src/archive.cpp`, `0U` and `1U` in `src/detail/parallel_work.cpp`, `0x00000100U` in `tests/unit/tes3_bsa_reader_tests.cpp`.

**Linting:**
- No separate lint tool is configured. Compiler warnings are the active style gate: MSVC `/W4` and non-MSVC `-Wall -Wextra -Wpedantic` in `CMakeLists.txt`.
- Public C++20 mode is enforced with `target_compile_features(libbsa PUBLIC cxx_std_20)` and `/Zc:__cplusplus` in `CMakeLists.txt`.
- Boundary tests enforce public API cleanliness instead of a linter: `tests/unit/public_include_boundary_tests.cpp` scans public headers for forbidden private dependencies and implementation names.

## Import Organization

**Order:**
1. Matching public or private header first: `#include <libbsa/archive.hpp>` in `src/archive.cpp`, `#include <detail/binary_io.hpp>` in `src/detail/binary_io.cpp`, `#include <catch2/catch_test_macros.hpp>` in tests.
2. Project-private headers with quoted includes for format-local headers and angle-bracket includes for `src/detail` headers: `src/archive.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`.
3. Third-party headers after project headers: `<libdeflate.h>` in `src/detail/deflate_codec.cpp`, `<nlohmann/json.hpp>` in `tests/unit/tes3_bsa_reader_tests.cpp`.
4. Standard library headers last, sorted roughly alphabetically by header name: `src/archive.cpp`, `src/detail/parallel_work.cpp`, `tests/unit/writer_publish_tests.cpp`.

**Path Aliases:**
- Public includes use installed-style paths: `<libbsa/archive.hpp>`, `<libbsa/writer.hpp>`, `<libbsa/result.hpp>`.
- Internal detail includes use the private source include root from `CMakeLists.txt`: `<detail/archive_path.hpp>`, `<detail/binary_io.hpp>`, `<detail/writer_publish.hpp>`.
- Format-local includes use quoted paths rooted at `src`: `"formats/ba2/ba2_gnrl_writer.hpp"`, `"formats/bsa/tes3_bsa_reader.hpp"`.

## Error Handling

**Patterns:**
- Public I/O, parsing, validation, extraction, compression, and writer failures return `libbsa::result<T>` or `libbsa::result<void>` with stable `libbsa::error_code` values. The core type is defined in `include/libbsa/result.hpp`.
- Use `error_code::invalid_argument` for caller-supplied invalid inputs such as empty archive paths and invalid `worker_count`: `archive_reader::open` in `src/archive.cpp`, `run_indexed_work` in `src/detail/parallel_work.cpp`, `ba2_gnrl_writer::write_to` in `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Use `error_code::io_error` for host filesystem failures, allocation failures, worker startup failures, and publish failures: `read_detection_prefix` in `src/archive.cpp`, `compress_deflate` in `src/detail/deflate_codec.cpp`, `run_indexed_work` in `src/detail/parallel_work.cpp`, `tests/unit/writer_publish_tests.cpp`.
- Use `error_code::format_error` for malformed archive bytes, truncation, invalid tables, invalid payload spans, hash mismatches, and decompression size mismatches: `src/detail/binary_io.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`.
- Use `error_code::unsupported` for unsupported or wrong archive variants/routes: `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`.
- Propagate result failures immediately with `if (!value) { return value.error(); }`: `src/archive.cpp`, `src/detail/binary_io.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Exceptions are reserved for programmer misuse or translated at the boundary. `result<T>::value()` and `result<T>::error()` throw `std::logic_error` on incorrect access in `include/libbsa/result.hpp`; thread creation and allocation exceptions are caught and converted to `io_error` in `src/detail/parallel_work.cpp`.

## Logging

**Framework:** console-free library code

**Patterns:**
- Do not introduce logging dependencies. The library reports structured `error` values and lets callers decide how to log or display messages; this is visible across `include/libbsa/result.hpp`, `src/archive.cpp`, and `src/validation.cpp`.
- Human-readable error and warning messages are diagnostics only. Tests should assert stable `error_code` or `compatibility_warning_code` values, not exact messages, except when verifying diagnostic prefixes or policy text as in `tests/unit/writer_publish_tests.cpp`.

## Comments

**When to Comment:**
- Use comments for compatibility-sensitive public API behavior, thread-safety contracts, ownership/lifetime decisions, and non-obvious archive-format constraints. Examples: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `docs/thread-safety.md`.
- Preserve existing compatibility notes and policy comments. `tests/unit/public_include_boundary_tests.cpp` includes explicit comments around DX10 public contract assertions and a SPEC wording correction.
- Do not add comments that restate simple control flow. Private helpers in `src/detail/archive_path.cpp` and `src/detail/binary_io.cpp` are short and mostly comment-free outside namespace-end comments.

**JSDoc/TSDoc:**
- Not applicable. This is a C++ codebase.
- Use Doxygen-style `///` comments for public headers and any new public API. Public documentation is expected for enums, structs, classes, options, methods, result semantics, and thread-safety contracts: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`.
- Optional Doxygen generation is configured in `CMakeLists.txt` and `docs/Doxyfile.in`; documentation policy is tested by `tests/unit/docs_policy_tests.cpp`.

## Function Design

**Size:** Keep helpers focused around one parsing, serialization, validation, or policy responsibility. File-local helper clusters are common in parser and test files: `src/archive.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`.

**Parameters:** Prefer `std::string_view` for borrowed public path/text inputs, `std::span<const std::byte>` for borrowed byte buffers, and `const std::filesystem::path&` for host filesystem paths inside implementation/tests. Examples: `archive_reader::open` in `include/libbsa/archive.hpp`, `ba2_gnrl_writer::add_bytes` in `include/libbsa/writer.hpp`, `write_binary_file` in `tests/unit/writer_publish_tests.cpp`.

**Return Values:** Use `result<T>` for fallible library operations and plain values for non-fallible accessors. Use `std::optional<T>` inside successful results when lookup can validly miss, as in `archive_reader::find` from `include/libbsa/archive.hpp` and `src/archive.cpp`.

## Module Design

**Exports:** Public APIs are declared in `include/libbsa/` and exported with `LIBBSA_API` where needed. Keep implementation details, dependency types, and format helpers in `src/`. The export policy is enforced by `CMakeLists.txt`, `include/libbsa/export.hpp`, `tests/unit/export_surface_policy_tests.cpp`, and `tests/export-surface/check-dll-exports.cmake`.

**Barrel Files:** Use `include/libbsa/libbsa.hpp` as the public umbrella header. Tests include it for public API coverage in `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/result_tests.cpp`, and `tests/unit/local_game_fixture_tests.cpp`. Do not expose private headers, `libdeflate`, `lz4`, `DirectXTex`, `TES5Edit`, or `std::expected` through the public umbrella.

---

*Convention analysis: 2026-05-11*
