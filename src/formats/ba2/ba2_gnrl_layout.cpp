#include "formats/ba2/ba2_gnrl_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {

namespace {

struct ba2_gnrl_dedupe_identity {
    std::uint32_t stored_size{};
    std::uint64_t fingerprint{};

    bool operator<(const ba2_gnrl_dedupe_identity& other) const noexcept {
        if (stored_size != other.stored_size) {
            return stored_size < other.stored_size;
        }
        return fingerprint < other.fingerprint;
    }
};

bool add_fits_u64(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
    if (lhs > std::numeric_limits<std::uint64_t>::max() - rhs) {
        return false;
    }
    total = lhs + rhs;
    return true;
}

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error, std::string{description} + " exceeds UInt32 range"};
    }
    return static_cast<std::uint32_t>(value);
}

}  // namespace

result<ba2_gnrl_placement_plan> ba2_gnrl_plan_placements(
    std::vector<ba2_gnrl_prepared_entry> entries, const ba2_profile& profile,
    bool deduplicate_payloads) {
    if (!profile.is_gnrl()) {
        return error{error_code::invalid_argument, "BA2 GNRL layout profile is not GNRL"};
    }

    if (entries.size() > std::numeric_limits<std::uint64_t>::max() / ba2_gnrl_record_size) {
        return error{error_code::format_error, "BA2 GNRL record table size overflows"};
    }
    const auto record_bytes = static_cast<std::uint64_t>(entries.size()) * ba2_gnrl_record_size;

    std::uint64_t cursor = 0;
    if (!add_fits_u64(profile.header_size(), record_bytes, cursor)) {
        return error{error_code::format_error, "BA2 GNRL metadata size overflows"};
    }
    const auto first_payload_offset = cursor;

    ba2_gnrl_placement_plan plan;
    plan.records.reserve(entries.size());
    plan.payloads.reserve(entries.size());
    std::map<ba2_gnrl_dedupe_identity, std::vector<std::size_t>> candidate_buckets;
    for (auto& entry : entries) {
        auto& payload = entry.payload;
        // GNRL record widths are fixed: even though Stored Payload size is
        // represented as uint64_t internally, every physical span must fit the
        // serialized UInt32 packed/raw size contract.
        auto stored_size = checked_u32(payload.size(), "BA2 GNRL stored payload size");
        if (!stored_size) {
            return stored_size.error();
        }

        std::size_t payload_index = plan.payloads.size();
        std::optional<ba2_gnrl_dedupe_identity> identity;
        if (deduplicate_payloads) {
            // Size and fingerprint are immutable narrowing facts. Physical
            // sharing still requires authoritative equality below.
            identity.emplace(ba2_gnrl_dedupe_identity{stored_size.value(), payload.fingerprint()});
            const auto bucket = candidate_buckets.find(*identity);
            if (bucket != candidate_buckets.end()) {
                for (const auto candidate_index : bucket->second) {
                    // Fingerprints narrow candidates only; authoritative sharing
                    // always compares the exact Stored Payload byte sequence.
                    auto equal = payload.exactly_equals(plan.payloads[candidate_index].payload);
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
            // Empty GNRL records always point to the first payload location.
            // They emit no bytes and therefore never advance the physical cursor,
            // including when every record in the archive is empty.
            const auto offset = stored_size.value() == 0U ? first_payload_offset : cursor;
            plan.payloads.push_back(
                ba2_gnrl_payload_placement{offset, stored_size.value(), std::move(payload)});
            if (identity.has_value()) {
                candidate_buckets[*identity].push_back(payload_index);
            }
            if (!add_fits_u64(cursor, stored_size.value(), cursor)) {
                return error{error_code::format_error, "BA2 GNRL payload span overflows"};
            }
        }

        // Preparation has already established deterministic canonical record
        // order. Appending records and new placements here makes the first
        // occurrence the dedupe representative and physical-order authority.
        plan.records.push_back(ba2_gnrl_placed_record{
            std::move(entry.archive_path_original),
            entry.extension,
            entry.name_hash,
            entry.directory_hash,
            entry.record_flags,
            entry.packed_size,
            entry.raw_size,
            payload_index,
        });
    }

    plan.filename_table_offset = cursor;
    return plan;
}

}  // namespace libbsa::formats::ba2
