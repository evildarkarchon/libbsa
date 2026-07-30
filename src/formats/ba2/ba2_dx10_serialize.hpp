#pragma once

#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <cstdint>
#include <filesystem>

namespace libbsa::formats::ba2 {

/// Retains only the version-gated public option fields stored in a DX10 header.
struct ba2_dx10_stored_header_options {
    std::uint32_t starfield_unknown1{1U};
    std::uint32_t starfield_unknown2{0U};
    std::uint32_t starfield_compression_method{3U};
};

/// Writes a BA2 DX10 Placement Plan to the publish helper's temporary path.
///
/// `header_options` supplies only version-gated stored fields without placing
/// per-archive metadata in the reusable BA2 Profile. The plan remains
/// authoritative for offsets, sharing, filename geometry, and emission order.
result<void> ba2_dx10_write_archive_bytes(const ba2_profile& profile,
                                          const ba2_dx10_stored_header_options& header_options,
                                          const ba2_dx10_placement_plan& plan,
                                          const std::filesystem::path& output_path);

}  // namespace libbsa::formats::ba2
