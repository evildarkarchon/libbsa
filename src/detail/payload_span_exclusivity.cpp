#include <detail/payload_span_exclusivity.hpp>

#include <detail/parser_primitives.hpp>

#include <iterator>
#include <string>

namespace libbsa::detail {

bool payload_span_exclusivity::span_order::operator()(
    const payload_span_exclusivity::payload_span& lhs,
    const payload_span_exclusivity::payload_span& rhs) const noexcept {
    if (lhs.offset != rhs.offset) {
        return lhs.offset < rhs.offset;
    }
    return lhs.size < rhs.size;
}

result<void> payload_span_exclusivity::insert(std::uint64_t offset, std::uint64_t size,
                                              std::string_view overlap_message) {
    if (size == 0U) {
        // An empty span overlaps nothing, so accepting it is correct -- but
        // storing it would seat a member between two real spans that
        // spans_overlap_u64 can never report against, and the neighbour-only
        // comparison would then look past a genuine partial overlap. Callers
        // exclude zero-size stored payloads before they get here; this keeps
        // the collection's disjointness invariant true regardless.
        return {};
    }

    const payload_span_exclusivity::payload_span candidate{offset, size};
    const auto successor = spans_.lower_bound(candidate);

    // Exact duplicates collapse onto the existing representative. Writer dedupe
    // can intentionally publish byte-identical stored spans (ADR-0001), and
    // keeping the collection distinct is what makes the neighbour comparison
    // below sound.
    if (successor != spans_.end() && successor->offset == offset && successor->size == size) {
        return {};
    }

    // The stored members are pairwise disjoint, so a candidate that overlaps any
    // member overlaps its immediate predecessor or successor in offset order.
    // Overlap arithmetic stays in spans_overlap_u64 rather than being
    // open-coded here, which preserves its saturating end computation and keeps
    // spans near the 64-bit maximum judged correctly instead of wrapping.
    if (successor != spans_.end() &&
        spans_overlap_u64(successor->offset, successor->size, offset, size)) {
        return error{error_code::format_error, std::string{overlap_message}};
    }
    if (successor != spans_.begin()) {
        const auto predecessor = std::prev(successor);
        if (spans_overlap_u64(predecessor->offset, predecessor->size, offset, size)) {
            return error{error_code::format_error, std::string{overlap_message}};
        }
    }

    // `successor` already names the element the candidate must precede, which is
    // exactly the hint std::set::insert wants for an amortised constant splice.
    spans_.insert(successor, candidate);
    return {};
}

std::size_t payload_span_exclusivity::distinct_span_count() const noexcept { return spans_.size(); }

}  // namespace libbsa::detail
