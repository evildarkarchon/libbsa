find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
  unit/tes3_bsa_reader_tests.cpp
  unit/ba2_gnrl_reader_tests.cpp
  unit/ba2_dx10_metadata_tests.cpp
  unit/dds_layout_tests.cpp
  unit/ba2_dx10_parser_tests.cpp
  unit/ba2_dx10_extraction_tests.cpp
  unit/ba2_dx10_malformed_tests.cpp
  unit/public_include_boundary_tests.cpp
  unit/local_game_fixture_tests.cpp
  unit/validation_policy_tests.cpp
  unit/binary_io_tests.cpp
  unit/archive_path_tests.cpp
  unit/payload_stream_tests.cpp
  unit/deflate_codec_tests.cpp
  unit/lz4_codec_tests.cpp
  unit/compression_router_tests.cpp
  unit/bethesda_hash_tests.cpp
)

target_link_libraries(libbsa_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

target_compile_features(libbsa_tests PRIVATE cxx_std_20)

target_include_directories(libbsa_tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(libbsa_tests
  PRIVATE
    LIBBSA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_executable(generate_tes4_bsa_fixtures_tool
  fixtures/generated/generate_tes4_bsa_fixtures.cpp
)

target_link_libraries(generate_tes4_bsa_fixtures_tool
  PRIVATE
    libbsa::libbsa
)

target_compile_features(generate_tes4_bsa_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_tes4_bsa_fixtures_tool
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(generate_tes3_bsa_fixtures_tool
  fixtures/generated/generate_tes3_bsa_fixtures.cpp
)

target_link_libraries(generate_tes3_bsa_fixtures_tool
  PRIVATE
    libbsa::libbsa
)

target_compile_features(generate_tes3_bsa_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_tes3_bsa_fixtures_tool
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(generate_ba2_gnrl_fixtures_tool
  fixtures/generated/generate_ba2_gnrl_fixtures.cpp
)

target_link_libraries(generate_ba2_gnrl_fixtures_tool
  PRIVATE
    libbsa::libbsa
)

target_compile_features(generate_ba2_gnrl_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_ba2_gnrl_fixtures_tool
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(generate_ba2_dx10_fixtures_tool
  fixtures/generated/generate_ba2_dx10_fixtures.cpp
)

target_link_libraries(generate_ba2_dx10_fixtures_tool
  PRIVATE
    libbsa::libbsa
)

target_compile_features(generate_ba2_dx10_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_ba2_dx10_fixtures_tool
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_custom_target(generate_tes4_bsa_fixtures
  COMMAND generate_tes4_bsa_fixtures_tool --success --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_tes4_bsa_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v103.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v103_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v104.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v104_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v105.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v105_manifest.json
  COMMENT "Generating TES4-family BSA success fixtures"
  VERBATIM
)

add_custom_target(generate_tes3_bsa_fixtures
  COMMAND generate_tes3_bsa_fixtures_tool --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_tes3_bsa_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_success.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_success_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_malformed_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_truncated_header.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_truncated_records.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_invalid_name_span.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_invalid_payload_span.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_duplicate_canonical_path.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_inconsistent_counts_offsets.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_stored_hash_mismatch.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_hash_collision.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_unsorted_hash_records.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_raw_offset_absolute_regression.bsa
  COMMENT "Generating TES3 BSA success and malformed fixtures"
  VERBATIM
)

add_custom_target(generate_ba2_gnrl_fixtures
  COMMAND generate_ba2_gnrl_fixtures_tool --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_ba2_gnrl_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_fo4.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv2.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv2_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv3.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_sfv3_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_malformed_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_unsupported.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_truncated_header.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_truncated_records.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_truncated_name_table.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_invalid_payload_span.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_duplicate_canonical_path.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_corrupt_compressed_payload.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_exact_size_mismatch.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_unsupported_v3_compression_method.ba2
  COMMENT "Generating BA2 GNRL success and malformed fixtures"
  VERBATIM
)

add_custom_target(generate_ba2_dx10_fixtures
  COMMAND generate_ba2_dx10_fixtures_tool --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_ba2_dx10_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_fo4.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_sfv3.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_sfv3_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_malformed_manifest.json
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_truncated_header.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_truncated_records.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_truncated_chunks.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_invalid_payload_span.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_inconsistent_chunk_sizes.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_bad_compressed_chunk.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_decoded_size_mismatch.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_mip_gap.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_duplicate_mip_face.ba2
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
