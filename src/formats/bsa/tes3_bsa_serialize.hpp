#pragma once

#include "formats/bsa/tes3_bsa_prepare.hpp"

#include <filesystem>
#include <span>

namespace libbsa::formats::bsa
{

    /// Writes the prepared TES3 archive bytes to the temporary output path supplied by the publish helper.
    result<void> tes3_write_archive_bytes(std::span<const tes3_prepared_entry> entries,
                                          const std::filesystem::path &output_path);

} // namespace libbsa::formats::bsa
