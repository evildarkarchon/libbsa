# Graph Report - libbsa  (2026-07-28)

## Corpus Check
- 226 files · ~150,813 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3775 nodes · 8774 edges · 174 communities (171 shown, 3 thin omitted)
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 182 edges (avg confidence: 0.83)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `048fa8fe`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- bulk_extraction_tests.cpp
- string
- libbsa_benchmarks.cpp
- generate_tes4_bsa_fixtures.cpp
- ba2_record_identity.cpp
- generate_ba2_gnrl_fixtures.cpp
- package-consumer/main.cpp
- ba2_dx10_reader_tests.cpp
- archive_reader_dispatch_tests.cpp
- tes4_bsa_profile
- dds_layout.cpp
- host_path_correctness_boundary_tests.cpp
- ba2_gnrl_writer_tests.cpp
- stored_payload.cpp
- ba2_gnrl_reader_tests.cpp
- tes3_bsa_reader_tests.cpp
- tes3_bsa_writer_tests.cpp
- tes3_prepared_entry
- Public API Core
- generate_tes3_bsa_fixtures.cpp
- entry_metadata
- tes4_bsa_reader_tests.cpp
- result
- tes4_bsa_profile_ownership_policy_tests.cpp
- generate_tes3_bsa_writer_fixtures.cpp
- ba2_writer_execution_tests.cpp
- tes4_bsa_profile.cpp
- tes4_bsa_header_fields
- local_game_fixture_tests.cpp
- writer_stage_tests.cpp
- binary_reader
- tes3_bsa_parser.cpp
- ba2_archive_opening_tests.cpp
- ba2_dx10_prepared_entry
- host_file.cpp
- ba2_dx10_record
- ba2_profile
- validation_policy_tests.cpp
- byte
- ba2_dx10_writer_tests.cpp
- tes4_bsa_prepare.cpp
- writer_publish_tests.cpp
- archive_metadata
- archive_reader::extract_entries
- file_sink_factory
- stored_payload_tests.cpp
- ba2_dx10_extraction_tests.cpp
- cli/main.cpp
- unordered_set
- ba2_gnrl_prepare.cpp
- writer_publish.cpp
- tes4_placement_plan
- ba2_gnrl_prepared_entry
- byte
- ba2_dx10_chunk_assembler.cpp
- ba2_dx10_writer::state
- ba2_gnrl_layout.cpp
- ba2_gnrl_write_archive_bytes
- tes4_bsa_raw_table
- validation.cpp
- materialize_entries
- ba2_dx10_write_archive_bytes
- bsa_writer_execution_tests.cpp
- compatibility_warning_tests.cpp
- validation_report
- ba2_dx10_writer_entry
- detected_bsa_format
- host_file_path
- string
- Archive Entry Catalog
- error
- path
- materialize_entries
- tes4_prepared_entry
- make_byte_vector
- source_dds_spec
- payload_stream.cpp
- ba2_dx10_build_writer_entry_snapshot
- make_tes4_bsa_payload_descriptor
- tes4_bsa_serialize.cpp
- chunk_spec
- ba2_record_identity
- host_file_tests.cpp
- validate_fixture_manifests.py
- ba2_dx10_writer
- write_ba2_gnrl_archive
- tes3_bsa_writer.cpp
- dedupe_key
- string
- payload_sink
- validation_api_tests.cpp
- bethesda_hash.cpp
- ba2_archive_source
- opened_ba2_archive
- generate_ba2_dx10_fixtures.cpp
- texture_spec
- recording_sink
- decompress_deflate_exact
- decompress_payload_exact_to_sink
- parser_primitives.cpp
- collecting_sink
- recording_sink
- profile_expectation
- format_case
- validation_archive_case
- commit_staged
- analyze_dds_source
- bulk_extract_entry_result
- ba2_gnrl_writer_options
- uint32_t
- build_archive
- byte
- writer_execution_options_tests.cpp
- writer_hotspot_policy_tests.cpp
- deflate_codec_tests.cpp
- writer_entry_compression
- extract_file_payload
- vector
- archive_reader_dispatch_policy_tests.cpp
- native_handle_guard
- writer_host_path_inventory_case
- format_descriptor
- texture_metadata
- ba2_gnrl_writer.cpp
- write_tes4_bsa_archive
- tes4_writer_entry
- archive_spec
- read_u32_le
- writer_ownership_tests.cpp
- Build.ps1
- tes4_bsa_target
- normalize_archive_path
- decompress_lz4_block_exact
- ba2_gnrl_writer_entry
- coverage_audit_matrix_docs_tests.cpp
- docs_policy_tests.cpp
- host_file_path_tests.cpp
- parser_preparer_seam_policy_tests.cpp
- string_view
- final_path_is_within_root
- dependencies
- ba2_dx10_subresource_snapshot
- tes4_bsa_constants.hpp
- tes4_bsa_writer::state
- ba2_dx10_malformed_tests.cpp
- benchmark_policy_tests.cpp
- target_format_policy_tests.cpp
- validation_options
- collecting_sink
- tes4_bsa_writer_tests.cpp
- thread_safety_docs_policy_tests.cpp
- Distinct Sink Contract
- ba2_dx10_writer_options
- bulk_request_group
- read_ba2_dx10_names
- .write
- TES5Edit Read-Only Boundary
- GitHub Issues as Tracker
- archive_policy_expectation
- find_source_root_from
- host_file_writer_name_tests.cpp
- payload_sink
- read_text_file
- read_text_file
- fourcc
- Validation Result and Report Contract
- Optional Local Corpus Evidence
- libbsa Library
- Stored Payload

## God Nodes (most connected - your core abstractions)
1. `result` - 414 edges
2. `host_file_path` - 60 edges
3. `ba2_profile` - 57 edges
4. `entry_metadata` - 55 edges
5. `tes4_bsa_profile` - 45 edges
6. `ba2_gnrl_prepared_entry` - 33 edges
7. `binary_reader` - 29 edges
8. `archive_metadata` - 28 edges
9. `ba2_dx10_prepared_entry` - 27 edges
10. `archive_reader` - 25 edges

## Surprising Connections (you probably didn't know these)
- `libbsa Library` --semantically_similar_to--> `Reusable Bethesda Archive Library`  [INFERRED] [semantically similar]
  AGENTS.md → README.md
- `TES5Edit Read-Only Boundary` --semantically_similar_to--> `TES5Edit Boundary`  [INFERRED] [semantically similar]
  AGENTS.md → CLAUDE.md
- `TES5Edit Read-Only Boundary` --semantically_similar_to--> `TES5Edit Reference Boundary`  [INFERRED] [semantically similar]
  AGENTS.md → README.md
- `TES5Edit Read-Only Boundary` --semantically_similar_to--> `TES5Edit Fixture Boundary`  [INFERRED] [semantically similar]
  AGENTS.md → tests/fixtures/README.md
- `TES4 BSA Profile` --semantically_similar_to--> `TES4-Family BSA Target Profiles`  [INFERRED] [semantically similar]
  CONTEXT.md → docs/target-format-guide.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Default Support Proof Ecosystem** — docs_coverage_audit_matrix_four_family_default_proof, docs_compatibility_evidence_default_reproducible_evidence, docs_api_mainpage_installed_package_proof, tests_cmakelists_package_consumer_smoke, tests_fixtures_readme_committed_generated_fixtures [EXTRACTED 1.00]
- **Supported Archive Family Profiles** — docs_target_format_guide_tes3_bsa, docs_target_format_guide_tes4_family_bsa, docs_target_format_guide_ba2_gnrl, docs_target_format_guide_ba2_dx10, docs_coverage_audit_matrix_four_family_default_proof [EXTRACTED 1.00]
- **BA2 DX10 Lifecycle Contract** — docs_api_mainpage_ba2_dx10_one_shot_lifecycle, docs_integration_examples_ba2_dx10_one_shot_lifecycle, docs_target_format_guide_ba2_dx10_snapshot_lifecycle [INFERRED 0.95]

## Communities (174 total, 3 thin omitted)

### Community 0 - "bulk_extraction_tests.cpp"
Cohesion: 0.06
Nodes (67): archive_reader, state_, LIBBSA_API, shared_ptr, state, build_raw_ba2_dx10_fixture(), bulk_extraction_test_dir(), byte_buffer (+59 more)

### Community 1 - "string"
Cohesion: 0.07
Nodes (13): bulk_extract_request, path, optional, string, vector, span, string_view, payload_sink (+5 more)

### Community 2 - "libbsa_benchmarks.cpp"
Cohesion: 0.08
Nodes (64): add_disk_payloads(), benchmark_result, bytes_processed, correctness_passed, elapsed_ms, scenario, worker_count, benchmark_sink_factory (+56 more)

### Community 3 - "generate_tes4_bsa_fixtures.cpp"
Cohesion: 0.07
Nodes (63): archive_spec, entries, file_flags, flags, folder, folder_hash, folder_offset, stem (+55 more)

### Community 4 - "ba2_record_identity.cpp"
Cohesion: 0.18
Nodes (31): ba2_record_identity_source, ascii_lower_byte(), ba2_record_path, canonical_path, display_path, array, ba2_subtype, byte (+23 more)

### Community 5 - "generate_ba2_gnrl_fixtures.cpp"
Cohesion: 0.08
Nodes (62): archive_spec, compression_method, entries, starfield_unknown1, starfield_unknown2, stem, variant, version (+54 more)

### Community 6 - "package-consumer/main.cpp"
Cohesion: 0.07
Nodes (57): archive_runtime_case, archive_host_path, archive_virtual_path, expect_texture_metadata, expected_ba2_compression_method, expected_default_compression, expected_payload, expected_type (+49 more)

### Community 7 - "ba2_dx10_reader_tests.cpp"
Cohesion: 0.10
Nodes (60): append_ascii(), append_dx10_chunk_record(), append_dx10_record_for_path(), append_dx10_record_header(), append_u16_le(), append_u32_le(), append_u64_le(), append_u8() (+52 more)

### Community 8 - "archive_reader_dispatch_tests.cpp"
Cohesion: 0.06
Nodes (53): archive_format_fixture, archive_filename, expected_extract_path, expected_missing_path, invalid_archive_path, manifest_filename, name, archive_format_fixtures() (+45 more)

### Community 9 - "tes4_bsa_profile"
Cohesion: 0.11
Nodes (29): add_fits_u64(), archive_flags_for(), calculate_table_lengths(), checked_name_size(), checked_stored_size(), checked_u32(), size_t, string_view (+21 more)

### Community 10 - "dds_layout.cpp"
Cohesion: 0.05
Nodes (80): append_chunks_for_ranges(), build_dds_dxt10_header(), capped_mip_ranges(), checked_add(), checked_mul(), byte, error, planned_texture_chunk (+72 more)

### Community 11 - "host_path_correctness_boundary_tests.cpp"
Cohesion: 0.07
Nodes (52): bytes_from_hex(), capture_report, create_count_by_path, sink_bytes_by_path, capturing_sink, capture_, collecting_sink, bytes_ (+44 more)

### Community 12 - "ba2_gnrl_writer_tests.cpp"
Cohesion: 0.08
Nodes (55): compression_case, compression_method, expected_compression, file_name, target, version, archive_variant, ba2_gnrl_target (+47 more)

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

### Community 17 - "tes3_prepared_entry"
Cohesion: 0.05
Nodes (61): add_fits_u64(), checked_add_u32(), checked_mul_u32(), checked_u32(), span, string_view, uint32_t, uint64_t (+53 more)

### Community 18 - "Public API Core"
Cohesion: 0.04
Nodes (47): Windows MSVC AddressSanitizer Lane, Windows MSVC Verification Matrix, Compression and Texture Dependency Policy, Fixture-Based Compatibility Validation, Minimal Dependency-Light Public API, Windows-Only Platform Contract, Report-Only Timing Policy, Synthetic Archive Benchmark Harness (+39 more)

### Community 19 - "generate_tes3_bsa_fixtures.cpp"
Cohesion: 0.13
Nodes (40): build_tes3_archive(), byte_buffer, bytes, bytes_from_string(), canonicalize(), checked_u32(), byte, path (+32 more)

### Community 20 - "entry_metadata"
Cohesion: 0.13
Nodes (17): entry_metadata, archive_hash, compression, embedded_name_prefix_size, has_embedded_name, original_path, path, payload_offset (+9 more)

### Community 21 - "tes4_bsa_reader_tests.cpp"
Cohesion: 0.10
Nodes (39): archive_original_path_from_manifest(), bytes_from_hex(), collecting_sink, bytes_, byte, entry_compression, error_code, json (+31 more)

### Community 22 - "result"
Cohesion: 0.08
Nodes (20): error, code, message, error_code, string, T, result, storage_ (+12 more)

### Community 23 - "tes4_bsa_profile_ownership_policy_tests.cpp"
Cohesion: 0.13
Nodes (39): branches_on_direct_tes4_identity(), code_only(), contains_integer_literal(), contains_word(), control_condition, keyword, text, control_conditions() (+31 more)

### Community 24 - "generate_tes3_bsa_writer_fixtures.cpp"
Cohesion: 0.13
Nodes (37): bytes_from_text(), canonicalize(), byte, path, size_t, span, string, string_view (+29 more)

### Community 25 - "ba2_writer_execution_tests.cpp"
Cohesion: 0.13
Nodes (34): add_dx10_sources(), add_gnrl_disk_sources(), bytes_from_text(), archive_variant, ba2_dx10_writer, ba2_gnrl_writer, byte, entry_compression (+26 more)

### Community 26 - "tes4_bsa_profile.cpp"
Cohesion: 0.12
Nodes (25): profile_facts, archive_variant, compression_method, entry_compression, size_t, string_view, tes4_bsa_writer_options, tes4_folder_record_shape (+17 more)

### Community 27 - "tes4_bsa_header_fields"
Cohesion: 0.12
Nodes (33): add_fits(), size_t, uint32_t, multiply_fits(), span_fits(), byte, size_t, span (+25 more)

### Community 28 - "local_game_fixture_tests.cpp"
Cohesion: 0.12
Nodes (29): archive_path_from_manifest(), archive_type_from_string(), archive_variant_from_string(), bsarchpro_expected_manifest_path(), bytes_from_hex(), archive_type, archive_variant, byte (+21 more)

### Community 29 - "writer_stage_tests.cpp"
Cohesion: 0.12
Nodes (35): ba2_dx10_prepared_stage_entry(), ba2_dx10_stage_entry(), ba2_gnrl_disk_stage_entry(), ba2_gnrl_memory_stage_entry(), bytes_from_text(), byte, finalization_workspace, pair (+27 more)

### Community 30 - "binary_reader"
Cohesion: 0.14
Nodes (31): binary_reader, binary_reader::binary_reader(), bytes_, can_read, position, read_bytes, read_u16_le, read_u32_le (+23 more)

### Community 31 - "tes3_bsa_parser.cpp"
Cohesion: 0.15
Nodes (31): byte, size_t, span, string, uint32_t, uint64_t, vector, file_record (+23 more)

### Community 32 - "ba2_archive_opening_tests.cpp"
Cohesion: 0.10
Nodes (29): append_placeholder_gnrl_record(), append_u16_le(), append_u32_le(), append_u64_le(), archive_variant, ba2_subtype, byte, entry_compression (+21 more)

### Community 33 - "ba2_dx10_prepared_entry"
Cohesion: 0.07
Nodes (31): ba2_dx10_prepared_chunk, compression, end_mip, owns_payload_bytes, packed_size, payload_offset, raw_size, start_mip (+23 more)

### Community 34 - "host_file.cpp"
Cohesion: 0.13
Nodes (48): checked_buffer_size(), byte, error, function, ifstream, path, session_diagnostics, size_t (+40 more)

### Community 35 - "ba2_dx10_record"
Cohesion: 0.07
Nodes (29): ba2_dx10_chunk_record, end_mip, offset, packed_size, raw_size, start_mip, ba2_dx10_record, chunk_count (+21 more)

### Community 36 - "ba2_profile"
Cohesion: 0.06
Nodes (62): ba2_archive_metadata, compression_method, starfield_unknown1, starfield_unknown2, ba2_archive_header, ba2_archive_header::ba2_archive_header(), file_count, filename_table_offset (+54 more)

### Community 37 - "validation_policy_tests.cpp"
Cohesion: 0.15
Nodes (29): command_succeeds(), compatibility_warning_codes_from_public_header(), count_occurrences(), array, initializer_list, optional, path, size_t (+21 more)

### Community 38 - "byte"
Cohesion: 0.13
Nodes (20): detail::payload_sink, detail::payload_source, byte, path, payload_sink, size_t, span, vector (+12 more)

### Community 39 - "ba2_dx10_writer_tests.cpp"
Cohesion: 0.05
Nodes (85): set, dds_source_analysis, dds_bytes, image_payload_bytes, metadata, subresources, dds_source_subresource, array_index (+77 more)

### Community 40 - "tes4_bsa_prepare.cpp"
Cohesion: 0.13
Nodes (34): append_u32_le(), checked_name_size(), checked_u32(), byte, entry_compression_policy, finalization_workspace, pair, size_t (+26 more)

### Community 41 - "writer_publish_tests.cpp"
Cohesion: 0.11
Nodes (25): bytes_from_text(), byte, path, span, string, string_view, vector, is_reparse_point() (+17 more)

### Community 42 - "archive_metadata"
Cohesion: 0.12
Nodes (17): extract_entry_callback, archive_metadata, archive_flags, ba2, default_compression, file_count, type, variant (+9 more)

### Community 43 - "archive_reader::extract_entries"
Cohesion: 0.20
Nodes (18): bulk_extract_sink_factory(), archive_reader::contains(), archive_reader::extract(), archive_reader::extract_bytes(), archive_reader::extract_entries(), archive_reader::find(), extract, byte (+10 more)

### Community 44 - "file_sink_factory"
Cohesion: 0.11
Nodes (21): less, bulk_extract_sink_factory, map, mutex, ofstream, shared_ptr, discard_staged(), file_payload_sink (+13 more)

### Community 45 - "stored_payload_tests.cpp"
Cohesion: 0.13
Nodes (22): stored_payload, streambuf, streamsize, bytes_from_text(), collecting_stream_buffer, bytes_, byte, finalization_workspace (+14 more)

### Community 46 - "ba2_dx10_extraction_tests.cpp"
Cohesion: 0.15
Nodes (23): ba2_dx10_fixture, archive, manifest, bytes_from_hex(), collecting_sink, bytes_, byte, json (+15 more)

### Community 47 - "cli/main.cpp"
Cohesion: 0.18
Nodes (24): add_help_argument(), add_positionals_argument(), add_thread_argument(), ascii_iequals(), ostream, string_view, dispatch(), final_path_for_handle() (+16 more)

### Community 48 - "unordered_set"
Cohesion: 0.12
Nodes (20): Allocator, Hash, Key, KeyEqual, error, size_t, string_view, T (+12 more)

### Community 49 - "ba2_gnrl_prepare.cpp"
Cohesion: 0.18
Nodes (23): archive_default_compressed(), ba2_gnrl_final_stored_dedupe_hash(), ba2_gnrl_make_writer_entry(), ba2_gnrl_prepare_entries(), ba2_gnrl_validate_entries(), checked_u32(), archive_compression_policy, ba2_gnrl_writer_options (+15 more)

### Community 50 - "writer_publish.cpp"
Cohesion: 0.20
Nodes (20): Finalize, finalization_workspace, path, size_t, string, string_view, finalization_workspace, cleanup (+12 more)

### Community 51 - "tes4_placement_plan"
Cohesion: 0.08
Nodes (30): size_t, string, tes4_folder_record_shape, uint32_t, uint64_t, vector, tes4_payload_placement, offset (+22 more)

### Community 52 - "ba2_gnrl_prepared_entry"
Cohesion: 0.09
Nodes (23): ba2_gnrl_prepared_entry, archive_path_canonical, archive_path_original, directory_hash, extension, final_stored_dedupe_hash, name_hash, owns_payload_bytes (+15 more)

### Community 53 - "byte"
Cohesion: 0.19
Nodes (19): bytes_from_text(), collecting_sink, bytes_, byte, payload_sink, size_t, span, string_view (+11 more)

### Community 54 - "ba2_dx10_chunk_assembler.cpp"
Cohesion: 0.26
Nodes (15): ba2_dx10_assemble_chunk(), ba2_dx10_assemble_planned_entry(), checked_size_t(), checked_u16(), checked_u32(), checked_u8(), collect_chunk_snapshots(), ba2_dx10_writer_options (+7 more)

### Community 55 - "ba2_dx10_writer::state"
Cohesion: 0.15
Nodes (19): ba2_dx10_writer::add_file(), ba2_dx10_writer::ba2_dx10_writer(), ba2_dx10_writer::state, consumed, entries, options, snapshot_dir, target (+11 more)

### Community 56 - "ba2_gnrl_layout.cpp"
Cohesion: 0.16
Nodes (20): add_fits_u64(), ba2_gnrl_assign_payload_offsets(), ba2_gnrl_final_stored_dedupe_key, final_stored_dedupe_hash, stored_size, ba2_gnrl_payloads_equal(), checked_u32(), compare_disk_payload_to_bytes() (+12 more)

### Community 57 - "ba2_gnrl_write_archive_bytes"
Cohesion: 0.18
Nodes (16): ba2_gnrl_write_archive_bytes(), checked_u16(), checked_u32(), ba2_gnrl_writer_options, byte, ostream, path, span (+8 more)

### Community 58 - "tes4_bsa_raw_table"
Cohesion: 0.11
Nodes (22): size_t, string, uint32_t, uint64_t, vector, tes4_bsa_file_record, hash, offset (+14 more)

### Community 59 - "validation.cpp"
Cohesion: 0.20
Nodes (20): append_entry_warnings(), append_fatal(), append_target_family_warning(), append_warning(), ascii_lowercase(), compatibility_warning_code, compatibility_warning_severity, error (+12 more)

### Community 60 - "materialize_entries"
Cohesion: 0.14
Nodes (24): compression_for(), array, byte, entry_compression, span, string, uint32_t, uint64_t (+16 more)

### Community 61 - "ba2_dx10_write_archive_bytes"
Cohesion: 0.19
Nodes (15): ba2_dx10_write_archive_bytes(), checked_u16(), checked_u32(), ba2_dx10_writer_options, byte, ostream, path, span (+7 more)

### Community 62 - "bsa_writer_execution_tests.cpp"
Cohesion: 0.23
Nodes (20): add_tes4_sources(), bytes_from_text(), byte, pair, path, size_t, span, string (+12 more)

### Community 63 - "compatibility_warning_tests.cpp"
Cohesion: 0.24
Nodes (20): bytes_from_text(), compatibility_warning_codes_from_public_header(), byte, compatibility_warning_code, path, size_t, string, string_view (+12 more)

### Community 64 - "validation_report"
Cohesion: 0.11
Nodes (20): compatibility_warning, archive_path, code, message, severity, compatibility_warning_code, compatibility_warning_severity, error_code (+12 more)

### Community 65 - "ba2_dx10_writer_entry"
Cohesion: 0.12
Nodes (20): ba2_dx10_make_writer_entry(), ba2_dx10_prepare_chunk(), ba2_dx10_prepare_entries(), ba2_dx10_validate_entries(), ba2_dx10_target, ba2_dx10_writer_options, path, planned_texture_chunk (+12 more)

### Community 66 - "detected_bsa_format"
Cohesion: 0.13
Nodes (24): detected_bsa_format, variant, version, archive_variant, uint32_t, byte, PayloadReader, size_t (+16 more)

### Community 67 - "host_file_path"
Cohesion: 0.19
Nodes (10): archive_file_size(), archive_open_host_context(), archive_reader::open(), uint64_t, read_detection_prefix(), string_view, host_file_path, resolved (+2 more)

### Community 68 - "string"
Cohesion: 0.20
Nodes (19): command_line_arguments(), compatibility_warning_code, compatibility_warning_severity, error_code, payload_sink, string, unique_ptr, error_code_name() (+11 more)

### Community 69 - "Archive Entry Catalog"
Cohesion: 0.11
Nodes (19): Single-Context Domain Documentation, Archive Entry Catalog, BA2 Archive Header, BA2 Archive Opening, BA2 Profile, BA2 Record Identity, TES4 BSA Profile, ADR Conflict Policy (+11 more)

### Community 70 - "error"
Cohesion: 0.29
Nodes (19): ArgumentParser, archive_type_name(), archive_variant_name(), compression_name(), archive_type, archive_variant, entry_compression, error (+11 more)

### Community 71 - "path"
Cohesion: 0.26
Nodes (19): compression_choice, archive_policy_from(), collect_input_files(), archive_compression_policy, path, uint32_t, vector, generic_utf8_path() (+11 more)

### Community 72 - "materialize_entries"
Cohesion: 0.21
Nodes (17): compression_for(), entry_compression, size_t, span, string, uint32_t, uint64_t, vector (+9 more)

### Community 73 - "tes4_prepared_entry"
Cohesion: 0.16
Nodes (16): stored_payload, string, uint32_t, uint64_t, vector, tes4_prepared_entry, canonical_folder, file_hash (+8 more)

### Community 74 - "make_byte_vector"
Cohesion: 0.14
Nodes (23): LZ4F_dctx, append_byte_vector(), byte_vector_allocation_error(), byte, error, size_t, span, vector (+15 more)

### Community 75 - "source_dds_spec"
Cohesion: 0.14
Nodes (14): source_dds_spec, archive_path, array_size, depth, file, format_id, format_name, height (+6 more)

### Community 76 - "payload_stream.cpp"
Cohesion: 0.21
Nodes (20): checked_materialized_payload_size(), checked_payload_size(), byte, ifstream, size_t, span, string_view, uint64_t (+12 more)

### Community 77 - "ba2_dx10_build_writer_entry_snapshot"
Cohesion: 0.20
Nodes (17): ba2_dx10_build_writer_entry_snapshot(), ba2_dx10_ensure_snapshot_directory(), ba2_dx10_validate_texture_format_for_target(), ba2_dx10_target, byte, path, size_t, span (+9 more)

### Community 78 - "make_tes4_bsa_payload_descriptor"
Cohesion: 0.17
Nodes (12): entry_compression, PayloadReader, size_t, uint32_t, uint64_t, make_tes4_bsa_payload_descriptor(), tes4_bsa_payload_descriptor, compression (+4 more)

### Community 79 - "tes4_bsa_serialize.cpp"
Cohesion: 0.19
Nodes (16): checked_name_size(), checked_u32(), byte, ofstream, path, size_t, span, string_view (+8 more)

### Community 80 - "chunk_spec"
Cohesion: 0.12
Nodes (17): chunk_spec, compression, decoded_payload, end_mip, packed_size, payload_offset, raw_size, segment (+9 more)

### Community 81 - "ba2_record_identity"
Cohesion: 0.18
Nodes (13): ba2_record_identity, canonical_path, directory_hash, display_path, extension, name_hash, ba2_stored_record_identity, directory_hash (+5 more)

### Community 82 - "host_file_tests.cpp"
Cohesion: 0.20
Nodes (16): bytes_from_text(), byte, DWORD, HANDLE, path, span, string_view, vector (+8 more)

### Community 83 - "validate_fixture_manifests.py"
Cohesion: 0.24
Nodes (16): Any, find_manifest_case(), load_json(), main(), Path, Validate malformed fixture case IDs, expected errors, and referenced archive fil, Return a manifest case by ID, failing if the manifest case list is malformed or, Validate the consolidated malformed matrix and its manifest/test evidence refere (+8 more)

### Community 84 - "ba2_dx10_writer"
Cohesion: 0.16
Nodes (17): ba2_dx10_writer, ba2_dx10_target, ba2_dx10_writer, state_, ba2_gnrl_writer, ba2_gnrl_target, ba2_gnrl_writer, state_ (+9 more)

### Community 85 - "write_ba2_gnrl_archive"
Cohesion: 0.15
Nodes (17): ba2_gnrl_entry_options, compression, record_flags, entry_compression_policy, optional, uint32_t, write_execution_options, worker_count (+9 more)

### Community 86 - "tes3_bsa_writer.cpp"
Cohesion: 0.18
Nodes (15): tes3_bsa_writer_options, overwrite_existing, byte, span, string_view, vector, tes3_bsa_writer::add_bytes(), tes3_bsa_writer::add_file() (+7 more)

### Community 87 - "dedupe_key"
Cohesion: 0.14
Nodes (15): add_fits_u64(), ba2_dx10_assign_payload_offsets(), byte, compression_method, span, uint32_t, uint64_t, vector (+7 more)

### Community 88 - "string"
Cohesion: 0.23
Nodes (13): ostringstream, canonicalize(), pair, string, string_view, extension_fourcc(), filename_stem(), hash_folder() (+5 more)

### Community 89 - "payload_sink"
Cohesion: 0.35
Nodes (10): payload_sink(), ba2_dx10_extraction_host_context(), ifstream, extract_ba2_dx10_payload(), extract_compressed_chunk(), stream_raw_chunk(), ba2_gnrl_extraction_host_context(), extract_ba2_gnrl_payload() (+2 more)

### Community 90 - "validation_api_tests.cpp"
Cohesion: 0.30
Nodes (15): bytes_from_text(), compatibility_matrix_path(), path, generated_source_dir(), read_json_file(), temp_file_cleanup, path_, unique_output_path() (+7 more)

### Community 91 - "bethesda_hash.cpp"
Cohesion: 0.32
Nodes (15): string_view, uint32_t, uint64_t, uint8_t, crc32_entry(), crc32_lookup(), extension_magic(), hash_fo4() (+7 more)

### Community 92 - "ba2_archive_source"
Cohesion: 0.13
Nodes (14): ba2_archive_session_context(), ba2_stable_archive_source, session_, byte, size_t, stable_host_file_session, string_view, uint64_t (+6 more)

### Community 93 - "opened_ba2_archive"
Cohesion: 0.22
Nodes (8): ba2_subtype, vector, opened_ba2_archive, entries, metadata, subtype, ba2_dx10_writer_options, ba2_gnrl_writer_options

### Community 94 - "generate_ba2_dx10_fixtures.cpp"
Cohesion: 0.35
Nodes (15): path, generate_malformed(), generate_success(), generate_writer_sources(), main(), make_duplicate_canonical_path_malformed(), make_fo4(), make_sfv3() (+7 more)

### Community 95 - "texture_spec"
Cohesion: 0.12
Nodes (16): uint8_t, texture_spec, array_size, chunks, cube_maps_raw, directory_hash, dxgi_format, ext (+8 more)

### Community 96 - "recording_sink"
Cohesion: 0.17
Nodes (14): byte, path, payload_sink, size_t, span, string, vector, read_text_file() (+6 more)

### Community 97 - "decompress_deflate_exact"
Cohesion: 0.19
Nodes (12): libdeflate_compressor, libdeflate_decompressor, codec_error(), compress_deflate(), compressor_deleter, byte, error, size_t (+4 more)

### Community 98 - "decompress_payload_exact_to_sink"
Cohesion: 0.30
Nodes (14): compress_payload(), copy_bytes(), byte, compression_method, error, ifstream, size_t, span (+6 more)

### Community 99 - "parser_primitives.cpp"
Cohesion: 0.21
Nodes (14): add_fits_u64(), archive_string_from_bytes(), byte, ifstream, span, string, string_view, uint64_t (+6 more)

### Community 100 - "collecting_sink"
Cohesion: 0.14
Nodes (13): collecting_sink, bytes_, bulk_extract_sink_factory, byte, path, payload_sink, size_t, span (+5 more)

### Community 101 - "recording_sink"
Cohesion: 0.18
Nodes (13): byte, path, payload_sink, size_t, span, vector, impossible_byte_vector_size(), lz4_vector() (+5 more)

### Community 102 - "profile_expectation"
Cohesion: 0.13
Nodes (15): compression_method, entry_compression, size_t, tes4_folder_record_shape, uint32_t, profile_expectation, compressed_entry_metadata, compressed_payload_method (+7 more)

### Community 103 - "format_case"
Cohesion: 0.14
Nodes (14): chunk(), byte, size_t, uint16_t, uint32_t, uint64_t, vector, format_case (+6 more)

### Community 104 - "validation_archive_case"
Cohesion: 0.12
Nodes (20): append_u32_le(), archive_type, archive_variant, entry_compression, optional, uint32_t, require_metadata_matches_case(), require_starfield_v3_ba2_route() (+12 more)

### Community 105 - "commit_staged"
Cohesion: 0.21
Nodes (11): clear_delete_on_close(), commit_staged(), byte, DWORD, HANDLE, size_t, span, mark_delete_on_close() (+3 more)

### Community 106 - "analyze_dds_source"
Cohesion: 0.26
Nodes (12): DXGI_FORMAT, analyze_dds_metadata(), analyze_dds_source(), checked_u32(), byte, size_t, span, uint32_t (+4 more)

### Community 107 - "bulk_extract_entry_result"
Cohesion: 0.33
Nodes (5): bulk_extract_entry_result, entry, failure, path, error

### Community 108 - "ba2_gnrl_writer_options"
Cohesion: 0.15
Nodes (13): ba2_gnrl_writer_options, compression, deduplicate_payloads, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2, archive_compression_policy (+5 more)

### Community 109 - "uint32_t"
Cohesion: 0.43
Nodes (7): bytes_per_block(), checked_u32(), uint32_t, dds_mip_size(), is_block_compressed(), mip_dimension(), name_table_size()

### Community 110 - "build_archive"
Cohesion: 0.37
Nodes (7): build_archive(), byte_buffer, bytes, uint64_t, record_table_size(), write_header(), write_records()

### Community 111 - "byte"
Cohesion: 0.38
Nodes (7): append_ascii(), append_u16_le(), append_u64_le(), byte, uint16_t, uint64_t, vector

### Community 112 - "writer_execution_options_tests.cpp"
Cohesion: 0.29
Nodes (12): bytes_from_text(), byte, path, span, string_view, vector, generated_dx10_source_path(), output_path() (+4 more)

### Community 113 - "writer_hotspot_policy_tests.cpp"
Cohesion: 0.27
Nodes (12): path, span, string, string_view, vector, declaration_block(), function_body(), public_declaration_lines() (+4 more)

### Community 114 - "deflate_codec_tests.cpp"
Cohesion: 0.33
Nodes (5): byte, size_t, vector, deflate_vector(), impossible_byte_vector_size()

### Community 115 - "writer_entry_compression"
Cohesion: 0.18
Nodes (11): archive_compression_policy, entry_compression_policy, uint64_t, entry_compression, uint32_t, archive_default_compressed, writer_entry_compression, tes4_bsa_writer_options (+3 more)

### Community 116 - "extract_file_payload"
Cohesion: 0.23
Nodes (11): compression_method_for(), byte, compression_method, entry_compression, ifstream, span, uint32_t, extract_file_payload() (+3 more)

### Community 117 - "vector"
Cohesion: 0.29
Nodes (11): build_source_dds(), bytes(), byte, initializer_list, size_t, span, vector, overwrite_u16() (+3 more)

### Community 118 - "archive_reader_dispatch_policy_tests.cpp"
Cohesion: 0.35
Nodes (11): optional, path, string, string_view, declaration_body(), extraction_function_member_name(), production_source_text(), read_text_file() (+3 more)

### Community 119 - "native_handle_guard"
Cohesion: 0.47
Nodes (3): HANDLE, native_handle_guard, handle_

### Community 120 - "writer_host_path_inventory_case"
Cohesion: 0.17
Nodes (12): string_view, writer_host_path_inventory_case, dedupe_absent_token, dedupe_file, dedupe_token, family, finalization_file, finalization_token (+4 more)

### Community 121 - "format_descriptor"
Cohesion: 0.17
Nodes (12): ba2_dx10_target, ba2_gnrl_target, format_descriptor, ba2_dx10_target, ba2_gnrl_target, description, family, supports_compressed (+4 more)

### Community 122 - "texture_metadata"
Cohesion: 0.09
Nodes (25): bulk_extract_options, worker_count, entry_compression, uint16_t, uint32_t, uint64_t, uint8_t, texture_chunk_metadata (+17 more)

### Community 123 - "ba2_gnrl_writer.cpp"
Cohesion: 0.25
Nodes (10): ba2_gnrl_writer::ba2_gnrl_writer(), ba2_gnrl_writer::state, entries, options, target, ba2_gnrl_writer::target(), ba2_gnrl_target, ba2_gnrl_writer (+2 more)

### Community 124 - "write_tes4_bsa_archive"
Cohesion: 0.24
Nodes (10): make_tes4_bsa_profile_for_writer(), byte, entry_compression_policy, span, string_view, uint32_t, tes4_bsa_writer::add_bytes(), tes4_bsa_writer::add_file() (+2 more)

### Community 125 - "tes4_writer_entry"
Cohesion: 0.18
Nodes (11): byte, entry_compression_policy, string, vector, tes4_writer_entry, archive_path_canonical, archive_path_original, compression (+3 more)

### Community 126 - "archive_spec"
Cohesion: 0.17
Nodes (13): archive_spec, compression_method, starfield_unknown1, starfield_unknown2, stem, textures, variant, version (+5 more)

### Community 127 - "read_u32_le"
Cohesion: 0.20
Nodes (10): byte, path, size_t, span, string_view, uint32_t, vector, generated_archive_path() (+2 more)

### Community 128 - "writer_ownership_tests.cpp"
Cohesion: 0.29
Nodes (10): bytes_from_text(), byte, path, span, string_view, vector, generated_source_dir(), require_extracted_bytes() (+2 more)

### Community 129 - "Build.ps1"
Cohesion: 0.27
Nodes (5): Assert-SafeCleanPath(), Get-SafePathForDisplay(), Invoke-CMakeBuildTarget(), Invoke-ExternalCommand(), Remove-BuildDirectory()

### Community 130 - "tes4_bsa_target"
Cohesion: 0.16
Nodes (14): tes4_bsa_target, tes4_bsa_writer::target(), compressed_entry_method(), entry_compression, uint32_t, expected_target_default_compression(), expected_version(), target_expectation (+6 more)

### Community 131 - "normalize_archive_path"
Cohesion: 0.29
Nodes (9): archive_path_key, value, error, string_view, string, invalid_path_error(), is_drive_rooted(), lower_ascii() (+1 more)

### Community 132 - "decompress_lz4_block_exact"
Cohesion: 0.40
Nodes (9): block_error(), checked_int_size(), compress_lz4_block(), byte, error, size_t, span, vector (+1 more)

### Community 133 - "ba2_gnrl_writer_entry"
Cohesion: 0.20
Nodes (10): ba2_gnrl_writer_entry, archive_path_canonical, archive_path_original, from_memory, host_path, memory_bytes, options, byte (+2 more)

### Community 134 - "coverage_audit_matrix_docs_tests.cpp"
Cohesion: 0.31
Nodes (9): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_markdown_section(), require_no_tokens() (+1 more)

### Community 135 - "docs_policy_tests.cpp"
Cohesion: 0.33
Nodes (9): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_no_planning_identifier_patterns(), require_no_tokens() (+1 more)

### Community 136 - "host_file_path_tests.cpp"
Cohesion: 0.40
Nodes (9): path, string, host_file_path_header_path(), host_file_path_source_path(), malformed_utf8_host_path(), project_root(), read_text_file(), unique_non_ascii_host_path() (+1 more)

### Community 137 - "parser_preparer_seam_policy_tests.cpp"
Cohesion: 0.31
Nodes (9): path, span, string, string_view, function_body(), read_text_file(), require_absent_tokens(), require_all_tokens() (+1 more)

### Community 138 - "string_view"
Cohesion: 0.36
Nodes (10): error_code, json, string_view, error_code_from_matrix(), generated_archive_dir(), report_has_error_code(), require_malformed_open_report(), require_matrix_extraction_report() (+2 more)

### Community 139 - "final_path_is_within_root"
Cohesion: 0.27
Nodes (9): final_path_is_within_root(), is_separator(), local_command_line_argv, value, same_windows_prefix(), trim_final_path(), wchar_t, wstring (+1 more)

### Community 140 - "dependencies"
Cohesion: 0.22
Nodes (8): argparse, catch2, directxtex, lz4, nlohmann-json, dependencies, name, version-semver

### Community 141 - "ba2_dx10_subresource_snapshot"
Cohesion: 0.14
Nodes (15): append_snapshot_bytes(), ba2_dx10_chunk_snapshot_batch, aggregate_size, snapshots, byte, vector, ba2_dx10_subresource_snapshot, array_index (+7 more)

### Community 142 - "tes4_bsa_constants.hpp"
Cohesion: 0.24
Nodes (7): byte, span, detect_bsa_format(), size_t, uint32_t, non_empty_span_intersects_prefix(), tes4_bsa_stored_payload_size()

### Community 143 - "tes4_bsa_writer::state"
Cohesion: 0.25
Nodes (8): tes4_bsa_writer, tes4_bsa_writer_options, vector, tes4_bsa_writer::state, entries, options, target, tes4_bsa_writer::tes4_bsa_writer()

### Community 144 - "ba2_dx10_malformed_tests.cpp"
Cohesion: 0.36
Nodes (8): error_code, json, path, string_view, error_code_from_manifest(), generated_archive_dir(), generated_archive_path(), read_json_file()

### Community 145 - "benchmark_policy_tests.cpp"
Cohesion: 0.33
Nodes (8): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_no_tokens(), source_root()

### Community 146 - "target_format_policy_tests.cpp"
Cohesion: 0.42
Nodes (8): compatibility_warning_codes_from_public_header(), path, string, vector, guide_has_warning_entry(), read_text_file(), source_root(), trim_copy()

### Community 147 - "validation_options"
Cohesion: 0.25
Nodes (8): archive_type, archive_variant, uint64_t, validation_options, expected_type, expected_variant, max_extractability_entry_bytes, validate_entry_extractability

### Community 148 - "collecting_sink"
Cohesion: 0.29
Nodes (7): collecting_sink, bytes_, byte, payload_sink, size_t, span, vector

### Community 149 - "tes4_bsa_writer_tests.cpp"
Cohesion: 0.38
Nodes (10): path, string, generated_source_dir(), non_ascii_output_path(), non_ascii_source_dir(), output_path(), target_name(), utf8_string_from_path() (+2 more)

### Community 150 - "thread_safety_docs_policy_tests.cpp"
Cohesion: 0.32
Nodes (7): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), source_root()

### Community 151 - "Distinct Sink Contract"
Cohesion: 0.33
Nodes (7): Bulk Extraction Contract, Bulk Extract Flow, Distinct Sink Contract, Isolated Object Ownership, archive_reader Concurrency, Validation Concurrency, Writer Object Concurrency

### Community 152 - "ba2_dx10_writer_options"
Cohesion: 0.29
Nodes (7): ba2_dx10_writer_options, deduplicate_payloads, max_decoded_chunk_bytes, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2

### Community 153 - "bulk_request_group"
Cohesion: 0.40
Nodes (5): bulk_request_group, path, result_indices, size_t, string

### Community 154 - "read_ba2_dx10_names"
Cohesion: 0.33
Nodes (5): string, uint32_t, uint64_t, vector, read_ba2_dx10_names()

### Community 155 - ".write"
Cohesion: 0.33
Nodes (5): byte, payload_sink, size_t, span, discard_payload_sink

### Community 156 - "TES5Edit Read-Only Boundary"
Cohesion: 0.40
Nodes (5): TES5Edit Read-Only CI Gate, TES5Edit Read-Only Boundary, TES5Edit Boundary, TES5Edit Reference Boundary, TES5Edit Fixture Boundary

### Community 157 - "GitHub Issues as Tracker"
Cohesion: 0.40
Nodes (5): GitHub Issue Tracker Contract, GitHub Issues as Tracker, Pull Requests Excluded from Triage, Wayfinder Issue Map, Canonical Triage Roles

### Community 158 - "archive_policy_expectation"
Cohesion: 0.40
Nodes (5): archive_policy_expectation, later_default, oblivion_default, policy, archive_compression_policy

### Community 159 - "find_source_root_from"
Cohesion: 0.80
Nodes (4): find_source_root_from(), path, is_source_root(), source_root()

### Community 160 - "host_file_writer_name_tests.cpp"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 162 - "read_text_file"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 163 - "read_text_file"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 164 - "fourcc"
Cohesion: 0.67
Nodes (3): array, byte, fourcc()

### Community 165 - "Validation Result and Report Contract"
Cohesion: 0.67
Nodes (3): Validation Result and Report Contract, Stable Result Error Branching, Validation Reports

### Community 166 - "Optional Local Corpus Evidence"
Cohesion: 0.67
Nodes (3): Optional Local Corpus Evidence, Deferred Compatibility Gaps, Optional Local Game Fixtures

## Knowledge Gaps
- **798 isolated node(s):** `scenario`, `worker_count`, `elapsed_ms`, `bytes_processed`, `correctness_passed` (+793 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `result` connect `result` to `bulk_extraction_tests.cpp`, `string`, `libbsa_benchmarks.cpp`, `ba2_record_identity.cpp`, `package-consumer/main.cpp`, `archive_reader_dispatch_tests.cpp`, `tes4_bsa_profile`, `dds_layout.cpp`, `host_path_correctness_boundary_tests.cpp`, `stored_payload.cpp`, `ba2_gnrl_reader_tests.cpp`, `tes3_bsa_reader_tests.cpp`, `tes3_bsa_writer_tests.cpp`, `tes3_prepared_entry`, `entry_metadata`, `tes4_bsa_reader_tests.cpp`, `tes4_bsa_profile.cpp`, `tes4_bsa_header_fields`, `local_game_fixture_tests.cpp`, `binary_reader`, `tes3_bsa_parser.cpp`, `ba2_archive_opening_tests.cpp`, `host_file.cpp`, `ba2_dx10_record`, `ba2_profile`, `byte`, `ba2_dx10_writer_tests.cpp`, `tes4_bsa_prepare.cpp`, `writer_publish_tests.cpp`, `archive_metadata`, `archive_reader::extract_entries`, `file_sink_factory`, `ba2_dx10_extraction_tests.cpp`, `cli/main.cpp`, `unordered_set`, `ba2_gnrl_prepare.cpp`, `writer_publish.cpp`, `byte`, `ba2_dx10_chunk_assembler.cpp`, `ba2_dx10_writer::state`, `ba2_gnrl_layout.cpp`, `ba2_gnrl_write_archive_bytes`, `validation.cpp`, `materialize_entries`, `ba2_dx10_write_archive_bytes`, `ba2_dx10_writer_entry`, `detected_bsa_format`, `host_file_path`, `string`, `path`, `materialize_entries`, `make_byte_vector`, `payload_stream.cpp`, `ba2_dx10_build_writer_entry_snapshot`, `make_tes4_bsa_payload_descriptor`, `tes4_bsa_serialize.cpp`, `write_ba2_gnrl_archive`, `tes3_bsa_writer.cpp`, `dedupe_key`, `payload_sink`, `ba2_archive_source`, `opened_ba2_archive`, `recording_sink`, `decompress_deflate_exact`, `decompress_payload_exact_to_sink`, `parser_primitives.cpp`, `collecting_sink`, `recording_sink`, `commit_staged`, `analyze_dds_source`, `writer_entry_compression`, `extract_file_payload`, `write_tes4_bsa_archive`, `normalize_archive_path`, `decompress_lz4_block_exact`, `ba2_dx10_subresource_snapshot`, `tes4_bsa_constants.hpp`, `collecting_sink`, `read_ba2_dx10_names`, `.write`?**
  _High betweenness centrality (0.298) - this node is a cross-community bridge._
- **Why does `tes4_bsa_profile` connect `tes4_bsa_profile` to `string`, `detected_bsa_format`, `tes4_bsa_prepare.cpp`, `make_tes4_bsa_payload_descriptor`, `writer_entry_compression`, `tes4_bsa_profile.cpp`, `tes4_bsa_header_fields`, `write_tes4_bsa_archive`, `writer_stage_tests.cpp`?**
  _High betweenness centrality (0.036) - this node is a cross-community bridge._
- **Why does `host_file_path` connect `host_file_path` to `string`, `host_file_path_tests.cpp`, `ba2_gnrl_writer_tests.cpp`, `stored_payload.cpp`, `ba2_gnrl_reader_tests.cpp`, `tes3_bsa_reader_tests.cpp`, `tes3_prepared_entry`, `entry_metadata`, `writer_stage_tests.cpp`, `tes3_bsa_parser.cpp`, `ba2_archive_opening_tests.cpp`, `host_file.cpp`, `tes4_bsa_prepare.cpp`, `archive_metadata`, `stored_payload_tests.cpp`, `ba2_dx10_extraction_tests.cpp`, `ba2_gnrl_prepare.cpp`, `ba2_gnrl_prepared_entry`, `ba2_dx10_writer::state`, `ba2_gnrl_layout.cpp`, `ba2_gnrl_write_archive_bytes`, `detected_bsa_format`, `host_file_tests.cpp`, `tes3_bsa_writer.cpp`, `payload_sink`, `ba2_archive_source`, `extract_file_payload`, `ba2_gnrl_writer.cpp`?**
  _High betweenness centrality (0.028) - this node is a cross-community bridge._
- **What connects `scenario`, `worker_count`, `elapsed_ms` to the rest of the system?**
  _798 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `bulk_extraction_tests.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.05798319327731093 - nodes in this community are weakly interconnected._
- **Should `string` be split into smaller, more focused modules?**
  _Cohesion score 0.07459505541346974 - nodes in this community are weakly interconnected._
- **Should `libbsa_benchmarks.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.07686453576864535 - nodes in this community are weakly interconnected._