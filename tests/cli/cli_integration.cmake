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

function(current_windows_user_sid sid_var result_var)
  execute_process(
    COMMAND "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0/powershell.exe"
      -NoProfile
      -NonInteractive
      -ExecutionPolicy Bypass
      -Command "[System.Security.Principal.WindowsIdentity]::GetCurrent().User.Value"
    RESULT_VARIABLE sid_code
    OUTPUT_VARIABLE sid_stdout
    ERROR_VARIABLE sid_stderr
  )
  if(sid_code EQUAL 0)
    string(STRIP "${sid_stdout}" sid)
    set(${sid_var} "${sid}" PARENT_SCOPE)
    set(${result_var} "CREATED" PARENT_SCOPE)
  else()
    message(STATUS "Skipping ACL-based test because current-user SID lookup failed (${sid_code}): ${sid_stdout}${sid_stderr}")
    set(${sid_var} "" PARENT_SCOPE)
    set(${result_var} "FAILED" PARENT_SCOPE)
  endif()
endfunction()

function(reset_directory_acl path)
  if(NOT WIN32 OR NOT EXISTS "${path}")
    return()
  endif()

  execute_process(
    COMMAND "$ENV{SystemRoot}/System32/icacls.exe" "${path}" /reset /T /C /Q
    RESULT_VARIABLE reset_code
    OUTPUT_VARIABLE reset_stdout
    ERROR_VARIABLE reset_stderr
  )
  if(NOT reset_code EQUAL 0)
    message(WARNING "Failed to reset test directory ACL before cleanup (${reset_code}): ${reset_stdout}${reset_stderr}")
  endif()
endfunction()

set(work_root "${LIBBSA_BINARY_DIR}/cli-integration/${CONFIG}")
reset_directory_acl("${work_root}")
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

function(require_match_count text regex expected context)
  string(REGEX MATCHALL "${regex}" matches "${text}")
  list(LENGTH matches actual)
  if(NOT actual EQUAL expected)
    message(FATAL_ERROR "Expected ${context} to match '${regex}' ${expected} time(s), got ${actual}")
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

function(require_child_name directory expected_name)
  if(NOT IS_DIRECTORY "${directory}")
    message(FATAL_ERROR "Expected directory to exist: ${directory}")
  endif()

  file(GLOB children RELATIVE "${directory}" "${directory}/*")
  foreach(child IN LISTS children)
    if(child STREQUAL expected_name)
      return()
    endif()
  endforeach()

  string(REPLACE ";" ", " rendered_children "${children}")
  message(FATAL_ERROR
    "Expected ${directory} to contain a child named exactly '${expected_name}', "
    "found: ${rendered_children}")
endfunction()

function(require_not_exists path context)
  if(EXISTS "${path}")
    message(FATAL_ERROR "Expected ${context} not to exist: ${path}")
  endif()
endfunction()

function(require_no_children directory context)
  if(NOT IS_DIRECTORY "${directory}")
    message(FATAL_ERROR "Expected ${context} directory to exist: ${directory}")
  endif()

  file(GLOB children RELATIVE "${directory}" "${directory}/*")
  if(children)
    string(REPLACE ";" ", " rendered_children "${children}")
    message(FATAL_ERROR "Expected ${context} to be empty, found: ${rendered_children}")
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

function(try_make_directory_unreadable path result_var)
  if(NOT WIN32)
    set(${result_var} "UNSUPPORTED" PARENT_SCOPE)
    return()
  endif()

  current_windows_user_sid(acl_sid acl_sid_status)
  if(NOT acl_sid_status STREQUAL "CREATED")
    set(${result_var} "FAILED" PARENT_SCOPE)
    return()
  endif()

  set(deny_ace "${acl_sid}:(RD)")
  execute_process(
    COMMAND "$ENV{SystemRoot}/System32/icacls.exe" "${path}" /deny "${deny_ace}"
    RESULT_VARIABLE acl_code
    OUTPUT_VARIABLE acl_stdout
    ERROR_VARIABLE acl_stderr
  )
  if(acl_code EQUAL 0)
    set(${result_var} "CREATED" PARENT_SCOPE)
  else()
    message(STATUS "Skipping unreadable-directory test because ACL setup failed (${acl_code}): ${acl_stdout}${acl_stderr}")
    set(${result_var} "FAILED" PARENT_SCOPE)
  endif()
endfunction()

function(restore_directory_readable path)
  if(NOT WIN32)
    return()
  endif()

  current_windows_user_sid(acl_sid acl_sid_status)
  if(NOT acl_sid_status STREQUAL "CREATED")
    message(WARNING "Failed to restore test directory ACL because current-user SID lookup failed")
    return()
  endif()

  execute_process(
    COMMAND "$ENV{SystemRoot}/System32/icacls.exe" "${path}" /remove:d "${acl_sid}"
    RESULT_VARIABLE acl_code
    OUTPUT_VARIABLE acl_stdout
    ERROR_VARIABLE acl_stderr
  )
  if(NOT acl_code EQUAL 0)
    message(WARNING "Failed to restore test directory ACL (${acl_code}): ${acl_stdout}${acl_stderr}")
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

file(READ "${LIBBSA_SOURCE_DIR}/tools/cli/main.cpp" cli_source)
require_contains("${cli_source}" "constexpr std::uint32_t max_cli_worker_count = 1024U" "CLI worker cap source")
require_contains("${cli_source}" "value == \"auto\" || value == \"0\"" "CLI worker auto parsing source")
require_contains("${cli_source}" "std::thread::hardware_concurrency()" "CLI worker auto resolution source")
require_contains("${cli_source}" "hardware_workers == 0U ? 1U : hardware_workers" "CLI worker auto clamp source")
require_contains("${cli_source}" "if (value.empty())" "CLI empty worker-count rejection source")
require_contains("${cli_source}" "parser.add_argument(\"-j\", \"--threads\")" "CLI argparse thread option wiring")
require_match_count("${cli_source}" "resolve_worker_count\\(parser\\.get<std::string>\\(\"--threads\"\\)\\)" 2 "CLI argparse thread-count resolution")
require_match_count("${cli_source}" "write_execution_options\\{worker_count\\}" 4 "CLI pack worker-count forwarding")
string(FIND "${cli_source}" "std::cout << entry.path" raw_list_path_at)
if(NOT raw_list_path_at EQUAL -1)
  message(FATAL_ERROR "CLI list output must not print archive-controlled paths without escaping control characters")
endif()
string(FIND "${cli_source}" "escaped_cli_text(entry.path)" escaped_list_path_at)
if(escaped_list_path_at EQUAL -1)
  message(FATAL_ERROR "CLI list output must render archive-controlled paths through escaped_cli_text")
endif()

run_cli(0 stdout stderr --help)
require_contains("${stdout}" "pack" "top-level help")
require_contains("${stdout}" "unpack" "top-level help")
require_contains("${stdout}" "validate" "top-level help")

run_cli(0 stdout stderr --version)
require_contains("${stdout}" "libbsa" "version output")

string(FIND "${cli_source}" "CommandLineToArgvW(::GetCommandLineW()" wide_argv_decoder_at)
if(wide_argv_decoder_at EQUAL -1)
  message(FATAL_ERROR "CLI Windows entry point must decode the wide command line before dispatch")
endif()
string(FIND "${cli_source}" "WideCharToMultiByte(CP_UTF8" wide_argv_utf8_at)
if(wide_argv_utf8_at EQUAL -1)
  message(FATAL_ERROR "CLI Windows entry point must encode decoded argv as UTF-8 before dispatch")
endif()
string(FIND "${cli_source}" "args.emplace_back(argv[index])" raw_argv_view_at)
if(NOT raw_argv_view_at EQUAL -1)
  message(FATAL_ERROR "CLI dispatch arguments must not be string_views over raw narrow argv bytes")
endif()
string(FIND "${cli_source}" "std::filesystem::path input_dir{parsed.value().positionals[0]}" narrow_pack_input_at)
if(NOT narrow_pack_input_at EQUAL -1)
  message(FATAL_ERROR "CLI pack input paths must decode UTF-8 argv before filesystem operations")
endif()
string(FIND "${cli_source}" "std::filesystem::path output_path{parsed.value().positionals[1]}" narrow_pack_output_at)
if(NOT narrow_pack_output_at EQUAL -1)
  message(FATAL_ERROR "CLI pack output paths must decode UTF-8 argv before filesystem operations")
endif()
string(FIND "${cli_source}" "directory_options::skip_permission_denied" skip_permission_denied_at)
if(NOT skip_permission_denied_at EQUAL -1)
  message(FATAL_ERROR "CLI pack input enumeration must not suppress permission-denied subdirectories")
endif()

run_cli(2 stdout stderr unknown-subcommand)
require_contains("${stderr}" "unknown subcommand" "unknown subcommand diagnostic")

run_cli(0 stdout stderr pack --help)
require_contains("${stdout}" "--threads" "pack help thread option")
require_contains("${stdout}" "-j <value>" "pack help thread alias")
require_contains("${stdout}" "1..1024" "pack help worker range")
require_contains("${stdout}" "auto" "pack help auto worker value")
require_contains("${stdout}" "0 for auto" "pack help zero auto alias")
foreach(token IN ITEMS
    bsa-tes3
    bsa-oblivion
    bsa-fo3
    bsa-sse
    ba2-gnrl-fo4
    ba2-gnrl-sf-v2
    ba2-gnrl-sf-v3
    ba2-dx10-fo4
    ba2-dx10-sf-v2
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

set(thread_root "${work_root}/threads")
set(thread_input "${thread_root}/input")
write_text("${thread_input}/meshes/threaded.nif" "threaded payload\n")

set(thread_integer_archive "${thread_root}/integer.bsa")
run_cli(0 stdout stderr pack --format bsa-tes3 --threads 2 "${thread_input}" "${thread_integer_archive}")
require_contains("${stdout}" "packed 1 file(s)" "pack integer thread-count output")

run_cli(0 stdout stderr pack --format bsa-tes3 -j auto --overwrite "${thread_input}" "${thread_integer_archive}")
require_contains("${stdout}" "packed 1 file(s)" "pack auto thread-count output")

run_cli(0 stdout stderr pack --format bsa-tes3 --threads 0 --overwrite "${thread_input}" "${thread_integer_archive}")
require_contains("${stdout}" "packed 1 file(s)" "pack zero-auto thread-count output")

foreach(bad_threads IN ITEMS -1 nope 1025)
  string(REPLACE "-" "minus" bad_suffix "${bad_threads}")
  set(bad_thread_archive "${thread_root}/bad-${bad_suffix}.bsa")
  run_cli(2 stdout stderr pack --format bsa-tes3 --threads "${bad_threads}" "${thread_root}/missing-input" "${bad_thread_archive}")
  require_contains("${stderr}" "invalid --threads value" "invalid pack thread-count diagnostic")
  require_not_exists("${bad_thread_archive}" "archive from invalid pack thread count")
endforeach()

set(short_bad_thread_archive "${thread_root}/bad-short-minus1.bsa")
run_cli(2 stdout stderr pack --format bsa-tes3 -j -1 "${thread_root}/missing-input" "${short_bad_thread_archive}")
require_contains("${stderr}" "invalid --threads value" "invalid short pack thread-count diagnostic")
require_not_exists("${short_bad_thread_archive}" "archive from invalid short pack thread count")

set(empty_thread_archive "${thread_root}/bad-empty.bsa")
run_cli(2 stdout stderr pack --format bsa-tes3 --threads= "${thread_input}" "${empty_thread_archive}")
require_not_exists("${empty_thread_archive}" "archive from empty pack thread count")

set(thread_unpack_output "${thread_root}/unpack-integer")
run_cli(0 stdout stderr unpack --threads 2 "${thread_integer_archive}" "${thread_unpack_output}")
require_file_text("${thread_unpack_output}/meshes/threaded.nif" "threaded payload\n")

set(thread_unpack_auto_output "${thread_root}/unpack-auto")
run_cli(0 stdout stderr unpack -j auto "${thread_integer_archive}" "${thread_unpack_auto_output}")
require_file_text("${thread_unpack_auto_output}/meshes/threaded.nif" "threaded payload\n")

set(thread_unpack_zero_output "${thread_root}/unpack-zero")
run_cli(0 stdout stderr unpack --threads 0 "${thread_integer_archive}" "${thread_unpack_zero_output}")
require_file_text("${thread_unpack_zero_output}/meshes/threaded.nif" "threaded payload\n")

foreach(bad_threads IN ITEMS -1 nope 1025)
  string(REPLACE "-" "minus" bad_suffix "${bad_threads}")
  set(bad_thread_output "${thread_root}/bad-unpack-${bad_suffix}")
  run_cli(2 stdout stderr unpack --threads "${bad_threads}" "${thread_root}/missing-archive.bsa" "${bad_thread_output}")
  require_contains("${stderr}" "invalid --threads value" "invalid unpack thread-count diagnostic")
  require_not_exists("${bad_thread_output}" "output directory from invalid unpack thread count")
endforeach()

run_cli(1 stdout stderr pack --format bsa-tes3 "${work_root}/does-not-exist" "${work_root}/missing-input.bsa")
require_contains("${stderr}" "io_error" "missing input diagnostic")

set(empty_input_archive "${work_root}/empty-input-out.bsa")
execute_process(
  COMMAND "${LIBBSA_CLI}" pack --format bsa-tes3 "" "${empty_input_archive}"
  RESULT_VARIABLE empty_input_code
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
)
if(NOT empty_input_code EQUAL 1)
  message(FATAL_ERROR
    "Expected pack to reject an empty input path\n"
    "  actual: ${empty_input_code}\n"
    "  stdout:\n${stdout}\n"
    "  stderr:\n${stderr}")
endif()
require_contains("${stderr}" "invalid_argument" "empty pack input path diagnostic")
require_not_exists("${empty_input_archive}" "archive from empty pack input path")

execute_process(
  COMMAND "${LIBBSA_CLI}" pack --format bsa-tes3 "${missing_format_root}" ""
  RESULT_VARIABLE empty_output_code
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
)
if(NOT empty_output_code EQUAL 1)
  message(FATAL_ERROR
    "Expected pack to reject an empty output archive path\n"
    "  actual: ${empty_output_code}\n"
    "  stdout:\n${stdout}\n"
    "  stderr:\n${stderr}")
endif()
require_contains("${stderr}" "invalid_argument" "empty pack output path diagnostic")

pack_unpack_roundtrip(bsa-tes3 bsa)
pack_unpack_roundtrip(bsa-oblivion bsa)
pack_unpack_roundtrip(ba2-gnrl-fo4 ba2)

set(utf8_entry_root "${work_root}/utf8-entry")
set(utf8_entry_input "${utf8_entry_root}/input")
set(utf8_entry_archive "${utf8_entry_root}/packed.bsa")
set(utf8_entry_output "${utf8_entry_root}/output")
set(utf8_entry_path "textures/café.dds")
write_text("${utf8_entry_input}/${utf8_entry_path}" "utf8 texture payload\n")
run_cli(0 stdout stderr pack --format bsa-tes3 "${utf8_entry_input}" "${utf8_entry_archive}")
run_cli(0 stdout stderr list "${utf8_entry_archive}")
require_contains("${stdout}" "${utf8_entry_path}" "UTF-8 archive path list output")
run_cli(0 stdout stderr unpack "${utf8_entry_archive}" "${utf8_entry_output}")
require_file_text("${utf8_entry_output}/${utf8_entry_path}" "utf8 texture payload\n")

set(mixed_case_entry_root "${work_root}/mixed-case-entry")
set(mixed_case_entry_input "${mixed_case_entry_root}/input")
set(mixed_case_entry_archive "${mixed_case_entry_root}/packed.bsa")
set(mixed_case_entry_output "${mixed_case_entry_root}/output")
set(mixed_case_entry_path "Meshes/Tiny/Probe.nif")
write_text("${mixed_case_entry_input}/${mixed_case_entry_path}" "mixed case mesh payload\n")
run_cli(0 stdout stderr pack --format bsa-tes3 "${mixed_case_entry_input}" "${mixed_case_entry_archive}")
run_cli(0 stdout stderr unpack "${mixed_case_entry_archive}" "${mixed_case_entry_output}")
require_file_text("${mixed_case_entry_output}/${mixed_case_entry_path}" "mixed case mesh payload\n")
require_child_name("${mixed_case_entry_output}" "Meshes")
require_child_name("${mixed_case_entry_output}/Meshes" "Tiny")
require_child_name("${mixed_case_entry_output}/Meshes/Tiny" "Probe.nif")

set(mixed_case_selective_output "${mixed_case_entry_root}/selective-output")
run_cli(0 stdout stderr unpack "${mixed_case_entry_archive}" "${mixed_case_selective_output}" --path meshes/tiny/probe.nif)
require_file_text("${mixed_case_selective_output}/${mixed_case_entry_path}" "mixed case mesh payload\n")
require_child_name("${mixed_case_selective_output}" "Meshes")
require_child_name("${mixed_case_selective_output}/Meshes" "Tiny")
require_child_name("${mixed_case_selective_output}/Meshes/Tiny" "Probe.nif")

# The unpack pre-pass derives its destination-directory set from the filtered
# request list, so extracting a subset must create no directory for an entry that
# was not requested -- including when the requested spelling differs from the
# archive's own, which is the case the pre-pass has to resolve through the entry.
set(subset_dirs_root "${work_root}/subset-directories")
set(subset_dirs_input "${subset_dirs_root}/input")
set(subset_dirs_archive "${subset_dirs_root}/packed.bsa")
set(subset_dirs_output "${subset_dirs_root}/output")
write_text("${subset_dirs_input}/Meshes/Tiny/Probe.nif" "requested mesh payload\n")
write_text("${subset_dirs_input}/Textures/Other/Skin.dds" "unrequested texture payload\n")
run_cli(0 stdout stderr pack --format bsa-tes3 "${subset_dirs_input}" "${subset_dirs_archive}")
run_cli(0 stdout stderr unpack "${subset_dirs_archive}" "${subset_dirs_output}" --path meshes/tiny/probe.nif)
require_contains("${stdout}" "extracted 1 of 1" "subset extraction count")
require_file_text("${subset_dirs_output}/Meshes/Tiny/Probe.nif" "requested mesh payload\n")
require_child_name("${subset_dirs_output}" "Meshes")
require_not_exists("${subset_dirs_output}/Textures" "destination directory for an unrequested entry")

set(selective_archive "${work_root}/roundtrip-bsa-tes3/packed.bsa")
set(selective_output "${work_root}/selective-output")

# Opening the archive precedes output preparation, but a valid archive prepares
# its root even when every requested entry is missing.
set(bad_archive_output "${work_root}/bad-archive-output")
run_cli(1 stdout stderr unpack "${work_root}/absent.bsa" "${bad_archive_output}")
require_contains("${stderr}" "absent.bsa" "archive-open failure context")
require_not_exists("${bad_archive_output}" "output directory after archive-open failure")
set(all_missing_output "${work_root}/all-missing-output")
run_cli(1 stdout stderr unpack "${selective_archive}" "${all_missing_output}"
  --path missing/first.txt --path missing/second.txt)
require_contains("${stdout}" "extracted 0 of 2 requested entries" "all-missing extraction count")
require_no_children("${all_missing_output}" "all-missing output root")
string(FIND "${stderr}" "missing/first.txt" first_missing_at)
string(FIND "${stderr}" "missing/second.txt" second_missing_at)
if(first_missing_at LESS 0 OR second_missing_at LESS first_missing_at)
  message(FATAL_ERROR "Missing-entry diagnostics must preserve request order: ${stderr}")
endif()

# Exact duplicate requests share publication and retain their own result record.
set(duplicate_output "${work_root}/duplicate-output")
run_cli(1 stdout stderr unpack --threads 2 "${selective_archive}" "${duplicate_output}"
  --path missing/first.txt --path meshes/model.nif --path meshes/model.nif
  --path missing/second.txt)
require_contains("${stdout}" "extracted 2 of 4 requested entries" "duplicate extraction count")
require_file_text("${duplicate_output}/meshes/model.nif" "mesh payload for bsa-tes3\n")
string(FIND "${stderr}" "missing/first.txt" first_missing_at)
string(FIND "${stderr}" "missing/second.txt" second_missing_at)
if(first_missing_at LESS 0 OR second_missing_at LESS first_missing_at)
  message(FATAL_ERROR "Partial extraction diagnostics must preserve request order: ${stderr}")
endif()

run_cli(0 stdout stderr unpack --help)
require_contains("${stdout}" "--threads" "unpack help thread option")
require_contains("${stdout}" "-j <value>" "unpack help thread alias")
require_contains("${stdout}" "1..1024" "unpack help worker range")
require_contains("${stdout}" "auto" "unpack help auto worker value")
require_contains("${stdout}" "0 for auto" "unpack help zero auto alias")
if(WIN32)
  set(utf8_host_root "${work_root}/utf8-host-Ångström-日本語")
  set(utf8_host_archive "${utf8_host_root}/packed.bsa")
  set(utf8_host_output "${work_root}/utf8-host-output-Ångström-日本語")
  file(MAKE_DIRECTORY "${utf8_host_root}")
  file(COPY_FILE "${selective_archive}" "${utf8_host_archive}")
  run_cli(0 stdout stderr list "${utf8_host_archive}")
  require_contains("${stdout}" "meshes/model.nif" "UTF-8 host archive path list output")
  run_cli(0 stdout stderr info "${utf8_host_archive}")
  require_contains("${stdout}" "variant: tes3" "UTF-8 host archive path info output")
  run_cli(0 stdout stderr validate "${utf8_host_archive}")
  require_contains("${stdout}" "valid: yes" "UTF-8 host archive path validate output")
  run_cli(0 stdout stderr unpack "${utf8_host_archive}" "${utf8_host_output}")
  require_file_text("${utf8_host_output}/meshes/model.nif" "mesh payload for bsa-tes3\n")
endif()
run_cli(1 stdout stderr unpack "${selective_archive}" "${selective_output}" --path meshes/model.nif --path missing/file.txt)
require_file_text("${selective_output}/meshes/model.nif" "mesh payload for bsa-tes3\n")
require_contains("${stderr}" "not_found" "missing selective path diagnostic")

run_cli(1 stdout stderr unpack "${selective_archive}" "${selective_output}")
require_contains("${stderr}" "--overwrite" "unpack overwrite refusal")

execute_process(
  COMMAND "${LIBBSA_CLI}" unpack "${selective_archive}" ""
  RESULT_VARIABLE empty_unpack_output_code
  OUTPUT_VARIABLE stdout
  ERROR_VARIABLE stderr
)
if(NOT empty_unpack_output_code EQUAL 1)
  message(FATAL_ERROR
    "Expected unpack to reject an empty output path\n"
    "  actual: ${empty_unpack_output_code}\n"
    "  stdout:\n${stdout}\n"
    "  stderr:\n${stderr}")
endif()
require_contains("${stderr}" "invalid_argument" "empty unpack output path diagnostic")

set(traversal_output "${work_root}/traversal-output")
run_cli(1 stdout stderr unpack "${selective_archive}" "${traversal_output}" --path ../escape.txt)
if(EXISTS "${work_root}/escape.txt")
  message(FATAL_ERROR "Traversal extraction wrote outside the output root")
endif()

# The case above exercises the *request* path: `../escape.txt` names no entry, so it
# fails lookup. An archive entry whose own path escapes the output root cannot be
# reached from a parsed archive at all -- normalize_archive_path rejects any segment
# that is "." or "..", a leading separator, and a drive-rooted path, so such an
# archive fails to open. The reachable destination rejections are the Windows-unsafe
# component names, and the pre-pass behaviour for those is covered against the
# tes3_windows_unsafe_names fixture below.

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

  set(pack_unreadable_root "${work_root}/pack-unreadable")
  set(pack_unreadable_input "${pack_unreadable_root}/input")
  set(pack_unreadable_blocked "${pack_unreadable_input}/blocked")
  set(pack_unreadable_archive "${pack_unreadable_root}/packed.bsa")
  write_text("${pack_unreadable_input}/visible.txt" "visible payload\n")
  write_text("${pack_unreadable_blocked}/hidden.txt" "hidden payload\n")
  try_make_directory_unreadable("${pack_unreadable_blocked}" pack_unreadable_status)
  if(pack_unreadable_status STREQUAL "CREATED")
    execute_process(
      COMMAND "${LIBBSA_CLI}" pack --format bsa-tes3 "${pack_unreadable_input}" "${pack_unreadable_archive}"
      RESULT_VARIABLE unreadable_code
      OUTPUT_VARIABLE stdout
      ERROR_VARIABLE stderr
    )
    restore_directory_readable("${pack_unreadable_blocked}")
    if(NOT unreadable_code EQUAL 1)
      message(FATAL_ERROR
        "Expected pack to fail on unreadable input subdirectory\n"
        "  actual: ${unreadable_code}\n"
        "  stdout:\n${stdout}\n"
        "  stderr:\n${stderr}")
    endif()
    require_contains("${stderr}" "io_error" "pack unreadable input diagnostic")
    require_not_exists("${pack_unreadable_archive}" "archive from unreadable input directory")
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

set(tes3_windows_unsafe_names_fixture "${generated_archive_dir}/tes3_windows_unsafe_names.bsa")
if(WIN32 AND EXISTS "${tes3_windows_unsafe_names_fixture}")
  set(windows_unsafe_names_output "${work_root}/windows-unsafe-names-output")
  run_cli(1 stdout stderr unpack "${tes3_windows_unsafe_names_fixture}" "${windows_unsafe_names_output}")
  require_contains("${stdout}" "extracted 0 of 6" "Windows unsafe archive entry extraction count")
  require_contains("${stderr}" "invalid_argument" "Windows unsafe archive entry path diagnostic")
  require_contains("${stderr}" "Windows-reserved" "Windows reserved device path diagnostic")
  require_contains("${stderr}" "trailing dot or space" "Windows trailing dot/space path diagnostic")
  require_contains("${stderr}" "colon" "Windows ADS-style stream path diagnostic")
  require_not_exists("${windows_unsafe_names_output}/textures/file.txt:stream" "ADS-style stream destination")

  # Every entry in this fixture has a destination the CLI refuses, and the pre-pass
  # refuses them before any worker starts. Nothing may be created under the output
  # root -- not the payloads and not the directory two of the rejected entries would
  # otherwise have lived in.
  set(windows_unsafe_names_prepass_output "${work_root}/windows-unsafe-names-prepass")
  run_cli(1 stdout stderr unpack "${tes3_windows_unsafe_names_fixture}" "${windows_unsafe_names_prepass_output}")
  require_contains("${stdout}" "extracted 0 of 6" "rejected-destination extraction count")
  require_not_exists("${windows_unsafe_names_prepass_output}/textures" "destination directory for a rejected entry")
  require_no_children("${windows_unsafe_names_prepass_output}" "rejected-destination extraction output")
endif()

set(corrupt_compressed_fixture "${generated_archive_dir}/malformed_corrupt_compressed_payload.bsa")
if(WIN32 AND EXISTS "${corrupt_compressed_fixture}")
  set(corrupt_extract_root "${work_root}/corrupt-compressed-extract")
  set(corrupt_extract_archive "${corrupt_extract_root}/corrupt.bsa")
  file(MAKE_DIRECTORY "${corrupt_extract_root}")
  file(COPY_FILE "${corrupt_compressed_fixture}" "${corrupt_extract_archive}")

  set(corrupt_fresh_output "${corrupt_extract_root}/fresh-output")
  run_cli(1 stdout stderr unpack "${corrupt_extract_archive}" "${corrupt_fresh_output}")
  require_contains("${stderr}" "format_error" "corrupt compressed payload diagnostic")
  require_not_exists("${corrupt_fresh_output}/meshes/tiny/packedmesh.nif" "failed fresh extraction destination")

  set(corrupt_overwrite_output "${corrupt_extract_root}/overwrite-output")
  set(corrupt_overwrite_path "${corrupt_overwrite_output}/meshes/tiny/packedmesh.nif")
  write_text("${corrupt_overwrite_path}" "sentinel payload\n")
  run_cli(1 stdout stderr unpack "${corrupt_extract_archive}" "${corrupt_overwrite_output}" --overwrite)
  require_contains("${stderr}" "format_error" "corrupt compressed overwrite diagnostic")
  require_file_text("${corrupt_overwrite_path}" "sentinel payload\n")
  file(GLOB corrupt_overwrite_temps RELATIVE "${corrupt_overwrite_output}/meshes/tiny" "${corrupt_overwrite_output}/meshes/tiny/.packedmesh.nif.bsa-tmp-*")
  if(corrupt_overwrite_temps)
    message(FATAL_ERROR "Unexpected corrupt overwrite extraction temp leftovers: ${corrupt_overwrite_temps}")
  endif()
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

  set(dx10_v2_archive "${dx10_root}/texture-sf-v2.ba2")
  set(dx10_v2_output "${dx10_root}/output-sf-v2")
  run_cli(0 stdout stderr pack --format ba2-dx10-sf-v2 "${dx10_input}" "${dx10_v2_archive}")
  run_cli(0 stdout stderr unpack "${dx10_v2_archive}" "${dx10_v2_output}")
  if(NOT EXISTS "${dx10_v2_output}/textures/cli/bc1.dds")
    message(FATAL_ERROR "Starfield v2 DX10 unpack did not write the expected DDS path")
  endif()
endif()
