#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

struct tes4_writer_entry {
    std::string archive_path_original;
    std::string archive_path_canonical;
    std::string host_path;
    std::vector<std::byte> memory_bytes;
    bool from_memory{false};
    entry_compression_policy compression{entry_compression_policy::inherit};
};

result<void> write_tes4_bsa_archive(tes4_bsa_target target, const tes4_bsa_writer_options& options,
                                    std::span<const tes4_writer_entry> entries,
                                    std::string_view output_host_path, std::uint32_t worker_count);

}  // namespace libbsa::formats::bsa
