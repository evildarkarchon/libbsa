#include "formats/ba2/ba2_gnrl_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <limits>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

namespace {

struct payload_assignment {
    std::uint64_t offset{};
    std::size_t entry_index{};
};

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

result<void> ba2_gnrl_assign_payload_offsets(std::span<ba2_gnrl_prepared_entry> entries,
                                             const ba2_profile& profile, bool deduplicate_payloads,
                                             std::uint64_t& file_table_offset) {
    if (!profile.is_gnrl()) {
        return error{error_code::invalid_argument, "BA2 GNRL layout profile is not GNRL"};
    }

    std::uint64_t record_bytes = 0;
    if (entries.size() > std::numeric_limits<std::uint64_t>::max() / ba2_gnrl_record_size) {
        return error{error_code::format_error, "BA2 GNRL record table size overflows"};
    }
    record_bytes = static_cast<std::uint64_t>(entries.size()) * ba2_gnrl_record_size;

    std::uint64_t cursor = 0;
    if (!add_fits_u64(profile.header_size(), record_bytes, cursor)) {
        return error{error_code::format_error, "BA2 GNRL metadata size overflows"};
    }
    const auto first_payload_offset = cursor;

    std::map<ba2_gnrl_dedupe_identity, std::vector<payload_assignment>> candidate_buckets;
    for (std::size_t index = 0; index < entries.size(); ++index) {
        auto& entry = entries[index];
        auto stored_size = checked_u32(entry.payload.size(), "BA2 GNRL stored payload size");
        if (!stored_size) {
            return stored_size.error();
        }
        if (deduplicate_payloads) {
            const ba2_gnrl_dedupe_identity identity{stored_size.value(),
                                                    entry.payload.fingerprint()};
            auto duplicate = candidate_buckets.find(identity);
            bool reused_payload = false;
            if (duplicate != candidate_buckets.end()) {
                for (const auto& candidate : duplicate->second) {
                    // Fingerprints narrow candidates only; authoritative sharing
                    // always compares the exact Stored Payload byte sequence.
                    auto equal =
                        entry.payload.exactly_equals(entries[candidate.entry_index].payload);
                    if (!equal) {
                        return equal.error();
                    }
                    if (equal.value()) {
                        entry.payload_offset = candidate.offset;
                        entry.is_payload_representative = false;
                        reused_payload = true;
                        break;
                    }
                }
            }
            if (reused_payload) {
                continue;
            }
        }

        // Empty BA2 GNRL entries do not own a physical payload span. Point them at
        // the first payload byte so FileTableOffset remains after every real
        // payload while readers validate the zero-length span safely.
        entry.payload_offset = stored_size.value() == 0U ? first_payload_offset : cursor;
        entry.is_payload_representative = true;
        if (deduplicate_payloads) {
            const ba2_gnrl_dedupe_identity identity{stored_size.value(),
                                                    entry.payload.fingerprint()};
            candidate_buckets[identity].push_back(payload_assignment{entry.payload_offset, index});
        }
        if (!add_fits_u64(cursor, stored_size.value(), cursor)) {
            return error{error_code::format_error, "BA2 GNRL payload span overflows"};
        }
    }

    file_table_offset = cursor;
    return {};
}

}  // namespace libbsa::formats::ba2
