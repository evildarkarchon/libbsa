#pragma once

#include "formats/ba2/ba2_dx10_writer.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace libbsa::formats::ba2 {

/// Validates target-specific DDS DXGI format compatibility before archive serialization.
result<void> ba2_dx10_validate_texture_format_for_target(ba2_dx10_target target, std::uint32_t dxgi_format);

/// Ensures the writer has a temporary snapshot directory for add-time DDS ownership.
result<void> ba2_dx10_ensure_snapshot_directory(std::filesystem::path& snapshot_dir_path);

/// Builds a BA2 DX10 writer entry by loading, validating, and snapshotting DDS source bytes.
result<ba2_dx10_writer_entry> ba2_dx10_build_writer_entry_snapshot(std::string_view archive_path,
                                                                    std::string_view dds_host_path,
                                                                    ba2_dx10_target target,
                                                                    const std::filesystem::path& snapshot_dir,
                                                                    std::size_t entry_index);

} // namespace libbsa::formats::ba2
