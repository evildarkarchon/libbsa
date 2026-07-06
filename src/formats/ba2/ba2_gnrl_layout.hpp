#pragma once

#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::ba2 {

/// Compares two prepared BA2 GNRL payloads using the exact stored byte stream.
result<bool> ba2_gnrl_payloads_equal(const ba2_gnrl_prepared_entry& lhs,
                                     const ba2_gnrl_prepared_entry& rhs);

/// Assigns BA2 GNRL payload offsets and filename-table offset, optionally
/// reusing duplicate payloads.
result<void> ba2_gnrl_assign_payload_offsets(std::span<ba2_gnrl_prepared_entry> entries,
                                             const ba2_profile& profile, bool deduplicate_payloads,
                                             std::uint64_t& file_table_offset);

}  // namespace libbsa::formats::ba2
