#pragma once

#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <filesystem>

namespace libbsa::formats::ba2 {

/// Writes a BA2 GNRL placement plan to the temporary output path supplied by
/// the publish helper.
///
/// `options` supplies version-gated raw header fields without placing per-archive
/// metadata in the reusable BA2 Profile.
result<void> ba2_gnrl_write_archive_bytes(const ba2_profile& profile,
                                          const ba2_gnrl_writer_options& options,
                                          const ba2_gnrl_placement_plan& plan,
                                          const std::filesystem::path& output_path);

}  // namespace libbsa::formats::ba2
