#include <detail/payload_placement.hpp>

#include <detail/parser_primitives.hpp>

#include <stdexcept>
#include <utility>

namespace libbsa::detail {

payload_placement_subject payload_placement_subject::of_payload(stored_payload payload) noexcept {
    return payload_placement_subject{
        std::variant<stored_payload, std::uint64_t>{std::move(payload)}};
}

payload_placement_subject payload_placement_subject::of_size(std::uint64_t size) noexcept {
    return payload_placement_subject{std::variant<stored_payload, std::uint64_t>{size}};
}

payload_placement_subject::payload_placement_subject(
    std::variant<stored_payload, std::uint64_t> held) noexcept
    : held_(std::move(held)) {}

const stored_payload* payload_placement_subject::payload() const noexcept {
    return std::get_if<stored_payload>(&held_);
}

std::uint64_t payload_placement_subject::size() const noexcept {
    const auto* offered = payload();
    return offered != nullptr ? offered->size() : std::get<std::uint64_t>(held_);
}

bool payload_placement_subject::has_payload() const noexcept { return payload() != nullptr; }

stored_payload payload_placement_subject::take_payload() {
    auto* offered = std::get_if<stored_payload>(&held_);
    if (offered == nullptr) {
        throw std::logic_error("libbsa::detail::payload_placement_subject holds no payload");
    }
    return std::move(*offered);
}

payload_placer::payload_placer(std::uint64_t base_offset, payload_sharing_policy sharing_policy,
                               std::string_view diagnostic_label)
    : cursor_(base_offset), sharing_policy_(sharing_policy), diagnostic_label_(diagnostic_label) {}

result<payload_placement> payload_placer::place(const payload_narrowing_key& key,
                                                payload_placement_subject subject,
                                                const payload_sharing_eligibility& eligible) {
    if (released_) {
        throw std::logic_error("libbsa::detail::payload_placer placed after release");
    }

    // A size-only subject can never share, and not merely because the families
    // that offer one run with sharing disabled. Exact Stored Payload byte
    // equality is the only authority for sharing (ADR-0001), and a subject
    // without bytes cannot supply that proof under any policy.
    const auto* offered = subject.payload();
    if (sharing_policy_ == payload_sharing_policy::enabled && offered != nullptr) {
        const auto bucket = candidate_buckets_.find(key);
        if (bucket != candidate_buckets_.end()) {
            // Candidates are held in acceptance order and the first that
            // satisfies every condition wins, which is what keeps the earliest
            // accepted payload the sharing representative.
            for (const auto candidate_index : bucket->second) {
                // Sharing Eligibility is checked before byte comparison because
                // it is a cheap family-side field test while `exactly_equals`
                // streams both payloads through a bounded scratch buffer, and
                // for snapshot-backed payloads that means disk reads. Both
                // conditions must hold for a share, so this ordering changes
                // only what the answer costs, never which candidate wins.
                //
                // An empty predicate means the family declared no eligibility
                // constraint, so every candidate passes.
                if (eligible && !eligible(candidate_index)) {
                    continue;
                }

                auto equal = offered->exactly_equals(payloads_[candidate_index].payload);
                if (!equal) {
                    return equal.error();
                }
                if (equal.value()) {
                    return payload_placement{payloads_[candidate_index].offset, candidate_index,
                                             true};
                }
            }
        }
    }

    return place_at_cursor(key, std::move(subject));
}

std::uint64_t payload_placer::cursor() const noexcept { return cursor_; }

std::size_t payload_placer::placed_payload_count() const noexcept { return payloads_.size(); }

std::vector<placed_payload> payload_placer::release() && {
    auto released = std::move(payloads_);
    payloads_.clear();
    candidate_buckets_.clear();
    released_ = true;
    return released;
}

result<payload_placement> payload_placer::place_at_cursor(const payload_narrowing_key& key,
                                                          payload_placement_subject subject) {
    // The placement takes the cursor as it stands and the cursor only then
    // advances by the stored size, so a zero-length subject leaves the cursor
    // where it is and shares an offset with whatever is placed next — another
    // payload, or the filename table when nothing follows. This matches
    // `TwbBSArchive.PackData`, which records `Offset := Position`
    // unconditionally and never special-cases an empty write.
    const std::uint64_t offset = cursor_;
    const std::uint64_t stored_size = subject.size();

    std::uint64_t advanced = 0U;
    if (!add_fits_u64(offset, stored_size, advanced)) {
        return error{error_code::format_error, diagnostic_label_ + " payload span overflows"};
    }

    payload_placement placement{offset, std::nullopt, false};
    if (subject.has_payload()) {
        const std::size_t payload_index = payloads_.size();
        payloads_.push_back(placed_payload{offset, subject.take_payload()});
        // Registering the candidate here, in the single place a payload is ever
        // accepted, is what makes the sharing index impossible to bypass. A
        // caller has no other route to add a payload the index would not see.
        //
        // A disabled policy never consults the buckets, so it does not build
        // them either. That keeps the default dedupe-off path free of a map
        // insertion per archive entry without changing any answer the index
        // could give, because the only reader is guarded by the same condition.
        if (sharing_policy_ == payload_sharing_policy::enabled) {
            candidate_buckets_[key].push_back(payload_index);
        }
        placement.payload_index = payload_index;
    }

    cursor_ = advanced;
    return placement;
}

}  // namespace libbsa::detail
