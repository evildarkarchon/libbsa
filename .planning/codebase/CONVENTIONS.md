# Coding Conventions

**Analysis Date:** 2026-05-11

## Naming Patterns

**Files:**
- Use lower snake_case file names for public headers, implementation files, and tests: `include/libbsa/archive.hpp`, `src/detail/binary_io.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`.
- Include archive-family and subtype prefixes in format-specific files: `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`.
- Keep test files named after the surface under test with `_tests.cpp`: `tests/unit/result_tests.cpp`, `tests/unit/archive_path_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.
- Keep fixture generator tools under `tests/fixtures/generated/` with `generate_<format>_fixtures.cpp` names: `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.

**Functions:**
- Use lower snake_case for functions and methods: `archive_reader::open` in `include/libbsa/archive.hpp`, `validate_archive` in `include/libbsa/validation.hpp`, `normalize_archive_path` in `src/detail/archive_path.hpp`.
- Use verb phrases for operations that do work: `read_detection_prefix` in `src/archive.cpp`, `append_warning` in `src/validation.cpp`, `compress_payload` in `src/detail/compression_router.cpp`.
- Use `require_*` helper names in tests for assertion helpers: `require_entry` and `require_extracted_bytes` in `tests/unit/tes4_bsa_writer_tests.cpp`, `require_valid_archive` in `tests/unit/validation_api_tests.cpp`.
- Use `*_from_*` names for conversion helpers: `error_code_from_manifest` in `tests/unit/ba2_dx10_malformed_tests.cpp`, `archive_type_from_string` in `tests/unit/local_game_fixture_tests.cpp`.

**Variables:**
- Use lower snake_case for local variables and public data members: `host_path`, `archive_flags`, `default_compression` in `include/libbsa/archive.hpp`.
- Use a trailing underscore for private class members: `state_` in `include/libbsa/archive.hpp`, `storage_` in `include/libbsa/result.hpp`, `bytes_` in `tests/unit/archive_reader_tests.cpp`.
- Use explicit fixed-width integer types for serialized archive fields: `std::uint32_t`, `std::uint64_t`, and `std::byte` in `src/detail/binary_io.hpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`.
- Use `constexpr` lower snake_case names for local constants: `archive_compress_by_default` in `tests/unit/tes4_bsa_writer_tests.cpp`, `payload_offset` in `tests/unit/validation_api_tests.cpp`.

**Types:**
- Public classes, structs, and enum types use lower snake_case: `archive_reader`, `payload_sink`, `archive_metadata`, `error_code`, `tes4_bsa_writer` in `include/libbsa/archive.hpp`, `include/libbsa/result.hpp`, and `include/libbsa/writer.hpp`.
- Enum values use lower snake_case: `archive_type::bsa`, `entry_compression::lz4_frame`, `archive_compression_policy::target_default` in `include/libbsa/archive.hpp` and `include/libbsa/writer.hpp`.
- Internal namespaces mirror directories: `libbsa::detail` in `src/detail/binary_io.hpp`, `libbsa::formats::bsa` in `src/formats/bsa/tes3_bsa_parser.hpp`, `libbsa::formats::ba2` in `src/formats/ba2/ba2_gnrl_parser.hpp`, `libbsa::texture` in `src/texture/dds_layout.hpp`.
- Internal implementation-only structs use lower snake_case names: `archive_reader::state` in `src/archive.cpp`, `physical_layout` in `tests/unit/ba2_gnrl_writer_tests.cpp`, `validation_archive_case` in `tests/unit/validation_api_tests.cpp`.

## Code Style

**Formatting:**
- Use C++20 throughout. The compile database hint in `.clangd` adds `-std=c++20` and `-Iinclude`, while `CMakeLists.txt` sets `target_compile_features(libbsa PUBLIC cxx_std_20)`.
- No `.clang-format` or `.editorconfig` file is detected. Match the surrounding file style instead of introducing a new formatter profile.
- Use two-space indentation in C++ and CMake files, as shown in `include/libbsa/result.hpp`, `src/detail/binary_io.cpp`, `tests/CMakeLists.txt`, and `CMakeLists.txt`.
- Place opening braces on the same line for functions, classes, namespaces, loops, and `TEST_CASE` blocks: `src/detail/archive_path.cpp`, `tests/unit/result_tests.cpp`.
- Prefer `#pragma once` in headers: `include/libbsa/archive.hpp`, `src/detail/binary_io.hpp`, `src/formats/ba2/ba2_gnrl_writer.hpp`.
- Use direct list initialization for structured errors and value objects: `error{error_code::invalid_argument, "archive path must not be empty"}` in `src/archive.cpp`, `validation_diagnostic{code, diagnostic_message_for(code)}` in `src/validation.cpp`.
- Keep CMake target-based and lower-case: `add_library`, `target_sources`, `target_link_libraries`, and `catch_discover_tests` in `CMakeLists.txt` and `tests/CMakeLists.txt`.

**Linting:**
- Dedicated lint configuration is not detected. There is no `.clang-tidy`, `.clang-format`, or standalone lint target in `CMakeLists.txt`.
- Compiler warnings are the active style gate: MSVC builds use `/W4`; non-MSVC builds use `-Wall -Wextra -Wpedantic` in `CMakeLists.txt`.
- Public API documentation has an optional Doxygen target. `CMakeLists.txt` uses `find_package(Doxygen QUIET)`, and `docs/Doxyfile.in` is checked by `tests/unit/docs_policy_tests.cpp`.
- Public API docs warn on missing documentation but do not fail the build on warnings: `WARN_IF_UNDOCUMENTED = YES` and `WARN_AS_ERROR = NO` are enforced by `tests/unit/docs_policy_tests.cpp`.

## Import Organization

**Order:**
1. Primary header first for implementation files, using quotes for same-module private headers or angle brackets for public headers: `#include <libbsa/archive.hpp>` in `src/archive.cpp`, `#include "formats/bsa/tes3_bsa_parser.hpp"` in `src/formats/bsa/tes3_bsa_parser.cpp`.
2. Local project headers next, grouped by public API, format internals, detail helpers, and texture helpers: `src/archive.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`.
3. Standard library headers next: `<algorithm>`, `<cstddef>`, `<filesystem>`, `<fstream>`, `<span>`, `<string_view>`, `<vector>` in `tests/unit/tes3_bsa_reader_tests.cpp`.
4. Third-party headers last when present: `<nlohmann/json.hpp>` in `tests/unit/local_game_fixture_tests.cpp`, `<DirectXTex.h>` in `src/texture/directxtex_analyzer.cpp`, `<libdeflate.h>` in `src/detail/deflate_codec.cpp`, `<lz4.h>` in `src/detail/lz4_block_codec.cpp`.

**Path Aliases:**
- Public headers are included as `<libbsa/...>` through the `include/` build interface in `CMakeLists.txt`: `include/libbsa/libbsa.hpp`, `tests/unit/result_tests.cpp`.
- Internal library and test code may include `src/`-relative headers because `CMakeLists.txt` and `tests/CMakeLists.txt` add `${CMAKE_CURRENT_SOURCE_DIR}/src` or `${PROJECT_SOURCE_DIR}/src`: `<detail/binary_io.hpp>` in `tests/unit/binary_io_tests.cpp`, `"formats/bsa/tes3_bsa_reader.hpp"` in `tests/unit/tes3_bsa_reader_tests.cpp`.
- Public headers must not include private dependency or implementation symbols. This boundary is enforced in `tests/unit/public_include_boundary_tests.cpp` against `include/libbsa/*.hpp`.

## Error Handling

**Patterns:**
- Use `libbsa::result<T>` and `libbsa::result<void>` for fallible public APIs and expected I/O, validation, and format failures: `include/libbsa/result.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/writer.hpp`.
- Use stable `libbsa::error_code` values for programmatic behavior and keep `error.message` diagnostic-only: `include/libbsa/result.hpp`, `tests/unit/result_tests.cpp`, `tests/unit/validation_api_tests.cpp`.
- Return `error_code::invalid_argument` for caller input errors, `error_code::io_error` for host I/O failures, `error_code::unsupported` for recognized unsupported archive families or variants, and `error_code::format_error` for malformed archive bytes. Examples are in `src/archive.cpp`, `src/detail/archive_path.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, and `src/formats/bsa/tes4_bsa_writer.cpp`.
- Do not throw for expected archive, compression, or filesystem failures. Convert them to `result` errors in boundary code such as `src/detail/parallel_work.cpp`, `src/detail/byte_vector.hpp`, and `src/validation.cpp`.
- Reserve exceptions for programmer misuse or tool-style hard failures. `libbsa::result<T>::value()` and `error()` throw `std::logic_error` on invalid access in `include/libbsa/result.hpp`; fixture generator tools report exceptions from `main()` in `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`.
- Keep validation setup failures at the result level and inspectable archive problems inside `validation_report::errors`: `validate_archive` in `src/validation.cpp`, with tests in `tests/unit/validation_api_tests.cpp`.
- For bulk extraction, report setup failures through the outer `result` and per-entry lookup, sink, and extraction failures inside `bulk_extract_entry_result::failure`: `include/libbsa/archive.hpp`, `src/archive.cpp`, `tests/unit/bulk_extraction_tests.cpp`.

## Logging

**Framework:** console for tools only; library logging is not used.

**Patterns:**
- Do not add logging dependencies such as `spdlog` or formatting libraries to library code. The library returns structured errors from `include/libbsa/result.hpp` and leaves display/logging to consumers.
- Keep `std::cerr` use limited to executable tools and benchmarks: `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `benchmarks/libbsa_benchmarks.cpp`.
- Do not write diagnostic output from parser, reader, writer, validation, compression, or public API code under `src/` or `include/libbsa/`.

## Comments

**When to Comment:**
- Preserve accurate comments. The project policy in `AGENTS.md` forbids deleting comments as cleanup and requires mentioning any removed or rewritten comment in the final reply for implementation work.
- Add comments for non-obvious archive compatibility, public API boundary, ownership, memory-bounds, threading, and error-shaping decisions. Examples include the bounded convenience extraction note in `src/archive.cpp`, parser-coordinate privacy in `src/validation.cpp`, and vcpkg runtime DLL export note in `CMakeLists.txt`.
- Keep inline comments short and focused on why the code exists, not on obvious mechanics. Examples are the sparse validation fixture cleanup note in `tests/unit/validation_api_tests.cpp` and `BAADF00D` physical-layout comments in `tests/unit/ba2_gnrl_writer_tests.cpp`.

**JSDoc/TSDoc:**
- Use Doxygen-style `///` comments for public C++ APIs and for added or substantially rewritten methods. Public headers in `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, and `include/libbsa/result.hpp` document purpose, error semantics, ownership, and thread-safety.
- Use Doxygen comments on non-trivial private helpers when they encode an important rule, as in `src/detail/binary_io.hpp`, `src/validation.cpp`, and `tests/unit/validation_api_tests.cpp`.
- Keep implementation comments out of generated binary fixture outputs and manifests. Fixture behavior and provenance belong in `tests/fixtures/README.md` and generator source files under `tests/fixtures/generated/`.

## Function Design

**Size:** Keep public methods as thin dispatchers over format-specific helpers when possible. `archive_reader::open`, `archive_reader::extract`, and `archive_reader::entries` in `src/archive.cpp` route to `src/formats/bsa/*` and `src/formats/ba2/*`; format-specific parser and writer files own the larger archive contracts.

**Parameters:** Prefer `std::string_view` for archive and host path input, `std::span<const std::byte>` for caller byte ranges, and fixed-width integers for serialized metadata. Examples are `archive_reader::find` in `include/libbsa/archive.hpp`, `tes4_bsa_writer::add_bytes` in `include/libbsa/writer.hpp`, and `binary_reader` in `src/detail/binary_io.hpp`.

**Return Values:** Return `result<T>` or `result<void>` from fallible code. Return plain values for noexcept accessors and deterministic metadata accessors when no failure is possible, such as `validation_report::is_valid` in `include/libbsa/validation.hpp` and `binary_reader::position` in `src/detail/binary_io.hpp`.

**Helpers:** Put file-local helpers and test fakes in anonymous namespaces: `src/archive.cpp`, `src/validation.cpp`, `tests/unit/local_game_fixture_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`. Use named internal namespaces for reusable implementation helpers: `libbsa::detail` in `src/detail/`, `libbsa::formats::bsa` in `src/formats/bsa/`, `libbsa::formats::ba2` in `src/formats/ba2/`.

**Memory and ownership:** Copy caller-provided memory into writer-owned state and document it in public APIs. This convention is explicit in `include/libbsa/writer.hpp` and tested in `tests/unit/tes4_bsa_writer_tests.cpp`. Use `std::shared_ptr<state>` for opaque public objects such as `archive_reader` in `include/libbsa/archive.hpp` and writer classes in `include/libbsa/writer.hpp`.

**Archive paths:** Treat archive-internal paths as normalized virtual keys, not host filesystem paths. Use `normalize_archive_path` in `src/detail/archive_path.cpp`; keep `std::filesystem::path` at host I/O and tests such as `tests/unit/local_game_fixture_tests.cpp` and `tests/unit/validation_api_tests.cpp`.

## Module Design

**Exports:** Public API is limited to the CMake `FILE_SET HEADERS` in `CMakeLists.txt`: `include/libbsa/archive.hpp`, `include/libbsa/libbsa.hpp`, `include/libbsa/result.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/version.hpp`, and `include/libbsa/writer.hpp`.

**Barrel Files:** Use `include/libbsa/libbsa.hpp` as the public umbrella header. Do not add public includes for private implementation headers under `src/detail/`, `src/formats/`, or `src/texture/`.

**Boundary Tests:** Maintain public boundary tests in `tests/unit/public_include_boundary_tests.cpp` when adding public API. This test forbids leaking `libdeflate`, `lz4`, `DirectXTex`, `DXGI`, `Windows.h`, `TES5Edit`, `std::expected`, private namespaces, and writer entry implementation types from `include/libbsa/*.hpp`.

**Dependency Adapters:** Keep external dependency use behind internal adapters: `src/detail/deflate_codec.cpp` for libdeflate, `src/detail/lz4_frame_codec.cpp` and `src/detail/lz4_block_codec.cpp` for LZ4, `src/texture/directxtex_analyzer.cpp` for DirectXTex. Public structs in `include/libbsa/archive.hpp` expose libbsa-owned metadata instead.

**Project Skills:** Project-local skills under `.codex/skills/openspec-*` are OpenSpec workflow guides, not C++ style rules. Use them for OpenSpec change handling only; code style remains governed by `AGENTS.md`, `CMakeLists.txt`, `include/libbsa/`, `src/`, and `tests/`.

---

*Convention analysis: 2026-05-11*
