#include "fingerprint_collision_fixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <detail/host_file.hpp>
#include <detail/payload_placement.hpp>
#include <detail/stored_payload.hpp>
#include <detail/writer_publish.hpp>

#include <libbsa/result.hpp>

#include <support/private_temp_root.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using libbsa::error_code;
using libbsa::detail::constrained_payload_placer;
using libbsa::detail::payload_narrowing_key;
using libbsa::detail::payload_placement;
using libbsa::detail::payload_placement_subject;
using libbsa::detail::payload_placer;
using libbsa::detail::payload_sharing_policy;
using libbsa::detail::payload_sharing_rule_for;
using libbsa::detail::placed_payload;
using libbsa::detail::stored_payload;
using libbsa::tests::fnv1a_fingerprint_collision;

// Deliberately not any archive family's wording. These tests prove the placer
// echoes whatever label it was constructed with rather than owning one.
constexpr std::string_view test_label = "Caller Family";

// A base offset with no family meaning, chosen so that a returned offset that
// forgot to include it is obviously wrong rather than accidentally right.
constexpr std::uint64_t test_base_offset = 4096U;

constexpr libbsa::detail::host_file_context snapshot_source_context{
    "Payload Placement test failed to open source",
    "Payload Placement test failed to inspect source",
    "Payload Placement test failed to read source", "Payload Placement test source changed",
    "Payload Placement test source"};

struct test_sharing_facts {
    std::uint32_t compatibility_mask{};
    std::shared_ptr<void> lifetime{};
};

struct matching_facts_rule {
    /// Returns whether the accepted and offered facts name the same group.
    bool operator()(const test_sharing_facts& accepted,
                    const test_sharing_facts& offered) const noexcept {
        return accepted.compatibility_mask == offered.compatibility_mask;
    }
};

struct overlapping_facts_rule {
    /// Returns whether accepted and offered facts have any compatible bit in common.
    bool operator()(const test_sharing_facts& accepted,
                    const test_sharing_facts& offered) const noexcept {
        return (accepted.compatibility_mask & offered.compatibility_mask) != 0U;
    }
};

struct successor_facts_rule {
    /// Accepts only the next offered fact, making argument order observable.
    bool operator()(const test_sharing_facts& accepted,
                    const test_sharing_facts& offered) const noexcept {
        return accepted.compatibility_mask + 1U == offered.compatibility_mask;
    }
};

struct throwing_facts_rule {
    /// Deliberately lacks `noexcept` so the rule concept must reject it.
    bool operator()(const test_sharing_facts&, const test_sharing_facts&) const { return true; }
};

struct non_boolean_facts_rule {
    /// Deliberately returns a non-Boolean value so the rule concept must reject it.
    int operator()(const test_sharing_facts&, const test_sharing_facts&) const noexcept {
        return 1;
    }
};

struct mutable_only_facts_rule {
    /// Deliberately requires mutable state so the rule concept must reject it.
    bool operator()(const test_sharing_facts&, const test_sharing_facts&) noexcept { return true; }
};

struct counting_facts_rule {
    std::size_t* calls{};

    /// Counts evaluations so sharing-disabled placement can prove it invokes no rule.
    bool operator()(const test_sharing_facts&, const test_sharing_facts&) const noexcept {
        ++*calls;
        return true;
    }
};

template <typename Placer>
concept accepts_placement_without_facts =
    requires(Placer& placer, payload_narrowing_key key, payload_placement_subject subject) {
        placer.place(key, std::move(subject));
    };

template <typename Placer>
concept accepts_candidate_index_callback =
    requires(Placer& placer, payload_narrowing_key key, payload_placement_subject subject) {
        placer.place(key, std::move(subject), +[](std::size_t) { return true; });
    };

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char character : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(character)));
    }
    return bytes;
}

/// Writes one snapshot source inside the test process's private temp root.
void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

/// Creates a snapshot-backed Stored Payload whose body path can later be made unavailable.
stored_payload make_snapshot_payload(const libbsa::detail::finalization_workspace& workspace,
                                     std::size_t identity, std::span<const std::byte> bytes) {
    const auto source_path = libbsa::tests::private_temp_root() /
                             ("payload-placement-source-" + std::to_string(identity) + ".bin");
    write_binary_file(source_path, bytes);
    auto resolved = libbsa::detail::resolve_host_file_path(source_path.string());
    REQUIRE(resolved.has_value());
    auto opened =
        libbsa::detail::stable_host_file_session::open(resolved.value(), snapshot_source_context);
    REQUIRE(opened.has_value());
    auto snapshot = libbsa::detail::stored_payload::from_workspace_snapshot(
        {}, std::move(opened).value(), workspace, identity, 2U);
    REQUIRE(snapshot.has_value());
    return std::move(snapshot).value();
}

std::string materialize(const stored_payload& payload) {
    std::ostringstream output{std::ios::binary};
    auto emitted = payload.emit(output);
    REQUIRE(emitted.has_value());
    return output.str();
}

/// Offers `bytes` to `placer` under the narrowing key derived from them.
///
/// Every caller derives the key the same way a family would — from immutable
/// Stored Payload facts — so byte-equal payloads always land in one bucket and
/// the tests exercise the confirmation step rather than bucket bookkeeping.
libbsa::result<payload_placement> place_bytes(payload_placer& placer,
                                              const std::vector<std::byte>& bytes) {
    auto payload = stored_payload::from_owned_bytes(bytes);
    const payload_narrowing_key key{payload.size(), payload.fingerprint()};
    return placer.place(key, payload_placement_subject::of_payload(std::move(payload)));
}

libbsa::result<payload_placement> place_text(payload_placer& placer, std::string_view text) {
    return place_bytes(placer, bytes_from_text(text));
}

/// Offers `bytes` and mandatory facts through the constrained placement lane.
template <typename Facts, typename Rule>
libbsa::result<payload_placement> place_bytes(constrained_payload_placer<Facts, Rule>& placer,
                                              const std::vector<std::byte>& bytes, Facts facts) {
    auto payload = stored_payload::from_owned_bytes(bytes);
    const payload_narrowing_key key{payload.size(), payload.fingerprint()};
    return placer.place(key, payload_placement_subject::of_payload(std::move(payload)),
                        std::move(facts));
}

/// Offers an existing Stored Payload and mandatory facts through the constrained lane.
template <typename Facts, typename Rule>
libbsa::result<payload_placement> place_payload(constrained_payload_placer<Facts, Rule>& placer,
                                                stored_payload payload, Facts facts) {
    const payload_narrowing_key key{payload.size(), payload.fingerprint()};
    return placer.place(key, payload_placement_subject::of_payload(std::move(payload)),
                        std::move(facts));
}

/// Offers text bytes and mandatory facts through the constrained placement lane.
template <typename Facts, typename Rule>
libbsa::result<payload_placement> place_text(constrained_payload_placer<Facts, Rule>& placer,
                                             std::string_view text, Facts facts) {
    return place_bytes(placer, bytes_from_text(text), std::move(facts));
}

/// Offers a bare length, as a family that has not adopted Stored Payload does.
libbsa::result<payload_placement> place_size(payload_placer& placer, std::uint64_t size) {
    return placer.place(payload_narrowing_key{size, 0U}, payload_placement_subject::of_size(size));
}

payload_placement require_placed(libbsa::result<payload_placement> placed) {
    REQUIRE(placed.has_value());
    return placed.value();
}

}  // namespace

TEST_CASE("payload_placement keeps exclusive ownership of what it accepts",
          "[unit][payload_placement]") {
    // Copying either type would hand out a second owner of a move-only Stored
    // Payload, and copying a placer would fork the sharing index away from the
    // payloads it describes. Both stay movable so a family can hand one on.
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<payload_placer>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<payload_placer>);
    STATIC_REQUIRE(std::is_move_constructible_v<payload_placer>);
    STATIC_REQUIRE(std::is_move_assignable_v<payload_placer>);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible_v<payload_placement_subject>);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable_v<payload_placement_subject>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<payload_placement_subject>);
    STATIC_REQUIRE_FALSE(accepts_candidate_index_callback<payload_placer>);
}

TEST_CASE("payload_placement starts at the base offset and advances by stored size",
          "[unit][payload_placement]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    const auto first = require_placed(place_text(placer, "first"));
    const auto second = require_placed(place_text(placer, "second-payload"));

    CHECK(first.offset == test_base_offset);
    CHECK_FALSE(first.shared);
    CHECK(second.offset == test_base_offset + 5U);
    CHECK_FALSE(second.shared);
    // The cursor is what a family reads to place whatever follows the payload
    // area, typically a filename table.
    CHECK(placer.cursor() == test_base_offset + 5U + 14U);
    CHECK(placer.placed_payload_count() == 2U);
}

TEST_CASE("payload_placement shares one location between byte-equal payloads",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    const auto first = require_placed(place_text(placer, "shared-bytes"));
    const auto separator = require_placed(place_text(placer, "unrelated"));
    const auto repeat = require_placed(place_text(placer, "shared-bytes"));

    CHECK_FALSE(first.shared);
    CHECK_FALSE(separator.shared);
    CHECK(repeat.shared);
    CHECK(repeat.offset == first.offset);
    CHECK(repeat.payload_index == first.payload_index);
    // The share must cost no payload-area bytes: only two locations exist.
    CHECK(placer.placed_payload_count() == 2U);
    CHECK(placer.cursor() == test_base_offset + 12U + 9U);
}

TEST_CASE("payload_placement keeps the first accepted payload as the sharing representative",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    const auto first = require_placed(place_text(placer, "repeated"));
    const auto second = require_placed(place_text(placer, "repeated"));
    const auto third = require_placed(place_text(placer, "repeated"));

    CHECK(second.offset == first.offset);
    CHECK(third.offset == first.offset);
    CHECK(second.payload_index == first.payload_index);
    CHECK(third.payload_index == first.payload_index);
    CHECK(placer.placed_payload_count() == 1U);
}

TEST_CASE("payload_placement refuses to share a fingerprint collision without byte equality",
          "[unit][payload_placement][dedupe][collision]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};
    const auto collision = fnv1a_fingerprint_collision();

    // Equal size and equal fingerprint put these in one bucket, so the only
    // thing keeping them apart is authoritative byte comparison (ADR-0001).
    const auto first = require_placed(place_bytes(placer, collision.distinct_a));
    const auto second = require_placed(place_bytes(placer, collision.distinct_b));

    CHECK_FALSE(second.shared);
    CHECK(second.offset != first.offset);
    CHECK(second.payload_index != first.payload_index);
    CHECK(placer.placed_payload_count() == 2U);
}

TEST_CASE("unconstrained payload placement shares the earliest exact candidate",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    const auto first = require_placed(place_text(placer, "no-constraint"));
    const auto second = require_placed(place_text(placer, "no-constraint"));

    CHECK(second.shared);
    CHECK(second.offset == first.offset);
    CHECK(placer.placed_payload_count() == 1U);
}

TEST_CASE("constrained payload placement requires facts and a non-throwing pairwise rule",
          "[unit][payload_placement][dedupe]") {
    using constrained_placer = constrained_payload_placer<test_sharing_facts, matching_facts_rule>;

    STATIC_REQUIRE(payload_sharing_rule_for<matching_facts_rule, test_sharing_facts>);
    STATIC_REQUIRE_FALSE(payload_sharing_rule_for<throwing_facts_rule, test_sharing_facts>);
    STATIC_REQUIRE_FALSE(payload_sharing_rule_for<non_boolean_facts_rule, test_sharing_facts>);
    STATIC_REQUIRE_FALSE(payload_sharing_rule_for<mutable_only_facts_rule, test_sharing_facts>);
    STATIC_REQUIRE_FALSE(accepts_placement_without_facts<constrained_placer>);
    STATIC_REQUIRE(accepts_placement_without_facts<payload_placer>);
    STATIC_REQUIRE(std::same_as<decltype(std::declval<constrained_placer&&>().release()),
                                std::vector<placed_payload>>);
}

TEST_CASE("constrained payload placement shares only byte-equal payloads with compatible facts",
          "[unit][payload_placement][dedupe]") {
    constrained_payload_placer<test_sharing_facts, matching_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, matching_facts_rule{}};

    const auto first = require_placed(place_text(placer, "identical", test_sharing_facts{1U}));
    const auto compatible = require_placed(place_text(placer, "identical", test_sharing_facts{1U}));
    const auto incompatible =
        require_placed(place_text(placer, "identical", test_sharing_facts{2U}));

    CHECK(compatible.shared);
    CHECK(compatible.payload_index == first.payload_index);
    CHECK_FALSE(incompatible.shared);
    CHECK(incompatible.offset == first.offset + 9U);
    CHECK(placer.placed_payload_count() == 2U);
}

TEST_CASE("constrained payload placement keeps exact equality authoritative after narrowing",
          "[unit][payload_placement][dedupe][collision]") {
    constrained_payload_placer<test_sharing_facts, matching_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, matching_facts_rule{}};
    const auto collision = fnv1a_fingerprint_collision();

    const auto first =
        require_placed(place_bytes(placer, collision.distinct_a, test_sharing_facts{1U}));
    const auto second =
        require_placed(place_bytes(placer, collision.distinct_b, test_sharing_facts{1U}));

    CHECK_FALSE(second.shared);
    CHECK(second.offset != first.offset);
    CHECK(second.payload_index != first.payload_index);
    CHECK(placer.placed_payload_count() == 2U);
}

TEST_CASE("constrained payload placement continues to a later eligible exact candidate",
          "[unit][payload_placement][dedupe][collision]") {
    constrained_payload_placer<test_sharing_facts, matching_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, matching_facts_rule{}};
    const auto collision = fnv1a_fingerprint_collision();

    // All four offers have one narrowing key. The final offer must skip the
    // first candidate by facts, reject the second by exact bytes, and share the
    // third, preserving the candidates' acceptance order throughout.
    const auto ineligible =
        require_placed(place_bytes(placer, collision.distinct_b, test_sharing_facts{1U}));
    const auto unequal =
        require_placed(place_bytes(placer, collision.distinct_a, test_sharing_facts{3U}));
    const auto later_exact =
        require_placed(place_bytes(placer, collision.distinct_b, test_sharing_facts{3U}));
    const auto offered =
        require_placed(place_bytes(placer, collision.distinct_b, test_sharing_facts{3U}));

    CHECK_FALSE(ineligible.shared);
    CHECK_FALSE(unequal.shared);
    CHECK_FALSE(later_exact.shared);
    CHECK(offered.shared);
    CHECK(offered.payload_index == later_exact.payload_index);
    CHECK(offered.offset == later_exact.offset);
    CHECK(placer.placed_payload_count() == 3U);
}

TEST_CASE("constrained payload placement skips unavailable ineligible snapshots",
          "[unit][payload_placement][dedupe][snapshot][failure]") {
    auto reserved = libbsa::detail::finalization_workspace::reserve(
        libbsa::tests::private_temp_root() / "payload-placement-ordering.bsa", test_label);
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();
    constexpr std::size_t snapshot_identity = 1U;
    const auto bytes = bytes_from_text("candidate bytes");

    constrained_payload_placer<test_sharing_facts, matching_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, matching_facts_rule{}};
    const auto candidate = require_placed(
        place_payload(placer, make_snapshot_payload(workspace, snapshot_identity, bytes),
                      test_sharing_facts{1U}));

    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(workspace.snapshot_path(snapshot_identity), removal_error));
    REQUIRE_FALSE(removal_error);

    const auto offered = place_bytes(placer, bytes, test_sharing_facts{2U});
    REQUIRE(offered.has_value());
    CHECK_FALSE(offered.value().shared);
    CHECK(offered.value().offset == candidate.offset + bytes.size());
    CHECK(placer.placed_payload_count() == 2U);
}

TEST_CASE("constrained payload placement propagates unavailable eligible snapshot errors unchanged",
          "[unit][payload_placement][dedupe][snapshot][failure]") {
    auto reserved = libbsa::detail::finalization_workspace::reserve(
        libbsa::tests::private_temp_root() / "payload-placement-error.bsa", test_label);
    REQUIRE(reserved.has_value());
    auto workspace = std::move(reserved).value();
    constexpr std::size_t snapshot_identity = 2U;
    const auto bytes = bytes_from_text("candidate bytes");

    constrained_payload_placer<test_sharing_facts, matching_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, matching_facts_rule{}};
    require_placed(place_payload(placer, make_snapshot_payload(workspace, snapshot_identity, bytes),
                                 test_sharing_facts{1U}));

    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(workspace.snapshot_path(snapshot_identity), removal_error));
    REQUIRE_FALSE(removal_error);

    const auto offered = place_bytes(placer, bytes, test_sharing_facts{1U});
    REQUIRE_FALSE(offered.has_value());
    CHECK(offered.error().code == error_code::io_error);
    CHECK(offered.error().message == "Stored Payload failed to open workspace snapshot");
    CHECK(placer.cursor() == test_base_offset + bytes.size());
    CHECK(placer.placed_payload_count() == 1U);
}

TEST_CASE("constrained payload placement selects the earliest eligible exact representative",
          "[unit][payload_placement][dedupe]") {
    constrained_payload_placer<test_sharing_facts, overlapping_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, overlapping_facts_rule{}};

    // The first two facts do not overlap, so identical bytes become two live
    // candidates. The third overlaps both and must choose the first accepted.
    const auto first = require_placed(place_text(placer, "identical", test_sharing_facts{1U}));
    const auto second = require_placed(place_text(placer, "identical", test_sharing_facts{2U}));
    const auto third = require_placed(place_text(placer, "identical", test_sharing_facts{3U}));

    REQUIRE_FALSE(second.shared);
    CHECK(third.shared);
    CHECK(third.payload_index == first.payload_index);
    CHECK(third.offset == first.offset);
}

TEST_CASE("constrained payload placement passes accepted facts before offered facts",
          "[unit][payload_placement][dedupe]") {
    constrained_payload_placer<test_sharing_facts, successor_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, successor_facts_rule{}};

    const auto first = require_placed(place_text(placer, "identical", test_sharing_facts{1U}));
    const auto second = require_placed(place_text(placer, "identical", test_sharing_facts{2U}));

    CHECK(second.shared);
    CHECK(second.payload_index == first.payload_index);
}

TEST_CASE("constrained payload placement retains facts only for accepted unique payloads",
          "[unit][payload_placement][dedupe]") {
    constrained_payload_placer<test_sharing_facts, successor_facts_rule> placer{
        test_base_offset, payload_sharing_policy::enabled, test_label, successor_facts_rule{}};

    auto first_lifetime = std::make_shared<int>(1);
    const std::weak_ptr<void> first_retained = first_lifetime;
    const auto first = require_placed(
        place_text(placer, "identical", test_sharing_facts{1U, std::move(first_lifetime)}));
    CHECK_FALSE(first_retained.expired());

    auto shared_lifetime = std::make_shared<int>(2);
    const std::weak_ptr<void> shared_discarded = shared_lifetime;
    const auto shared = require_placed(
        place_text(placer, "identical", test_sharing_facts{2U, std::move(shared_lifetime)}));
    REQUIRE(shared.shared);
    CHECK(shared.payload_index == first.payload_index);
    CHECK(shared_discarded.expired());

    auto later_lifetime = std::make_shared<int>(3);
    const std::weak_ptr<void> later_retained = later_lifetime;
    const auto later = require_placed(
        place_text(placer, "identical", test_sharing_facts{3U, std::move(later_lifetime)}));
    // A shared offer must not become a candidate: only accepted fact 1 remains,
    // and the successor rule rejects offered fact 3 against it.
    REQUIRE_FALSE(later.shared);
    CHECK_FALSE(later_retained.expired());

    const auto payloads = std::move(placer).release();
    REQUIRE(payloads.size() == 2U);
    CHECK(materialize(payloads[0].payload) == "identical");
    CHECK(materialize(payloads[1].payload) == "identical");
    CHECK(first_retained.expired());
    CHECK(later_retained.expired());
}

TEST_CASE("constrained payload placement retains no facts when sharing is disabled",
          "[unit][payload_placement][dedupe]") {
    std::size_t rule_calls = 0U;
    constrained_payload_placer<test_sharing_facts, counting_facts_rule> placer{
        test_base_offset, payload_sharing_policy::disabled, test_label,
        counting_facts_rule{&rule_calls}};

    auto first_lifetime = std::make_shared<int>(1);
    const std::weak_ptr<void> first_discarded = first_lifetime;
    const auto first = require_placed(
        place_text(placer, "identical", test_sharing_facts{1U, std::move(first_lifetime)}));
    auto second_lifetime = std::make_shared<int>(2);
    const std::weak_ptr<void> second_discarded = second_lifetime;
    const auto second = require_placed(
        place_text(placer, "identical", test_sharing_facts{1U, std::move(second_lifetime)}));

    CHECK(rule_calls == 0U);
    CHECK(first_discarded.expired());
    CHECK(second_discarded.expired());
    CHECK_FALSE(first.shared);
    CHECK_FALSE(second.shared);
    CHECK(second.offset == first.offset + 9U);
}

TEST_CASE("payload_placement gives a zero-length payload the cursor without advancing it",
          "[unit][payload_placement]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    const auto empty = require_placed(place_text(placer, ""));
    const auto following = require_placed(place_text(placer, "follows"));

    CHECK(empty.offset == test_base_offset);
    CHECK(placer.cursor() == test_base_offset + 7U);
    // The empty payload shares its offset with whatever comes next. That is the
    // reference's behaviour, not an accident of ordering.
    CHECK(following.offset == empty.offset);
}

TEST_CASE("payload_placement rejects a payload span that leaves the 64-bit range",
          "[unit][payload_placement][malformed]") {
    constexpr auto highest = std::numeric_limits<std::uint64_t>::max();
    payload_placer placer{highest - 8U, payload_sharing_policy::enabled, test_label};

    // The span itself fits exactly; the next one cannot.
    const auto fits = place_size(placer, 8U);
    REQUIRE(fits.has_value());
    CHECK(placer.cursor() == highest);

    const auto overflows = place_size(placer, 1U);
    REQUIRE_FALSE(overflows.has_value());
    CHECK(overflows.error().code == error_code::format_error);
    CHECK(overflows.error().message.find(test_label) != std::string::npos);
}

TEST_CASE("payload_placement with sharing disabled assigns every placement its own location",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::disabled, test_label};

    const auto first = require_placed(place_text(placer, "identical"));
    const auto second = require_placed(place_text(placer, "identical"));

    CHECK_FALSE(first.shared);
    CHECK_FALSE(second.shared);
    CHECK(second.offset == first.offset + 9U);
    CHECK(placer.placed_payload_count() == 2U);
    CHECK(placer.cursor() == test_base_offset + 18U);
}

TEST_CASE("payload_placement accepts a size without a Stored Payload",
          "[unit][payload_placement]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::disabled, test_label};

    const auto first = require_placed(place_size(placer, 40U));
    const auto second = require_placed(place_size(placer, 0U));
    const auto third = require_placed(place_size(placer, 24U));

    CHECK(first.offset == test_base_offset);
    // The zero-length rule holds for size-only families too.
    CHECK(second.offset == test_base_offset + 40U);
    CHECK(third.offset == second.offset);
    CHECK(placer.cursor() == test_base_offset + 64U);
    // A size-only placement contributes no Stored Payload, so it has no index.
    CHECK_FALSE(first.payload_index.has_value());
    CHECK(placer.placed_payload_count() == 0U);
    CHECK(std::move(placer).release().empty());
}

TEST_CASE("payload_placement never shares a size-only placement even with sharing enabled",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    // Identical keys and identical sizes, but no bytes to compare. Byte
    // equality is the only authority for sharing, so these cannot collapse.
    const auto first = require_placed(place_size(placer, 16U));
    const auto second = require_placed(place_size(placer, 16U));

    CHECK_FALSE(first.shared);
    CHECK_FALSE(second.shared);
    CHECK(second.offset == first.offset + 16U);
}

TEST_CASE("payload_placement releases accepted payloads with their offsets in placement order",
          "[unit][payload_placement]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    const auto first = require_placed(place_text(placer, "alpha"));
    const auto second = require_placed(place_text(placer, "bravo"));
    const auto repeat = require_placed(place_text(placer, "alpha"));

    REQUIRE(first.payload_index == 0U);
    REQUIRE(second.payload_index == 1U);
    REQUIRE(repeat.payload_index == 0U);

    const auto payloads = std::move(placer).release();
    REQUIRE(payloads.size() == 2U);
    // Released order matches the indices already handed out, which is what lets
    // a family serialize payloads by index without a second lookup table.
    CHECK(materialize(payloads[0].payload) == "alpha");
    CHECK(materialize(payloads[1].payload) == "bravo");
    // Each payload arrives with the offset it was assigned, so no family has to
    // keep a vector of offsets running parallel to the payloads.
    CHECK(payloads[0].offset == first.offset);
    CHECK(payloads[1].offset == second.offset);
}

TEST_CASE("payload_placement refuses to place after releasing its payloads",
          "[unit][payload_placement]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};
    REQUIRE(place_text(placer, "released").has_value());
    const auto payloads = std::move(placer).release();
    REQUIRE(payloads.size() == 1U);

    // The placer no longer owns what its indices refer to, so a further
    // placement would hand back an index aliasing a payload the caller already
    // holds. That is a programmer error, not archive data the library can
    // reject, so it throws rather than returning a result.
    CHECK_THROWS_AS(place_text(placer, "afterwards"), std::logic_error);
}
