#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {

/// Reads count-delimited BA2 DX10 filename-table bytes from a file-backed
/// archive without materializing payload gaps.
[[nodiscard]] result<std::vector<std::byte>> read_ba2_dx10_names_from_file(
    std::ifstream& input, std::uint64_t file_table_offset, std::uint64_t first_payload_offset,
    std::uint32_t file_count);

/// Decodes a BA2 DX10 filename table and reports the exact byte count consumed
/// by the encoded names.
[[nodiscard]] result<std::vector<std::string>> read_ba2_dx10_names(
    std::span<const std::byte> name_table, std::uint32_t file_count, std::size_t& consumed);

}  // namespace libbsa::formats::ba2
