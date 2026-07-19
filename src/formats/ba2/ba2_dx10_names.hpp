#pragma once

#include "formats/ba2/ba2_archive_source.hpp"

#include <libbsa/result.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {

/// Reads exactly `file_count` length-prefixed DX10 names through the stable source.
///
/// The returned `table_end` is the archive-absolute byte after the last encoded
/// name. Padding before `first_payload_offset` is deliberately not materialized.
[[nodiscard]] result<std::vector<std::string>> read_ba2_dx10_names(
    const ba2_archive_source& source, std::uint64_t file_table_offset,
    std::uint64_t first_payload_offset, std::uint32_t file_count, std::uint64_t& table_end);

}  // namespace libbsa::formats::ba2
