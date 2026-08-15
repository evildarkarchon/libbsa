find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

function(libbsa_link_internal_test_support target_name)
  if(BUILD_SHARED_LIBS)
    set(libbsa_test_library_sources)
    foreach(libbsa_source IN LISTS libbsa_library_sources)
      list(APPEND libbsa_test_library_sources "${PROJECT_SOURCE_DIR}/${libbsa_source}")
    endforeach()

    target_sources(${target_name}
      PRIVATE
        ${libbsa_test_library_sources}
    )

    target_compile_definitions(${target_name}
      PRIVATE
        LIBBSA_STATIC_DEFINE
    )

    target_compile_options(${target_name}
      PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
    )

    target_include_directories(${target_name}
      PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
    )

    target_link_libraries(${target_name}
      PRIVATE
        $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,libdeflate::libdeflate_static,libdeflate::libdeflate_shared>
        ${libbsa_lz4_target}
        Microsoft::DirectXTex
        bcrypt
    )
  else()
    target_link_libraries(${target_name}
      PRIVATE
        libbsa::libbsa
    )
  endif()

  libbsa_enable_msvc_asan(${target_name})
  libbsa_copy_msvc_asan_runtime_dlls(${target_name})
endfunction()

add_executable(libbsa_tests
  # Shared test support. CATCH_REGISTER_LISTENER needs a compiled translation
  # unit of its own: the force-included unit/test_source_root.hpp reaches every
  # source file and would register the listener once per translation unit.
  support/private_temp_root.cpp
  support/single_instance_guard.cpp
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/archive_reader_dispatch_tests.cpp
  unit/bulk_extraction_tests.cpp
  unit/writer_execution_options_tests.cpp
  unit/bsa_writer_execution_tests.cpp
  unit/ba2_writer_execution_tests.cpp
  unit/writer_ownership_tests.cpp
  unit/writer_stage_tests.cpp
  unit/benchmark_policy_tests.cpp
  unit/target_format_policy_tests.cpp
  unit/ba2_gnrl_writer_tests.cpp
  unit/writer_publish_tests.cpp
  unit/tes3_bsa_writer_tests.cpp
  unit/tes4_bsa_writer_tests.cpp
  unit/tes4_bsa_profile_tests.cpp
  unit/ba2_dx10_preparer_seam_tests.cpp
  unit/tes4_bsa_parser_seam_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
  unit/tes3_bsa_reader_tests.cpp
  unit/ba2_gnrl_reader_tests.cpp
  unit/ba2_archive_opening_tests.cpp
  unit/ba2_record_identity_tests.cpp
  unit/ba2_profile_tests.cpp
  unit/ba2_dx10_metadata_tests.cpp
  unit/dds_layout_tests.cpp
  unit/ba2_dx10_reader_tests.cpp
  unit/ba2_dx10_extraction_tests.cpp
  unit/ba2_dx10_writer_tests.cpp
  unit/ba2_dx10_malformed_tests.cpp
  unit/public_include_boundary_tests.cpp
  unit/local_game_fixture_tests.cpp
  unit/validation_policy_tests.cpp
  unit/validation_api_tests.cpp
  unit/export_surface_policy_tests.cpp
  unit/compatibility_warning_tests.cpp
  unit/compatibility_matrix_tests.cpp
  unit/binary_io_tests.cpp
  unit/parser_primitives_tests.cpp
  unit/payload_placement_tests.cpp
  unit/payload_span_exclusivity_tests.cpp
  unit/archive_path_tests.cpp
  unit/payload_stream_tests.cpp
  unit/zlib_codec_tests.cpp
  unit/lz4_codec_tests.cpp
  unit/compression_router_tests.cpp
  unit/stored_payload_tests.cpp
  unit/host_file_tests.cpp
  unit/host_file_path_tests.cpp
  unit/host_path_correctness_boundary_tests.cpp
  unit/temp_directory_redirection_tests.cpp
  unit/private_temp_root_tests.cpp
  unit/single_instance_guard_tests.cpp
  unit/bethesda_hash_tests.cpp
)

target_link_libraries(libbsa_tests
  PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

libbsa_link_internal_test_support(libbsa_tests)

target_compile_features(libbsa_tests PRIVATE cxx_std_20)

target_include_directories(libbsa_tests
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    # Lets any test include shared support headers as "support/<name>.hpp"
    # instead of reaching across directories with a relative path.
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(libbsa_tests
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/FI${CMAKE_CURRENT_SOURCE_DIR}/unit/test_source_root.hpp>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-include>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${CMAKE_CURRENT_SOURCE_DIR}/unit/test_source_root.hpp>
)

add_executable(generate_tes4_bsa_fixtures_tool
  fixtures/generated/generate_tes4_bsa_fixtures.cpp
)

libbsa_link_internal_test_support(generate_tes4_bsa_fixtures_tool)

target_compile_features(generate_tes4_bsa_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_tes4_bsa_fixtures_tool
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_executable(generate_tes3_bsa_fixtures_tool
  fixtures/generated/generate_tes3_bsa_fixtures.cpp
)

libbsa_link_internal_test_support(generate_tes3_bsa_fixtures_tool)

target_compile_features(generate_tes3_bsa_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_tes3_bsa_fixtures_tool
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_executable(generate_tes3_bsa_writer_fixtures_tool
  fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp
)

libbsa_link_internal_test_support(generate_tes3_bsa_writer_fixtures_tool)

target_compile_features(generate_tes3_bsa_writer_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_tes3_bsa_writer_fixtures_tool
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_executable(generate_ba2_gnrl_fixtures_tool
  fixtures/generated/generate_ba2_gnrl_fixtures.cpp
)

libbsa_link_internal_test_support(generate_ba2_gnrl_fixtures_tool)

target_compile_features(generate_ba2_gnrl_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_ba2_gnrl_fixtures_tool
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_executable(generate_ba2_dx10_fixtures_tool
  fixtures/generated/generate_ba2_dx10_fixtures.cpp
)

libbsa_link_internal_test_support(generate_ba2_dx10_fixtures_tool)

target_compile_features(generate_ba2_dx10_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_ba2_dx10_fixtures_tool
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

add_custom_target(generate_tes4_bsa_fixtures
  COMMAND generate_tes4_bsa_fixtures_tool --success --output ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_tes4_bsa_fixtures_tool
  BYPRODUCTS
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v103.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v103_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v104.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v104_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v105.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v105_manifest.json
  COMMENT "Generating TES4-family BSA success fixtures"
  VERBATIM
)

add_custom_target(generate_tes3_bsa_fixtures
  COMMAND generate_tes3_bsa_fixtures_tool --output ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_tes3_bsa_fixtures_tool
  BYPRODUCTS
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_success.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_success_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_windows_unsafe_names.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_windows_unsafe_names_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_malformed_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_truncated_header.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_truncated_records.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_invalid_name_span.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_invalid_payload_span.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_duplicate_canonical_path.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_inconsistent_counts_offsets.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_stored_hash_mismatch.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_hash_collision.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_unsorted_hash_records.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_raw_offset_absolute_regression.bsa
  COMMENT "Generating TES3 BSA success and malformed fixtures"
  VERBATIM
)

add_custom_target(generate_tes3_bsa_writer_fixtures
  COMMAND generate_tes3_bsa_writer_fixtures_tool --output ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_tes3_bsa_writer_fixtures_tool
  BYPRODUCTS
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_writer_canonical.bsa
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json
  COMMENT "Generating TES3 public-writer BSA fixture"
  VERBATIM
)

add_custom_target(generate_ba2_gnrl_fixtures
  COMMAND generate_ba2_gnrl_fixtures_tool --output ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_ba2_gnrl_fixtures_tool
  BYPRODUCTS
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_fo4.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv2.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv2_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv3.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_malformed_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_unsupported.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_truncated_header.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_truncated_records.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_truncated_name_table.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_invalid_payload_span.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_duplicate_canonical_path.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_corrupt_compressed_payload.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_exact_size_mismatch.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_unsupported_v3_compression_method.ba2
  COMMENT "Generating BA2 GNRL success and malformed fixtures"
  VERBATIM
)

add_custom_target(generate_ba2_dx10_fixtures
  COMMAND generate_ba2_dx10_fixtures_tool --output ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_ba2_dx10_fixtures_tool
  BYPRODUCTS
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_fo4.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_sfv3.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_sfv3_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_truncated_header.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_truncated_records.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_truncated_chunks.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_invalid_payload_span.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_inconsistent_chunk_sizes.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_bad_compressed_chunk.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_decoded_size_mismatch.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_mip_gap.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_duplicate_mip_face.ba2
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/source/ba2_dx10_multi_mip_bc7_unorm.dds
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/source/ba2_dx10_array_bc5_unorm_2slice.dds
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/source/ba2_dx10_cubemap_bc1_unorm_6face.dds
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/source/ba2_dx10_cubemap_array_bc1_unorm_12face.dds
  COMMENT "Generating BA2 DX10 texture success and malformed fixtures"
  VERBATIM
)

include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)

add_test(
  NAME package_consumer_smoke
  COMMAND ${CMAKE_COMMAND}
    -DLIBBSA_BUILD_DIR=${CMAKE_BINARY_DIR}
    -DLIBBSA_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/package-consumer-prefix
    -DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/package-consumer
    -DCONSUMER_BUILD_DIR=${CMAKE_BINARY_DIR}/package-consumer-build
    -DCONFIG=$<CONFIG>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/package-consumer/smoke.cmake
)

# The supported windows-msvc-release-static and windows-msvc-release-shared lanes own this
# Windows-only package proof inside the normal CTest graph. Keep opt-in requires-game-fixture
# corpus checks separate so Release proof stays runnable from committed repository assets.

add_test(
  NAME package_consumer_runtime_dll_copy
  COMMAND ${CMAKE_COMMAND}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/package-consumer/verify-runtime-dll-copy.cmake
)

set_tests_properties(package_consumer_smoke package_consumer_runtime_dll_copy PROPERTIES
  LABELS "package_consumer;target_format_policy"
)

if(LIBBSA_BUILD_CLI AND TARGET bsa)
  add_test(
    NAME cli_integration
    COMMAND ${CMAKE_COMMAND}
      -DLIBBSA_CLI=$<TARGET_FILE:bsa>
      -DLIBBSA_SOURCE_DIR=${PROJECT_SOURCE_DIR}
      -DLIBBSA_BINARY_DIR=${CMAKE_BINARY_DIR}
      -DCONFIG=$<CONFIG>
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cli/cli_integration.cmake
  )

  set_tests_properties(cli_integration PROPERTIES
    LABELS "cli;fixture;roundtrip;malformed"
  )

  set_property(TEST cli_integration PROPERTY ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:$<TARGET_FILE_DIR:libbsa>"
    "PATH=path_list_prepend:$<TARGET_FILE_DIR:bsa>"
  )
endif()

if(WIN32 AND BUILD_SHARED_LIBS)
  add_test(
    NAME shared_export_surface
    COMMAND ${CMAKE_COMMAND}
      -DLIBBSA_DLL=$<TARGET_FILE:libbsa>
      -DLIBBSA_DUMPBIN_HINT=${CMAKE_LINKER}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/export-surface/check-dll-exports.cmake
  )

  set_tests_properties(shared_export_surface PROPERTIES
    LABELS "export_surface;public-api"
  )
endif()

add_test(
  NAME validate_fixture_manifests
  COMMAND Python3::Interpreter
    ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/validate_fixture_manifests.py
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set_tests_properties(validate_fixture_manifests PROPERTIES
  LABELS "unit;fixture;malformed;compatibility_matrix"
)
