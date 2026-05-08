find_package(Catch2 CONFIG REQUIRED)

add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/public_include_boundary_tests.cpp
  unit/local_game_fixture_tests.cpp
  unit/validation_policy_tests.cpp
  unit/binary_io_tests.cpp
  unit/archive_path_tests.cpp
  unit/payload_stream_tests.cpp
)

target_link_libraries(libbsa_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain
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
