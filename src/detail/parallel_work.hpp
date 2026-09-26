#pragma once

#include <libbsa/result.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace libbsa::detail {

/// Runs indexed work items using a positive caller-specified worker count.
///
/// `worker_count == 1` executes indexes in ascending order on the calling
/// thread. Larger counts use scoped C++20 worker threads and leave
/// deterministic result placement to the caller-provided indexed work function.
result<void> run_indexed_work(std::size_t task_count, std::uint32_t worker_count,
                              const std::function<result<void>(std::size_t)>& work);

/// Collects move-only preparation results in input order without exposing partial results.
///
/// Each work(index) returns result<T>; concurrent calls must own independent work state.
/// Worker limits, serial caller-thread execution, and scoped joining follow run_indexed_work.
/// The first observed callback failure wins, whether it returns an error or throws. Workers
/// stop as they observe failure; already-running work is joined and partial values are destroyed
/// before returning that error or rethrowing the original exception on the calling thread.
/// Scheduler failures return an error when no callback failed. Collection allocation/move
/// exceptions propagate on the calling thread after joined work is safely released.
/// Values may borrow a workspace, but this module never takes ownership of its snapshot files.
template <typename T, typename Work>
    requires requires(Work& work, std::size_t index) {
        { std::invoke(work, index) } -> std::same_as<result<T>>;
    }
result<std::vector<T>> collect_indexed_work(std::size_t task_count, std::uint32_t worker_count,
                                            Work&& work) {
    // Reuse the scheduler's argument contract before allocating archive-count-sized storage.
    // With no tasks this validates the worker count without invoking work or starting threads.
    auto validated = run_indexed_work(0U, worker_count, {});
    if (!validated) {
        return validated.error();
    }

    std::vector<std::optional<T>> values(task_count);
    std::mutex failure_mutex;
    std::variant<std::monostate, error, std::exception_ptr> first_failure;
    auto collect = [&](std::size_t index) -> result<void> {
        try {
            auto prepared = std::invoke(work, index);
            if (prepared) {
                values[index].emplace(std::move(prepared).value());
                return {};
            }

            // Copy before publishing: an allocation failure must not leave the shared variant
            // valueless and prevent the catch below from recording that exception instead.
            auto failure = prepared.error();
            std::lock_guard lock{failure_mutex};
            if (std::holds_alternative<std::monostate>(first_failure)) {
                first_failure.template emplace<error>(std::move(failure));
            }
        } catch (...) {
            std::lock_guard lock{failure_mutex};
            if (std::holds_alternative<std::monostate>(first_failure)) {
                first_failure.template emplace<std::exception_ptr>(std::current_exception());
            }
        }

        // Only the stop signal crosses the existing scheduler. Keeping its diagnostic empty
        // avoids another allocating error copy on a worker, including after bad_alloc.
        return error{error_code::io_error, {}};
    };

    auto worked = run_indexed_work(task_count, worker_count, collect);
    if (!worked) {
        // run_indexed_work has joined every started worker; releasing slots here cannot race
        // a late successful callback, and caller-visible failure never retains partial values.
        values.clear();
        if (auto failure = std::get_if<error>(&first_failure)) {
            return std::move(*failure);
        }
        if (auto failure = std::get_if<std::exception_ptr>(&first_failure)) {
            std::rethrow_exception(*failure);
        }
        return worked.error();
    }

    std::vector<T> collected;
    collected.reserve(task_count);
    for (auto& value : values) {
        collected.push_back(std::move(value).value());
    }
    return collected;
}

}  // namespace libbsa::detail
