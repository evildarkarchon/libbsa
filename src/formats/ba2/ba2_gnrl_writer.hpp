#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

struct ba2_gnrl_writer_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::string host_path;
  std::vector<std::byte> memory_bytes;
  bool from_memory{false};
  ba2_gnrl_entry_options options{};
};

result<void> write_ba2_gnrl_archive(ba2_gnrl_target target,
                                    const ba2_gnrl_writer_options& options,
                                    std::span<const ba2_gnrl_writer_entry> entries,
                                    std::string_view output_host_path,
                                    std::uint32_t worker_count);

namespace gnrl_detail {

/// Compares a prepared disk source with in-memory bytes and fails if the disk source size changed.
result<bool> compare_disk_payload_to_bytes(const std::string& host_path, std::span<const std::byte> expected);

/// Compares two prepared disk sources and fails if either source size changed.
result<bool> compare_disk_payloads(const std::string& lhs_path,
                                   const std::string& rhs_path,
                                   std::uint32_t expected_size);

/// Streams exactly the prepared byte count from a disk source and fails if the source was mutated.
result<void> stream_disk_payload(const std::string& host_path,
                                 std::uint32_t expected_size,
                                 std::ostream& output);

} // namespace gnrl_detail

} // namespace libbsa::formats::ba2
