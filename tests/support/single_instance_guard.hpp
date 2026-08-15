#pragma once

#include <string>
#include <string_view>

/// The single-instance guard every `libbsa_tests` process runs at start-up.
///
/// A second concurrent instance announces itself and exits rather than running.
/// The guard is *not* a safety mechanism -- issue #62 gave every test process a
/// private temp root, so two instances no longer corrupt each other -- it is a
/// legibility mechanism. The failure it replaces was a moving set of BA2 DX10
/// cleanup failures that each passed on `--rerun-failed`, which reads as a real
/// defect in whatever code was just touched. A named, deterministic refusal reads
/// as what it is. It also catches a stray test process left running from an
/// earlier session, which nothing else does. Issue #65 has the account.
namespace libbsa::tests {

/// Environment variable that disables the guard.
///
/// Set it to any non-empty value in the environment of every instance that may
/// run concurrently. An empty value does *not* opt out, because clearing a
/// variable to the empty string is how several shells "unset" one.
///
/// Shared rather than restated, for the same reason the private temp root's name
/// constants are: a guard that read one spelling and a test that set another would
/// prove nothing about each other.
inline constexpr std::wstring_view single_instance_opt_out_variable =
    L"LIBBSA_TEST_ALLOW_CONCURRENT";

/// Name of the named mutex the guard acquires.
///
/// `Local\` rather than `Global\` deliberately. Creating an object in the global
/// kernel namespace needs `SeCreateGlobalPrivilege`, which a standard user account
/// does not hold, so a `Global\` name would fail outright for exactly the
/// developer this guard is for. The residual gap is two logon sessions of the
/// *same* user -- an RDP session alongside the console, say -- which share a temp
/// root but not a `Local\` namespace, and therefore do not contend. That is
/// accepted: it is rare, and the private temp root already makes it safe.
inline constexpr std::wstring_view single_instance_lock_name =
    LR"(Local\libbsa-tests-single-instance)";

/// What one attempt to take a single-instance lock decided.
enum class single_instance_outcome {
    /// No attempt has been made yet. Only ever observed if the start-up hook did
    /// not run, which is the case for `--list-tests`.
    not_attempted,
    /// Nothing held the lock, and this process now does.
    acquired,
    /// The lock's previous owner died without releasing it, and this process has
    /// taken it over. Windows reports this as `WAIT_ABANDONED`; it is the ordinary
    /// outcome after a crashed or force-terminated run.
    recovered_abandoned,
    /// `single_instance_opt_out_variable` was set, so no lock was taken.
    opted_out,
    /// Another live instance holds the lock.
    refused,
    /// The lock object could not be created or waited on at all. Treated as
    /// permission to continue: the guard buys legibility, not safety, so a machine
    /// where it cannot run is better served by a warning than by a blocked suite.
    unavailable,
};

/// A held single-instance lock, released when it goes out of scope.
///
/// Movable but not copyable: exactly one owner releases the mutex.
class single_instance_lock {
   public:
    single_instance_lock() = default;

    single_instance_lock(const single_instance_lock&) = delete;
    single_instance_lock& operator=(const single_instance_lock&) = delete;
    single_instance_lock(single_instance_lock&& other) noexcept;
    single_instance_lock& operator=(single_instance_lock&& other) noexcept;

    ~single_instance_lock();

    /// What the acquisition attempt that produced this lock decided.
    single_instance_outcome outcome() const noexcept { return outcome_; }

    /// Whether this object owns the mutex and will release it.
    bool held() const noexcept { return handle_ != nullptr; }

    /// Releases the mutex and closes the handle. Idempotent.
    ///
    /// Release must come from the thread that acquired the mutex, because Windows
    /// mutex ownership is per-thread. If it does not, the release fails and the
    /// handle is closed anyway, which *abandons* the mutex -- and an abandoned
    /// mutex is recovered by the next instance rather than blocking it. The failure
    /// mode is therefore benign in both directions, which is why this is `noexcept`
    /// and reports nothing.
    void release() noexcept;

   private:
    /// Wraps an already-acquired native mutex handle.
    ///
    /// Private, and reachable only through the acquisition function below: this
    /// object *releases* the mutex it is handed, so letting arbitrary code hand it
    /// a handle would let arbitrary code call `ReleaseMutex` on a mutex it never
    /// acquired.
    ///
    /// @param handle Mutex handle owned by the calling thread, or nullptr.
    /// @param outcome What the acquisition attempt decided.
    single_instance_lock(void* handle, single_instance_outcome outcome) noexcept;

    friend single_instance_lock try_acquire_single_instance_lock(
        const std::wstring& lock_name) noexcept;

    void* handle_{nullptr};
    single_instance_outcome outcome_{single_instance_outcome::not_attempted};
};

/// Attempts to take the named single-instance lock, without consulting the
/// opt-out.
///
/// Exposed with the name as a parameter so a test can drive the acquisition rule
/// against a probe name of its own instead of against the lock the running process
/// already holds -- the same shape as `sweep_stale_private_temp_roots()` taking a
/// sandbox rather than the system temp root.
///
/// @param lock_name Full mutex name, including any `Local\` prefix.
/// @return A lock that is held when the outcome is `acquired` or
///         `recovered_abandoned`, and empty otherwise.
single_instance_lock try_acquire_single_instance_lock(const std::wstring& lock_name) noexcept;

/// Takes the process-wide guard, or refuses to let the run start.
///
/// Called from the Catch2 start-up listener before anything else, so a refused
/// instance leaves no temp root behind and runs no test body.
///
/// @throws std::runtime_error if another live instance holds the lock. The message
///         is `single_instance_refusal_message()`. Catch2's session catches it out
///         of `testRunStarting`, prints `what()` itself, and exits non-zero, so the
///         guard deliberately does not also write it to stderr -- doing both
///         printed the whole paragraph twice.
void acquire_single_instance_guard();

/// Releases the process-wide guard. Safe to call more than once, and safe to call
/// when the guard was never taken.
void release_single_instance_guard() noexcept;

/// What the process-wide guard decided at start-up.
///
/// `not_attempted` when the start-up hook never ran, which is what `--list-tests`
/// does: listing test cases is not a test run, so Catch2 fires no
/// `testRunStarting` and CTest's discovery invocation never contends.
single_instance_outcome startup_single_instance_outcome() noexcept;

/// The diagnostic a refused instance prints.
///
/// Names the condition -- another instance of this binary is already running --
/// and names `single_instance_opt_out_variable`, so a reader who wants two
/// instances does not have to go looking for how.
std::string single_instance_refusal_message();

/// `single_instance_opt_out_variable` narrowed to ASCII.
///
/// Shared rather than narrowed twice: the refusal message composes the variable's
/// name, and a test searches a refused child's output for it. Both have to start
/// from the same wide constant *and* narrow it the same way, or the test would be
/// looking for a string the guard never printed.
std::string single_instance_opt_out_variable_ascii();

}  // namespace libbsa::tests
