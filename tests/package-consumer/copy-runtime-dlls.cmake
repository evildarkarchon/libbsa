if(NOT DEFINED TARGET_DIR OR TARGET_DIR STREQUAL "")
  message(FATAL_ERROR "TARGET_DIR is required")
endif()

# Static package-consumer builds can have no target runtime DLLs while still needing explicit
# ASan runtime copies, so only no-op when both caller-provided lists are empty.
if((NOT DEFINED RUNTIME_DLLS OR RUNTIME_DLLS STREQUAL "")
   AND (NOT DEFINED EXTRA_RUNTIME_DLLS OR EXTRA_RUNTIME_DLLS STREQUAL ""))
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
