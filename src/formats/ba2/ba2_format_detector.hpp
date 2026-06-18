#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::ba2 {

/// Byte-classified BA2 variant selected before full GNRL parser dispatch.
struct detected_ba2_format {
    archive_variant variant;
    std::uint32_t version;
    entry_compression default_compression;
    ba2_archive_metadata ba2;
    bool is_gnrl;
    bool is_dx10;
    std::uint32_t file_count;
};

/// Classifies BA2 `BTDX` bytes by version and subtype without using the host
/// filename.
result<detected_ba2_format> detect_ba2_format(std::span<const std::byte> bytes);

}  // namespace libbsa::formats::ba2
