# Codebase Concerns

**Analysis Date:** 2026-05-11

## Tech Debt

**Parser allocation and exception translation are inconsistent:**
- Issue: Parser hot paths reserve and grow archive-controlled containers without uniformly translating `std::bad_alloc`, `std::length_error`, or `std::unordered_set` allocation failures into `libbsa::result` errors. Byte-buffer helpers use safe wrappers in `src/detail/byte_vector.hpp`, but metadata vectors and hash sets call `reserve`, `push_back`, and `insert` directly.
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/detail/parser_primitives.cpp`, `src/detail/byte_vector.hpp`
- Impact: Malformed archives with very large `file_count`, `chunk_count`, or filename-table counts can surface C++ exceptions instead of stable `error_code::format_error`, which weakens the public no-throw-for-data-errors model.
- Fix approach: Add reusable `reserve_vector`, `reserve_unordered_set`, and `push_back`/`emplace` wrappers similar to `detail::reserve_byte_vector`; use them before all archive-controlled metadata allocations.

**Writer preparation has duplicated whole-file read loops:**
- Issue: Several writer paths read disk inputs byte-by-byte into `std::vector<std::byte>` before compression or texture analysis. Streaming raw disk paths exist for some uncompressed cases, but compressed BSA/BA2 paths and DX10 add-time analysis still load full source payloads.
- Files: `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/detail/compression_router.cpp`
- Impact: Large payloads can cause high memory pressure, slow byte-at-a-time reads, and duplicated buffering before codec calls. This is acceptable for current whole-buffer codecs but is the main scalability hotspot for packing.
- Fix approach: Centralize disk-source reads behind bounded chunk readers. Keep whole-buffer codec boundaries explicit, but use pre-sized reads and allocation wrappers rather than `input.get` loops.

**Format-specific extraction helpers duplicate sink/write/stream logic:**
- Issue: `write_all`, `write_in_chunks`, `checked_size`, and stream-limit checks are repeated across readers.
- Files: `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/detail/payload_stream.cpp`, `src/detail/payload_stream.hpp`
- Impact: Future fixes to partial-sink behavior, chunk sizing, or stream-limit handling must be copied into multiple files, increasing drift risk.
- Fix approach: Move common extraction helpers into `src/detail/payload_stream.cpp`/`src/detail/payload_stream.hpp`; keep only format-specific compression routing and DDS header reconstruction in format readers.

**Compatibility knowledge is encoded as scattered literals:**
- Issue: Archive magic values, record sizes, version numbers, sentinel values, compression methods, and texture constants are duplicated in parser, writer, and fixture-generator code.
- Files: `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_prepare.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`
- Impact: Parser/writer/fixture drift can silently change compatibility behavior in one path without the others.
- Fix approach: Introduce internal format-constants headers per family, such as `src/formats/ba2/ba2_constants.hpp` and `src/formats/bsa/tes4_bsa_constants.hpp`, with comments referencing compatibility evidence.

## Known Bugs

**BA2 DX10 aggregate payload sizes can overflow silently:**
- Symptoms: `raw_payload_size` and `stored_payload_size` are accumulated with unchecked `+=` across chunks and then exposed in `entry_metadata`.
- Files: `src/formats/ba2/ba2_dx10_parser.cpp`
- Trigger: A malicious DX10 archive with many chunks whose per-chunk spans fit the file size but whose total decoded or stored chunk sizes overflow `std::uint64_t` during entry materialization.
- Workaround: None in the public API; current chunk-level bounds still prevent direct out-of-file reads.

**BA2 GNRL filename table end can overflow:**
- Symptoms: `name_table_end` is computed as `file_table_offset + name_table_consumed` after parsing host-file names.
- Files: `src/formats/ba2/ba2_gnrl_parser.cpp`
- Trigger: An archive with a very high `FileTableOffset` and enough parsed name bytes to wrap the `std::uint64_t` sum.
- Workaround: Name reads are individually bounded by `archive_size`; add an `add_fits_u64` helper before using the aggregate end value.

**Parallel bulk extraction can duplicate expensive work for repeated paths:**
- Symptoms: `archive_reader::extract_entries` performs `find`, sink creation, and extraction per request without de-duplicating repeated archive paths.
- Files: `src/archive.cpp`, `src/detail/parallel_work.cpp`
- Trigger: A caller submits many duplicate `bulk_extract_request` paths with `worker_count > 1`.
- Workaround: Callers can de-duplicate requests before invoking `extract_entries`.

## Security Considerations

**Archive-controlled counts can drive memory and CPU denial of service:**
- Risk: High `file_count`, `folder_count`, or DX10 chunk counts can cause large metadata allocations, sort costs, and hash-set insertion work before the archive is rejected.
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`
- Current mitigation: Arithmetic helpers (`src/detail/parser_primitives.cpp`) validate table byte spans, and byte-vector allocation helpers (`src/detail/byte_vector.hpp`) translate some allocation failures.
- Recommendations: Add explicit maximum metadata-count policies or caller-configurable validation limits; translate all metadata allocation failures into `format_error`.

**Temporary DX10 snapshot directory names are predictable:**
- Risk: Snapshot directories use `std::filesystem::temp_directory_path()` plus `libbsa-dx10-snapshot-<counter>`, with only 1024 attempts.
- Files: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Current mitigation: `std::filesystem::create_directory` makes reservation atomic, and `ba2_dx10_writer.cpp` removes the snapshot directory best-effort.
- Recommendations: Use a cryptographically strong random suffix or Windows temp-file APIs; keep the current create-directory reservation and cleanup ownership model.

**Output publish safety depends on same-volume rename behavior:**
- Risk: Writer output is created in a temporary directory next to the destination and published with `MoveFileExW`; callers using unusual paths, reparse points, or network filesystems may see platform-specific failures.
- Files: `src/detail/writer_publish.cpp`, `src/detail/atomic_file_ops.hpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Current mitigation: Existing destinations must be regular files when overwrite is enabled, non-overwrite publish is atomic, and cleanup is isolated to a writer-owned temp directory.
- Recommendations: Document reparse-point/network-share expectations in `docs/target-format-guide.md` or writer API docs; add tests for directory targets, read-only existing files, and reparse-point refusal if supported by the CI environment.

## Performance Bottlenecks

**Compressed extraction is whole-buffer per entry or chunk:**
- Problem: Compressed BSA entries and BA2 payloads are read and decompressed into memory before being written to the sink.
- Files: `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`
- Cause: `detail::decompress_payload_exact` returns a complete `std::vector<std::byte>`, matching whole-buffer libdeflate/LZ4 APIs.
- Improvement path: Keep exact-size validation, but add streaming sink adapters for codec outputs where practical, or enforce public size limits for convenience APIs such as `archive_reader::extract_bytes`.

**DX10 writer snapshots then re-reads texture subresources:**
- Problem: `add_file` analyzes a full DDS, writes each subresource to temp snapshot files, and later `write_to` reopens those snapshot files to assemble and compress chunks.
- Files: `src/formats/ba2/ba2_dx10_prepare.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/texture/directxtex_analyzer.cpp`
- Cause: Snapshotting stabilizes mutable source-file inputs between add and finalize, but introduces extra disk I/O and temp cleanup work.
- Improvement path: Preserve snapshot semantics, but batch reads, pre-size chunk buffers, and measure snapshot overhead in `benchmarks/libbsa_benchmarks.cpp` before changing ownership.

**TES3 payload overlap validation is quadratic:**
- Problem: Each TES3 entry span is checked against every previously seen payload span.
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`
- Cause: `payload_spans` is kept in insertion order and scanned linearly for every entry.
- Improvement path: Sort payload spans by offset after materialization or maintain an ordered interval structure. Preserve the current hash-order validation before changing overlap logic.

## Fragile Areas

**BA2 DX10 chunk layout and DDS reconstruction:**
- Files: `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/texture/dds_layout.cpp`, `src/texture/directxtex_analyzer.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`, `tests/unit/dds_layout_tests.cpp`
- Why fragile: Correctness depends on `start_mip`/`end_mip`, cubemap face grouping, DDS DXT10 header reconstruction, and Starfield compression method routing all agreeing.
- Safe modification: Change one invariant at a time and add malformed cases to `tests/fixtures/generated/compatibility_matrix.json` plus unit coverage in `tests/unit/ba2_dx10_malformed_tests.cpp`.
- Test coverage: Good generated coverage exists, but broader real-game DDS and BSArchPro-derived comparison coverage is opt-in through `tests/unit/local_game_fixture_tests.cpp`.

**TES4 BSA embedded-name and compression-size prefixes:**
- Files: `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/bsa/tes4_bsa_serialize.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`
- Why fragile: Stored size flags, default-compression XOR behavior, embedded-name prefixes, and codec-specific raw-size prefixes interact in both parser and writer paths.
- Safe modification: Keep parser and writer changes paired; test zero-byte entries, embedded-name entries, compressed entries, and target-specific v103/v104/v105 cases together.
- Test coverage: Strong synthetic tests exist; local game corpus compatibility remains optional.

**Parallel execution error propagation:**
- Files: `src/detail/parallel_work.cpp`, `src/archive.cpp`, `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, `tests/unit/bulk_extraction_tests.cpp`, `tests/unit/bsa_writer_execution_tests.cpp`, `tests/unit/ba2_writer_execution_tests.cpp`
- Why fragile: Work lambdas mutate per-index vectors and shared sink factories may be caller-defined; `run_indexed_work` stops on the first infrastructure error but bulk extraction records per-entry failures.
- Safe modification: Preserve per-index ownership and never call user factories/sinks while holding internal locks. Add tests for worker creation failure only if it can be injected deterministically.
- Test coverage: Functional parallel output equivalence is covered; stress/fuzz-level concurrency coverage is limited.

## Scaling Limits

**Archive sizes above platform `size_t` are unsupported for parsing metadata:**
- Current capacity: Host-file parser entry points reject archives where `archive_size > std::numeric_limits<std::size_t>::max()`.
- Limit: 32-bit builds cannot open large archives that fit `std::uint64_t` metadata fields; Windows MSVC x64 is the intended environment.
- Scaling path: Keep Windows x64 as the supported target. Do not add 32-bit portability work unless project support changes.

**Payload size fields are 32-bit in several archive families:**
- Current capacity: TES3/TES4/BA2 record sizes are checked against `std::uint32_t` or format-specific size-flag limits.
- Limit: Individual writer entries larger than the format's record field range fail during preparation.
- Scaling path: Preserve format limits; expose clearer diagnostics in public docs for users packing large loose files.

**Worker count is hard-capped at 1024:**
- Current capacity: `detail::run_indexed_work` accepts `worker_count` from 1 to 1024.
- Limit: Higher values return `error_code::invalid_argument`; there is no `auto` worker-count selection.
- Scaling path: Keep the cap; add an explicit `auto` policy only if benchmarks prove it improves usability.

## Dependencies at Risk

**DirectXTex API and DXGI format assumptions:**
- Risk: DX10 writer and DDS analysis depend on DirectXTex metadata behavior while keeping DirectX types out of public headers.
- Impact: DirectXTex version changes can affect accepted DDS inputs, mip metadata, and DXT10 header interpretation.
- Migration plan: Keep all DirectXTex calls inside `src/texture/directxtex_analyzer.cpp`; update generated DDS fixture coverage in `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` when dependency behavior changes.

**nlohmann-json is test-only but listed as a top-level vcpkg dependency:**
- Risk: `vcpkg.json` includes `nlohmann-json` for test fixture manifests, but the library target does not link it.
- Impact: Consumers using manifest dependencies may install an unnecessary package unless tests are separated later.
- Migration plan: If packaging needs a lean runtime manifest, move test-only dependencies behind a vcpkg feature or document them as development dependencies.

## Missing Critical Features

**Mandatory compatibility corpus is synthetic by default:**
- Problem: Default tests use generated legal fixtures and writer-output archives; real game archive and BSArchPro-derived expected comparisons are opt-in.
- Blocks: Byte-level confidence against broad in-the-wild archives depends on local `LIBBSA_GAME_FIXTURES`/`LIBBSA_BSARCHPRO_EXPECTED` setup.

**No fuzzing harness is present for malformed binary inputs:**
- Problem: Malformed tests are curated fixtures and unit cases, not coverage-guided fuzzing over parser entry points.
- Blocks: Parser hardening against novel corrupted table combinations depends on manual fixture generation.

## Test Coverage Gaps

**Allocation-failure paths for metadata containers:**
- What's not tested: `reserve`, `insert`, and `push_back` failures in metadata vectors and hash sets.
- Files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`
- Risk: Exceptions can escape public APIs on adversarial counts.
- Priority: High

**Real corpus compatibility and BSArchPro comparisons:**
- What's not tested: Default CI does not compare metadata or payload hashes against a broad local game archive corpus.
- Files: `tests/unit/local_game_fixture_tests.cpp`, `docs/compatibility-evidence.md`, `tests/fixtures/README.md`
- Risk: Synthetic fixtures can miss rare Bethesda archive quirks.
- Priority: Medium

**Filesystem edge cases for writer publish and DX10 snapshots:**
- What's not tested: Reparse points, read-only targets, network shares, temp-directory collision exhaustion, and cleanup failure paths.
- Files: `src/detail/writer_publish.cpp`, `src/detail/atomic_file_ops.hpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`, `tests/unit/writer_publish_tests.cpp`
- Risk: Archive creation can fail or leave temp artifacts in unusual Windows filesystem environments.
- Priority: Medium

---

*Concerns audit: 2026-05-11*
