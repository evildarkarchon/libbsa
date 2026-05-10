# @mainpage libbsa Public API

libbsa is a reusable C++20 library for reading, extracting, validating, and writing Bethesda archive formats through dependency-light public headers.

Start with `libbsa::archive_reader` to open archives, inspect `libbsa::archive_metadata`, enumerate `libbsa::entry_metadata`, and extract entries through `libbsa::payload_sink`.

Use `libbsa::archive_reader::extract_entries`, `libbsa::bulk_extract_options`, `libbsa::bulk_extract_request`, `libbsa::bulk_extract_sink_factory`, and `libbsa::bulk_extract_entry_result` for bulk extraction with deterministic per-entry reporting.

Use `libbsa::tes3_bsa_writer`, `libbsa::tes4_bsa_writer`, `libbsa::ba2_gnrl_writer`, and `libbsa::ba2_dx10_writer` to create supported archives. `libbsa::write_execution_options` controls opt-in write-call worker scheduling while preserving serial defaults.

Use `libbsa::validate_archive`, `libbsa::validation_options`, `libbsa::validation_report`, `libbsa::validation_diagnostic`, and `libbsa::compatibility_warning` to validate archives and inspect public compatibility warnings.

See @ref thread_safety for reader, writer, validation, benchmark, callback, sink, and bulk extraction concurrency rules.

See the target-format guide for supported archive families, writer targets, compression routes, and compatibility-warning policy.
