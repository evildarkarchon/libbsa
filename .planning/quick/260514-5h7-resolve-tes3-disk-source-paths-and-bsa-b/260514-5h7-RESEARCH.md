# Quick Research: Resolve TES3 Disk Sources and Writer Output Paths Through UTF-8 Host Helpers

**Researched:** 2026-05-14  
**Domain:** Windows UTF-8 host-path boundary for BSA/BA2 writer source and output paths  
**Confidence:** HIGH

## Summary

The project already has a single internal host-path boundary: `detail::resolve_host_file_path(std::string_view)` stores caller UTF-8 bytes for diagnostics and a decoded native `std::filesystem::path` for I/O. [VERIFIED: src/detail/host_file_path.hpp:11-21] On Windows this boundary uses `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS)` before constructing `std::filesystem::path`, explicitly avoiding MSVC narrow-path active-code-page behavior. [VERIFIED: src/detail/host_file_path.cpp:16-42]

The immediate gap is writer-side consistency. TES4 and BA2 GNRL disk-source flows already resolve source host paths before shared host-file reads and carry resolved paths for raw finalization. [VERIFIED: src/formats/bsa/tes4_bsa_prepare.cpp:171-225] [VERIFIED: src/formats/bsa/tes4_bsa_prepare.hpp:16-31] [VERIFIED: src/formats/ba2/ba2_gnrl_prepare.cpp:51-77,239-267] [VERIFIED: src/formats/ba2/ba2_gnrl_prepare.hpp:17-34] TES3 still sizes and later streams disk sources from raw `std::string`/narrow `ifstream`, and all four public writer output paths currently convert public UTF-8 text via `std::filesystem::path{output_host_path}` before publish validation. [VERIFIED: src/formats/bsa/tes3_bsa_prepare.cpp:31-42,104-110] [VERIFIED: src/formats/bsa/tes3_bsa_serialize.cpp:71-104,201-213] [VERIFIED: src/formats/bsa/tes3_bsa_writer.cpp:81-111] [VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp:91-145] [VERIFIED: src/formats/ba2/ba2_gnrl_writer.cpp:107-147] [VERIFIED: src/formats/ba2/ba2_dx10_writer.cpp:90-128]

**Primary recommendation:** resolve public writer output paths once with `detail::resolve_host_file_path`, pass `resolved.resolved` to `publish_writer_output`, and migrate TES3 disk-source prepared state to carry `detail::host_file_path` into serialization.

## Project Constraints (from AGENTS.md)

- Implementation must stay C++20, Windows/MSVC/vcpkg oriented, and avoid speculative dependencies. [VERIFIED: AGENTS.md:16-18,35-53]
- `TES5Edit/` is read-only and must not be edited, formatted, staged, or compiled into libbsa. [VERIFIED: AGENTS.md:20-33]
- Public headers should remain minimal; internal platform/path details should not leak into the public API. [VERIFIED: .planning/PROJECT.md:77-87]
- Tests should be focused archive parsing/writing/round-trip/compatibility coverage and must not use `TES5Edit/` as a mutable fixture. [VERIFIED: AGENTS.md:63-68]

## Existing Host Path Helper Contract

| Helper | Current role | Expected use in this task |
|---|---|---|
| `detail::resolve_host_file_path(std::string_view)` | Decodes public UTF-8 host text once, rejects malformed UTF-8, stores original bytes and native resolved path. [VERIFIED: src/detail/host_file_path.cpp:16-42] | Call at writer host-boundary functions after existing empty/NUL validation and before filesystem validation/publish. |
| `detail::host_file_path` | Carries `original_utf8` and `resolved` together. [VERIFIED: src/detail/host_file_path.hpp:11-18] | Add to TES3 prepared disk entries, analogous to TES4/BA2 GNRL prepared structs. |
| `detail::inspect_host_file_size(const host_file_path&)` | Sizes resolved host files without reopening from raw caller text. [VERIFIED: src/detail/host_file.hpp:36-40] [VERIFIED: src/detail/host_file.cpp:80-96] | Replace TES3 `std::filesystem::path{host_path}` sizing. |
| `detail::open_host_file(const host_file_path&)` / `for_each_host_file_chunk(const host_file_path&)` | Open/stream through resolved paths while caller-owned diagnostics stay format-specific. [VERIFIED: src/detail/host_file.hpp:30-34,76-81] [VERIFIED: src/detail/host_file.cpp:76-78,229-235] | Replace TES3 raw `std::ifstream input{host_path,...}` in serializer or use `for_each_host_file_chunk`. |
| `detail::publish_writer_output(const std::filesystem::path&)` | Performs prepublish validation, temp directory reservation, temp write, publish, rollback/cleanup. [VERIFIED: src/detail/writer_publish.hpp:25-59] | Keep this helper unchanged; only feed it a UTF-8-resolved native path. |

## TES3 Source Finalization Flow

Current TES3 flow: public `add_file()` stores raw `entry.host_path`; `tes3_prepare_entries()` calls `disk_payload_size(entry.host_path)`; `disk_payload_size()` constructs a narrow `std::filesystem::path` and calls `is_regular_file`/`file_size`; prepared entries copy `host_path`; `tes3_write_archive_bytes()` streams disk payloads through `std::ifstream input{host_path, std::ios::binary}` and checks for appended bytes after expected payload size. [VERIFIED: src/formats/bsa/tes3_bsa_writer.cpp:37-50] [VERIFIED: src/formats/bsa/tes3_bsa_prepare.cpp:31-42,88-114] [VERIFIED: src/formats/bsa/tes3_bsa_serialize.cpp:71-104,201-213]

Recommended implementation shape:

1. Include `<detail/host_file.hpp>` in `tes3_bsa_prepare.cpp` and `tes3_bsa_serialize.cpp`, and include `<detail/host_file_path.hpp>` in `tes3_bsa_prepare.hpp`. [VERIFIED: existing TES4/BA2 GNRL pattern in src/formats/bsa/tes4_bsa_prepare.hpp:5 and src/formats/ba2/ba2_gnrl_prepare.hpp:5]
2. Add `detail::host_file_path resolved_host_path;` to `tes3_prepared_entry`, keeping `std::string host_path` only if diagnostics/source attribution still need caller text. [VERIFIED: analogous comments/fields in src/formats/bsa/tes4_bsa_prepare.hpp:28-30 and src/formats/ba2/ba2_gnrl_prepare.hpp:20-22]
3. Replace `disk_payload_size(const std::string&)` with a path-resolving helper that calls `detail::resolve_host_file_path`, then `detail::inspect_host_file_size(resolved, context)`, and returns both resolved path and checked `uint32_t` size or resolves once in `tes3_prepare_entries()` and passes the resolved value to sizing. [VERIFIED: src/formats/bsa/tes3_bsa_prepare.cpp:31-42] [VERIFIED: src/formats/bsa/tes4_bsa_prepare.cpp:216-225]
4. During non-memory preparation, store the resolved path in the prepared entry before layout/serialization. [VERIFIED: src/formats/bsa/tes3_bsa_prepare.cpp:104-110]
5. Change `stream_disk_payload_to_output` to accept `const detail::host_file_path&` and stream through `detail::open_host_file` or `detail::for_each_host_file_chunk`; preserve the existing appended-byte/change detection diagnostic (`"TES3 BSA disk source changed during finalization"`). [VERIFIED: src/formats/bsa/tes3_bsa_serialize.cpp:71-104]

## Writer Output Path Audit

All public writer output functions currently validate empty paths and then construct a narrow `std::filesystem::path` directly from `std::string_view output_host_path`; these are the analogous writer publish gaps. [VERIFIED: grep `std::filesystem::path{output_host_path}` in src/formats]

| Writer | Direct conversion site | Fix |
|---|---|---|
| TES3 BSA | `write_tes3_bsa_archive`: `const auto output_path = std::filesystem::path{output_host_path};` [VERIFIED: src/formats/bsa/tes3_bsa_writer.cpp:81-111] | After `tes3_validate_host_path`, call `detail::resolve_host_file_path(output_host_path)` and pass `.resolved` to `publish_writer_output`. |
| TES4 BSA | `write_tes4_bsa_archive`: direct conversion after empty/worker validation. [VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp:91-145] | Resolve once before `publish_writer_output`; preserve existing empty-path and worker-count diagnostics. |
| BA2 GNRL | `write_ba2_gnrl_archive`: direct conversion after empty-path and target option validation. [VERIFIED: src/formats/ba2/ba2_gnrl_writer.cpp:107-147] | Resolve once before `publish_writer_output`; validate target/options in same order unless tests lock otherwise. |
| BA2 DX10 | `write_ba2_dx10_archive`: direct conversion after empty-path and target option validation. [VERIFIED: src/formats/ba2/ba2_dx10_writer.cpp:90-128] | Resolve once before `publish_writer_output`; use `.resolved` for temp filename/parent computation. |

Do **not** change `writer_publish` to accept public `std::string_view`; it is already an internal publish primitive over native `std::filesystem::path` and should remain responsible only for publish semantics, not UTF-8 decoding. [VERIFIED: src/detail/writer_publish.hpp:11-59]

## Focused Validation Strategy

1. Add source-policy assertions to `host_file_writer_name_tests.cpp` so TES3 prepare/serialize are included in the host-file seam checks and all four writer files no longer contain `std::filesystem::path{output_host_path}`. [VERIFIED: tests/unit/host_file_writer_name_tests.cpp:69-97]
2. Add a TES3 writer regression mirroring TES4/BA2 GNRL non-ASCII source helpers: create source under a native wide non-ASCII directory, convert it to explicit UTF-8 with `path.u8string()`, call public `tes3_bsa_writer::add_file`, write output to a native wide non-ASCII destination converted to UTF-8, then read the produced archive bytes or reopen through `archive_reader::open`. [VERIFIED: tests/unit/tes3_bsa_writer_tests.cpp:25-45] [VERIFIED: tests/unit/tes4_bsa_writer_tests.cpp:21-45]
3. Add one writer output-path regression per writer family if runtime cost stays low: memory-backed TES3/TES4/BA2 GNRL outputs can avoid source-file setup; BA2 DX10 can reuse existing generated DDS source and write only to a non-ASCII output path. [VERIFIED: tests/unit/tes4_bsa_writer_tests.cpp:35-45] [VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp:26-46] [VERIFIED: tests/unit/ba2_dx10_writer_tests.cpp:27-53]
4. Use existing CTest/Catch2 wiring; `libbsa_tests` already includes `host_file_path_tests`, `host_file_writer_name_tests`, `host_path_correctness_boundary_tests`, and writer tests. [VERIFIED: tests/CMakeLists.txt:65-113]

Suggested quick commands:

```powershell
ctest --test-dir build/windows-msvc-debug-static --output-on-failure -R "host_file|tes3_bsa_writer|tes4_bsa_writer|ba2_gnrl_writer|ba2_dx10_writer"
```

[ASSUMED] The build directory/preset name above matches the local developer machine; if not, use the currently configured Windows MSVC debug-static build directory.

## Pitfalls

- Resolving at `add_file()` would change timing of filesystem/UTF-8 errors for disk sources; existing migrated patterns resolve during preparation/finalization rather than at add-time for TES4/BA2 GNRL. [VERIFIED: src/formats/bsa/tes4_bsa_writer.cpp:43-58] [VERIFIED: src/formats/bsa/tes4_bsa_prepare.cpp:176-225]
- Reconstructing a path later from `host_file_path.original_utf8` reintroduces the same bug; later I/O should use `.resolved` or host-file helper overloads. [VERIFIED: src/detail/host_file_path.hpp:11-18] [VERIFIED: src/detail/host_file.cpp:76-78,94-96]
- `std::filesystem::path::string()` in tests is unsafe for non-ASCII Windows public API inputs; existing tests convert native paths with `u8string()` and a byte-preserving `std::string` wrapper. [VERIFIED: tests/unit/host_path_correctness_boundary_tests.cpp:145-150] [VERIFIED: tests/unit/host_file_path_tests.cpp:38-40]
- BA2 DX10 snapshots are internal temp paths produced by native filesystem APIs and do not need public UTF-8 resolution; the public DDS source read already calls `resolve_host_file_path(dds_host_path)`. [VERIFIED: src/formats/ba2/ba2_dx10_prepare.cpp:78-84,107-142,297-337]

## Open Questions

1. Should output-path malformed UTF-8 errors use generic `"archive path is not valid UTF-8"` from `resolve_host_file_path`, or format-specific output diagnostics? Existing helper currently returns generic text, and reader/open already uses it. [VERIFIED: src/detail/host_file_path.cpp:31-40] [VERIFIED: src/archive.cpp:165]
2. Should `tes3_prepared_entry::host_path` remain after adding `resolved_host_path`? It is not needed for I/O if serialization accepts `host_file_path`, but retaining it may reduce churn if any diagnostics or tests inspect prepared state. [VERIFIED: src/formats/bsa/tes3_bsa_prepare.hpp:14-22]

## Sources

- `AGENTS.md` and `.planning/PROJECT.md` — project constraints and current host-path decision. [VERIFIED: AGENTS.md] [VERIFIED: .planning/PROJECT.md]
- `src/detail/host_file_path.*`, `src/detail/host_file.*`, `src/detail/writer_publish.*` — internal path/helper contracts. [VERIFIED: codebase read]
- `src/formats/bsa/*writer/prepare/serialize*`, `src/formats/ba2/*writer/prepare/serialize*` — writer source/output path flows. [VERIFIED: codebase read/grep]
- `tests/unit/host_file*_tests.cpp`, `tests/unit/host_path_correctness_boundary_tests.cpp`, writer test files, `tests/CMakeLists.txt` — validation patterns. [VERIFIED: codebase read/grep]

## Assumptions Log

| # | Claim | Risk if Wrong |
|---|---|---|
| A1 | The suggested CTest build directory name matches the local build tree. | Command may need directory/preset adjustment; implementation and test design are unaffected. |
