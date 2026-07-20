#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/writer.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

struct ba2_dx10_subresource_snapshot {
    std::uint32_t array_index{};
    std::uint32_t face_index{};
    std::uint32_t mip{};
    std::uint64_t size{};
    std::filesystem::path snapshot_path;
};

struct ba2_dx10_writer_entry {
    std::string archive_path_original;
    std::string archive_path_canonical;
    texture_metadata metadata;
    std::vector<ba2_dx10_subresource_snapshot> subresources;
};

/// Validates and publishes one BA2 DX10 archive through a Finalization Workspace.
///
/// Existing add-time DDS snapshots remain the source of truth during indexed
/// preparation. Returns the first validation, snapshot, format, compression,
/// serialization, or publication error.
result<void> write_ba2_dx10_archive(ba2_dx10_target target, const ba2_dx10_writer_options& options,
                                    std::span<const ba2_dx10_writer_entry> entries,
                                    std::string_view output_host_path, std::uint32_t worker_count);

}  // namespace libbsa::formats::ba2
