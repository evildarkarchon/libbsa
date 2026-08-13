#include "fingerprint_collision_fixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <detail/payload_placement.hpp>
#include <detail/stored_payload.hpp>

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
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
using libbsa::detail::payload_narrowing_key;
using libbsa::detail::payload_placement;
using libbsa::detail::payload_placement_subject;
using libbsa::detail::payload_placer;
using libbsa::detail::payload_sharing_eligibility;
using libbsa::detail::payload_sharing_policy;
using libbsa::detail::stored_payload;
using libbsa::tests::fnv1a_fingerprint_collision;

// Deliberately not any archive family's wording. These tests prove the placer
// echoes whatever label it was constructed with rather than owning one.
constexpr std::string_view test_label = "Caller Family";

// A base offset with no family meaning, chosen so that a returned offset that
// forgot to include it is obviously wrong rather than accidentally right.
constexpr std::uint64_t test_base_offset = 4096U;

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char character : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(character)));
    }
    return bytes;
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
                                              const std::vector<std::byte>& bytes,
                                              const payload_sharing_eligibility& eligible = {}) {
    auto payload = stored_payload::from_owned_bytes(bytes);
    const payload_narrowing_key key{payload.size(), payload.fingerprint()};
    return placer.place(key, payload_placement_subject::of_payload(std::move(payload)), eligible);
}

libbsa::result<payload_placement> place_text(payload_placer& placer, std::string_view text,
                                             const payload_sharing_eligibility& eligible = {}) {
    return place_bytes(placer, bytes_from_text(text), eligible);
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

TEST_CASE("payload_placement never shares when a Sharing Eligibility predicate refuses",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};
    const payload_sharing_eligibility refuse_every_candidate = [](std::size_t) { return false; };

    const auto first = require_placed(place_text(placer, "identical", refuse_every_candidate));
    const auto second = require_placed(place_text(placer, "identical", refuse_every_candidate));

    CHECK_FALSE(second.shared);
    CHECK(second.offset == first.offset + 9U);
    CHECK(placer.placed_payload_count() == 2U);
}

TEST_CASE("payload_placement lets Sharing Eligibility choose among byte-equal candidates",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};
    const payload_sharing_eligibility refuse_every_candidate = [](std::size_t) { return false; };

    // Two byte-equal payloads forced into distinct locations, so the bucket
    // holds two live candidates rather than one.
    const auto first = require_placed(place_text(placer, "identical", refuse_every_candidate));
    const auto second = require_placed(place_text(placer, "identical", refuse_every_candidate));
    REQUIRE(first.payload_index.has_value());
    REQUIRE(second.payload_index.has_value());

    const auto second_index = second.payload_index.value();
    const payload_sharing_eligibility accept_only_second = [second_index](std::size_t candidate) {
        return candidate == second_index;
    };
    const auto third = require_placed(place_text(placer, "identical", accept_only_second));

    // Without eligibility the earliest candidate would win. Proving the later
    // one wins here shows the predicate genuinely gates each candidate rather
    // than gating the placement as a whole.
    CHECK(third.shared);
    CHECK(third.payload_index == second.payload_index);
    CHECK(third.offset == second.offset);
    CHECK(placer.placed_payload_count() == 2U);
}

TEST_CASE("payload_placement treats an omitted Sharing Eligibility predicate as always eligible",
          "[unit][payload_placement][dedupe]") {
    payload_placer placer{test_base_offset, payload_sharing_policy::enabled, test_label};

    const auto first = require_placed(place_text(placer, "no-constraint"));
    // No predicate argument at all: the call site states that this family has
    // no eligibility constraint rather than passing an always-true lambda.
    const auto second = require_placed(place_text(placer, "no-constraint"));

    CHECK(second.shared);
    CHECK(second.offset == first.offset);
    CHECK(placer.placed_payload_count() == 1U);
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
    const payload_sharing_eligibility accept_every_candidate = [](std::size_t) { return true; };

    const auto first = require_placed(place_text(placer, "identical", accept_every_candidate));
    const auto second = require_placed(place_text(placer, "identical", accept_every_candidate));

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
