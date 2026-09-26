// Refuses to start a second concurrent libbsa_tests process.
//
// == Why this exists, given that concurrency is already safe ==
//
// Issue #62 gave every test process a private temp root, so two instances no
// longer destroy each other's BA2 DX10 snapshot directories. This guard is not
// what makes that safe, and removing it would not make the suite unsafe.
//
// What it buys is legibility. The failure it replaces is a *moving* set of DX10
// cleanup failures -- a different pair each run, every one of them passing on
// `--rerun-failed` -- which is indistinguishable from a real regression in
// whatever code was just touched. Issue #60 records a full afternoon spent
// diagnosing exactly that. A named, deterministic refusal at start-up reads as
// what it is.
//
// It also catches something isolation does not: a stray test process left running
// from an earlier session, which otherwise sits there holding a temp root and
// contributing nothing but confusion.
//
// == Why a named mutex, and not a lock file ==
//
// The acceptance criterion that decides this is "a lock left behind by a crashed
// process is recovered rather than blocking every later run forever". A kernel
// object gets that for free, twice over:
//
// - When the last handle to a named mutex closes, the object is destroyed. A
//   process that was killed therefore leaves *nothing* behind, and the next
//   instance creates the object fresh.
// - When a process holding a mutex dies without releasing it while some other
//   handle keeps the object alive, the next waiter is told so with
//   `WAIT_ABANDONED` and is given ownership anyway.
//
// Windows destroys a process's entire handle table when the process terminates,
// whatever terminated it, so neither path can get stuck. A lock *file* would need
// the exclusive-open dance the private temp root's owner marker already uses, plus
// its own age-based escape hatch for the case where the file outlives its owner --
// which is the machinery issue #63 had to justify at length. There is no reason to
// repeat it here when the kernel already maintains the bit.
//
// Ownership is taken with a zero-timeout wait rather than with
// `CreateMutexW(..., bInitialOwner = TRUE)`, because `ERROR_ALREADY_EXISTS` only
// says the *object* existed, while the wait distinguishes held-by-a-live-owner
// from abandoned-by-a-dead-one. The guard reports which, and a test asserts on it.
//
// == Ordering, and why this is not its own listener ==
//
// The guard runs from the same `testRunStarting` hook that installs the private
// temp root (`tests/support/private_temp_root.cpp`), first, before anything else
// happens. Registering a second `CATCH_REGISTER_LISTENER` would leave the relative
// order to static-initialisation order across translation units, which is
// unspecified; a refused instance would then sometimes create and sweep a temp
// root before being told it should not have started. Both orders are *correct* --
// the root's teardown backstop cleans up either way -- but only one of them is
// predictable, and a start-up sequence is a bad place to accept a coin flip.
//
// == Granularity ==
//
// `catch_discover_tests` registers one CTest test per `TEST_CASE`, so each case is
// its own process and the lock is held for the duration of a single case rather
// than a whole suite run. A serial `ctest` therefore never contends -- CTest starts
// the next process only after the previous one exits -- while two overlapping
// `ctest` runs contend within the first case or two. Cost is one `CreateMutexW`
// plus one already-signalled wait per test case.
//
// Listing test cases is not a test run, so Catch2 fires no `testRunStarting` for
// `--list-tests` and CTest's discovery invocation never takes the lock. That is
// the same property the private temp root relies on, and it is asserted directly
// by `tests/unit/single_instance_guard_tests.cpp`.

#include "support/single_instance_guard.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace libbsa::tests {
namespace {

/// Whether the opt-out variable is set to a non-empty value.
///
/// Read from the Win32 process environment block rather than through
/// `std::getenv`, because that is the store a spawned child inherits and the store
/// `SetEnvironmentVariableW` writes. `_wputenv_s` writes through to it as well, so
/// a value set either way is seen here; a value set *only* in the CRT copy is not
/// reachable by any API this repository uses.
/// Deliberately not noexcept: composing the name allocates, and terminating on a
/// failed allocation would be a worse answer than letting it propagate to Catch2's
/// session, which reports and exits non-zero. Its only caller is not noexcept
/// either.
bool opt_out_requested() {
    const std::wstring name{single_instance_opt_out_variable};
    // A zero return means unset *or* set to the empty string, and both are
    // "not opted out", so the two do not need distinguishing.
    return ::GetEnvironmentVariableW(name.c_str(), nullptr, 0) > 1;
}

/// Narrows an ASCII wide string for use in the diagnostic. The only wide strings
/// this file formats are its own constants, which are ASCII by construction.
std::string narrow_ascii(std::wstring_view text) {
    std::string narrowed;
    narrowed.reserve(text.size());
    for (const wchar_t character : text) {
        narrowed.push_back(character < 128 ? static_cast<char>(character) : '?');
    }
    return narrowed;
}

/// The process-wide guard's lock, and what taking it decided.
///
/// A function-local static so its destructor is the backstop for a run that leaves
/// early without Catch2's `testRunEnded` firing -- the same shape, and for the same
/// reason, as the private temp root's state object. Releasing twice is harmless.
single_instance_lock& process_lock() {
    static single_instance_lock lock;
    return lock;
}

/// What start-up decided, kept separately from the lock so it survives release.
///
/// `process_lock().outcome()` would read `not_attempted` again once the lock is
/// released at the end of the run, and a test that asked afterwards would get an
/// answer about the present rather than about start-up.
single_instance_outcome& startup_outcome() {
    static single_instance_outcome outcome = single_instance_outcome::not_attempted;
    return outcome;
}

}  // namespace

single_instance_lock::single_instance_lock(void* handle, single_instance_outcome outcome) noexcept
    : handle_{handle}, outcome_{outcome} {}

single_instance_lock::single_instance_lock(single_instance_lock&& other) noexcept
    : handle_{std::exchange(other.handle_, nullptr)},
      outcome_{std::exchange(other.outcome_, single_instance_outcome::not_attempted)} {}

single_instance_lock& single_instance_lock::operator=(single_instance_lock&& other) noexcept {
    if (this != &other) {
        release();
        handle_ = std::exchange(other.handle_, nullptr);
        outcome_ = std::exchange(other.outcome_, single_instance_outcome::not_attempted);
    }
    return *this;
}

single_instance_lock::~single_instance_lock() { release(); }

void single_instance_lock::release() noexcept {
    if (handle_ == nullptr) {
        return;
    }
    // Release before close. Closing alone would abandon the mutex, which the next
    // instance recovers from -- but it would also mean every clean run looked like
    // a crashed one, and the abandoned outcome would stop carrying information.
    (void)::ReleaseMutex(handle_);
    (void)::CloseHandle(handle_);
    handle_ = nullptr;
    // outcome_ is deliberately left alone: it describes the acquisition that
    // happened, and that stays true after the lock is given up.
}

single_instance_lock try_acquire_single_instance_lock(const std::wstring& lock_name) noexcept {
    // bInitialOwner is FALSE so that ownership is decided by the wait below rather
    // than by creation. Creation only ever tells us whether the object existed,
    // which does not separate a live owner from a dead one.
    const HANDLE handle = ::CreateMutexW(nullptr, FALSE, lock_name.c_str());
    if (handle == nullptr) {
        return single_instance_lock{nullptr, single_instance_outcome::unavailable};
    }

    switch (::WaitForSingleObject(handle, 0)) {
        case WAIT_OBJECT_0:
            return single_instance_lock{handle, single_instance_outcome::acquired};
        case WAIT_ABANDONED:
            // The previous owner died without releasing. This thread owns the mutex
            // now and must still release it, so the handle is kept exactly as in
            // the clean case.
            return single_instance_lock{handle, single_instance_outcome::recovered_abandoned};
        case WAIT_TIMEOUT:
            // Held by a live owner. Our handle is closed rather than kept: it grants
            // nothing, and holding it would keep the object alive past the point
            // where the owner exits, turning a killed owner's clean disappearance
            // into an abandoned lock for no gain.
            (void)::CloseHandle(handle);
            return single_instance_lock{nullptr, single_instance_outcome::refused};
        default: {
            // Preserved across CloseHandle, which sets its own last-error value on
            // both success and failure. Without this the caller's "Win32 error N"
            // warning would name whatever the close did, not what the wait failed
            // with -- a diagnostic that points at the wrong thing is worse than none.
            const auto wait_error = ::GetLastError();
            (void)::CloseHandle(handle);
            ::SetLastError(wait_error);
            return single_instance_lock{nullptr, single_instance_outcome::unavailable};
        }
    }
}

std::string single_instance_refusal_message() {
    return "libbsa_tests: another instance of this test binary is already running on this "
           "machine, so this instance is refusing to start.\n"
           "\n"
           "This is the single-instance guard, not a test failure and not a defect in the code "
           "under test. It exists so that a second concurrent run -- or a test process left "
           "over from an earlier session -- is named here, instead of surfacing later as a "
           "shifting set of BA2 DX10 cleanup failures that each pass on --rerun-failed.\n"
           "\n"
           "Running two instances at once is safe: every libbsa_tests process owns a private "
           "temp root and cannot see another process's temp state. To do it on purpose, set " +
           single_instance_opt_out_variable_ascii() +
           "=1 in the environment of every instance that may overlap.\n";
}

std::string single_instance_opt_out_variable_ascii() {
    return narrow_ascii(single_instance_opt_out_variable);
}

void acquire_single_instance_guard() {
    if (opt_out_requested()) {
        startup_outcome() = single_instance_outcome::opted_out;
        return;
    }

    process_lock() = try_acquire_single_instance_lock(std::wstring{single_instance_lock_name});
    startup_outcome() = process_lock().outcome();

    switch (startup_outcome()) {
        case single_instance_outcome::acquired:
        case single_instance_outcome::recovered_abandoned:
            return;
        case single_instance_outcome::refused:
            // Thrown and not also written to stderr here, unlike the private temp
            // root's install failures. Catch2's session catches this out of
            // testRunStarting, prints what() to its own cerr, and exits non-zero
            // before any test case runs -- so writing it as well produced the whole
            // paragraph twice, which is a poor result for a change whose entire
            // purpose is a legible failure. `tests/unit/single_instance_guard_tests.cpp`
            // asserts a spawned child's output carries the text, so the dependency on
            // Catch2 printing it is pinned rather than assumed.
            throw std::runtime_error{single_instance_refusal_message()};
        default: {
            // The guard could not run at all -- no mutex object, or a wait that
            // failed for a reason Windows does not name here. Warn and continue.
            // Blocking the suite would trade a legibility aid for an outage, and
            // the isolation that actually makes concurrency safe is unaffected.
            //
            // Read into a local *before* the stream statement. `a << b` sequences
            // the left operand first, so writing the literal to std::cerr -- which
            // is unbuffered and reaches WriteFile -- would run before
            // ::GetLastError() and could overwrite the value the failed acquisition
            // left behind. That would silently undo the preservation
            // try_acquire_single_instance_lock does across its CloseHandle.
            const auto acquire_error = ::GetLastError();
            std::cerr << "libbsa_tests: the single-instance guard could not acquire its lock "
                         "(Win32 error "
                      << acquire_error << "); continuing without it.\n"
                      << std::flush;
            return;
        }
    }
}

void release_single_instance_guard() noexcept { process_lock().release(); }

single_instance_outcome startup_single_instance_outcome() noexcept { return startup_outcome(); }

}  // namespace libbsa::tests
