#pragma once

#include "formats/bsa/bsa_format_detector.hpp"

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <span>

namespace libbsa::formats::bsa {

/// Parses enough checked TES4-family BSA header state to expose public metadata.
result<archive_metadata> parse_tes4_bsa_metadata(std::span<const std::byte> bytes, detected_bsa_format detected);

} // namespace libbsa::formats::bsa
