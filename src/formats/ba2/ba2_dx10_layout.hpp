#pragma once

#include "formats/ba2/ba2_dx10_prepare.hpp"

#include <cstdint>
#include <span>

namespace libbsa::formats::ba2
{

    /// Assigns BA2 DX10 filename table and chunk payload offsets, optionally reusing duplicate chunk bytes.
    result<void> ba2_dx10_assign_payload_offsets(std::span<ba2_dx10_prepared_entry> entries,
                                                 std::uint32_t version,
                                                 bool deduplicate_payloads,
                                                 std::uint64_t &file_table_offset);

} // namespace libbsa::formats::ba2
