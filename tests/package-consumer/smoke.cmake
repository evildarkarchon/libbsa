foreach(required_var IN ITEMS LIBBSA_BUILD_DIR LIBBSA_INSTALL_PREFIX CONSUMER_SOURCE_DIR CONSUMER_BUILD_DIR CONFIG)
  if(NOT DEFINED ${required_var})
    message(FATAL_ERROR "Missing required variable: ${required_var}")
  endif()
endforeach()

if(CONFIG STREQUAL "")
  message(FATAL_ERROR "CONFIG must not be empty")
endif()

if(NOT CONFIG STREQUAL "Debug" AND NOT CONFIG STREQUAL "Release")
  message(FATAL_ERROR "Unsupported package-consumer smoke CONFIG: ${CONFIG}")
endif()

# Single-config producer trees only build one configuration, so fail closed when the smoke lane is
# asked to validate a different config than the artifacts the producer tree was configured for.
set(producer_cache_file "${LIBBSA_BUILD_DIR}/CMakeCache.txt")
if(EXISTS "${producer_cache_file}")
  file(STRINGS "${producer_cache_file}" producer_build_type_line REGEX "^CMAKE_BUILD_TYPE:STRING=")
  if(producer_build_type_line)
    string(REPLACE "CMAKE_BUILD_TYPE:STRING=" "" producer_build_type "${producer_build_type_line}")
    if(NOT producer_build_type STREQUAL "" AND NOT producer_build_type STREQUAL CONFIG)
      message(FATAL_ERROR "package consumer smoke CONFIG ${CONFIG} does not match producer build type ${producer_build_type}")
    endif()
  endif()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --install "${LIBBSA_BUILD_DIR}" --config "${CONFIG}" --prefix "${LIBBSA_INSTALL_PREFIX}"
  RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
  message(FATAL_ERROR "libbsa install failed: ${install_result}")
endif()

file(REMOVE_RECURSE "${CONSUMER_BUILD_DIR}")

set(consumer_prefix_path "${LIBBSA_INSTALL_PREFIX}")
file(GLOB vcpkg_triplet_prefixes LIST_DIRECTORIES true "${LIBBSA_BUILD_DIR}/vcpkg_installed/*")
foreach(vcpkg_triplet_prefix IN LISTS vcpkg_triplet_prefixes)
  if(IS_DIRECTORY "${vcpkg_triplet_prefix}")
    list(APPEND consumer_prefix_path "${vcpkg_triplet_prefix}")
  endif()
endforeach()

execute_process(
  # Mirror the selected lane config into single-config generators instead of relying on their
  # default build type when this smoke path validates the supported package-proof lanes.
  COMMAND ${CMAKE_COMMAND} -S "${CONSUMER_SOURCE_DIR}" -B "${CONSUMER_BUILD_DIR}" "-DCMAKE_PREFIX_PATH=${consumer_prefix_path}" "-DCMAKE_BUILD_TYPE=${CONFIG}"
  RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
  message(FATAL_ERROR "package consumer configure failed: ${configure_result}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --build "${CONSUMER_BUILD_DIR}" --config "${CONFIG}"
  RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
  message(FATAL_ERROR "package consumer build failed: ${build_result}")
endif()

# Preserve the generator's real consumer output directory instead of assuming every build uses a
# multi-config `${buildDir}/${CONFIG}` layout.
set(consumer_runtime_dir "${CONSUMER_BUILD_DIR}")
if(IS_DIRECTORY "${CONSUMER_BUILD_DIR}/${CONFIG}")
  set(consumer_runtime_dir "${CONSUMER_BUILD_DIR}/${CONFIG}")
endif()

# Static package installs can still depend on transitive vcpkg runtime DLLs, but the Release
# package-proof lanes must not pick up Debug copies from `debug/bin` by directory-copy order.
if(CONFIG STREQUAL "Debug")
  set(runtime_bin_subdir "debug/bin")
else()
  set(runtime_bin_subdir "bin")
endif()

foreach(prefix IN LISTS consumer_prefix_path)
  set(runtime_bin_dir "${prefix}/${runtime_bin_subdir}")
  if(IS_DIRECTORY "${runtime_bin_dir}")
    file(GLOB runtime_dlls "${runtime_bin_dir}/*.dll")
    if(runtime_dlls)
      file(COPY ${runtime_dlls} DESTINATION "${consumer_runtime_dir}")
    endif()
  endif()
endforeach()

execute_process(
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CONSUMER_BUILD_DIR}" -C "${CONFIG}" --output-on-failure
  RESULT_VARIABLE test_result
)
if(NOT test_result EQUAL 0)
  message(FATAL_ERROR "package consumer test failed: ${test_result}")
endif()
