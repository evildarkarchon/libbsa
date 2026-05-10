# Coding Conventions

**Analysis Date:** 2026-05-10

## Naming Patterns

**Files:**
- Use lowercase snake_case for C++ source and headers. Public headers use `.hpp` under `include/libbsa/`, such as `include/libbsa/archive.hpp`, `include/libbsa/result.hpp`, `include/libbsa/validation.hpp`, and `include/libbsa/writer.hpp`.
- Keep implementation files beside their ownership area: shared internals in `src/detail/binary_io.cpp`, `src/detail/archive_path.cpp`, and `src/detail/parallel_work.cpp`; format code in `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, and `src/formats/ba2/ba2_dx10_writer.cpp`; texture code in `src/texture/directxtex_analyzer.cpp` and `src/texture/dds_layout.cpp`.
- Name tests as `<area>_tests.cpp` under `tests/unit/`, for example `tests/unit/result_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, and `tests/unit/validation_api_tests.cpp`.
- Name fixture generators as `generate_<family>_fixtures.cpp`, such as `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, and `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.

**Functions:**
- Use lowercase snake_case for public APIs and internal helpers: `libbsa::archive_reader::extract_bytes` in `include/libbsa/archive.hpp`, `libbsa::validate_archive` in `include/libbsa/validation.hpp`, `libbsa::detail::normalize_archive_path` in `src/detail/archive_path.cpp`, and `libbsa::formats::ba2::write_ba2_gnrl_archive` in `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Prefer verb-led helper names that state the operation and failure boundary: `read_detection_prefix` in `src/archive.cpp`, `validate_extractability_size_bounds` in `src/validation.cpp`, `checked_u32` in `src/formats/ba2/ba2_gnrl_writer.cpp`, `path_exists_noexcept` in `src/formats/bsa/tes4_bsa_writer.cpp`, and `run_indexed_work` in `src/detail/parallel_work.cpp`.
- Use `*_for` helpers for mapping decisions from enums or metadata, as in `version_for`, `compression_method_for`, `file_flag_for_extension`, and `header_size_for` in `src/formats/bsa/tes4_bsa_writer.cpp` and `src/formats/ba2/ba2_dx10_writer.cpp`.

**Variables:**
- Use lowercase snake_case for locals, parameters, and members exposed in public value types: `host_path`, `archive_path`, `worker_count`, `raw_size`, and `stored_size` in `include/libbsa/archive.hpp` and `include/libbsa/writer.hpp`.
- Use a trailing underscore for private class members, as in `storage_` and `error_` in `include/libbsa/result.hpp`, `state_` in `include/libbsa/archive.hpp`, `bytes_` in `tests/unit/archive_reader_tests.cpp`, and `snapshot_dir` inside `ba2_dx10_writer::state` in `src/formats/ba2/ba2_dx10_writer.cpp`.
- Use lowercase snake_case `constexpr` constants without a `k` prefix: `fixed_header_size` in `src/formats/bsa/tes4_bsa_parser.cpp`, `ba2_record_sentinel` in `src/formats/ba2/ba2_gnrl_writer.cpp`, `payload_stream_chunk_size` in `src/formats/bsa/tes4_bsa_writer.cpp`, and `max_worker_count` in `src/detail/parallel_work.cpp`.

**Types:**
- Use lowercase snake_case for public classes, structs, enums, and enum values: `archive_reader`, `entry_metadata`, `validation_report`, `tes4_bsa_writer`, `error_code::format_error`, and `entry_compression::lz4_block` in `include/libbsa/*.hpp`.
- Use lowercase snake_case for internal structs and helpers: `header_fields`, `folder_record`, `prepared_entry`, `payload_assignment`, and `dedupe_key` in `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, and `src/formats/ba2/ba2_dx10_writer.cpp`.
- Use descriptive local fake/test-double types in tests, such as `collecting_sink`, `partial_sink`, `capturing_sink_factory`, and `fnv1a32_sink` in `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/bulk_extraction_tests.cpp`, and `tests/unit/local_game_fixture_tests.cpp`.

## Code Style

**Formatting:**
- No `.clang-format`, `.clang-tidy`, `.editorconfig`, or lint config is present. Match the existing hand-formatted C++ style in `include/libbsa/archive.hpp`, `src/archive.cpp`, `src/detail/parallel_work.cpp`, and `tests/unit/result_tests.cpp`.
- Use 2-space indentation, K&R braces, one statement per line, and blank lines between include groups, namespaces, helper declarations, and test cases.
- Use `#pragma once` in headers, as shown in `include/libbsa/archive.hpp`, `include/libbsa/result.hpp`, `src/detail/binary_io.hpp`, and `src/texture/dds_layout.hpp`.
- Prefer uniform initialization and default member initializers for value types: `bulk_extract_options::worker_count{1U}` in `include/libbsa/archive.hpp`, `write_execution_options::worker_count{1U}` in `include/libbsa/writer.hpp`, and `validation_report::valid{false}` in `include/libbsa/validation.hpp`.
- Use C++20 vocabulary types at API boundaries: `std::string_view` for host/archive paths, `std::span<const std::byte>` for byte ranges, `std::optional` for absent metadata, and `std::vector<std::byte>` for owned payloads in `include/libbsa/archive.hpp` and `include/libbsa/writer.hpp`.

**Linting:**
- No standalone lint target is configured. Compiler warnings are enforced through CMake: `/W4` for MSVC and `-Wall -Wextra -Wpedantic` for non-MSVC in `CMakeLists.txt`.
- Public documentation coverage is enforced through optional Doxygen configuration and policy tests, not a compiler lint. See `docs/Doxyfile.in`, `tests/unit/docs_policy_tests.cpp`, and `tests/unit/thread_safety_docs_policy_tests.cpp`.
- Public include cleanliness is guarded by source tests in `tests/unit/public_include_boundary_tests.cpp`, which reject private implementation tokens such as `libdeflate`, `DirectXTex`, `formats::`, and `TES5Edit` from headers in `include/libbsa/`.

## Import Organization

**Order:**
1. In implementation files, include the matching local header first, for example `#include "formats/bsa/tes4_bsa_parser.hpp"` in `src/formats/bsa/tes4_bsa_parser.cpp` and `#include "texture/directxtex_analyzer.hpp"` in `src/texture/directxtex_analyzer.cpp`.
2. Include same-project private headers next, using quoted includes for same-area headers such as `"formats/ba2/ba2_publish.hpp"` and angle includes for private include-root headers such as `<detail/archive_path.hpp>` in `src/formats/ba2/ba2_dx10_writer.cpp`.
3. Include public libbsa headers as `<libbsa/...>` when a file consumes the public surface, as in `src/archive.cpp`, `src/validation.cpp`, and tests under `tests/unit/`.
4. Include third-party headers privately in implementation/tests only, such as `<DirectXTex.h>` in `src/texture/directxtex_analyzer.cpp`, `<libdeflate.h>` in `src/detail/deflate_codec.cpp`, and `<nlohmann/json.hpp>` in fixture-backed tests under `tests/unit/`.
5. Include standard library headers after project/third-party headers in source files and after Catch2/public headers in tests, matching `tests/unit/archive_reader_tests.cpp` and `tests/unit/validation_policy_tests.cpp`.

**Path Aliases:**
- Public consumers include `#include <libbsa/libbsa.hpp>` or specific public headers from `include/libbsa/`. The umbrella header is `include/libbsa/libbsa.hpp`.
- Internal code includes from the CMake private `src` include root using paths like `<detail/binary_io.hpp>`, `<detail/compression_router.hpp>`, and `"formats/ba2/ba2_gnrl_reader.hpp"` configured by `target_include_directories(libbsa PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)` in `CMakeLists.txt`.
- Tests may include private headers only for internal policy/unit checks because `tests/CMakeLists.txt` adds `${PROJECT_SOURCE_DIR}/src` to `libbsa_tests`.

## Error Handling

**Patterns:**
- Public APIs return `libbsa::result<T>` or `libbsa::result<void>` for I/O, format, validation, and caller-data failures. Use the stable `libbsa::error_code` enum in `include/libbsa/result.hpp` and return `error{error_code::..., "human diagnostic"}` as done in `src/archive.cpp`, `src/validation.cpp`, `src/detail/deflate_codec.cpp`, and `src/formats/ba2/ba2_gnrl_parser.cpp`.
- Treat `error.message` as diagnostic text only. Tests compare `error().code`, not exact messages, as shown in `tests/unit/result_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, and `tests/unit/validation_api_tests.cpp`.
- Reserve exceptions for programmer misuse or unavoidable standard-library boundaries. `result<T>::value()` and `result<T>::error()` throw `std::logic_error` for misuse in `include/libbsa/result.hpp`; validation catches `std::bad_alloc` and `std::length_error` to keep archive-controlled extractability failures inside `validation_report` in `src/validation.cpp`.
- Use `std::filesystem` overloads with `std::error_code` for expected filesystem failures. See `path_exists_noexcept`, `make_unique_publish_directory`, and cleanup helpers in `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, and `src/formats/ba2/ba2_dx10_writer.cpp`.
- Validate archive-controlled counts, sizes, spans, and offsets before allocating or reading payloads. Use helpers such as `multiply_fits`, `add_fits`, `span_fits`, `checked_u32`, `checked_u16`, and `checked_size` in `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, and `src/texture/dds_layout.cpp`.
- Map partial sink writes to `error_code::io_error` instead of treating them as success. The pattern is implemented in `src/formats/bsa/tes4_bsa_reader.cpp`, `src/detail/payload_stream.cpp`, and exercised in `tests/unit/payload_stream_tests.cpp` and `tests/unit/bulk_extraction_tests.cpp`.

## Logging

**Framework:** None.

**Patterns:**
- Library code under `include/libbsa/` and `src/` does not log. It returns structured `libbsa::error` values and leaves logging/display to consumers; see `docs/integration-examples.md` and `tests/package-consumer/main.cpp`.
- Tests use Catch2 `INFO(...)` for assertion context in files such as `tests/unit/validation_policy_tests.cpp`, `tests/unit/compatibility_matrix_tests.cpp`, and `tests/unit/ba2_dx10_writer_tests.cpp`.
- The benchmark executable is maintainer tooling and may write user-facing failures to `std::cerr`; see `benchmarks/libbsa_benchmarks.cpp`.

## Comments

**When to Comment:**
- Keep accurate existing comments. `AGENTS.md` explicitly forbids deleting accurate comments as cleanup and requires noting any comment removal or rewrite.
- Add comments for non-obvious why: Bethesda compatibility constraints, offset math, snapshot/publish safety, bounded-memory behavior, threading, lifetime ownership, and deliberate private/public boundary decisions.
- Existing examples to follow: TES4 folder offset compatibility in `src/formats/bsa/tes4_bsa_parser.cpp`, no-overwrite publish behavior in `src/formats/ba2/ba2_dx10_writer.cpp`, snapshot ownership in `src/texture/directxtex_analyzer.cpp`, and validation streaming/no-retention behavior in `src/validation.cpp`.

**JSDoc/TSDoc:**
- Use Doxygen-style C++ doc comments (`///`) for public APIs and substantial rewritten methods. Public headers in `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/result.hpp`, `include/libbsa/validation.hpp`, and `include/libbsa/version.hpp` are documented this way.
- Internal headers also document non-obvious contracts when they define reusable helpers, such as `src/detail/binary_io.hpp`, `src/detail/parallel_work.hpp`, `src/detail/deflate_codec.hpp`, and `src/texture/dds_layout.hpp`.
- Doxygen is optional at configure time but the configured documentation input is public headers and public docs only, enforced by `docs/Doxyfile.in` and `tests/unit/docs_policy_tests.cpp`.

## Function Design

**Size:** Keep public wrapper functions short and delegate format-specific behavior to helpers. `src/archive.cpp` routes by detected family, while detailed parsing/writing lives in `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, and `src/formats/ba2/ba2_dx10_writer.cpp`.

**Parameters:** Use `std::string_view` for caller-provided host/archive paths, `std::span<const std::byte>` for byte input, explicit options structs for behavior knobs, and `std::uint32_t` worker counts. Examples are `archive_reader::open`, `archive_reader::extract`, writer `add_file`/`add_bytes` overloads, and `validate_archive` in `include/libbsa/*.hpp`.

**Return Values:** Return `result<T>` for fallible operations and `std::optional` inside successful results for valid absence. `archive_reader::find` returns `result<std::optional<entry_metadata>>` in `include/libbsa/archive.hpp`; `validate_archive` reserves result-level errors for setup failures and reports readable archive problems inside `validation_report` in `src/validation.cpp`.

## Module Design

**Exports:** Export only dependency-light public headers through the `FILE_SET HEADERS` list in `CMakeLists.txt`. Keep `libdeflate`, `lz4`, DirectXTex, private parser structs, threading primitives, and format namespaces out of `include/libbsa/`, as enforced by `tests/unit/public_include_boundary_tests.cpp`.

**Barrel Files:** Use `include/libbsa/libbsa.hpp` as the single public umbrella header. It includes `archive.hpp`, `result.hpp`, `validation.hpp`, `version.hpp`, and `writer.hpp`.

**Internal Boundaries:**
- Place shared implementation helpers in `src/detail/` and keep them under `namespace libbsa::detail`, such as `src/detail/archive_path.hpp`, `src/detail/binary_io.hpp`, `src/detail/compression_router.hpp`, and `src/detail/payload_stream.hpp`.
- Place format-specific readers, parsers, and writers under `src/formats/bsa/` and `src/formats/ba2/` with namespaces `libbsa::formats::bsa` and `libbsa::formats::ba2`.
- Place texture/DDS analysis behind `src/texture/` with namespace `libbsa::texture`; DirectXTex usage belongs in `src/texture/directxtex_analyzer.cpp`, not public headers.
- Treat `TES5Edit/` as read-only reference material only. Do not edit, format, compile, stage, or copy generated outputs into `TES5Edit/`; this boundary is stated in `AGENTS.md`, `tests/fixtures/README.md`, and checked in `.github/workflows/ci.yml`.

---

*Convention analysis: 2026-05-10*
