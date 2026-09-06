#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
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

/// Validates and publishes one BA2 GNRL archive through a Finalization Workspace.
///
/// `worker_count` bounds indexed preparation after destination reservation.
/// Returns the first validation, source, format, compression, serialization, or
/// publication error.
result<void> write_ba2_gnrl_archive(ba2_gnrl_target target, const ba2_gnrl_writer_options& options,
                                    std::span<const ba2_gnrl_writer_entry> entries,
                                    std::string_view output_host_path, std::uint32_t worker_count);

}  // namespace libbsa::formats::ba2
