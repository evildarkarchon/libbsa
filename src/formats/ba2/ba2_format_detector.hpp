#pragma once

#include <libbsa/result.hpp>

#include "formats/ba2/ba2_profile.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::ba2 {

/// Byte-classified BA2 variant selected before full GNRL parser dispatch.
struct detected_ba2_format {
    ba2_profile profile;
    std::uint32_t file_count;
    /// Legacy parser adapter copy; BA2 Archive Header remains authoritative.
    ba2_archive_metadata stored_metadata;
};

/// Classifies BA2 `BTDX` bytes by version and subtype without using the host
/// filename.
result<detected_ba2_format> detect_ba2_format(std::span<const std::byte> bytes);

}  // namespace libbsa::formats::ba2
