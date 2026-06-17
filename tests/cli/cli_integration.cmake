if(NOT DEFINED LIBBSA_CLI OR NOT EXISTS "${LIBBSA_CLI}")
  message(FATAL_ERROR "LIBBSA_CLI must name the built bsa executable")
endif()

if(NOT DEFINED LIBBSA_SOURCE_DIR OR NOT EXISTS "${LIBBSA_SOURCE_DIR}")
  message(FATAL_ERROR "LIBBSA_SOURCE_DIR must name the repository root")
endif()

if(NOT DEFINED LIBBSA_BINARY_DIR)
  message(FATAL_ERROR "LIBBSA_BINARY_DIR must name the build root")
endif()

if(NOT DEFINED CONFIG)
  set(CONFIG "single")
endif()

set(work_root "${LIBBSA_BINARY_DIR}/cli-integration/${CONFIG}")
file(REMOVE_RECURSE "${work_root}")
file(MAKE_DIRECTORY "${work_root}")

function(run_cli expected_code stdout_var stderr_var)
  execute_process(
    COMMAND "${LIBBSA_CLI}" ${ARGN}
    RESULT_VARIABLE actual_code
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
  )
  if(NOT actual_code EQUAL expected_code)
    string(REPLACE ";" " " rendered_args "${ARGN}")
    message(FATAL_ERROR
      "CLI command failed expectation\n"
      "  command: ${LIBBSA_CLI} ${rendered_args}\n"
      "  expected: ${expected_code}\n"
      "  actual: ${actual_code}\n"
      "  stdout:\n${stdout}\n"
      "  stderr:\n${stderr}")
  endif()
  set(${stdout_var} "${stdout}" PARENT_SCOPE)
  set(${stderr_var} "${stderr}" PARENT_SCOPE)
endfunction()

function(require_contains text needle context)
  string(FIND "${text}" "${needle}" found_at)
  if(found_at EQUAL -1)
    message(FATAL_ERROR "Expected ${context} to contain '${needle}', but got:\n${text}")
  endif()
endfunction()

function(write_text path text)
  cmake_path(GET path PARENT_PATH parent)
  file(MAKE_DIRECTORY "${parent}")
  file(WRITE "${path}" "${text}")
endfunction()

function(require_file_text path expected)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "Expected file to exist: ${path}")
  endif()
  file(READ "${path}" actual)
  if(NOT actual STREQUAL expected)
    message(FATAL_ERROR "Unexpected file contents for ${path}\nexpected: ${expected}\nactual: ${actual}")
  endif()
endfunction()

function(require_not_exists path context)
  if(EXISTS "${path}")
    message(FATAL_ERROR "Expected ${context} not to exist: ${path}")
  endif()
endfunction()

function(try_create_junction link target result_var)
  if(NOT WIN32)
    set(${result_var} "UNSUPPORTED" PARENT_SCOPE)
    return()
  endif()

  execute_process(
    COMMAND "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0/powershell.exe"
      -NoProfile
      -NonInteractive
      -ExecutionPolicy Bypass
      -Command "New-Item -ItemType Junction -Path '${link}' -Target '${target}' | Out-Null"
    RESULT_VARIABLE link_code
    OUTPUT_VARIABLE link_stdout
    ERROR_VARIABLE link_stderr
  )
  if(link_code EQUAL 0)
    set(${result_var} "CREATED" PARENT_SCOPE)
  else()
    message(STATUS "Skipping reparse-point test because junction creation failed (${link_code}): ${link_stdout}${link_stderr}")
    set(${result_var} "FAILED" PARENT_SCOPE)
  endif()
endfunction()

function(pack_unpack_roundtrip format extension)
  set(case_root "${work_root}/roundtrip-${format}")
  set(input_root "${case_root}/input")
  set(archive "${case_root}/packed.${extension}")
  set(output_root "${case_root}/output")

  write_text("${input_root}/meshes/model.nif" "mesh payload for ${format}\n")
  write_text("${input_root}/textures/stone.dds" "texture payload for ${format}\n")

  run_cli(0 stdout stderr pack --format "${format}" "${input_root}" "${archive}")
  require_contains("${stdout}" "packed 2 file(s)" "pack output")

  run_cli(0 stdout stderr list --details "${archive}")
  require_contains("${stdout}" "meshes/model.nif" "list output")
  require_contains("${stdout}" "raw=" "detailed list output")
  require_contains("${stdout}" "compression=" "detailed list output")

  run_cli(0 stdout stderr info "${archive}")
  require_contains("${stdout}" "type:" "info output")
  require_contains("${stdout}" "file_count: 2" "info output")

  run_cli(0 stdout stderr validate "${archive}")
  require_contains("${stdout}" "valid: yes" "validate output")

  run_cli(0 stdout stderr unpack "${archive}" "${output_root}")
  require_file_text("${output_root}/meshes/model.nif" "mesh payload for ${format}\n")
  require_file_text("${output_root}/textures/stone.dds" "texture payload for ${format}\n")

  run_cli(1 stdout stderr pack --format "${format}" "${input_root}" "${archive}")
  require_contains("${stderr}" "io_error" "pack overwrite refusal")

  run_cli(0 stdout stderr pack --format "${format}" --overwrite "${input_root}" "${archive}")
endfunction()

run_cli(0 stdout stderr --help)
require_contains("${stdout}" "pack" "top-level help")
require_contains("${stdout}" "unpack" "top-level help")
require_contains("${stdout}" "validate" "top-level help")

run_cli(0 stdout stderr --version)
require_contains("${stdout}" "libbsa" "version output")

run_cli(2 stdout stderr unknown-subcommand)
require_contains("${stderr}" "unknown subcommand" "unknown subcommand diagnostic")

run_cli(0 stdout stderr pack --help)
foreach(token IN ITEMS
    bsa-tes3
    bsa-oblivion
    bsa-fo3
    bsa-sse
    ba2-gnrl-fo4
    ba2-gnrl-sf-v2
    ba2-gnrl-sf-v3
    ba2-dx10-fo4
    ba2-dx10-sf-v3)
  require_contains("${stdout}" "${token}" "pack help format table")
endforeach()

run_cli(2 stdout stderr pack)
require_contains("${stderr}" "pack requires" "missing pack args")

set(missing_format_root "${work_root}/missing-format/input")
write_text("${missing_format_root}/file.txt" "payload\n")
run_cli(2 stdout stderr pack "${missing_format_root}" "${work_root}/missing-format/out.bsa")
require_contains("${stderr}" "requires --format" "missing format diagnostic")

run_cli(2 stdout stderr pack --format no-such-format "${missing_format_root}" "${work_root}/missing-format/out.bsa")
require_contains("${stderr}" "valid tokens" "unknown format diagnostic")

run_cli(2 stdout stderr pack --format bsa-tes3 --compress compressed "${missing_format_root}" "${work_root}/bad-compress.bsa")
require_contains("${stderr}" "does not support compressed" "unsupported TES3 compression diagnostic")

run_cli(2 stdout stderr pack --format ba2-dx10-fo4 --compress raw "${missing_format_root}" "${work_root}/bad-dx10-compress.ba2")
require_contains("${stderr}" "does not support raw" "unsupported DX10 compression diagnostic")

run_cli(1 stdout stderr pack --format bsa-tes3 "${work_root}/does-not-exist" "${work_root}/missing-input.bsa")
require_contains("${stderr}" "io_error" "missing input diagnostic")

pack_unpack_roundtrip(bsa-tes3 bsa)
pack_unpack_roundtrip(bsa-oblivion bsa)
pack_unpack_roundtrip(ba2-gnrl-fo4 ba2)

set(selective_archive "${work_root}/roundtrip-bsa-tes3/packed.bsa")
set(selective_output "${work_root}/selective-output")
run_cli(1 stdout stderr unpack "${selective_archive}" "${selective_output}" --path meshes/model.nif --path missing/file.txt)
require_file_text("${selective_output}/meshes/model.nif" "mesh payload for bsa-tes3\n")
require_contains("${stderr}" "not_found" "missing selective path diagnostic")

run_cli(1 stdout stderr unpack "${selective_archive}" "${selective_output}")
require_contains("${stderr}" "--overwrite" "unpack overwrite refusal")

set(traversal_output "${work_root}/traversal-output")
run_cli(1 stdout stderr unpack "${selective_archive}" "${traversal_output}" --path ../escape.txt)
if(EXISTS "${work_root}/escape.txt")
  message(FATAL_ERROR "Traversal extraction wrote outside the output root")
endif()

if(WIN32)
  set(pack_reparse_root "${work_root}/pack-reparse")
  set(pack_reparse_source "${pack_reparse_root}/source")
  set(pack_reparse_input "${pack_reparse_root}/input-link")
  set(pack_reparse_archive "${pack_reparse_root}/packed.bsa")
  write_text("${pack_reparse_source}/outside.txt" "outside payload\n")
  try_create_junction("${pack_reparse_input}" "${pack_reparse_source}" pack_reparse_status)
  if(pack_reparse_status STREQUAL "CREATED")
    run_cli(1 stdout stderr pack --format bsa-tes3 "${pack_reparse_input}" "${pack_reparse_archive}")
    require_contains("${stderr}" "reparse" "pack reparse-point input diagnostic")
    require_not_exists("${pack_reparse_archive}" "archive from reparse-point input")
  endif()

  set(unpack_reparse_output "${work_root}/unpack-reparse/output")
  set(unpack_reparse_outside "${work_root}/unpack-reparse/outside")
  file(MAKE_DIRECTORY "${unpack_reparse_output}" "${unpack_reparse_outside}")
  try_create_junction("${unpack_reparse_output}/meshes" "${unpack_reparse_outside}" unpack_reparse_status)
  if(unpack_reparse_status STREQUAL "CREATED")
    run_cli(1 stdout stderr unpack "${selective_archive}" "${unpack_reparse_output}")
    require_contains("${stderr}" "reparse" "unpack reparse-point destination diagnostic")
    require_not_exists("${unpack_reparse_outside}/model.nif" "payload outside output root")
  endif()
endif()

set(warning_root "${work_root}/warning/input")
write_text("${warning_root}/sound/fx/alert.wav" "alert sound payload alert sound payload alert sound payload alert sound payload\n")
set(warning_archive "${work_root}/warning/compressed-sound.bsa")
run_cli(0 stdout stderr pack --format bsa-fo3 --compress compressed "${warning_root}" "${warning_archive}")
run_cli(0 stdout stderr validate "${warning_archive}")
require_contains("${stdout}" "warning:" "validation warning output")
require_contains("${stdout}" "compressed_sound_payload" "validation warning code")
run_cli(1 stdout stderr validate --strict "${warning_archive}")
require_contains("${stdout}" "compressed_sound_payload" "strict validation warning output")

set(generated_archive_dir "${LIBBSA_SOURCE_DIR}/tests/fixtures/generated/archives")
set(tes3_fixture "${generated_archive_dir}/tes3_success.bsa")
if(EXISTS "${tes3_fixture}")
  run_cli(0 stdout stderr list "${tes3_fixture}")
  require_contains("${stdout}" "meshes/tiny/probe.nif" "known fixture list output")
  run_cli(0 stdout stderr info "${tes3_fixture}")
  require_contains("${stdout}" "variant: tes3" "known fixture info output")
  run_cli(0 stdout stderr validate "${tes3_fixture}")
  require_contains("${stdout}" "valid: yes" "known fixture validate output")
endif()

set(invalid_archive "${work_root}/not-an-archive.bsa")
write_text("${invalid_archive}" "not an archive\n")
run_cli(1 stdout stderr validate "${invalid_archive}")
require_contains("${stdout}" "valid: no" "invalid archive validate output")
require_contains("${stdout}" "unsupported" "invalid archive diagnostic")

set(dx10_source "${LIBBSA_SOURCE_DIR}/tests/fixtures/generated/source/ba2_dx10_bc1_unorm.dds")
if(EXISTS "${dx10_source}")
  set(dx10_root "${work_root}/dx10")
  set(dx10_input "${dx10_root}/input")
  set(dx10_archive "${dx10_root}/texture.ba2")
  set(dx10_output "${dx10_root}/output")
  file(MAKE_DIRECTORY "${dx10_input}/textures/cli")
  file(COPY_FILE "${dx10_source}" "${dx10_input}/textures/cli/bc1.dds")
  run_cli(0 stdout stderr pack --format ba2-dx10-fo4 "${dx10_input}" "${dx10_archive}")
  run_cli(0 stdout stderr unpack "${dx10_archive}" "${dx10_output}")
  if(NOT EXISTS "${dx10_output}/textures/cli/bc1.dds")
    message(FATAL_ERROR "DX10 unpack did not write the expected DDS path")
  endif()
endif()
