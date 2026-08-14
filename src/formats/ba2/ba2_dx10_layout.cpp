#include "formats/ba2/ba2_dx10_layout.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/parser_primitives.hpp>
#include <detail/payload_placement.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {

namespace {

/// The record fields that state how one placed chunk's stored bytes decode.
///
/// These are deliberately not part of the narrowing key. They are a correctness
/// rule, not a bucketing device: two byte-equal chunks whose records declare
/// different decode facts must never share a location, or the loser's record
/// would describe content it cannot produce. Expressing them as Sharing
/// Eligibility keeps DX10's candidate key the same stored-size-and-fingerprint
/// shape as every other sharing family's (ADR-0001, CONTEXT.md).
struct chunk_decode_facts {
    std::uint32_t raw_size{};
    std::uint32_t packed_size{};
    detail::compression_method compression{};

    /// Two chunks are decode-compatible only when all three facts agree.
    bool operator==(const chunk_decode_facts& other) const noexcept = default;
};

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
        if (!detail::add_fits_u64(ba2_dx10_record_size, chunk_bytes, entry_record_bytes) ||
            !detail::add_fits_u64(record_bytes, entry_record_bytes, record_bytes)) {
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
    std::uint64_t payload_base_offset = 0U;
    if (!detail::add_fits_u64(profile.header_size(), record_bytes, payload_base_offset)) {
        return error{error_code::format_error, "BA2 DX10 metadata size overflows"};
    }

    // Deduplication stays opt-in, so the caller's flag selects the policy rather
    // than this layout deciding it.
    detail::payload_placer placer{payload_base_offset,
                                  deduplicate_payloads ? detail::payload_sharing_policy::enabled
                                                       : detail::payload_sharing_policy::disabled,
                                  "BA2 DX10"};

    // Decode facts for every payload the placer has accepted, indexed by the
    // payload index it handed back for that payload. The placer mints indices in
    // strict acceptance order, so appending here on each non-shared placement is
    // what keeps this vector indexed exactly as the placer indexes its own
    // payloads; the eligibility predicate relies on that correspondence.
    std::vector<chunk_decode_facts> accepted_decode_facts;
    accepted_decode_facts.reserve(total_chunks);

    plan.records.reserve(entries.size());
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
            // DX10 is the only family that refuses an empty payload instead of
            // placing one at the cursor, and the rejection stays here, ahead of
            // the placer, because it is a DX10 write-path divergence from the
            // reference rather than a Payload Placement rule (ADR-0001). Moving
            // it into the module would mean giving the module a policy flag to
            // save a single condition in a single caller.
            if (chunk.payload.size() == 0U) {
                return error{error_code::format_error,
                             "BA2 DX10 writer refuses to place an empty texture chunk"};
            }

            const chunk_decode_facts facts{chunk.raw_size, chunk.packed_size, chunk.compression};

            // Sharing Eligibility: the reason DX10's narrowing key can be stored
            // size and fingerprint like every other sharing family's. Byte
            // equality alone does not authorise a share here, because a shared
            // location must also satisfy every record field describing how the
            // payload is decoded.
            //
            // The placer evaluates this before exact byte comparison. Both
            // conditions must hold for a share, so that ordering changes only
            // what the answer costs and never which candidate wins (ADR-0001,
            // CONTEXT.md).
            //
            // `facts` is captured by value so the predicate stays valid after
            // this chunk's payload is moved into the subject below.
            //
            // `at` rather than `operator[]`: the index correspondence below is
            // an invariant this function maintains by hand, and a future edit
            // that returned between the placement and the append would silently
            // read the wrong candidate's facts. A programmer precondition
            // violation is the one case AGENTS.md reserves exceptions for.
            const detail::payload_sharing_eligibility decode_facts_agree =
                [&accepted_decode_facts, facts](std::size_t candidate_payload_index) {
                    return accepted_decode_facts.at(candidate_payload_index) == facts;
                };

            // The fingerprint is requested only under an enabled sharing policy,
            // because that is the only policy under which the placer reads the
            // narrowing key at all. `stored_payload` hashes owned bytes lazily
            // precisely so a caller that skips deduplication does not pay for a
            // scan it will never use, and every DX10 chunk payload is owned
            // bytes, so asking unconditionally would put a full byte-wise hash
            // per chunk on the default dedupe-off path.
            const detail::payload_narrowing_key key{
                chunk.payload.size(),
                deduplicate_payloads ? chunk.payload.fingerprint() : std::uint64_t{0}};
            auto placed = placer.place(
                key, detail::payload_placement_subject::of_payload(std::move(chunk.payload)),
                decode_facts_agree);
            if (!placed) {
                return placed.error();
            }
            const auto payload_index = placed.value().payload_index.value();
            // Only a new payload mints a new index, so only a new payload
            // appends. Appending on a share would push this vector one ahead of
            // the placer's and every later eligibility lookup would read some
            // other chunk's decode facts.
            if (!placed.value().shared) {
                accepted_decode_facts.push_back(facts);
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

        // Preparation establishes canonical record order. Appending records in
        // that order, against a placer that keeps its earliest accepted payload
        // the sharing representative, preserves first occurrence in canonical
        // entry and entry-local chunk order as representative and physical
        // emission authority.
        plan.records.push_back(std::move(record));
    }

    // The filename table starts where the payload area ends, which is the
    // payload cursor as it stands after the final placement.
    plan.filename_table_offset = placer.cursor();

    auto accepted_payloads = std::move(placer).release();
    plan.payloads.reserve(accepted_payloads.size());
    for (auto& accepted : accepted_payloads) {
        const auto stored_size = accepted.payload.size();
        plan.payloads.push_back(
            ba2_dx10_payload_placement{accepted.offset, stored_size, std::move(accepted.payload)});
    }

    // Names are read from the placed records because `entries` has had its paths
    // moved out by now. This walk only proves the name table fits the 64-bit
    // range; its running total is not an output, because the table offset was
    // already taken from the payload cursor above.
    std::uint64_t name_table_cursor = plan.filename_table_offset;
    for (const auto& record : plan.records) {
        std::uint64_t name_bytes = 0;
        if (!detail::add_fits_u64(2U, record.archive_path_original.size(), name_bytes) ||
            !detail::add_fits_u64(name_table_cursor, name_bytes, name_table_cursor)) {
            return error{error_code::format_error, "BA2 DX10 filename table size overflows"};
        }
    }
    return plan;
}

}  // namespace libbsa::formats::ba2
