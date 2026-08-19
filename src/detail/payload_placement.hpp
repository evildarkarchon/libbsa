#pragma once

#include <detail/parser_primitives.hpp>
#include <detail/stored_payload.hpp>

#include <libbsa/result.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace libbsa::detail {

/// Whether a placer may give two records one payload-area location.
///
/// This exists as a named policy rather than a bare `bool` so a family that
/// never shares — TES3 BSA — states that as a decision in code instead of
/// expressing it by having no sharing code at all (ADR-0002 records the open
/// question behind TES3's exclusion).
enum class payload_sharing_policy {
    /// Every placement takes a distinct location, whatever its bytes.
    disabled,
    /// Byte-equal, eligible payloads may collapse onto one location.
    enabled,
};

/// The bucketing facts that narrow which earlier payloads are worth comparing.
///
/// The key is *only* a bucketing device: two payloads with equal keys are
/// candidates for sharing, never proven equal by it. Changing the key alone can
/// therefore change how much comparison work happens, but not which payloads
/// end up sharing a location (ADR-0001). Correctness constraints that must
/// forbid a share belong in a Sharing Eligibility rule instead, which is what
/// keeps that claim true for every archive family.
struct payload_narrowing_key {
    std::uint64_t stored_size{};
    std::uint64_t fingerprint{};

    /// Orders keys so they can bucket in an ordered map; carries no meaning.
    bool operator<(const payload_narrowing_key& other) const noexcept {
        if (stored_size != other.stored_size) {
            return stored_size < other.stored_size;
        }
        return fingerprint < other.fingerprint;
    }
};

/// Legacy callback deciding whether an earlier payload is a legal sharing partner.
///
/// The predicate receives the index of an already-accepted payload — the same
/// index `payload_placement::payload_index` reports — so a family can look the
/// candidate up in its own record table and compare whatever record fields
/// govern decoding. BA2 DX10 uses this for its raw size, packed size and
/// compression method: byte-equal chunks whose records declare different decode
/// facts must not share, or a record would describe content it cannot produce.
///
/// Sharing Eligibility is a precondition, never a proof. Returning `true` does
/// not authorise a share; byte equality still has to hold (CONTEXT.md).
///
/// An empty predicate means "no constraint", which is why families without one
/// omit the argument rather than passing an always-true lambda. This callback is
/// retained only until BA2 DX10 migrates to `constrained_payload_placer`; new
/// constrained callers must not adopt the leaked candidate-index contract.
using payload_sharing_eligibility = std::function<bool(std::size_t candidate_payload_index)>;

/// True when `Rule` is a const, non-throwing pairwise rule returning exactly `bool`.
///
/// The accepted facts are always the first argument and offered facts the
/// second. Requiring invocation through `const Rule&` rejects rules that need a
/// mutable rule object; C++ cannot prove absence of external side effects, so
/// callers retain the semantic obligation that the rule is otherwise pure.
template <typename Rule, typename Facts>
concept payload_sharing_rule_for =
    requires(const Rule& rule, const Facts& accepted, const Facts& offered) {
        { std::invoke(rule, accepted, offered) } noexcept -> std::same_as<bool>;
    };

/// What a family offers the placer for one placement: bytes, or just a length.
///
/// Families that own Stored Payloads move one in and the placer takes ownership
/// of it. TES3 BSA deliberately does not adopt Stored Payload, so it offers a
/// size alone and takes only the cursor and the overflow arithmetic.
///
/// A size-only subject can never share, and not because TES3 happens to run
/// with sharing disabled: byte equality is the sole authority for sharing
/// (ADR-0001), and there are no bytes to compare. The placer enforces that
/// independently of the policy.
class payload_placement_subject final {
   public:
    /// Offers a Stored Payload whose ownership transfers to the placer.
    static payload_placement_subject of_payload(stored_payload payload) noexcept;

    /// Offers a payload length only, for a family without Stored Payload.
    static payload_placement_subject of_size(std::uint64_t size) noexcept;

    /// Subjects carry a move-only Stored Payload, so they are move-only too.
    payload_placement_subject(const payload_placement_subject&) = delete;

    /// Subjects carry a move-only Stored Payload, so they are move-only too.
    payload_placement_subject& operator=(const payload_placement_subject&) = delete;

    payload_placement_subject(payload_placement_subject&&) noexcept = default;
    payload_placement_subject& operator=(payload_placement_subject&&) noexcept = default;
    ~payload_placement_subject() noexcept = default;

    /// Returns the stored byte count this subject will occupy.
    [[nodiscard]] std::uint64_t size() const noexcept;

    /// Returns the offered Stored Payload, or `nullptr` for a size-only subject.
    ///
    /// This is the one place the payload-or-size distinction is resolved; every
    /// other question about a subject is answered from the returned pointer.
    [[nodiscard]] const stored_payload* payload() const noexcept;

    /// Returns true when the subject carries comparable Stored Payload bytes.
    [[nodiscard]] bool has_payload() const noexcept;

    /// Moves the offered Stored Payload out of the subject.
    ///
    /// Precondition: `has_payload()`. Calling this on a size-only subject is a
    /// programmer error and throws `std::logic_error`.
    [[nodiscard]] stored_payload take_payload();

   private:
    explicit payload_placement_subject(std::variant<stored_payload, std::uint64_t> held) noexcept;

    std::variant<stored_payload, std::uint64_t> held_;
};

/// One accepted Stored Payload together with the location it was given.
///
/// This is what `payload_placer::release` hands back, rather than bare payloads.
/// The placer is the only thing that knows a payload's offset — Stored Payload
/// deliberately carries no archive location — so releasing the two together is
/// what stops every archive family from rebuilding a vector of offsets running
/// parallel to the payloads, which is the arrangement this module exists to
/// avoid.
struct placed_payload {
    /// The archive-absolute payload-area offset, always 64-bit. Families that
    /// serialize a narrower field narrow it here, at read-out.
    std::uint64_t offset{};
    stored_payload payload;
};

/// Where one Stored Payload ended up, and whether it reused a location.
struct payload_placement {
    /// The archive-absolute payload-area offset, always 64-bit.
    ///
    /// Families that serialize a narrower field narrow here, at read-out, the
    /// pattern TES4-family layout already uses for its UInt32 offsets.
    std::uint64_t offset{};

    /// Index into the vector `payload_placer::release` yields, when there is
    /// one. Empty for a size-only placement, which contributes no payload.
    std::optional<std::size_t> payload_index{};

    /// True when this placement collapsed onto an earlier payload's location.
    bool shared{};
};

namespace payload_placement_detail {

/// One accepted constrained candidate, keeping its Stored Payload and facts together.
template <typename Facts>
struct constrained_accepted_payload {
    placed_payload placed;
    std::optional<Facts> facts;
};

/// Projects the public placed-payload representation from unconstrained state.
inline const placed_payload& placed_payload_of(const placed_payload& accepted) noexcept {
    return accepted;
}

/// Projects the public placed-payload representation from constrained state.
template <typename Facts>
const placed_payload& placed_payload_of(
    const constrained_accepted_payload<Facts>& accepted) noexcept {
    return accepted.placed;
}

/// Shared private engine for constrained and unconstrained Payload Placement.
template <typename AcceptedPayload>
class payload_placement_engine final {
   public:
    /// Starts a payload area at `base_offset` under one sharing policy.
    payload_placement_engine(std::uint64_t base_offset, payload_sharing_policy sharing_policy,
                             std::string_view diagnostic_label)
        : cursor_(base_offset),
          sharing_policy_(sharing_policy),
          diagnostic_label_(diagnostic_label) {}

    payload_placement_engine(const payload_placement_engine&) = delete;
    payload_placement_engine& operator=(const payload_placement_engine&) = delete;
    payload_placement_engine(payload_placement_engine&&) noexcept = default;
    payload_placement_engine& operator=(payload_placement_engine&&) noexcept = default;
    ~payload_placement_engine() noexcept = default;

    /// Places one subject using the lane's candidate rule and acceptance factory.
    ///
    /// `eligible` receives accepted state and its stable payload index. It runs
    /// before exact comparison. `accept` receives a newly placed payload and
    /// whether candidate state will be retained, allowing the constrained lane
    /// to discard facts when sharing is disabled.
    template <typename Eligible, typename Accept>
    result<payload_placement> place(const payload_narrowing_key& key,
                                    payload_placement_subject subject, Eligible&& eligible,
                                    Accept&& accept) {
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
                    if (!std::invoke(eligible, accepted_payloads_[candidate_index],
                                     candidate_index)) {
                        continue;
                    }

                    auto equal = offered->exactly_equals(
                        placed_payload_of(accepted_payloads_[candidate_index]).payload);
                    if (!equal) {
                        return equal.error();
                    }
                    if (equal.value()) {
                        const auto& candidate =
                            placed_payload_of(accepted_payloads_[candidate_index]);
                        return payload_placement{candidate.offset, candidate_index, true};
                    }
                }
            }
        }

        return place_at_cursor(key, std::move(subject), std::forward<Accept>(accept));
    }

    /// Returns the first offset past the placed payload area.
    [[nodiscard]] std::uint64_t cursor() const noexcept { return cursor_; }

    /// Returns the number of accepted unique Stored Payloads.
    [[nodiscard]] std::size_t placed_payload_count() const noexcept {
        return accepted_payloads_.size();
    }

    /// Returns whether accepted payloads participate in future candidate traversal.
    [[nodiscard]] bool retains_candidates() const noexcept {
        return sharing_policy_ == payload_sharing_policy::enabled;
    }

    /// Releases accepted lane state in acceptance order and invalidates placement.
    [[nodiscard]] std::vector<AcceptedPayload> release() && {
        auto released = std::move(accepted_payloads_);
        accepted_payloads_.clear();
        candidate_buckets_.clear();
        released_ = true;
        return released;
    }

   private:
    /// Appends `subject` at the cursor and advances it, or reports overflow.
    template <typename Accept>
    result<payload_placement> place_at_cursor(const payload_narrowing_key& key,
                                              payload_placement_subject subject, Accept&& accept) {
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
            const std::size_t payload_index = accepted_payloads_.size();
            placed_payload placed{offset, subject.take_payload()};
            accepted_payloads_.push_back(
                std::invoke(std::forward<Accept>(accept), std::move(placed), retains_candidates()));
            // Registering the candidate here, in the single place a payload is ever
            // accepted, is what makes the sharing index impossible to bypass. A
            // caller has no other route to add a payload the index would not see.
            //
            // A disabled policy never consults the buckets, so it does not build
            // them either. That keeps the default dedupe-off path free of a map
            // insertion per archive entry and lets the constrained lane discard
            // facts that no later placement could observe.
            if (retains_candidates()) {
                candidate_buckets_[key].push_back(payload_index);
            }
            placement.payload_index = payload_index;
        }

        cursor_ = advanced;
        return placement;
    }

    std::uint64_t cursor_;
    payload_sharing_policy sharing_policy_;
    std::string diagnostic_label_;

    /// Accepted lane state in placement order; positions are stable payload indices.
    std::vector<AcceptedPayload> accepted_payloads_;

    /// Candidate indices bucketed by narrowing key in acceptance order.
    std::map<payload_narrowing_key, std::vector<std::size_t>> candidate_buckets_;

    /// True once `release` has handed accepted state to its façade.
    bool released_{false};
};

}  // namespace payload_placement_detail

/// Owns Payload Placement for one archive being written.
///
/// The module owns the sharing decision
/// (bucket by a narrowing key, confirm by exact Stored Payload byte equality),
/// the payload cursor including the zero-length rule, overflow-checked payload
/// span arithmetic, the accepted Stored Payloads, and — for the constrained
/// lane — the facts associated with each accepted sharing candidate.
///
/// It deliberately owns the payloads rather than indexing into a family-owned
/// collection. Stored Payload is move-only, and this is the only arrangement in
/// which the placer can enforce its own invariant: under the alternative a
/// family could append a payload without telling the sharing index, and the
/// bucket map would silently go stale.
///
/// It owns none of: record geometry, BA2 DX10's empty-chunk rejection, offset
/// narrowing to family-specific widths, or TES3's data-section relativity.
/// Those stay with their families.
///
/// One placer belongs to one archive write and is not thread-safe; concurrency
/// comes from isolated placers, matching the project's no-shared-state rule.
class payload_placer final {
   public:
    /// Starts a payload area at `base_offset` under one sharing policy.
    ///
    /// `base_offset` is where the family's payload area begins — normally the
    /// byte just past the header and record tables. `diagnostic_label` names
    /// the family in overflow diagnostics ("TES4 BSA", "BA2 DX10"); it is
    /// copied, so callers may pass a temporary.
    payload_placer(std::uint64_t base_offset, payload_sharing_policy sharing_policy,
                   std::string_view diagnostic_label);

    /// Placers own accepted payloads exclusively and cannot be copied.
    payload_placer(const payload_placer&) = delete;

    /// Placers own accepted payloads exclusively and cannot be copied.
    payload_placer& operator=(const payload_placer&) = delete;

    payload_placer(payload_placer&&) noexcept = default;
    payload_placer& operator=(payload_placer&&) noexcept = default;
    ~payload_placer() noexcept = default;

    /// Places one subject, sharing an earlier location when that is authorised.
    ///
    /// Sharing requires all of: the policy is `enabled`, the subject carries
    /// bytes, an already-accepted payload sits in the same narrowing-key
    /// bucket, `eligible` accepts that candidate, and the two Stored Payloads
    /// are exactly byte-equal. Candidates are examined in acceptance order and
    /// the first that satisfies every condition wins, so the earliest accepted
    /// payload stays the sharing representative.
    ///
    /// `eligible` is evaluated *before* byte comparison because comparison
    /// streams through a bounded scratch buffer and is by far the expensive
    /// half. Both conditions must hold for a share, so the order cannot change
    /// which candidate wins — only how much work it costs to find out. An empty
    /// `eligible` imposes no constraint.
    ///
    /// A new location takes the cursor as it stands and then advances it by the
    /// stored size. A zero-length subject therefore takes the cursor without
    /// moving it and shares its offset with whatever is placed next, matching
    /// `TwbBSArchive.PackData`'s unconditional `Offset := Position`.
    ///
    /// Returns a `format_error` naming `diagnostic_label` when advancing the
    /// cursor would leave the 64-bit range, and propagates any I/O error raised
    /// by the Stored Payload comparison.
    ///
    /// Throws `std::logic_error` when called after `release`. That is a
    /// programmer error rather than an archive-data failure: the placer no
    /// longer owns the payloads its indices refer to, so a further placement
    /// would mint an index that aliases a payload the caller already holds.
    result<payload_placement> place(const payload_narrowing_key& key,
                                    payload_placement_subject subject,
                                    const payload_sharing_eligibility& eligible = {});

    /// Returns the cursor, i.e. the first offset past the placed payload area.
    ///
    /// Families that follow the payload area with a filename table read the
    /// table's offset from here once every placement is done.
    [[nodiscard]] std::uint64_t cursor() const noexcept;

    /// Returns how many distinct payload locations have been assigned.
    [[nodiscard]] std::size_t placed_payload_count() const noexcept;

    /// Hands the accepted payloads and their offsets over in placement order.
    ///
    /// Vector positions are the `payload_index` values already handed out, so a
    /// family can serialize by index without a second lookup table, and each
    /// entry carries the offset the placer assigned so no family has to keep its
    /// own record of where a payload went.
    ///
    /// Rvalue-qualified because releasing consumes the placer: the sharing
    /// index refers to payloads it no longer owns afterwards, so continuing to
    /// place against it would be meaningless. Callers write
    /// `std::move(placer).release()`, and a later `place` throws.
    [[nodiscard]] std::vector<placed_payload> release() &&;

   private:
    payload_placement_detail::payload_placement_engine<placed_payload> engine_;
};

/// Owns constrained Payload Placement and its family-defined eligibility facts.
///
/// `Facts` remains family-owned vocabulary, while this module owns one facts
/// value beside every accepted unique Stored Payload whenever sharing is
/// enabled. `Rule` is stored by value and invoked as
/// `rule(accepted_facts, offered_facts)` before exact byte comparison. Facts are
/// mandatory for every `place` call, never exposed by `release`, and destroyed
/// when accepted state is released.
template <typename Facts, typename Rule>
    requires std::move_constructible<Facts> && std::move_constructible<Rule> &&
             payload_sharing_rule_for<Rule, Facts>
class constrained_payload_placer final {
   private:
    using accepted_payload = payload_placement_detail::constrained_accepted_payload<Facts>;

   public:
    /// Starts a constrained payload area and stores the family rule by value.
    constrained_payload_placer(std::uint64_t base_offset, payload_sharing_policy sharing_policy,
                               std::string_view diagnostic_label, Rule rule)
        : engine_(base_offset, sharing_policy, diagnostic_label), rule_(std::move(rule)) {}

    constrained_payload_placer(const constrained_payload_placer&) = delete;
    constrained_payload_placer& operator=(const constrained_payload_placer&) = delete;
    constrained_payload_placer(constrained_payload_placer&&) = default;
    constrained_payload_placer& operator=(constrained_payload_placer&&) = default;
    ~constrained_payload_placer() = default;

    /// Places one subject with mandatory facts for this offered payload.
    ///
    /// The rule sees accepted facts first and offered facts second. Offered
    /// facts move into accepted state only when a new unique Stored Payload can
    /// become a future candidate; shared and sharing-disabled offers retain no
    /// facts. Exact Stored Payload comparison remains authoritative.
    result<payload_placement> place(const payload_narrowing_key& key,
                                    payload_placement_subject subject, Facts offered_facts) {
        const auto eligible = [this, &offered_facts](const accepted_payload& candidate,
                                                     std::size_t) noexcept {
            // Candidate buckets exist only when sharing is enabled, and enabled
            // acceptance always stores facts, so dereferencing cannot observe an
            // empty optional. Avoiding `value()` keeps this infallible rule path
            // statically non-throwing.
            return std::invoke(rule_, *candidate.facts, offered_facts);
        };
        const auto accept = [&offered_facts](placed_payload placed, bool retain_facts) {
            std::optional<Facts> accepted_facts;
            if (retain_facts) {
                accepted_facts.emplace(std::move(offered_facts));
            }
            return accepted_payload{std::move(placed), std::move(accepted_facts)};
        };
        return engine_.place(key, std::move(subject), eligible, accept);
    }

    /// Returns the first offset past the placed payload area.
    [[nodiscard]] std::uint64_t cursor() const noexcept { return engine_.cursor(); }

    /// Returns the number of accepted unique Stored Payloads.
    [[nodiscard]] std::size_t placed_payload_count() const noexcept {
        return engine_.placed_payload_count();
    }

    /// Releases only placed payloads in acceptance order and destroys all facts.
    [[nodiscard]] std::vector<placed_payload> release() && {
        auto accepted = std::move(engine_).release();
        std::vector<placed_payload> released;
        released.reserve(accepted.size());
        for (auto& candidate : accepted) {
            released.push_back(std::move(candidate.placed));
        }
        return released;
    }

   private:
    payload_placement_detail::payload_placement_engine<accepted_payload> engine_;
    Rule rule_;
};

}  // namespace libbsa::detail
