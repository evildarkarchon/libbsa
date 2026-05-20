# Integration Examples

These examples are compile-checked by `tests/package-consumer/main.cpp` through the installed `libbsa::libbsa` package target. Each snippet includes only `<libbsa/libbsa.hpp>` plus standard C++ headers. The declarations below are representative entry points; the prose calls out adjacent public APIs that a consumer should choose from in the same flow.

The installed-package `package_consumer_smoke` CTest configures these examples against the installed `libbsa::libbsa` target and, at runtime, creates writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives, then opens, validates, and extracts them through the public API. That is default package-consumer proof for the current families, not exhaustive real-game or BSArchPro corpus compatibility.

## Path model

Keep host filesystem paths and archive virtual paths separate. Host paths name archives, source files, DDS inputs, and output destinations on disk. Archive virtual paths name entries inside an archive and use libbsa lookup normalization, independent of host separator, drive, casing, or output-root policy. libbsa does not decide whether `textures/example.dds` should be written under a particular host directory; callers own that mapping.

## `example_open_list_extract`

```cpp
libbsa::result<void> example_open_list_extract(std::string_view archive_host_path,
                                               std::string_view archive_virtual_path,
                                               libbsa::payload_sink& sink);
```

Open an archive with `archive_reader::open`, inspect `metadata()` and `entries()`, confirm the requested archive virtual path with `find()`, then stream that one entry into a caller-owned `payload_sink` with `extract()`.

Use `find()` when the caller needs the `entry_metadata` or needs to distinguish invalid archive path syntax from a valid path that is absent. Use `contains()` when an existence-only `bool` is enough. Both APIs return `result` so invalid archive paths are reported with `error_code::invalid_argument` instead of being silently treated as missing.

Use `extract()` for streaming extraction, host output, or entries that should not be buffered all at once. Use `extract_bytes()` for bounded in-memory reads, such as small known payloads or tests that intentionally want a `std::vector<std::byte>` result.

## `example_bulk_extract`

```cpp
libbsa::result<std::vector<libbsa::bulk_extract_entry_result>> example_bulk_extract(
    std::string_view archive_host_path,
    std::span<const std::string> archive_virtual_paths,
    libbsa::bulk_extract_sink_factory& sink_factory);
```

Build `bulk_extract_request` records from caller-selected archive virtual paths and call `archive_reader::extract_entries` with `bulk_extract_options`. The default `worker_count` is serial (`1`); opt into parallel-capable extraction with a positive value greater than one. `worker_count == 0` is invalid and never means "auto".

The sink factory creates one sink per unique entry. When `worker_count > 1`, the factory may be called concurrently and each returned sink may be written from worker threads, so shared bookkeeping belongs behind caller-owned synchronization. The outer `result` reports setup failures such as invalid options; lookup, sink-creation, and extraction failures are recorded on request-order `bulk_extract_entry_result` values so independent entries can still complete.

## `example_create_tes3_bsa`

```cpp
libbsa::result<void> example_create_tes3_bsa(std::string_view source_host_path,
                                             std::string_view output_host_path);
```

Create a `tes3_bsa_writer`, add a disk file under an archive virtual path, and call `write_to(output_host_path, write_execution_options{})`. TES3 output is raw/uncompressed; the shared `write_execution_options` shape keeps the finalization call consistent with other writer families. Any explicit `write_execution_options::worker_count` must be positive.

## `example_create_tes4_bsa`

```cpp
libbsa::result<void> example_create_tes4_bsa(std::string_view source_host_path,
                                             std::string_view output_host_path);
```

Create a `tes4_bsa_writer` for `tes4_bsa_target::skyrim_se`, add a disk file with an `entry_compression_policy`, and finalize with `write_execution_options`. Use target profiles and writer options for compatibility behavior instead of raw archive flags. The writer stages entries by archive virtual path and publishes the final archive to a separate host filesystem path.

## `example_create_ba2_gnrl`

```cpp
libbsa::result<void> example_create_ba2_gnrl(std::string_view source_host_path,
                                             std::string_view output_host_path);
```

Create a `ba2_gnrl_writer` for `ba2_gnrl_target::starfield_v3`, choose an archive compression policy, keep `starfield_compression_method == 3` for the Starfield v3 compressed-entry route, add source files with archive virtual paths, and finalize with `write_execution_options`. Method `0` remains the explicit deflate compatibility route when that target behavior is required.

## `example_create_ba2_dx10`

```cpp
libbsa::result<void> example_create_ba2_dx10(std::string_view dds_host_path,
                                             std::string_view output_host_path);
```

Create a `ba2_dx10_writer` for a texture target and add DDS host files with archive virtual texture paths. The writer validates and snapshots DDS input at add time, then `write_to` uses target metadata to select the compressed BA2 DX10 route.

BA2 DX10 has a one-shot writer lifecycle. Do not reuse a BA2 DX10 writer after `write_to`: the write attempt consumes the writer, runs best-effort cleanup for its temporary DDS snapshots, and later `add_file` or `write_to` calls report `invalid_argument`. Ordinary success and `result`-returning failure paths clean writer-owned snapshot data, but a crash, forced termination, OS shutdown, or external temp-directory interference can still leave residual temp artifacts.

## `example_handle_result_errors`

```cpp
int example_handle_result_errors(std::string_view archive_host_path);
```

Branch on stable `result<T>::error().code` values such as `io_error`, `format_error`, and `unsupported`. Treat `error().message` as human-readable diagnostic text rather than a stable machine-readable contract; tests and application logic should not compare exact message text.

## `example_validate_archive`

```cpp
libbsa::result<libbsa::validation_report> example_validate_archive(std::string_view archive_host_path);
```

Call `validate_archive` with `validation_options` when a consumer wants a structured `validation_report`. Result-level failures describe setup errors such as invalid arguments or unreadable host paths. Inspect `validation_report::errors` for fatal archive diagnostics and `validation_report::warnings` for public `compatibility_warning` records, including stable `compatibility_warning_code` values and advisory/risky severities.
