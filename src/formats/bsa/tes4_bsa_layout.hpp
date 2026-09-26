#pragma once

#include "formats/bsa/tes4_bsa_prepare.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace libbsa::formats::bsa {

struct tes4_payload_placement {
    std::uint32_t offset{0};
    std::uint32_t stored_size{0};
    detail::stored_payload payload;
};

struct tes4_placed_entry {
    std::string file_name;
    std::uint64_t file_hash{0};
    std::uint32_t record_flags{0};
    std::size_t payload_index{0};
};

struct tes4_placed_folder {
    std::string name;
    std::uint64_t hash{0};
    std::uint64_t folder_block_offset{0};
    std::vector<tes4_placed_entry> entries;
};

/// Owns TES4 record locations and each unique Stored Payload in physical order.
///
/// Entry indices refer to the representative payload placement chosen by exact
/// equality. Distinct zero-length placements may intentionally share an offset
/// because Bethesda's cursor does not advance when no stored bytes are emitted.
struct tes4_placement_plan {
    std::uint32_t version{0};
    std::uint32_t archive_flags{0};
    std::uint32_t file_flags{0};
    tes4_folder_record_shape folder_record_shape{tes4_folder_record_shape::legacy_32_bit_offset};
    std::uint32_t total_folder_name_length{0};
    std::uint32_t total_file_name_length{0};
    std::uint32_t file_count{0};
    std::vector<tes4_placed_folder> folders;
    std::vector<tes4_payload_placement> payloads;
};

/// Moves prepared Stored Payloads into a format-owned placement plan.
///
/// The resolved profile supplies the version-specific folder record width.
/// Deduplication narrows candidates by stored size and fingerprint, then
/// authorizes sharing only through exact Stored Payload equality.
result<tes4_placement_plan> tes4_plan_placements(std::vector<tes4_prepared_folder> folders,
                                                 const tes4_bsa_profile& profile,
                                                 const tes4_bsa_writer_options& options,
                                                 std::uint32_t file_flags);

}  // namespace libbsa::formats::bsa
