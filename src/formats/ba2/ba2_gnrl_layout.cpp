#include "formats/ba2/ba2_gnrl_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/parser_primitives.hpp>
#include <detail/payload_placement.hpp>

#include <limits>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {

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

    // The payload area starts immediately past the header and record table.
    std::uint64_t payload_base_offset = 0;
    if (!detail::add_fits_u64(profile.header_size(), record_bytes, payload_base_offset)) {
        return error{error_code::format_error, "BA2 GNRL metadata size overflows"};
    }

    // Deduplication stays opt-in, so the caller's flag selects the policy rather
    // than this layout deciding it.
    //
    // No Sharing Eligibility predicate is supplied, and the omission is the
    // documentation: a GNRL record carries no decode facts beyond its stored
    // bytes, so differing compression already yields differing stored bytes and
    // exact byte equality refuses the share unaided (ADR-0001). An always-true
    // predicate here would claim a constraint exists.
    detail::payload_placer placer{payload_base_offset,
                                  deduplicate_payloads
                                      ? detail::payload_sharing_policy::enabled
                                      : detail::payload_sharing_policy::disabled,
                                  "BA2 GNRL"};

    ba2_gnrl_placement_plan plan;
    plan.records.reserve(entries.size());
    for (auto& entry : entries) {
        auto& payload = entry.payload;
        // GNRL record widths are fixed: even though Stored Payload size is
        // represented as uint64_t internally, every physical span must fit the
        // serialized UInt32 packed/raw size contract.
        if (payload.size() > std::numeric_limits<std::uint32_t>::max()) {
            return error{error_code::format_error,
                         "BA2 GNRL stored payload size exceeds UInt32 range"};
        }
        const auto stored_size = static_cast<std::uint32_t>(payload.size());

        const detail::payload_narrowing_key key{stored_size, payload.fingerprint()};
        auto placed =
            placer.place(key, detail::payload_placement_subject::of_payload(std::move(payload)));
        if (!placed) {
            return placed.error();
        }

        // Preparation has already established deterministic canonical record
        // order. Appending records in that order, against a placer that keeps
        // its earliest accepted payload the sharing representative, makes the
        // first occurrence the dedupe representative and physical-order
        // authority.
        plan.records.push_back(ba2_gnrl_placed_record{
            std::move(entry.archive_path_original),
            entry.extension,
            entry.name_hash,
            entry.directory_hash,
            entry.record_flags,
            entry.packed_size,
            entry.raw_size,
            placed.value().payload_index.value(),
        });
    }

    // The filename table follows the payload area, so its offset is the cursor
    // as it stands after the final placement. `TwbBSArchive.PackData` records
    // `Offset := Position` unconditionally for baFO4/baSF, where Position is the
    // write cursor sampled before the payload write, and a zero-length write
    // does not move the stream. A trailing empty record therefore shares its
    // offset with the filename table, exactly as the reference produces.
    plan.filename_table_offset = placer.cursor();

    auto accepted_payloads = std::move(placer).release();
    plan.payloads.reserve(accepted_payloads.size());
    for (auto& accepted : accepted_payloads) {
        // Every accepted payload passed the UInt32 range check above before it
        // was placed, so narrowing its size here cannot lose information.
        const auto stored_size = static_cast<std::uint32_t>(accepted.payload.size());
        plan.payloads.push_back(
            ba2_gnrl_payload_placement{accepted.offset, stored_size, std::move(accepted.payload)});
    }

    return plan;
}

}  // namespace libbsa::formats::ba2
