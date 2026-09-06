// Proves the single-instance guard refuses a second concurrent libbsa_tests
// process, says why, and can be opted out of.
//
// The guard is a legibility mechanism, not a safety one: issue #62 gave every test
// process a private temp root, so two instances no longer corrupt each other. What
// the guard replaces is a *moving* set of BA2 DX10 cleanup failures that each pass
// on --rerun-failed and read as a regression in whatever code was just touched.
// So the assertions here are about the diagnostic and the refusal being
// deterministic, not about anything being protected.
//
// Two shapes of test, for two different reasons.
//
// The acquisition-rule cases drive `try_acquire_single_instance_lock` against a
// probe name of their own rather than against the lock this process is holding --
// the same sandboxing the stale-root sweep tests use, and for the same reason: a
// test that manipulated the live lock would be interfering with the guard it is
// trying to observe.
//
// The refusal cases spawn a real second process, because a guard cannot refuse the
// process that holds it. `tests/support/child_test_process.hpp` has that
// machinery; note that it sets or *deletes* the opt-out explicitly rather than
// letting the child inherit whatever the developer's shell has, so a shell with
// the opt-out set cannot turn "this child must be refused" into a false pass.

#include <catch2/catch_test_macros.hpp>

#include "support/child_test_process.hpp"
#include "support/private_temp_root.hpp"
#include "support/single_instance_guard.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <filesystem>
#include <semaphore>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

namespace {

using libbsa::tests::child_test_process_options;
using libbsa::tests::private_root_report_variable;
using libbsa::tests::read_text_file;
using libbsa::tests::run_child_test_binary;
using libbsa::tests::scoped_environment_variable;
using libbsa::tests::scoped_handle;
using libbsa::tests::single_instance_lock_name;
using libbsa::tests::single_instance_opt_out_variable_ascii;
using libbsa::tests::single_instance_outcome;
using libbsa::tests::try_acquire_single_instance_lock;

/// A probe lock name that cannot collide with the real one or with another case's.
std::wstring probe_lock_name(std::wstring_view suffix) {
    return std::wstring{single_instance_lock_name} + L"-probe-" + std::wstring{suffix};
}

/// Requires that this process holds the guard's lock, skipping only when it was
/// deliberately told not to take one.
///
/// Every spawn-a-child case below asserts something about what happens *while
/// another instance holds the lock*. If this process was started with the opt-out
/// set -- which is exactly how the two-instance concurrency proof runs the binary --
/// nothing holds it, a child would start happily, and the case would pass or fail
/// for a reason unrelated to the guard. Skipping says so instead.
///
/// Every *other* way of not holding the lock is a hard failure rather than a skip,
/// and the distinction is load-bearing. CTest reads Catch2's skip as a pass, so a
/// guard that quietly stopped acquiring would otherwise turn all four spawn-based
/// cases green by skipping them -- the regression hiding in the mechanism meant to
/// report it. Only an explicit opt-out is a configuration these cases cannot speak
/// about; `unavailable` is a malfunction and should read as one.
void require_own_guard_held_or_skip() {
    const auto outcome = libbsa::tests::startup_single_instance_outcome();
    switch (outcome) {
        case single_instance_outcome::acquired:
        case single_instance_outcome::recovered_abandoned:
            return;
        case single_instance_outcome::opted_out:
            SKIP("this process was started with " + single_instance_opt_out_variable_ascii() +
                 " set, so it holds no single-instance lock for a second instance to contend "
                 "with");
        default:
            FAIL("the single-instance guard did not take its lock at start-up (outcome "
                 << static_cast<int>(outcome)
                 << "), so it is not working -- which is a failure, not a reason to skip");
    }
}

/// Removes a file, ignoring the case where it was never created.
void remove_quietly(const std::filesystem::path& path) {
    std::error_code fs_error;
    std::filesystem::remove(path, fs_error);
}

}  // namespace

TEST_CASE("the running test process holds the single-instance lock",
          "[unit][single_instance_guard]") {
    require_own_guard_held_or_skip();

    const scoped_handle probe{
        ::OpenMutexW(SYNCHRONIZE, FALSE, std::wstring{single_instance_lock_name}.c_str())};
    // The object exists because this process holds it. A failure here means the
    // guard took a lock under some other name than the one it publishes.
    REQUIRE(probe.valid());

    // Probed from another thread deliberately. Windows mutex ownership is per
    // thread and recursive, so a zero-timeout wait issued on the thread that
    // already owns the mutex succeeds and would prove nothing at all.
    DWORD wait_result = WAIT_FAILED;
    std::thread prober{[&] { wait_result = ::WaitForSingleObject(probe.get(), 0); }};
    prober.join();

    CHECK(wait_result == WAIT_TIMEOUT);
}

TEST_CASE("an unheld single-instance lock is acquired and a held one is refused",
          "[unit][single_instance_guard]") {
    const auto lock_name = probe_lock_name(L"contention");

    // A second handle to the same object, held for the whole case, and it is what
    // makes the outcomes below mean anything.
    //
    // A named mutex is destroyed when its *last* handle closes. Without this
    // keep-alive, every release below would take the object with it and the next
    // acquisition would create a brand-new, unowned mutex -- which reports
    // `acquired` whether the previous owner released properly or merely closed its
    // handle. Every assertion in this case would then pass against a guard whose
    // `release()` never called ReleaseMutex at all.
    const scoped_handle keepalive{::CreateMutexW(nullptr, FALSE, lock_name.c_str())};
    REQUIRE(keepalive.valid());

    {
        const auto first = try_acquire_single_instance_lock(lock_name);
        CHECK(first.outcome() == single_instance_outcome::acquired);
        CHECK(first.held());
    }

    // A second holder on another thread, because ownership is per thread: taking
    // the same mutex twice on this one would succeed recursively and show nothing.
    std::binary_semaphore holder_ready{0};
    std::binary_semaphore holder_may_finish{0};
    auto holder_outcome = single_instance_outcome::not_attempted;

    std::thread holder{[&] {
        const auto held = try_acquire_single_instance_lock(lock_name);
        holder_outcome = held.outcome();
        holder_ready.release();
        holder_may_finish.acquire();
        // `held` is released here, on the thread that acquired it, which is the
        // only thread allowed to release it.
    }};
    holder_ready.acquire();

    // Asserted on the main thread: Catch2's assertion macros are not thread-safe,
    // so the worker only ever records what happened.
    //
    // This is also what pins release-on-scope-exit, given the keep-alive above. The
    // block released the mutex, so the holder sees `acquired`. Had `release()` only
    // closed its handle, the mutex would still be owned by *this* thread -- which is
    // alive and never released it -- and the holder's zero-timeout wait would come
    // back `refused`.
    CHECK(holder_outcome == single_instance_outcome::acquired);

    const auto contending = try_acquire_single_instance_lock(lock_name);
    CHECK(contending.outcome() == single_instance_outcome::refused);
    CHECK_FALSE(contending.held());

    holder_may_finish.release();
    holder.join();

    // Once the holder is gone the same name is free again, which is what makes the
    // refusal above a statement about the *holder* rather than about the name.
    //
    // And this one distinguishes released from abandoned in the other direction. The
    // holder thread has ended; had it dropped the mutex by closing its handle rather
    // than releasing it, the thread's termination would have abandoned an object the
    // keep-alive kept alive, and this would read `recovered_abandoned`.
    const auto after = try_acquire_single_instance_lock(lock_name);
    CHECK(after.outcome() == single_instance_outcome::acquired);
    CHECK(after.held());
}

TEST_CASE("a single-instance lock abandoned by a dead owner is recovered",
          "[unit][single_instance_guard]") {
    // The criterion this covers is that a lock left behind by a crashed process is
    // recovered rather than blocking every later run forever.
    //
    // Windows gives that two ways, and this covers the harder one. When a killed
    // process held the *only* handle, the object is destroyed outright and the next
    // instance simply creates it fresh -- nothing to recover. When some other handle
    // keeps the object alive, the dead owner's ownership survives as an *abandoned*
    // state, and only a waiter that treats WAIT_ABANDONED as success gets through.
    //
    // A thread that ends while owning a mutex abandons it exactly as a process that
    // dies while owning one does, so the dying owner here is a thread. The handle
    // below is what plays the part of the surviving second handle.
    const auto lock_name = probe_lock_name(L"abandoned");

    const scoped_handle keepalive{::CreateMutexW(nullptr, FALSE, lock_name.c_str())};
    REQUIRE(keepalive.valid());

    DWORD doomed_wait_result = WAIT_FAILED;
    std::thread doomed{[&] { doomed_wait_result = ::WaitForSingleObject(keepalive.get(), 0); }};
    doomed.join();
    // The thread took ownership and then ended without releasing it. Nothing else
    // in this test touches the mutex in between.
    REQUIRE(doomed_wait_result == WAIT_OBJECT_0);

    const auto recovered = try_acquire_single_instance_lock(lock_name);
    CHECK(recovered.outcome() == single_instance_outcome::recovered_abandoned);
    // The half that matters: recovery means the lock is *taken*, not merely that
    // the abandonment was noticed. A guard that reported the state and then refused
    // would still block every later run.
    CHECK(recovered.held());
}

TEST_CASE("a second concurrent test instance is refused with a diagnostic naming the condition",
          "[unit][single_instance_guard]") {
    require_own_guard_held_or_skip();

    const auto& private_root = libbsa::tests::private_temp_root();
    REQUIRE_FALSE(private_root.empty());

    const auto report_path = private_root / "refused-child-private-root.txt";
    const auto log_path = private_root / "refused-child-output.txt";
    remove_quietly(report_path);

    DWORD exit_code = 0;
    {
        // The child is filtered to the case that writes this file as its first act,
        // so the file's absence afterwards is a direct statement that no test body
        // ran -- rather than an inference from the exit code.
        const scoped_environment_variable report_variable{private_root_report_variable,
                                                          report_path};
        exit_code = run_child_test_binary(
            child_test_process_options{.arguments = {L"[private_temp_root_report]"},
                                       .allow_concurrent_instances = false},
            log_path);
    }

    const auto log = read_text_file(log_path);
    INFO("child output: " << log);
    INFO("child exit code: " << exit_code);

    CHECK(exit_code != 0U);

    // Names the condition rather than failing generically. The phrase is restated
    // here rather than taken from the guard, because a test that asserted on the
    // guard's own message could not tell a message that names the condition from
    // one that says nothing at all.
    CHECK(log.find("already running") != std::string::npos);

    // Mentions the opt-out. This half *is* shared with the guard, for the opposite
    // reason: a test that checked some other spelling of the variable would prove
    // nothing about the one the guard actually reads.
    CHECK(log.find(single_instance_opt_out_variable_ascii()) != std::string::npos);

    CHECK_FALSE(std::filesystem::exists(report_path));

    remove_quietly(log_path);
}

TEST_CASE("the single-instance opt-out lets a second instance run",
          "[unit][single_instance_guard]") {
    require_own_guard_held_or_skip();

    const auto& private_root = libbsa::tests::private_temp_root();
    REQUIRE_FALSE(private_root.empty());

    const auto report_path = private_root / "opted-in-child-private-root.txt";
    const auto log_path = private_root / "opted-in-child-output.txt";
    remove_quietly(report_path);

    DWORD exit_code = 0;
    {
        const scoped_environment_variable report_variable{private_root_report_variable,
                                                          report_path};
        exit_code = run_child_test_binary(
            child_test_process_options{.arguments = {L"[private_temp_root_report]"},
                                       .allow_concurrent_instances = true},
            log_path);
    }

    INFO("child output: " << read_text_file(log_path));
    CHECK(exit_code == 0U);

    // The child got past the guard *and* ran a test body. The exit code alone would
    // also be zero for a run that matched no test cases at all.
    CHECK(std::filesystem::exists(report_path));

    remove_quietly(report_path);
    remove_quietly(log_path);
}

TEST_CASE("CTest test discovery is unaffected by the single-instance guard",
          "[unit][single_instance_guard]") {
    require_own_guard_held_or_skip();

    const auto& private_root = libbsa::tests::private_temp_root();
    REQUIRE_FALSE(private_root.empty());

    const auto log_path = private_root / "discovery-child-output.txt";

    // Exactly the invocation CatchAddTests.cmake issues under DISCOVERY_MODE
    // PRE_TEST, and deliberately without the opt-out: CTest does not set it, so a
    // discovery run that needed it would break every configure of this project.
    //
    // What makes this pass is that listing test cases is not a test run, so Catch2
    // fires no testRunStarting and the guard never executes. That is a property of
    // Catch2 rather than of this repository, which is why it is asserted rather
    // than assumed.
    const auto exit_code = run_child_test_binary(
        child_test_process_options{.arguments = {L"--list-tests", L"--reporter", L"json"},
                                   .allow_concurrent_instances = false},
        log_path);

    const auto log = read_text_file(log_path);
    INFO("child output: " << log);

    CHECK(exit_code == 0U);
    // A real listing, not an empty success: this case's own name has to be in it.
    CHECK(log.find("CTest test discovery is unaffected by the single-instance guard") !=
          std::string::npos);
    // And the guard said nothing at all, which is what "unaffected" means here --
    // not merely that it let the listing through. Probed by the opt-out variable's
    // name, which the guard names in every message it prints and which cannot
    // collide with a test case name in the listing the way an English phrase could.
    CHECK(log.find(single_instance_opt_out_variable_ascii()) == std::string::npos);

    remove_quietly(log_path);
}
