# Graph Report - src, include, tests, tools, benchmarks  (2026-07-28)

## Corpus Check
- 209 files · ~134,048 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3840 nodes · 8549 edges · 165 communities (164 shown, 1 thin omitted)
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 149 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Health
- Final export integrity is clean: 0 dangling endpoints and 0 duplicate endpoint pairs.
- Pre-build extraction is lossy: 1,155 raw relationships had unresolved endpoints and 482 raw relationships shared an undirected endpoint pair.
- 17 JSON fixture manifests produced zero structural nodes.
- Use this graph for navigation, not as complete behavioral authority. See `.graphify_health.json` for per-scope counts.

## Graph Freshness
- Built from commit: `ecbefb98`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Host File Path
- Result Cluster
- Path Cluster
- Normalize Archive Path
- String Cluster
- Bethesda Hash Cpp
- Binary Reader
- Make Byte Vector
- Decompress Payload Exact To Sink
- Decompress Deflate Exact
- Host File Cpp
- Decompress Lz4 Block Exact
- Decompress Lz4 Frame Exact To
- Optional Cluster
- Parser Primitives Cpp
- Tes4 Bsa Header Fields
- Materialize Entries
- Reserve Metadata Set
- Payload Stream Cpp
- Payload Sink
- Payload Source
- Stored Payload Cpp
- Ba2 Archive Header
- Ba2 Archive Source
- Opened Ba2 Archive
- Ba2 Dx10 Chunk Assembler Cpp
- Ba2 Profile
- Dedupe Key
- Ba2 Dx10 Writer Entry
- Ba2 Dx10 Prepared Entry
- Ba2 Dx10 Record
- Ba2 Dx10 Write Archive Bytes
- Ba2 Dx10 Build Writer Entry
- Ba2 Dx10 Subresource Snapshot
- Ba2 Gnrl Assign Payload Offsets
- Ba2 Record Identity Cpp
- Prepare Entry
- Ba2 Gnrl Prepared Entry
- Ba2 Gnrl Write Archive Bytes
- Ba2 Gnrl Writer State
- Write Ba2 Gnrl Archive
- Ba2 Gnrl Writer Entry
- Make Tes4 Bsa Payload Descriptor
- Detected Bsa Format
- Checked U32
- Tes3 Bsa Parser Cpp
- Tes3 Prepare Entries
- Tes3 Prepared Entry
- Stream From Host
- Tes3 Write Archive Bytes
- Tes3 Writer Entry
- Write Tes3 Bsa Archive
- Tes4 Plan Placements
- Tes4 Placement Plan
- Tes4 Bsa Prepare Cpp
- Tes4 Prepared Entry
- Tes4 Bsa Profile
- Writer Entry Compression
- Write Tes4 Bsa Archive
- Extract File Payload
- Tes4 Bsa Raw Table
- Tes4 Bsa Writer Cpp
- Tes4 Writer Entry
- Dds Layout Cpp
- Dds Source Analysis
- Validation Cpp
- Archive Hpp
- Archive Metadata
- Texture Metadata
- Error Cluster Include
- Entry Metadata
- Archive Reader
- Validation Report
- Ba2 Dx10 Writer Options
- Ba2 Gnrl Writer Options
- Ba2 Dx10 Writer
- Generate Ba2 Dx10 Fixtures Cpp
- Build Archive
- Vector Cluster
- Texture Spec
- Chunk Spec
- Uint32 T
- Archive Spec
- Source Dds Spec
- Generate Malformed
- Generate Ba2 Gnrl Fixtures Cpp
- Generate Tes3 Bsa Fixtures Cpp
- Generate Tes3 Bsa Writer Fixtures
- Generate Tes4 Bsa Fixtures Cpp
- Validate Fixture Manifests Py
- Main Cpp
- Archive Reader Dispatch Policy Tests
- Archive Reader Dispatch Tests Cpp
- Archive Reader Tests Cpp
- Supported Profile Case
- Ba2 Dx10 Extraction Tests Cpp
- Ba2 Dx10 Malformed Tests Cpp
- Ba2 Dx10 Preparer Seam Tests
- Ba2 Dx10 Reader Tests Cpp
- Ba2 Dx10 Writer Tests Cpp
- Ba2 Gnrl Reader Tests Cpp
- Ba2 Gnrl Writer Tests Cpp
- Fourcc Cluster
- Ba2 Writer Execution Tests Cpp
- Benchmark Policy Tests Cpp
- Bounded Memory Policy Tests Cpp
- Bsa Writer Execution Tests Cpp
- Bulk Extraction Tests Cpp
- Compatibility Warning Tests Cpp
- Recording Sink
- Coverage Audit Matrix Docs Tests
- Format Case
- Deflate Codec Tests Cpp
- Docs Policy Tests Cpp
- Export Surface Policy Tests Cpp
- Host File Path Tests Cpp
- Host File Tests Cpp
- Writer Host Path Inventory Case
- Host Path Correctness Boundary Tests
- Local Game Fixture Tests Cpp
- Recording Sink Tests
- Parser Preparer Seam Policy Tests
- Parser Primitives Tests Cpp
- Byte Cluster
- Stored Payload Tests Cpp
- Target Format Policy Tests Cpp
- Tes3 Bsa Reader Tests Cpp
- Tes3 Bsa Writer Tests Cpp
- Tes4 Bsa Parser Seam Tests
- Tes4 Bsa Profile Ownership Policy
- Archive Policy Expectation
- Profile Expectation
- Tes4 Bsa Reader Tests Cpp
- Tes4 Bsa Writer Tests Cpp
- Find Source Root From
- Thread Safety Docs Policy Tests
- Validation Api Tests Cpp
- Validation Policy Tests Cpp
- Writer Execution Options Tests Cpp
- Writer Hotspot Policy Tests Cpp
- Writer Ownership Tests Cpp
- Writer Publish Tests Cpp
- Writer Stage Tests Cpp
- Byte Cluster Tests
- Serialization Expectation
- Main Cpp Tools
- Format Descriptor
- Path Cluster Tools
- String Cluster Tools
- Error Cluster
- Final Path Is Within Root
- Open Staged Destination
- File Sink Factory
- Libbsa Benchmarks Cpp
- Libbsa Fixture Policy

## God Nodes (most connected - your core abstractions)
1. `ba2_profile` - 49 edges
2. `host_file_path` - 46 edges
3. `tes4_bsa_profile` - 41 edges
4. `path` - 30 edges
5. `binary_reader` - 29 edges
6. `ba2_dx10_prepared_entry` - 26 edges
7. `make_byte_vector()` - 25 edges
8. `ba2_dx10_record` - 25 edges
9. `texture_spec` - 24 edges
10. `entry_spec` - 24 edges

## Surprising Connections (you probably didn't know these)
- `Committed Generated Fixtures` --semantically_similar_to--> `Synthetic Benchmark Scenarios`  [INFERRED] [semantically similar]
  tests/fixtures/README.md → benchmarks/README.md
- `Benchmark Generated Data Policy` --references--> `libbsa Benchmarks`  [EXTRACTED]
  tests/fixtures/README.md → benchmarks/README.md
- `byte_vector_allocation_error()` --calls--> `checked_buffer_size()`  [INFERRED]
  src/detail/byte_vector.hpp → src/detail/host_file.cpp
- `byte_vector_allocation_error()` --calls--> `read_exact`  [INFERRED]
  src/detail/byte_vector.hpp → src/detail/host_file.hpp
- `byte_vector_allocation_error()` --calls--> `checked_materialized_payload_size()`  [INFERRED]
  src/detail/byte_vector.hpp → src/detail/payload_stream.cpp

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Legal Synthetic Evidence Boundary** — tests::tests_fixtures_readme_committed_generated_fixtures, tests::tests_fixtures_readme_benchmark_generated_data_policy, benchmarks::benchmarks_readme_benchmark_harness, benchmarks::benchmarks_readme_synthetic_benchmark_scenarios [EXTRACTED 1.00]
- **Windows Package Consumer Proof** — tests::tests_fixtures_readme_windows_msvc_verification_matrix, tests::tests_cmakelists_package_consumer_smoke, tests::tests_package_consumer_cmakelists_libbsa_package_consumer [INFERRED 0.95]

## Communities (165 total, 1 thin omitted)

### Community 37 - "Host File Path"
Cohesion: 0.12
Nodes (12): archive_open_host_context(), read_detection_prefix(), archive_file_size(), archive_reader::open(), archive_reader, result, resolve_host_file_path(), result (+4 more)

### Community 24 - "Result Cluster"
Cohesion: 0.09
Nodes (37): archive_reader::archive_reader(), archive_metadata, archive_reader::state, metadata, vector, entry_metadata, entries, host_path (+29 more)

### Community 49 - "Path Cluster"
Cohesion: 0.17
Nodes (23): path, publish_file_without_replace(), replace_file_atomically(), prefixed_message(), string, string_view, path_exists_noexcept(), result (+15 more)

### Community 140 - "Normalize Archive Path"
Cohesion: 0.39
Nodes (7): invalid_path_error(), error, lower_ascii(), is_drive_rooted(), string_view, normalize_archive_path(), result

### Community 19 - "String Cluster"
Cohesion: 0.09
Nodes (17): archive_path_key, string, value, span, vector, string_view, payload_sink, payload_sink (+9 more)

### Community 89 - "Bethesda Hash Cpp"
Cohesion: 0.32
Nodes (15): lower_byte(), uint8_t, rotate_right(), uint32_t, extension_magic(), string_view, crc32_entry(), crc32_lookup() (+7 more)

### Community 38 - "Binary Reader"
Cohesion: 0.16
Nodes (31): truncated_error(), error, binary_reader::binary_reader(), span, byte, size_t, result, uint8_t (+23 more)

### Community 127 - "Make Byte Vector"
Cohesion: 0.36
Nodes (10): byte_vector_allocation_error(), error, make_byte_vector(), result, vector, byte, size_t, reserve_byte_vector() (+2 more)

### Community 81 - "Decompress Payload Exact To Sink"
Cohesion: 0.26
Nodes (16): copy_bytes(), result, vector, byte, span, unsupported_method_error(), error, compress_payload() (+8 more)

### Community 90 - "Decompress Deflate Exact"
Cohesion: 0.18
Nodes (13): compressor_deleter, libdeflate_compressor, decompressor_deleter, libdeflate_decompressor, codec_error(), error, compress_deflate(), result (+5 more)

### Community 20 - "Host File Cpp"
Cohesion: 0.14
Nodes (47): io_error(), error, string_view, checked_buffer_size(), result, size_t, uint64_t, validate_expected_host_file_size() (+39 more)

### Community 119 - "Decompress Lz4 Block Exact"
Cohesion: 0.38
Nodes (10): block_error(), error, checked_int_size(), result, size_t, compress_lz4_block(), vector, byte (+2 more)

### Community 74 - "Decompress Lz4 Frame Exact To"
Cohesion: 0.18
Nodes (16): frame_context_deleter, LZ4F_dctx, frame_error(), error, compress_lz4_frame(), result, vector, byte (+8 more)

### Community 128 - "Optional Cluster"
Cohesion: 0.20
Nodes (8): run_indexed_work(), result, size_t, uint32_t, function, finalization_workspace, stable_host_file_session, optional

### Community 91 - "Parser Primitives Cpp"
Cohesion: 0.21
Nodes (15): validate_metadata_count(), result, uint64_t, string_view, add_fits_u64(), span_fits_u64(), spans_overlap_u64(), read_file_bytes_at() (+7 more)

### Community 35 - "Tes4 Bsa Header Fields"
Cohesion: 0.13
Nodes (33): multiply_fits(), uint32_t, size_t, span_fits(), skip_checked(), result, size_t, read_header() (+25 more)

### Community 46 - "Materialize Entries"
Cohesion: 0.13
Nodes (25): add_fits(), stored_chunk_span, uint64_t, offset, size, compression_for(), entry_compression, record_table_size_for() (+17 more)

### Community 99 - "Reserve Metadata Set"
Cohesion: 0.20
Nodes (14): metadata_allocation_error(), error, string_view, reserve_metadata_vector(), result, vector, T, size_t (+6 more)

### Community 69 - "Payload Stream Cpp"
Cohesion: 0.30
Nodes (19): streamoff_limit(), uint64_t, streamsize_limit(), transfer_payload(), result, payload_sink, size_t, checked_payload_size() (+11 more)

### Community 152 - "Payload Source"
Cohesion: 0.50
Nodes (3): payload_source, read, remaining

### Community 17 - "Stored Payload Cpp"
Cohesion: 0.09
Nodes (43): fingerprint_bytes(), uint64_t, span, byte, snapshot_io_error(), error, string_view, stored_payload::logical_reader (+35 more)

### Community 58 - "Ba2 Archive Header"
Cohesion: 0.15
Nodes (21): read_required_u32(), result, uint32_t, string_view, read_required_u64(), uint64_t, ba2_archive_header::ba2_archive_header(), ba2_archive_metadata (+13 more)

### Community 47 - "Ba2 Archive Source"
Cohesion: 0.10
Nodes (21): ba2_archive_session_context(), ba2_stable_archive_source, result, uint64_t, vector, byte, size_t, string_view (+13 more)

### Community 143 - "Opened Ba2 Archive"
Cohesion: 0.29
Nodes (7): opened_ba2_archive, archive_metadata, metadata, vector, entry_metadata, entries, subtype

### Community 59 - "Ba2 Dx10 Chunk Assembler Cpp"
Cohesion: 0.20
Nodes (21): checked_u32(), result, uint32_t, uint64_t, string_view, checked_u16(), uint16_t, checked_u8() (+13 more)

### Community 9 - "Ba2 Profile"
Cohesion: 0.07
Nodes (55): ba2_dx10_extraction_host_context(), stream_raw_chunk(), result, ifstream, texture_chunk_metadata, payload_sink, extract_compressed_chunk(), extract_ba2_dx10_payload() (+47 more)

### Community 75 - "Dedupe Key"
Cohesion: 0.13
Nodes (16): payload_assignment, uint64_t, offset, dedupe_key, vector, byte, stored_payload, uint32_t (+8 more)

### Community 25 - "Ba2 Dx10 Writer Entry"
Cohesion: 0.07
Nodes (38): ba2_dx10_make_writer_entry(), result, string_view, ba2_dx10_target, size_t, ba2_dx10_validate_entries(), span, ba2_dx10_prepare_chunk() (+30 more)

### Community 39 - "Ba2 Dx10 Prepared Entry"
Cohesion: 0.07
Nodes (31): ba2_dx10_prepared_chunk, uint64_t, payload_offset, uint32_t, packed_size, raw_size, uint16_t, start_mip (+23 more)

### Community 50 - "Ba2 Dx10 Record"
Cohesion: 0.08
Nodes (26): ba2_dx10_chunk_record, uint64_t, offset, uint32_t, packed_size, raw_size, uint16_t, start_mip (+18 more)

### Community 64 - "Ba2 Dx10 Write Archive Bytes"
Cohesion: 0.25
Nodes (14): stream_writer, ostream, result, span, byte, uint8_t, uint16_t, uint32_t (+6 more)

### Community 76 - "Ba2 Dx10 Build Writer Entry"
Cohesion: 0.22
Nodes (17): is_starfield_only_dx10_format(), uint32_t, read_dds_file(), result, vector, byte, string_view, make_snapshot_random_suffix() (+9 more)

### Community 141 - "Ba2 Dx10 Subresource Snapshot"
Cohesion: 0.25
Nodes (8): ba2_dx10_subresource_snapshot, uint32_t, array_index, face_index, mip, uint64_t, size, snapshot_path

### Community 92 - "Ba2 Gnrl Assign Payload Offsets"
Cohesion: 0.17
Nodes (15): payload_assignment, uint64_t, offset, size_t, entry_index, ba2_gnrl_dedupe_identity, uint32_t, stored_size (+7 more)

### Community 3 - "Ba2 Record Identity Cpp"
Cohesion: 0.06
Nodes (71): gnrl_record, uint32_t, name_hash, array, byte, extension, directory_hash, unknown (+63 more)

### Community 65 - "Prepare Entry"
Cohesion: 0.17
Nodes (20): checked_u32(), result, uint32_t, uint64_t, string_view, resolve_ba2_gnrl_source_path(), archive_default_compressed(), archive_compression_policy (+12 more)

### Community 77 - "Ba2 Gnrl Prepared Entry"
Cohesion: 0.14
Nodes (17): ba2_gnrl_prepared_entry, string, array, byte, uint32_t, archive_path_original, archive_path_canonical, extension (+9 more)

### Community 66 - "Ba2 Gnrl Write Archive Bytes"
Cohesion: 0.24
Nodes (14): stream_writer, ostream, result, span, byte, uint8_t, uint16_t, uint32_t (+6 more)

### Community 144 - "Ba2 Gnrl Writer State"
Cohesion: 0.29
Nodes (7): ba2_gnrl_writer::state, ba2_gnrl_target, target, options, vector, entries, ba2_gnrl_writer::target()

### Community 114 - "Write Ba2 Gnrl Archive"
Cohesion: 0.26
Nodes (12): ba2_gnrl_writer::add_file(), result, string_view, entry_compression_policy, ba2_gnrl_entry_options, ba2_gnrl_writer::add_bytes(), span, byte (+4 more)

### Community 120 - "Ba2 Gnrl Writer Entry"
Cohesion: 0.18
Nodes (11): ba2_gnrl_writer_entry, string, archive_path_original, archive_path_canonical, host_path, vector, byte, memory_bytes (+3 more)

### Community 54 - "Make Tes4 Bsa Payload Descriptor"
Cohesion: 0.09
Nodes (22): detect_bsa_format(), result, span, byte, non_empty_span_intersects_prefix(), size_t, tes4_bsa_stored_payload_size(), result (+14 more)

### Community 48 - "Detected Bsa Format"
Cohesion: 0.11
Nodes (28): detected_bsa_format, archive_variant, variant, uint32_t, version, stored_payload_span, uint64_t, offset (+20 more)

### Community 121 - "Checked U32"
Cohesion: 0.38
Nodes (10): add_fits_u64(), uint64_t, checked_u32(), result, uint32_t, string_view, checked_add_u32(), checked_mul_u32() (+2 more)

### Community 30 - "Tes3 Bsa Parser Cpp"
Cohesion: 0.14
Nodes (35): header_fields, uint32_t, version, hash_offset_minus_header, file_count, file_record, size, raw_offset (+27 more)

### Community 93 - "Tes3 Prepare Entries"
Cohesion: 0.27
Nodes (15): preserved_archive_path(), string, string_view, checked_u32(), result, uint32_t, uint64_t, resolve_tes3_source_path() (+7 more)

### Community 103 - "Tes3 Prepared Entry"
Cohesion: 0.14
Nodes (14): tes3_prepared_entry, string, archive_path_original, vector, byte, payload, host_path, resolved_host_path (+6 more)

### Community 145 - "Stream From Host"
Cohesion: 0.52
Nodes (6): tes3_extraction_host_context(), stream_from_host(), result, entry_metadata, payload_sink, extract_tes3_bsa_payload()

### Community 94 - "Tes3 Write Archive Bytes"
Cohesion: 0.34
Nodes (15): add_fits_u64(), uint64_t, checked_u32(), result, uint32_t, string_view, checked_add_u32(), checked_mul_u32() (+7 more)

### Community 104 - "Tes3 Writer Entry"
Cohesion: 0.14
Nodes (14): tes3_bsa_writer::state, tes3_bsa_writer_options, options, vector, entries, tes3_writer_entry, string, archive_path_original (+6 more)

### Community 135 - "Write Tes3 Bsa Archive"
Cohesion: 0.33
Nodes (9): tes3_bsa_writer::add_file(), result, string_view, tes3_bsa_writer::add_bytes(), span, byte, tes3_bsa_writer::write_to(), write_execution_options (+1 more)

### Community 55 - "Tes4 Plan Placements"
Cohesion: 0.18
Nodes (21): tes4_table_lengths, uint32_t, total_folder_name_length, total_file_name_length, file_count, tes4_dedupe_identity, stored_size, uint64_t (+13 more)

### Community 21 - "Tes4 Placement Plan"
Cohesion: 0.06
Nodes (47): tes4_payload_placement, uint32_t, offset, stored_size, stored_payload, payload, tes4_placed_entry, string (+39 more)

### Community 33 - "Tes4 Bsa Prepare Cpp"
Cohesion: 0.15
Nodes (34): prepared_entry_result, entry, uint32_t, file_flags, prepared_folder_group, string, display_name, vector (+26 more)

### Community 82 - "Tes4 Prepared Entry"
Cohesion: 0.16
Nodes (16): tes4_prepared_entry, string, uint64_t, uint32_t, stored_payload, folder, canonical_folder, file_name (+8 more)

### Community 31 - "Tes4 Bsa Profile"
Cohesion: 0.10
Nodes (35): extension_for_path(), string_view, extension_is(), is_dx9_bsa_texture_format(), uint32_t, is_sse_bsa_texture_format(), tes4_bsa_profile::tes4_bsa_profile(), profile_facts (+27 more)

### Community 129 - "Writer Entry Compression"
Cohesion: 0.22
Nodes (10): archive_compression_policy, entry_compression_policy, uint64_t, tes4_writer_compression_decision, entry_compression, compression, uint32_t, record_flags (+2 more)

### Community 108 - "Write Tes4 Bsa Archive"
Cohesion: 0.22
Nodes (13): tes4_bsa_writer_options, writer_emits_embedded_names, tes4_bsa_writer::add_file(), result, string_view, entry_compression_policy, tes4_bsa_writer::add_bytes(), span (+5 more)

### Community 100 - "Extract File Payload"
Cohesion: 0.20
Nodes (14): tes4_extraction_host_context(), read_u32_le(), uint32_t, span, byte, compression_method_for(), compression_method, entry_compression (+6 more)

### Community 60 - "Tes4 Bsa Raw Table"
Cohesion: 0.11
Nodes (22): uint32_t, tes4_bsa_folder_record, uint64_t, hash, file_count, offset, tes4_bsa_file_record, hash (+14 more)

### Community 130 - "Tes4 Bsa Writer Cpp"
Cohesion: 0.27
Nodes (9): tes4_bsa_writer::state, tes4_bsa_target, target, options, vector, entries, tes4_bsa_writer::tes4_bsa_writer(), tes4_bsa_writer (+1 more)

### Community 122 - "Tes4 Writer Entry"
Cohesion: 0.18
Nodes (11): tes4_writer_entry, string, archive_path_original, archive_path_canonical, host_path, vector, byte, memory_bytes (+3 more)

### Community 10 - "Dds Layout Cpp"
Cohesion: 0.08
Nodes (60): format_error(), error, string, validate_layout_shape(), checked_add(), uint64_t, checked_mul(), dxgi_format_descriptor (+52 more)

### Community 43 - "Dds Source Analysis"
Cohesion: 0.11
Nodes (28): checked_u32(), result, uint32_t, size_t, is_supported_writer_source_format(), DXGI_FORMAT, validate_writer_source_shape(), TexMetadata (+20 more)

### Community 34 - "Validation Cpp"
Cohesion: 0.12
Nodes (31): discard_payload_sink, payload_sink, result, size_t, span, byte, diagnostic_message_for(), string (+23 more)

### Community 57 - "Archive Hpp"
Cohesion: 0.15
Nodes (12): optional, vector, string, bulk_extract_request, path, bulk_extract_entry_result, path, entry (+4 more)

### Community 87 - "Archive Metadata"
Cohesion: 0.13
Nodes (16): ba2_archive_metadata, uint32_t, starfield_unknown1, starfield_unknown2, compression_method, archive_metadata, archive_type, type (+8 more)

### Community 63 - "Texture Metadata"
Cohesion: 0.10
Nodes (21): texture_chunk_metadata, uint64_t, payload_offset, stored_size, raw_size, uint16_t, start_mip, end_mip (+13 more)

### Community 88 - "Error Cluster Include"
Cohesion: 0.16
Nodes (11): variant, error, error_code, code, string, message, result, T (+3 more)

### Community 113 - "Entry Metadata"
Cohesion: 0.17
Nodes (12): entry_metadata, path, original_path, raw_size, stored_size, payload_offset, archive_hash, compression (+4 more)

### Community 148 - "Archive Reader"
Cohesion: 0.40
Nodes (5): archive_reader, LIBBSA_API, shared_ptr, state, state_

### Community 45 - "Validation Report"
Cohesion: 0.08
Nodes (28): validation_diagnostic, error_code, code, string, message, compatibility_warning, compatibility_warning_code, code (+20 more)

### Community 98 - "Ba2 Dx10 Writer Options"
Cohesion: 0.13
Nodes (15): write_execution_options, uint32_t, worker_count, ba2_dx10_writer_options, overwrite_existing, deduplicate_payloads, max_decoded_chunk_bytes, starfield_unknown1 (+7 more)

### Community 107 - "Ba2 Gnrl Writer Options"
Cohesion: 0.15
Nodes (13): tes4_bsa_writer_options, archive_compression_policy, compression_policy, embed_file_names, deduplicate_payloads, overwrite_existing, ba2_gnrl_writer_options, compression (+5 more)

### Community 73 - "Ba2 Dx10 Writer"
Cohesion: 0.15
Nodes (18): tes4_bsa_writer, tes4_bsa_writer, LIBBSA_API, tes4_bsa_target, unique_ptr, state, state_, tes3_bsa_writer (+10 more)

### Community 67 - "Generate Ba2 Dx10 Fixtures Cpp"
Cohesion: 0.22
Nodes (20): span, string_view, string, to_hex(), json_escape(), canonicalize(), checked_u32(), write_source_case_json() (+12 more)

### Community 109 - "Build Archive"
Cohesion: 0.37
Nodes (7): byte_buffer, bytes, uint64_t, record_table_size(), write_header(), write_records(), build_archive()

### Community 115 - "Vector Cluster"
Cohesion: 0.33
Nodes (11): vector, byte, bytes(), initializer_list, repeated_bytes(), size_t, build_source_dds(), decoded_payload_bytes() (+3 more)

### Community 96 - "Texture Spec"
Cohesion: 0.12
Nodes (16): uint8_t, texture_spec, original_path, path, ext, name_hash, directory_hash, unknown_tex (+8 more)

### Community 78 - "Chunk Spec"
Cohesion: 0.12
Nodes (18): uint16_t, logical_segment, array_index, face_index, start_mip, end_mip, source_chunk_index, chunk_spec (+10 more)

### Community 146 - "Uint32 T"
Cohesion: 0.47
Nodes (6): uint32_t, mip_dimension(), bytes_per_block(), is_block_compressed(), dds_mip_size(), header_size_for()

### Community 124 - "Archive Spec"
Cohesion: 0.20
Nodes (11): archive_spec, stem, variant, version, starfield_unknown1, starfield_unknown2, compression_method, textures (+3 more)

### Community 101 - "Source Dds Spec"
Cohesion: 0.13
Nodes (15): source_dds_spec, id, file, format_id, format_name, width, height, mip_count (+7 more)

### Community 136 - "Generate Malformed"
Cohesion: 0.50
Nodes (9): make_fo4(), write_file(), path, write_text(), generate_success(), generate_writer_sources(), generate_malformed(), parse_output_dir() (+1 more)

### Community 5 - "Generate Ba2 Gnrl Fixtures Cpp"
Cohesion: 0.08
Nodes (62): byte_buffer, vector, byte, bytes, uint8_t, uint16_t, uint32_t, uint64_t (+54 more)

### Community 22 - "Generate Tes3 Bsa Fixtures Cpp"
Cohesion: 0.13
Nodes (40): byte_buffer, vector, byte, bytes, uint8_t, uint32_t, uint64_t, span (+32 more)

### Community 27 - "Generate Tes3 Bsa Writer Fixtures"
Cohesion: 0.14
Nodes (37): source_entry, string, source_kind, original_path, canonical_path, vector, byte, expected_bytes (+29 more)

### Community 4 - "Generate Tes4 Bsa Fixtures Cpp"
Cohesion: 0.07
Nodes (63): byte_buffer, vector, byte, bytes, uint8_t, uint32_t, uint64_t, span (+55 more)

### Community 83 - "Validate Fixture Manifests Py"
Cohesion: 0.24
Nodes (16): load_json(), Path, Any, require_keys(), validate_success_manifest(), validate_malformed_manifest(), find_manifest_case(), validate_compatibility_matrix() (+8 more)

### Community 6 - "Main Cpp"
Cohesion: 0.07
Nodes (61): byte_vector_sink, payload_sink, result, size_t, span, byte, vector, bytes_ (+53 more)

### Community 116 - "Archive Reader Dispatch Policy Tests"
Cohesion: 0.39
Nodes (11): source_root(), path, read_text_file(), string, production_source_text(), remove_whitespace(), string_view, remove_comments() (+3 more)

### Community 8 - "Archive Reader Dispatch Tests Cpp"
Cohesion: 0.06
Nodes (56): generated_archive_dir(), path, generated_archive_path(), string_view, read_json_file(), json, bytes_from_hex(), vector (+48 more)

### Community 71 - "Archive Reader Tests Cpp"
Cohesion: 0.14
Nodes (17): generated_archive_path(), path, string_view, collecting_sink, payload_sink, result, size_t, span (+9 more)

### Community 36 - "Supported Profile Case"
Cohesion: 0.10
Nodes (31): temporary_archive, span, byte, path, path_, append_u32_le(), vector, uint32_t (+23 more)

### Community 52 - "Ba2 Dx10 Extraction Tests Cpp"
Cohesion: 0.15
Nodes (24): generated_archive_dir(), path, generated_archive_path(), string_view, read_json_file(), json, bytes_from_hex(), vector (+16 more)

### Community 79 - "Ba2 Dx10 Malformed Tests Cpp"
Cohesion: 0.17
Nodes (16): generated_archive_dir(), path, generated_archive_path(), string_view, read_json_file(), json, error_code_from_manifest(), error_code (+8 more)

### Community 51 - "Ba2 Dx10 Preparer Seam Tests"
Cohesion: 0.15
Nodes (26): generated_source_dir(), path, seam_test_dir(), seam_output_path(), string, read_json_file(), json, read_binary_file() (+18 more)

### Community 7 - "Ba2 Dx10 Reader Tests Cpp"
Cohesion: 0.10
Nodes (62): generated_archive_dir(), path, generated_archive_path(), string_view, read_json_file(), json, expected_default_compression(), entry_compression (+54 more)

### Community 1 - "Ba2 Dx10 Writer Tests Cpp"
Cohesion: 0.06
Nodes (78): generated_source_dir(), path, read_json_file(), json, read_binary_file(), vector, byte, writer_test_dir() (+70 more)

### Community 15 - "Ba2 Gnrl Reader Tests Cpp"
Cohesion: 0.09
Nodes (49): generated_archive_dir(), path, generated_archive_path(), string_view, read_json_file(), json, expected_default_compression(), entry_compression (+41 more)

### Community 12 - "Ba2 Gnrl Writer Tests Cpp"
Cohesion: 0.07
Nodes (56): native_handle_guard, HANDLE, handle_, writer_test_dir(), path, output_path(), string, non_ascii_output_path() (+48 more)

### Community 153 - "Fourcc Cluster"
Cohesion: 0.67
Nodes (3): fourcc(), array, byte

### Community 32 - "Ba2 Writer Execution Tests Cpp"
Cohesion: 0.13
Nodes (35): writer_test_dir(), path, output_path(), string, generated_source_dir(), repeated_bytes(), vector, byte (+27 more)

### Community 137 - "Benchmark Policy Tests Cpp"
Cohesion: 0.39
Nodes (8): source_root(), path, read_text_file(), string, require_all_tokens(), string_view, initializer_list, require_no_tokens()

### Community 149 - "Bounded Memory Policy Tests Cpp"
Cohesion: 0.60
Nodes (4): source_root(), path, read_text_file(), string

### Community 56 - "Bsa Writer Execution Tests Cpp"
Cohesion: 0.21
Nodes (22): writer_test_dir(), path, output_path(), string, patterned_bytes(), vector, byte, size_t (+14 more)

### Community 0 - "Bulk Extraction Tests Cpp"
Cohesion: 0.06
Nodes (68): bulk_extraction_test_dir(), path, output_path(), string_view, bytes_from_text(), vector, byte, patterned_bytes() (+60 more)

### Community 61 - "Compatibility Warning Tests Cpp"
Cohesion: 0.23
Nodes (21): warning_test_dir(), path, source_root(), unique_output_path(), string_view, bytes_from_text(), vector, byte (+13 more)

### Community 80 - "Recording Sink"
Cohesion: 0.18
Nodes (15): router_vector(), vector, byte, read_text_file(), string, path, recording_sink, payload_sink (+7 more)

### Community 131 - "Coverage Audit Matrix Docs Tests"
Cohesion: 0.36
Nodes (9): source_root(), path, read_text_file(), string, require_all_tokens(), string_view, initializer_list, require_no_tokens() (+1 more)

### Community 97 - "Format Case"
Cohesion: 0.15
Nodes (15): read_u32_le(), uint32_t, vector, byte, size_t, chunk(), texture_chunk_metadata, uint16_t (+7 more)

### Community 147 - "Deflate Codec Tests Cpp"
Cohesion: 0.40
Nodes (5): deflate_vector(), vector, byte, impossible_byte_vector_size(), size_t

### Community 132 - "Docs Policy Tests Cpp"
Cohesion: 0.38
Nodes (9): source_root(), path, read_text_file(), string, require_all_tokens(), string_view, initializer_list, require_no_tokens() (+1 more)

### Community 150 - "Export Surface Policy Tests Cpp"
Cohesion: 0.60
Nodes (4): source_root(), path, read_text_file(), string

### Community 133 - "Host File Path Tests Cpp"
Cohesion: 0.42
Nodes (9): project_root(), path, host_file_path_source_path(), host_file_path_header_path(), read_text_file(), string, unique_non_ascii_host_path(), utf8_string_from_path() (+1 more)

### Community 70 - "Host File Tests Cpp"
Cohesion: 0.18
Nodes (18): test_context(), host_file_context, source_test_dir(), path, source_path(), string_view, unique_source_path(), bytes_from_text() (+10 more)

### Community 84 - "Writer Host Path Inventory Case"
Cohesion: 0.14
Nodes (16): source_root(), path, read_text_file(), string, writer_host_path_inventory_case, string_view, family, source_file (+8 more)

### Community 11 - "Host Path Correctness Boundary Tests"
Cohesion: 0.07
Nodes (54): project_root(), path, suite_source_path(), tests_cmake_path(), generated_archive_dir(), read_text_file(), string, contains_text() (+46 more)

### Community 28 - "Local Game Fixture Tests Cpp"
Cohesion: 0.11
Nodes (33): local_fixture_root(), optional, path, bsarchpro_expected_manifest_path(), read_json_file(), json, archive_path_from_manifest(), archive_type_from_string() (+25 more)

### Community 85 - "Recording Sink Tests"
Cohesion: 0.19
Nodes (14): lz4_vector(), vector, byte, impossible_byte_vector_size(), size_t, recording_sink, payload_sink, result (+6 more)

### Community 134 - "Parser Preparer Seam Policy Tests"
Cohesion: 0.38
Nodes (9): source_root(), path, read_text_file(), string, function_body(), string_view, require_all_tokens(), span (+1 more)

### Community 138 - "Parser Primitives Tests Cpp"
Cohesion: 0.31
Nodes (7): impossible_string_size(), size_t, impossible_vector_capacity(), impossible_set_capacity(), temp_file_cleanup, path, path_

### Community 40 - "Byte Cluster"
Cohesion: 0.15
Nodes (21): memory_source, detail::payload_source, vector, byte, result, size_t, span, bytes_ (+13 more)

### Community 53 - "Stored Payload Tests Cpp"
Cohesion: 0.14
Nodes (21): unique_test_root(), path, scoped_test_root, path, collecting_stream_buffer, streambuf, size_t, streamsize (+13 more)

### Community 139 - "Target Format Policy Tests Cpp"
Cohesion: 0.47
Nodes (8): source_root(), path, read_text_file(), string, trim_copy(), compatibility_warning_codes_from_public_header(), vector, guide_has_warning_entry()

### Community 18 - "Tes3 Bsa Reader Tests Cpp"
Cohesion: 0.10
Nodes (48): generated_archive_dir(), path, generated_archive_path(), string_view, read_json_file(), json, hex_u64_from_manifest(), uint64_t (+40 more)

### Community 14 - "Tes3 Bsa Writer Tests Cpp"
Cohesion: 0.09
Nodes (52): writer_test_dir(), path, output_path(), string, non_ascii_output_path(), string_view, utf8_string_from_path(), write_binary_file() (+44 more)

### Community 125 - "Tes4 Bsa Parser Seam Tests"
Cohesion: 0.27
Nodes (10): generated_archive_path(), path, string_view, read_binary_file(), vector, byte, read_u32_le(), uint32_t (+2 more)

### Community 26 - "Tes4 Bsa Profile Ownership Policy"
Cohesion: 0.13
Nodes (39): source_unit, path, path, string, text, code, source_range, size_t (+31 more)

### Community 123 - "Archive Policy Expectation"
Cohesion: 0.18
Nodes (10): uint32_t, texture_with_format(), texture_metadata, archive_policy_expectation, archive_compression_policy, policy, oblivion_default, later_default (+2 more)

### Community 105 - "Profile Expectation"
Cohesion: 0.14
Nodes (14): profile_expectation, version, tes4_bsa_target, target, tes4_folder_record_shape, folder_shape, size_t, folder_record_size (+6 more)

### Community 23 - "Tes4 Bsa Reader Tests Cpp"
Cohesion: 0.10
Nodes (40): generated_archive_dir(), path, generated_archive_path(), string_view, expected_default_compression(), entry_compression, uint32_t, success_fixture (+32 more)

### Community 16 - "Tes4 Bsa Writer Tests Cpp"
Cohesion: 0.08
Nodes (48): native_handle_guard, HANDLE, handle_, writer_test_dir(), path, generated_source_dir(), output_path(), string (+40 more)

### Community 151 - "Find Source Root From"
Cohesion: 0.80
Nodes (4): is_source_root(), path, find_source_root_from(), source_root()

### Community 142 - "Thread Safety Docs Policy Tests"
Cohesion: 0.39
Nodes (7): source_root(), path, read_text_file(), string, require_all_tokens(), string_view, initializer_list

### Community 13 - "Validation Api Tests Cpp"
Cohesion: 0.09
Nodes (53): validation_archive_case, string_view, file_name, archive_type, expected_type, archive_variant, expected_variant, uint32_t (+45 more)

### Community 41 - "Validation Policy Tests Cpp"
Cohesion: 0.16
Nodes (29): source_root(), path, read_text_file(), string, command_succeeds(), quoted_path(), trim_copy(), compatibility_warning_codes_from_public_header() (+21 more)

### Community 110 - "Writer Execution Options Tests Cpp"
Cohesion: 0.33
Nodes (12): writer_execution_test_dir(), path, output_path(), string_view, generated_dx10_source_path(), bytes_from_text(), vector, byte (+4 more)

### Community 111 - "Writer Hotspot Policy Tests Cpp"
Cohesion: 0.32
Nodes (12): source_root(), path, read_text_file(), string, function_body(), string_view, require_all_tokens(), span (+4 more)

### Community 117 - "Writer Ownership Tests Cpp"
Cohesion: 0.30
Nodes (11): writer_test_dir(), path, unique_output_path(), string_view, generated_source_dir(), bytes_from_text(), vector, byte (+3 more)

### Community 42 - "Writer Publish Tests Cpp"
Cohesion: 0.11
Nodes (26): path_only_publish_callback, result, path, writer_publish_test_dir(), output_path(), string, bytes_from_text(), vector (+18 more)

### Community 102 - "Writer Stage Tests Cpp"
Cohesion: 0.25
Nodes (14): stage_test_dir(), path, stage_output_path(), string, write_stage_binary_file(), span, require_gnrl_profile(), ba2_profile (+6 more)

### Community 95 - "Byte Cluster Tests"
Cohesion: 0.18
Nodes (16): bytes_from_text(), vector, byte, string_view, read_stage_binary_file(), ba2_gnrl_memory_stage_entry(), ba2_gnrl_prepared_entry, tes4_memory_stage_entry() (+8 more)

### Community 106 - "Serialization Expectation"
Cohesion: 0.15
Nodes (14): read_stage_u32_le_at(), uint32_t, size_t, layout_expectation, tes4_bsa_target, target, uint64_t, folder_block_offset (+6 more)

### Community 62 - "Main Cpp Tools"
Cohesion: 0.21
Nodes (21): string_view, valid_format_tokens(), path_from_utf8(), lower_ascii(), ascii_iequals(), windows_reserved_device_stem(), is_windows_reserved_device_name(), reject_windows_unsafe_destination_components() (+13 more)

### Community 118 - "Format Descriptor"
Cohesion: 0.17
Nodes (12): format_descriptor, token, writer_family, family, description, tes4_bsa_target, tes4_target, ba2_gnrl_target (+4 more)

### Community 86 - "Path Cluster Tools"
Cohesion: 0.26
Nodes (17): input_file, path, host_path, archive_path, path_to_utf8(), generic_utf8_path(), vector, parse_compression() (+9 more)

### Community 72 - "String Cluster Tools"
Cohesion: 0.22
Nodes (19): string, make_error(), error_code, error_code_name(), warning_code_name(), compatibility_warning_code, warning_severity_name(), compatibility_warning_severity (+11 more)

### Community 68 - "Error Cluster"
Cohesion: 0.28
Nodes (21): error, make_usage_error(), archive_type_name(), archive_type, archive_variant_name(), archive_variant, compression_name(), entry_compression (+13 more)

### Community 126 - "Final Path Is Within Root"
Cohesion: 0.24
Nodes (10): local_command_line_argv, wchar_t, value, wstring_view, wstring, is_separator(), trim_final_path(), same_windows_prefix() (+2 more)

### Community 112 - "Open Staged Destination"
Cohesion: 0.29
Nodes (11): unique_windows_handle, HANDLE, windows_error_message(), DWORD, windows_io_error(), final_path_for_handle(), handle_is_reparse_point(), mark_delete_on_close() (+3 more)

### Community 44 - "File Sink Factory"
Cohesion: 0.10
Nodes (23): staged_extraction, temp_path, final_path, overwrite, finished, shared_ptr, discard_staged(), file_payload_sink (+15 more)

### Community 2 - "Libbsa Benchmarks Cpp"
Cohesion: 0.08
Nodes (65): benchmark_result, string, scenario, uint32_t, worker_count, elapsed_ms, uint64_t, bytes_processed (+57 more)

### Community 29 - "Libbsa Fixture Policy"
Cohesion: 0.08
Nodes (36): libbsa_link_internal_test_support, libbsa_tests, generate_tes4_bsa_fixtures_tool, generate_tes3_bsa_fixtures_tool, generate_tes3_bsa_writer_fixtures_tool, generate_ba2_gnrl_fixtures_tool, generate_ba2_dx10_fixtures_tool, generate_tes4_bsa_fixtures (+28 more)

## Knowledge Gaps
- **760 isolated node(s):** `metadata`, `entries`, `host_path`, `bytes_`, `allocation_error_` (+755 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Which BA2 writer seams are exercised by both unit tests and fixture generators?**
  _The scoped graph combines writer internals, test helpers, and generated compatibility evidence._
- **Where do archive-format policies cross from public API into format-specific serialization?**
  _The src and include components expose hubs around profile, planning, and serialization concepts._
- **How do fixture provenance rules constrain benchmarks and package-consumer verification?**
  _Semantic document edges connect the fixture policy, benchmark policy, CTest labels, and package smoke tests._
