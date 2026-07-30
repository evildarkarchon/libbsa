# Graph Report - .  (2026-07-29)

## Corpus Check
- cluster-only mode — file stats not available

## Summary
- 3819 nodes · 8832 edges · 185 communities
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 169 edges (avg confidence: 0.82)
- Token cost: 8,698 input · 1,973 output

## Graph Freshness
- Built from commit: `f8cbbf25`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- BA2 DX10 Writer Metadata
- Bulk Archive Extraction
- Archive Interface Headers
- DDS Texture Chunk Handling
- Benchmarking and Performance
- TES4 BSA Archive Fixtures
- BA2 Record Identity Management
- BA2 General Archive Fixtures
- Byte Buffer and Vector Sinks
- BA2 DX10 Reader Tests
- Archive Reader Dispatch Tests
- BA2 General Writer Tests
- Capture and Sink Testing
- Logical Reader and Storage
- BA2 General Reader Tests
- TES3 BSA Reader Tests
- TES3 BSA Writer Tests
- TES4 Compression and Profiles
- TES3 BSA Archive Fixtures
- TES4 BSA Reader Tests
- TES4 BSA Ownership Policy
- BA2 Archive Metadata
- TES4 BSA Archive Parsing
- TES3 BSA Writer Fixtures
- Buffer and File Size Checks
- TES3 BSA Parsing Utilities
- Writer Stage and Payloads
- Archive Extraction Operations
- TES4 BSA Entry Preparation
- DDS Source and Metadata
- Archive Type and Variant Parsing
- Binary Reader Utilities
- BA2 Archive Opening Tests
- Archive Compression Options
- BA2 DX10 Archive Layout
- TES4 Payload and Folder Records
- Text and Payload Sink Handling
- BA2 DX10 Chunk Records
- BA2 Compression and Profiles
- TES4 BSA Layout and Options
- Validation Policy Tests
- Payload Sink and Source Details
- Writer Publish Tests
- Compression Policy Management
- File Payload Sink Management
- Stream Buffer and Finalization
- BA2 DX10 Extraction Tests
- BA2 GNRL Archive Writing
- CLI Argument Parsing
- Bulk Extraction Options
- TES4 BSA Payload Descriptor
- Finalization Workspace
- BA2 DX10 Write Archive
- BA2 DX10 Chunk Snapshots
- Validation and Warning Handling
- Atomic File Operations
- BA2 DX10 Writer State
- BA2 General Payload Placement
- TES4 BSA Writer Execution
- Compatibility Warning Tests
- Compatibility Warning Diagnostics
- BA2 DX10 Chunk Preparation
- Validation API Tests
- Command Line and Error Handling
- Archive CLI Argument Parsing
- TES4 BSA Writer Targets
- BA2 DX10 Parsing and Compression
- Host File Tests and Handles
- BA2 DX10 Preparer Tests
- DDS Texture Metadata
- Payload Size Validation
- BA2 Stable Archive Source
- Archive Compression Policies
- TES4 BSA Profile Management
- BA2 General Prepared Entries
- Chunk Specification and Compression
- Manifest Validation Utilities
- BA2 General Write Operations
- BA2 and BSA Writer States
- TES4 BSA Serialization Checks
- BA2 DX10 Layout and Compression
- BSA Format Detection
- Stored Payload and File Records
- BA2 GNRL Entry Validation
- File Path and JSON Utils
- Compression and Directory Metadata
- LZ4 Frame Compression Codec
- Bethesda Hash Functions
- Archive Metadata and Flags
- TES3 BSA Serialization Checks
- BA2 DX10 Fixture Generation
- Texture and Cube Map Metadata
- Payload Sink and Recording
- Byte Vector and Recording Sink
- Deflate Compression Codec
- Payload Compression Router
- File Parsing and Normalization
- BA2 GNRL Deduplication
- Bulk Extraction Sink Management
- TES3 BSA Writer Options
- Data Types and Formats
- Windows Handle Management
- Memory Allocation and Hashing
- Archive Writer Policy Overview
- DDS Metadata Analysis
- BA2 DX10 Writer Entry Validation
- Archive Payload Handling
- TES4 BSA Entry Validation
- Archive Header Writing
- Writer Execution Testing
- Writer Hotspot Policy Tests
- BA2 DX10 Placed Records
- Result and Error Handling
- entry_metadata
- TES4 BSA Writer Tests
- TES4 BSA Writer Core
- TES4 BSA Folder Metadata
- DDS Source Building
- Archive Reader Dispatch Tests
- TES4 BSA Compression Extraction
- BA2 and TES4 Target Policies
- libbsa Project and Packaging
- Public API and Documentation
- Archive Compression Metadata
- TES4 BSA Parser Tests
- Writer Ownership Tests
- Build and Toolchain Scripts
- Archive Path Normalization
- LZ4 Block Compression
- Coverage Audit Tests
- Documentation Policy Tests
- Host File Path Tests
- Parser Preparer Policy Tests
- BA2 General Writer Options
- File Session Diagnostics
- BA2 DX10 Writer Options
- BA2 DX10 Subresource Snapshots
- BA2 DX10 Error Handling Tests
- Benchmark Policy Tests
- Native Handle Management
- Target Format Policy Tests
- Windows Development Policies
- Bulk Extraction Concurrency
- Test Fixture and Manifest Policies
- Issue #31 Finalization Query
- BA2 Archive Session Management
- Path and Command Line Utils
- Thread Safety Policy Tests
- BA2 DX10 Chunk Compression
- BA2 Archive Catalog and Glossary
- Host File Path and IO
- Payload Sink Interfaces
- Archive Runtime Cases
- Archive Entry Preparation
- DDS Mip and Block Sizes
- BA2 Payload Extraction
- Archive Variant and Compression
- Source Root Detection
- Benchmarking and Performance Policy
- Compatibility Warning Evidence
- Bulk Extraction Results
- BA2 DX10 Compression Query
- BA2 DX10 Payload Placement
- Issue Tracking and Triage
- Local Corpus and Compatibility Checks
- MSVC BA2 Plan Placement Query
- TES5Edit BA2 DX10 Compression Trace
- BA2 DX10 Chunk Layout
- Deflate Codec Tests
- DX10 Reference Compatibility Audit
- TES3 BSA Layout Checks
- Byte Vector Management
- ba2_profile
- Parser Primitive Tests
- Archive Validation Options
- BA2 DX10 Header Options
- Payload Sink Management
- Payload Source Management
- BA2 DX10 Name Handling
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

## Communities (185 total, 0 thin omitted)

### Community 0 - "BA2 DX10 Writer Metadata"
Cohesion: 0.07
Nodes (73): set, add_duplicate_dds_pair(), add_matrix_cases(), ba2_dx10_record_metadata, directory_hash, extension, filename_table_path, name_hash (+65 more)

### Community 1 - "Bulk Archive Extraction"
Cohesion: 0.06
Nodes (69): archive_reader, state_, bulk_extract_request, path, LIBBSA_API, shared_ptr, state, build_raw_ba2_dx10_fixture() (+61 more)

### Community 2 - "Archive Interface Headers"
Cohesion: 0.07
Nodes (24): bulk_extract_sink_factory(), string, string_view, array, stored_payload, planned_texture_chunk, path, string (+16 more)

### Community 3 - "DDS Texture Chunk Handling"
Cohesion: 0.08
Nodes (58): append_chunks_for_ranges(), build_dds_dxt10_header(), capped_mip_ranges(), checked_add(), checked_mul(), byte, error, planned_texture_chunk (+50 more)

### Community 4 - "Benchmarking and Performance"
Cohesion: 0.08
Nodes (64): add_disk_payloads(), benchmark_result, bytes_processed, correctness_passed, elapsed_ms, scenario, worker_count, benchmark_sink_factory (+56 more)

### Community 5 - "TES4 BSA Archive Fixtures"
Cohesion: 0.07
Nodes (63): archive_spec, entries, file_flags, flags, folder, folder_hash, folder_offset, stem (+55 more)

### Community 6 - "BA2 Record Identity Management"
Cohesion: 0.11
Nodes (44): ba2_record_identity_source, ascii_lower_byte(), ba2_record_identity, canonical_path, directory_hash, display_path, extension, name_hash (+36 more)

### Community 7 - "BA2 General Archive Fixtures"
Cohesion: 0.08
Nodes (62): archive_spec, compression_method, entries, starfield_unknown1, starfield_unknown2, stem, variant, version (+54 more)

### Community 8 - "Byte Buffer and Vector Sinks"
Cohesion: 0.10
Nodes (41): build_tiny_bc1_dds_dxt10_source(), byte_buffer, bytes, byte_vector_sink, bytes_, byte_vector_sink_factory, mutex_, observed_entries_ (+33 more)

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

### Community 17 - "TES4 Compression and Profiles"
Cohesion: 0.13
Nodes (15): compression_method, entry_compression, size_t, tes4_folder_record_shape, uint32_t, profile_expectation, compressed_entry_metadata, compressed_payload_method (+7 more)

### Community 18 - "TES3 BSA Archive Fixtures"
Cohesion: 0.13
Nodes (40): build_tes3_archive(), byte_buffer, bytes, bytes_from_string(), canonicalize(), checked_u32(), byte, path (+32 more)

### Community 19 - "TES4 BSA Reader Tests"
Cohesion: 0.10
Nodes (39): archive_original_path_from_manifest(), bytes_from_hex(), collecting_sink, bytes_, byte, entry_compression, error_code, json (+31 more)

### Community 20 - "TES4 BSA Ownership Policy"
Cohesion: 0.13
Nodes (39): branches_on_direct_tes4_identity(), code_only(), contains_integer_literal(), contains_word(), control_condition, keyword, text, control_conditions() (+31 more)

### Community 21 - "BA2 Archive Metadata"
Cohesion: 0.14
Nodes (21): ba2_archive_metadata, compression_method, starfield_unknown1, starfield_unknown2, ba2_archive_header, ba2_archive_header::ba2_archive_header(), file_count, filename_table_offset (+13 more)

### Community 22 - "TES4 BSA Archive Parsing"
Cohesion: 0.12
Nodes (33): size_t, uint32_t, multiply_fits(), span_fits(), parse_tes4_bsa_archive_file(), byte, size_t, span (+25 more)

### Community 23 - "TES3 BSA Writer Fixtures"
Cohesion: 0.13
Nodes (37): bytes_from_text(), canonicalize(), byte, path, size_t, span, string, string_view (+29 more)

### Community 24 - "Buffer and File Size Checks"
Cohesion: 0.19
Nodes (36): checked_buffer_size(), byte, error, function, ifstream, path, size_t, span (+28 more)

### Community 25 - "TES3 BSA Parsing Utilities"
Cohesion: 0.15
Nodes (31): byte, size_t, span, string, uint32_t, uint64_t, vector, file_record (+23 more)

### Community 26 - "Writer Stage and Payloads"
Cohesion: 0.10
Nodes (41): ba2_dx10_prepared_stage_entries(), ba2_dx10_stage_entry(), ba2_gnrl_memory_stage_entry(), bytes_from_text(), byte, finalization_workspace, pair, path (+33 more)

### Community 27 - "Archive Extraction Operations"
Cohesion: 0.12
Nodes (32): extract_entry_callback, archive_file_size(), archive_open_host_context(), archive_reader::contains(), archive_reader::extract(), archive_reader::extract_bytes(), archive_reader::extract_entries(), archive_reader::find() (+24 more)

### Community 28 - "TES4 BSA Entry Preparation"
Cohesion: 0.14
Nodes (33): append_u32_le(), checked_name_size(), checked_u32(), byte, entry_compression_policy, finalization_workspace, pair, size_t (+25 more)

### Community 29 - "DDS Source and Metadata"
Cohesion: 0.08
Nodes (47): dds_source_analysis, dds_bytes, image_payload_bytes, metadata, subresources, dds_source_subresource, array_index, bytes (+39 more)

### Community 30 - "Archive Type and Variant Parsing"
Cohesion: 0.12
Nodes (29): archive_path_from_manifest(), archive_type_from_string(), archive_variant_from_string(), bsarchpro_expected_manifest_path(), bytes_from_hex(), archive_type, archive_variant, byte (+21 more)

### Community 31 - "Binary Reader Utilities"
Cohesion: 0.14
Nodes (31): binary_reader, binary_reader::binary_reader(), bytes_, can_read, position, read_bytes, read_u16_le, read_u32_le (+23 more)

### Community 32 - "BA2 Archive Opening Tests"
Cohesion: 0.19
Nodes (17): append_placeholder_gnrl_record(), append_u16_le(), append_u32_le(), append_u64_le(), byte, path, size_t, span (+9 more)

### Community 33 - "Archive Compression Options"
Cohesion: 0.15
Nodes (13): ba2_gnrl_writer_options, compression, deduplicate_payloads, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2, archive_compression_policy (+5 more)

### Community 34 - "BA2 DX10 Archive Layout"
Cohesion: 0.11
Nodes (19): ba2_dx10_prepared_entry, archive_path_canonical, archive_path_original, chunk_count, chunks, cube_maps_raw, directory_hash, dxgi_format (+11 more)

### Community 35 - "TES4 Payload and Folder Records"
Cohesion: 0.08
Nodes (31): size_t, stored_payload, string, tes4_folder_record_shape, uint32_t, uint64_t, vector, tes4_payload_placement (+23 more)

### Community 36 - "Text and Payload Sink Handling"
Cohesion: 0.19
Nodes (19): bytes_from_text(), collecting_sink, bytes_, byte, payload_sink, size_t, span, string_view (+11 more)

### Community 37 - "BA2 DX10 Chunk Records"
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

### Community 41 - "Payload Sink and Source Details"
Cohesion: 0.13
Nodes (20): detail::payload_sink, detail::payload_source, byte, path, payload_sink, size_t, span, vector (+12 more)

### Community 42 - "Writer Publish Tests"
Cohesion: 0.11
Nodes (26): bytes_from_text(), byte, path, span, string, string_view, vector, gnrl_disk_entry() (+18 more)

### Community 43 - "Compression Policy Management"
Cohesion: 0.18
Nodes (11): archive_compression_policy, entry_compression_policy, uint64_t, entry_compression, uint32_t, archive_default_compressed, writer_entry_compression, tes4_bsa_writer_options (+3 more)

### Community 44 - "File Payload Sink Management"
Cohesion: 0.11
Nodes (21): less, bulk_extract_sink_factory, map, mutex, ofstream, shared_ptr, discard_staged(), file_payload_sink (+13 more)

### Community 45 - "Stream Buffer and Finalization"
Cohesion: 0.11
Nodes (21): streambuf, streamsize, bytes_from_text(), collecting_stream_buffer, bytes_, byte, finalization_workspace, path (+13 more)

### Community 46 - "BA2 DX10 Extraction Tests"
Cohesion: 0.15
Nodes (23): ba2_dx10_fixture, archive, manifest, bytes_from_hex(), collecting_sink, bytes_, byte, json (+15 more)

### Community 47 - "BA2 GNRL Archive Writing"
Cohesion: 0.20
Nodes (12): ba2_gnrl_writer::ba2_gnrl_writer(), ba2_gnrl_writer::state, entries, options, target, ba2_gnrl_writer::target(), ba2_gnrl_target, ba2_gnrl_writer (+4 more)

### Community 48 - "CLI Argument Parsing"
Cohesion: 0.18
Nodes (24): add_help_argument(), add_positionals_argument(), add_thread_argument(), ascii_iequals(), ostream, string_view, dispatch(), final_path_for_handle() (+16 more)

### Community 49 - "Bulk Extraction Options"
Cohesion: 0.09
Nodes (24): bulk_extract_options, worker_count, entry_compression, uint16_t, uint32_t, uint64_t, uint8_t, texture_chunk_metadata (+16 more)

### Community 50 - "TES4 BSA Payload Descriptor"
Cohesion: 0.08
Nodes (26): size_t, uint32_t, entry_compression, PayloadReader, size_t, uint32_t, uint64_t, make_tes4_bsa_payload_descriptor() (+18 more)

### Community 51 - "Finalization Workspace"
Cohesion: 0.20
Nodes (20): Finalize, finalization_workspace, path, size_t, string, string_view, finalization_workspace, cleanup (+12 more)

### Community 52 - "BA2 DX10 Write Archive"
Cohesion: 0.19
Nodes (14): ba2_dx10_write_archive_bytes(), checked_u16(), checked_u32(), byte, ostream, path, span, string_view (+6 more)

### Community 53 - "BA2 DX10 Chunk Snapshots"
Cohesion: 0.18
Nodes (21): append_snapshot_bytes(), ba2_dx10_assemble_chunk(), ba2_dx10_assemble_planned_entry(), ba2_dx10_chunk_snapshot_batch, aggregate_size, snapshots, checked_size_t(), checked_u16() (+13 more)

### Community 54 - "Validation and Warning Handling"
Cohesion: 0.20
Nodes (20): append_entry_warnings(), append_fatal(), append_target_family_warning(), append_warning(), ascii_lowercase(), compatibility_warning_code, compatibility_warning_severity, error (+12 more)

### Community 55 - "Atomic File Operations"
Cohesion: 0.09
Nodes (18): T, result, storage_, path, publish_file_without_replace(), replace_file_atomically(), function, size_t (+10 more)

### Community 56 - "BA2 DX10 Writer State"
Cohesion: 0.13
Nodes (19): ba2_dx10_writer::add_file(), ba2_dx10_writer::ba2_dx10_writer(), ba2_dx10_writer::state, consumed, entries, options, snapshot_dir, target (+11 more)

### Community 57 - "BA2 General Payload Placement"
Cohesion: 0.09
Nodes (26): ba2_gnrl_payload_placement, offset, payload, stored_size, ba2_gnrl_placed_record, archive_path_original, directory_hash, extension (+18 more)

### Community 58 - "TES4 BSA Writer Execution"
Cohesion: 0.23
Nodes (20): add_tes4_sources(), bytes_from_text(), byte, pair, path, size_t, span, string (+12 more)

### Community 59 - "Compatibility Warning Tests"
Cohesion: 0.24
Nodes (20): bytes_from_text(), compatibility_warning_codes_from_public_header(), byte, compatibility_warning_code, path, size_t, string, string_view (+12 more)

### Community 60 - "Compatibility Warning Diagnostics"
Cohesion: 0.11
Nodes (20): compatibility_warning, archive_path, code, message, severity, compatibility_warning_code, compatibility_warning_severity, error_code (+12 more)

### Community 61 - "BA2 DX10 Chunk Preparation"
Cohesion: 0.17
Nodes (13): ba2_dx10_prepare_chunk(), ba2_dx10_prepare_entries(), ba2_dx10_writer_options, planned_texture_chunk, uint32_t, vector, ba2_dx10_writer_entry, archive_path_canonical (+5 more)

### Community 62 - "Validation API Tests"
Cohesion: 0.08
Nodes (52): append_ascii(), append_u16_le(), append_u32_le(), append_u64_le(), bytes_from_text(), compatibility_matrix_path(), archive_type, archive_variant (+44 more)

### Community 63 - "Command Line and Error Handling"
Cohesion: 0.20
Nodes (19): command_line_arguments(), compatibility_warning_code, compatibility_warning_severity, error_code, payload_sink, string, unique_ptr, error_code_name() (+11 more)

### Community 64 - "Archive CLI Argument Parsing"
Cohesion: 0.29
Nodes (19): ArgumentParser, archive_type_name(), archive_variant_name(), compression_name(), archive_type, archive_variant, entry_compression, error (+11 more)

### Community 65 - "TES4 BSA Writer Targets"
Cohesion: 0.16
Nodes (14): tes4_bsa_target, tes4_bsa_writer::target(), compressed_entry_method(), entry_compression, uint32_t, expected_target_default_compression(), expected_version(), target_expectation (+6 more)

### Community 66 - "BA2 DX10 Parsing and Compression"
Cohesion: 0.20
Nodes (18): add_fits(), compression_for(), entry_compression, size_t, span, string, uint32_t, uint64_t (+10 more)

### Community 67 - "Host File Tests and Handles"
Cohesion: 0.18
Nodes (17): bytes_from_text(), byte, DWORD, HANDLE, path, span, string_view, vector (+9 more)

### Community 68 - "BA2 DX10 Preparer Tests"
Cohesion: 0.20
Nodes (19): byte, json, path, planned_texture_chunk, size_t, span, string, uint8_t (+11 more)

### Community 69 - "DDS Texture Metadata"
Cohesion: 0.14
Nodes (14): source_dds_spec, archive_path, array_size, depth, file, format_id, format_name, height (+6 more)

### Community 70 - "Payload Size Validation"
Cohesion: 0.29
Nodes (17): checked_materialized_payload_size(), checked_payload_size(), byte, ifstream, size_t, span, string_view, uint64_t (+9 more)

### Community 71 - "BA2 Stable Archive Source"
Cohesion: 0.17
Nodes (12): ba2_stable_archive_source, session_, byte, size_t, stable_host_file_session, string_view, uint64_t, vector (+4 more)

### Community 72 - "Archive Compression Policies"
Cohesion: 0.26
Nodes (19): compression_choice, archive_policy_from(), collect_input_files(), archive_compression_policy, path, uint32_t, vector, generic_utf8_path() (+11 more)

### Community 73 - "TES4 BSA Profile Management"
Cohesion: 0.10
Nodes (35): profile_facts, archive_variant, compression_method, entry_compression, size_t, string_view, tes4_bsa_writer_options, tes4_folder_record_shape (+27 more)

### Community 74 - "BA2 General Prepared Entries"
Cohesion: 0.17
Nodes (15): ba2_gnrl_prepared_entry, archive_path_canonical, archive_path_original, directory_hash, extension, name_hash, packed_size, payload (+7 more)

### Community 75 - "Chunk Specification and Compression"
Cohesion: 0.12
Nodes (17): chunk_spec, compression, decoded_payload, end_mip, packed_size, payload_offset, raw_size, segment (+9 more)

### Community 76 - "Manifest Validation Utilities"
Cohesion: 0.24
Nodes (16): Any, find_manifest_case(), load_json(), main(), Path, Validate malformed fixture case IDs, expected errors, and referenced archive fil, Return a manifest case by ID, failing if the manifest case list is malformed or, Validate the consolidated malformed matrix and its manifest/test evidence refere (+8 more)

### Community 77 - "BA2 General Write Operations"
Cohesion: 0.17
Nodes (15): ba2_gnrl_write_archive_bytes(), checked_u16(), checked_u32(), ba2_gnrl_writer_options, byte, ostream, path, span (+7 more)

### Community 78 - "BA2 and BSA Writer States"
Cohesion: 0.16
Nodes (17): ba2_dx10_writer, ba2_dx10_target, ba2_dx10_writer, state_, ba2_gnrl_writer, ba2_gnrl_target, ba2_gnrl_writer, state_ (+9 more)

### Community 79 - "TES4 BSA Serialization Checks"
Cohesion: 0.19
Nodes (16): checked_name_size(), checked_u32(), byte, ofstream, path, size_t, span, string_view (+8 more)

### Community 80 - "BA2 DX10 Layout and Compression"
Cohesion: 0.17
Nodes (14): add_fits_u64(), ba2_dx10_plan_placements(), compression_method, uint32_t, uint64_t, vector, dedupe_key, compression (+6 more)

### Community 81 - "BSA Format Detection"
Cohesion: 0.10
Nodes (25): byte, span, detect_bsa_format(), detected_bsa_format, variant, version, archive_variant, uint32_t (+17 more)

### Community 82 - "Stored Payload and File Records"
Cohesion: 0.16
Nodes (16): stored_payload, string, uint32_t, uint64_t, vector, tes4_prepared_entry, canonical_folder, file_hash (+8 more)

### Community 83 - "BA2 GNRL Entry Validation"
Cohesion: 0.17
Nodes (12): ba2_gnrl_validate_entries(), span, ba2_gnrl_writer_entry, archive_path_canonical, archive_path_original, from_memory, host_path, memory_bytes (+4 more)

### Community 84 - "File Path and JSON Utils"
Cohesion: 0.23
Nodes (13): ostringstream, canonicalize(), pair, string, string_view, extension_fourcc(), filename_stem(), hash_folder() (+5 more)

### Community 85 - "Compression and Directory Metadata"
Cohesion: 0.13
Nodes (24): compression_for(), array, byte, entry_compression, span, string, uint32_t, uint64_t (+16 more)

### Community 86 - "LZ4 Frame Compression Codec"
Cohesion: 0.20
Nodes (14): LZ4F_dctx, compress_lz4_frame(), byte, error, ifstream, size_t, span, string_view (+6 more)

### Community 87 - "Bethesda Hash Functions"
Cohesion: 0.32
Nodes (15): string_view, uint32_t, uint64_t, uint8_t, crc32_entry(), crc32_lookup(), extension_magic(), hash_fo4() (+7 more)

### Community 88 - "Archive Metadata and Flags"
Cohesion: 0.17
Nodes (12): archive_metadata, archive_flags, ba2, default_compression, file_count, type, variant, version (+4 more)

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

### Community 94 - "Deflate Compression Codec"
Cohesion: 0.19
Nodes (12): libdeflate_compressor, libdeflate_decompressor, codec_error(), compress_deflate(), compressor_deleter, byte, error, size_t (+4 more)

### Community 95 - "Payload Compression Router"
Cohesion: 0.30
Nodes (14): compress_payload(), copy_bytes(), byte, compression_method, error, ifstream, size_t, span (+6 more)

### Community 96 - "File Parsing and Normalization"
Cohesion: 0.21
Nodes (14): add_fits_u64(), archive_string_from_bytes(), byte, ifstream, span, string, string_view, uint64_t (+6 more)

### Community 97 - "BA2 GNRL Deduplication"
Cohesion: 0.24
Nodes (10): add_fits_u64(), ba2_gnrl_dedupe_identity, fingerprint, stored_size, ba2_gnrl_plan_placements(), checked_u32(), string_view, uint32_t (+2 more)

### Community 98 - "Bulk Extraction Sink Management"
Cohesion: 0.14
Nodes (13): collecting_sink, bytes_, bulk_extract_sink_factory, byte, path, payload_sink, size_t, span (+5 more)

### Community 99 - "TES3 BSA Writer Options"
Cohesion: 0.10
Nodes (30): tes3_bsa_writer_options, overwrite_existing, checked_u32(), span, string, string_view, uint32_t, uint64_t (+22 more)

### Community 100 - "Data Types and Formats"
Cohesion: 0.14
Nodes (14): chunk(), byte, size_t, uint16_t, uint32_t, uint64_t, vector, format_case (+6 more)

### Community 101 - "Windows Handle Management"
Cohesion: 0.21
Nodes (11): clear_delete_on_close(), commit_staged(), byte, DWORD, HANDLE, size_t, span, mark_delete_on_close() (+3 more)

### Community 102 - "Memory Allocation and Hashing"
Cohesion: 0.22
Nodes (12): Allocator, Hash, Key, KeyEqual, error, size_t, string_view, T (+4 more)

### Community 103 - "Archive Writer Policy Overview"
Cohesion: 0.24
Nodes (11): TES4 BSA Profile, Archive Family Writer Examples, BA2 DX10 Snapshot Lifecycle, BA2 DX10 Target Policies, BA2 GNRL Target Policies, Deflate Compression Route, LZ4 Frame Compression Route, Raw LZ4 Block Compression Route (+3 more)

### Community 104 - "DDS Metadata Analysis"
Cohesion: 0.26
Nodes (12): DXGI_FORMAT, analyze_dds_metadata(), analyze_dds_source(), checked_u32(), byte, size_t, span, uint32_t (+4 more)

### Community 105 - "BA2 DX10 Writer Entry Validation"
Cohesion: 0.11
Nodes (24): ba2_dx10_make_writer_entry(), ba2_dx10_validate_entries(), ba2_dx10_target, path, size_t, span, string_view, ba2_dx10_build_writer_entry_snapshot() (+16 more)

### Community 106 - "Archive Payload Handling"
Cohesion: 0.14
Nodes (14): byte, string, uint32_t, uint64_t, vector, tes3_prepared_entry, archive_path_original, from_memory (+6 more)

### Community 107 - "TES4 BSA Entry Validation"
Cohesion: 0.11
Nodes (21): tes4_validate_entries(), byte, entry_compression_policy, span, string_view, uint32_t, byte, entry_compression_policy (+13 more)

### Community 108 - "Archive Header Writing"
Cohesion: 0.37
Nodes (7): build_archive(), byte_buffer, bytes, uint64_t, record_table_size(), write_header(), write_records()

### Community 109 - "Writer Execution Testing"
Cohesion: 0.29
Nodes (12): bytes_from_text(), byte, path, span, string_view, vector, generated_dx10_source_path(), output_path() (+4 more)

### Community 110 - "Writer Hotspot Policy Tests"
Cohesion: 0.27
Nodes (11): path, span, string, string_view, vector, declaration_block(), public_declaration_lines(), read_text_file() (+3 more)

### Community 111 - "BA2 DX10 Placed Records"
Cohesion: 0.12
Nodes (16): ba2_dx10_placed_record, archive_path_original, chunk_count, chunks, cube_maps_raw, directory_hash, dxgi_format, extension (+8 more)

### Community 112 - "Result and Error Handling"
Cohesion: 0.16
Nodes (10): error, code, message, error_code, string, result<void>, error_, finalization_workspace (+2 more)

### Community 113 - "entry_metadata"
Cohesion: 0.13
Nodes (17): entry_metadata, archive_hash, compression, embedded_name_prefix_size, has_embedded_name, original_path, path, payload_offset (+9 more)

### Community 114 - "TES4 BSA Writer Tests"
Cohesion: 0.38
Nodes (10): path, string, generated_source_dir(), non_ascii_output_path(), non_ascii_source_dir(), output_path(), target_name(), utf8_string_from_path() (+2 more)

### Community 115 - "TES4 BSA Writer Core"
Cohesion: 0.28
Nodes (8): tes4_bsa_writer, tes4_bsa_writer_options, vector, tes4_bsa_writer::state, entries, options, target, tes4_bsa_writer::tes4_bsa_writer()

### Community 116 - "TES4 BSA Folder Metadata"
Cohesion: 0.20
Nodes (12): size_t, string, vector, tes4_bsa_folder_block, files, name, tes4_bsa_raw_table, file_names (+4 more)

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

### Community 124 - "TES4 BSA Parser Tests"
Cohesion: 0.22
Nodes (10): byte, path, size_t, span, string_view, uint32_t, vector, generated_archive_path() (+2 more)

### Community 125 - "Writer Ownership Tests"
Cohesion: 0.29
Nodes (10): bytes_from_text(), byte, path, span, string_view, vector, generated_source_dir(), require_extracted_bytes() (+2 more)

### Community 126 - "Build and Toolchain Scripts"
Cohesion: 0.27
Nodes (5): Assert-SafeCleanPath(), Get-SafePathForDisplay(), Invoke-CMakeBuildTarget(), Invoke-ExternalCommand(), Remove-BuildDirectory()

### Community 127 - "Archive Path Normalization"
Cohesion: 0.29
Nodes (9): archive_path_key, value, error, string_view, string, invalid_path_error(), is_drive_rooted(), lower_ascii() (+1 more)

### Community 128 - "LZ4 Block Compression"
Cohesion: 0.40
Nodes (9): block_error(), checked_int_size(), compress_lz4_block(), byte, error, size_t, span, vector (+1 more)

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

### Community 134 - "File Session Diagnostics"
Cohesion: 0.20
Nodes (11): session_diagnostics, stable_host_file_session, session_diagnostics, uint64_t, stable_host_file_session, close, diagnostics_, native_handle_ (+3 more)

### Community 135 - "BA2 DX10 Writer Options"
Cohesion: 0.29
Nodes (7): ba2_dx10_writer_options, deduplicate_payloads, max_decoded_chunk_bytes, overwrite_existing, starfield_compression_method, starfield_unknown1, starfield_unknown2

### Community 136 - "BA2 DX10 Subresource Snapshots"
Cohesion: 0.22
Nodes (9): ba2_dx10_subresource_snapshot, array_index, face_index, mip, size, snapshot_path, path, uint32_t (+1 more)

### Community 137 - "BA2 DX10 Error Handling Tests"
Cohesion: 0.36
Nodes (8): error_code, json, path, string_view, error_code_from_manifest(), generated_archive_dir(), generated_archive_path(), read_json_file()

### Community 138 - "Benchmark Policy Tests"
Cohesion: 0.33
Nodes (8): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), require_no_tokens(), source_root()

### Community 139 - "Native Handle Management"
Cohesion: 0.47
Nodes (3): HANDLE, native_handle_guard, handle_

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

### Community 145 - "BA2 Archive Session Management"
Cohesion: 0.20
Nodes (8): ba2_archive_session_context(), ba2_subtype, vector, open_ba2_archive(), opened_ba2_archive, entries, metadata, subtype

### Community 146 - "Path and Command Line Utils"
Cohesion: 0.27
Nodes (9): final_path_is_within_root(), is_separator(), local_command_line_argv, value, same_windows_prefix(), trim_final_path(), wchar_t, wstring (+1 more)

### Community 147 - "Thread Safety Policy Tests"
Cohesion: 0.32
Nodes (7): initializer_list, path, string, string_view, read_text_file(), require_all_tokens(), source_root()

### Community 148 - "BA2 DX10 Chunk Compression"
Cohesion: 0.18
Nodes (11): ba2_dx10_prepared_chunk, compression, end_mip, packed_size, payload, raw_size, start_mip, compression_method (+3 more)

### Community 149 - "BA2 Archive Catalog and Glossary"
Cohesion: 0.25
Nodes (8): Archive Entry Catalog, BA2 Archive Header, BA2 Archive Opening, BA2 Profile, BA2 Record Identity, Stored Payload, Glossary Vocabulary Policy, Single-Context Domain Layout

### Community 150 - "Host File Path and IO"
Cohesion: 0.07
Nodes (13): vector, span, payload_sink, string_view, host_file_path, resolved, path, resolve_host_file_path() (+5 more)

### Community 151 - "Payload Sink Interfaces"
Cohesion: 0.33
Nodes (5): byte, payload_sink, size_t, span, discard_payload_sink

### Community 152 - "Archive Runtime Cases"
Cohesion: 0.12
Nodes (16): archive_runtime_case, archive_host_path, archive_virtual_path, expect_texture_metadata, expected_ba2_compression_method, expected_default_compression, expected_payload, expected_type (+8 more)

### Community 153 - "Archive Entry Preparation"
Cohesion: 0.19
Nodes (13): archive_default_compressed(), ba2_gnrl_prepare_entries(), checked_u32(), archive_compression_policy, ba2_gnrl_writer_options, entry_compression_policy, finalization_workspace, size_t (+5 more)

### Community 154 - "DDS Mip and Block Sizes"
Cohesion: 0.43
Nodes (7): bytes_per_block(), checked_u32(), uint32_t, dds_mip_size(), is_block_compressed(), mip_dimension(), name_table_size()

### Community 155 - "BA2 Payload Extraction"
Cohesion: 0.35
Nodes (10): payload_sink(), ba2_dx10_extraction_host_context(), ifstream, extract_ba2_dx10_payload(), extract_compressed_chunk(), stream_raw_chunk(), ba2_gnrl_extraction_host_context(), extract_ba2_gnrl_payload() (+2 more)

### Community 156 - "Archive Variant and Compression"
Cohesion: 0.17
Nodes (12): archive_variant, ba2_subtype, entry_compression, string_view, supported_profile_case, compression_method, default_compression, name (+4 more)

### Community 157 - "Source Root Detection"
Cohesion: 0.80
Nodes (4): find_source_root_from(), path, is_source_root(), source_root()

### Community 158 - "Benchmarking and Performance Policy"
Cohesion: 0.50
Nodes (4): Report-Only Performance Policy, Synthetic Benchmark Harness, libbsa Benchmark Report Target, libbsa Benchmarks Target

### Community 159 - "Compatibility Warning Evidence"
Cohesion: 0.50
Nodes (4): BSA Embedded Name Compatibility Risk Evidence, Compressed Sound Payload Warning Evidence, Target Family Mismatch Warning Evidence, Compatibility Warning Policy

### Community 160 - "Bulk Extraction Results"
Cohesion: 0.15
Nodes (9): bulk_extract_entry_result, entry, failure, path, error, optional, ba2_gnrl_make_writer_entry(), string_view (+1 more)

### Community 161 - "BA2 DX10 Compression Query"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: How do BA2 DX10 prepared chunks compression finalization deduplication serialization work and where are Stored Payload integration points?, Source Nodes

### Community 162 - "BA2 DX10 Payload Placement"
Cohesion: 0.22
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

### Community 174 - "Deflate Codec Tests"
Cohesion: 0.33
Nodes (5): byte, size_t, vector, deflate_vector(), impossible_byte_vector_size()

### Community 175 - "DX10 Reference Compatibility Audit"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: Issue 33 DX10 reference compatibility audit, Source Nodes

### Community 176 - "TES3 BSA Layout Checks"
Cohesion: 0.38
Nodes (9): add_fits_u64(), checked_add_u32(), checked_mul_u32(), checked_u32(), span, string_view, uint32_t, uint64_t (+1 more)

### Community 177 - "Byte Vector Management"
Cohesion: 0.36
Nodes (9): append_byte_vector(), byte_vector_allocation_error(), byte, error, size_t, span, vector, make_byte_vector() (+1 more)

### Community 178 - "ba2_profile"
Cohesion: 0.10
Nodes (17): ba2_dx10_writer_options, ba2_gnrl_writer_options, ba2_profile, compressed_method_, is_dx10, is_gnrl, archive_variant, ba2_subtype (+9 more)

### Community 179 - "Parser Primitive Tests"
Cohesion: 0.31
Nodes (7): path, size_t, impossible_set_capacity(), impossible_string_size(), impossible_vector_capacity(), temp_file_cleanup, path_

### Community 180 - "Archive Validation Options"
Cohesion: 0.25
Nodes (8): archive_type, archive_variant, uint64_t, validation_options, expected_type, expected_variant, max_extractability_entry_bytes, validate_entry_extractability

### Community 181 - "BA2 DX10 Header Options"
Cohesion: 0.33
Nodes (5): ba2_dx10_stored_header_options, starfield_compression_method, starfield_unknown1, starfield_unknown2, uint32_t

### Community 182 - "Payload Sink Management"
Cohesion: 0.29
Nodes (7): collecting_sink, bytes_, byte, payload_sink, size_t, span, vector

### Community 183 - "Payload Source Management"
Cohesion: 0.50
Nodes (3): payload_source, read, remaining

### Community 184 - "BA2 DX10 Name Handling"
Cohesion: 0.33
Nodes (5): string, uint32_t, uint64_t, vector, read_ba2_dx10_names()

### Community 185 - "Byte Array Utilities"
Cohesion: 0.67
Nodes (3): array, byte, fourcc()

## Knowledge Gaps
- **820 isolated node(s):** `scenario`, `worker_count`, `elapsed_ms`, `bytes_processed`, `correctness_passed` (+815 more)
  These have ≤1 connection - possible missing edges or undocumented components.

## Work-memory lessons

**Known dead ends** — questions that led nowhere; don't re-derive.
- "Trace TES5Edit BSArchPro BA2 DX10 compression, sizing, deduplication, and serialization behavior for issue 33" -> `ba2_dx10_serialize.cpp`, `packed_size`, `writer.hpp`

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `result` connect `Atomic File Operations` to `BA2 DX10 Writer Metadata`, `Bulk Archive Extraction`, `Archive Interface Headers`, `DDS Texture Chunk Handling`, `Benchmarking and Performance`, `BA2 Record Identity Management`, `Byte Buffer and Vector Sinks`, `Archive Reader Dispatch Tests`, `Capture and Sink Testing`, `Logical Reader and Storage`, `BA2 General Reader Tests`, `TES3 BSA Reader Tests`, `TES3 BSA Writer Tests`, `TES4 BSA Reader Tests`, `BA2 Archive Metadata`, `TES4 BSA Archive Parsing`, `Buffer and File Size Checks`, `TES3 BSA Parsing Utilities`, `Archive Extraction Operations`, `TES4 BSA Entry Preparation`, `Archive Type and Variant Parsing`, `Binary Reader Utilities`, `BA2 Archive Opening Tests`, `Text and Payload Sink Handling`, `BA2 DX10 Chunk Records`, `BA2 Compression and Profiles`, `TES4 BSA Layout and Options`, `Payload Sink and Source Details`, `Writer Publish Tests`, `Compression Policy Management`, `File Payload Sink Management`, `BA2 DX10 Extraction Tests`, `BA2 GNRL Archive Writing`, `CLI Argument Parsing`, `TES4 BSA Payload Descriptor`, `Finalization Workspace`, `BA2 DX10 Write Archive`, `BA2 DX10 Chunk Snapshots`, `Validation and Warning Handling`, `BA2 DX10 Writer State`, `BA2 General Payload Placement`, `BA2 DX10 Chunk Preparation`, `Command Line and Error Handling`, `BA2 DX10 Parsing and Compression`, `Payload Size Validation`, `BA2 Stable Archive Source`, `Archive Compression Policies`, `TES4 BSA Profile Management`, `BA2 General Write Operations`, `TES4 BSA Serialization Checks`, `BA2 DX10 Layout and Compression`, `BSA Format Detection`, `BA2 GNRL Entry Validation`, `Compression and Directory Metadata`, `LZ4 Frame Compression Codec`, `Archive Metadata and Flags`, `TES3 BSA Serialization Checks`, `Payload Sink and Recording`, `Byte Vector and Recording Sink`, `Deflate Compression Codec`, `Payload Compression Router`, `File Parsing and Normalization`, `BA2 GNRL Deduplication`, `Bulk Extraction Sink Management`, `TES3 BSA Writer Options`, `Windows Handle Management`, `Memory Allocation and Hashing`, `DDS Metadata Analysis`, `BA2 DX10 Writer Entry Validation`, `TES4 BSA Entry Validation`, `Result and Error Handling`, `entry_metadata`, `TES4 BSA Compression Extraction`, `Archive Path Normalization`, `LZ4 Block Compression`, `BA2 General Writer Options`, `File Session Diagnostics`, `BA2 Archive Session Management`, `Host File Path and IO`, `Payload Sink Interfaces`, `Archive Entry Preparation`, `BA2 Payload Extraction`, `Bulk Extraction Results`, `BA2 DX10 Payload Placement`, `TES3 BSA Layout Checks`, `Byte Vector Management`, `ba2_profile`, `Payload Sink Management`, `BA2 DX10 Name Handling`?**
  _High betweenness centrality (0.323) - this node is a cross-community bridge._
- **Why does `host_file_path` connect `Host File Path and IO` to `Archive Interface Headers`, `Host File Path Tests`, `File Session Diagnostics`, `Logical Reader and Storage`, `BA2 General Reader Tests`, `TES3 BSA Reader Tests`, `BA2 Archive Session Management`, `TES4 BSA Archive Parsing`, `Buffer and File Size Checks`, `TES3 BSA Parsing Utilities`, `Writer Stage and Payloads`, `BA2 Payload Extraction`, `Archive Extraction Operations`, `TES4 BSA Entry Preparation`, `Bulk Extraction Results`, `BA2 Archive Opening Tests`, `BA2 DX10 Extraction Tests`, `Host File Tests and Handles`, `TES3 BSA Serialization Checks`, `TES3 BSA Writer Options`, `Archive Payload Handling`, `entry_metadata`, `TES4 BSA Writer Core`, `TES4 BSA Compression Extraction`?**
  _High betweenness centrality (0.026) - this node is a cross-community bridge._
- **Why does `ba2_profile` connect `ba2_profile` to `BA2 GNRL Deduplication`, `BA2 DX10 Parsing and Compression`, `Archive Interface Headers`, `Archive Entry Preparation`, `BA2 DX10 Preparer Tests`, `BA2 Compression and Profiles`, `Writer Publish Tests`, `BA2 General Write Operations`, `BA2 DX10 Layout and Compression`, `BA2 DX10 Write Archive`, `BA2 DX10 Chunk Snapshots`, `BA2 Archive Metadata`, `BA2 DX10 Header Options`, `Host File Path and IO`, `Compression and Directory Metadata`, `Writer Stage and Payloads`, `BA2 Payload Extraction`, `BA2 DX10 Chunk Preparation`?**
  _High betweenness centrality (0.025) - this node is a cross-community bridge._
- **What connects `scenario`, `worker_count`, `elapsed_ms` to the rest of the system?**
  _820 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `BA2 DX10 Writer Metadata` be split into smaller, more focused modules?**
  _Cohesion score 0.06771929824561404 - nodes in this community are weakly interconnected._
- **Should `Bulk Archive Extraction` be split into smaller, more focused modules?**
  _Cohesion score 0.056134723336006415 - nodes in this community are weakly interconnected._
- **Should `Archive Interface Headers` be split into smaller, more focused modules?**
  _Cohesion score 0.06557377049180328 - nodes in this community are weakly interconnected._