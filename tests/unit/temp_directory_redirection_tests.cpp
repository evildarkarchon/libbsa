// Pins how std::filesystem::temp_directory_path() resolves TMP/TEMP at runtime.
//
// The per-process temp root design (issue #60) rests on one assumption: a
// running test process can repoint its own temp directory and have both test
// code and libbsa itself observe the new location immediately. These tests
// prove that assumption rather than inheriting it, and the mechanism they pin
// is what every later ticket in that spec builds on.
//
// == The mechanism ==
//
// MSVC's std::filesystem::temp_directory_path() calls __std_fs_get_temp_path,
// which calls GetTempPath2W (GetTempPathW before Win11) on every invocation and
// then GetFileAttributesW on the result. No layer in that chain holds a static,
// so the value is re-read per call rather than cached.
//
// GetTempPath2W reads the *Win32 process environment block*, checking TMP, then
// TEMP, then USERPROFILE, then the Windows directory. That block is written by
// SetEnvironmentVariableW. It is a different store from the CRT's environment
// copy that std::getenv/_wgetenv_s read, so the tests below assert on both and
// record the direction of synchronisation rather than assuming it:
// SetEnvironmentVariableW alone is sufficient and leaves the CRT copy stale,
// while _wputenv_s writes through to the Win32 block and therefore also works.
// SetEnvironmentVariableW is the primitive to prefer, because it is the store
// GetTempPath2W actually reads.
//
// == Where the mechanism does not work ==
//
// GetTempPath2W differs from GetTempPathW in exactly one respect, and it is the
// respect that matters here: for a process running as SYSTEM it ignores TMP and
// TEMP entirely and returns %SystemRoot%\SystemTemp. A libbsa process running
// as SYSTEM on Windows 11 or later therefore cannot repoint its own temp root
// this way. That is not a silent failure for these tests -- the assertions
// below fail loudly under SYSTEM -- but any later ticket that relies on the
// private-root design must treat SYSTEM as out of scope.
//
// == Constraints this imposes on a private temp root ==
//
// - The root must already exist when the environment is repointed. The STL
//   verifies the resolved path is a directory and reports not_a_directory
//   otherwise, so create-then-repoint is the only safe order.
// - The root must fit in MAX_PATH. The STL passes a fixed __std_fs_max_path + 1
//   (261) wide-character buffer to GetTempPath2W, and an overlong root is
//   reported as errc::not_a_directory rather than as a length error: the STL
//   zero-fills that buffer, GetTempPath2W leaves it unwritten when it overflows,
//   and the subsequent GetFileAttributesW then fails on an empty path. A later
//   ticket that hits this will see a misleading error, so bound the root length
//   up front.
// - The returned path carries a trailing backslash, because GetTempPath2W
//   always appends one. It is therefore never lexically equal to the directory
//   it names; comparisons must be canonicalised.
// - Both TMP and TEMP must be set. TMP wins when both are present, but leaving
//   a stale TEMP behind would resolve wrongly if TMP were ever cleared.

#include <catch2/catch_test_macros.hpp>

#include "formats/ba2/ba2_dx10_snapshot_builder.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>

namespace {

/// Names of the two variables GetTempPath2W consults before falling back to
/// USERPROFILE. Both are repointed together so a cleared TMP cannot expose a
/// stale TEMP.
constexpr std::array<const wchar_t*, 2> temp_variable_names{L"TMP", L"TEMP"};

/// Reads a variable from the Win32 process environment block, which is the
/// store GetTempPath2W consults. Returns nullopt only when the variable is
/// unset; a variable set to an empty string reads back as an empty string, so
/// capture-and-restore does not silently delete one.
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
    // A short write means the value grew between the two calls. Reporting nullopt
    // would be a lie that later restores an unset variable, so fail loudly.
    REQUIRE(written < required);
    value.resize(written);
    return value;
}

/// Reads a variable from the CRT's own environment copy, the store std::getenv
/// and _wgetenv_s serve. Deliberately separate from win32_environment_value so
/// the tests can show the two stores diverging.
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
    const wchar_t* name;
    std::optional<std::wstring> win32_value;
    std::optional<std::wstring> crt_value;
};

/// Captures TMP and TEMP on construction and restores both, in both stores, on
/// destruction, so a repointing test cannot leak its private root into the rest
/// of the suite. The BA2 DX10 snapshot tests scan the real system temp root, so
/// a leaked repoint would silently change what they see.
class scoped_temp_environment {
   public:
    scoped_temp_environment() {
        for (std::size_t index = 0; index < temp_variable_names.size(); ++index) {
            const auto* name = temp_variable_names[index];
            snapshots_[index] = temp_variable_snapshot{name, win32_environment_value(name),
                                                       crt_environment_value(name)};
        }
    }

    scoped_temp_environment(const scoped_temp_environment&) = delete;
    scoped_temp_environment& operator=(const scoped_temp_environment&) = delete;
    scoped_temp_environment(scoped_temp_environment&&) = delete;
    scoped_temp_environment& operator=(scoped_temp_environment&&) = delete;

    ~scoped_temp_environment() {
        // Restoration cannot use Catch2 assertions: they throw, destructors are
        // implicitly noexcept, and a throw from one terminates the process.
        // Failures to restore surface through the explicit post-restore
        // assertions the test bodies make instead.
        for (const auto& snapshot : snapshots_) {
            // The CRT store goes first because _wputenv_s also writes through to
            // the Win32 block; restoring the Win32 block afterwards is what leaves
            // both stores holding their original values.
            restore_crt(snapshot);
            restore_win32(snapshot);
        }
    }

   private:
    static void restore_win32(const temp_variable_snapshot& snapshot) {
        // A null value deletes the variable, which is how an originally-unset TMP
        // or TEMP is restored rather than left behind as an empty string that
        // GetTempPath2W would treat as a valid (and wrong) answer.
        ::SetEnvironmentVariableW(snapshot.name,
                                  snapshot.win32_value ? snapshot.win32_value->c_str() : nullptr);
    }

    static void restore_crt(const temp_variable_snapshot& snapshot) {
        // An empty value is how _wputenv_s removes a variable, mirroring the null
        // pointer SetEnvironmentVariableW takes for the same purpose.
        (void)::_wputenv_s(snapshot.name, snapshot.crt_value ? snapshot.crt_value->c_str() : L"");
    }

    std::array<temp_variable_snapshot, temp_variable_names.size()> snapshots_{};
};

/// Creates a private directory under `parent` and removes it on destruction.
/// RAII rather than a trailing cleanup call because a failing REQUIRE throws out
/// of the test body, and a leaked directory under the real system temp root is
/// exactly the accumulation issue #60 sets out to stop.
class scoped_private_temp_root {
   public:
    explicit scoped_private_temp_root(const std::filesystem::path& parent) : root_{make(parent)} {}

    scoped_private_temp_root(const scoped_private_temp_root&) = delete;
    scoped_private_temp_root& operator=(const scoped_private_temp_root&) = delete;
    scoped_private_temp_root(scoped_private_temp_root&&) = delete;
    scoped_private_temp_root& operator=(scoped_private_temp_root&&) = delete;

    ~scoped_private_temp_root() {
        // Removal failures are swallowed deliberately: destructors cannot throw
        // Catch2 assertions, and a leftover under the system temp root is litter
        // for the OS to reclaim rather than evidence about the mechanism.
        std::error_code error;
        std::filesystem::remove_all(root_, error);
    }

    const std::filesystem::path& path() const noexcept { return root_; }

   private:
    /// Builds a root name unique across concurrent test processes. A per-process
    /// counter alone would collide between two simultaneous runs, which is the
    /// exact hazard issue #60 exists to remove.
    static std::filesystem::path make(const std::filesystem::path& parent) {
        static std::atomic_uint32_t counter{0};
        const auto name = L"libbsa-temp-redirect-" + std::to_wstring(::GetCurrentProcessId()) +
                          L"-" + std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed));
        auto root = parent / name;

        // An overlong root fails as not_a_directory, which would look like a
        // broken mechanism rather than a too-long path. Bound it here so the
        // failure names its real cause.
        REQUIRE(root.native().size() < MAX_PATH);

        std::error_code error;
        std::filesystem::create_directories(root, error);
        REQUIRE_FALSE(error);
        return root;
    }

    std::filesystem::path root_;
};

/// Points the Win32 process environment block's TMP and TEMP at `root`. Uses
/// only SetEnvironmentVariableW, so a passing assertion afterwards proves the
/// Win32 block alone is sufficient.
void repoint_win32_temp_environment(const std::filesystem::path& root) {
    const auto native = root.native();
    for (const auto* name : temp_variable_names) {
        REQUIRE(::SetEnvironmentVariableW(name, native.c_str()) != FALSE);
    }
}

/// Compares a temp_directory_path() result against the directory it should name.
/// GetTempPath2W always appends a trailing backslash, so the raw paths are never
/// lexically equal even when they identify the same directory.
///
/// Canonicalisation failure is a hard test failure rather than a `false` return:
/// returning `false` would let a negative assertion such as CHECK_FALSE pass for
/// the wrong reason.
bool names_same_directory(const std::filesystem::path& resolved,
                          const std::filesystem::path& expected) {
    std::error_code error;
    const auto canonical_resolved = std::filesystem::canonical(resolved, error);
    REQUIRE_FALSE(error);
    const auto canonical_expected = std::filesystem::canonical(expected, error);
    REQUIRE_FALSE(error);
    return canonical_resolved == canonical_expected;
}

}  // namespace

TEST_CASE("temp_directory_path resolves a temp root repointed after its first call",
          "[unit][temp_directory]") {
    scoped_temp_environment environment_guard;

    // Resolving once up front is the point of the test: a cached first
    // resolution would make every later assertion here fail rather than pass by
    // accident.
    const auto original_root = std::filesystem::temp_directory_path();
    REQUIRE(std::filesystem::is_directory(original_root));

    const scoped_private_temp_root private_root{original_root};
    repoint_win32_temp_environment(private_root.path());

    const auto redirected_root = std::filesystem::temp_directory_path();
    CHECK(names_same_directory(redirected_root, private_root.path()));
    CHECK_FALSE(names_same_directory(redirected_root, original_root));

    // Pinned because later tickets compare temp paths: GetTempPath2W appends a
    // trailing backslash and the STL passes it straight through, so the result
    // is not lexically equal to the root that was installed.
    CHECK(redirected_root.native().back() == L'\\');
    CHECK(redirected_root != private_root.path());
}

TEST_CASE("temp_directory_path re-reads the environment on every call", "[unit][temp_directory]") {
    scoped_temp_environment environment_guard;

    const auto original_root = std::filesystem::temp_directory_path();
    const scoped_private_temp_root first_root{original_root};
    const scoped_private_temp_root second_root{original_root};

    repoint_win32_temp_environment(first_root.path());
    CHECK(names_same_directory(std::filesystem::temp_directory_path(), first_root.path()));

    // A single successful repoint would also be explained by a one-shot cache
    // that happened to be populated late. Repointing again, and then back, is
    // what distinguishes per-call resolution from a cache.
    repoint_win32_temp_environment(second_root.path());
    CHECK(names_same_directory(std::filesystem::temp_directory_path(), second_root.path()));

    repoint_win32_temp_environment(first_root.path());
    CHECK(names_same_directory(std::filesystem::temp_directory_path(), first_root.path()));
}

TEST_CASE("temp_directory_path reads the Win32 environment block, not the CRT copy",
          "[unit][temp_directory]") {
    scoped_temp_environment environment_guard;

    const auto original_root = std::filesystem::temp_directory_path();

    // Materialise the CRT environment copy before repointing. The UCRT builds it
    // lazily from the Win32 block, so reading it afterwards for the first time
    // could pick up the new value and hide the divergence being asserted.
    const auto crt_tmp_before = crt_environment_value(L"TMP");

    const scoped_private_temp_root private_root{original_root};
    repoint_win32_temp_environment(private_root.path());

    // This is the load-bearing assertion of the case: setting only the Win32
    // block is enough for temp_directory_path(). The two CRT assertions below
    // are negative-form and would also hold if SetEnvironmentVariableW had done
    // nothing at all, so they qualify this result rather than establish it.
    CHECK(names_same_directory(std::filesystem::temp_directory_path(), private_root.path()));

    // The CRT copy is untouched by SetEnvironmentVariableW. Code that needs to
    // read the repointed value back must go through GetEnvironmentVariableW, not
    // std::getenv. The store-vs-store comparison states the divergence without
    // depending on whether TMP was set to begin with: the Win32 side definitely
    // holds the private root by this point, so an equal CRT side would mean the
    // two stores are one.
    CHECK(crt_environment_value(L"TMP") == crt_tmp_before);
    CHECK(win32_environment_value(L"TMP") != crt_environment_value(L"TMP"));
}

TEST_CASE("temp_directory_path follows a repoint made through the CRT environment",
          "[unit][temp_directory]") {
    scoped_temp_environment environment_guard;

    const auto original_root = std::filesystem::temp_directory_path();
    const scoped_private_temp_root private_root{original_root};

    // Recorded because #61 asks which API to set: the CRT route also works, but
    // only because the UCRT's _wputenv_s writes through to the Win32 block. That
    // write-through is what the second assertion pins, and it is why
    // SetEnvironmentVariableW -- the store GetTempPath2W actually reads -- stays
    // the primitive to prefer.
    const auto native = private_root.path().native();
    for (const auto* name : temp_variable_names) {
        REQUIRE(::_wputenv_s(name, native.c_str()) == 0);
    }

    CHECK(names_same_directory(std::filesystem::temp_directory_path(), private_root.path()));
    CHECK(win32_environment_value(L"TMP") == native);
}

TEST_CASE("libbsa library code observes a repointed temp root", "[unit][temp_directory]") {
    scoped_temp_environment environment_guard;

    const auto original_root = std::filesystem::temp_directory_path();
    const scoped_private_temp_root private_root{original_root};
    repoint_win32_temp_environment(private_root.path());

    // The BA2 DX10 writer resolves its snapshot root through
    // std::filesystem::temp_directory_path() inside library code, so this proves
    // the redirection reaches production paths and not just the test's own
    // calls. That is what lets issue #60 isolate libbsa's temp state by
    // repointing the environment instead of threading a path helper through the
    // library.
    std::filesystem::path snapshot_dir;
    const auto ensured = libbsa::formats::ba2::ba2_dx10_ensure_snapshot_directory(snapshot_dir);
    REQUIRE(ensured.has_value());
    REQUIRE_FALSE(snapshot_dir.empty());
    CHECK(names_same_directory(snapshot_dir.parent_path(), private_root.path()));
}

TEST_CASE("repointing the temp environment is reverted for later tests", "[unit][temp_directory]") {
    const auto original_root = std::filesystem::temp_directory_path();

    // Covers the acceptance criterion that the test "restores the original
    // environment afterwards so it cannot perturb other test cases". Without it,
    // a guard that silently stopped restoring would corrupt every later test
    // case in the binary instead of failing here.
    {
        scoped_temp_environment environment_guard;
        const scoped_private_temp_root private_root{original_root};
        repoint_win32_temp_environment(private_root.path());
        REQUIRE(names_same_directory(std::filesystem::temp_directory_path(), private_root.path()));
    }

    CHECK(names_same_directory(std::filesystem::temp_directory_path(), original_root));
}
