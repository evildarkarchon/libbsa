# Proves the concurrency guarantee by running two libbsa_tests instances at once.
#
# == Why this test is at CTest level and not inside the binary ==
#
# Issue #60 was a cross-process defect: two test binaries sharing the system temp
# root deleted each other's BA2 DX10 snapshot directories, producing a *moving* set
# of cleanup failures that each passed on --rerun-failed. Issue #62 fixed it by
# giving every test process a private temp root. Nothing inside a single process can
# fail when that regresses, which is exactly how the hazard survived until it was
# found by accident. The proof therefore has to start two processes, and the only
# place in this repository that can is a script-driven CTest case -- the same shape
# the package-consumer, CLI integration, and export-surface checks already use.
#
# == What is asserted, and in which order ==
#
# 1. Two instances, filtered to the BA2 DX10 snapshot cleanup cases and both carrying
#    the #65 opt-out, run concurrently and both exit zero.
# 2. A fast child proves the lock holder records an exact non-zero exit code and
#    keeps its stdout and stderr separate.
# 3. A third instance started *without* the opt-out, while the guard's lock is held,
#    exits non-zero and says why.
#
# The first is the regression proof. The third covers the guard from #65 in the same
# case, because the opt-out the first half depends on and the refusal it disables are
# two halves of one mechanism.
#
# == How the two instances are started at the same time ==
#
# `execute_process` with more than one COMMAND builds a pipeline: every process is
# started at once and the call returns when all of them have finished, with one exit
# code per process in RESULTS_VARIABLE. CMake script mode offers nothing else that
# starts a process without also waiting for it, and a pipeline is genuinely
# concurrent, so this is the mechanism rather than a workaround.
#
# Its one sharp edge is the pipe. The first instance's stdout is the second's stdin,
# and a Catch2 binary never reads stdin, so a first instance chatty enough to fill
# the pipe buffer would block forever. Both instances are therefore given Catch2's
# `-o`, which sends all reporter output to a file and leaves stdout empty. The files
# are read back and reported on failure, so nothing is lost by redirecting them.
#
# == Why the DX10 cleanup filter ==
#
# `[ba2_dx10_writer][cleanup]` selects the snapshot-cleanup cases and nothing else.
# Those are the cases whose helpers diff a directory scan to decide which snapshot
# directories they own, which is the attribution issue #60 broke, so they are both
# the cases that reproduce the defect and a small enough set to keep this case off
# the critical path of the developer and coding-agent inner loop.

if(NOT DEFINED LIBBSA_TEST_BINARY OR NOT EXISTS "${LIBBSA_TEST_BINARY}")
  message(FATAL_ERROR "LIBBSA_TEST_BINARY must name the built libbsa_tests executable")
endif()

if(NOT DEFINED LIBBSA_BINARY_DIR)
  message(FATAL_ERROR "LIBBSA_BINARY_DIR must name the build root")
endif()

if(NOT DEFINED LIBBSA_LOCK_HOLDER_SCRIPT OR NOT EXISTS "${LIBBSA_LOCK_HOLDER_SCRIPT}")
  message(FATAL_ERROR "LIBBSA_LOCK_HOLDER_SCRIPT must name hold-single-instance-lock.ps1")
endif()

if(NOT DEFINED CONFIG)
  set(CONFIG "single")
endif()

# Kept in step with tests/support/single_instance_guard.hpp by hand. There is no way
# to read a C++ constant from a CMake script, so the two spellings are pinned against
# each other by the probe below instead: a driver that held some other name would see
# its probe start happily, and the refusal assertions would fail.
set(single_instance_lock_name "Local\\libbsa-tests-single-instance")
set(single_instance_opt_out_variable "LIBBSA_TEST_ALLOW_CONCURRENT")

# Catch2 test spec selecting the BA2 DX10 snapshot cleanup cases. Both tags are
# required, so this is an intersection rather than a union.
set(dx10_cleanup_filter "[ba2_dx10_writer][cleanup]")

# Generous relative to the sub-second runs expected here. It exists to turn a hang
# into a reported failure, not to police runtime.
set(instance_timeout_seconds 300)

# Windows PowerShell's fixed location, the same one tests/cli/cli_integration.cmake
# resolves. Preferred over find_program because it cannot be shadowed by whatever
# `pwsh` happens to be on PATH.
set(windows_powershell "$ENV{SystemRoot}/System32/WindowsPowerShell/v1.0/powershell.exe")
if(NOT EXISTS "${windows_powershell}")
  message(FATAL_ERROR "Windows PowerShell is required to hold the single-instance lock: ${windows_powershell}")
endif()

set(command_interpreter "$ENV{ComSpec}")
if(NOT EXISTS "${command_interpreter}")
  message(FATAL_ERROR "The Windows command interpreter is required to probe the lock holder: ${command_interpreter}")
endif()

set(work_root "${LIBBSA_BINARY_DIR}/concurrent-test-instances/${CONFIG}")
file(REMOVE_RECURSE "${work_root}")
file(MAKE_DIRECTORY "${work_root}")

# Collected rather than raised. Nothing below may call message(FATAL_ERROR) directly,
# because the work root has to be removed even when an instance fails -- and a
# FATAL_ERROR ends the script immediately, skipping the cleanup at the bottom.
#
# A plain string rather than a list, deliberately. Captured Catch2 output contains
# semicolons, and a CMake list is a semicolon-delimited string, so appending output
# to a list would split one report into several elements and any join would then
# either duplicate or destroy the semicolons in the text.
set(failure_report "")
set(any_failure FALSE)

# A function rather than a macro: macro arguments are pasted into the body as text
# and would re-expand any `${...}` sequence that happened to appear in captured
# output, while function arguments are ordinary variables and are read exactly once.
function(record_failure text)
  string(APPEND failure_report "${text}\n")
  set(failure_report "${failure_report}" PARENT_SCOPE)
  set(any_failure TRUE PARENT_SCOPE)
endfunction()

# Records a failure unless `text` contains `needle`.
#
# The three diagnostic checks below are the same shape, and tests/cli/cli_integration.cmake
# already names this shape `require_contains` for its own use. Kept local rather than
# shared with it: there is no common tests/*.cmake include today, and inventing one to
# hold a four-line helper would be a larger change than the duplication it removes.
function(require_contains text needle message)
  string(FIND "${text}" "${needle}" found_at)
  if(found_at EQUAL -1)
    # record_failure's PARENT_SCOPE reaches this function's scope, not the script's,
    # so the two variables have to be forwarded one level further by hand. Without
    # this, every failure recorded through require_contains would be discarded when
    # the function returned.
    record_failure("${message}\n  output:\n${text}")
    set(failure_report "${failure_report}" PARENT_SCOPE)
    set(any_failure TRUE PARENT_SCOPE)
  endif()
endfunction()

function(read_log_file path out_var)
  if(EXISTS "${path}")
    file(READ "${path}" contents)
  else()
    set(contents "<no output file was written>")
  endif()
  set(${out_var} "${contents}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# 1. Two concurrent instances, both opted out of the guard, must both succeed.
# ---------------------------------------------------------------------------

set(first_log "${work_root}/instance-a.txt")
set(second_log "${work_root}/instance-b.txt")

string(TIMESTAMP concurrent_run_started "%s")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E env "${single_instance_opt_out_variable}=1"
    "${LIBBSA_TEST_BINARY}" "${dx10_cleanup_filter}" -o "${first_log}"
  COMMAND ${CMAKE_COMMAND} -E env "${single_instance_opt_out_variable}=1"
    "${LIBBSA_TEST_BINARY}" "${dx10_cleanup_filter}" -o "${second_log}"
  RESULTS_VARIABLE instance_results
  # Captured and then deliberately unused: `-o` sends every reporter byte to a file,
  # so the last instance's stdout should be empty. Capturing it keeps anything that
  # does appear out of this case's own output, where it would read as a message from
  # the driver rather than from a test binary.
  OUTPUT_VARIABLE pipeline_stdout
  ERROR_VARIABLE pipeline_stderr
  TIMEOUT ${instance_timeout_seconds}
)

string(TIMESTAMP concurrent_run_finished "%s")
math(EXPR concurrent_run_seconds "${concurrent_run_finished} - ${concurrent_run_started}")
message(STATUS "Two concurrent libbsa_tests instances finished in ${concurrent_run_seconds}s")

read_log_file("${first_log}" first_output)
read_log_file("${second_log}" second_output)

foreach(instance_index RANGE 0 1)
  list(GET instance_results ${instance_index} instance_result)
  if(instance_index EQUAL 0)
    set(instance_label "A")
    set(instance_output "${first_output}")
  else()
    set(instance_label "B")
    set(instance_output "${second_output}")
  endif()

  if(NOT instance_result STREQUAL "0")
    # The pipeline's stderr is reported alongside the instance's own log because the
    # two carry different failures. Catch2's `-o` file holds assertion failures; a
    # start-up failure -- a private temp root that could not be installed, say -- is
    # thrown before the reporter exists and reaches stderr instead. Reporting only
    # the log would leave that second kind looking like a silent non-zero exit.
    # Both instances share one stderr capture, which is why it is labelled as the
    # pipeline's rather than as this instance's.
    record_failure("Concurrent instance ${instance_label} did not exit zero.\n  result: ${instance_result}\n  filter: ${dx10_cleanup_filter}\n  output:\n${instance_output}\n  pipeline stderr (both instances):\n${pipeline_stderr}")
  endif()

  # Names filter rot as filter rot. Catch2 already fails a run that matched nothing,
  # so the exit code above catches this on its own -- but it catches it as a bare
  # exit code 2, which reads as "a test failed" rather than "this case has quietly
  # stopped testing anything". A concurrency proof that selects no cases is still
  # green in every other respect, which is the shape of failure issue #60 is about.
  #
  # Phrased as the absence of Catch2's no-match line rather than the presence of a
  # summary line. Searching for "test case" looked equivalent and was not: Catch2
  # writes "No test cases matched '<spec>'", which contains that substring, so the
  # check passed in exactly the situation it was written to detect.
  string(FIND "${instance_output}" "No test cases matched" matched_nothing)
  if(NOT matched_nothing EQUAL -1)
    record_failure("Concurrent instance ${instance_label} matched no test cases, so the filter '${dx10_cleanup_filter}' no longer selects the BA2 DX10 snapshot cleanup cases.\n  output:\n${instance_output}")
  endif()
endforeach()

# ---------------------------------------------------------------------------
# 2. The lock holder must preserve a fast child's exact process result.
# ---------------------------------------------------------------------------

set(fast_child_stdout_log "${work_root}/fast-child-stdout.txt")
set(fast_child_stderr_log "${work_root}/fast-child-stderr.txt")
set(fast_child_exit_code_file "${work_root}/fast-child-exit-code.txt")
string(RANDOM LENGTH 16 ALPHABET 0123456789abcdef fast_child_lock_suffix)

# Kept space-free to honour hold-single-instance-lock.ps1's Windows PowerShell 5.1
# argument contract. cmd.exe's echo. spelling emits the marker without the dot.
set(fast_child_arguments "/d/c\"echo.holder-fast-stdout&echo.holder-fast-stderr>&2&exit/b37\"")

execute_process(
  COMMAND "${windows_powershell}"
    -NoProfile
    -NonInteractive
    -ExecutionPolicy Bypass
    -File "${LIBBSA_LOCK_HOLDER_SCRIPT}"
    -LockName "Local\\libbsa-tests-fast-child-${fast_child_lock_suffix}"
    -OptOutVariable "${single_instance_opt_out_variable}"
    -ChildCommand "${command_interpreter}"
    -ChildArguments "${fast_child_arguments}"
    -StdoutFile "${fast_child_stdout_log}"
    -StderrFile "${fast_child_stderr_log}"
    -ExitCodeFile "${fast_child_exit_code_file}"
    -TimeoutSeconds ${instance_timeout_seconds}
  RESULT_VARIABLE fast_child_holder_result
  OUTPUT_VARIABLE fast_child_holder_stdout
  ERROR_VARIABLE fast_child_holder_stderr
  TIMEOUT ${instance_timeout_seconds}
)

read_log_file("${fast_child_stdout_log}" fast_child_stdout)
read_log_file("${fast_child_stderr_log}" fast_child_stderr)
set(fast_child_exit_code "")
if(EXISTS "${fast_child_exit_code_file}")
  file(READ "${fast_child_exit_code_file}" fast_child_exit_code)
  string(STRIP "${fast_child_exit_code}" fast_child_exit_code)
endif()
string(STRIP "${fast_child_stdout}" fast_child_stdout)
string(STRIP "${fast_child_stderr}" fast_child_stderr)

if(NOT fast_child_holder_result STREQUAL "0")
  record_failure("The single-instance lock holder failed while probing fast-child result capture.\n  result: ${fast_child_holder_result}\n  stdout:\n${fast_child_holder_stdout}\n  stderr:\n${fast_child_holder_stderr}")
endif()
if(NOT fast_child_exit_code STREQUAL "37")
  record_failure("The fast child exit code was not recorded exactly.\n  expected: 37\n  actual: ${fast_child_exit_code}")
endif()
if(NOT fast_child_stdout STREQUAL "holder-fast-stdout")
  record_failure("The fast child's stdout was not captured exactly.\n  expected: holder-fast-stdout\n  actual: ${fast_child_stdout}")
endif()
if(NOT fast_child_stderr STREQUAL "holder-fast-stderr")
  record_failure("The fast child's stderr was not captured exactly.\n  expected: holder-fast-stderr\n  actual: ${fast_child_stderr}")
endif()

# ---------------------------------------------------------------------------
# 3. A second instance without the opt-out must be refused, and must say why.
# ---------------------------------------------------------------------------

set(probe_stdout_log "${work_root}/refused-instance-stdout.txt")
set(probe_stderr_log "${work_root}/refused-instance-stderr.txt")
set(probe_exit_code_file "${work_root}/refused-instance-exit-code.txt")

execute_process(
  COMMAND "${windows_powershell}"
    -NoProfile
    -NonInteractive
    -ExecutionPolicy Bypass
    -File "${LIBBSA_LOCK_HOLDER_SCRIPT}"
    -LockName "${single_instance_lock_name}"
    -OptOutVariable "${single_instance_opt_out_variable}"
    -ChildCommand "${LIBBSA_TEST_BINARY}"
    -ChildArguments "${dx10_cleanup_filter}"
    -StdoutFile "${probe_stdout_log}"
    -StderrFile "${probe_stderr_log}"
    -ExitCodeFile "${probe_exit_code_file}"
    # Passed rather than left to the script's default, so that one value bounds both
    # halves of this case and the CTest TIMEOUT stays the outer bound of the two.
    -TimeoutSeconds ${instance_timeout_seconds}
  RESULT_VARIABLE holder_result
  OUTPUT_VARIABLE holder_stdout
  ERROR_VARIABLE holder_stderr
  TIMEOUT ${instance_timeout_seconds}
)

read_log_file("${probe_stdout_log}" probe_stdout)
read_log_file("${probe_stderr_log}" probe_stderr)
set(probe_output "${probe_stdout}${probe_stderr}")

if(NOT holder_result STREQUAL "0")
  record_failure("The single-instance lock holder failed, so nothing was proved about the guard.\n  result: ${holder_result}\n  stdout:\n${holder_stdout}\n  stderr:\n${holder_stderr}\n  probe output:\n${probe_output}")
else()
  set(probe_exit_code "")
  if(EXISTS "${probe_exit_code_file}")
    file(READ "${probe_exit_code_file}" probe_exit_code)
    string(STRIP "${probe_exit_code}" probe_exit_code)
  endif()

  if(probe_exit_code STREQUAL "")
    record_failure("The refused instance recorded no exit code.\n  holder stdout:\n${holder_stdout}\n  holder stderr:\n${holder_stderr}")
  elseif(probe_exit_code STREQUAL "0")
    record_failure("A second instance started without ${single_instance_opt_out_variable}, while the single-instance lock was held, exited zero instead of refusing to start.\n  output:\n${probe_output}")
  endif()

  # Names the condition rather than failing generically. The phrase is restated here
  # rather than read from the guard, for the same reason the unit test restates it:
  # a check that asked the guard for its own message could not tell a message that
  # names the condition from one that says nothing at all.
  require_contains("${probe_output}" "already running"
    "The refused instance's diagnostic does not name the condition -- a reader would have no way to tell it from a real test failure.")

  # Mentions the opt-out. This half deliberately *is* shared with the guard: a check
  # that looked for some other spelling would prove nothing about the variable the
  # guard actually reads, and the first half of this case depends on that variable.
  require_contains("${probe_output}" "${single_instance_opt_out_variable}"
    "The refused instance's diagnostic does not mention ${single_instance_opt_out_variable}, so a reader who wants two instances is not told how.")
endif()

# ---------------------------------------------------------------------------
# Cleanup, then report.
# ---------------------------------------------------------------------------

# Unconditional, and before any failure is raised: everything the report needs has
# already been read into variables, so a failing instance leaves no more behind than
# a passing one. The instances' own private temp roots are removed by the instances
# themselves; one killed by the timeout above leaves a root that the next test
# process's start-up sweep collects (issue #63).
file(REMOVE_RECURSE "${work_root}")

if(any_failure)
  message(FATAL_ERROR "Concurrent test-instance proof failed.\n\n${failure_report}")
endif()
