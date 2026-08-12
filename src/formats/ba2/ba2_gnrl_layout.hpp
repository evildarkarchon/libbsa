#pragma once

#include "formats/ba2/ba2_gnrl_prepare.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {

/// Owns one unique GNRL Stored Payload at its format-assigned archive location.
struct ba2_gnrl_payload_placement {
    std::uint64_t offset{};
    std::uint32_t stored_size{};
    detail::stored_payload payload;
};

/// Retains GNRL record metadata and a reference to its physical payload placement.
struct ba2_gnrl_placed_record {
    std::string archive_path_original;
    std::array<std::byte, 4> extension{};
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::uint32_t record_flags{};
    std::uint32_t packed_size{};
    std::uint32_t raw_size{};
    std::size_t payload_index{};
};

/// Owns GNRL records, unique Stored Payloads in physical order, and archive geometry.
///
/// Record payload indices refer to placements selected by exact Stored Payload
/// equality. A zero-length placement takes the payload cursor as it stands when
/// the record is placed and does not advance it, so it may share an offset with
/// whichever placement follows it — or with the filename table when no payload
/// follows.
struct ba2_gnrl_placement_plan {
    std::vector<ba2_gnrl_placed_record> records;
    std::vector<ba2_gnrl_payload_placement> payloads;
    std::uint64_t filename_table_offset{};
};

/// Moves prepared GNRL Stored Payloads into a format-owned placement plan.
///
/// Deduplication requests fingerprints only when enabled and treats them as
/// candidate keys. Shared placement always requires exact Stored Payload
/// equality.
result<ba2_gnrl_placement_plan> ba2_gnrl_plan_placements(
    std::vector<ba2_gnrl_prepared_entry> entries, const ba2_profile& profile,
    bool deduplicate_payloads);

}  // namespace libbsa::formats::ba2
