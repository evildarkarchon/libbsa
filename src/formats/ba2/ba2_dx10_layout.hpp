#pragma once

#include "formats/ba2/ba2_dx10_prepare.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {

/// Owns one unique DX10 Stored Payload at its format-assigned archive location.
struct ba2_dx10_payload_placement {
    std::uint64_t offset{};
    std::uint64_t stored_size{};
    detail::stored_payload payload;
};

/// Retains one DX10 chunk's decode facts and stable physical payload reference.
struct ba2_dx10_placed_chunk {
    std::uint32_t packed_size{};
    std::uint32_t raw_size{};
    std::uint16_t start_mip{};
    std::uint16_t end_mip{};
    detail::compression_method compression{};
    std::size_t payload_index{};
};

/// Owns one ordered DX10 record and its entry-local placed chunk geometry.
struct ba2_dx10_placed_record {
    std::string archive_path_original;
    std::array<std::byte, 4> extension{};
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::uint8_t unknown_tex{};
    std::uint8_t chunk_count{};
    std::uint16_t height{};
    std::uint16_t width{};
    std::uint8_t mip_count{};
    std::uint8_t dxgi_format{};
    std::uint16_t cube_maps_raw{};
    std::vector<ba2_dx10_placed_chunk> chunks;
};

/// Owns DX10 records, unique Stored Payloads in emission order, and archive geometry.
///
/// Chunk payload indices remain stable for the plan lifetime. The plan is the
/// sole authority for sharing and assigned offsets; profile and stored header
/// options remain separate writer inputs.
struct ba2_dx10_placement_plan {
    std::vector<ba2_dx10_placed_record> records;
    std::vector<ba2_dx10_payload_placement> payloads;
    std::uint64_t filename_table_offset{};
};

/// Moves prepared DX10 entries into a format-owned placement plan.
///
/// Canonical entry and entry-local chunk order select the first compatible
/// payload representative. Fingerprints narrow candidates only when
/// deduplication is enabled; exact Stored Payload equality authorizes sharing.
///
/// Sharing additionally requires that the two chunks' records agree on raw size,
/// packed size and compression method. DX10 supplies those family-owned facts
/// through constrained Payload Placement, which retains facts beside accepted
/// unique candidates and compares accepted facts first with offered facts second.
/// The rule narrows candidates before exact Stored Payload equality authorizes a
/// share; facts are not part of the bucketing key and are never exposed in the
/// released plan (ADR-0001).
result<ba2_dx10_placement_plan> ba2_dx10_plan_placements(
    std::vector<ba2_dx10_prepared_entry> entries, const ba2_profile& profile,
    bool deduplicate_payloads);

}  // namespace libbsa::formats::ba2
