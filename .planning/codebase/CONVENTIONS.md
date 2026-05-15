---
last_mapped: 2026-05-15
last_mapped_commit: ead68b6856b37c3ea3acbf007180951c3cc22698
---

# Coding Conventions

**Analysis Date:** 2026-05-15

## Project-Specific Constraints

- Treat `TES5Edit/` as a read-only behavioral reference. Do not edit, format, stage, compile, or vendor files under `TES5Edit/`; implementation and tests live under `include/`, `src/`, `tests/`, `docs/`, `cmake/`, and `.github/`.
- Maintain the Windows-only C++20 library target. Do not add portability work for POSIX/Linux/macOS unless the project constraint changes. Windows-specific code is acceptable behind `#if defined(_WIN32)`, as in `src/detail/writer_publish.cpp` and `tests/unit/writer_publish_tests.cpp`.
- Keep public headers dependency-light. Public API files in `include/libbsa/` must not expose `libdeflate`, LZ4, DirectXTex, DXGI, Windows headers, internal namespaces, or TES5Edit names; this boundary is enforced by `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/bounded_memory_policy_tests.cpp`, and `tests/unit/export_surface_policy_tests.cpp`.
- Use the local C++20 `libbsa::result<T>` type in `include/libbsa/result.hpp`; do not expose C++23 `std::expected`.
- Project skills under `.claude/skills/` are OpenSpec workflow skills (`openspec-*`). They do not define C++ formatting rules, but they reinforce artifact-driven changes and validation before archiving.

## Naming Patterns

**Files:**
- Public API headers use lowercase component names under `include/libbsa/`: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`.
- Private implementation files use snake_case and mirror module names: `src/detail/archive_path.cpp`, `src/detail/compression_router.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`.
- Private headers use `.hpp` and sit next to their implementation in `src/`: `src/formats/bsa/tes4_bsa_table.hpp`, `src/texture/dds_layout.hpp`.
- Tests use `*_tests.cpp` under `tests/unit/` with feature-oriented names: `tests/unit/archive_path_tests.cpp`, `tests/unit/validation_api_tests.cpp`, `tests/unit/writer_publish_tests.cpp`.
- CMake helper scripts use descriptive kebab words in scoped folders: `tests/package-consumer/smoke.cmake`, `tests/export-surface/check-dll-exports.cmake`.

**Functions:**
- Use snake_case for free functions and member functions: `normalize_archive_path` in `src/detail/archive_path.cpp`, `validate_archive` in `src/validation.cpp`, `extract_ba2_gnrl_payload` in `src/formats/ba2/ba2_gnrl_reader.cpp`.
- Use `read_*`, `write_*`, `parse_*`, `validate_*`, `make_*`, and `append_*` prefixes for action-oriented helpers: `read_u32_le` in `src/detail/binary_io.cpp`, `append_warning` in `src/validation.cpp`, `make_tes4_bsa_payload_descriptor` referenced by `src/formats/bsa/tes4_bsa_parser.cpp`.
- Keep helper functions in anonymous namespaces when translation-unit local, as in `src/validation.cpp`, `src/detail/compression_router.cpp`, and `tests/unit/ba2_gnrl_reader_tests.cpp`.
- Use `require_*` helper names in tests for assertion wrappers, such as `require_valid_archive` and `require_matrix_open_report` in `tests/unit/validation_api_tests.cpp`.

**Variables:**
- Use snake_case for local variables and data members. Private members use a trailing underscore: `storage_` in `include/libbsa/result.hpp`, `bytes_` in `tests/unit/ba2_gnrl_reader_tests.cpp`, `state_` in `include/libbsa/archive.hpp`.
- Use `constexpr` for constants and append `U`/`ULL` where unsigned width matters: `extraction_chunk_size` in `src/formats/ba2/ba2_gnrl_reader.cpp`, `reconstructed_dds_header_size` in `src/formats/ba2/ba2_dx10_parser.cpp`.
- Use descriptive domain names instead of abbreviations when possible: `archive_path`, `payload_offset`, `compression_method`, `metadata`, `validation_options`.

**Types:**
- Use lowercase snake_case for public and private types: `archive_reader`, `payload_sink`, `ba2_gnrl_writer`, `validation_report`, `header_fields`, `dx10_record`.
- Public enums use `enum class` with lowercase enumerators: `error_code::format_error` in `include/libbsa/result.hpp`, `entry_compression::lz4_block` in `include/libbsa/archive.hpp`.
- Private implementation structs are simple aggregate-like types when they represent parsed fields, such as `header_fields` and `dx10_chunk_record` in `src/formats/ba2/ba2_dx10_parser.cpp`.

## Code Style

**Formatting:**
- C++ uses two-space indentation inside namespaces/classes/functions and Allman-style braces for functions, classes, namespaces, `if`, `switch`, and `for` blocks.
- Namespace comments are present at namespace close sites: `} // namespace libbsa`, `} // namespace libbsa::detail`, and `} // namespace`.
- Include guards use `#pragma once` in headers such as `include/libbsa/archive.hpp` and `src/texture/directxtex_analyzer.hpp`.
- `.clangd` sets `-std=c++20` and `-Iinclude`; no `.clang-format`, `.editorconfig`, or formatter config is detected.
- Use `std::byte` for binary data and explicit casts when crossing `char`/integer boundaries, as in `src/detail/binary_io.cpp`, `tests/unit/writer_publish_tests.cpp`, and `tests/unit/local_game_fixture_tests.cpp`.

**Linting:**
- No standalone linter configuration is detected. Compiler warnings are configured in `CMakeLists.txt`: MSVC `/W4`, non-MSVC `-Wall -Wextra -Wpedantic`, and public MSVC `/Zc:__cplusplus`.
- Treat warning-clean C++20 builds as the style gate. CI runs Windows MSVC Debug/Release static/shared and an ASan lane in `.github/workflows/ci.yml`.

## Include Organization

**Order:**
1. The current file's header first for `.cpp` files, using either angle brackets for `src/detail` headers (`#include <detail/binary_io.hpp>`) or quotes for sibling format/texture headers (`#include "formats/ba2/ba2_dx10_parser.hpp"`).
2. Project public headers such as `#include <libbsa/libbsa.hpp>` or `#include <libbsa/result.hpp>`.
3. Project private headers such as `#include <detail/archive_path.hpp>` and format-private headers such as `#include "formats/bsa/tes4_bsa_constants.hpp"`.
4. Standard library headers.
5. Third-party headers such as `#include <nlohmann/json.hpp>` or `#include <DirectXTex.h>`.

**Path Aliases:**
- Public includes are rooted at `include/`: `#include <libbsa/archive.hpp>`.
- Private implementation includes are rooted at `src/`: `#include <detail/host_file.hpp>` and `#include "formats/ba2/ba2_gnrl_parser.hpp"`.
- Tests include both `include/` and `src/` through `tests/CMakeLists.txt`, so tests may cover internal seams with `#include <detail/...>` and `#include "formats/..."`.

## API Boundary and Module Design

**Public API:**
- Add exported public types and functions only under `include/libbsa/` and annotate emitted classes/functions with `LIBBSA_API`, as checked by `tests/unit/export_surface_policy_tests.cpp`.
- Public headers must include Doxygen `///` documentation for public types and methods. Examples include `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and `include/libbsa/validation.hpp`.
- Public APIs should use `std::string_view`, `std::span<const std::byte>`, concrete metadata values, and `libbsa::result<T>`; avoid leaking `std::filesystem::path`, `DirectX::TexMetadata`, codec handles, threads, or private namespaces.
- Use move-only writer classes for staged mutable state. Writers in `include/libbsa/writer.hpp` delete copy operations and define move operations because copying would alias mutable staged entries.

**Private implementation:**
- Put reusable low-level helpers under `src/detail/`: binary I/O (`src/detail/binary_io.cpp`), host file I/O (`src/detail/host_file.cpp`), path normalization (`src/detail/archive_path.cpp`), compression routing (`src/detail/compression_router.cpp`), publishing (`src/detail/writer_publish.cpp`), and parallel work (`src/detail/parallel_work.cpp`).
- Put archive-family logic under `src/formats/bsa/` and `src/formats/ba2/`, with parser/reader/prepare/layout/serialize/writer seams split into separate files.
- Keep DirectXTex usage private under `src/texture/`, especially `src/texture/directxtex_analyzer.cpp`.
- Prefer explicit seam functions over monolithic implementations; tests directly exercise parser/preparer/layout/publish seams through files such as `tests/unit/tes4_bsa_parser_seam_tests.cpp`, `tests/unit/ba2_dx10_preparer_seam_tests.cpp`, and `tests/unit/writer_stage_tests.cpp`.

**Exports:**
- There are no broad barrel files beyond the public umbrella header `include/libbsa/libbsa.hpp`, which includes `archive.hpp`, `result.hpp`, `validation.hpp`, `version.hpp`, and `writer.hpp`.
- Do not add transitive public dependencies casually; `tests/unit/public_include_boundary_tests.cpp` scans public headers for forbidden dependency tokens.

## Error Handling

**Primary pattern:**
```cpp
auto opened = libbsa::archive_reader::open(host_path);
if (!opened)
{
  return opened.error();
}
```
- Return `libbsa::result<T>` or `libbsa::result<void>` for fallible library operations. This pattern is used in `src/detail/binary_io.cpp`, `src/detail/host_file.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, and `src/validation.cpp`.
- Use stable `libbsa::error_code` categories from `include/libbsa/result.hpp`: `unsupported`, `invalid_argument`, `not_found`, `io_error`, and `format_error`.
- Tests should assert `error().code`, not exact diagnostic messages, unless message prefix/containment is the behavior under test. `include/libbsa/result.hpp` documents that `error.message` is human diagnostic text and should not be compared exactly by tests.

**When to return which error:**
- Use `error_code::invalid_argument` for caller misuse or invalid options, such as empty host paths in `src/validation.cpp`, invalid archive virtual paths in `src/detail/archive_path.cpp`, zero chunk sizes in `src/detail/host_file.cpp`, and `worker_count == 0` in `src/detail/parallel_work.cpp`.
- Use `error_code::io_error` for host filesystem access, publication, file mutation, thread startup, or sink short-write failures. Examples: `src/detail/host_file.cpp`, `src/detail/writer_publish.cpp`, and `src/detail/parallel_work.cpp`.
- Use `error_code::format_error` for malformed archive bytes, inconsistent metadata, decompression mismatch, unsupported in-family record shapes, and archive-controlled size failures. Examples: `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, and `src/texture/directxtex_analyzer.cpp`.
- Use `error_code::unsupported` for recognized-but-unsupported archive profiles/routes where parsing should fail as unsupported.
- Use `error_code::not_found` when a valid path lookup is absent at the API usage layer, as shown by `tests/package-consumer/main.cpp`.

**Exceptions:**
- Public and internal fallible paths should not throw for expected I/O, format, validation, or caller-data errors; return `result` instead.
- `libbsa::result<T>::value()` and `.error()` throw `std::logic_error` for programmer misuse, as implemented in `include/libbsa/result.hpp` and tested by `tests/unit/result_tests.cpp`.
- Catch allocation and system exceptions at boundaries where vector allocation or worker creation can fail. Examples: `src/formats/bsa/tes4_bsa_parser.cpp` catches `std::bad_alloc`/`std::length_error` and maps to allocation-related format diagnostics; `src/detail/parallel_work.cpp` catches `std::system_error` and `std::bad_alloc`.

## Validation Practices

- Archive path validation is centralized in `src/detail/archive_path.cpp`. Normalize separators to `/`, lowercase ASCII, reject empty paths, absolute/rooted paths, drive-rooted paths, embedded NULs, empty segments, `.`, and `..`.
- Metadata size and span validation should happen before allocation or slicing. Use helpers from `src/detail/parser_primitives.hpp` and patterns in `src/formats/ba2/ba2_dx10_parser.cpp`, such as checking aggregate chunk counts and table/payload overlap before materializing metadata.
- Host file reads validate expected size before and after read to detect mutation while reading. Use `src/detail/host_file.cpp` helpers (`inspect_host_file_size`, `read_host_file_exact`, `for_each_host_file_chunk`) instead of ad hoc file reads in production code.
- Writer output publication must go through `src/detail/writer_publish.cpp` to reserve isolated temp directories, reject unsafe overwrite targets, reject reparse-point outputs on Windows, and publish atomically.
- Validation reports separate setup failures from inspectable archive failures. In `src/validation.cpp`, unreadable/invalid host paths fail the outer `result`, while readable malformed archives become `validation_report::errors`.
- Compatibility warnings have stable codes and severities in `include/libbsa/validation.hpp`; add new warning classes there instead of encoding warning semantics only in strings.
- Bound extraction validation with `validation_options::max_extractability_entry_bytes` in `include/libbsa/validation.hpp`; do not allocate archive-controlled sizes without a cap.

## Logging

**Framework:** None.

**Patterns:**
- Library code does not log. Return structured `error` and `validation_report` values and let the caller decide how to display diagnostics.
- Tests may use Catch2 `INFO` and `WARN` for context or environment-dependent skips, as in `tests/unit/ba2_dx10_malformed_tests.cpp` and `tests/unit/writer_publish_tests.cpp`.
- Do not add external logging dependencies such as `spdlog`; the project dependency policy favors structured errors.

## Comments and Documentation

**When to Comment:**
- Add comments for non-obvious format compatibility constraints, file-system safety decisions, allocation limits, concurrency behavior, or deliberate public API boundaries.
- Good examples:
  - Hash compatibility rationale in `src/formats/bsa/tes4_bsa_parser.cpp` before rejecting mismatched TES4 file record hashes.
  - Public warning boundary rationale in `src/validation.cpp` before appending warnings without parser offsets.
  - Writer temp directory cleanup rationale in `src/detail/writer_publish.cpp`.
  - DirectXTex snapshot ownership rationale in `src/texture/directxtex_analyzer.cpp`.
- Do not delete accurate comments as cleanup. Rewrite comments only when the code they describe changes enough to make them wrong.

**Doxygen/TSDoc:**
- Use Doxygen-style `///` comments for public C++ APIs and non-obvious helper classes/functions. Public headers in `include/libbsa/` consistently use this form.
- Public documentation policy is tested by `tests/unit/docs_policy_tests.cpp` and `tests/unit/thread_safety_docs_policy_tests.cpp`.
- Avoid leaking planning identifiers such as phase labels, milestone labels, and decision IDs into public documentation; `tests/unit/docs_policy_tests.cpp` checks selected public docs for these tokens.

## Function Design

**Size:**
- Prefer small helpers with a single validation or transformation responsibility. Examples: `invalid_path_error` and `lower_ascii` in `src/detail/archive_path.cpp`, `compression_method_for` in `src/formats/ba2/ba2_gnrl_reader.cpp`, and `checked_u32` in `src/texture/directxtex_analyzer.cpp`.
- Larger parser functions are acceptable when they encode sequential binary format parsing, but keep inner operations factored into `read_header`, `read_records`, table-size helpers, and payload descriptor helpers as in `src/formats/ba2/ba2_dx10_parser.cpp` and `src/formats/bsa/tes4_bsa_parser.cpp`.

**Parameters:**
- Use `std::string_view` for caller-provided paths/text where ownership is not needed.
- Use `std::span<const std::byte>` for binary input views.
- Use `const std::filesystem::path &` only for internal host filesystem helpers; keep host path types out of public APIs.
- Use options structs for public configuration, such as `tes4_bsa_writer_options`, `ba2_gnrl_writer_options`, `validation_options`, and `write_execution_options`.

**Return Values:**
- Use `result<T>` for fallible computations and immediately propagate errors with `return some_result.error();`.
- Use `std::optional<T>` inside successful results when absence is not an error, such as `archive_reader::find` returning `result<std::optional<entry_metadata>>` in `include/libbsa/archive.hpp`.
- Mark public query methods `[[nodiscard]]` when ignoring the result would be suspicious.

## Module Design

**Tests as Policy:**
- Policy/static-boundary tests are part of conventions. Add or update tests such as `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`, `tests/unit/bounded_memory_policy_tests.cpp`, and `tests/unit/docs_policy_tests.cpp` when public boundary conventions change.

**Barrel Files:**
- Use `include/libbsa/libbsa.hpp` as the only public umbrella include. Do not create broad private barrel headers unless they simplify a clear internal seam.

**Generated or Reference Code:**
- Generated fixtures and manifests live under `tests/fixtures/generated/`. Fixture generator source is committed under `tests/fixtures/generated/*.cpp`; archive/manifest outputs are committed as compatibility evidence.
- Do not use `TES5Edit/` as mutable fixture storage. Do not scan it into production builds.

---

*Convention analysis: 2026-05-15*
