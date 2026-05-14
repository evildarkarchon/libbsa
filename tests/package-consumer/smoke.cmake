foreach(required_var IN ITEMS LIBBSA_BUILD_DIR LIBBSA_INSTALL_PREFIX CONSUMER_SOURCE_DIR CONSUMER_BUILD_DIR CONFIG)
  if(NOT DEFINED ${required_var})
    message(FATAL_ERROR "Missing required variable: ${required_var}")
  endif()
endforeach()

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
  COMMAND ${CMAKE_COMMAND} -S "${CONSUMER_SOURCE_DIR}" -B "${CONSUMER_BUILD_DIR}" "-DCMAKE_PREFIX_PATH=${consumer_prefix_path}"
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

# Static package installs can still depend on runtime DLLs from transitive vcpkg libraries, so
# mirror the configured prefix bin directories into the consumer output before its ctest run.
foreach(prefix IN LISTS consumer_prefix_path)
  foreach(runtime_bin_dir IN ITEMS "${prefix}/bin" "${prefix}/debug/bin")
    if(IS_DIRECTORY "${runtime_bin_dir}")
      file(GLOB runtime_dlls "${runtime_bin_dir}/*.dll")
      if(runtime_dlls)
        file(COPY ${runtime_dlls} DESTINATION "${CONSUMER_BUILD_DIR}/${CONFIG}")
      endif()
    endif()
  endforeach()
endforeach()

execute_process(
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CONSUMER_BUILD_DIR}" -C "${CONFIG}" --output-on-failure
  RESULT_VARIABLE test_result
)
if(NOT test_result EQUAL 0)
  message(FATAL_ERROR "package consumer test failed: ${test_result}")
endif()
