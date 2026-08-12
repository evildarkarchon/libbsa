#include "formats/ba2/ba2_dx10_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {

namespace {

struct dedupe_key {
    std::uint64_t stored_size{};
    std::uint64_t fingerprint{};
    std::uint32_t raw_size{};
    std::uint32_t packed_size{};
    detail::compression_method compression{};

    bool operator<(const dedupe_key& other) const noexcept {
        if (stored_size != other.stored_size) {
            return stored_size < other.stored_size;
        }
        if (fingerprint != other.fingerprint) {
            return fingerprint < other.fingerprint;
        }
        if (raw_size != other.raw_size) {
            return raw_size < other.raw_size;
        }
        if (packed_size != other.packed_size) {
            return packed_size < other.packed_size;
        }
        return static_cast<int>(compression) < static_cast<int>(other.compression);
    }
};

bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
    if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
        return false;
    }
    total = lhs + rhs;
    return true;
}

bool multiply_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& product) noexcept {
    if (lhs != 0U && rhs > std::numeric_limits<std::uint64_t>::max() / lhs) {
        return false;
    }
    product = lhs * rhs;
    return true;
}

result<void> validate_entry_shape(const ba2_dx10_prepared_entry& entry) {
    if (entry.chunks.empty()) {
        return error{error_code::format_error, "BA2 DX10 record has no texture chunks"};
    }
    if (entry.chunks.size() > std::numeric_limits<std::uint8_t>::max()) {
        return error{error_code::format_error, "BA2 DX10 chunk count exceeds UInt8 range"};
    }
    if (entry.chunk_count != entry.chunks.size()) {
        return error{error_code::format_error,
                     "BA2 DX10 prepared chunk count does not match record metadata"};
    }
    if (entry.archive_path_original.size() > std::numeric_limits<std::uint16_t>::max()) {
        return error{error_code::format_error,
                     "BA2 DX10 filename-table entry length exceeds UInt16 range"};
    }
    return {};
}

}  // namespace

result<ba2_dx10_placement_plan> ba2_dx10_plan_placements(
    std::vector<ba2_dx10_prepared_entry> entries, const ba2_profile& profile,
    bool deduplicate_payloads) {
    if (!profile.is_dx10()) {
        return error{error_code::invalid_argument, "BA2 DX10 layout profile is not DX10"};
    }
    if (entries.size() > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error, "BA2 DX10 file count exceeds UInt32 range"};
    }

    std::uint64_t record_bytes = 0;
    std::size_t total_chunks = 0;
    for (const auto& entry : entries) {
        auto valid = validate_entry_shape(entry);
        if (!valid) {
            return valid.error();
        }
        std::uint64_t chunk_bytes = 0;
        if (!multiply_fits_u64(entry.chunks.size(), ba2_dx10_chunk_header_size, chunk_bytes)) {
            return error{error_code::format_error, "BA2 DX10 chunk table size overflows"};
        }
        std::uint64_t entry_record_bytes = 0;
        if (!add_fits_u64(ba2_dx10_record_size, chunk_bytes, entry_record_bytes) ||
            !add_fits_u64(record_bytes, entry_record_bytes, record_bytes)) {
            return error{error_code::format_error, "BA2 DX10 record table size overflows"};
        }
        if (total_chunks > std::numeric_limits<std::size_t>::max() - entry.chunks.size()) {
            return error{error_code::format_error, "BA2 DX10 total chunk count overflows"};
        }
        total_chunks += entry.chunks.size();
    }

    ba2_dx10_placement_plan plan;
    // Reference order, from TwbBSArchive.Save's baFO4dds/baSFdds branch: header,
    // records, payloads, names. FileTableOffset is assigned from the stream
    // position reached after every payload is packed, so payload placement
    // begins immediately after the record table and the name table is last.
    std::uint64_t cursor = 0U;
    if (!add_fits_u64(profile.header_size(), record_bytes, cursor)) {
        return error{error_code::format_error, "BA2 DX10 metadata size overflows"};
    }

    plan.records.reserve(entries.size());
    plan.payloads.reserve(total_chunks);
    std::map<dedupe_key, std::vector<std::size_t>> candidate_buckets;
    for (auto& entry : entries) {
        ba2_dx10_placed_record record{
            std::move(entry.archive_path_original),
            entry.extension,
            entry.name_hash,
            entry.directory_hash,
            entry.unknown_tex,
            entry.chunk_count,
            entry.height,
            entry.width,
            entry.mip_count,
            entry.dxgi_format,
            entry.cube_maps_raw,
            {},
        };
        record.chunks.reserve(entry.chunks.size());

        for (auto& chunk : entry.chunks) {
            if (chunk.payload.size() == 0U) {
                return error{error_code::format_error,
                             "BA2 DX10 writer refuses to place an empty texture chunk"};
            }

            std::size_t payload_index = plan.payloads.size();
            std::optional<dedupe_key> identity;
            if (deduplicate_payloads) {
                // Decode facts join immutable byte facts in the candidate key
                // because equal stored bytes are not shareable under different
                // decompression contracts.
                identity.emplace(dedupe_key{chunk.payload.size(), chunk.payload.fingerprint(),
                                            chunk.raw_size, chunk.packed_size, chunk.compression});
                const auto bucket = candidate_buckets.find(*identity);
                if (bucket != candidate_buckets.end()) {
                    for (const auto candidate_index : bucket->second) {
                        // Fingerprints never establish equality; the Stored
                        // Payload comparison remains the sharing authority.
                        auto equal =
                            chunk.payload.exactly_equals(plan.payloads[candidate_index].payload);
                        if (!equal) {
                            return equal.error();
                        }
                        if (equal.value()) {
                            payload_index = candidate_index;
                            break;
                        }
                    }
                }
            }

            if (payload_index == plan.payloads.size()) {
                const auto stored_size = chunk.payload.size();
                plan.payloads.push_back(
                    ba2_dx10_payload_placement{cursor, stored_size, std::move(chunk.payload)});
                if (identity.has_value()) {
                    candidate_buckets[*identity].push_back(payload_index);
                }
                if (!add_fits_u64(cursor, stored_size, cursor)) {
                    return error{error_code::format_error, "BA2 DX10 payload span overflows"};
                }
            }

            record.chunks.push_back(ba2_dx10_placed_chunk{
                chunk.packed_size,
                chunk.raw_size,
                chunk.start_mip,
                chunk.end_mip,
                chunk.compression,
                payload_index,
            });
        }

        // Preparation establishes canonical record order. Appending records and
        // new placements preserves first occurrence as representative and
        // physical emission authority.
        plan.records.push_back(std::move(record));
    }

    // The filename table starts where the payload area ends. Names are read from
    // the placed records because `entries` has had its paths moved out by now.
    plan.filename_table_offset = cursor;
    for (const auto& record : plan.records) {
        std::uint64_t name_bytes = 0;
        if (!add_fits_u64(2U, record.archive_path_original.size(), name_bytes) ||
            !add_fits_u64(cursor, name_bytes, cursor)) {
            return error{error_code::format_error, "BA2 DX10 filename table size overflows"};
        }
    }
    return plan;
}

}  // namespace libbsa::formats::ba2
