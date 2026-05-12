#pragma once

#include "formats/bsa/tes4_bsa_prepare.hpp"

#include <cstdint>
#include <span>

namespace libbsa::formats::bsa {

struct tes4_layout_result {
  std::uint32_t total_folder_name_length{0};
  std::uint32_t total_file_name_length{0};
  std::uint32_t file_count{0};
};

/// Compares two prepared TES4 stored payload streams, materializing raw disk entries only when needed.
result<bool> tes4_stored_payloads_equal(const tes4_prepared_entry& lhs, const tes4_prepared_entry& rhs);

/// Assigns TES4 folder and payload offsets and returns the table-size values used by serialization.
result<tes4_layout_result> tes4_assign_offsets(std::span<tes4_prepared_folder> folders,
                                               std::uint32_t version,
                                               bool deduplicate_payloads);

} // namespace libbsa::formats::bsa
