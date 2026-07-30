# Graph Report - libbsa  (2026-07-29)

## Corpus Check
- 211 files · ~149,326 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3819 nodes · 8831 edges · 184 communities
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 169 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `5d298f78`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- ba2_dx10_writer_tests.cpp
- bulk_extraction_tests.cpp
- string
- dds_layout.cpp
- libbsa_benchmarks.cpp
- generate_tes4_bsa_fixtures.cpp
- ba2_record_identity
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
- ba2_archive_header
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
- writer_entry_compression
- file_sink_factory
- stored_payload_tests.cpp
- ba2_dx10_extraction_tests.cpp
- write_ba2_gnrl_archive
- cli/main.cpp
- texture_metadata
- tes4_bsa_folder_record
- writer_publish.cpp
- ba2_dx10_write_archive_bytes
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
- ba2_dx10_preparer_seam_tests.cpp
- source_dds_spec
- payload_stream.cpp
- ba2_archive_source
- path
- tes4_bsa_profile
- ba2_gnrl_prepared_entry
- chunk_spec
- validate_fixture_manifests.py
- ba2_gnrl_write_archive_bytes
- ba2_dx10_writer
- tes4_bsa_serialize.cpp
- ba2_dx10_plan_placements
- detected_bsa_format
- tes4_prepared_entry
- ba2_gnrl_writer_entry
- generate_ba2_dx10_fixtures.cpp
- materialize_entries
- decompress_lz4_frame_exact_to_sink
- bethesda_hash.cpp
- archive_metadata
- tes3_write_archive_bytes
- generate_malformed
- texture_spec
- recording_sink
- recording_sink
- decompress_deflate_exact
- decompress_payload_exact_to_sink
- parser_primitives.cpp
- ba2_gnrl_plan_placements
- collecting_sink
- tes3_writer_entry
- format_case
- commit_staged
- reserve_metadata_set
- BA2 DX10 Target Policies
- analyze_dds_source
- ba2_dx10_build_writer_entry_snapshot
- tes3_prepared_entry
- tes4_writer_entry
- build_archive
- writer_execution_options_tests.cpp
- writer_hotspot_policy_tests.cpp
- ba2_dx10_placed_record
- serialization_expectation
- entry_metadata
- tes4_bsa_writer_tests.cpp
- tes4_bsa_writer.cpp
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
- stable_host_file_session
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
- final_path_is_within_root
- thread_safety_docs_policy_tests.cpp
- write_tes4_bsa_archive
- Archive Entry Catalog
- read_text_file
- .write
- archive_runtime_case
- prepare_entry
- read_text_file
- payload_sink
- supported_profile_case
- find_source_root_from
- Synthetic Benchmark Harness
- Compatibility Warning Policy
- optional
- Q: How do BA2 DX10 prepared chunks compression finalization deduplication serialization work and where are Stored Payload integration points?
- ba2_dx10_placement_plan
- GitHub Issue Tracker
- Optional Local Corpus Checks
- Q: MSVC C2039/C3861: ba2_gnrl_plan_placements is not a member of libbsa::formats::ba2 in writer_stage_tests.cpp
- read_text_file
- Q: Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33
- ba2_dx10_placed_chunk
- deflate_codec_tests.cpp
- Q: Issue 33 DX10 reference compatibility audit
- checked_u32
- make_byte_vector
- ba2_profile
- parser_primitives_tests.cpp
- validation_options
- ba2_dx10_stored_header_options
- checked_u16
- payload_source

## God Nodes (most connected - your core abstractions)
1. `result` - 410 edges
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

## Communities (184 total, 0 thin omitted)

### Community 0 - "ba2_dx10_writer_tests.cpp"
Cohesion: 0.07
Nodes (73): set, add_duplicate_dds_pair(), add_matrix_cases(), ba2_dx10_record_metadata, directory_hash, extension, filename_table_path, name_hash (+65 more)

### Community 1 - "bulk_extraction_tests.cpp"
Cohesion: 0.06
Nodes (69): archive_reader, state_, bulk_extract_request, path, LIBBSA_API, shared_ptr, state, build_raw_ba2_dx10_fixture() (+61 more)

### Community 2 - "string"
Cohesion: 0.07
Nodes (18): string, vector, span, string_view, payload_sink, payload_sink, payload_sink, write (+10 more)

### Community 3 - "dds_layout.cpp"
Cohesion: 0.08
Nodes (58): append_chunks_for_ranges(), build_dds_dxt10_header(), capped_mip_ranges(), checked_add(), checked_mul(), byte, error, planned_texture_chunk (+50 more)

### Community 4 - "libbsa_benchmarks.cpp"
Cohesion: 0.08
Nodes (64): add_disk_payloads(), benchmark_result, bytes_processed, correctness_passed, elapsed_ms, scenario, worker_count, benchmark_sink_factory (+56 more)

### Community 5 - "generate_tes4_bsa_fixtures.cpp"
Cohesion: 0.07
Nodes (63): archive_spec, entries, file_flags, flags, folder, folder_hash, folder_offset, stem (+55 more)

### Community 6 - "ba2_record_identity"
Cohesion: 0.11
Nodes (44): ba2_record_identity_source, ascii_lower_byte(), ba2_record_identity, canonical_path, directory_hash, display_path, extension, name_hash (+36 more)

### Community 7 - "generate_ba2_gnrl_fixtures.cpp"
Cohesion: 0.08
Nodes (62): archive_spec, compression_method, entries, starfield_unknown1, starfield_unknown2, stem, variant, version (+54 more)

### Community 8 - "package-consumer/main.cpp"
Cohesion: 0.10
Nodes (42): build_tiny_bc1_dds_dxt10_source(), byte_buffer, bytes, byte_vector_sink, bytes_, byte_vector_sink_factory, mutex_, observed_entries_ (+34 more)

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
Nodes (43): logical_reader, byte, error, finalization_workspace, optional, ostream, size_t, span (+35 more)

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

### Community 21 - "ba2_archive_header"
Cohesion: 0.14
Nodes (21): ba2_archive_metadata, compression_method, starfield_unknown1, starfield_unknown2, ba2_archive_header, ba2_archive_header::ba2_archive_header(), file_count, filename_table_offset (+13 more)

### Community 22 - "tes4_bsa_header_fields"
Cohesion: 0.12
Nodes (32): size_t, uint32_t, multiply_fits(), span_fits(), byte, size_t, span, string (+24 more)

### Community 23 - "generate_tes3_bsa_writer_fixtures.cpp"
Cohesion: 0.13
Nodes (37): bytes_from_text(), canonicalize(), byte, path, size_t, span, string, string_view (+29 more)

### Community 24 - "host_file.cpp"
Cohesion: 0.19
Nodes (36): checked_buffer_size(), byte, error, function, ifstream, path, size_t, span (+28 more)

### Community 25 - "tes3_bsa_parser.cpp"
Cohesion: 0.15
Nodes (31): byte, size_t, span, string, uint32_t, uint64_t, vector, file_record (+23 more)

### Community 26 - "writer_stage_tests.cpp"
Cohesion: 0.17
Nodes (26): ba2_dx10_prepared_stage_entries(), ba2_dx10_stage_entry(), ba2_gnrl_memory_stage_entry(), bytes_from_text(), byte, finalization_workspace, pair, path (+18 more)

### Community 27 - "host_file_path"
Cohesion: 0.08
Nodes (37): extract_entry_callback, archive_file_size(), archive_open_host_context(), archive_reader::contains(), archive_reader::extract(), archive_reader::extract_bytes(), archive_reader::extract_entries(), archive_reader::find() (+29 more)

### Community 28 - "tes4_bsa_prepare.cpp"
Cohesion: 0.13
Nodes (34): append_u32_le(), checked_name_size(), checked_u32(), byte, entry_compression_policy, finalization_workspace, pair, size_t (+26 more)

### Community 29 - "ba2_writer_execution_tests.cpp"
Cohesion: 0.08
Nodes (47): dds_source_analysis, dds_bytes, image_payload_bytes, metadata, subresources, dds_source_subresource, array_index, bytes (+39 more)

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
Nodes (30): ba2_dx10_prepared_chunk, compression, end_mip, packed_size, payload, raw_size, start_mip, ba2_dx10_prepared_entry (+22 more)

### Community 35 - "tes4_placement_plan"
Cohesion: 0.08
Nodes (31): size_t, stored_payload, string, tes4_folder_record_shape, uint32_t, uint64_t, vector, tes4_payload_placement (+23 more)

### Community 36 - "byte"
Cohesion: 0.19
Nodes (19): bytes_from_text(), collecting_sink, bytes_, byte, payload_sink, size_t, span, string_view (+11 more)

### Community 37 - "ba2_dx10_record"
Cohesion: 0.07
Nodes (29): ba2_dx10_chunk_record, end_mip, offset, packed_size, raw_size, start_mip, ba2_dx10_record, chunk_count (+21 more)

### Community 38 - "ba2_profile.cpp"
Cohesion: 0.15
Nodes (29): ba2_compressed_payload_method(), ba2_profile::ba2_profile(), compressed_payload_method, default_compression, header_size, subtype, subtype_magic, variant (+21 more)

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

### Community 43 - "writer_entry_compression"
Cohesion: 0.18
Nodes (11): archive_compression_policy, entry_compression_policy, uint64_t, entry_compression, uint32_t, archive_default_compressed, writer_entry_compression, tes4_bsa_writer_options (+3 more)

### Community 44 - "file_sink_factory"
Cohesion: 0.11
Nodes (21): less, bulk_extract_sink_factory, map, mutex, ofstream, shared_ptr, discard_staged(), file_payload_sink (+13 more)

### Community 45 - "stored_payload_tests.cpp"
Cohesion: 0.13
Nodes (21): streambuf, streamsize, bytes_from_text(), collecting_stream_buffer, bytes_, byte, finalization_workspace, path (+13 more)

### Community 46 - "ba2_dx10_extraction_tests.cpp"
Cohesion: 0.15
Nodes (23): ba2_dx10_fixture, archive, manifest, bytes_from_hex(), collecting_sink, bytes_, byte, json (+15 more)

### Community 47 - "write_ba2_gnrl_archive"
Cohesion: 0.20
Nodes (12): ba2_gnrl_writer::ba2_gnrl_writer(), ba2_gnrl_writer::state, entries, options, target, ba2_gnrl_writer::target(), ba2_gnrl_target, ba2_gnrl_writer (+4 more)

### Community 48 - "cli/main.cpp"
Cohesion: 0.18
Nodes (24): add_help_argument(), add_positionals_argument(), add_thread_argument(), ascii_iequals(), ostream, string_view, dispatch(), final_path_for_handle() (+16 more)

### Community 49 - "texture_metadata"
Cohesion: 0.09
Nodes (24): bulk_extract_options, worker_count, entry_compression, uint16_t, uint32_t, uint64_t, uint8_t, texture_chunk_metadata (+16 more)

### Community 50 - "tes4_bsa_folder_record"
Cohesion: 0.08
Nodes (26): size_t, uint32_t, entry_compression, PayloadReader, size_t, uint32_t, uint64_t, make_tes4_bsa_payload_descriptor() (+18 more)

### Community 51 - "writer_publish.cpp"
Cohesion: 0.20
Nodes (20): Finalize, finalization_workspace, path, size_t, string, string_view, finalization_workspace, cleanup (+12 more)

### Community 52 - "ba2_dx10_write_archive_bytes"
Cohesion: 0.27
Nodes (8): ba2_dx10_write_archive_bytes(), byte, ostream, path, span, uint8_t, stream_writer, write_name()

### Community 53 - "ba2_dx10_chunk_assembler.cpp"
Cohesion: 0.18
Nodes (21): append_snapshot_bytes(), ba2_dx10_assemble_chunk(), ba2_dx10_assemble_planned_entry(), ba2_dx10_chunk_snapshot_batch, aggregate_size, snapshots, checked_size_t(), checked_u16() (+13 more)

### Community 54 - "validation.cpp"
Cohesion: 0.20
Nodes (20): append_entry_warnings(), append_fatal(), append_target_family_warning(), append_warning(), ascii_lowercase(), compatibility_warning_code, compatibility_warning_severity, error (+12 more)

### Community 55 - "result"
Cohesion: 0.06
Nodes (28): error, code, message, error_code, string, T, result, storage_ (+20 more)

### Community 56 - "ba2_dx10_writer::state"
Cohesion: 0.13
Nodes (19): ba2_dx10_writer::add_file(), ba2_dx10_writer::ba2_dx10_writer(), ba2_dx10_writer::state, consumed, entries, options, snapshot_dir, target (+11 more)

### Community 57 - "ba2_gnrl_placed_record"
Cohesion: 0.09
Nodes (26): ba2_gnrl_payload_placement, offset, payload, stored_size, ba2_gnrl_placed_record, archive_path_original, directory_hash, extension (+18 more)

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
Cohesion: 0.17
Nodes (13): ba2_dx10_prepare_chunk(), ba2_dx10_prepare_entries(), ba2_dx10_writer_options, planned_texture_chunk, uint32_t, vector, ba2_dx10_writer_entry, archive_path_canonical (+5 more)

### Community 62 - "validation_api_tests.cpp"
Cohesion: 0.08
Nodes (52): append_ascii(), append_u16_le(), append_u32_le(), append_u64_le(), bytes_from_text(), compatibility_matrix_path(), archive_type, archive_variant (+44 more)

### Community 63 - "string"
Cohesion: 0.20
Nodes (19): command_line_arguments(), compatibility_warning_code, compatibility_warning_severity, error_code, payload_sink, string, unique_ptr, error_code_name() (+11 more)

### Community 64 - "error"
Cohesion: 0.29
Nodes (19): ArgumentParser, archive_type_name(), archive_variant_name(), compression_name(), archive_type, archive_variant, entry_compression, error (+11 more)

### Community 65 - "tes4_bsa_target"
Cohesion: 0.16
Nodes (14): tes4_bsa_target, tes4_bsa_writer::target(), compressed_entry_method(), entry_compression, uint32_t, expected_target_default_compression(), expected_version(), target_expectation (+6 more)

### Community 66 - "materialize_entries"
Cohesion: 0.20
Nodes (18): add_fits(), compression_for(), entry_compression, size_t, span, string, uint32_t, uint64_t (+10 more)

### Community 67 - "host_file_tests.cpp"
Cohesion: 0.18
Nodes (17): bytes_from_text(), byte, DWORD, HANDLE, path, span, string_view, vector (+9 more)

### Community 68 - "ba2_dx10_preparer_seam_tests.cpp"
Cohesion: 0.20
Nodes (19): byte, json, path, planned_texture_chunk, size_t, span, string, uint8_t (+11 more)

### Community 69 - "source_dds_spec"
Cohesion: 0.12
Nodes (18): ostringstream, source_dds_spec, archive_path, array_size, depth, file, format_id, format_name (+10 more)

### Community 70 - "payload_stream.cpp"
Cohesion: 0.29
Nodes (17): checked_materialized_payload_size(), checked_payload_size(), byte, ifstream, size_t, span, string_view, uint64_t (+9 more)

### Community 71 - "ba2_archive_source"
Cohesion: 0.11
Nodes (17): ba2_stable_archive_source, session_, byte, size_t, stable_host_file_session, string_view, uint64_t, vector (+9 more)

### Community 72 - "path"
Cohesion: 0.26
Nodes (19): compression_choice, archive_policy_from(), collect_input_files(), archive_compression_policy, path, uint32_t, vector, generic_utf8_path() (+11 more)

### Community 73 - "tes4_bsa_profile"
Cohesion: 0.10
Nodes (35): profile_facts, archive_variant, compression_method, entry_compression, size_t, string_view, tes4_bsa_writer_options, tes4_folder_record_shape (+27 more)

### Community 74 - "ba2_gnrl_prepared_entry"
Cohesion: 0.17
Nodes (15): ba2_gnrl_prepared_entry, archive_path_canonical, archive_path_original, directory_hash, extension, name_hash, packed_size, payload (+7 more)

### Community 75 - "chunk_spec"
Cohesion: 0.12
Nodes (18): chunk_spec, compression, decoded_payload, end_mip, packed_size, payload_offset, raw_size, segment (+10 more)

### Community 76 - "validate_fixture_manifests.py"
Cohesion: 0.24
Nodes (16): Any, find_manifest_case(), load_json(), main(), Path, Validate malformed fixture case IDs, expected errors, and referenced archive fil, Return a manifest case by ID, failing if the manifest case list is malformed or, Validate the consolidated malformed matrix and its manifest/test evidence refere (+8 more)

### Community 77 - "ba2_gnrl_write_archive_bytes"
Cohesion: 0.17
Nodes (15): ba2_gnrl_write_archive_bytes(), checked_u16(), checked_u32(), ba2_gnrl_writer_options, byte, ostream, path, span (+7 more)

### Community 78 - "ba2_dx10_writer"
Cohesion: 0.16
Nodes (17): ba2_dx10_writer, ba2_dx10_target, ba2_dx10_writer, state_, ba2_gnrl_writer, ba2_gnrl_target, ba2_gnrl_writer, state_ (+9 more)

### Community 79 - "tes4_bsa_serialize.cpp"
Cohesion: 0.19
Nodes (16): checked_name_size(), checked_u32(), byte, ofstream, path, size_t, span, string_view (+8 more)

### Community 80 - "ba2_dx10_plan_placements"
Cohesion: 0.17
Nodes (14): add_fits_u64(), ba2_dx10_plan_placements(), compression_method, uint32_t, uint64_t, vector, dedupe_key, compression (+6 more)

### Community 81 - "detected_bsa_format"
Cohesion: 0.11
Nodes (26): byte, span, detect_bsa_format(), detected_bsa_format, variant, version, archive_variant, uint32_t (+18 more)

### Community 82 - "tes4_prepared_entry"
Cohesion: 0.16
Nodes (16): stored_payload, string, uint32_t, uint64_t, vector, tes4_prepared_entry, canonical_folder, file_hash (+8 more)

### Community 83 - "ba2_gnrl_writer_entry"
Cohesion: 0.14
Nodes (14): ba2_gnrl_make_writer_entry(), ba2_gnrl_validate_entries(), span, string_view, ba2_gnrl_writer_entry, archive_path_canonical, archive_path_original, from_memory (+6 more)

### Community 84 - "generate_ba2_dx10_fixtures.cpp"
Cohesion: 0.26
Nodes (16): bytes_per_block(), canonicalize(), checked_u32(), pair, string_view, uint32_t, dds_mip_size(), extension_fourcc() (+8 more)

### Community 85 - "materialize_entries"
Cohesion: 0.13
Nodes (24): compression_for(), array, byte, entry_compression, span, string, uint32_t, uint64_t (+16 more)

### Community 86 - "decompress_lz4_frame_exact_to_sink"
Cohesion: 0.20
Nodes (14): LZ4F_dctx, compress_lz4_frame(), byte, error, ifstream, size_t, span, string_view (+6 more)

### Community 87 - "bethesda_hash.cpp"
Cohesion: 0.32
Nodes (15): string_view, uint32_t, uint64_t, uint8_t, crc32_entry(), crc32_lookup(), extension_magic(), hash_fo4() (+7 more)

### Community 88 - "archive_metadata"
Cohesion: 0.17
Nodes (12): archive_metadata, archive_flags, ba2, default_compression, file_count, type, variant, version (+4 more)

### Community 89 - "tes3_write_archive_bytes"
Cohesion: 0.29
Nodes (15): add_fits_u64(), checked_add_u32(), checked_mul_u32(), checked_u32(), byte, ofstream, path, span (+7 more)

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

### Community 99 - "tes3_writer_entry"
Cohesion: 0.10
Nodes (30): tes3_bsa_writer_options, overwrite_existing, checked_u32(), span, string, string_view, uint32_t, uint64_t (+22 more)

### Community 100 - "format_case"
Cohesion: 0.14
Nodes (14): chunk(), byte, size_t, uint16_t, uint32_t, uint64_t, vector, format_case (+6 more)

### Community 101 - "commit_staged"
Cohesion: 0.21
Nodes (11): clear_delete_on_close(), commit_staged(), byte, DWORD, HANDLE, size_t, span, mark_delete_on_close() (+3 more)

### Community 102 - "reserve_metadata_set"
Cohesion: 0.22
Nodes (12): Allocator, Hash, Key, KeyEqual, error, size_t, string_view, T (+4 more)

### Community 103 - "BA2 DX10 Target Policies"
Cohesion: 0.24
Nodes (11): TES4 BSA Profile, Archive Family Writer Examples, BA2 DX10 Snapshot Lifecycle, BA2 DX10 Target Policies, BA2 GNRL Target Policies, Deflate Compression Route, LZ4 Frame Compression Route, Raw LZ4 Block Compression Route (+3 more)

### Community 104 - "analyze_dds_source"
Cohesion: 0.26
Nodes (12): DXGI_FORMAT, analyze_dds_metadata(), analyze_dds_source(), checked_u32(), byte, size_t, span, uint32_t (+4 more)

### Community 105 - "ba2_dx10_build_writer_entry_snapshot"
Cohesion: 0.11
Nodes (24): ba2_dx10_make_writer_entry(), ba2_dx10_validate_entries(), ba2_dx10_target, path, size_t, span, string_view, ba2_dx10_build_writer_entry_snapshot() (+16 more)

### Community 106 - "tes3_prepared_entry"
Cohesion: 0.14
Nodes (14): byte, string, uint32_t, uint64_t, vector, tes3_prepared_entry, archive_path_original, from_memory (+6 more)

### Community 107 - "tes4_writer_entry"
Cohesion: 0.18
Nodes (11): byte, entry_compression_policy, string, vector, tes4_writer_entry, archive_path_canonical, archive_path_original, compression (+3 more)

### Community 108 - "build_archive"
Cohesion: 0.37
Nodes (7): build_archive(), byte_buffer, bytes, uint64_t, record_table_size(), write_header(), write_records()

### Community 109 - "writer_execution_options_tests.cpp"
Cohesion: 0.29
Nodes (12): bytes_from_text(), byte, path, span, string_view, vector, generated_dx10_source_path(), output_path() (+4 more)

### Community 110 - "writer_hotspot_policy_tests.cpp"
Cohesion: 0.27
Nodes (11): path, span, string, string_view, vector, declaration_block(), public_declaration_lines(), read_text_file() (+3 more)

### Community 111 - "ba2_dx10_placed_record"
Cohesion: 0.12
Nodes (16): ba2_dx10_placed_record, archive_path_original, chunk_count, chunks, cube_maps_raw, directory_hash, dxgi_format, extension (+8 more)

### Community 112 - "serialization_expectation"
Cohesion: 0.15
Nodes (15): size_t, span, uint32_t, uint64_t, layout_expectation, folder_block_offset, payload_offset, target (+7 more)

### Community 113 - "entry_metadata"
Cohesion: 0.13
Nodes (17): entry_metadata, archive_hash, compression, embedded_name_prefix_size, has_embedded_name, original_path, path, payload_offset (+9 more)

### Community 114 - "tes4_bsa_writer_tests.cpp"
Cohesion: 0.38
Nodes (10): path, string, generated_source_dir(), non_ascii_output_path(), non_ascii_source_dir(), output_path(), target_name(), utf8_string_from_path() (+2 more)

### Community 115 - "tes4_bsa_writer.cpp"
Cohesion: 0.28
Nodes (8): tes4_bsa_writer, tes4_bsa_writer_options, vector, tes4_bsa_writer::state, entries, options, target, tes4_bsa_writer::tes4_bsa_writer()

### Community 116 - "tes4_bsa_raw_table"
Cohesion: 0.20
Nodes (12): size_t, string, vector, tes4_bsa_folder_block, files, name, tes4_bsa_raw_table, file_names (+4 more)

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
Cohesion: 0.17
Nodes (12): ba2_dx10_target, ba2_gnrl_target, format_descriptor, ba2_dx10_target, ba2_gnrl_target, description, family, supports_compressed (+4 more)

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

### Community 134 - "stable_host_file_session"
Cohesion: 0.20
Nodes (11): session_diagnostics, stable_host_file_session, session_diagnostics, uint64_t, stable_host_file_session, close, diagnostics_, native_handle_ (+3 more)

### Community 135 - "ba2_dx10_writer_options"
Cohesion: 0.29
Nodes (7): ba2_dx10_writer_options, deduplicate_payloads, max_decoded_chunk_bytes, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2

### Community 136 - "ba2_dx10_subresource_snapshot"
Cohesion: 0.22
Nodes (9): ba2_dx10_subresource_snapshot, array_index, face_index, mip, size, snapshot_path, path, uint32_t (+1 more)

### Community 137 - "ba2_dx10_malformed_tests.cpp"
Cohesion: 0.16
Nodes (15): collecting_sink, bytes_, byte, error_code, json, path, payload_sink, size_t (+7 more)

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
Cohesion: 0.20
Nodes (8): ba2_archive_session_context(), ba2_subtype, vector, open_ba2_archive(), opened_ba2_archive, entries, metadata, subtype

### Community 146 - "final_path_is_within_root"
Cohesion: 0.27
Nodes (9): final_path_is_within_root(), is_separator(), local_command_line_argv, value, same_windows_prefix(), trim_final_path(), wchar_t, wstring (+1 more)

### Community 147 - "thread_safety_docs_policy_tests.cpp"
Cohesion: 0.32
Nodes (7): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), source_root()

### Community 148 - "write_tes4_bsa_archive"
Cohesion: 0.28
Nodes (9): byte, entry_compression_policy, span, string_view, uint32_t, tes4_bsa_writer::add_bytes(), tes4_bsa_writer::add_file(), tes4_bsa_writer::write_to() (+1 more)

### Community 149 - "Archive Entry Catalog"
Cohesion: 0.25
Nodes (8): Archive Entry Catalog, BA2 Archive Header, BA2 Archive Opening, BA2 Profile, BA2 Record Identity, Stored Payload, Glossary Vocabulary Policy, Single-Context Domain Layout

### Community 150 - "read_text_file"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 151 - ".write"
Cohesion: 0.33
Nodes (5): byte, payload_sink, size_t, span, discard_payload_sink

### Community 152 - "archive_runtime_case"
Cohesion: 0.13
Nodes (15): archive_runtime_case, archive_host_path, archive_virtual_path, expect_texture_metadata, expected_ba2_compression_method, expected_default_compression, expected_payload, expected_type (+7 more)

### Community 153 - "prepare_entry"
Cohesion: 0.19
Nodes (13): archive_default_compressed(), ba2_gnrl_prepare_entries(), checked_u32(), archive_compression_policy, ba2_gnrl_writer_options, entry_compression_policy, finalization_workspace, size_t (+5 more)

### Community 154 - "read_text_file"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 155 - "payload_sink"
Cohesion: 0.35
Nodes (10): payload_sink(), ba2_dx10_extraction_host_context(), ifstream, extract_ba2_dx10_payload(), extract_compressed_chunk(), stream_raw_chunk(), ba2_gnrl_extraction_host_context(), extract_ba2_gnrl_payload() (+2 more)

### Community 156 - "supported_profile_case"
Cohesion: 0.17
Nodes (12): archive_variant, ba2_subtype, entry_compression, string_view, supported_profile_case, compression_method, default_compression, name (+4 more)

### Community 157 - "find_source_root_from"
Cohesion: 0.80
Nodes (4): find_source_root_from(), path, is_source_root(), source_root()

### Community 158 - "Synthetic Benchmark Harness"
Cohesion: 0.50
Nodes (4): Report-Only Performance Policy, Synthetic Benchmark Harness, libbsa Benchmark Report Target, libbsa Benchmarks Target

### Community 159 - "Compatibility Warning Policy"
Cohesion: 0.50
Nodes (4): BSA Embedded Name Compatibility Risk Evidence, Compressed Sound Payload Warning Evidence, Target Family Mismatch Warning Evidence, Compatibility Warning Policy

### Community 160 - "optional"
Cohesion: 0.11
Nodes (12): bulk_extract_entry_result, entry, failure, path, bulk_extract_sink_factory(), error, optional, archive_policy_expectation (+4 more)

### Community 161 - "Q: How do BA2 DX10 prepared chunks compression finalization deduplication serialization work and where are Stored Payload integration points?"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: How do BA2 DX10 prepared chunks compression finalization deduplication serialization work and where are Stored Payload integration points?, Source Nodes

### Community 162 - "ba2_dx10_placement_plan"
Cohesion: 0.22
Nodes (11): ba2_dx10_payload_placement, offset, payload, stored_size, ba2_dx10_placement_plan, filename_table_offset, payloads, records (+3 more)

### Community 163 - "GitHub Issue Tracker"
Cohesion: 0.67
Nodes (3): GitHub Issue Tracker, Wayfinder Issue Map, Canonical Triage States

### Community 164 - "Optional Local Corpus Checks"
Cohesion: 0.67
Nodes (3): Optional Local Corpus Checks, Deferred Compatibility Gaps, Local Game Fixture Policy

### Community 170 - "Q: MSVC C2039/C3861: ba2_gnrl_plan_placements is not a member of libbsa::formats::ba2 in writer_stage_tests.cpp"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: MSVC C2039/C3861: ba2_gnrl_plan_placements is not a member of libbsa::formats::ba2 in writer_stage_tests.cpp, Source Nodes

### Community 171 - "read_text_file"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 172 - "Q: Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33, Source Nodes

### Community 173 - "ba2_dx10_placed_chunk"
Cohesion: 0.18
Nodes (11): ba2_dx10_placed_chunk, compression, end_mip, packed_size, payload_index, raw_size, start_mip, compression_method (+3 more)

### Community 174 - "deflate_codec_tests.cpp"
Cohesion: 0.33
Nodes (5): byte, size_t, vector, deflate_vector(), impossible_byte_vector_size()

### Community 175 - "Q: Issue 33 DX10 reference compatibility audit"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: Issue 33 DX10 reference compatibility audit, Source Nodes

### Community 176 - "checked_u32"
Cohesion: 0.38
Nodes (9): add_fits_u64(), checked_add_u32(), checked_mul_u32(), checked_u32(), span, string_view, uint32_t, uint64_t (+1 more)

### Community 177 - "make_byte_vector"
Cohesion: 0.36
Nodes (9): append_byte_vector(), byte_vector_allocation_error(), byte, error, size_t, span, vector, make_byte_vector() (+1 more)

### Community 178 - "ba2_profile"
Cohesion: 0.10
Nodes (17): ba2_dx10_writer_options, ba2_gnrl_writer_options, ba2_profile, compressed_method_, is_dx10, is_gnrl, archive_variant, ba2_subtype (+9 more)

### Community 179 - "parser_primitives_tests.cpp"
Cohesion: 0.31
Nodes (7): path, size_t, impossible_set_capacity(), impossible_string_size(), impossible_vector_capacity(), temp_file_cleanup, path_

### Community 180 - "validation_options"
Cohesion: 0.25
Nodes (8): archive_type, archive_variant, uint64_t, validation_options, expected_type, expected_variant, max_extractability_entry_bytes, validate_entry_extractability

### Community 181 - "ba2_dx10_stored_header_options"
Cohesion: 0.33
Nodes (5): ba2_dx10_stored_header_options, starfield_compression_method, starfield_unknown1, starfield_unknown2, uint32_t

### Community 182 - "checked_u16"
Cohesion: 0.40
Nodes (6): checked_u16(), checked_u32(), string_view, uint16_t, uint32_t, uint64_t

### Community 183 - "payload_source"
Cohesion: 0.50
Nodes (3): payload_source, read, remaining

## Knowledge Gaps
- **820 isolated node(s):** `scenario`, `worker_count`, `elapsed_ms`, `bytes_processed`, `correctness_passed` (+815 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Work-memory lessons

**Known dead ends** — questions that led nowhere; don't re-derive.
- "Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33" -> `ba2_dx10_serialize.cpp`, `packed_size`, `writer.hpp`

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `result` connect `result` to `ba2_dx10_writer_tests.cpp`, `bulk_extraction_tests.cpp`, `string`, `dds_layout.cpp`, `libbsa_benchmarks.cpp`, `ba2_record_identity`, `package-consumer/main.cpp`, `archive_reader_dispatch_tests.cpp`, `host_path_correctness_boundary_tests.cpp`, `stored_payload.cpp`, `ba2_gnrl_reader_tests.cpp`, `tes3_bsa_reader_tests.cpp`, `tes3_bsa_writer_tests.cpp`, `tes4_bsa_reader_tests.cpp`, `ba2_archive_header`, `tes4_bsa_header_fields`, `host_file.cpp`, `tes3_bsa_parser.cpp`, `host_file_path`, `tes4_bsa_prepare.cpp`, `local_game_fixture_tests.cpp`, `binary_reader`, `ba2_archive_opening_tests.cpp`, `byte`, `ba2_dx10_record`, `ba2_profile.cpp`, `tes4_plan_placements`, `byte`, `writer_publish_tests.cpp`, `writer_entry_compression`, `file_sink_factory`, `ba2_dx10_extraction_tests.cpp`, `write_ba2_gnrl_archive`, `cli/main.cpp`, `tes4_bsa_folder_record`, `writer_publish.cpp`, `ba2_dx10_write_archive_bytes`, `ba2_dx10_chunk_assembler.cpp`, `validation.cpp`, `ba2_dx10_writer::state`, `ba2_gnrl_placed_record`, `ba2_dx10_writer_entry`, `string`, `materialize_entries`, `payload_stream.cpp`, `ba2_archive_source`, `path`, `tes4_bsa_profile`, `ba2_gnrl_write_archive_bytes`, `tes4_bsa_serialize.cpp`, `ba2_dx10_plan_placements`, `detected_bsa_format`, `ba2_gnrl_writer_entry`, `materialize_entries`, `decompress_lz4_frame_exact_to_sink`, `archive_metadata`, `tes3_write_archive_bytes`, `recording_sink`, `recording_sink`, `decompress_deflate_exact`, `decompress_payload_exact_to_sink`, `parser_primitives.cpp`, `ba2_gnrl_plan_placements`, `collecting_sink`, `tes3_writer_entry`, `commit_staged`, `reserve_metadata_set`, `analyze_dds_source`, `ba2_dx10_build_writer_entry_snapshot`, `entry_metadata`, `extract_file_payload`, `normalize_archive_path`, `decompress_lz4_block_exact`, `ba2_gnrl_entry_options`, `stable_host_file_session`, `ba2_dx10_malformed_tests.cpp`, `opened_ba2_archive`, `write_tes4_bsa_archive`, `.write`, `prepare_entry`, `payload_sink`, `optional`, `ba2_dx10_placement_plan`, `checked_u32`, `make_byte_vector`, `ba2_profile`, `checked_u16`?**
  _High betweenness centrality (0.323) - this node is a cross-community bridge._
- **Why does `host_file_path` connect `host_file_path` to `string`, `host_file_path_tests.cpp`, `stable_host_file_session`, `stored_payload.cpp`, `ba2_gnrl_reader_tests.cpp`, `tes3_bsa_reader_tests.cpp`, `opened_ba2_archive`, `host_file.cpp`, `tes3_bsa_parser.cpp`, `writer_stage_tests.cpp`, `payload_sink`, `tes4_bsa_prepare.cpp`, `ba2_archive_opening_tests.cpp`, `stored_payload_tests.cpp`, `ba2_dx10_extraction_tests.cpp`, `host_file_tests.cpp`, `detected_bsa_format`, `tes3_write_archive_bytes`, `tes3_writer_entry`, `tes3_prepared_entry`, `entry_metadata`, `tes4_bsa_writer.cpp`, `extract_file_payload`?**
  _High betweenness centrality (0.026) - this node is a cross-community bridge._
- **Why does `ba2_profile` connect `ba2_profile` to `ba2_gnrl_plan_placements`, `materialize_entries`, `string`, `ba2_dx10_preparer_seam_tests.cpp`, `ba2_profile.cpp`, `writer_publish_tests.cpp`, `ba2_gnrl_write_archive_bytes`, `ba2_dx10_plan_placements`, `ba2_dx10_write_archive_bytes`, `ba2_dx10_chunk_assembler.cpp`, `ba2_archive_header`, `ba2_dx10_stored_header_options`, `materialize_entries`, `prepare_entry`, `writer_stage_tests.cpp`, `payload_sink`, `ba2_dx10_writer_entry`?**
  _High betweenness centrality (0.025) - this node is a cross-community bridge._
- **What connects `scenario`, `worker_count`, `elapsed_ms` to the rest of the system?**
  _820 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ba2_dx10_writer_tests.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.06771929824561404 - nodes in this community are weakly interconnected._
- **Should `bulk_extraction_tests.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.056134723336006415 - nodes in this community are weakly interconnected._
- **Should `string` be split into smaller, more focused modules?**
  _Cohesion score 0.06736842105263158 - nodes in this community are weakly interconnected._