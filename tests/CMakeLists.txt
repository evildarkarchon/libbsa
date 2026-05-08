find_package(Catch2 CONFIG REQUIRED)

add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/public_include_boundary_tests.cpp
  unit/local_game_fixture_tests.cpp
)

target_link_libraries(libbsa_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain
)

target_compile_features(libbsa_tests PRIVATE cxx_std_20)

include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
)
