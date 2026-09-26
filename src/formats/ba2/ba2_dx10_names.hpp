#pragma once

#include "formats/ba2/ba2_archive_source.hpp"

#include <libbsa/result.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {

/// Reads exactly `file_count` length-prefixed DX10 names through the stable source.
///
/// The table is count-delimited, so `archive_size` is only a truncation guard.
/// Bounding it by the first payload offset instead would reject the reference
/// layout, where BSArchPro writes the filename table after every payload.
/// Overlap between the names and the payload area is checked afterwards against
/// the measured table extent.
///
/// The returned `table_end` is the archive-absolute byte after the last encoded
/// name; trailing padding is deliberately not materialized.
[[nodiscard]] result<std::vector<std::string>> read_ba2_dx10_names(const ba2_archive_source& source,
                                                                   std::uint64_t file_table_offset,
                                                                   std::uint64_t archive_size,
                                                                   std::uint32_t file_count,
                                                                   std::uint64_t& table_end);

}  // namespace libbsa::formats::ba2
