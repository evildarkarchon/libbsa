#pragma once

#include "formats/bsa/tes4_bsa_layout.hpp"

#include <cstdint>
#include <filesystem>
#include <span>

namespace libbsa::formats::bsa {

/// Writes prepared TES4 BSA archive bytes using policy from the resolved
/// profile and archive-specific writer options.
///
/// The serializer retains ownership of byte emission, offset narrowing, and
/// payload streaming; the profile supplies version-dependent decisions only.
result<void> tes4_write_archive_bytes(std::span<const tes4_prepared_folder> folders,
                                      const tes4_bsa_profile& profile,
                                      const tes4_bsa_writer_options& options,
                                      std::uint32_t file_flags, const tes4_layout_result& layout,
                                      const std::filesystem::path& output_path);

}  // namespace libbsa::formats::bsa
