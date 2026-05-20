# @mainpage libbsa Public API

libbsa is a reusable C++20 library for reading, extracting, validating, and writing Bethesda archive formats through dependency-light public headers. The public core is intentionally small: use `archive_reader` for existing archives, `payload_sink` or `extract_bytes` for payload access, `validate_archive` for structured diagnostics, `result<T>` for fallible calls, and the family-specific writer classes for write-new archive flows.

For the audited map from this real public surface to current support proof and routed gaps, see `docs/public-api-reality-check.md`. For archive-family support axes and evidence boundaries, see `docs/coverage-audit-matrix.md`; for compatibility-warning proof, see `docs/compatibility-evidence.md`. Those documents distinguish default repository-reproducible proof from optional local game-corpus or BSArchPro-derived checks. Optional corpus evidence may increase confidence, but it is not required for default support claims and does not replace committed fixtures, package-consumer checks, or policy tests.

The installed-package `package_consumer_smoke` default proof builds a consumer against the installed `libbsa::libbsa` target and creates, opens, validates, and extracts writer-produced TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives through the public API. This proves the current package-consumer path without local copyrighted fixtures; it is not an exhaustive real-game or BSArchPro corpus compatibility claim.

Use `libbsa::archive_reader::open` with a host filesystem path string to open an archive. After opening, inspect `libbsa::archive_metadata` through `metadata()`, enumerate `libbsa::entry_metadata` through `entries()`, and use archive virtual paths for lookup and extraction. Archive virtual paths are normalized archive keys, not host filesystem paths; callers choose their own mapping from archive paths to output directories.

Use `archive_reader::find` when you need metadata or a precise valid-missing-path result. Use `archive_reader::contains` when an existence-only boolean is enough. Invalid archive path syntax is still reported as `error_code::invalid_argument` instead of being treated as absence.

Use `archive_reader::extract` with a caller-owned `libbsa::payload_sink` for streaming extraction and host output. Use `archive_reader::extract_bytes` only when a bounded in-memory `std::vector<std::byte>` result is appropriate for the requested entry.

Use `libbsa::archive_reader::extract_entries`, `libbsa::bulk_extract_options`, `libbsa::bulk_extract_request`, `libbsa::bulk_extract_sink_factory`, and `libbsa::bulk_extract_entry_result` for bulk extraction with deterministic request-order reporting. The serial default is `worker_count == 1`; any opt-in worker count must be positive, and `0` is invalid rather than "auto". Inspect each `bulk_extract_entry_result` because lookup, sink creation, and extraction failures are reported per entry when the outer setup succeeds.

Use `libbsa::tes3_bsa_writer`, `libbsa::tes4_bsa_writer`, `libbsa::ba2_gnrl_writer`, and `libbsa::ba2_dx10_writer` to create supported archives. Add entries with explicit archive virtual paths and finalize to a host filesystem path using `write_to`. `libbsa::write_execution_options` controls opt-in write-call worker scheduling while preserving serial defaults, and `worker_count` must be positive.

BA2 DX10 texture writers have a one-shot writer lifecycle: DDS host files are validated and snapshotted when added, and any ordinary `write_to` attempt consumes the writer so snapshot cleanup can run promptly. Do not reuse a `ba2_dx10_writer` after `write_to`; later `add_file` or `write_to` calls report `error_code::invalid_argument`.

Use `libbsa::validate_archive`, `libbsa::validation_options`, `libbsa::validation_report`, `libbsa::validation_diagnostic`, and `libbsa::compatibility_warning` to validate archives and inspect public compatibility warnings. Result-level failures describe call/setup problems such as invalid or unreadable host paths; archive problems that can be inspected are reported in `validation_report::errors`, and compatible-but-noteworthy conditions are reported in `validation_report::warnings`.

Branch programmatically on stable `result<T>::error().code` and `compatibility_warning::code` values. Treat `error().message`, validation diagnostic messages, and compatibility warning messages as human-readable diagnostic text that may change as wording improves.

See @ref thread_safety for reader, writer, validation, benchmark, callback, sink, and bulk extraction concurrency rules.

See the target-format guide for supported archive families, writer targets, compression routes, publication safety, and compatibility-warning policy.
