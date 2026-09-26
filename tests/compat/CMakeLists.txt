# Exercise the real public library, including its DLL boundary in shared lanes.
add_executable(libbsa_compat_bridge bridge.cpp)
target_link_libraries(libbsa_compat_bridge PRIVATE libbsa::libbsa nlohmann_json::nlohmann_json bcrypt)
target_compile_features(libbsa_compat_bridge PRIVATE cxx_std_20)
target_compile_options(libbsa_compat_bridge PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4>)
libbsa_enable_msvc_asan(libbsa_compat_bridge)
libbsa_copy_msvc_asan_runtime_dlls(libbsa_compat_bridge)

file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/artifacts-$<CONFIG>.json"
  CONTENT "{\"bridge\":\"$<TARGET_FILE:libbsa_compat_bridge>\",\"library\":\"$<TARGET_FILE:libbsa>\"}\n")

add_test(NAME compatibility_harness_contracts
  COMMAND Python3::Interpreter -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR} -p "test_*.py" -v)
set_tests_properties(compatibility_harness_contracts PROPERTIES LABELS "unit;compat;malformed")

add_test(NAME independent_oracle_synthetic
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/runner.py synthetic
    --bridge $<TARGET_FILE:libbsa_compat_bridge>
    --library $<TARGET_FILE:libbsa>
    --build-dir ${CMAKE_BINARY_DIR}
    --report ${CMAKE_CURRENT_BINARY_DIR}/synthetic-$<CONFIG>.json
    --optional)
set_tests_properties(independent_oracle_synthetic PROPERTIES
  LABELS "compat;fixture;roundtrip;requires-oracle"
  SKIP_RETURN_CODE 77 TIMEOUT 1800)
set_property(TEST independent_oracle_synthetic PROPERTY ENVIRONMENT_MODIFICATION
  "PATH=path_list_prepend:$<TARGET_FILE_DIR:libbsa>"
  "PATH=path_list_prepend:$<TARGET_FILE_DIR:libbsa_compat_bridge>")

# Deliberately not an ordinary CTest case: missing local assets must fail this gate.
add_custom_target(libbsa_compatibility_check
  COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/runner.py thorough
    --bridge $<TARGET_FILE:libbsa_compat_bridge>
    --library $<TARGET_FILE:libbsa>
    --build-dir ${CMAKE_BINARY_DIR}
    --report ${CMAKE_CURRENT_BINARY_DIR}/thorough-$<CONFIG>.json
  DEPENDS libbsa_compat_bridge USES_TERMINAL VERBATIM)
