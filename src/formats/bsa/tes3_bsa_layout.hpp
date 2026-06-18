#pragma once

#include "formats/bsa/tes3_bsa_prepare.hpp"

#include <span>

namespace libbsa::formats::bsa {

/// Assigns TES3 data-section-relative raw offsets to prepared entries.
result<void> tes3_assign_raw_offsets(std::span<tes3_prepared_entry> entries);

}  // namespace libbsa::formats::bsa
