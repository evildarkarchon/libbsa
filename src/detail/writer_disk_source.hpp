#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::detail {

/// Diagnostics supplied by archive-family writer code for shared disk-source reads.
struct writer_disk_source_context {
  std::string_view open_error;
  std::string_view inspect_error;
  std::string_view read_error;
  std::string_view changed_error;
  std::string_view allocation_description;
};

/// Default scratch-buffer size used when writer code streams disk source bytes internally.
inline constexpr std::size_t writer_disk_source_chunk_size = 64U * 1024U;

/// Returns the current byte size of a regular disk source using caller-owned diagnostics.
result<std::uint64_t> inspect_disk_source_size(std::string_view host_path,
                                               const writer_disk_source_context& context);

/// Reads exactly `expected_size` bytes from `host_path` and rejects source size changes.
result<std::vector<std::byte>> read_disk_source_exact(std::string_view host_path,
                                                      std::uint64_t expected_size,
                                                      const writer_disk_source_context& context);

/// Sizes and reads an entire disk source without byte-at-a-time vector growth.
result<std::vector<std::byte>> read_disk_source_exact(std::string_view host_path,
                                                      const writer_disk_source_context& context);

/// Reads at most `max_bytes` from `host_path`, returning a shorter prefix when the file is shorter.
result<std::vector<std::byte>> read_disk_source_prefix(std::string_view host_path,
                                                       std::size_t max_bytes,
                                                       const writer_disk_source_context& context);

/// Iterates exactly `expected_size` source bytes through bounded chunks and rejects source changes.
result<void> for_each_disk_source_chunk(
    std::string_view host_path,
    std::uint64_t expected_size,
    const writer_disk_source_context& context,
    const std::function<result<void>(std::span<const std::byte>)>& callback,
    std::size_t chunk_size = writer_disk_source_chunk_size);

} // namespace libbsa::detail
