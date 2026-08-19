#pragma once

#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <cstdint>
#include <filesystem>

namespace libbsa::formats::ba2 {

/// Stores the version-gated raw fields emitted in a BA2 fixed header.
///
/// The defaults mirror xEdit/BSArchPro's Starfield writer initialization:
/// Unknown1/Unknown2 are 1/0, and version 3 selects raw LZ4 blocks with method 3.
struct ba2_stored_header_fields {
    std::uint32_t starfield_unknown1{1U};
    std::uint32_t starfield_unknown2{0U};
    std::uint32_t starfield_compression_method{3U};
};

/// Serializes a finalized GNRL Placement Plan to the supplied host output path.
///
/// The plan remains authoritative for record order, payload placement and
/// filename-table geometry. The function validates the typed profile before
/// creating or truncating the output.
result<void> serialize_ba2_archive(const ba2_profile& profile,
                                   const ba2_stored_header_fields& stored_header_fields,
                                   const ba2_gnrl_placement_plan& plan,
                                   const std::filesystem::path& output_path);

/// Serializes a finalized DX10 Placement Plan to the supplied host output path.
///
/// The plan remains authoritative for texture order, chunk geometry, payload
/// placement and filename-table geometry. The function validates the typed
/// profile before creating or truncating the output.
result<void> serialize_ba2_archive(const ba2_profile& profile,
                                   const ba2_stored_header_fields& stored_header_fields,
                                   const ba2_dx10_placement_plan& plan,
                                   const std::filesystem::path& output_path);

}  // namespace libbsa::formats::ba2
