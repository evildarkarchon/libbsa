# Coding Conventions

**Analysis Date:** 2026-05-12

## Naming Patterns

**Files:**
- Use lowercase `snake_case` filenames for both public and private C++ units, such as `include/libbsa/archive.hpp`, `src/detail/payload_stream.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, and `tests/unit/validation_policy_tests.cpp`.
- Use `*_tests.cpp` for unit/policy tests under `tests/unit/` and `generate_*_fixtures.cpp` for committed fixture generators under `tests/fixtures/generated/`.

**Functions:**
- Use lowercase `snake_case` for free functions, helpers, and methods, including `archive_reader::extract_entries` in `src/archive.cpp`, `validate_payload_stream_range` in `src/detail/payload_stream.cpp`, and `generated_archive_path` in `tests/unit/ba2_dx10_malformed_tests.cpp`.
- Keep file-local helpers inside anonymous namespaces in `.cpp` files, as in `src/archive.cpp`, `src/detail/payload_stream.cpp`, `src/validation.cpp`, and many `tests/unit/*.cpp` files.

**Variables:**
- Use lowercase `snake_case` locals and members, such as `archive_size`, `worker_count`, `first_error`, `allocation_error_`, and `observed_cases` in `src/archive.cpp`, `src/detail/parallel_work.cpp`, and `tests/unit/ba2_dx10_malformed_tests.cpp`.
- Use trailing `_` for private data members, for example `state_` in `include/libbsa/archive.hpp` and `bytes_` in `tests/unit/archive_reader_tests.cpp`.

**Types:**
- Use lowercase `snake_case` even for public types and enums, such as `archive_reader`, `entry_metadata`, `bulk_extract_request`, `validation_report`, and `tes4_bsa_writer` in `include/libbsa/archive.hpp`, `include/libbsa/validation.hpp`, and `include/libbsa/writer.hpp`.
- Use `enum class` for public categories and option sets, as in `include/libbsa/result.hpp`, `include/libbsa/archive.hpp`, and `include/libbsa/writer.hpp`.

## Code Style

**Formatting:**
- No repository formatter config was detected at the root: no `.clang-format`, `.clang-tidy`, or `.editorconfig` accompanies `CMakeLists.txt`.
- Follow the existing hand-formatted style visible in `src/archive.cpp`, `src/detail/payload_stream.cpp`, and `tests/unit/public_include_boundary_tests.cpp`:
  - 2-space indentation
  - opening braces on the same line
  - wrapped parameter lists aligned under the function name
  - namespace closing comments such as `} // namespace libbsa::detail`
  - `#pragma once` in headers such as `include/libbsa/archive.hpp` and `src/detail/payload_stream.hpp`

**Linting:**
- No standalone lint target or lint config is defined; compile warnings are the primary automated style gate in `CMakeLists.txt`.
- Match the warning posture from `CMakeLists.txt`: MSVC builds use `/W4`, non-MSVC builds use `-Wall -Wextra -Wpedantic`, and public builds expose `/Zc:__cplusplus`.
- Policy-style tests in `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/docs_policy_tests.cpp`, `tests/unit/thread_safety_docs_policy_tests.cpp`, and `tests/unit/validation_policy_tests.cpp` act as convention enforcement beyond compiler warnings.

## Import Organization

**Order:**
1. Primary public header or Catch2 header first (`src/archive.cpp`, `src/detail/payload_stream.cpp`, `tests/unit/result_tests.cpp`)
2. Project-private headers next (`src/archive.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`)
3. Standard library headers last (`src/archive.cpp`, `tests/unit/local_game_fixture_tests.cpp`)

**Path Aliases:**
- Public headers use angle-bracket includes rooted at `include/libbsa/`, for example `#include <libbsa/archive.hpp>` in `src/archive.cpp` and `#include <libbsa/libbsa.hpp>` in `tests/unit/result_tests.cpp`.
- Internal sources use `src/` as a private include root and include detail headers as `<detail/...>` or quoted relative paths, for example `#include <detail/payload_stream.hpp>` in `src/detail/payload_stream.cpp` and `#include "formats/ba2/ba2_dx10_reader.hpp"` in `src/archive.cpp`.

## Error Handling

**Patterns:**
- Return `libbsa::result<T>` or `libbsa::result<void>` for fallible work instead of throwing for runtime/archive failures, as defined in `include/libbsa/result.hpp` and used throughout `src/archive.cpp`, `src/detail/payload_stream.cpp`, and `src/validation.cpp`.
- Reserve exceptions for programmer misuse or unavoidable standard-library boundaries:
  - `libbsa::result::value()` and `error()` throw `std::logic_error` on misuse in `include/libbsa/result.hpp`
  - `src/validation.cpp` catches `std::bad_alloc` and `std::length_error` and translates them to structured `format_error`
- Validate arguments early and return stable categories (`invalid_argument`, `io_error`, `format_error`, `unsupported`, `not_found`) before deeper processing, as in `src/archive.cpp`, `src/detail/parallel_work.cpp`, and `src/detail/payload_stream.cpp`.
- Keep error messages diagnostic and human-readable, but treat `error.code` as the stable contract. Tests in `tests/unit/result_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, and `tests/unit/ba2_dx10_malformed_tests.cpp` compare codes and only occasionally check message substrings.

## Logging

**Framework:** None in library code.

**Patterns:**
- Do not add runtime logging to the reusable library surface; `src/` contains no `std::cout`, `std::cerr`, or external logging framework usage.
- Diagnostic output is limited to standalone fixture generator tools under `tests/fixtures/generated/`, such as `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` and `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, where `std::cerr` is used only for command-line failure reporting.

## Comments

**When to Comment:**
- Write comments for non-obvious compatibility, safety, or ownership reasons rather than restating mechanics.
- Follow the why-focused pattern in:
  - `src/formats/bsa/tes3_bsa_parser.cpp` for TES5Edit/UESP compatibility offsets and collision ordering
  - `src/archive.cpp` for bounded extraction and duplicate-request coalescing
  - `src/validation.cpp` for public/private warning-boundary rationale
  - `src/detail/writer_publish.cpp` for Windows publish safety and cleanup behavior

**JSDoc/TSDoc:**
- Use Doxygen-style `///` doc comments for public headers and for important internal abstractions. Examples: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/result.hpp`, and `src/detail/payload_stream.hpp`.
- Keep doc comments tight and contract-oriented: purpose, failure semantics, thread-safety, and ownership.

## Function Design

**Size:**
- Use small single-purpose helpers for parsing, validation, and translation, then compose them from one coordinator. See `src/detail/payload_stream.cpp`, `src/validation.cpp`, and `src/detail/parallel_work.cpp`.
- For larger orchestration functions, prefer sequential guard clauses and explicit variant dispatch over deep nesting, as in `archive_reader::open` and `archive_reader::extract_entries` in `src/archive.cpp`.

**Parameters:**
- Prefer lightweight views and spans at boundaries: `std::string_view` for paths/messages and `std::span<const std::byte>` or `std::span<std::byte>` for payloads, as in `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and `src/detail/payload_stream.hpp`.
- Pass options as value objects when the callee owns scheduling or validation state, such as `bulk_extract_options` in `include/libbsa/archive.hpp` and `write_execution_options` in `include/libbsa/writer.hpp`.

**Return Values:**
- Mark query-like functions `[[nodiscard]]` where ignoring the result would be suspicious, as in `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, and `src/archive.cpp`.
- Return plain values, `std::optional`, or vectors inside `result<T>` instead of output parameters.

## Module Design

**Exports:**
- Keep the public API thin and dependency-light behind `include/libbsa/*.hpp`, re-exported through `include/libbsa/libbsa.hpp`.
- Hide implementation state behind pimpl-style `state` structs in public classes such as `archive_reader` in `include/libbsa/archive.hpp` / `src/archive.cpp` and writer classes in `include/libbsa/writer.hpp`.
- Keep Windows, codec, and DirectX headers private to `src/`, for example `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/detail/writer_publish.cpp`, and `src/texture/directxtex_analyzer.cpp`.

**Barrel Files:**
- Use a single umbrella header, `include/libbsa/libbsa.hpp`, as the only barrel file.
- Do not create additional barrel headers in `src/`; internal code includes specific headers directly.

## Project-Specific Workflow Conventions

- Repo-local OpenSpec skill indexes under `.claude/skills/openspec-*/SKILL.md` standardize artifact-first workflow and verification habits, but they do not introduce a separate C++ formatting style.
- When changing public API or developer-facing behavior, keep code, docs, and policy tests aligned across `include/libbsa/*.hpp`, `docs/*.md`, and `tests/unit/*policy_tests.cpp`.

---

*Convention analysis: 2026-05-12*
