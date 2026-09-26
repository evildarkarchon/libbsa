#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <set>
#include <string_view>

namespace libbsa::detail {

/// Enforces Payload Span Exclusivity incrementally as an archive is opened.
///
/// The collection owns the **distinct** spans accepted so far, ordered by
/// offset. Each attempt compares the candidate against only its immediate
/// predecessor and successor: because the stored members are already pairwise
/// disjoint, a span that overlaps any member necessarily overlaps one of those
/// two neighbours. That is what turns the former per-record linear scan into an
/// O(log n) probe and archive opening into O(n log n) in entries (ADR-0002).
///
/// Storing distinct spans is what makes the neighbour comparison sound. Exact
/// duplicates are legal — Payload Placement may deliberately share one location
/// between records (ADR-0001) — so a duplicate collapses onto the existing
/// representative instead of being stored again. Were duplicate runs kept, the
/// immediate predecessor of a candidate could be a duplicate rather than the
/// nearest distinct span, and a partial overlap hiding behind the run would be
/// missed.
///
/// The type carries no family knowledge: callers supply the diagnostic wording
/// so each archive family keeps its own message verbatim.
class payload_span_exclusivity final {
   public:
    /// Attempts to accept one Stored Payload span into the distinct collection.
    ///
    /// Returns an empty success when the span is disjoint from every accepted
    /// span, or when it is byte-identical to one already accepted — in which
    /// case the distinct set does not grow. Returns a `format_error` carrying
    /// `overlap_message` verbatim when the span partially overlaps an accepted
    /// span.
    ///
    /// Empty spans are accepted without being stored; see the definition for
    /// why storing one would break the neighbour comparison.
    ///
    /// Growing the collection may throw `std::bad_alloc`. Parsers already wrap
    /// entry materialisation in the family-scoped allocation boundary that
    /// translates it, so the failure keeps family-correct wording.
    result<void> insert(std::uint64_t offset, std::uint64_t size, std::string_view overlap_message);

    /// Returns how many distinct spans have been accepted so far.
    [[nodiscard]] std::size_t distinct_span_count() const noexcept;

   private:
    /// One archive-relative Stored Payload byte span, as stored in the
    /// collection. Deliberately not part of the interface: callers already hold
    /// loose offset/size values, and the sibling span predicates in
    /// `parser_primitives.hpp` take the same pair.
    struct payload_span {
        std::uint64_t offset;
        std::uint64_t size;
    };

    /// Orders spans by offset, then by size.
    ///
    /// Size participates so that two spans sharing an offset stay distinct.
    /// Ordering on offset alone would make `(100, 10)` and `(100, 20)` compare
    /// equal, and the second — a genuine partial overlap — would be silently
    /// treated as a duplicate.
    struct span_order {
        bool operator()(const payload_span& lhs, const payload_span& rhs) const noexcept;
    };

    std::set<payload_span, span_order> spans_;
};

}  // namespace libbsa::detail
