#pragma once

#include "formats/ba2/ba2_dx10_prepare.hpp"

#include <cstdint>
#include <span>

namespace libbsa::formats::ba2 {

/// Assigns BA2 DX10 filename-table and chunk payload offsets.
///
/// When deduplication is enabled, Stored Payload fingerprints and DX10 decode
/// facts narrow candidates; only exact Stored Payload equality authorizes a
/// shared offset. Disabled deduplication does not request fingerprints.
result<void> ba2_dx10_assign_payload_offsets(std::span<ba2_dx10_prepared_entry> entries,
                                             const ba2_profile& profile, bool deduplicate_payloads,
                                             std::uint64_t& file_table_offset);

}  // namespace libbsa::formats::ba2
