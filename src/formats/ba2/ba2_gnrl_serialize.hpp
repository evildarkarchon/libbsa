#pragma once

#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include <cstdint>
#include <filesystem>
#include <span>

namespace libbsa::formats::ba2 {

/// Writes prepared BA2 GNRL archive bytes to the temporary output path supplied
/// by the publish helper.
result<void> ba2_gnrl_write_archive_bytes(ba2_gnrl_target target,
                                          const ba2_gnrl_writer_options& options,
                                          std::span<const ba2_gnrl_prepared_entry> entries,
                                          std::uint32_t version, std::uint64_t file_table_offset,
                                          const std::filesystem::path& output_path);

}  // namespace libbsa::formats::ba2
