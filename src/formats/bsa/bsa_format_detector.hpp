#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::bsa {

/// Byte-classified TES4-family BSA variant selected before parser dispatch.
struct detected_bsa_format {
  archive_variant variant;
  std::uint32_t version;
  entry_compression default_compression;
};

/// Classifies BSA bytes by magic and version without using the host filename.
result<detected_bsa_format> detect_bsa_format(std::span<const std::byte> bytes);

} // namespace libbsa::formats::bsa
