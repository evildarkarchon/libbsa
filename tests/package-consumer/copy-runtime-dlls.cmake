if(NOT DEFINED TARGET_DIR OR TARGET_DIR STREQUAL "")
  message(FATAL_ERROR "TARGET_DIR is required")
endif()

# Static package-consumer builds can have no runtime DLLs; that case must not
# collapse into `cmake -E copy <destination>`.
if(NOT DEFINED RUNTIME_DLLS OR RUNTIME_DLLS STREQUAL "")
  return()
endif()

foreach(runtime_dll IN LISTS RUNTIME_DLLS)
  if(runtime_dll STREQUAL "")
    continue()
  endif()

  file(COPY "${runtime_dll}" DESTINATION "${TARGET_DIR}")
endforeach()

foreach(extra_runtime_dll IN LISTS EXTRA_RUNTIME_DLLS)
  if(extra_runtime_dll STREQUAL "")
    continue()
  endif()

  if(EXISTS "${extra_runtime_dll}")
    file(COPY "${extra_runtime_dll}" DESTINATION "${TARGET_DIR}")
  endif()
endforeach()
