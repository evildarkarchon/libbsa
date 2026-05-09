#pragma once

#include "formats/ba2/ba2_format_detector.hpp"

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

/// Parsed BA2 DX10 archive metadata and deterministic public texture entry values.
struct ba2_dx10_archive {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
};

/// Parses checked BA2 DX10 header, texture records, chunks, filename table, and metadata state.
result<ba2_dx10_archive> parse_ba2_dx10_archive(std::span<const std::byte> bytes, detected_ba2_format detected);

/// Parses checked BA2 DX10 state from bounded host-file metadata and filename-table reads.
result<ba2_dx10_archive> parse_ba2_dx10_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected);

} // namespace libbsa::formats::ba2
