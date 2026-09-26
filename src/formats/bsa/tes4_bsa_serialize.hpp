#pragma once

#include "formats/bsa/tes4_bsa_layout.hpp"

#include <cstdint>
#include <filesystem>

namespace libbsa::formats::bsa {

/// Writes TES4 BSA archive bytes from a complete format-owned placement plan.
///
/// Header and record metadata come only from `plan`, and each unique Stored
/// Payload is emitted once in physical plan order without reopening original
/// sources.
result<void> tes4_write_archive_bytes(const tes4_placement_plan& plan,
                                      const std::filesystem::path& output_path);

}  // namespace libbsa::formats::bsa
