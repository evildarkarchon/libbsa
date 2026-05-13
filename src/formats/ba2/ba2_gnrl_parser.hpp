#pragma once

#include "formats/ba2/ba2_format_detector.hpp"

#include <detail/host_file_path.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

/// Parsed BA2 GNRL archive metadata and deterministic public entry values.
struct ba2_gnrl_archive {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
};

/// Parses checked BA2 GNRL header, record, filename table, and entry metadata state.
result<ba2_gnrl_archive> parse_ba2_gnrl_archive(std::span<const std::byte> bytes, detected_ba2_format detected);

/// Parses checked BA2 GNRL state from a resolved host-file contract and never reopens from raw caller UTF-8 text.
result<ba2_gnrl_archive> parse_ba2_gnrl_archive_file(const detail::host_file_path& host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected);

} // namespace libbsa::formats::ba2
