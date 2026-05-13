#pragma once

#include "formats/bsa/bsa_format_detector.hpp"

#include <detail/host_file_path.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

/// Parsed TES3 archive metadata and deterministic public entry values.
struct tes3_bsa_archive {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
};

/// Parses checked TES3 BSA header, table, name, hash, and entry metadata state.
result<tes3_bsa_archive> parse_tes3_bsa_archive(std::span<const std::byte> bytes, detected_bsa_format detected);

/// Parses checked TES3 BSA state from a resolved host-file contract and never reopens from raw caller UTF-8 text.
result<tes3_bsa_archive> parse_tes3_bsa_archive_file(const detail::host_file_path& host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected);

} // namespace libbsa::formats::bsa
