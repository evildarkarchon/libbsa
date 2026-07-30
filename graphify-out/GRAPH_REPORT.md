# Graph Report - libbsa  (2026-07-29)

## Corpus Check
- 211 files · ~149,927 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3821 nodes · 8837 edges · 169 communities (168 shown, 1 thin omitted)
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 169 edges (avg confidence: 0.82)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `f8cbbf25`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- ba2_dx10_writer_tests.cpp
- bulk_extraction_tests.cpp
- string
- dds_layout.cpp
- Benchmarking and Performance
- TES4 BSA Archive Fixtures
- ba2_record_identity
- BA2 General Archive Fixtures
- package-consumer/main.cpp
- BA2 DX10 Reader Tests
- Archive Reader Dispatch Tests
- BA2 General Writer Tests
- Capture and Sink Testing
- Logical Reader and Storage
- BA2 General Reader Tests
- TES3 BSA Reader Tests
- TES3 BSA Writer Tests
- validation_archive_case
- TES3 BSA Archive Fixtures
- TES4 BSA Reader Tests
- TES4 BSA Ownership Policy
- ba2_archive_header
- tes4_bsa_header_fields
- TES3 BSA Writer Fixtures
- make_byte_vector
- tes3_bsa_parser.cpp
- writer_stage_tests.cpp
- archive_reader::extract_entries
- tes4_bsa_prepare.cpp
- ba2_writer_execution_tests.cpp
- Archive Type and Variant Parsing
- binary_reader
- ba2_archive_opening_tests.cpp
- Archive Compression Options
- ba2_dx10_prepared_entry
- tes4_placement_plan
- tes4_bsa_writer_tests.cpp
- ba2_dx10_record
- BA2 Compression and Profiles
- TES4 BSA Layout and Options
- Validation Policy Tests
- payload_stream_tests.cpp
- writer_publish_tests.cpp
- Compression Policy Management
- File Payload Sink Management
- stored_payload_tests.cpp
- BA2 DX10 Extraction Tests
- ba2_gnrl_writer_entry
- CLI Argument Parsing
- texture_metadata
- tes4_bsa_constants.hpp
- Finalization Workspace
- ba2_dx10_write_archive_bytes
- BA2 DX10 Chunk Snapshots
- validation_report
- tes3_bsa_writer.cpp
- ba2_dx10_writer::state
- ba2_gnrl_placed_record
- TES4 BSA Writer Execution
- Compatibility Warning Tests
- validation_options
- ba2_dx10_writer_entry
- validation_api_tests.cpp
- Command Line and Error Handling
- Archive CLI Argument Parsing
- writer.hpp
- materialize_entries
- Host File Tests and Handles
- tes3_writer_entry
- DDS Texture Metadata
- payload_stream.cpp
- ba2_archive_source
- Archive Compression Policies
- tes4_bsa_profile
- string_view
- Chunk Specification and Compression
- Manifest Validation Utilities
- byte
- BA2 and BSA Writer States
- writer_source_expectation
- BA2 DX10 Layout and Compression
- detected_bsa_format
- Stored Payload and File Records
- read_text_file
- File Path and JSON Utils
- materialize_entries
- LZ4 Frame Compression Codec
- Bethesda Hash Functions
- optional
- TES3 BSA Serialization Checks
- BA2 DX10 Fixture Generation
- Texture and Cube Map Metadata
- Payload Sink and Recording
- Byte Vector and Recording Sink
- read_text_file
- Payload Compression Router
- archive_path_key
- payload_sink
- Bulk Extraction Sink Management
- tes3_prepare_entries
- format_case
- Windows Handle Management
- unordered_set
- Archive Writer Policy Overview
- ba2_dx10_build_writer_entry_snapshot
- Archive Payload Handling
- tes4_writer_entry
- Archive Header Writing
- Writer Execution Testing
- string_view
- BA2 DX10 Placed Records
- result
- entry_metadata
- tes4_bsa_writer.cpp
- tes4_bsa_raw_table
- DDS Source Building
- Archive Reader Dispatch Tests
- TES4 BSA Compression Extraction
- BA2 and TES4 Target Policies
- libbsa Project and Packaging
- Public API and Documentation
- Archive Compression Metadata
- read_u32_le
- Writer Ownership Tests
- Build and Toolchain Scripts
- normalize_archive_path
- Coverage Audit Tests
- Documentation Policy Tests
- Host File Path Tests
- Parser Preparer Policy Tests
- BA2 General Writer Options
- BA2 DX10 Writer Options
- ba2_dx10_subresource_snapshot
- ba2_dx10_malformed_tests.cpp
- Benchmark Policy Tests
- Target Format Policy Tests
- Windows Development Policies
- Bulk Extraction Concurrency
- Test Fixture and Manifest Policies
- Issue #31 Finalization Query
- opened_ba2_archive
- Path and Command Line Utils
- Thread Safety Policy Tests
- BA2 Archive Catalog and Glossary
- host_file_path
- Payload Sink Interfaces
- DDS Mip and Block Sizes
- Source Root Detection
- Benchmarking and Performance Policy
- Compatibility Warning Evidence
- BA2 DX10 Compression Query
- ba2_dx10_placement_plan
- Issue Tracking and Triage
- Local Corpus and Compatibility Checks
- MSVC BA2 Plan Placement Query
- TES5Edit BA2 DX10 Compression Trace
- BA2 DX10 Chunk Layout
- deflate_vector
- DX10 Reference Compatibility Audit
- TES3 BSA Layout Checks
- ba2_profile
- Parser Primitive Tests
- BA2 DX10 Header Options
- Byte Array Utilities

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

## Communities (169 total, 1 thin omitted)

### Community 0 - "ba2_dx10_writer_tests.cpp"
Cohesion: 0.05
Nodes (86): set, dds_source_analysis, dds_bytes, image_payload_bytes, metadata, subresources, dds_source_subresource, array_index (+78 more)

### Community 1 - "bulk_extraction_tests.cpp"
Cohesion: 0.06
Nodes (67): archive_reader, state_, LIBBSA_API, shared_ptr, state, build_raw_ba2_dx10_fixture(), bulk_extraction_test_dir(), byte_buffer (+59 more)

### Community 2 - "string"
Cohesion: 0.08
Nodes (21): string, vector, span, append_byte_vector(), byte_vector_allocation_error(), byte, error, size_t (+13 more)

### Community 3 - "dds_layout.cpp"
Cohesion: 0.05
Nodes (80): append_chunks_for_ranges(), build_dds_dxt10_header(), capped_mip_ranges(), checked_add(), checked_mul(), byte, error, planned_texture_chunk (+72 more)

### Community 4 - "Benchmarking and Performance"
Cohesion: 0.08
Nodes (64): add_disk_payloads(), benchmark_result, bytes_processed, correctness_passed, elapsed_ms, scenario, worker_count, benchmark_sink_factory (+56 more)

### Community 5 - "TES4 BSA Archive Fixtures"
Cohesion: 0.07
Nodes (63): archive_spec, entries, file_flags, flags, folder, folder_hash, folder_offset, stem (+55 more)

### Community 6 - "ba2_record_identity"
Cohesion: 0.05
Nodes (77): ba2_record_identity_source, archive_default_compressed(), ba2_gnrl_make_writer_entry(), ba2_gnrl_prepare_entries(), ba2_gnrl_prepared_entry, archive_path_canonical, archive_path_original, directory_hash (+69 more)

### Community 7 - "BA2 General Archive Fixtures"
Cohesion: 0.08
Nodes (62): archive_spec, compression_method, entries, starfield_unknown1, starfield_unknown2, stem, variant, version (+54 more)

### Community 8 - "package-consumer/main.cpp"
Cohesion: 0.07
Nodes (58): archive_runtime_case, archive_host_path, archive_virtual_path, expect_texture_metadata, expected_ba2_compression_method, expected_default_compression, expected_payload, expected_type (+50 more)

### Community 9 - "BA2 DX10 Reader Tests"
Cohesion: 0.10
Nodes (60): append_ascii(), append_dx10_chunk_record(), append_dx10_record_for_path(), append_dx10_record_header(), append_u16_le(), append_u32_le(), append_u64_le(), append_u8() (+52 more)

### Community 10 - "Archive Reader Dispatch Tests"
Cohesion: 0.06
Nodes (53): archive_format_fixture, archive_filename, expected_extract_path, expected_missing_path, invalid_archive_path, manifest_filename, name, archive_format_fixtures() (+45 more)

### Community 11 - "BA2 General Writer Tests"
Cohesion: 0.07
Nodes (56): bytes_from_text(), compression_case, compression_method, expected_compression, file_name, target, version, archive_variant (+48 more)

### Community 12 - "Capture and Sink Testing"
Cohesion: 0.07
Nodes (52): bytes_from_hex(), capture_report, create_count_by_path, sink_bytes_by_path, capturing_sink, capture_, collecting_sink, bytes_ (+44 more)

### Community 13 - "Logical Reader and Storage"
Cohesion: 0.08
Nodes (43): logical_reader, byte, error, finalization_workspace, optional, ostream, size_t, span (+35 more)

### Community 14 - "BA2 General Reader Tests"
Cohesion: 0.09
Nodes (47): append_ascii(), append_u16_le(), append_u32_le(), append_u64_le(), archive_original_path_from_manifest(), ba2_success_fixture, archive, manifest (+39 more)

### Community 15 - "TES3 BSA Reader Tests"
Cohesion: 0.09
Nodes (47): append_u32_le(), append_u64_le(), archive_original_path_from_manifest(), build_synthetic_tes3_archive(), bytes_from_hex(), bytes_from_text(), checked_test_u32(), collecting_sink (+39 more)

### Community 16 - "TES3 BSA Writer Tests"
Cohesion: 0.10
Nodes (49): bytes_from_hex(), bytes_from_text(), collecting_sink, bytes_, byte, json, path, payload_sink (+41 more)

### Community 17 - "validation_archive_case"
Cohesion: 0.12
Nodes (17): archive_type, archive_variant, entry_compression, optional, require_metadata_matches_case(), require_valid_archive(), starfield_validation_writer_case, compression_method (+9 more)

### Community 18 - "TES3 BSA Archive Fixtures"
Cohesion: 0.13
Nodes (40): build_tes3_archive(), byte_buffer, bytes, bytes_from_string(), canonicalize(), checked_u32(), byte, path (+32 more)

### Community 19 - "TES4 BSA Reader Tests"
Cohesion: 0.10
Nodes (39): archive_original_path_from_manifest(), bytes_from_hex(), collecting_sink, bytes_, byte, entry_compression, error_code, json (+31 more)

### Community 20 - "TES4 BSA Ownership Policy"
Cohesion: 0.13
Nodes (39): branches_on_direct_tes4_identity(), code_only(), contains_integer_literal(), contains_word(), control_condition, keyword, text, control_conditions() (+31 more)

### Community 21 - "ba2_archive_header"
Cohesion: 0.18
Nodes (17): ba2_archive_header, ba2_archive_header::ba2_archive_header(), file_count, filename_table_offset, materialize_metadata, profile_, stored_metadata_, byte (+9 more)

### Community 22 - "tes4_bsa_header_fields"
Cohesion: 0.12
Nodes (34): add_fits(), size_t, uint32_t, multiply_fits(), span_fits(), table_size_for(), byte, size_t (+26 more)

### Community 23 - "TES3 BSA Writer Fixtures"
Cohesion: 0.13
Nodes (37): bytes_from_text(), canonicalize(), byte, path, size_t, span, string, string_view (+29 more)

### Community 24 - "make_byte_vector"
Cohesion: 0.07
Nodes (69): libdeflate_compressor, libdeflate_decompressor, make_byte_vector(), codec_error(), compress_deflate(), compressor_deleter, byte, error (+61 more)

### Community 25 - "tes3_bsa_parser.cpp"
Cohesion: 0.15
Nodes (30): byte, size_t, span, string, uint32_t, uint64_t, vector, file_record (+22 more)

### Community 26 - "writer_stage_tests.cpp"
Cohesion: 0.09
Nodes (42): ba2_dx10_prepared_stage_entries(), ba2_dx10_stage_entry(), ba2_gnrl_memory_stage_entry(), bytes_from_text(), byte, finalization_workspace, pair, path (+34 more)

### Community 27 - "archive_reader::extract_entries"
Cohesion: 0.11
Nodes (28): extract_entry_callback, archive_reader::contains(), archive_reader::extract(), archive_reader::extract_bytes(), archive_reader::extract_entries(), archive_reader::find(), archive_reader::state, entries (+20 more)

### Community 28 - "tes4_bsa_prepare.cpp"
Cohesion: 0.13
Nodes (34): append_u32_le(), checked_name_size(), checked_u32(), byte, entry_compression_policy, finalization_workspace, pair, size_t (+26 more)

### Community 29 - "ba2_writer_execution_tests.cpp"
Cohesion: 0.13
Nodes (34): add_dx10_sources(), add_gnrl_disk_sources(), bytes_from_text(), archive_variant, ba2_dx10_writer, ba2_gnrl_writer, byte, entry_compression (+26 more)

### Community 30 - "Archive Type and Variant Parsing"
Cohesion: 0.12
Nodes (29): archive_path_from_manifest(), archive_type_from_string(), archive_variant_from_string(), bsarchpro_expected_manifest_path(), bytes_from_hex(), archive_type, archive_variant, byte (+21 more)

### Community 31 - "binary_reader"
Cohesion: 0.14
Nodes (31): binary_reader, binary_reader::binary_reader(), bytes_, can_read, position, read_bytes, read_u16_le, read_u32_le (+23 more)

### Community 32 - "ba2_archive_opening_tests.cpp"
Cohesion: 0.10
Nodes (29): append_placeholder_gnrl_record(), append_u16_le(), append_u32_le(), append_u64_le(), archive_variant, ba2_subtype, byte, entry_compression (+21 more)

### Community 33 - "Archive Compression Options"
Cohesion: 0.15
Nodes (13): ba2_gnrl_writer_options, compression, deduplicate_payloads, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2, archive_compression_policy (+5 more)

### Community 34 - "ba2_dx10_prepared_entry"
Cohesion: 0.07
Nodes (30): ba2_dx10_prepared_chunk, compression, end_mip, packed_size, payload, raw_size, start_mip, ba2_dx10_prepared_entry (+22 more)

### Community 35 - "tes4_placement_plan"
Cohesion: 0.06
Nodes (47): size_t, stored_payload, string, tes4_folder_record_shape, uint32_t, uint64_t, vector, tes4_payload_placement (+39 more)

### Community 36 - "tes4_bsa_writer_tests.cpp"
Cohesion: 0.05
Nodes (66): tes4_bsa_target, tes4_bsa_writer::target(), archive_policy_expectation, later_default, oblivion_default, policy, archive_compression_policy, compression_method (+58 more)

### Community 37 - "ba2_dx10_record"
Cohesion: 0.07
Nodes (29): ba2_dx10_chunk_record, end_mip, offset, packed_size, raw_size, start_mip, ba2_dx10_record, chunk_count (+21 more)

### Community 38 - "BA2 Compression and Profiles"
Cohesion: 0.15
Nodes (29): ba2_compressed_payload_method(), ba2_profile::ba2_profile(), compressed_payload_method, default_compression, header_size, subtype, subtype_magic, variant (+21 more)

### Community 39 - "TES4 BSA Layout and Options"
Cohesion: 0.17
Nodes (21): add_fits_u64(), archive_flags_for(), calculate_table_lengths(), checked_name_size(), checked_stored_size(), checked_u32(), size_t, string_view (+13 more)

### Community 40 - "Validation Policy Tests"
Cohesion: 0.15
Nodes (29): command_succeeds(), compatibility_warning_codes_from_public_header(), count_occurrences(), array, initializer_list, optional, path, size_t (+21 more)

### Community 41 - "payload_stream_tests.cpp"
Cohesion: 0.14
Nodes (20): detail::payload_sink, detail::payload_source, byte, path, payload_sink, size_t, span, vector (+12 more)

### Community 42 - "writer_publish_tests.cpp"
Cohesion: 0.16
Nodes (18): bytes_from_text(), byte, path, span, string, string_view, vector, gnrl_disk_entry() (+10 more)

### Community 43 - "Compression Policy Management"
Cohesion: 0.18
Nodes (11): archive_compression_policy, entry_compression_policy, uint64_t, entry_compression, uint32_t, archive_default_compressed, writer_entry_compression, tes4_bsa_writer_options (+3 more)

### Community 44 - "File Payload Sink Management"
Cohesion: 0.11
Nodes (21): less, bulk_extract_sink_factory, map, mutex, ofstream, shared_ptr, discard_staged(), file_payload_sink (+13 more)

### Community 45 - "stored_payload_tests.cpp"
Cohesion: 0.13
Nodes (21): streambuf, streamsize, bytes_from_text(), collecting_stream_buffer, bytes_, byte, finalization_workspace, path (+13 more)

### Community 46 - "BA2 DX10 Extraction Tests"
Cohesion: 0.15
Nodes (23): ba2_dx10_fixture, archive, manifest, bytes_from_hex(), collecting_sink, bytes_, byte, json (+15 more)

### Community 47 - "ba2_gnrl_writer_entry"
Cohesion: 0.11
Nodes (22): ba2_gnrl_writer::ba2_gnrl_writer(), ba2_gnrl_writer_entry, archive_path_canonical, archive_path_original, from_memory, host_path, memory_bytes, options (+14 more)

### Community 48 - "CLI Argument Parsing"
Cohesion: 0.18
Nodes (24): add_help_argument(), add_positionals_argument(), add_thread_argument(), ascii_iequals(), ostream, string_view, dispatch(), final_path_for_handle() (+16 more)

### Community 49 - "texture_metadata"
Cohesion: 0.10
Nodes (21): entry_compression, uint16_t, uint64_t, uint8_t, texture_chunk_metadata, compression, end_mip, payload_offset (+13 more)

### Community 50 - "tes4_bsa_constants.hpp"
Cohesion: 0.10
Nodes (19): byte, span, detect_bsa_format(), size_t, uint32_t, entry_compression, PayloadReader, size_t (+11 more)

### Community 51 - "Finalization Workspace"
Cohesion: 0.20
Nodes (20): Finalize, finalization_workspace, path, size_t, string, string_view, finalization_workspace, cleanup (+12 more)

### Community 52 - "ba2_dx10_write_archive_bytes"
Cohesion: 0.20
Nodes (14): ba2_dx10_write_archive_bytes(), checked_u16(), checked_u32(), byte, ostream, path, span, string_view (+6 more)

### Community 53 - "BA2 DX10 Chunk Snapshots"
Cohesion: 0.18
Nodes (21): append_snapshot_bytes(), ba2_dx10_assemble_chunk(), ba2_dx10_assemble_planned_entry(), ba2_dx10_chunk_snapshot_batch, aggregate_size, snapshots, checked_size_t(), checked_u16() (+13 more)

### Community 54 - "validation_report"
Cohesion: 0.15
Nodes (27): vector, validation_report, errors, is_valid, metadata, valid, warnings, append_entry_warnings() (+19 more)

### Community 55 - "tes3_bsa_writer.cpp"
Cohesion: 0.18
Nodes (15): tes3_bsa_writer_options, overwrite_existing, byte, span, string_view, vector, tes3_bsa_writer::add_bytes(), tes3_bsa_writer::add_file() (+7 more)

### Community 56 - "ba2_dx10_writer::state"
Cohesion: 0.15
Nodes (19): ba2_dx10_writer::add_file(), ba2_dx10_writer::ba2_dx10_writer(), ba2_dx10_writer::state, consumed, entries, options, snapshot_dir, target (+11 more)

### Community 57 - "ba2_gnrl_placed_record"
Cohesion: 0.05
Nodes (51): add_fits_u64(), ba2_gnrl_dedupe_identity, fingerprint, stored_size, ba2_gnrl_payload_placement, offset, payload, stored_size (+43 more)

### Community 58 - "TES4 BSA Writer Execution"
Cohesion: 0.23
Nodes (20): add_tes4_sources(), bytes_from_text(), byte, pair, path, size_t, span, string (+12 more)

### Community 59 - "Compatibility Warning Tests"
Cohesion: 0.24
Nodes (20): bytes_from_text(), compatibility_warning_codes_from_public_header(), byte, compatibility_warning_code, path, size_t, string, string_view (+12 more)

### Community 60 - "validation_options"
Cohesion: 0.10
Nodes (21): compatibility_warning, archive_path, code, message, severity, archive_type, archive_variant, compatibility_warning_code (+13 more)

### Community 61 - "ba2_dx10_writer_entry"
Cohesion: 0.11
Nodes (20): ba2_dx10_make_writer_entry(), ba2_dx10_prepare_chunk(), ba2_dx10_prepare_entries(), ba2_dx10_validate_entries(), ba2_dx10_target, ba2_dx10_writer_options, path, planned_texture_chunk (+12 more)

### Community 62 - "validation_api_tests.cpp"
Cohesion: 0.26
Nodes (18): bytes_from_text(), compatibility_matrix_path(), path, uint32_t, generated_source_dir(), read_json_file(), require_starfield_v3_ba2_route(), temp_file_cleanup (+10 more)

### Community 63 - "Command Line and Error Handling"
Cohesion: 0.20
Nodes (19): command_line_arguments(), compatibility_warning_code, compatibility_warning_severity, error_code, payload_sink, string, unique_ptr, error_code_name() (+11 more)

### Community 64 - "Archive CLI Argument Parsing"
Cohesion: 0.29
Nodes (19): ArgumentParser, archive_type_name(), archive_variant_name(), compression_name(), archive_type, archive_variant, entry_compression, error (+11 more)

### Community 65 - "writer.hpp"
Cohesion: 0.24
Nodes (4): path, string, read_text_file(), source_root()

### Community 66 - "materialize_entries"
Cohesion: 0.08
Nodes (36): add_fits_u64(), archive_string_from_bytes(), byte, ifstream, span, string, string_view, uint64_t (+28 more)

### Community 67 - "Host File Tests and Handles"
Cohesion: 0.18
Nodes (17): bytes_from_text(), byte, DWORD, HANDLE, path, span, string_view, vector (+9 more)

### Community 68 - "tes3_writer_entry"
Cohesion: 0.20
Nodes (9): byte, string, vector, tes3_writer_entry, archive_path_canonical, archive_path_original, from_memory, host_path (+1 more)

### Community 69 - "DDS Texture Metadata"
Cohesion: 0.14
Nodes (14): source_dds_spec, archive_path, array_size, depth, file, format_id, format_name, height (+6 more)

### Community 70 - "payload_stream.cpp"
Cohesion: 0.21
Nodes (20): checked_materialized_payload_size(), checked_payload_size(), byte, ifstream, size_t, span, string_view, uint64_t (+12 more)

### Community 71 - "ba2_archive_source"
Cohesion: 0.15
Nodes (14): ba2_archive_session_context(), ba2_stable_archive_source, session_, byte, size_t, stable_host_file_session, string_view, uint64_t (+6 more)

### Community 72 - "Archive Compression Policies"
Cohesion: 0.26
Nodes (19): compression_choice, archive_policy_from(), collect_input_files(), archive_compression_policy, path, uint32_t, vector, generic_utf8_path() (+11 more)

### Community 73 - "tes4_bsa_profile"
Cohesion: 0.10
Nodes (34): profile_facts, archive_variant, compression_method, entry_compression, size_t, string_view, tes4_bsa_writer_options, tes4_folder_record_shape (+26 more)

### Community 74 - "string_view"
Cohesion: 0.36
Nodes (10): error_code, json, string_view, error_code_from_matrix(), generated_archive_dir(), report_has_error_code(), require_malformed_open_report(), require_matrix_extraction_report() (+2 more)

### Community 75 - "Chunk Specification and Compression"
Cohesion: 0.12
Nodes (17): chunk_spec, compression, decoded_payload, end_mip, packed_size, payload_offset, raw_size, segment (+9 more)

### Community 76 - "Manifest Validation Utilities"
Cohesion: 0.24
Nodes (16): Any, find_manifest_case(), load_json(), main(), Path, Validate malformed fixture case IDs, expected errors, and referenced archive…, Return a manifest case by ID, failing if the manifest case list is malformed or…, Validate the consolidated malformed matrix and its manifest/test evidence… (+8 more)

### Community 77 - "byte"
Cohesion: 0.36
Nodes (8): append_ascii(), append_u16_le(), append_u32_le(), append_u64_le(), byte, uint16_t, uint64_t, vector

### Community 78 - "BA2 and BSA Writer States"
Cohesion: 0.16
Nodes (17): ba2_dx10_writer, ba2_dx10_target, ba2_dx10_writer, state_, ba2_gnrl_writer, ba2_gnrl_target, ba2_gnrl_writer, state_ (+9 more)

### Community 79 - "writer_source_expectation"
Cohesion: 0.25
Nodes (8): writer_source_expectation, layout, prefix, preparation, profile, relative_source, serialization, validation

### Community 80 - "BA2 DX10 Layout and Compression"
Cohesion: 0.17
Nodes (14): add_fits_u64(), ba2_dx10_plan_placements(), compression_method, uint32_t, uint64_t, vector, dedupe_key, compression (+6 more)

### Community 81 - "detected_bsa_format"
Cohesion: 0.13
Nodes (23): detected_bsa_format, variant, version, archive_variant, uint32_t, byte, PayloadReader, size_t (+15 more)

### Community 82 - "Stored Payload and File Records"
Cohesion: 0.16
Nodes (16): stored_payload, string, uint32_t, uint64_t, vector, tes4_prepared_entry, canonical_folder, file_hash (+8 more)

### Community 83 - "read_text_file"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 84 - "File Path and JSON Utils"
Cohesion: 0.23
Nodes (13): ostringstream, canonicalize(), pair, string, string_view, extension_fourcc(), filename_stem(), hash_folder() (+5 more)

### Community 85 - "materialize_entries"
Cohesion: 0.14
Nodes (24): compression_for(), array, byte, entry_compression, span, string, uint32_t, uint64_t (+16 more)

### Community 86 - "LZ4 Frame Compression Codec"
Cohesion: 0.20
Nodes (14): LZ4F_dctx, compress_lz4_frame(), byte, error, ifstream, size_t, span, string_view (+6 more)

### Community 87 - "Bethesda Hash Functions"
Cohesion: 0.32
Nodes (15): string_view, uint32_t, uint64_t, uint8_t, crc32_entry(), crc32_lookup(), extension_magic(), hash_fo4() (+7 more)

### Community 88 - "optional"
Cohesion: 0.07
Nodes (32): archive_metadata, archive_flags, ba2, default_compression, file_count, type, variant, version (+24 more)

### Community 89 - "TES3 BSA Serialization Checks"
Cohesion: 0.29
Nodes (15): add_fits_u64(), checked_add_u32(), checked_mul_u32(), checked_u32(), byte, ofstream, path, span (+7 more)

### Community 90 - "BA2 DX10 Fixture Generation"
Cohesion: 0.35
Nodes (15): path, generate_malformed(), generate_success(), generate_writer_sources(), main(), make_duplicate_canonical_path_malformed(), make_fo4(), make_sfv3() (+7 more)

### Community 91 - "Texture and Cube Map Metadata"
Cohesion: 0.12
Nodes (16): uint8_t, texture_spec, array_size, chunks, cube_maps_raw, directory_hash, dxgi_format, ext (+8 more)

### Community 92 - "Payload Sink and Recording"
Cohesion: 0.17
Nodes (14): byte, path, payload_sink, size_t, span, string, vector, read_text_file() (+6 more)

### Community 93 - "Byte Vector and Recording Sink"
Cohesion: 0.18
Nodes (13): byte, path, payload_sink, size_t, span, vector, impossible_byte_vector_size(), lz4_vector() (+5 more)

### Community 94 - "read_text_file"
Cohesion: 0.50
Nodes (4): path, string, read_text_file(), source_root()

### Community 95 - "Payload Compression Router"
Cohesion: 0.30
Nodes (14): compress_payload(), copy_bytes(), byte, compression_method, error, ifstream, size_t, span (+6 more)

### Community 96 - "archive_path_key"
Cohesion: 0.67
Nodes (3): archive_path_key, value, string

### Community 98 - "Bulk Extraction Sink Management"
Cohesion: 0.14
Nodes (13): collecting_sink, bytes_, bulk_extract_sink_factory, byte, path, payload_sink, size_t, span (+5 more)

### Community 99 - "tes3_prepare_entries"
Cohesion: 0.24
Nodes (14): checked_u32(), span, string, string_view, uint32_t, uint64_t, vector, disk_payload_size() (+6 more)

### Community 100 - "format_case"
Cohesion: 0.15
Nodes (14): chunk(), byte, size_t, uint16_t, uint32_t, uint64_t, vector, format_case (+6 more)

### Community 101 - "Windows Handle Management"
Cohesion: 0.21
Nodes (11): clear_delete_on_close(), commit_staged(), byte, DWORD, HANDLE, size_t, span, mark_delete_on_close() (+3 more)

### Community 102 - "unordered_set"
Cohesion: 0.21
Nodes (13): Allocator, Hash, Key, KeyEqual, error, size_t, string_view, T (+5 more)

### Community 103 - "Archive Writer Policy Overview"
Cohesion: 0.24
Nodes (11): TES4 BSA Profile, Archive Family Writer Examples, BA2 DX10 Snapshot Lifecycle, BA2 DX10 Target Policies, BA2 GNRL Target Policies, Deflate Compression Route, LZ4 Frame Compression Route, Raw LZ4 Block Compression Route (+3 more)

### Community 105 - "ba2_dx10_build_writer_entry_snapshot"
Cohesion: 0.20
Nodes (17): ba2_dx10_build_writer_entry_snapshot(), ba2_dx10_ensure_snapshot_directory(), ba2_dx10_validate_texture_format_for_target(), ba2_dx10_target, byte, path, size_t, span (+9 more)

### Community 106 - "Archive Payload Handling"
Cohesion: 0.14
Nodes (14): byte, string, uint32_t, uint64_t, vector, tes3_prepared_entry, archive_path_original, from_memory (+6 more)

### Community 107 - "tes4_writer_entry"
Cohesion: 0.17
Nodes (11): byte, entry_compression_policy, string, vector, tes4_writer_entry, archive_path_canonical, archive_path_original, compression (+3 more)

### Community 108 - "Archive Header Writing"
Cohesion: 0.37
Nodes (7): build_archive(), byte_buffer, bytes, uint64_t, record_table_size(), write_header(), write_records()

### Community 109 - "Writer Execution Testing"
Cohesion: 0.29
Nodes (12): bytes_from_text(), byte, path, span, string_view, vector, generated_dx10_source_path(), output_path() (+4 more)

### Community 110 - "string_view"
Cohesion: 0.22
Nodes (11): path, span, string, string_view, vector, declaration_block(), public_declaration_lines(), read_text_file() (+3 more)

### Community 111 - "BA2 DX10 Placed Records"
Cohesion: 0.12
Nodes (16): ba2_dx10_placed_record, archive_path_original, chunk_count, chunks, cube_maps_raw, directory_hash, dxgi_format, extension (+8 more)

### Community 112 - "result"
Cohesion: 0.09
Nodes (28): DXGI_FORMAT, error, code, message, error_code, string, T, result (+20 more)

### Community 113 - "entry_metadata"
Cohesion: 0.11
Nodes (27): entry_metadata, archive_hash, compression, embedded_name_prefix_size, has_embedded_name, original_path, path, payload_offset (+19 more)

### Community 115 - "tes4_bsa_writer.cpp"
Cohesion: 0.16
Nodes (17): byte, entry_compression_policy, span, string_view, tes4_bsa_writer, tes4_bsa_writer_options, uint32_t, vector (+9 more)

### Community 116 - "tes4_bsa_raw_table"
Cohesion: 0.11
Nodes (22): size_t, string, uint32_t, uint64_t, vector, tes4_bsa_file_record, hash, offset (+14 more)

### Community 117 - "DDS Source Building"
Cohesion: 0.29
Nodes (11): build_source_dds(), bytes(), byte, initializer_list, size_t, span, vector, overwrite_u16() (+3 more)

### Community 118 - "Archive Reader Dispatch Tests"
Cohesion: 0.35
Nodes (11): optional, path, string, string_view, declaration_body(), extraction_function_member_name(), production_source_text(), read_text_file() (+3 more)

### Community 119 - "TES4 BSA Compression Extraction"
Cohesion: 0.23
Nodes (11): compression_method_for(), byte, compression_method, entry_compression, ifstream, span, uint32_t, extract_file_payload() (+3 more)

### Community 120 - "BA2 and TES4 Target Policies"
Cohesion: 0.17
Nodes (12): ba2_dx10_target, ba2_gnrl_target, format_descriptor, ba2_dx10_target, ba2_gnrl_target, description, family, supports_compressed (+4 more)

### Community 121 - "libbsa Project and Packaging"
Cohesion: 0.24
Nodes (11): libbsa Project Contract, Required Compression and Texture Dependency Policy, bsa CLI Target, libbsa Library Target, libbsa Install and Export Package, bsa Command-Line Tool, libbsa Public Capabilities, CLI Integration Test (+3 more)

### Community 122 - "Public API and Documentation"
Cohesion: 0.31
Nodes (11): Installed Package Consumer Smoke Proof, Public API Core, Default Compatibility Evidence Policy, Default Evidence Status Vocabulary, Archive Family Support Truth Matrix, Open List and Extract Example, Host and Archive Path Separation, Documentation-First API Stewardship (+3 more)

### Community 123 - "Archive Compression Metadata"
Cohesion: 0.17
Nodes (13): archive_spec, compression_method, starfield_unknown1, starfield_unknown2, stem, textures, variant, version (+5 more)

### Community 124 - "read_u32_le"
Cohesion: 0.20
Nodes (10): byte, path, size_t, span, string_view, uint32_t, vector, generated_archive_path() (+2 more)

### Community 125 - "Writer Ownership Tests"
Cohesion: 0.29
Nodes (10): bytes_from_text(), byte, path, span, string_view, vector, generated_source_dir(), require_extracted_bytes() (+2 more)

### Community 126 - "Build and Toolchain Scripts"
Cohesion: 0.27
Nodes (5): Assert-SafeCleanPath(), Get-SafePathForDisplay(), Invoke-CMakeBuildTarget(), Invoke-ExternalCommand(), Remove-BuildDirectory()

### Community 127 - "normalize_archive_path"
Cohesion: 0.48
Nodes (6): error, string_view, invalid_path_error(), is_drive_rooted(), lower_ascii(), normalize_archive_path()

### Community 129 - "Coverage Audit Tests"
Cohesion: 0.31
Nodes (9): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_markdown_section(), require_no_tokens() (+1 more)

### Community 130 - "Documentation Policy Tests"
Cohesion: 0.33
Nodes (9): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_no_planning_identifier_patterns(), require_no_tokens() (+1 more)

### Community 131 - "Host File Path Tests"
Cohesion: 0.40
Nodes (9): path, string, host_file_path_header_path(), host_file_path_source_path(), malformed_utf8_host_path(), project_root(), read_text_file(), unique_non_ascii_host_path() (+1 more)

### Community 132 - "Parser Preparer Policy Tests"
Cohesion: 0.31
Nodes (9): path, span, string, string_view, function_body(), read_text_file(), require_absent_tokens(), require_all_tokens() (+1 more)

### Community 133 - "BA2 General Writer Options"
Cohesion: 0.16
Nodes (15): ba2_gnrl_entry_options, compression, record_flags, entry_compression_policy, optional, uint32_t, write_execution_options, worker_count (+7 more)

### Community 135 - "BA2 DX10 Writer Options"
Cohesion: 0.29
Nodes (7): ba2_dx10_writer_options, deduplicate_payloads, max_decoded_chunk_bytes, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2

### Community 136 - "ba2_dx10_subresource_snapshot"
Cohesion: 0.20
Nodes (9): ba2_dx10_subresource_snapshot, array_index, face_index, mip, size, snapshot_path, path, uint32_t (+1 more)

### Community 137 - "ba2_dx10_malformed_tests.cpp"
Cohesion: 0.16
Nodes (15): collecting_sink, bytes_, byte, error_code, json, path, payload_sink, size_t (+7 more)

### Community 138 - "Benchmark Policy Tests"
Cohesion: 0.33
Nodes (8): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_no_tokens(), source_root()

### Community 140 - "Target Format Policy Tests"
Cohesion: 0.42
Nodes (8): compatibility_warning_codes_from_public_header(), path, string, vector, guide_has_warning_entry(), read_text_file(), source_root(), trim_copy()

### Community 141 - "Windows Development Policies"
Cohesion: 0.29
Nodes (8): TES5Edit Read-Only CI Guard, Windows MSVC Verification Matrix, TES5Edit Read-Only Boundary, Windows-Only Platform Policy, Windows Development and Reference Contract, MSVC AddressSanitizer Instrumentation, Supported Windows Verification Lanes, Fixture Provenance and TES5Edit Boundary

### Community 142 - "Bulk Extraction Concurrency"
Cohesion: 0.29
Nodes (8): Bulk Extraction Contract, Validation Result and Report Contract, Bulk Extraction Example, Validate Archive Example, Archive Reader Concurrency Contract, Bulk Extraction Sink Concurrency Contract, Ownership-Isolated Thread Safety Model, Writer and Validation Concurrency Contract

### Community 143 - "Test Fixture and Manifest Policies"
Cohesion: 0.25
Nodes (8): Malformed Hardening Submatrix, Archive Fixture Generator Targets, Fixture Manifest Validation Test, Internal Test Support Linkage, libbsa Tests Target, Committed Generated Fixture Policy, Fixture Manifest Contracts, CTest Fixture Label Taxonomy

### Community 144 - "Issue #31 Finalization Query"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: Implement libbsa issue #31 shared finalization boundary, Source Nodes

### Community 145 - "opened_ba2_archive"
Cohesion: 0.40
Nodes (5): vector, opened_ba2_archive, entries, metadata, subtype

### Community 146 - "Path and Command Line Utils"
Cohesion: 0.27
Nodes (9): final_path_is_within_root(), is_separator(), local_command_line_argv, value, same_windows_prefix(), trim_final_path(), wchar_t, wstring (+1 more)

### Community 147 - "Thread Safety Policy Tests"
Cohesion: 0.32
Nodes (7): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), source_root()

### Community 149 - "BA2 Archive Catalog and Glossary"
Cohesion: 0.25
Nodes (8): Archive Entry Catalog, BA2 Archive Header, BA2 Archive Opening, BA2 Profile, BA2 Record Identity, Stored Payload, Glossary Vocabulary Policy, Single-Context Domain Layout

### Community 150 - "host_file_path"
Cohesion: 0.19
Nodes (9): archive_file_size(), archive_open_host_context(), archive_reader::open(), read_detection_prefix(), string_view, host_file_path, resolved, path (+1 more)

### Community 151 - "Payload Sink Interfaces"
Cohesion: 0.33
Nodes (5): byte, payload_sink, size_t, span, discard_payload_sink

### Community 154 - "DDS Mip and Block Sizes"
Cohesion: 0.43
Nodes (7): bytes_per_block(), checked_u32(), uint32_t, dds_mip_size(), is_block_compressed(), mip_dimension(), name_table_size()

### Community 157 - "Source Root Detection"
Cohesion: 0.80
Nodes (4): find_source_root_from(), path, is_source_root(), source_root()

### Community 158 - "Benchmarking and Performance Policy"
Cohesion: 0.50
Nodes (4): Report-Only Performance Policy, Synthetic Benchmark Harness, libbsa Benchmark Report Target, libbsa Benchmarks Target

### Community 159 - "Compatibility Warning Evidence"
Cohesion: 0.50
Nodes (4): BSA Embedded Name Compatibility Risk Evidence, Compressed Sound Payload Warning Evidence, Target Family Mismatch Warning Evidence, Compatibility Warning Policy

### Community 161 - "BA2 DX10 Compression Query"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: How do BA2 DX10 prepared chunks compression finalization deduplication serialization work and where are Stored Payload integration points?, Source Nodes

### Community 162 - "ba2_dx10_placement_plan"
Cohesion: 0.21
Nodes (11): ba2_dx10_payload_placement, offset, payload, stored_size, ba2_dx10_placement_plan, filename_table_offset, payloads, records (+3 more)

### Community 163 - "Issue Tracking and Triage"
Cohesion: 0.67
Nodes (3): GitHub Issue Tracker, Wayfinder Issue Map, Canonical Triage States

### Community 164 - "Local Corpus and Compatibility Checks"
Cohesion: 0.67
Nodes (3): Optional Local Corpus Checks, Deferred Compatibility Gaps, Local Game Fixture Policy

### Community 170 - "MSVC BA2 Plan Placement Query"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: MSVC C2039/C3861: ba2_gnrl_plan_placements is not a member of libbsa::formats::ba2 in writer_stage_tests.cpp, Source Nodes

### Community 172 - "TES5Edit BA2 DX10 Compression Trace"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33, Source Nodes

### Community 173 - "BA2 DX10 Chunk Layout"
Cohesion: 0.18
Nodes (11): ba2_dx10_placed_chunk, compression, end_mip, packed_size, payload_index, raw_size, start_mip, compression_method (+3 more)

### Community 174 - "deflate_vector"
Cohesion: 0.67
Nodes (3): byte, vector, deflate_vector()

### Community 175 - "DX10 Reference Compatibility Audit"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: Issue 33 DX10 reference compatibility audit, Source Nodes

### Community 176 - "TES3 BSA Layout Checks"
Cohesion: 0.38
Nodes (9): add_fits_u64(), checked_add_u32(), checked_mul_u32(), checked_u32(), span, string_view, uint32_t, uint64_t (+1 more)

### Community 178 - "ba2_profile"
Cohesion: 0.11
Nodes (15): ba2_subtype, ba2_dx10_writer_options, ba2_gnrl_writer_options, ba2_profile, compressed_method_, is_dx10, is_gnrl, archive_variant (+7 more)

### Community 179 - "Parser Primitive Tests"
Cohesion: 0.31
Nodes (7): path, size_t, impossible_set_capacity(), impossible_string_size(), impossible_vector_capacity(), temp_file_cleanup, path_

### Community 181 - "BA2 DX10 Header Options"
Cohesion: 0.33
Nodes (5): ba2_dx10_stored_header_options, starfield_compression_method, starfield_unknown1, starfield_unknown2, uint32_t

### Community 185 - "Byte Array Utilities"
Cohesion: 0.67
Nodes (3): array, byte, fourcc()

## Knowledge Gaps
- **820 isolated node(s):** `scenario`, `worker_count`, `elapsed_ms`, `bytes_processed`, `correctness_passed` (+815 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Work-memory lessons

**Known dead ends** — questions that led nowhere; don't re-derive.
- "Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33" -> `ba2_dx10_serialize.cpp`, `packed_size`, `writer.hpp`

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `result` connect `result` to `ba2_dx10_writer_tests.cpp`, `bulk_extraction_tests.cpp`, `string`, `dds_layout.cpp`, `Benchmarking and Performance`, `ba2_record_identity`, `package-consumer/main.cpp`, `Archive Reader Dispatch Tests`, `Capture and Sink Testing`, `Logical Reader and Storage`, `BA2 General Reader Tests`, `TES3 BSA Reader Tests`, `TES3 BSA Writer Tests`, `TES4 BSA Reader Tests`, `ba2_archive_header`, `tes4_bsa_header_fields`, `make_byte_vector`, `tes3_bsa_parser.cpp`, `archive_reader::extract_entries`, `tes4_bsa_prepare.cpp`, `Archive Type and Variant Parsing`, `binary_reader`, `ba2_archive_opening_tests.cpp`, `tes4_placement_plan`, `tes4_bsa_writer_tests.cpp`, `ba2_dx10_record`, `BA2 Compression and Profiles`, `TES4 BSA Layout and Options`, `payload_stream_tests.cpp`, `writer_publish_tests.cpp`, `Compression Policy Management`, `File Payload Sink Management`, `BA2 DX10 Extraction Tests`, `ba2_gnrl_writer_entry`, `CLI Argument Parsing`, `tes4_bsa_constants.hpp`, `Finalization Workspace`, `ba2_dx10_write_archive_bytes`, `BA2 DX10 Chunk Snapshots`, `validation_report`, `tes3_bsa_writer.cpp`, `ba2_dx10_writer::state`, `ba2_gnrl_placed_record`, `validation_options`, `ba2_dx10_writer_entry`, `Command Line and Error Handling`, `writer.hpp`, `materialize_entries`, `payload_stream.cpp`, `ba2_archive_source`, `Archive Compression Policies`, `tes4_bsa_profile`, `BA2 DX10 Layout and Compression`, `detected_bsa_format`, `materialize_entries`, `LZ4 Frame Compression Codec`, `optional`, `TES3 BSA Serialization Checks`, `Payload Sink and Recording`, `Byte Vector and Recording Sink`, `Payload Compression Router`, `Bulk Extraction Sink Management`, `tes3_prepare_entries`, `Windows Handle Management`, `unordered_set`, `ba2_dx10_build_writer_entry_snapshot`, `entry_metadata`, `tes4_bsa_writer.cpp`, `tes4_bsa_raw_table`, `TES4 BSA Compression Extraction`, `normalize_archive_path`, `BA2 General Writer Options`, `ba2_dx10_malformed_tests.cpp`, `host_file_path`, `Payload Sink Interfaces`, `ba2_dx10_placement_plan`, `TES3 BSA Layout Checks`, `ba2_profile`?**
  _High betweenness centrality (0.324) - this node is a cross-community bridge._
- **Why does `host_file_path` connect `host_file_path` to `string`, `Host File Path Tests`, `ba2_record_identity`, `Logical Reader and Storage`, `BA2 General Reader Tests`, `TES3 BSA Reader Tests`, `make_byte_vector`, `tes3_bsa_parser.cpp`, `writer_stage_tests.cpp`, `archive_reader::extract_entries`, `tes4_bsa_prepare.cpp`, `ba2_archive_opening_tests.cpp`, `stored_payload_tests.cpp`, `BA2 DX10 Extraction Tests`, `ba2_gnrl_writer_entry`, `tes3_bsa_writer.cpp`, `ba2_dx10_writer::state`, `Host File Tests and Handles`, `ba2_archive_source`, `detected_bsa_format`, `TES3 BSA Serialization Checks`, `tes3_prepare_entries`, `Archive Payload Handling`, `entry_metadata`, `tes4_bsa_writer.cpp`, `TES4 BSA Compression Extraction`?**
  _High betweenness centrality (0.025) - this node is a cross-community bridge._
- **Why does `tes4_bsa_profile` connect `tes4_bsa_profile` to `string`, `tes4_bsa_writer_tests.cpp`, `TES4 BSA Layout and Options`, `Compression Policy Management`, `detected_bsa_format`, `tes4_bsa_constants.hpp`, `tes4_bsa_writer.cpp`, `tes4_bsa_raw_table`, `tes4_bsa_header_fields`, `writer_stage_tests.cpp`, `tes4_bsa_prepare.cpp`?**
  _High betweenness centrality (0.025) - this node is a cross-community bridge._
- **What connects `scenario`, `worker_count`, `elapsed_ms` to the rest of the system?**
  _820 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `ba2_dx10_writer_tests.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.05268414481897628 - nodes in this community are weakly interconnected._
- **Should `bulk_extraction_tests.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.05798319327731093 - nodes in this community are weakly interconnected._
- **Should `string` be split into smaller, more focused modules?**
  _Cohesion score 0.08385744234800839 - nodes in this community are weakly interconnected._