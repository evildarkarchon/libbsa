#pragma once

#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::ba2 {

/// Returns the BA2 GNRL archive version for a target profile.
std::uint32_t ba2_gnrl_version_for(ba2_gnrl_target target) noexcept;

/// Returns the BA2 GNRL header byte size for the archive version.
std::size_t ba2_gnrl_header_size_for(std::uint32_t version) noexcept;

/// Compares two prepared BA2 GNRL payloads using the exact stored byte stream.
result<bool> ba2_gnrl_payloads_equal(const ba2_gnrl_prepared_entry& lhs, const ba2_gnrl_prepared_entry& rhs);

/// Assigns BA2 GNRL payload offsets and filename-table offset, optionally reusing duplicate payloads.
result<void> ba2_gnrl_assign_payload_offsets(std::span<ba2_gnrl_prepared_entry> entries,
                                             std::uint32_t version,
                                             bool deduplicate_payloads,
                                             std::uint64_t& file_table_offset);

} // namespace libbsa::formats::ba2
