#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

struct tes3_writer_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::string host_path;
  std::vector<std::byte> memory_bytes;
  bool from_memory{false};
};

result<void> write_tes3_bsa_archive(const tes3_bsa_writer_options& options,
                                    std::span<const tes3_writer_entry> entries,
                                    std::string_view output_host_path);

} // namespace libbsa::formats::bsa
