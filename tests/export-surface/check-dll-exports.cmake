if(NOT DEFINED LIBBSA_DLL)
  message(FATAL_ERROR "Missing required variable: LIBBSA_DLL")
endif()

if(NOT EXISTS "${LIBBSA_DLL}")
  message(FATAL_ERROR "libbsa DLL does not exist: ${LIBBSA_DLL}")
endif()

if(DEFINED LIBBSA_DUMPBIN_HINT)
  get_filename_component(dumpbin_hint_dir "${LIBBSA_DUMPBIN_HINT}" DIRECTORY)
  find_program(DUMPBIN_EXECUTABLE NAMES dumpbin PATHS "${dumpbin_hint_dir}" NO_DEFAULT_PATH)
endif()

if(NOT DUMPBIN_EXECUTABLE)
  find_program(DUMPBIN_EXECUTABLE NAMES dumpbin)
endif()

if(NOT DUMPBIN_EXECUTABLE)
  message(FATAL_ERROR "dumpbin.exe is required to inspect the Windows DLL export surface")
endif()

execute_process(
  COMMAND "${DUMPBIN_EXECUTABLE}" /exports "${LIBBSA_DLL}"
  RESULT_VARIABLE dumpbin_result
  OUTPUT_VARIABLE export_table
  ERROR_VARIABLE dumpbin_error
)
if(NOT dumpbin_result EQUAL 0)
  message(FATAL_ERROR "dumpbin /exports failed: ${dumpbin_error}")
endif()

foreach(required_symbol IN ITEMS
    "archive_reader"
    "tes3_bsa_writer"
    "tes4_bsa_writer"
    "ba2_gnrl_writer"
    "ba2_dx10_writer"
    "validation_report"
    "validate_archive")
  if(NOT export_table MATCHES "${required_symbol}")
    message(FATAL_ERROR "Expected public symbol is missing from libbsa exports: ${required_symbol}")
  endif()
endforeach()

foreach(private_symbol IN ITEMS
    "libbsa::detail"
    "libbsa::formats"
    "libbsa::texture"
    "@detail@libbsa"
    "@formats@libbsa"
    "@texture@libbsa"
    "write_tes3_bsa_archive"
    "write_tes4_bsa_archive"
    "write_ba2_gnrl_archive"
    "write_ba2_dx10_archive"
    "deflate_"
    "lz4_"
    "directxtex"
    "writer_publish")
  if(export_table MATCHES "${private_symbol}")
    message(FATAL_ERROR "Private implementation symbol leaked from libbsa exports: ${private_symbol}")
  endif()
endforeach()
