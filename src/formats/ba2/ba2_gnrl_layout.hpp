#pragma once

#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::ba2 {

/// Assigns BA2 GNRL payload offsets and filename-table offset, optionally
/// reusing duplicate Stored Payloads.
///
/// Deduplication requests fingerprints only when enabled and treats them as
/// candidate keys. Shared placement always requires exact Stored Payload
/// equality.
result<void> ba2_gnrl_assign_payload_offsets(std::span<ba2_gnrl_prepared_entry> entries,
                                             const ba2_profile& profile, bool deduplicate_payloads,
                                             std::uint64_t& file_table_offset);

}  // namespace libbsa::formats::ba2
