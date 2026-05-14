file(READ "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" consumer_cmake)
if(NOT consumer_cmake MATCHES "copy-runtime-dlls\\.cmake")
  message(FATAL_ERROR "package consumer must copy runtime DLLs through the no-op-safe helper")
endif()
if(consumer_cmake MATCHES "COMMAND_EXPAND_LISTS")
  message(FATAL_ERROR "package consumer runtime DLL copy must not expand an empty DLL list into a one-argument copy")
endif()

set(empty_target_dir "${CMAKE_CURRENT_BINARY_DIR}/package-consumer-empty-runtime-copy")
file(REMOVE_RECURSE "${empty_target_dir}")
file(MAKE_DIRECTORY "${empty_target_dir}")

execute_process(
  COMMAND ${CMAKE_COMMAND}
    "-DTARGET_DIR=${empty_target_dir}"
    -P "${CMAKE_CURRENT_LIST_DIR}/copy-runtime-dlls.cmake"
  RESULT_VARIABLE empty_result
  OUTPUT_VARIABLE empty_output
  ERROR_VARIABLE empty_error
)
if(NOT empty_result EQUAL 0)
  message(FATAL_ERROR "empty runtime DLL copy should be a no-op\n${empty_output}\n${empty_error}")
endif()

set(runtime_source_dir "${CMAKE_CURRENT_BINARY_DIR}/package-consumer-runtime-copy-source")
set(runtime_target_dir "${CMAKE_CURRENT_BINARY_DIR}/package-consumer-runtime-copy-target")
file(REMOVE_RECURSE "${runtime_source_dir}" "${runtime_target_dir}")
file(MAKE_DIRECTORY "${runtime_source_dir}" "${runtime_target_dir}")
set(runtime_dll "${runtime_source_dir}/example-runtime.dll")
file(WRITE "${runtime_dll}" "runtime placeholder")

execute_process(
  COMMAND ${CMAKE_COMMAND}
    "-DRUNTIME_DLLS=${runtime_dll}"
    "-DTARGET_DIR=${runtime_target_dir}"
    -P "${CMAKE_CURRENT_LIST_DIR}/copy-runtime-dlls.cmake"
  RESULT_VARIABLE copy_result
  OUTPUT_VARIABLE copy_output
  ERROR_VARIABLE copy_error
)
if(NOT copy_result EQUAL 0)
  message(FATAL_ERROR "runtime DLL copy failed\n${copy_output}\n${copy_error}")
endif()
if(NOT EXISTS "${runtime_target_dir}/example-runtime.dll")
  message(FATAL_ERROR "runtime DLL helper did not copy the requested DLL")
endif()

set(extra_runtime_target_dir "${CMAKE_CURRENT_BINARY_DIR}/package-consumer-extra-runtime-copy-target")
file(REMOVE_RECURSE "${extra_runtime_target_dir}")
file(MAKE_DIRECTORY "${extra_runtime_target_dir}")
set(extra_runtime_dll "${runtime_source_dir}/example-asan-runtime.dll")
file(WRITE "${extra_runtime_dll}" "asan runtime placeholder")

execute_process(
  COMMAND ${CMAKE_COMMAND}
    "-DEXTRA_RUNTIME_DLLS=${extra_runtime_dll}"
    "-DTARGET_DIR=${extra_runtime_target_dir}"
    -P "${CMAKE_CURRENT_LIST_DIR}/copy-runtime-dlls.cmake"
  RESULT_VARIABLE extra_copy_result
  OUTPUT_VARIABLE extra_copy_output
  ERROR_VARIABLE extra_copy_error
)
if(NOT extra_copy_result EQUAL 0)
  message(FATAL_ERROR "extra runtime DLL copy failed\n${extra_copy_output}\n${extra_copy_error}")
endif()
if(NOT EXISTS "${extra_runtime_target_dir}/example-asan-runtime.dll")
  message(FATAL_ERROR "runtime DLL helper did not copy the requested extra DLL when target runtime DLLs were empty")
endif()
