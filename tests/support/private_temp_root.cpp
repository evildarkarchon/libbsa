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
// == Sweeping roots left behind by dead runs ==
//
// Teardown is best effort, so a run that crashed, was killed, or was force
// terminated leaves its root behind. Start-up therefore sweeps the system temp
// root for private roots that no live process owns (issue #63). Sweeping is a
// destructive scan over directories this process does not own -- structurally the
// same operation that caused issue #60 -- so the rule that decides what may be
// deleted is written out here rather than inferred from the code.
//
// **The rule: a root may be removed only when this process can open its owner
// marker with exclusive access.**
//
// Every root's owner creates `.libbsa-tests-owner` inside the root at install
// time and holds that handle open, with `dwShareMode == 0`, until uninstall. A
// second opener of a share-nothing handle is refused with ERROR_SHARING_VIOLATION,
// so an open that *succeeds* proves no process holds the marker.
//
// That is what makes the rule safe against every abnormal exit, and it is why
// the rule is a handle rather than an age. Windows destroys a process's entire
// handle table when the process terminates, whatever terminated it -- a clean
// exit, an unhandled exception, TerminateProcess, or the ASan lane aborting. The
// kernel therefore maintains the liveness bit for us, and it cannot get stuck on:
// there is no path where a dead process still holds a handle, and none where a
// live one has silently lost it. An age threshold has neither property. A long
// ASan lane run can hold a root open for longer than any cutoff worth picking,
// and deleting it would reintroduce exactly the cross-process destruction this
// file exists to remove.
//
// One narrow second branch exists, and it is gated on age *in addition to* the
// rule above, never instead of it: a candidate root carrying **no marker at all**
// is removed once its creation time is older than
// `private_root_unclaimed_grace_ticks`. A live owner is never in that state for
// more than the few microseconds between `create_directory` and the marker's
// `CreateFileW` -- install fails loudly if the marker cannot be created, and
// nothing else ever deletes it -- so a grace period of an hour clears the race by
// some seven orders of magnitude. What the branch is actually for is a root whose
// *contents* were partly deleted: a failed `remove_all` can take the marker and
// then stop on a locked file, and without this branch that root would never be
// collectible again.
//
// That margin argument only holds if both stamps come from the same clock, so the
// branch compares the candidate's creation time against *this process's own
// root's* creation time rather than against `GetSystemTimeAsFileTime()`. The wall
// clock belongs to this process; `ftCreationTime` belongs to whichever filesystem
// hosts the temp root. Point TEMP at a network share whose server clock lags the
// client by more than the grace period and every freshly created root reads as
// aged out, which would turn the microsecond race above into a live deletion. Two
// stamps from one clock have no skew to exploit.
//
// The accepted cost is that a volume recording no creation times at all disables
// the branch entirely, because the reference reads as zero along with the
// candidates. A marker-less root there is never collected. That is deliberate --
// "cannot tell" has to mean "leave it alone" -- and it is narrow: it needs a
// filesystem with no creation timestamps *and* a root that lost its marker, while
// the ordinary crashed-run case still has its marker and is collected by the rule
// above.
//
// The sweep never looks at anything but private roots. `FindFirstFileEx` is given
// the `libbsa-tests-*` pattern, so the filesystem itself does the filtering and
// no arbitrary temp entry is ever handed to the loop, let alone to `remove_all`.
// Reparse points are skipped so a junction planted under the temp root cannot
// redirect a delete outside it.
//
// Failures are counted and skipped, never propagated. A leftover another user
// owns, or one holding a file open, is litter; turning it into a test-run failure
// would make the suite fail on the machine's history rather than on the code.
//
// == The listener also runs the single-instance guard ==
//
// `test_process_startup_listener` below is the process's whole start-up hook, not
// just this file's. It takes the single-instance guard (issue #65,
// `tests/support/single_instance_guard.cpp`) before installing the root, because
// two separately registered listeners would run in static-initialisation order
// across translation units, which is unspecified. The guard is a legibility
// mechanism rather than a safety one -- the private root is what makes concurrency
// safe -- so nothing in this file depends on it.
//
// == Granularity ==
//
// `catch_discover_tests` registers one CTest test per TEST_CASE, so each test
// case runs as its own process and this listener installs and tears down a root
// once per test case rather than once per suite run. Setup is one CreateDirectory,
// one CreateFile, four environment writes, and one sweep; teardown is one
// CloseHandle plus one remove_all. All of it has to stay that cheap at this rate,
// which is why the sweep pushes its name filter into FindFirstFileEx instead of
// enumerating the temp root and filtering afterwards: the usual match count is
// zero, and the usual cost is one directory-search syscall pair.

#include "support/private_temp_root.hpp"

#include "support/single_instance_guard.hpp"

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

/// Creates a root's owner marker and returns the handle that has to stay open for
/// as long as the root is in use.
///
/// The share mode is deliberately zero: refusing every other opener is the whole
/// mechanism, because that refusal is what a sweeping process reads as "an owner
/// is alive". CREATE_NEW rather than CREATE_ALWAYS so adopting a root that somehow
/// already had a marker is impossible.
///
/// FILE_FLAG_DELETE_ON_CLOSE is deliberately *not* used. It would make the marker
/// vanish on a crash, which is precisely when the root has to remain identifiable
/// as a claimed-but-dead one rather than fall back to the age-gated branch.
///
/// @return The open handle, or INVALID_HANDLE_VALUE on failure.
HANDLE acquire_owner_marker(const std::filesystem::path& root) {
    const auto marker = root / std::wstring{private_root_owner_marker_name};
    return ::CreateFileW(marker.native().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                         CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN, nullptr);
}

/// Closes a FindFirstFileEx search handle on scope exit, including the exception
/// path that the sweep's function-try-block would otherwise skip.
class scoped_find_handle {
   public:
    explicit scoped_find_handle(HANDLE handle) noexcept : handle_{handle} {}

    scoped_find_handle(const scoped_find_handle&) = delete;
    scoped_find_handle& operator=(const scoped_find_handle&) = delete;
    scoped_find_handle(scoped_find_handle&&) = delete;
    scoped_find_handle& operator=(scoped_find_handle&&) = delete;

    ~scoped_find_handle() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            ::FindClose(handle_);
        }
    }

    HANDLE get() const noexcept { return handle_; }
    bool valid() const noexcept { return handle_ != INVALID_HANDLE_VALUE; }

   private:
    HANDLE handle_;
};

/// Reads a FILETIME as a single 100ns tick count.
std::uint64_t filetime_ticks(const FILETIME& value) noexcept {
    return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32) |
           static_cast<std::uint64_t>(value.dwLowDateTime);
}

/// Reads a directory's creation time as a tick count, or 0 if it cannot be read.
std::uint64_t directory_creation_ticks(const std::filesystem::path& directory) noexcept {
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (::GetFileAttributesExW(directory.native().c_str(), GetFileExInfoStandard, &attributes) ==
        FALSE) {
        return 0U;
    }
    return filetime_ticks(attributes.ftCreationTime);
}

/// Whether a candidate root with no owner marker is old enough to remove.
///
/// `reference_now` must be a creation stamp read from the *same volume* as the
/// candidate, not `GetSystemTimeAsFileTime()`. The wall clock is the process's
/// own; `ftCreationTime` is whatever the filesystem hosting the temp root wrote.
/// Point TEMP at a network share whose server clock lags the client by more than
/// the grace period -- an ordinary enterprise configuration -- and every freshly
/// created root reads as aged out against the local clock. A sweeper landing in
/// the microseconds between another process's `create_directory` and its
/// `acquire_owner_marker` would then delete a live root, which is the one thing
/// this whole file exists to make impossible. Comparing two stamps from the same
/// clock removes the skew entirely.
///
/// The reference is this process's own root, created moments before the sweep, so
/// it is always slightly behind the true present. That only ever makes the answer
/// more conservative.
///
/// Conservative in the other directions that matter too. A zero creation time --
/// which a filesystem that does not record one reports, and which also disables
/// this branch wholesale by zeroing the reference -- is treated as "cannot tell",
/// not as "infinitely old". A candidate stamped later than the reference, which a
/// root created after this process started legitimately is, is treated the same
/// way. Either answer only ever leaves litter behind; the opposite answer could
/// delete a live root.
bool unclaimed_root_aged_out(const FILETIME& creation_time, std::uint64_t reference_now) noexcept {
    const auto created = filetime_ticks(creation_time);
    if (created == 0U || reference_now == 0U) {
        return false;
    }
    if (reference_now <= created) {
        return false;
    }
    return (reference_now - created) >= private_root_unclaimed_grace_ticks;
}

/// Sweeps private temp roots under `search_root`, skipping `own_root`.
///
/// See the rule at the top of this file. In short: a root is removed only when its
/// owner marker can be opened with exclusive access, which no live owner permits,
/// or when it carries no marker and is older than the grace period.
///
/// @param search_root Directory to scan; nothing outside it is touched.
/// @param own_root This process's own root. Excluded by path -- redundant, since
///        this process holds its own marker and CreateFileW refuses a
///        share-nothing handle even to the process that holds it, but stated
///        anyway because the redundancy is what survives someone later changing
///        how the marker is held. It doubles as the age branch's clock: its
///        creation stamp comes from the same volume as the candidates', which is
///        what keeps a skewed wall clock from ageing out a live root.
stale_root_sweep_report sweep_roots_under(const std::filesystem::path& search_root,
                                          const std::filesystem::path& own_root) noexcept try {
    stale_root_sweep_report report{};
    if (search_root.empty()) {
        return report;
    }

    // Read once, before the scan, so every candidate is judged against the same
    // instant. Zero when it cannot be read, which disables the age branch rather
    // than falling back to the wall clock.
    const auto reference_now = directory_creation_ticks(own_root);

    // The name convention is pushed into the search pattern rather than applied
    // to the results, so an entry that is not a private root never reaches the
    // loop body at all. An 8.3 short name cannot alias into this pattern either:
    // short names are at most eight characters before the extension, and the
    // pattern's literal prefix is thirteen.
    const auto pattern = search_root / (std::wstring{private_root_name_prefix} + L"*");

    WIN32_FIND_DATAW found{};
    // FindExInfoBasic skips the short-name lookup the sweep has no use for.
    const scoped_find_handle search{::FindFirstFileExW(
        pattern.native().c_str(), FindExInfoBasic, &found, FindExSearchNameMatch, nullptr, 0)};
    if (!search.valid()) {
        // No matches, or the search root cannot be read. Both are ordinary.
        return report;
    }

    do {
        const std::wstring_view name{found.cFileName};
        // Belt and braces against the pattern match: the criterion is that only
        // private roots are ever considered, and this is where that is enforced
        // in code the reader can see.
        if (!name.starts_with(private_root_name_prefix)) {
            continue;
        }
        if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0U) {
            continue;
        }
        // A junction or symlink named like a private root would let a delete reach
        // outside the search root. remove_all does not follow one, but refusing to
        // consider it at all is the property worth having.
        if ((found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
            continue;
        }

        const auto candidate = search_root / name;
        if (!own_root.empty() && candidate == own_root) {
            continue;
        }
        ++report.examined;

        const auto marker = candidate / std::wstring{private_root_owner_marker_name};
        const HANDLE marker_handle =
            ::CreateFileW(marker.native().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

        bool removable = false;
        if (marker_handle != INVALID_HANDLE_VALUE) {
            // The open succeeded, so nothing holds the marker and the owner is
            // gone. Closed before the removal: a handle without FILE_SHARE_DELETE
            // blocks deletion of its own file, including by the process holding it.
            ::CloseHandle(marker_handle);
            removable = true;
        } else {
            const auto open_error = ::GetLastError();
            if (open_error == ERROR_FILE_NOT_FOUND || open_error == ERROR_PATH_NOT_FOUND) {
                if (unclaimed_root_aged_out(found.ftCreationTime, reference_now)) {
                    removable = true;
                } else {
                    ++report.unclaimed_recent;
                }
            } else if (open_error == ERROR_SHARING_VIOLATION) {
                ++report.owner_live;
            } else {
                // Permission denied on another user's leftover, most often.
                ++report.skipped;
            }
        }

        if (removable) {
            std::error_code fs_error;
            std::filesystem::remove_all(candidate, fs_error);
            if (fs_error) {
                ++report.remove_failed;
            } else {
                ++report.removed;
            }
        }
    } while (::FindNextFileW(search.get(), &found) != FALSE);

    return report;
} catch (...) {
    // The criterion is that sweep failures are non-fatal, and the only throwing
    // operations left are allocations inside path composition. Report nothing
    // rather than take the run down over litter.
    return stale_root_sweep_report{};
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

        // Acquired before the sweep runs and before any test body can observe the
        // root, so there is no window in which this process owns a root that a
        // concurrent sweeper would read as unclaimed.
        owner_marker_ = acquire_owner_marker(private_root_);
        if (owner_marker_ == INVALID_HANDLE_VALUE) {
            fail(
                "libbsa_tests could not claim its private temp root; without the owner marker "
                "another run's start-up sweep has no way to tell this root from a dead one");
        }

        // Create-then-repoint is the only safe order: the STL verifies the
        // resolved path is a directory and reports not_a_directory otherwise.
        for (const auto* name : temp_variable_names) {
            if (!set_temp_variable(name, private_root_.native())) {
                fail("libbsa_tests could not repoint the process temp environment");
            }
        }

        // Last, and against the *system* root rather than the repointed one:
        // stale roots sit beside this process's root, not under it. Non-fatal by
        // construction -- sweep_roots_under reports failures rather than raising
        // them -- because a leftover this process cannot delete is litter, and
        // failing the run over it would make the suite depend on the machine's
        // history instead of on the code.
        startup_sweep_ = sweep_roots_under(system_root_, private_root_);
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

        // Released before the removal, for two reasons. A handle opened without
        // FILE_SHARE_DELETE blocks deletion of its own file even by the process
        // holding it, so remove_all would fail with the marker still open; and
        // closing it here is what turns this root into a sweepable one if the
        // removal below only partly succeeds.
        //
        // This is the one window in which a still-running process's root is
        // sweepable, and it is stated rather than left to be discovered. It opens
        // after the environment has been restored and after the last test body has
        // run, and it closes when this function returns, so everything another
        // process could delete inside it is state this one has already finished
        // with. Nothing resolves a temp path into the private root any more, and
        // the concurrent remove_all it would race with is removing a directory
        // this process is itself removing.
        if (owner_marker_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(owner_marker_);
            owner_marker_ = INVALID_HANDLE_VALUE;
        }

        if (!private_root_.empty()) {
            // Best-effort: a destructor cannot report, and a leftover directory
            // under the system temp root is litter rather than a correctness
            // problem. A later run's start-up sweep collects whatever survives,
            // which is what keeps best-effort teardown from accumulating.
            std::error_code fs_error;
            std::filesystem::remove_all(private_root_, fs_error);
        }
    }

    const std::filesystem::path& system_root() const noexcept { return system_root_; }
    const std::filesystem::path& private_root() const noexcept { return private_root_; }
    const stale_root_sweep_report& startup_sweep() const noexcept { return startup_sweep_; }

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
            const auto name = std::wstring{private_root_name_prefix} +
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
    /// Held open for the root's lifetime; its openability is what a sweeping
    /// process reads as "this root's owner is dead".
    HANDLE owner_marker_{INVALID_HANDLE_VALUE};
    /// Kept past install so a test can read what the start-up sweep decided.
    /// Deliberately not cleared by uninstall: the counts describe the run.
    stale_root_sweep_report startup_sweep_{};
    std::array<temp_variable_snapshot, temp_variable_names.size()> snapshots_{};
    bool environment_captured_{false};
    bool installed_{false};
};

private_temp_root_state& state() {
    static private_temp_root_state instance;
    return instance;
}

/// Runs the test process's start-up and shutdown work: the single-instance guard
/// first, then the private temp root.
///
/// testRunStarting is the earliest hook that runs after Catch2 has parsed its
/// command line and before the first test case, which is what the acceptance
/// criteria for both pieces ask for. Listing test cases does not go through a test
/// run, so `--list-tests` -- which `catch_discover_tests` invokes at test time
/// under DISCOVERY_MODE PRE_TEST -- neither creates a root nor takes the lock.
///
/// The guard (issue #65) shares this listener rather than registering one of its
/// own, because two listeners would run in static-initialisation order across
/// translation units, which is unspecified. Ordering it first is what keeps a
/// refused instance from creating a temp root and running a sweep on its way to
/// being told it should not have started. `tests/support/single_instance_guard.cpp`
/// has the full reasoning.
class test_process_startup_listener final : public Catch::EventListenerBase {
   public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(const Catch::TestRunInfo& /*run_info*/) override {
        // Throws on refusal, so nothing below runs for a second concurrent
        // instance.
        acquire_single_instance_guard();
        state().install();
    }

    void testRunEnded(const Catch::TestRunStats& /*run_stats*/) override {
        state().uninstall();
        // Released last, so the lock still covers the removal of this process's
        // temp root rather than ending at the last test body.
        release_single_instance_guard();
    }
};

CATCH_REGISTER_LISTENER(test_process_startup_listener)

}  // namespace

const std::filesystem::path& system_temp_root() { return state().system_root(); }

const std::filesystem::path& private_temp_root() { return state().private_root(); }

stale_root_sweep_report sweep_stale_private_temp_roots(
    const std::filesystem::path& search_root) noexcept {
    // This process's own root is passed through so a test that points the sweep at
    // the system root cannot delete the root it is running out of. When the search
    // root is a sandbox, the exclusion simply never matches.
    return sweep_roots_under(search_root, state().private_root());
}

const stale_root_sweep_report& startup_sweep_report() { return state().startup_sweep(); }

}  // namespace libbsa::tests
