# Integration Examples

These examples are compile-checked by `tests/package-consumer/main.cpp` through the installed `libbsa::libbsa` package target. Each snippet includes only `<libbsa/libbsa.hpp>` plus standard C++ headers. Host filesystem paths and archive virtual paths are kept as separate parameters so consumers can choose their own output-root policy instead of blindly joining archive names onto host paths.

## `example_open_list_extract`

```cpp
libbsa::result<void> example_open_list_extract(std::string_view archive_host_path,
                                               std::string_view archive_virtual_path,
                                               libbsa::payload_sink& sink);
```

Open an archive with `archive_reader::open`, inspect `metadata()` and `entries()`, confirm the requested archive virtual path with `find()`, then stream that one entry into a caller-owned `payload_sink` with `extract()`.

## `example_bulk_extract`

```cpp
libbsa::result<std::vector<libbsa::bulk_extract_entry_result>> example_bulk_extract(
    std::string_view archive_host_path,
    std::span<const std::string> archive_virtual_paths,
    libbsa::bulk_extract_sink_factory& sink_factory);
```

Build `bulk_extract_request` records from caller-selected archive virtual paths and call `archive_reader::extract_entries` with a positive `bulk_extract_options::worker_count`. The sink factory creates one sink per entry; callers keep host output roots outside the archive-path values.

## `example_create_tes3_bsa`

```cpp
libbsa::result<void> example_create_tes3_bsa(std::string_view source_host_path,
                                             std::string_view output_host_path);
```

Create a `tes3_bsa_writer`, add a disk file under an archive virtual path, and call `write_to(output_host_path, write_execution_options{})`. TES3 output is raw/uncompressed; the shared `write_execution_options` shape keeps the finalization call consistent with other writer families.

## `example_create_tes4_bsa`

```cpp
libbsa::result<void> example_create_tes4_bsa(std::string_view source_host_path,
                                             std::string_view output_host_path);
```

Create a `tes4_bsa_writer` for `tes4_bsa_target::skyrim_se`, add a disk file with an `entry_compression_policy`, and finalize with `write_execution_options`. Use target profiles and writer options for compatibility behavior instead of raw archive flags.

## `example_create_ba2_gnrl`

```cpp
libbsa::result<void> example_create_ba2_gnrl(std::string_view source_host_path,
                                             std::string_view output_host_path);
```

Create a `ba2_gnrl_writer` for `ba2_gnrl_target::starfield_v3`, choose an archive compression policy, keep `starfield_compression_method == 3` for raw LZ4 block routing, add source files, and finalize with `write_execution_options`.

## `example_create_ba2_dx10`

```cpp
libbsa::result<void> example_create_ba2_dx10(std::string_view dds_host_path,
                                             std::string_view output_host_path);
```

Create a `ba2_dx10_writer` for a texture target and add DDS host files with archive virtual texture paths. The writer validates and snapshots DDS input at add time, then `write_to` uses target metadata to select the compressed BA2 DX10 route.

Do not reuse a BA2 DX10 writer after `write_to`: the write attempt consumes the writer, runs best-effort cleanup for its temporary DDS snapshots, and later `add_file` or `write_to` calls report `invalid_argument`. Ordinary success and `result`-returning failure paths clean writer-owned snapshot data, but a crash, forced termination, OS shutdown, or external temp-directory interference can still leave residual temp artifacts.

## `example_handle_result_errors`

```cpp
int example_handle_result_errors(std::string_view archive_host_path);
```

Branch on stable `result.error().code` values such as `io_error`, `format_error`, and `unsupported`. Treat `error().message` as human-readable diagnostic text rather than a stable machine-readable contract.

## `example_validate_archive`

```cpp
libbsa::result<libbsa::validation_report> example_validate_archive(std::string_view archive_host_path);
```

Call `validate_archive` with `validation_options` when a consumer wants a structured `validation_report`. Result-level failures describe setup errors such as unreadable paths; inspect `validation_report::errors` and `validation_report::warnings` for archive diagnostics and compatibility warnings.
