#pragma once

#include "formats/bsa/tes4_bsa_layout.hpp"

#include <cstdint>
#include <filesystem>
#include <span>

namespace libbsa::formats::bsa {

/// Writes prepared TES4 BSA archive bytes to the temporary output path supplied
/// by the publish helper.
result<void> tes4_write_archive_bytes(std::span<const tes4_prepared_folder> folders,
                                      std::uint32_t version, bool archive_default_is_compressed,
                                      bool emit_embedded_names, std::uint32_t file_flags,
                                      const tes4_layout_result& layout,
                                      const std::filesystem::path& output_path);

}  // namespace libbsa::formats::bsa
