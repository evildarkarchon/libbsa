#include "formats/ba2/ba2_dx10_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {

namespace {

struct payload_assignment {
    std::uint64_t offset{};
    const detail::stored_payload* payload{};
};

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

}  // namespace

result<void> ba2_dx10_assign_payload_offsets(std::span<ba2_dx10_prepared_entry> entries,
                                             const ba2_profile& profile, bool deduplicate_payloads,
                                             std::uint64_t& file_table_offset) {
    if (!profile.is_dx10()) {
        return error{error_code::invalid_argument, "BA2 DX10 layout profile is not DX10"};
    }

    std::uint64_t record_bytes = 0;
    for (const auto& entry : entries) {
        std::uint64_t entry_record_bytes = 0;
        if (!add_fits_u64(
                ba2_dx10_record_size,
                static_cast<std::uint64_t>(entry.chunks.size()) * ba2_dx10_chunk_header_size,
                entry_record_bytes) ||
            !add_fits_u64(record_bytes, entry_record_bytes, record_bytes)) {
            return error{error_code::format_error, "BA2 DX10 record table size overflows"};
        }
    }

    if (!add_fits_u64(profile.header_size(), record_bytes, file_table_offset)) {
        return error{error_code::format_error, "BA2 DX10 metadata size overflows"};
    }

    std::uint64_t cursor = file_table_offset;
    for (const auto& entry : entries) {
        std::uint64_t name_bytes = 0;
        if (!add_fits_u64(2U, entry.archive_path_original.size(), name_bytes) ||
            !add_fits_u64(cursor, name_bytes, cursor)) {
            return error{error_code::format_error, "BA2 DX10 filename table size overflows"};
        }
    }

    std::map<dedupe_key, std::vector<payload_assignment>> candidate_buckets;
    for (auto& entry : entries) {
        for (auto& chunk : entry.chunks) {
            chunk.is_payload_representative = true;
            std::optional<dedupe_key> identity;
            if (deduplicate_payloads) {
                // Size, fingerprint, and decode facts only narrow candidates.
                // Sharing still requires authoritative exact Stored Payload
                // equality so fingerprint collisions cannot corrupt an archive.
                identity.emplace(dedupe_key{chunk.payload.size(), chunk.payload.fingerprint(),
                                            chunk.raw_size, chunk.packed_size, chunk.compression});
                const auto bucket = candidate_buckets.find(*identity);
                if (bucket != candidate_buckets.end()) {
                    for (const auto& candidate : bucket->second) {
                        auto equal = chunk.payload.exactly_equals(*candidate.payload);
                        if (!equal) {
                            return equal.error();
                        }
                        if (equal.value()) {
                            chunk.payload_offset = candidate.offset;
                            chunk.is_payload_representative = false;
                            break;
                        }
                    }
                    if (!chunk.is_payload_representative) {
                        continue;
                    }
                }
            }
            chunk.payload_offset = cursor;
            if (identity.has_value()) {
                // Layout never changes the fully materialized entry/chunk
                // vectors, so representative payload addresses remain stable
                // for the duration of this candidate search.
                candidate_buckets[*identity].push_back(payload_assignment{cursor, &chunk.payload});
            }
            if (!add_fits_u64(cursor, chunk.payload.size(), cursor)) {
                return error{error_code::format_error, "BA2 DX10 payload span overflows"};
            }
        }
    }
    return {};
}

}  // namespace libbsa::formats::ba2
