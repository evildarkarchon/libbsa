#pragma once

#include <detail/stored_payload.hpp>

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
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
/// forbid a share belong in a `payload_sharing_eligibility` predicate instead,
/// which is what keeps that claim true for every archive family.
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

/// Decides whether a byte-equal earlier payload is a legal sharing partner.
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
/// omit the argument rather than passing an always-true lambda.
using payload_sharing_eligibility = std::function<bool(std::size_t candidate_payload_index)>;

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

/// Owns Payload Placement for one archive being written.
///
/// The module owns four things and only those four: the sharing decision
/// (bucket by a narrowing key, confirm by exact Stored Payload byte equality),
/// the payload cursor including the zero-length rule, overflow-checked payload
/// span arithmetic, and the accepted Stored Payloads themselves.
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
    /// Appends `subject` at the cursor and advances it, or reports overflow.
    result<payload_placement> place_at_cursor(const payload_narrowing_key& key,
                                              payload_placement_subject subject);

    std::uint64_t cursor_;
    payload_sharing_policy sharing_policy_;
    std::string diagnostic_label_;

    /// Accepted payloads in placement order, each paired with the offset it was
    /// given. Index positions are the `payload_index` values handed back to
    /// callers and to eligibility predicates, so entries are only ever appended.
    ///
    /// The offset lives beside the payload rather than in a parallel vector so
    /// that accepting a payload is a single append and the two can never fall
    /// out of step. Stored Payload itself deliberately carries no archive
    /// location, so the pairing has to happen somewhere, and this is the only
    /// place that knows it. `release` hands the pairs on intact for the same
    /// reason.
    std::vector<placed_payload> payloads_;

    /// Candidate indices into `payloads_`, bucketed by narrowing key and held
    /// in acceptance order so the first accepted payload stays representative.
    std::map<payload_narrowing_key, std::vector<std::size_t>> candidate_buckets_;

    /// True once `release` has handed the accepted payloads away.
    bool released_{false};
};

}  // namespace libbsa::detail
