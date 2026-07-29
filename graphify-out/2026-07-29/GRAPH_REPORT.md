# Graph Report - libbsa  (2026-07-29)

## Corpus Check
- 207 files · ~147,550 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3755 nodes · 8747 edges · 170 communities
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 169 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `20b717da`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- ba2_dx10_writer_tests.cpp
- bulk_extraction_tests.cpp
- string
- dds_layout.cpp
- libbsa_benchmarks.cpp
- generate_tes4_bsa_fixtures.cpp
- ba2_record_identity.cpp
- generate_ba2_gnrl_fixtures.cpp
- package-consumer/main.cpp
- ba2_dx10_reader_tests.cpp
- archive_reader_dispatch_tests.cpp
- ba2_gnrl_writer_tests.cpp
- host_path_correctness_boundary_tests.cpp
- stored_payload.cpp
- ba2_gnrl_reader_tests.cpp
- tes3_bsa_reader_tests.cpp
- tes3_bsa_writer_tests.cpp
- profile_expectation
- generate_tes3_bsa_fixtures.cpp
- tes4_bsa_reader_tests.cpp
- tes4_bsa_profile_ownership_policy_tests.cpp
- error
- tes4_bsa_header_fields
- generate_tes3_bsa_writer_fixtures.cpp
- host_file.cpp
- tes3_bsa_parser.cpp
- writer_stage_tests.cpp
- host_file_path
- tes4_bsa_prepare.cpp
- ba2_writer_execution_tests.cpp
- local_game_fixture_tests.cpp
- binary_reader
- ba2_archive_opening_tests.cpp
- ba2_gnrl_writer_options
- ba2_dx10_prepared_entry
- tes4_placement_plan
- byte
- ba2_dx10_record
- ba2_profile.cpp
- tes4_plan_placements
- validation_policy_tests.cpp
- byte
- writer_publish_tests.cpp
- tes4_bsa_profile
- file_sink_factory
- collecting_stream_buffer
- ba2_dx10_extraction_tests.cpp
- write_ba2_gnrl_archive
- cli/main.cpp
- texture_metadata
- make_tes4_bsa_payload_descriptor
- writer_publish.cpp
- ba2_archive_header
- ba2_dx10_chunk_assembler.cpp
- validation.cpp
- result
- ba2_dx10_writer::state
- ba2_gnrl_placed_record
- bsa_writer_execution_tests.cpp
- compatibility_warning_tests.cpp
- validation_report
- ba2_dx10_writer_entry
- validation_api_tests.cpp
- string
- error
- tes4_bsa_target
- materialize_entries
- host_file_tests.cpp
- write_tes4_bsa_archive
- source_dds_spec
- payload_stream.cpp
- .read_exact
- validation_options
- ba2_dx10_build_writer_entry_snapshot
- ba2_gnrl_prepared_entry
- chunk_spec
- validate_fixture_manifests.py
- archive_metadata
- ba2_dx10_writer
- tes3_bsa_writer.cpp
- dedupe_key
- detected_bsa_format
- tes4_prepared_entry
- prepare_entry
- generate_ba2_dx10_fixtures.cpp
- materialize_entries
- make_byte_vector
- bethesda_hash.cpp
- ba2_gnrl_writer_entry
- tes3_prepared_entry
- generate_malformed
- texture_spec
- recording_sink
- recording_sink
- decompress_deflate_exact
- decompress_payload_exact_to_sink
- parser_primitives.cpp
- ba2_gnrl_plan_placements
- collecting_sink
- ba2_record_identity
- format_case
- open_staged_destination
- ba2_gnrl_prepare.cpp
- BA2 DX10 Target Policies
- analyze_dds_source
- writer_entry_compression
- payload_sink
- tes4_writer_entry
- build_archive
- writer_execution_options_tests.cpp
- writer_hotspot_policy_tests.cpp
- ba2_profile
- supported_profile_case
- entry_metadata
- tes4_bsa_writer_tests.cpp
- read_ba2_dx10_records
- tes4_bsa_raw_table
- vector
- archive_reader_dispatch_policy_tests.cpp
- extract_file_payload
- format_descriptor
- libbsa Library Target
- Public API Core
- archive_spec
- tes4_bsa_parser_seam_tests.cpp
- writer_ownership_tests.cpp
- Build.ps1
- normalize_archive_path
- decompress_lz4_block_exact
- coverage_audit_matrix_docs_tests.cpp
- docs_policy_tests.cpp
- host_file_path_tests.cpp
- parser_preparer_seam_policy_tests.cpp
- ba2_gnrl_entry_options
- host_file_context
- ba2_dx10_writer_options
- ba2_dx10_subresource_snapshot
- ba2_dx10_malformed_tests.cpp
- benchmark_policy_tests.cpp
- native_handle_guard
- target_format_policy_tests.cpp
- TES5Edit Read-Only Boundary
- Bulk Extraction Contract
- Internal Test Support Linkage
- Q: Implement libbsa issue #31 shared finalization boundary
- opened_ba2_archive
- collecting_sink
- thread_safety_docs_policy_tests.cpp
- tes4_bsa_constants.hpp
- Archive Entry Catalog
- host_file_writer_name_tests.cpp
- .write
- tes4_bsa_profile_tests.cpp
- ba2_archive_source
- bounded_memory_policy_tests.cpp
- payload_source
- extension_fourcc_matches
- find_source_root_from
- Synthetic Benchmark Harness
- Compatibility Warning Policy
- bulk_extract_entry_result
- ba2_record_path
- ba2_record_identity_tests.cpp
- GitHub Issue Tracker
- Optional Local Corpus Checks

## God Nodes (most connected - your core abstractions)
1. `result` - 408 edges
2. `ba2_profile` - 56 edges
3. `entry_metadata` - 55 edges
4. `host_file_path` - 54 edges
5. `tes4_bsa_profile` - 45 edges
6. `binary_reader` - 29 edges
7. `archive_metadata` - 28 edges
8. `ba2_dx10_prepared_entry` - 27 edges
9. `archive_reader` - 25 edges
10. `make_byte_vector()` - 25 edges

## Surprising Connections (you probably didn't know these)
- `libbsa Project Contract` --semantically_similar_to--> `libbsa Public Capabilities`  [INFERRED] [semantically similar]
  AGENTS.md → README.md
- `Windows MSVC Verification Matrix` --semantically_similar_to--> `Supported Windows Verification Lanes`  [INFERRED] [semantically similar]
  .github/workflows/ci.yml → README.md
- `Windows Development and Reference Contract` --semantically_similar_to--> `TES5Edit Read-Only Boundary`  [INFERRED] [semantically similar]
  CLAUDE.md → AGENTS.md
- `Windows Development and Reference Contract` --semantically_similar_to--> `Windows-Only Platform Policy`  [INFERRED] [semantically similar]
  CLAUDE.md → AGENTS.md
- `Supported Windows Verification Lanes` --semantically_similar_to--> `Windows Development and Reference Contract`  [INFERRED] [semantically similar]
  README.md → CLAUDE.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Default Public Compatibility Proof** — docs_compatibility_evidence_default_evidence_policy, docs_coverage_audit_matrix_support_truth_matrix, docs_api_mainpage_package_consumer_smoke, tests_cmakelists_package_consumer_smoke, tests_fixtures_readme_committed_generated_fixture_policy [INFERRED 0.95]
- **Supported Archive Consumer Flow** — docs_api_mainpage_public_api_core, docs_integration_examples_example_open_list_extract, docs_integration_examples_family_writer_examples, docs_target_format_guide_tes3_bsa_target, docs_target_format_guide_tes4_family_bsa_targets, docs_target_format_guide_ba2_gnrl_targets, docs_target_format_guide_ba2_dx10_targets, tests_cmakelists_package_consumer_smoke [INFERRED 0.95]
- **Ownership-Isolated Concurrency Contract** — docs_thread_safety_ownership_isolation_model, docs_thread_safety_archive_reader_concurrency, docs_thread_safety_bulk_extraction_concurrency, docs_thread_safety_writer_and_validation_concurrency [EXTRACTED 1.00]

## Communities (170 total, 0 thin omitted)

### Community 0 - "ba2_dx10_writer_tests.cpp"
Cohesion: 0.05
Nodes (85): set, dds_source_analysis, dds_bytes, image_payload_bytes, metadata, subresources, dds_source_subresource, array_index (+77 more)

### Community 1 - "bulk_extraction_tests.cpp"
Cohesion: 0.06
Nodes (69): archive_reader, state_, bulk_extract_request, path, LIBBSA_API, shared_ptr, state, build_raw_ba2_dx10_fixture() (+61 more)

### Community 2 - "string"
Cohesion: 0.07
Nodes (19): bulk_extract_sink_factory(), optional, string, vector, span, string_view, payload_sink, payload_sink (+11 more)

### Community 3 - "dds_layout.cpp"
Cohesion: 0.05
Nodes (80): append_chunks_for_ranges(), build_dds_dxt10_header(), capped_mip_ranges(), checked_add(), checked_mul(), byte, error, planned_texture_chunk (+72 more)

### Community 4 - "libbsa_benchmarks.cpp"
Cohesion: 0.08
Nodes (64): add_disk_payloads(), benchmark_result, bytes_processed, correctness_passed, elapsed_ms, scenario, worker_count, benchmark_sink_factory (+56 more)

### Community 5 - "generate_tes4_bsa_fixtures.cpp"
Cohesion: 0.07
Nodes (63): archive_spec, entries, file_flags, flags, folder, folder_hash, folder_offset, stem (+55 more)

### Community 6 - "ba2_record_identity.cpp"
Cohesion: 0.26
Nodes (23): ba2_record_identity_source, ba2_subtype, error_code, pair, string, string_view, diagnostic_code_for(), extension_fourcc_for() (+15 more)

### Community 7 - "generate_ba2_gnrl_fixtures.cpp"
Cohesion: 0.08
Nodes (62): archive_spec, compression_method, entries, starfield_unknown1, starfield_unknown2, stem, variant, version (+54 more)

### Community 8 - "package-consumer/main.cpp"
Cohesion: 0.07
Nodes (57): archive_runtime_case, archive_host_path, archive_virtual_path, expect_texture_metadata, expected_ba2_compression_method, expected_default_compression, expected_payload, expected_type (+49 more)

### Community 9 - "ba2_dx10_reader_tests.cpp"
Cohesion: 0.10
Nodes (60): append_ascii(), append_dx10_chunk_record(), append_dx10_record_for_path(), append_dx10_record_header(), append_u16_le(), append_u32_le(), append_u64_le(), append_u8() (+52 more)

### Community 10 - "archive_reader_dispatch_tests.cpp"
Cohesion: 0.06
Nodes (53): archive_format_fixture, archive_filename, expected_extract_path, expected_missing_path, invalid_archive_path, manifest_filename, name, archive_format_fixtures() (+45 more)

### Community 11 - "ba2_gnrl_writer_tests.cpp"
Cohesion: 0.07
Nodes (56): bytes_from_text(), compression_case, compression_method, expected_compression, file_name, target, version, archive_variant (+48 more)

### Community 12 - "host_path_correctness_boundary_tests.cpp"
Cohesion: 0.07
Nodes (52): bytes_from_hex(), capture_report, create_count_by_path, sink_bytes_by_path, capturing_sink, capture_, collecting_sink, bytes_ (+44 more)

### Community 13 - "stored_payload.cpp"
Cohesion: 0.08
Nodes (45): logical_reader, byte_vector_allocation_error(), error, byte, error, finalization_workspace, optional, ostream (+37 more)

### Community 14 - "ba2_gnrl_reader_tests.cpp"
Cohesion: 0.09
Nodes (47): append_ascii(), append_u16_le(), append_u32_le(), append_u64_le(), archive_original_path_from_manifest(), ba2_success_fixture, archive, manifest (+39 more)

### Community 15 - "tes3_bsa_reader_tests.cpp"
Cohesion: 0.09
Nodes (47): append_u32_le(), append_u64_le(), archive_original_path_from_manifest(), build_synthetic_tes3_archive(), bytes_from_hex(), bytes_from_text(), checked_test_u32(), collecting_sink (+39 more)

### Community 16 - "tes3_bsa_writer_tests.cpp"
Cohesion: 0.10
Nodes (49): bytes_from_hex(), bytes_from_text(), collecting_sink, bytes_, byte, json, path, payload_sink (+41 more)

### Community 17 - "profile_expectation"
Cohesion: 0.13
Nodes (15): compression_method, entry_compression, size_t, tes4_folder_record_shape, uint32_t, profile_expectation, compressed_entry_metadata, compressed_payload_method (+7 more)

### Community 18 - "generate_tes3_bsa_fixtures.cpp"
Cohesion: 0.13
Nodes (40): build_tes3_archive(), byte_buffer, bytes, bytes_from_string(), canonicalize(), checked_u32(), byte, path (+32 more)

### Community 19 - "tes4_bsa_reader_tests.cpp"
Cohesion: 0.10
Nodes (39): archive_original_path_from_manifest(), bytes_from_hex(), collecting_sink, bytes_, byte, entry_compression, error_code, json (+31 more)

### Community 20 - "tes4_bsa_profile_ownership_policy_tests.cpp"
Cohesion: 0.13
Nodes (39): branches_on_direct_tes4_identity(), code_only(), contains_integer_literal(), contains_word(), control_condition, keyword, text, control_conditions() (+31 more)

### Community 21 - "error"
Cohesion: 0.24
Nodes (7): error, code, message, error_code, string, result<void>, error_

### Community 22 - "tes4_bsa_header_fields"
Cohesion: 0.12
Nodes (32): size_t, uint32_t, multiply_fits(), span_fits(), byte, size_t, span, string (+24 more)

### Community 23 - "generate_tes3_bsa_writer_fixtures.cpp"
Cohesion: 0.13
Nodes (37): bytes_from_text(), canonicalize(), byte, path, size_t, span, string, string_view (+29 more)

### Community 24 - "host_file.cpp"
Cohesion: 0.16
Nodes (40): checked_buffer_size(), byte, error, function, ifstream, path, session_diagnostics, size_t (+32 more)

### Community 25 - "tes3_bsa_parser.cpp"
Cohesion: 0.15
Nodes (31): byte, size_t, span, string, uint32_t, uint64_t, vector, file_record (+23 more)

### Community 26 - "writer_stage_tests.cpp"
Cohesion: 0.09
Nodes (41): ba2_dx10_prepared_stage_entry(), ba2_dx10_stage_entry(), ba2_gnrl_memory_stage_entry(), bytes_from_text(), byte, finalization_workspace, pair, path (+33 more)

### Community 27 - "host_file_path"
Cohesion: 0.08
Nodes (37): extract_entry_callback, archive_file_size(), archive_open_host_context(), archive_reader::contains(), archive_reader::extract(), archive_reader::extract_bytes(), archive_reader::extract_entries(), archive_reader::find() (+29 more)

### Community 28 - "tes4_bsa_prepare.cpp"
Cohesion: 0.13
Nodes (34): append_u32_le(), checked_name_size(), checked_u32(), byte, entry_compression_policy, finalization_workspace, pair, size_t (+26 more)

### Community 29 - "ba2_writer_execution_tests.cpp"
Cohesion: 0.13
Nodes (34): add_dx10_sources(), add_gnrl_disk_sources(), bytes_from_text(), archive_variant, ba2_dx10_writer, ba2_gnrl_writer, byte, entry_compression (+26 more)

### Community 30 - "local_game_fixture_tests.cpp"
Cohesion: 0.12
Nodes (29): archive_path_from_manifest(), archive_type_from_string(), archive_variant_from_string(), bsarchpro_expected_manifest_path(), bytes_from_hex(), archive_type, archive_variant, byte (+21 more)

### Community 31 - "binary_reader"
Cohesion: 0.14
Nodes (31): binary_reader, binary_reader::binary_reader(), bytes_, can_read, position, read_bytes, read_u16_le, read_u32_le (+23 more)

### Community 32 - "ba2_archive_opening_tests.cpp"
Cohesion: 0.19
Nodes (17): append_placeholder_gnrl_record(), append_u16_le(), append_u32_le(), append_u64_le(), byte, path, size_t, span (+9 more)

### Community 33 - "ba2_gnrl_writer_options"
Cohesion: 0.15
Nodes (13): ba2_gnrl_writer_options, compression, deduplicate_payloads, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2, archive_compression_policy (+5 more)

### Community 34 - "ba2_dx10_prepared_entry"
Cohesion: 0.07
Nodes (31): ba2_dx10_prepared_chunk, compression, end_mip, owns_payload_bytes, packed_size, payload_offset, raw_size, start_mip (+23 more)

### Community 35 - "tes4_placement_plan"
Cohesion: 0.06
Nodes (47): size_t, stored_payload, string, tes4_folder_record_shape, uint32_t, uint64_t, vector, tes4_payload_placement (+39 more)

### Community 36 - "byte"
Cohesion: 0.19
Nodes (19): bytes_from_text(), collecting_sink, bytes_, byte, payload_sink, size_t, span, string_view (+11 more)

### Community 37 - "ba2_dx10_record"
Cohesion: 0.08
Nodes (26): ba2_dx10_chunk_record, end_mip, offset, packed_size, raw_size, start_mip, ba2_dx10_record, chunk_count (+18 more)

### Community 38 - "ba2_profile.cpp"
Cohesion: 0.14
Nodes (31): ba2_compressed_payload_method(), ba2_profile::ba2_profile(), compressed_payload_method, default_compression, header_size, is_dx10, is_gnrl, subtype (+23 more)

### Community 39 - "tes4_plan_placements"
Cohesion: 0.17
Nodes (21): add_fits_u64(), archive_flags_for(), calculate_table_lengths(), checked_name_size(), checked_stored_size(), checked_u32(), size_t, string_view (+13 more)

### Community 40 - "validation_policy_tests.cpp"
Cohesion: 0.15
Nodes (29): command_succeeds(), compatibility_warning_codes_from_public_header(), count_occurrences(), array, initializer_list, optional, path, size_t (+21 more)

### Community 41 - "byte"
Cohesion: 0.13
Nodes (20): detail::payload_sink, detail::payload_source, byte, path, payload_sink, size_t, span, vector (+12 more)

### Community 42 - "writer_publish_tests.cpp"
Cohesion: 0.11
Nodes (26): bytes_from_text(), byte, path, span, string, string_view, vector, gnrl_disk_entry() (+18 more)

### Community 43 - "tes4_bsa_profile"
Cohesion: 0.10
Nodes (34): profile_facts, archive_variant, compression_method, entry_compression, size_t, string_view, tes4_bsa_writer_options, tes4_folder_record_shape (+26 more)

### Community 44 - "file_sink_factory"
Cohesion: 0.09
Nodes (24): less, bulk_extract_sink_factory, byte, map, mutex, ofstream, shared_ptr, size_t (+16 more)

### Community 45 - "collecting_stream_buffer"
Cohesion: 0.11
Nodes (21): streambuf, streamsize, bytes_from_text(), collecting_stream_buffer, bytes_, byte, finalization_workspace, path (+13 more)

### Community 46 - "ba2_dx10_extraction_tests.cpp"
Cohesion: 0.15
Nodes (23): ba2_dx10_fixture, archive, manifest, bytes_from_hex(), collecting_sink, bytes_, byte, json (+15 more)

### Community 47 - "write_ba2_gnrl_archive"
Cohesion: 0.20
Nodes (12): ba2_gnrl_writer::ba2_gnrl_writer(), ba2_gnrl_writer::state, entries, options, target, ba2_gnrl_writer::target(), ba2_gnrl_target, ba2_gnrl_writer (+4 more)

### Community 48 - "cli/main.cpp"
Cohesion: 0.17
Nodes (22): add_help_argument(), add_positionals_argument(), add_thread_argument(), ascii_iequals(), compatibility_warning_code, compatibility_warning_severity, ostream, string_view (+14 more)

### Community 49 - "texture_metadata"
Cohesion: 0.09
Nodes (25): bulk_extract_options, worker_count, entry_compression, uint16_t, uint32_t, uint64_t, uint8_t, texture_chunk_metadata (+17 more)

### Community 50 - "make_tes4_bsa_payload_descriptor"
Cohesion: 0.17
Nodes (12): entry_compression, PayloadReader, size_t, uint32_t, uint64_t, make_tes4_bsa_payload_descriptor(), tes4_bsa_payload_descriptor, compression (+4 more)

### Community 51 - "writer_publish.cpp"
Cohesion: 0.20
Nodes (20): Finalize, finalization_workspace, path, size_t, string, string_view, finalization_workspace, cleanup (+12 more)

### Community 52 - "ba2_archive_header"
Cohesion: 0.14
Nodes (21): ba2_archive_metadata, compression_method, starfield_unknown1, starfield_unknown2, ba2_archive_header, ba2_archive_header::ba2_archive_header(), file_count, filename_table_offset (+13 more)

### Community 53 - "ba2_dx10_chunk_assembler.cpp"
Cohesion: 0.18
Nodes (21): append_snapshot_bytes(), ba2_dx10_assemble_chunk(), ba2_dx10_assemble_planned_entry(), ba2_dx10_chunk_snapshot_batch, aggregate_size, snapshots, checked_size_t(), checked_u16() (+13 more)

### Community 54 - "validation.cpp"
Cohesion: 0.20
Nodes (20): append_entry_warnings(), append_fatal(), append_target_family_warning(), append_warning(), ascii_lowercase(), compatibility_warning_code, compatibility_warning_severity, error (+12 more)

### Community 55 - "result"
Cohesion: 0.07
Nodes (30): T, result, storage_, path, publish_file_without_replace(), replace_file_atomically(), function, size_t (+22 more)

### Community 56 - "ba2_dx10_writer::state"
Cohesion: 0.13
Nodes (19): ba2_dx10_writer::add_file(), ba2_dx10_writer::ba2_dx10_writer(), ba2_dx10_writer::state, consumed, entries, options, snapshot_dir, target (+11 more)

### Community 57 - "ba2_gnrl_placed_record"
Cohesion: 0.06
Nodes (40): ba2_gnrl_payload_placement, offset, payload, stored_size, ba2_gnrl_placed_record, archive_path_original, directory_hash, extension (+32 more)

### Community 58 - "bsa_writer_execution_tests.cpp"
Cohesion: 0.23
Nodes (20): add_tes4_sources(), bytes_from_text(), byte, pair, path, size_t, span, string (+12 more)

### Community 59 - "compatibility_warning_tests.cpp"
Cohesion: 0.24
Nodes (20): bytes_from_text(), compatibility_warning_codes_from_public_header(), byte, compatibility_warning_code, path, size_t, string, string_view (+12 more)

### Community 60 - "validation_report"
Cohesion: 0.11
Nodes (20): compatibility_warning, archive_path, code, message, severity, compatibility_warning_code, compatibility_warning_severity, error_code (+12 more)

### Community 61 - "ba2_dx10_writer_entry"
Cohesion: 0.12
Nodes (20): ba2_dx10_make_writer_entry(), ba2_dx10_prepare_chunk(), ba2_dx10_prepare_entries(), ba2_dx10_validate_entries(), ba2_dx10_target, ba2_dx10_writer_options, path, planned_texture_chunk (+12 more)

### Community 62 - "validation_api_tests.cpp"
Cohesion: 0.08
Nodes (52): append_ascii(), append_u16_le(), append_u32_le(), append_u64_le(), bytes_from_text(), compatibility_matrix_path(), archive_type, archive_variant (+44 more)

### Community 63 - "string"
Cohesion: 0.21
Nodes (22): collect_input_files(), error_code, path, payload_sink, string, unique_ptr, error_code_name(), generic_utf8_path() (+14 more)

### Community 64 - "error"
Cohesion: 0.19
Nodes (33): ArgumentParser, compression_choice, archive_policy_from(), archive_type_name(), archive_variant_name(), command_line_arguments(), compression_name(), archive_compression_policy (+25 more)

### Community 65 - "tes4_bsa_target"
Cohesion: 0.16
Nodes (14): tes4_bsa_target, tes4_bsa_writer::target(), compressed_entry_method(), entry_compression, uint32_t, expected_target_default_compression(), expected_version(), target_expectation (+6 more)

### Community 66 - "materialize_entries"
Cohesion: 0.20
Nodes (18): add_fits(), compression_for(), entry_compression, size_t, span, string, uint32_t, uint64_t (+10 more)

### Community 67 - "host_file_tests.cpp"
Cohesion: 0.20
Nodes (16): bytes_from_text(), byte, DWORD, HANDLE, path, span, string_view, vector (+8 more)

### Community 68 - "write_tes4_bsa_archive"
Cohesion: 0.20
Nodes (12): byte, entry_compression_policy, span, string_view, tes4_bsa_writer, tes4_bsa_writer_options, uint32_t, tes4_bsa_writer::add_bytes() (+4 more)

### Community 69 - "source_dds_spec"
Cohesion: 0.12
Nodes (18): ostringstream, source_dds_spec, archive_path, array_size, depth, file, format_id, format_name (+10 more)

### Community 70 - "payload_stream.cpp"
Cohesion: 0.29
Nodes (17): checked_materialized_payload_size(), checked_payload_size(), byte, ifstream, size_t, span, string_view, uint64_t (+9 more)

### Community 71 - ".read_exact"
Cohesion: 0.24
Nodes (8): ba2_stable_archive_source, session_, byte, size_t, stable_host_file_session, string_view, uint64_t, vector

### Community 72 - "validation_options"
Cohesion: 0.25
Nodes (8): archive_type, archive_variant, uint64_t, validation_options, expected_type, expected_variant, max_extractability_entry_bytes, validate_entry_extractability

### Community 73 - "ba2_dx10_build_writer_entry_snapshot"
Cohesion: 0.20
Nodes (17): ba2_dx10_build_writer_entry_snapshot(), ba2_dx10_ensure_snapshot_directory(), ba2_dx10_validate_texture_format_for_target(), ba2_dx10_target, byte, path, size_t, span (+9 more)

### Community 74 - "ba2_gnrl_prepared_entry"
Cohesion: 0.17
Nodes (15): ba2_gnrl_prepared_entry, archive_path_canonical, archive_path_original, directory_hash, extension, name_hash, packed_size, payload (+7 more)

### Community 75 - "chunk_spec"
Cohesion: 0.12
Nodes (18): chunk_spec, compression, decoded_payload, end_mip, packed_size, payload_offset, raw_size, segment (+10 more)

### Community 76 - "validate_fixture_manifests.py"
Cohesion: 0.24
Nodes (16): Any, find_manifest_case(), load_json(), main(), Path, Validate malformed fixture case IDs, expected errors, and referenced archive fil, Return a manifest case by ID, failing if the manifest case list is malformed or, Validate the consolidated malformed matrix and its manifest/test evidence refere (+8 more)

### Community 77 - "archive_metadata"
Cohesion: 0.17
Nodes (12): archive_metadata, archive_flags, ba2, default_compression, file_count, type, variant, version (+4 more)

### Community 78 - "ba2_dx10_writer"
Cohesion: 0.16
Nodes (17): ba2_dx10_writer, ba2_dx10_target, ba2_dx10_writer, state_, ba2_gnrl_writer, ba2_gnrl_target, ba2_gnrl_writer, state_ (+9 more)

### Community 79 - "tes3_bsa_writer.cpp"
Cohesion: 0.17
Nodes (15): tes3_bsa_writer_options, overwrite_existing, byte, span, string_view, vector, tes3_bsa_writer::add_bytes(), tes3_bsa_writer::add_file() (+7 more)

### Community 80 - "dedupe_key"
Cohesion: 0.14
Nodes (15): add_fits_u64(), ba2_dx10_assign_payload_offsets(), byte, compression_method, span, uint32_t, uint64_t, vector (+7 more)

### Community 81 - "detected_bsa_format"
Cohesion: 0.14
Nodes (23): detected_bsa_format, variant, version, archive_variant, uint32_t, byte, PayloadReader, size_t (+15 more)

### Community 82 - "tes4_prepared_entry"
Cohesion: 0.16
Nodes (16): stored_payload, string, uint32_t, uint64_t, vector, tes4_prepared_entry, canonical_folder, file_hash (+8 more)

### Community 83 - "prepare_entry"
Cohesion: 0.19
Nodes (13): archive_default_compressed(), ba2_gnrl_prepare_entries(), checked_u32(), archive_compression_policy, ba2_gnrl_writer_options, entry_compression_policy, finalization_workspace, size_t (+5 more)

### Community 84 - "generate_ba2_dx10_fixtures.cpp"
Cohesion: 0.26
Nodes (16): bytes_per_block(), canonicalize(), checked_u32(), pair, string_view, uint32_t, dds_mip_size(), extension_fourcc() (+8 more)

### Community 85 - "materialize_entries"
Cohesion: 0.14
Nodes (24): compression_for(), array, byte, entry_compression, span, string, uint32_t, uint64_t (+16 more)

### Community 86 - "make_byte_vector"
Cohesion: 0.14
Nodes (21): LZ4F_dctx, append_byte_vector(), byte, size_t, span, vector, make_byte_vector(), reserve_byte_vector() (+13 more)

### Community 87 - "bethesda_hash.cpp"
Cohesion: 0.32
Nodes (15): string_view, uint32_t, uint64_t, uint8_t, crc32_entry(), crc32_lookup(), extension_magic(), hash_fo4() (+7 more)

### Community 88 - "ba2_gnrl_writer_entry"
Cohesion: 0.14
Nodes (14): ba2_gnrl_make_writer_entry(), ba2_gnrl_validate_entries(), span, string_view, ba2_gnrl_writer_entry, archive_path_canonical, archive_path_original, from_memory (+6 more)

### Community 89 - "tes3_prepared_entry"
Cohesion: 0.05
Nodes (61): add_fits_u64(), checked_add_u32(), checked_mul_u32(), checked_u32(), span, string_view, uint32_t, uint64_t (+53 more)

### Community 90 - "generate_malformed"
Cohesion: 0.27
Nodes (16): compression_route_name(), path, span, string, generate_malformed(), generate_success(), generate_writer_sources(), json_escape() (+8 more)

### Community 91 - "texture_spec"
Cohesion: 0.12
Nodes (16): uint8_t, texture_spec, array_size, chunks, cube_maps_raw, directory_hash, dxgi_format, ext (+8 more)

### Community 92 - "recording_sink"
Cohesion: 0.17
Nodes (14): byte, path, payload_sink, size_t, span, string, vector, read_text_file() (+6 more)

### Community 93 - "recording_sink"
Cohesion: 0.18
Nodes (13): byte, path, payload_sink, size_t, span, vector, impossible_byte_vector_size(), lz4_vector() (+5 more)

### Community 94 - "decompress_deflate_exact"
Cohesion: 0.19
Nodes (12): libdeflate_compressor, libdeflate_decompressor, codec_error(), compress_deflate(), compressor_deleter, byte, error, size_t (+4 more)

### Community 95 - "decompress_payload_exact_to_sink"
Cohesion: 0.30
Nodes (14): compress_payload(), copy_bytes(), byte, compression_method, error, ifstream, size_t, span (+6 more)

### Community 96 - "parser_primitives.cpp"
Cohesion: 0.21
Nodes (14): add_fits_u64(), archive_string_from_bytes(), byte, ifstream, span, string, string_view, uint64_t (+6 more)

### Community 97 - "ba2_gnrl_plan_placements"
Cohesion: 0.24
Nodes (10): add_fits_u64(), ba2_gnrl_dedupe_identity, fingerprint, stored_size, ba2_gnrl_plan_placements(), checked_u32(), string_view, uint32_t (+2 more)

### Community 98 - "collecting_sink"
Cohesion: 0.14
Nodes (13): collecting_sink, bytes_, bulk_extract_sink_factory, byte, path, payload_sink, size_t, span (+5 more)

### Community 99 - "ba2_record_identity"
Cohesion: 0.18
Nodes (13): ba2_record_identity, canonical_path, directory_hash, display_path, extension, name_hash, ba2_stored_record_identity, directory_hash (+5 more)

### Community 100 - "format_case"
Cohesion: 0.14
Nodes (14): chunk(), byte, size_t, uint16_t, uint32_t, uint64_t, vector, format_case (+6 more)

### Community 101 - "open_staged_destination"
Cohesion: 0.15
Nodes (20): clear_delete_on_close(), commit_staged(), DWORD, HANDLE, final_path_for_handle(), final_path_is_within_root(), handle_is_reparse_point(), is_separator() (+12 more)

### Community 102 - "ba2_gnrl_prepare.cpp"
Cohesion: 0.11
Nodes (21): Allocator, Hash, Key, KeyEqual, error, size_t, string_view, T (+13 more)

### Community 103 - "BA2 DX10 Target Policies"
Cohesion: 0.24
Nodes (11): TES4 BSA Profile, Archive Family Writer Examples, BA2 DX10 Snapshot Lifecycle, BA2 DX10 Target Policies, BA2 GNRL Target Policies, Deflate Compression Route, LZ4 Frame Compression Route, Raw LZ4 Block Compression Route (+3 more)

### Community 104 - "analyze_dds_source"
Cohesion: 0.26
Nodes (12): DXGI_FORMAT, analyze_dds_metadata(), analyze_dds_source(), checked_u32(), byte, size_t, span, uint32_t (+4 more)

### Community 105 - "writer_entry_compression"
Cohesion: 0.18
Nodes (11): archive_compression_policy, entry_compression_policy, uint64_t, entry_compression, uint32_t, archive_default_compressed, writer_entry_compression, tes4_bsa_writer_options (+3 more)

### Community 106 - "payload_sink"
Cohesion: 0.35
Nodes (10): payload_sink(), ba2_dx10_extraction_host_context(), ifstream, extract_ba2_dx10_payload(), extract_compressed_chunk(), stream_raw_chunk(), ba2_gnrl_extraction_host_context(), extract_ba2_gnrl_payload() (+2 more)

### Community 107 - "tes4_writer_entry"
Cohesion: 0.12
Nodes (16): vector, byte, entry_compression_policy, string, vector, tes4_bsa_writer::state, entries, options (+8 more)

### Community 108 - "build_archive"
Cohesion: 0.37
Nodes (7): build_archive(), byte_buffer, bytes, uint64_t, record_table_size(), write_header(), write_records()

### Community 109 - "writer_execution_options_tests.cpp"
Cohesion: 0.29
Nodes (12): bytes_from_text(), byte, path, span, string_view, vector, generated_dx10_source_path(), output_path() (+4 more)

### Community 110 - "writer_hotspot_policy_tests.cpp"
Cohesion: 0.27
Nodes (11): path, span, string, string_view, vector, declaration_block(), public_declaration_lines(), read_text_file() (+3 more)

### Community 111 - "ba2_profile"
Cohesion: 0.14
Nodes (10): ba2_dx10_writer_options, ba2_gnrl_writer_options, ba2_profile, compressed_method_, archive_variant, ba2_subtype, compression_method, entry_compression (+2 more)

### Community 112 - "supported_profile_case"
Cohesion: 0.17
Nodes (12): archive_variant, ba2_subtype, entry_compression, string_view, supported_profile_case, compression_method, default_compression, name (+4 more)

### Community 113 - "entry_metadata"
Cohesion: 0.13
Nodes (17): entry_metadata, archive_hash, compression, embedded_name_prefix_size, has_embedded_name, original_path, path, payload_offset (+9 more)

### Community 114 - "tes4_bsa_writer_tests.cpp"
Cohesion: 0.38
Nodes (10): path, string, generated_source_dir(), non_ascii_output_path(), non_ascii_source_dir(), output_path(), target_name(), utf8_string_from_path() (+2 more)

### Community 115 - "read_ba2_dx10_records"
Cohesion: 0.50
Nodes (3): uint32_t, vector, read_ba2_dx10_records()

### Community 116 - "tes4_bsa_raw_table"
Cohesion: 0.11
Nodes (22): size_t, string, uint32_t, uint64_t, vector, tes4_bsa_file_record, hash, offset (+14 more)

### Community 117 - "vector"
Cohesion: 0.33
Nodes (11): build_source_dds(), bytes(), byte, initializer_list, size_t, vector, decoded_payload_bytes(), overwrite_u16() (+3 more)

### Community 118 - "archive_reader_dispatch_policy_tests.cpp"
Cohesion: 0.35
Nodes (11): optional, path, string, string_view, declaration_body(), extraction_function_member_name(), production_source_text(), read_text_file() (+3 more)

### Community 119 - "extract_file_payload"
Cohesion: 0.23
Nodes (11): compression_method_for(), byte, compression_method, entry_compression, ifstream, span, uint32_t, extract_file_payload() (+3 more)

### Community 120 - "format_descriptor"
Cohesion: 0.15
Nodes (13): ba2_dx10_target, ba2_gnrl_target, find_format(), format_descriptor, ba2_dx10_target, ba2_gnrl_target, description, family (+5 more)

### Community 121 - "libbsa Library Target"
Cohesion: 0.24
Nodes (11): libbsa Project Contract, Required Compression and Texture Dependency Policy, bsa CLI Target, libbsa Library Target, libbsa Install and Export Package, bsa Command-Line Tool, libbsa Public Capabilities, CLI Integration Test (+3 more)

### Community 122 - "Public API Core"
Cohesion: 0.31
Nodes (11): Installed Package Consumer Smoke Proof, Public API Core, Default Compatibility Evidence Policy, Default Evidence Status Vocabulary, Archive Family Support Truth Matrix, Open List and Extract Example, Host and Archive Path Separation, Documentation-First API Stewardship (+3 more)

### Community 123 - "archive_spec"
Cohesion: 0.20
Nodes (11): archive_spec, compression_method, starfield_unknown1, starfield_unknown2, stem, textures, variant, version (+3 more)

### Community 124 - "tes4_bsa_parser_seam_tests.cpp"
Cohesion: 0.22
Nodes (10): byte, path, size_t, span, string_view, uint32_t, vector, generated_archive_path() (+2 more)

### Community 125 - "writer_ownership_tests.cpp"
Cohesion: 0.29
Nodes (10): bytes_from_text(), byte, path, span, string_view, vector, generated_source_dir(), require_extracted_bytes() (+2 more)

### Community 126 - "Build.ps1"
Cohesion: 0.27
Nodes (5): Assert-SafeCleanPath(), Get-SafePathForDisplay(), Invoke-CMakeBuildTarget(), Invoke-ExternalCommand(), Remove-BuildDirectory()

### Community 127 - "normalize_archive_path"
Cohesion: 0.29
Nodes (9): archive_path_key, value, error, string_view, string, invalid_path_error(), is_drive_rooted(), lower_ascii() (+1 more)

### Community 128 - "decompress_lz4_block_exact"
Cohesion: 0.40
Nodes (9): block_error(), checked_int_size(), compress_lz4_block(), byte, error, size_t, span, vector (+1 more)

### Community 129 - "coverage_audit_matrix_docs_tests.cpp"
Cohesion: 0.31
Nodes (9): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_markdown_section(), require_no_tokens() (+1 more)

### Community 130 - "docs_policy_tests.cpp"
Cohesion: 0.33
Nodes (9): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_no_planning_identifier_patterns(), require_no_tokens() (+1 more)

### Community 131 - "host_file_path_tests.cpp"
Cohesion: 0.40
Nodes (9): path, string, host_file_path_header_path(), host_file_path_source_path(), malformed_utf8_host_path(), project_root(), read_text_file(), unique_non_ascii_host_path() (+1 more)

### Community 132 - "parser_preparer_seam_policy_tests.cpp"
Cohesion: 0.31
Nodes (9): path, span, string, string_view, function_body(), read_text_file(), require_absent_tokens(), require_all_tokens() (+1 more)

### Community 133 - "ba2_gnrl_entry_options"
Cohesion: 0.16
Nodes (15): ba2_gnrl_entry_options, compression, record_flags, entry_compression_policy, optional, uint32_t, write_execution_options, worker_count (+7 more)

### Community 134 - "host_file_context"
Cohesion: 0.25
Nodes (8): host_file_context, allocation_description, changed_error, inspect_error, open_error, read_error, string_view, test_context()

### Community 135 - "ba2_dx10_writer_options"
Cohesion: 0.29
Nodes (7): ba2_dx10_writer_options, deduplicate_payloads, max_decoded_chunk_bytes, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2

### Community 136 - "ba2_dx10_subresource_snapshot"
Cohesion: 0.22
Nodes (9): ba2_dx10_subresource_snapshot, array_index, face_index, mip, size, snapshot_path, path, uint32_t (+1 more)

### Community 137 - "ba2_dx10_malformed_tests.cpp"
Cohesion: 0.36
Nodes (8): error_code, json, path, string_view, error_code_from_manifest(), generated_archive_dir(), generated_archive_path(), read_json_file()

### Community 138 - "benchmark_policy_tests.cpp"
Cohesion: 0.33
Nodes (8): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_no_tokens(), source_root()

### Community 139 - "native_handle_guard"
Cohesion: 0.47
Nodes (3): HANDLE, native_handle_guard, handle_

### Community 140 - "target_format_policy_tests.cpp"
Cohesion: 0.42
Nodes (8): compatibility_warning_codes_from_public_header(), path, string, vector, guide_has_warning_entry(), read_text_file(), source_root(), trim_copy()

### Community 141 - "TES5Edit Read-Only Boundary"
Cohesion: 0.29
Nodes (8): TES5Edit Read-Only CI Guard, Windows MSVC Verification Matrix, TES5Edit Read-Only Boundary, Windows-Only Platform Policy, Windows Development and Reference Contract, MSVC AddressSanitizer Instrumentation, Supported Windows Verification Lanes, Fixture Provenance and TES5Edit Boundary

### Community 142 - "Bulk Extraction Contract"
Cohesion: 0.29
Nodes (8): Bulk Extraction Contract, Validation Result and Report Contract, Bulk Extraction Example, Validate Archive Example, Archive Reader Concurrency Contract, Bulk Extraction Sink Concurrency Contract, Ownership-Isolated Thread Safety Model, Writer and Validation Concurrency Contract

### Community 143 - "Internal Test Support Linkage"
Cohesion: 0.25
Nodes (8): Malformed Hardening Submatrix, Archive Fixture Generator Targets, Fixture Manifest Validation Test, Internal Test Support Linkage, libbsa Tests Target, Committed Generated Fixture Policy, Fixture Manifest Contracts, CTest Fixture Label Taxonomy

### Community 144 - "Q: Implement libbsa issue #31 shared finalization boundary"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: Implement libbsa issue #31 shared finalization boundary, Source Nodes

### Community 145 - "opened_ba2_archive"
Cohesion: 0.18
Nodes (9): ba2_archive_session_context(), ba2_subtype, vector, open_ba2_archive(), opened_ba2_archive, entries, metadata, subtype (+1 more)

### Community 146 - "collecting_sink"
Cohesion: 0.29
Nodes (7): collecting_sink, bytes_, byte, payload_sink, size_t, span, vector

### Community 147 - "thread_safety_docs_policy_tests.cpp"
Cohesion: 0.32
Nodes (7): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), source_root()

### Community 148 - "tes4_bsa_constants.hpp"
Cohesion: 0.24
Nodes (7): byte, span, detect_bsa_format(), size_t, uint32_t, non_empty_span_intersects_prefix(), tes4_bsa_stored_payload_size()

### Community 149 - "Archive Entry Catalog"
Cohesion: 0.25
Nodes (8): Archive Entry Catalog, BA2 Archive Header, BA2 Archive Opening, BA2 Profile, BA2 Record Identity, Stored Payload, Glossary Vocabulary Policy, Single-Context Domain Layout

### Community 150 - "host_file_writer_name_tests.cpp"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 151 - ".write"
Cohesion: 0.33
Nodes (5): byte, payload_sink, size_t, span, discard_payload_sink

### Community 152 - "tes4_bsa_profile_tests.cpp"
Cohesion: 0.19
Nodes (8): byte, vector, deflate_vector(), archive_policy_expectation, later_default, oblivion_default, policy, archive_compression_policy

### Community 153 - "ba2_archive_source"
Cohesion: 0.20
Nodes (8): ba2_archive_source, read_exact, size, string, uint32_t, uint64_t, vector, read_ba2_dx10_names()

### Community 154 - "bounded_memory_policy_tests.cpp"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 155 - "payload_source"
Cohesion: 0.50
Nodes (3): payload_source, read, remaining

### Community 156 - "extension_fourcc_matches"
Cohesion: 0.67
Nodes (4): ascii_lower_byte(), array, byte, extension_fourcc_matches()

### Community 157 - "find_source_root_from"
Cohesion: 0.80
Nodes (4): find_source_root_from(), path, is_source_root(), source_root()

### Community 158 - "Synthetic Benchmark Harness"
Cohesion: 0.50
Nodes (4): Report-Only Performance Policy, Synthetic Benchmark Harness, libbsa Benchmark Report Target, libbsa Benchmarks Target

### Community 159 - "Compatibility Warning Policy"
Cohesion: 0.50
Nodes (4): BSA Embedded Name Compatibility Risk Evidence, Compressed Sound Payload Warning Evidence, Target Family Mismatch Warning Evidence, Compatibility Warning Policy

### Community 160 - "bulk_extract_entry_result"
Cohesion: 0.33
Nodes (5): bulk_extract_entry_result, entry, failure, path, error

### Community 161 - "ba2_record_path"
Cohesion: 0.50
Nodes (4): ba2_record_path, canonical_path, display_path, string

### Community 162 - "ba2_record_identity_tests.cpp"
Cohesion: 0.50
Nodes (3): array, byte, fourcc()

### Community 163 - "GitHub Issue Tracker"
Cohesion: 0.67
Nodes (3): GitHub Issue Tracker, Wayfinder Issue Map, Canonical Triage States

### Community 164 - "Optional Local Corpus Checks"
Cohesion: 0.67
Nodes (3): Optional Local Corpus Checks, Deferred Compatibility Gaps, Local Game Fixture Policy

## Knowledge Gaps
- **784 isolated node(s):** `scenario`, `worker_count`, `elapsed_ms`, `bytes_processed`, `correctness_passed` (+779 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `result` connect `result` to `ba2_dx10_writer_tests.cpp`, `bulk_extraction_tests.cpp`, `string`, `dds_layout.cpp`, `libbsa_benchmarks.cpp`, `ba2_record_identity.cpp`, `package-consumer/main.cpp`, `archive_reader_dispatch_tests.cpp`, `host_path_correctness_boundary_tests.cpp`, `stored_payload.cpp`, `ba2_gnrl_reader_tests.cpp`, `tes3_bsa_reader_tests.cpp`, `tes3_bsa_writer_tests.cpp`, `tes4_bsa_reader_tests.cpp`, `error`, `tes4_bsa_header_fields`, `host_file.cpp`, `tes3_bsa_parser.cpp`, `host_file_path`, `tes4_bsa_prepare.cpp`, `local_game_fixture_tests.cpp`, `binary_reader`, `ba2_archive_opening_tests.cpp`, `tes4_placement_plan`, `byte`, `ba2_profile.cpp`, `tes4_plan_placements`, `byte`, `writer_publish_tests.cpp`, `tes4_bsa_profile`, `file_sink_factory`, `ba2_dx10_extraction_tests.cpp`, `write_ba2_gnrl_archive`, `cli/main.cpp`, `make_tes4_bsa_payload_descriptor`, `writer_publish.cpp`, `ba2_archive_header`, `ba2_dx10_chunk_assembler.cpp`, `validation.cpp`, `ba2_dx10_writer::state`, `ba2_gnrl_placed_record`, `ba2_dx10_writer_entry`, `string`, `error`, `materialize_entries`, `write_tes4_bsa_archive`, `payload_stream.cpp`, `.read_exact`, `ba2_dx10_build_writer_entry_snapshot`, `archive_metadata`, `tes3_bsa_writer.cpp`, `dedupe_key`, `detected_bsa_format`, `prepare_entry`, `materialize_entries`, `make_byte_vector`, `ba2_gnrl_writer_entry`, `tes3_prepared_entry`, `recording_sink`, `recording_sink`, `decompress_deflate_exact`, `decompress_payload_exact_to_sink`, `parser_primitives.cpp`, `ba2_gnrl_plan_placements`, `collecting_sink`, `open_staged_destination`, `ba2_gnrl_prepare.cpp`, `analyze_dds_source`, `writer_entry_compression`, `payload_sink`, `ba2_profile`, `entry_metadata`, `read_ba2_dx10_records`, `extract_file_payload`, `normalize_archive_path`, `decompress_lz4_block_exact`, `ba2_gnrl_entry_options`, `opened_ba2_archive`, `collecting_sink`, `tes4_bsa_constants.hpp`, `.write`, `ba2_archive_source`?**
  _High betweenness centrality (0.291) - this node is a cross-community bridge._
- **Why does `entry_metadata` connect `entry_metadata` to `bulk_extraction_tests.cpp`, `string`, `libbsa_benchmarks.cpp`, `package-consumer/main.cpp`, `ba2_dx10_reader_tests.cpp`, `archive_reader_dispatch_tests.cpp`, `host_path_correctness_boundary_tests.cpp`, `tes3_bsa_writer_tests.cpp`, `opened_ba2_archive`, `tes3_bsa_parser.cpp`, `host_file_path`, `local_game_fixture_tests.cpp`, `bulk_extract_entry_result`, `byte`, `texture_metadata`, `validation.cpp`, `string`, `materialize_entries`, `detected_bsa_format`, `materialize_entries`, `collecting_sink`, `payload_sink`, `extract_file_payload`?**
  _High betweenness centrality (0.027) - this node is a cross-community bridge._
- **Why does `tes4_bsa_profile` connect `tes4_bsa_profile` to `string`, `tes4_plan_placements`, `writer_entry_compression`, `tes4_bsa_parser_seam_tests.cpp`, `detected_bsa_format`, `make_tes4_bsa_payload_descriptor`, `tes4_bsa_header_fields`, `tes4_bsa_profile_tests.cpp`, `writer_stage_tests.cpp`, `tes4_bsa_prepare.cpp`?**
  _High betweenness centrality (0.026) - this node is a cross-community bridge._
- **What connects `scenario`, `worker_count`, `elapsed_ms` to the rest of the system?**
  _784 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ba2_dx10_writer_tests.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.05381400208986416 - nodes in this community are weakly interconnected._
- **Should `bulk_extraction_tests.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.056134723336006415 - nodes in this community are weakly interconnected._
- **Should `string` be split into smaller, more focused modules?**
  _Cohesion score 0.06666666666666667 - nodes in this community are weakly interconnected._