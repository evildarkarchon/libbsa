## 1. Setup and TES3 BSA Pilot

- [ ] 1.1 Add empty internal headers `tes3_bsa_prepare.hpp`, `tes3_bsa_layout.hpp`, `tes3_bsa_serialize.hpp` and matching empty `.cpp` translation units under `src/formats/bsa/`.
- [ ] 1.2 Register the three new TES3 stage `.cpp` files in `CMakeLists.txt` alongside `src/formats/bsa/tes3_bsa_writer.cpp` (line 109); confirm the build still passes with empty stage TUs.
- [ ] 1.3 Move TES3 source preparation (`prepare_entries`, `disk_payload_size`, `make_entry`, `validate_host_path`, `validate_entries`, `preserved_archive_path`) from `tes3_bsa_writer.cpp` into `tes3_bsa_prepare.{hpp,cpp}`; expose only the stage entry point and `prepared_entry` value type from the header.
- [ ] 1.4 Move TES3 layout (`assign_raw_offsets`, `add_fits_u64`, `checked_u32`, `checked_add_u32`, `checked_mul_u32`) from `tes3_bsa_writer.cpp` into `tes3_bsa_layout.{hpp,cpp}`; expose only the stage entry point.
- [ ] 1.5 Move TES3 serialization (`write_archive_bytes`, `write_string_terminated`, `write_span_to_stream`, `stream_disk_payload_to_output`) into `tes3_bsa_serialize.{hpp,cpp}`; expose only the stage entry point.
- [ ] 1.6 Reduce `tes3_bsa_writer.cpp` to public class implementation and a thin `write_tes3_bsa_archive` orchestrator that calls preparation → layout → `detail::publish_writer_output(... serialize ...)` exactly once.
- [ ] 1.7 Run the full writer test suite and any existing TES3 round-trip / fixture / compatibility tests; verify all previously passing tests still pass.
- [ ] 1.8 Add focused unit tests that call `tes3_prepare_entries` and `tes3_assign_raw_offsets` directly with minimal inputs (one entry, malformed entry, oversized entry).

## 2. BA2 GNRL Family

- [ ] 2.1 Add empty internal headers and `.cpp` files for `ba2_gnrl_prepare`, `ba2_gnrl_layout`, `ba2_gnrl_serialize` under `src/formats/ba2/`; register them in `CMakeLists.txt` alongside line 105.
- [ ] 2.2 Move BA2 GNRL source preparation (`prepare_entry`, `prepare_entries`, `read_source_bytes`, `disk_file_size`, `compression_method_for_compressed_entry`, `archive_default_compressed`, `requested_entry_compression`, `extension_fourcc_for`, `is_ascii_extension_byte`, `make_entry`, `preserved_archive_path`, `split_directory_file`, `validate_target_options`, `validate_entries`) into `ba2_gnrl_prepare.{hpp,cpp}`.
- [ ] 2.3 Move BA2 GNRL layout (`assign_payload_offsets`, `payloads_equal`, `compare_disk_payload_to_bytes`, `compare_disk_payloads`, `hash_bytes`, `hash_disk_payload`, `add_fits_u64`, `checked_u32`, `checked_u16`, `version_for`, `header_size_for`) into `ba2_gnrl_layout.{hpp,cpp}`.
- [ ] 2.4 Move BA2 GNRL serialization (`write_archive_bytes`, `write_name`, `stream_disk_payload`) into `ba2_gnrl_serialize.{hpp,cpp}`.
- [ ] 2.5 Reduce `ba2_gnrl_writer.cpp` to public class plus a thin `write_ba2_gnrl_archive` orchestrator that calls preparation → layout → `detail::publish_writer_output(... serialize ...)` exactly once.
- [ ] 2.6 Run existing BA2 GNRL round-trip / fixture / compatibility tests; verify no regressions.
- [ ] 2.7 Add focused unit tests for `ba2_gnrl_prepare_entries`, `ba2_gnrl_assign_payload_offsets` (with `deduplicate_payloads` toggled both directions), and `ba2_gnrl_payloads_equal`.

## 3. TES4 BSA Family

- [ ] 3.1 Add empty internal headers and `.cpp` files for `tes4_bsa_prepare`, `tes4_bsa_layout`, `tes4_bsa_serialize` under `src/formats/bsa/`; register them in `CMakeLists.txt` alongside line 112.
- [ ] 3.2 Move TES4 source preparation (`prepare_one_entry`, `prepare_folders`, `make_embedded_name_prefix`, `encode_stored_payload`, `read_source_bytes`, `read_disk_source_bytes`, `read_disk_source_prefix`, `disk_payload_size`, `archive_default_compressed`, `requested_entry_compression`, `compression_method_for_target`, `validate_parseable_dds_texture_for_target`, `validate_bsa_texture_format_for_target`, `is_dx9_bsa_texture_format`, `is_fallout4_compatible_bsa_texture_format`, `file_flag_for_extension`, `file_hash_for`, `extension_of`, `lower_ascii`, `split_folder_file`, `join_folder_file`, `make_entry`, `stored_tes4_archive_path`, `validate_entries`, `version_for`) into `tes4_bsa_prepare.{hpp,cpp}`.
- [ ] 3.3 Move TES4 layout (`assign_offsets`, `materialize_stored_payload`, `stored_payloads_equal`, `add_fits_u64`, `checked_u32`, `checked_size_flags_payload_size`, `checked_name_size`) into `tes4_bsa_layout.{hpp,cpp}`.
- [ ] 3.4 Move TES4 serialization (`write_archive_bytes`, `write_string_terminated`, `write_folder_name`, `write_span_to_stream`, `stream_disk_payload_to_output`, `append_u32_le`) into `tes4_bsa_serialize.{hpp,cpp}`.
- [ ] 3.5 Reduce `tes4_bsa_writer.cpp` to public class plus a thin `write_tes4_bsa_archive` orchestrator that calls preparation → layout → `detail::publish_writer_output(... serialize ...)` exactly once.
- [ ] 3.6 Run existing TES4 round-trip / fixture / compatibility tests across all supported `tes4_bsa_target` variants; verify no regressions.
- [ ] 3.7 Add focused unit tests for `tes4_prepare_folders`, `tes4_assign_offsets` (with `deduplicate_payloads` toggled both directions), and `tes4_stored_payloads_equal` covering raw-disk vs in-memory payload comparison branches.

## 4. BA2 DX10 Family

- [ ] 4.1 Add empty internal headers and `.cpp` files for `ba2_dx10_prepare`, `ba2_dx10_layout`, `ba2_dx10_serialize` under `src/formats/ba2/`; register them in `CMakeLists.txt` alongside line 102.
- [ ] 4.2 Move BA2 DX10 source preparation (`prepare_chunk`, `prepare_entry`, `prepare_entries`, `read_dds_file`, `make_unique_snapshot_directory`, `ensure_snapshot_directory`, `write_snapshot_file`, `append_snapshot_bytes`, `append_subresource_bytes`, `compression_method_for`, `validate_dx10_texture_format_for_target`, `is_starfield_only_dx10_format`, `extension_fourcc_for`, `is_ascii_extension_byte`, `split_directory_file`, `split_stem_extension`, `make_entry`, `preserved_archive_path`, `validate_target_options`, `validate_entries`, `version_for`, `header_size_for`) into `ba2_dx10_prepare.{hpp,cpp}`.
- [ ] 4.3 Move BA2 DX10 layout (`assign_payload_offsets`, `add_fits_u64`, `checked_u32`, `checked_u16`, `checked_u8`) into `ba2_dx10_layout.{hpp,cpp}`.
- [ ] 4.4 Move BA2 DX10 serialization (`write_archive_bytes`, `write_name`) into `ba2_dx10_serialize.{hpp,cpp}`.
- [ ] 4.5 Reduce `ba2_dx10_writer.cpp` to public class plus a thin `write_ba2_dx10_archive` orchestrator that calls preparation → layout → `detail::publish_writer_output(... serialize ...)` exactly once.
- [ ] 4.6 Run existing BA2 DX10 round-trip / fixture / compatibility tests across all supported `ba2_dx10_target` variants and Starfield compression methods; verify no regressions.
- [ ] 4.7 Add focused unit tests for `ba2_dx10_prepare_chunk`, `ba2_dx10_prepare_entries`, and `ba2_dx10_assign_payload_offsets` covering single-mip, multi-mip, and cubemap inputs.

## 5. Capability Validation

- [ ] 5.1 Confirm each `*_writer.cpp` (TES3, TES4, BA2 GNRL, BA2 DX10) contains only public class methods plus a `write_*_archive` orchestrator that calls `detail::publish_writer_output` exactly once and contains no compression routing, deduplication, table sizing, payload offset math, or byte serialization logic.
- [ ] 5.2 Confirm each writer family has exactly three new stage translation units (preparation, layout, serialization) that own their helpers in anonymous namespaces and expose only stage entry-point functions and prepared-data value types through internal stage headers.
- [ ] 5.3 Run `openspec validate split-format-writers --json` and confirm it reports `valid: true`.
- [ ] 5.4 Run the full libbsa test suite (unit + fixture + round-trip + compatibility) on a Windows MSVC build configured through the existing CMake presets; confirm no regressions versus the pre-change baseline.
- [ ] 5.5 Confirm that no public header under `include/libbsa/` was modified, no `vcpkg.json` / `vcpkg-configuration.json` change occurred, and the TES5Edit submodule pointer is unchanged.
- [ ] 5.6 Confirm that produced archive bytes for at least one representative fixture per family match a pre-change reference artifact (byte-for-byte).
