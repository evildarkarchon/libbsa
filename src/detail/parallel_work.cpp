#include <detail/parallel_work.hpp>

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace libbsa::detail {

result<void> run_indexed_work(std::size_t task_count,
                              std::uint32_t worker_count,
                              const std::function<result<void>(std::size_t)>& work) {
  if (worker_count == 0U) {
    return error{error_code::invalid_argument, "worker_count must be greater than zero"};
  }
  if (task_count == 0U) {
    return {};
  }

  if (worker_count == 1U) {
    for (std::size_t index = 0; index < task_count; ++index) {
      auto result = work(index);
      if (!result) {
        return result.error();
      }
    }
    return {};
  }

  std::atomic_size_t next_index{0U};
  std::atomic_bool stop_requested{false};
  std::mutex error_mutex;
  std::optional<error> first_error;

  auto run_worker = [&]() {
    while (!stop_requested.load(std::memory_order_acquire)) {
      const auto index = next_index.fetch_add(1U, std::memory_order_relaxed);
      if (index >= task_count) {
        return;
      }

      auto result = work(index);
      if (!result) {
        {
          std::lock_guard lock{error_mutex};
          if (!first_error.has_value()) {
            first_error = result.error();
          }
        }
        stop_requested.store(true, std::memory_order_release);
        return;
      }
    }
  };

  std::vector<std::jthread> workers;
  workers.reserve(worker_count);
  for (std::uint32_t worker = 0; worker < worker_count; ++worker) {
    workers.emplace_back(run_worker);
  }
  workers.clear();

  if (first_error.has_value()) {
    return *first_error;
  }
  return {};
}

} // namespace libbsa::detail
