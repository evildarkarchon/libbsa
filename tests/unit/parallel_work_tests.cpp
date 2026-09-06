#include <catch2/catch_test_macros.hpp>

#include <detail/parallel_work.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {

/// Releases a value while counting ownership destruction across worker threads.
struct counted_delete {
    std::atomic_size_t* destroyed;

    /// Records only destruction of owned integers; moved-from pointers do not call this.
    void operator()(int* value) const noexcept {
        delete value;
        destroyed->fetch_add(1U);
    }
};

/// A non-default-constructible, move-only preparation result with observable ownership.
struct prepared_value {
    std::unique_ptr<int, counted_delete> value;

    /// Creates one owned value whose lifetime is counted until collection or cleanup.
    prepared_value(int number, std::atomic_size_t& destroyed)
        : value(new int{number}, counted_delete{&destroyed}) {}

    prepared_value(prepared_value&&) noexcept = default;
    prepared_value& operator=(prepared_value&&) noexcept = default;
    prepared_value(const prepared_value&) = delete;
    prepared_value& operator=(const prepared_value&) = delete;
};

/// Waits for a test handshake, bounding broken scheduling instead of hanging the suite.
void await_signal(std::binary_semaphore& signal) {
    if (!signal.try_acquire_for(std::chrono::seconds{5})) {
        throw std::runtime_error{"ordered preparation test handshake timed out"};
    }
}

/// Carries an identity independent of diagnostic wording through exception propagation.
struct preparation_exception {
    std::size_t index;
};

/// A preparation result that can fail only on moves after its callback returns.
struct move_sensitive_value {
    std::unique_ptr<int, counted_delete> value;
    std::shared_ptr<std::atomic_bool> throw_on_move;

    /// Creates an owned value with a separately armed move failure.
    move_sensitive_value(int index, std::atomic_size_t& destroyed,
                         std::shared_ptr<std::atomic_bool> armed)
        : value(new int{index}, counted_delete{&destroyed}), throw_on_move(std::move(armed)) {}

    /// Transfers ownership unless armed, preserving the source on a failed move.
    move_sensitive_value(move_sensitive_value&& other)
        : value(nullptr, other.value.get_deleter()), throw_on_move(other.throw_on_move) {
        if (throw_on_move->load()) {
            throw preparation_exception{static_cast<std::size_t>(*other.value)};
        }
        value = std::move(other.value);
    }
};

/// Arms a result after its return expression finishes constructing the callback result.
struct arm_move_on_return {
    std::shared_ptr<std::atomic_bool> armed;

    /// Avoids relying on the number of moves used to construct a successful result.
    ~arm_move_on_return() {
        if (armed) {
            armed->store(true);
        }
    }
};

/// Signals actual worker exit, after the collector has observed that worker's failure.
struct notify_worker_exit {
    std::shared_ptr<std::binary_semaphore> signal;

    /// Releases the shared handshake only after the worker's callback and body end.
    ~notify_worker_exit() {
        if (signal) {
            signal->release();
        }
    }
};

/// Keeps cleanup observations alive for every worker participating in failure handshakes.
struct failure_cleanup_state {
    std::atomic_size_t destroyed{0U};
    std::atomic_size_t unexpected_work{0U};
    std::atomic_bool delayed_work_finished{false};
    std::binary_semaphore delayed_work_started{0};
    std::shared_ptr<std::binary_semaphore> failed_worker_exited =
        std::make_shared<std::binary_semaphore>(0);
};

}  // namespace

TEST_CASE("ordered preparation returns move-only values in input order after out-of-order work",
          "[unit][parallel_work]") {
    std::atomic_size_t destroyed{0U};
    std::binary_semaphore later_work_started{0};

    {
        auto collected = libbsa::detail::collect_indexed_work<prepared_value>(
            4U, 2U, [&](std::size_t index) -> libbsa::result<prepared_value> {
                if (index == 0U) {
                    await_signal(later_work_started);
                } else if (index == 2U) {
                    // With two workers, reaching item 2 proves item 1 was already
                    // collected while item 0 remained blocked inside its callback.
                    later_work_started.release();
                }
                return prepared_value{static_cast<int>((index + 1U) * 10U), destroyed};
            });

        REQUIRE(collected.has_value());
        REQUIRE(collected.value().size() == 4U);
        CHECK(*collected.value()[0].value == 10);
        CHECK(*collected.value()[1].value == 20);
        CHECK(*collected.value()[2].value == 30);
        CHECK(*collected.value()[3].value == 40);
        CHECK(destroyed.load() == 0U);
    }

    CHECK(destroyed.load() == 4U);
}

TEST_CASE("ordered preparation runs serial callbacks in ascending order on the caller thread",
          "[unit][parallel_work]") {
    const auto caller = std::this_thread::get_id();
    bool used_caller_thread = true;
    std::vector<std::size_t> visited;

    auto collected = libbsa::detail::collect_indexed_work<int>(
        3U, 1U, [&](std::size_t index) -> libbsa::result<int> {
            used_caller_thread = used_caller_thread && std::this_thread::get_id() == caller;
            visited.push_back(index);
            return static_cast<int>(index + 10U);
        });

    REQUIRE(collected.has_value());
    CHECK(used_caller_thread);
    CHECK(visited == std::vector<std::size_t>{0U, 1U, 2U});
    CHECK(collected.value() == std::vector<int>{10, 11, 12});
}

TEST_CASE("ordered preparation stops serial work at the first error and destroys partial values",
          "[unit][parallel_work]") {
    std::atomic_size_t destroyed{0U};
    std::vector<std::size_t> visited;

    auto collected = libbsa::detail::collect_indexed_work<prepared_value>(
        5U, 1U, [&](std::size_t index) -> libbsa::result<prepared_value> {
            visited.push_back(index);
            if (index == 2U) {
                return libbsa::error{libbsa::error_code::not_found, "missing source"};
            }
            return prepared_value{static_cast<int>(index), destroyed};
        });

    REQUIRE_FALSE(collected.has_value());
    CHECK(collected.error().code == libbsa::error_code::not_found);
    CHECK(visited == std::vector<std::size_t>{0U, 1U, 2U});
    CHECK(destroyed.load() == 2U);
}

TEST_CASE("ordered preparation validates worker bounds before empty work",
          "[unit][parallel_work]") {
    for (const auto worker_count : {0U, 1025U}) {
        for (const std::size_t task_count : {0U, 3U}) {
            std::atomic_size_t invoked{0U};
            auto collected = libbsa::detail::collect_indexed_work<int>(
                task_count, worker_count, [&](std::size_t) -> libbsa::result<int> {
                    invoked.fetch_add(1U);
                    return 1;
                });

            REQUIRE_FALSE(collected.has_value());
            CHECK(collected.error().code == libbsa::error_code::invalid_argument);
            CHECK(invoked.load() == 0U);
        }
    }

    for (const auto worker_count : {1U, 1024U}) {
        std::atomic_size_t invoked{0U};
        auto collected = libbsa::detail::collect_indexed_work<int>(
            0U, worker_count, [&](std::size_t) -> libbsa::result<int> {
                invoked.fetch_add(1U);
                return 1;
            });

        REQUIRE(collected.has_value());
        CHECK(collected.value().empty());
        CHECK(invoked.load() == 0U);
    }
}

TEST_CASE("ordered preparation preserves the first observed failure across errors and exceptions",
          "[unit][parallel_work]") {
    bool exception_first = false;
    SECTION("a returned error wins over a later exception at a lower input index") {}
    SECTION("an exception wins over a later returned error at a lower input index") {
        exception_first = true;
    }

    auto lower_index_started = std::make_shared<std::binary_semaphore>(0);
    auto first_worker_exited = std::make_shared<std::binary_semaphore>(0);
    auto finished = std::make_shared<std::atomic_size_t>(0U);
    std::optional<libbsa::result<std::vector<int>>> collected;
    std::optional<std::size_t> thrown_index;

    try {
        collected.emplace(libbsa::detail::collect_indexed_work<int>(
            2U, 2U, [=](std::size_t index) -> libbsa::result<int> {
                if (index == 0U) {
                    lower_index_started->release();
                    // Returning or throwing from item 1 alone would race failure
                    // recording. Its thread exit proves the collector saw it first.
                    await_signal(*first_worker_exited);
                    finished->fetch_add(1U);
                    if (exception_first) {
                        return libbsa::error{libbsa::error_code::io_error, "later error"};
                    }
                    throw preparation_exception{0U};
                }

                await_signal(*lower_index_started);
                thread_local notify_worker_exit exit_notice;
                exit_notice.signal = first_worker_exited;
                finished->fetch_add(1U);
                if (exception_first) {
                    throw preparation_exception{1U};
                }
                return libbsa::error{libbsa::error_code::format_error, "first error"};
            }));
    } catch (const preparation_exception& failure) {
        thrown_index = failure.index;
    }

    CHECK(finished->load() == 2U);
    if (exception_first) {
        CHECK_FALSE(collected.has_value());
        REQUIRE(thrown_index.has_value());
        CHECK(*thrown_index == 1U);
    } else {
        CHECK_FALSE(thrown_index.has_value());
        REQUIRE(collected.has_value());
        REQUIRE_FALSE(collected->has_value());
        CHECK(collected->error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("ordered preparation destroys partial values before serial exceptions reach the caller",
          "[unit][parallel_work]") {
    std::atomic_size_t destroyed{0U};
    std::vector<std::size_t> visited;
    std::optional<std::size_t> thrown_index;

    try {
        (void)libbsa::detail::collect_indexed_work<prepared_value>(
            4U, 1U, [&](std::size_t index) -> libbsa::result<prepared_value> {
                visited.push_back(index);
                if (index == 2U) {
                    throw preparation_exception{index};
                }
                return prepared_value{static_cast<int>(index), destroyed};
            });
    } catch (const preparation_exception& failure) {
        thrown_index = failure.index;
        CHECK(destroyed.load() == 2U);
    }

    REQUIRE(thrown_index.has_value());
    CHECK(*thrown_index == 2U);
    CHECK(visited == std::vector<std::size_t>{0U, 1U, 2U});
}

TEST_CASE(
    "ordered preparation stops scheduling and cleans up joined workers before reporting failure",
    "[unit][parallel_work]") {
    bool throw_failure = false;
    SECTION("returned error") {}
    SECTION("callback exception") { throw_failure = true; }

    auto state = std::make_shared<failure_cleanup_state>();
    std::optional<libbsa::result<std::vector<prepared_value>>> collected;
    std::optional<std::size_t> thrown_index;

    try {
        collected.emplace(libbsa::detail::collect_indexed_work<prepared_value>(
            6U, 2U, [state, throw_failure](std::size_t index) -> libbsa::result<prepared_value> {
                if (index == 1U) {
                    state->delayed_work_started.release();
                    await_signal(*state->failed_worker_exited);
                    state->delayed_work_finished.store(true);
                } else if (index == 2U) {
                    await_signal(state->delayed_work_started);
                    thread_local notify_worker_exit exit_notice;
                    exit_notice.signal = state->failed_worker_exited;
                    if (throw_failure) {
                        throw preparation_exception{index};
                    }
                    return libbsa::error{libbsa::error_code::io_error, "preparation failed"};
                } else if (index >= 3U) {
                    // Item 1 resumes only after failure recording, so none of
                    // these items should be scheduled by its returning worker.
                    state->unexpected_work.fetch_add(1U);
                }
                return prepared_value{static_cast<int>(index), state->destroyed};
            }));
    } catch (const preparation_exception& failure) {
        thrown_index = failure.index;
        CHECK(state->delayed_work_finished.load());
        CHECK(state->destroyed.load() == 2U);
    }

    CHECK(state->delayed_work_finished.load());
    CHECK(state->unexpected_work.load() == 0U);
    CHECK(state->destroyed.load() == 2U);
    if (throw_failure) {
        CHECK_FALSE(collected.has_value());
        REQUIRE(thrown_index.has_value());
        CHECK(*thrown_index == 2U);
    } else {
        CHECK_FALSE(thrown_index.has_value());
        REQUIRE(collected.has_value());
        REQUIRE_FALSE(collected->has_value());
        CHECK(collected->error().code == libbsa::error_code::io_error);
    }
}

TEST_CASE("ordered preparation keeps overlapping calls and later recovery isolated",
          "[unit][parallel_work]") {
    std::binary_semaphore success_started{0};
    std::binary_semaphore failure_finished{0};
    std::optional<libbsa::result<std::vector<int>>> successful;
    std::optional<libbsa::result<std::vector<int>>> failed;
    std::exception_ptr success_exception;
    std::exception_ptr failure_exception;

    {
        std::jthread successful_call{[&] {
            try {
                successful.emplace(libbsa::detail::collect_indexed_work<int>(
                    3U, 2U, [&](std::size_t index) -> libbsa::result<int> {
                        if (index == 0U) {
                            success_started.release();
                            await_signal(failure_finished);
                        }
                        return static_cast<int>(index + 20U);
                    }));
            } catch (...) {
                success_exception = std::current_exception();
            }
        }};
        std::jthread failed_call{[&] {
            try {
                await_signal(success_started);
                failed.emplace(libbsa::detail::collect_indexed_work<int>(
                    2U, 2U, [](std::size_t) -> libbsa::result<int> {
                        return libbsa::error{libbsa::error_code::not_found, "missing source"};
                    }));
            } catch (...) {
                failure_exception = std::current_exception();
            }
            failure_finished.release();
        }};
    }

    REQUIRE_FALSE(success_exception);
    REQUIRE_FALSE(failure_exception);
    REQUIRE(failed.has_value());
    REQUIRE_FALSE(failed->has_value());
    CHECK(failed->error().code == libbsa::error_code::not_found);
    REQUIRE(successful.has_value());
    REQUIRE(successful->has_value());
    CHECK(successful->value() == std::vector<int>{20, 21, 22});

    auto recovered = libbsa::detail::collect_indexed_work<int>(
        2U, 2U,
        [](std::size_t index) -> libbsa::result<int> { return static_cast<int>(index + 30U); });
    REQUIRE(recovered.has_value());
    CHECK(recovered.value() == std::vector<int>{30, 31});
}

TEST_CASE("ordered preparation propagates result storage move failures after cleaning up",
          "[unit][parallel_work]") {
    std::uint32_t worker_count = 1U;
    SECTION("serial storage") {}
    SECTION("parallel storage") { worker_count = 2U; }

    auto destroyed = std::make_shared<std::atomic_size_t>(0U);
    auto later_work_started = std::make_shared<std::binary_semaphore>(0);
    std::optional<std::size_t> thrown_index;

    try {
        (void)libbsa::detail::collect_indexed_work<move_sensitive_value>(
            3U, worker_count, [=](std::size_t index) -> libbsa::result<move_sensitive_value> {
                if (worker_count > 1U && index == 1U) {
                    await_signal(*later_work_started);
                } else if (index == 2U) {
                    // Item 0 was already stored before a worker could claim item 2.
                    later_work_started->release();
                }

                auto armed = std::make_shared<std::atomic_bool>(false);
                arm_move_on_return arm{index == 1U ? armed : nullptr};
                // The return value is fully constructed before the local guard
                // arms it; only the collector's subsequent ownership move throws.
                return libbsa::result<move_sensitive_value>{
                    move_sensitive_value{static_cast<int>(index), *destroyed, armed}};
            });
    } catch (const preparation_exception& failure) {
        thrown_index = failure.index;
        CHECK(destroyed->load() == (worker_count == 1U ? 2U : 3U));
    }

    REQUIRE(thrown_index.has_value());
    CHECK(*thrown_index == 1U);
}
