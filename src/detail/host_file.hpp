#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::detail {

/// Diagnostics supplied by archive-family code for shared host-file reads.
struct host_file_context {
  std::string_view open_error;
  std::string_view inspect_error;
  std::string_view read_error;
  std::string_view changed_error;
  std::string_view allocation_description;
};

/// Default scratch-buffer size used when internal code streams host-file bytes.
inline constexpr std::size_t host_file_chunk_size = 64U * 1024U;

/// Opens a host file through the shared filesystem boundary using caller-owned diagnostics.
result<std::ifstream> open_host_file(std::string_view host_path, const host_file_context& context);

/// Returns the current byte size of a regular host file using caller-owned diagnostics.
result<std::uint64_t> inspect_host_file_size(std::string_view host_path, const host_file_context& context);

/// Reads exactly `expected_size` bytes from `host_path` and rejects host-file size changes.
result<std::vector<std::byte>> read_host_file_exact(std::string_view host_path,
                                                    std::uint64_t expected_size,
                                                    const host_file_context& context);

/// Sizes and reads an entire host file without byte-at-a-time vector growth.
result<std::vector<std::byte>> read_host_file_exact(std::string_view host_path, const host_file_context& context);

/// Reads at most `max_bytes` from `host_path`, returning a shorter prefix when the file is shorter.
result<std::vector<std::byte>> read_host_file_prefix(std::string_view host_path,
                                                     std::size_t max_bytes,
                                                     const host_file_context& context);

/// Iterates exactly `expected_size` host-file bytes through bounded chunks and rejects source changes.
result<void> for_each_host_file_chunk(std::string_view host_path,
                                      std::uint64_t expected_size,
                                      const host_file_context& context,
                                      const std::function<result<void>(std::span<const std::byte>)>& callback,
                                      std::size_t chunk_size = host_file_chunk_size);

} // namespace libbsa::detail
