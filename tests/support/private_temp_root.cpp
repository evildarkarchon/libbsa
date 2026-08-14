// Gives every libbsa_tests process a temp root of its own.
//
// == Why this exists ==
//
// The BA2 DX10 snapshot cleanup tests attribute ownership of a
// `libbsa-dx10-snapshot-*` directory by diffing a scan of the temp root taken
// before the writer ran against one taken after. The production name carries no
// owner identity by design -- it is a 128-bit random suffix, and
// "BA2 DX10 writer snapshot directory names are unpredictable" pins that -- so
// the diff is the only attribution available. In a shared temp root that diff is
// wrong the moment another process creates a snapshot directory inside the
// window, and the cleanup helper then calls remove_all on a directory whose owner
// is still using it. Issue #60 has the full account.
//
// Once a process cannot see another process's snapshot directories, the diff
// becomes a correct ownership proxy rather than a lucky one.
//
// == Why the environment, and not a path helper ==
//
// Repointing TMP and TEMP is what makes this cover *production* code.
// `make_unique_snapshot_directory()` resolves its root through
// `std::filesystem::temp_directory_path()` inside the library, at a single call
// site in an anonymous namespace with no option field and nothing linkable from a
// test. Teaching the tests a new path helper would leave that call site pointed
// at the shared root; adding an injectable snapshot root would be production code
// that exists solely for test isolation, which AGENTS.md forbids. Repointing the
// environment adds no seam to the library at all. ADR-0003 records the decision.
//
// == The mechanism, and its constraints ==
//
// Issue #61 pinned all of this against the toolchain in CMakePresets.json; see
// `tests/unit/temp_directory_redirection_tests.cpp` for the proof and the full
// notes. In short:
//
// - MSVC's `temp_directory_path()` reaches `GetTempPath2W` on every call and no
//   layer in that chain holds a static, so resolution is per-call rather than
//   cached and a mid-process repoint takes effect immediately.
// - `GetTempPath2W` reads the *Win32 process environment block*, which
//   `SetEnvironmentVariableW` writes. That is a different store from the CRT's
//   environment copy that `std::getenv` serves, so both are written below and a
//   repointed value must be read back with `GetEnvironmentVariableW`.
// - The root must already exist when the environment is repointed, because the
//   STL verifies the resolved path is a directory.
// - The root must fit in MAX_PATH. An overlong root is reported as
//   `not_a_directory` rather than as a length error, so the length is checked
//   here and fails with a message that names its real cause.
// - Both TMP and TEMP are set, because a stale TEMP would resolve wrongly if TMP
//   were ever cleared.
// - `GetTempPath2W` ignores TMP and TEMP entirely for a process running as
//   SYSTEM. A test binary run as SYSTEM therefore cannot isolate itself this way;
//   issue #60 treats SYSTEM as out of scope, and the assertions in
//   `private_temp_root_tests.cpp` fail loudly there rather than degrading.
//
// == Granularity ==
//
// `catch_discover_tests` registers one CTest test per TEST_CASE, so each test
// case runs as its own process and this listener installs and tears down a root
// once per test case rather than once per suite run. Setup is one CreateDirectory
// plus four environment writes, and teardown is one remove_all; both have to stay
// that cheap at this rate.

#include "support/private_temp_root.hpp"

#include <catch2/interfaces/catch_interfaces_reporter.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace libbsa::tests {
namespace {

// The environment-reading and environment-restoring helpers below deliberately
// duplicate their counterparts in tests/unit/temp_directory_redirection_tests.cpp
// rather than sharing a header with them. That file is the *specification* of the
// mechanism -- it exists to prove that SetEnvironmentVariableW is what
// temp_directory_path() follows and that the Win32 and CRT stores diverge -- and
// this file is a consumer of that mechanism. Reading both through one
// implementation would mean a bug in the shared code makes the pin and its user
// wrong together, and the pin would no longer catch anything. The two also cannot
// share as written: the test-side versions use Catch2 REQUIRE, which needs an
// active assertion context that a reporter callback does not have.

/// The variables GetTempPath2W consults before falling back to USERPROFILE.
constexpr std::array<const wchar_t*, 2> temp_variable_names{L"TMP", L"TEMP"};

/// Distinctive enough to be greppable and to let a later sweep of stale roots
/// (issue #63) recognise one, but kept short: everything the suite creates nests
/// under the root, and the deepest existing test paths already run to roughly
/// 180 characters against a MAX_PATH ceiling.
constexpr std::wstring_view private_root_prefix = L"libbsa-tests-";

/// Attempts before giving up on finding an unused root name. Directory creation
/// is the atomic reservation, so this only bounds the cost of collisions.
constexpr int max_root_creation_attempts = 64;

/// Reads a variable from the Win32 process environment block, the store
/// GetTempPath2W consults. Returns nullopt only when the variable is unset; a
/// variable set to an empty string reads back as an empty string, so
/// capture-and-restore cannot silently delete one.
std::optional<std::wstring> win32_environment_value(const wchar_t* name) {
    // Two-call sizing: with a zero-sized buffer GetEnvironmentVariableW returns
    // the required length including the terminator, or 0 when the variable does
    // not exist.
    const auto required = ::GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) {
        return std::nullopt;
    }

    std::wstring value(required, L'\0');
    const auto written = ::GetEnvironmentVariableW(name, value.data(), required);
    if (written >= required) {
        // A buffer-too-small result means the value grew between the two calls.
        // Nothing in this process should be racing us on TMP/TEMP, so report unset
        // rather than invent a value; restoration then deletes the variable, which
        // is the safe direction. A zero return is not treated as failure here: it
        // is also how a variable set to an empty string reads back, and the first
        // call already established that the variable exists.
        return std::nullopt;
    }
    value.resize(written);
    return value;
}

/// Reads a variable from the CRT's own environment copy, the store std::getenv
/// serves. Kept separate from win32_environment_value because the two stores
/// diverge: SetEnvironmentVariableW does not write through to this one.
std::optional<std::wstring> crt_environment_value(const wchar_t* name) {
    // _wgetenv_s rather than _wgetenv: the latter is deprecated under the secure
    // CRT and warns at the project's default warning level.
    std::size_t required = 0;
    if (::_wgetenv_s(&required, nullptr, 0, name) != 0 || required == 0) {
        return std::nullopt;
    }

    std::wstring value(required, L'\0');
    if (::_wgetenv_s(&required, value.data(), value.size(), name) != 0) {
        return std::nullopt;
    }
    // The reported size counts the terminator, which std::wstring stores itself.
    value.resize(required - 1);
    return value;
}

/// Both stores' values for one temp variable, captured together so restoration
/// cannot put them back out of step.
struct temp_variable_snapshot {
    std::optional<std::wstring> win32_value;
    std::optional<std::wstring> crt_value;
};

/// Points one temp variable at `value` in both environment stores.
///
/// Writing the Win32 block alone would be enough for temp_directory_path(), and
/// issue #61 records SetEnvironmentVariableW as the primitive to prefer for
/// exactly that reason. The CRT store is written as well so that the *process's*
/// idea of its temp directory is consistent whichever store is asked. Nothing
/// reads TMP or TEMP through std::getenv today, but several tests do reach for
/// std::getenv for other variables, and a future one that reached for TMP would
/// otherwise get the shared system root back with no indication that it had
/// stepped outside the isolation.
///
/// The CRT store is written first because _wputenv_s also writes through to the
/// Win32 block; writing the Win32 block afterwards is what leaves both stores
/// holding the intended value regardless of that write-through.
bool set_temp_variable(const wchar_t* name, const std::wstring& value) {
    (void)::_wputenv_s(name, value.c_str());
    return ::SetEnvironmentVariableW(name, value.c_str()) != FALSE;
}

/// Puts one temp variable back the way it was found, in both stores.
///
/// A null pointer (Win32) or an empty string (CRT) is how each API deletes a
/// variable, which is how an originally-unset TMP or TEMP is restored rather than
/// left behind as an empty string that GetTempPath2W would treat as a valid --
/// and wrong -- answer.
void restore_temp_variable(const wchar_t* name, const temp_variable_snapshot& snapshot) noexcept {
    (void)::_wputenv_s(name, snapshot.crt_value ? snapshot.crt_value->c_str() : L"");
    (void)::SetEnvironmentVariableW(name,
                                    snapshot.win32_value ? snapshot.win32_value->c_str() : nullptr);
}

/// Formats `value` as exactly four lowercase hex digits.
std::wstring hex4(std::uint32_t value) {
    constexpr wchar_t hex_digits[] = L"0123456789abcdef";
    std::wstring text(4, L'0');
    for (std::size_t index = 0; index < text.size(); ++index) {
        text[text.size() - 1U - index] = hex_digits[(value >> (index * 4U)) & 0x0FU];
    }
    return text;
}

/// Owns the process's private temp root and the environment repoint that points
/// at it.
///
/// A single instance lives in a function-local static, so its destructor is the
/// backstop that removes the root if the listener's testRunEnded hook never fires
/// -- Catch2 can leave the run early, and the acceptance criterion is that the
/// root goes away when the *process* exits, not when the run ends cleanly.
/// Teardown is idempotent because both paths call it.
class private_temp_root_state {
   public:
    private_temp_root_state() = default;

    private_temp_root_state(const private_temp_root_state&) = delete;
    private_temp_root_state& operator=(const private_temp_root_state&) = delete;
    private_temp_root_state(private_temp_root_state&&) = delete;
    private_temp_root_state& operator=(private_temp_root_state&&) = delete;

    ~private_temp_root_state() { uninstall(); }

    /// Creates the private root and repoints the process at it.
    ///
    /// @throws std::runtime_error if the system temp root cannot be resolved, no
    ///         root name can be reserved, the root would exceed MAX_PATH, or the
    ///         environment cannot be written. Throwing rather than degrading is
    ///         deliberate: a silently shared temp root reintroduces exactly the
    ///         cross-process corruption this exists to prevent, and Catch2's
    ///         session wraps the run so the message reaches stderr and the
    ///         process exits non-zero.
    void install() {
        if (installed_) {
            return;
        }

        std::error_code fs_error;
        const auto resolved = std::filesystem::temp_directory_path(fs_error);
        if (fs_error) {
            fail("libbsa_tests could not resolve the system temp root: " + fs_error.message());
        }

        // canonical() rather than the raw result: GetTempPath2W always appends a
        // trailing backslash, so the raw string is never lexically equal to the
        // directory it names.
        system_root_ = std::filesystem::canonical(resolved, fs_error);
        if (fs_error || system_root_.empty()) {
            fail("libbsa_tests could not canonicalise the system temp root: " + fs_error.message());
        }

        // Captured before anything is written so a partially applied repoint still
        // unwinds to the original values.
        for (std::size_t index = 0; index < temp_variable_names.size(); ++index) {
            const auto* name = temp_variable_names[index];
            snapshots_[index] =
                temp_variable_snapshot{win32_environment_value(name), crt_environment_value(name)};
        }
        environment_captured_ = true;
        installed_ = true;

        private_root_ = create_private_root();

        // Create-then-repoint is the only safe order: the STL verifies the
        // resolved path is a directory and reports not_a_directory otherwise.
        for (const auto* name : temp_variable_names) {
            if (!set_temp_variable(name, private_root_.native())) {
                fail("libbsa_tests could not repoint the process temp environment");
            }
        }
    }

    /// Restores the temp environment and removes the private root. Safe to call
    /// more than once and safe to call after a failed install.
    void uninstall() noexcept {
        if (!installed_) {
            return;
        }
        installed_ = false;

        if (environment_captured_) {
            // Restored before the removal so that anything resolving a temp path
            // during process shutdown lands in the system root rather than in a
            // directory that is about to stop existing.
            for (std::size_t index = 0; index < temp_variable_names.size(); ++index) {
                restore_temp_variable(temp_variable_names[index], snapshots_[index]);
            }
            environment_captured_ = false;
        }

        if (!private_root_.empty()) {
            // Best-effort: a destructor cannot report, and a leftover directory
            // under the system temp root is litter rather than a correctness
            // problem. Issue #63 sweeps stale roots at start-up.
            std::error_code fs_error;
            std::filesystem::remove_all(private_root_, fs_error);
        }
    }

    const std::filesystem::path& system_root() const noexcept { return system_root_; }
    const std::filesystem::path& private_root() const noexcept { return private_root_; }

   private:
    [[noreturn]] static void fail(const std::string& message) {
        // Written to stderr as well as thrown: the throw is what stops the run,
        // but the stderr line survives whatever a reporter does with the
        // exception, and CTest captures it on failure.
        std::cerr << message << '\n' << std::flush;
        throw std::runtime_error{message};
    }

    /// Reserves a uniquely named directory under the system temp root.
    ///
    /// The process id alone is not enough -- a crashed run can leave a directory
    /// behind and the id is eventually reused -- so a random component is added
    /// and directory creation itself is the atomic reservation, exactly as the
    /// production snapshot builder does it.
    std::filesystem::path create_private_root() {
        std::random_device entropy;
        for (int attempt = 0; attempt < max_root_creation_attempts; ++attempt) {
            const auto name = std::wstring{private_root_prefix} +
                              std::to_wstring(::GetCurrentProcessId()) + L"-" +
                              hex4(entropy() & 0xFFFFU);
            auto candidate = system_root_ / name;

            // An overlong root is reported by the STL as not_a_directory rather
            // than as a length error, so anyone who hit it would diagnose the
            // wrong thing. Name the real cause here instead.
            if (candidate.native().size() >= MAX_PATH) {
                fail(
                    "libbsa_tests private temp root would exceed MAX_PATH; the system temp "
                    "root is too deep for the test suite to isolate itself under it");
            }

            std::error_code fs_error;
            if (std::filesystem::create_directory(candidate, fs_error)) {
                return candidate;
            }
            if (fs_error) {
                fail("libbsa_tests could not create a private temp root: " + fs_error.message());
            }
            // No error and no creation means the name was taken; try another.
        }

        fail("libbsa_tests exhausted private temp root names");
    }

    std::filesystem::path system_root_;
    std::filesystem::path private_root_;
    std::array<temp_variable_snapshot, temp_variable_names.size()> snapshots_{};
    bool environment_captured_{false};
    bool installed_{false};
};

private_temp_root_state& state() {
    static private_temp_root_state instance;
    return instance;
}

/// Installs the private temp root before any test body runs and tears it down
/// when the run ends.
///
/// testRunStarting is the earliest hook that runs after Catch2 has parsed its
/// command line and before the first test case, which is what the acceptance
/// criterion asks for. Listing test cases does not go through a test run, so
/// `--list-tests` -- which `catch_discover_tests` invokes at test time under
/// DISCOVERY_MODE PRE_TEST -- creates no root.
class private_temp_root_listener final : public Catch::EventListenerBase {
   public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(const Catch::TestRunInfo& /*run_info*/) override { state().install(); }

    void testRunEnded(const Catch::TestRunStats& /*run_stats*/) override { state().uninstall(); }
};

CATCH_REGISTER_LISTENER(private_temp_root_listener)

}  // namespace

const std::filesystem::path& system_temp_root() { return state().system_root(); }

const std::filesystem::path& private_temp_root() { return state().private_root(); }

}  // namespace libbsa::tests
