#pragma once

#include "formats/ba2/ba2_gnrl_prepare.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <cstdint>
#include <filesystem>
#include <span>

namespace libbsa::formats::ba2 {

/// Writes prepared BA2 GNRL archive bytes to the temporary output path supplied
/// by the publish helper.
///
/// `options` supplies version-gated raw header fields without placing per-archive
/// metadata in the reusable BA2 Profile.
result<void> ba2_gnrl_write_archive_bytes(const ba2_profile& profile,
                                          const ba2_gnrl_writer_options& options,
                                          std::span<const ba2_gnrl_prepared_entry> entries,
                                          std::uint64_t file_table_offset,
                                          const std::filesystem::path& output_path);

}  // namespace libbsa::formats::ba2
