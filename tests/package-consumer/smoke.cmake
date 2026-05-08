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

execute_process(
  COMMAND ${CMAKE_COMMAND} -S "${CONSUMER_SOURCE_DIR}" -B "${CONSUMER_BUILD_DIR}" "-DCMAKE_PREFIX_PATH=${LIBBSA_INSTALL_PREFIX}"
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

execute_process(
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CONSUMER_BUILD_DIR}" -C "${CONFIG}" --output-on-failure
  RESULT_VARIABLE test_result
)
if(NOT test_result EQUAL 0)
  message(FATAL_ERROR "package consumer test failed: ${test_result}")
endif()
