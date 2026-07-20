#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::bsa {

/// Byte-classified BSA variant selected before parser dispatch.
struct detected_bsa_format {
    archive_variant variant;
    std::uint32_t version;
};

/// Classifies BSA bytes by syntax before resolving a TES4 BSA Profile.
result<detected_bsa_format> detect_bsa_format(std::span<const std::byte> bytes);

}  // namespace libbsa::formats::bsa
