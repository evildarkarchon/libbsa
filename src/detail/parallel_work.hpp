#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>

namespace libbsa::detail {

/// Runs indexed work items using a positive caller-specified worker count.
///
/// `worker_count == 1` executes indexes in ascending order on the calling
/// thread. Larger counts use scoped C++20 worker threads and leave
/// deterministic result placement to the caller-provided indexed work function.
result<void> run_indexed_work(std::size_t task_count, std::uint32_t worker_count,
                              const std::function<result<void>(std::size_t)>& work);

}  // namespace libbsa::detail
