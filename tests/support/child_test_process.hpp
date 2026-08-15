#pragma once

/// Running a second copy of `libbsa_tests` and getting an answer back from it.
///
/// Several properties this suite has to prove are invisible from inside the
/// process that has them. A process cannot watch its own teardown, it cannot check
/// that *another* process's start-up sweep spared its directories, and it cannot
/// be refused by a guard it is itself holding. A real second process is the only
/// instrument available for any of them, so the spawning machinery is shared here
/// rather than copied into each test file that needs it.
///
/// Header-only, and it uses Catch2 assertion macros: everything here runs inside a
/// test body, which is where those macros have the active assertion context they
/// need. That is the line between this header and `private_temp_root.cpp` or
/// `single_instance_guard.cpp`, which run from a reporter callback and therefore
/// cannot use them.

#include <catch2/catch_test_macros.hpp>

#include "support/single_instance_guard.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::tests {

/// Set by the out-of-process teardown check on the child it spawns. The child
/// writes its own private root to the named file so the parent can assert the
/// directory is gone once the child has exited.
///
/// Doubles as the "did a test body actually run?" probe for the single-instance
/// guard: a refused child never reaches a test body, so the file never appears.
inline constexpr const wchar_t* private_root_report_variable =
    L"LIBBSA_TEST_PRIVATE_ROOT_REPORT";

/// Set by the out-of-process sweep check on the child it spawns. The child writes
/// what its start-up sweep decided to the named file, so the parent can assert the
/// child *considered* the parent's live root and chose to spare it rather than
/// merely failing to delete it.
inline constexpr const wchar_t* startup_sweep_report_variable =
    L"LIBBSA_TEST_STARTUP_SWEEP_REPORT";

/// Closes a Win32 handle on destruction so a failing REQUIRE cannot leak one.
class scoped_handle {
   public:
    explicit scoped_handle(HANDLE handle) noexcept : handle_{handle} {}

    scoped_handle(const scoped_handle&) = delete;
    scoped_handle& operator=(const scoped_handle&) = delete;
    scoped_handle(scoped_handle&&) = delete;
    scoped_handle& operator=(scoped_handle&&) = delete;

    ~scoped_handle() { reset(); }

    /// Closes the handle early. The sweep tests need this: releasing an owner
    /// marker mid-test is how they turn a live root into a dead one, which is the
    /// only way to show the sweep's decision follows the handle rather than the
    /// directory's name.
    void reset() noexcept {
        if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
        }
        handle_ = INVALID_HANDLE_VALUE;
    }

    HANDLE get() const noexcept { return handle_; }
    bool valid() const noexcept { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }

   private:
    HANDLE handle_;
};

/// Reads a variable from the Win32 process environment block.
///
/// That is the store a spawned child inherits, and the store
/// `SetEnvironmentVariableW` writes; the CRT's own copy is not guaranteed to carry
/// a variable the parent set after start-up.
inline std::optional<std::wstring> win32_environment_value(const wchar_t* name) {
    const auto required = ::GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) {
        return std::nullopt;
    }

    std::wstring value(required, L'\0');
    const auto written = ::GetEnvironmentVariableW(name, value.data(), required);
    REQUIRE(written < required);
    value.resize(written);
    return value;
}

/// Sets or deletes a Win32 environment variable for as long as it is alive, then
/// puts back whatever was there before.
///
/// A spawned child inherits the parent's environment block, so this is how a test
/// hands a value to a child without leaking it into every later test case.
///
/// Restoring the *previous* value rather than always deleting matters for the
/// single-instance opt-out: a developer may well have it set in their shell, and a
/// test that cleared it permanently would silently change the conditions of every
/// case that ran afterwards.
class scoped_environment_variable {
   public:
    /// @param name Variable to set. Taken by value and *owned*: the name is read
    ///        again in the destructor, long after the caller's expression has
    ///        ended, so a borrowed pointer into a caller-side temporary would
    ///        dangle. Callers do build names from `wstring_view` constants.
    /// @param value New value, or nullopt to delete the variable for the duration.
    scoped_environment_variable(std::wstring name, std::optional<std::wstring> value)
        : name_{std::move(name)}, previous_{win32_environment_value(name_.c_str())} {
        REQUIRE(::SetEnvironmentVariableW(name_.c_str(), value ? value->c_str() : nullptr) != FALSE);
    }

    /// Convenience for the common case of handing a path to a child.
    scoped_environment_variable(std::wstring name, const std::filesystem::path& value)
        : scoped_environment_variable{std::move(name),
                                      std::optional<std::wstring>{value.native()}} {}

    scoped_environment_variable(const scoped_environment_variable&) = delete;
    scoped_environment_variable& operator=(const scoped_environment_variable&) = delete;
    scoped_environment_variable(scoped_environment_variable&&) = delete;
    scoped_environment_variable& operator=(scoped_environment_variable&&) = delete;

    // A null pointer is how SetEnvironmentVariableW deletes a variable, which is
    // how an originally-unset one is restored rather than left behind as an empty
    // string that a reader would treat as a value.
    ~scoped_environment_variable() {
        ::SetEnvironmentVariableW(name_.c_str(), previous_ ? previous_->c_str() : nullptr);
    }

   private:
    std::wstring name_;
    std::optional<std::wstring> previous_;
};

inline std::filesystem::path own_executable_path() {
    // Deliberately larger than MAX_PATH: a build tree can sit deeper than that,
    // and GetModuleFileNameW signals truncation only by filling the buffer.
    std::wstring buffer(4096, L'\0');
    const auto written =
        ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    REQUIRE(written > 0);
    REQUIRE(static_cast<std::size_t>(written) < buffer.size());
    buffer.resize(written);
    return std::filesystem::path{buffer};
}

inline std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    REQUIRE(stream.is_open());
    std::ostringstream text;
    text << stream.rdbuf();
    return text.str();
}

inline void write_text_file(const std::filesystem::path& path, std::string_view contents) {
    std::ofstream stream{path, std::ios::binary | std::ios::trunc};
    REQUIRE(stream.is_open());
    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    REQUIRE(stream.good());
}

/// How to launch the child.
struct child_test_process_options {
    /// Arguments, each passed as one quoted argv entry.
    std::vector<std::wstring> arguments;

    /// Whether the child is allowed past the single-instance guard.
    ///
    /// Defaults to true because the parent is itself a running instance, so every
    /// child that means to run a test body needs the opt-out. It is applied as an
    /// explicit set-or-delete rather than by inheritance in *both* directions: a
    /// developer who has the opt-out set in their own shell would otherwise turn
    /// every "this child must be refused" case into a false pass.
    bool allow_concurrent_instances{true};
};

/// Runs a second copy of this test binary, waits for it to exit, and returns its
/// exit code.
///
/// The child's output is captured to `log_path` rather than inherited, so a
/// passing run stays quiet and a failing one still has the child's report to
/// explain itself. The child inherits this process's environment, and therefore
/// its `TMP`/`TEMP`, which is what puts the child's own private root -- and the
/// system temp root its start-up sweep scans -- inside this process's private root.
///
/// @param options Arguments and guard handling for the child.
/// @param log_path File to capture the child's stdout and stderr into.
/// @return The child's exit code.
inline DWORD run_child_test_binary(const child_test_process_options& options,
                                   const std::filesystem::path& log_path) {
    const scoped_environment_variable opt_out{
        std::wstring{single_instance_opt_out_variable},
        options.allow_concurrent_instances ? std::optional<std::wstring>{L"1"} : std::nullopt};

    SECURITY_ATTRIBUTES inheritable{};
    inheritable.nLength = sizeof(inheritable);
    inheritable.bInheritHandle = TRUE;
    const scoped_handle child_log{::CreateFileW(log_path.native().c_str(), GENERIC_WRITE,
                                                FILE_SHARE_READ | FILE_SHARE_WRITE, &inheritable,
                                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
    REQUIRE(child_log.valid());

    // CreateProcessW may write to its command-line argument, so it cannot be a
    // string literal or a const buffer.
    std::wstring command_line = L"\"" + own_executable_path().native() + L"\"";
    for (const auto& argument : options.arguments) {
        command_line += L" \"" + argument + L"\"";
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = child_log.get();
    startup.hStdError = child_log.get();

    PROCESS_INFORMATION process{};
    const auto started = ::CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, TRUE, 0,
                                          nullptr, nullptr, &startup, &process);
    REQUIRE(started != FALSE);

    const scoped_handle child_process{process.hProcess};
    const scoped_handle child_thread{process.hThread};

    // Generous, because the ASan lane is slow to start a process. A hang here
    // should fail the test rather than wedge the suite.
    REQUIRE(::WaitForSingleObject(child_process.get(), 120000) == WAIT_OBJECT_0);

    DWORD exit_code = 0;
    REQUIRE(::GetExitCodeProcess(child_process.get(), &exit_code) != FALSE);
    return exit_code;
}

/// Runs a second copy of this test binary filtered to `catch_filter`, with the
/// single-instance guard opted out so the child can actually run.
///
/// @param catch_filter Catch2 test specification, passed as the child's only
///        argument.
/// @param log_path File to capture the child's stdout and stderr into.
/// @return The child's exit code.
inline DWORD run_child_test_binary(std::wstring_view catch_filter,
                                   const std::filesystem::path& log_path) {
    return run_child_test_binary(
        child_test_process_options{.arguments = {std::wstring{catch_filter}},
                                   .allow_concurrent_instances = true},
        log_path);
}

}  // namespace libbsa::tests
