#include <catch2/catch_test_macros.hpp>

#include <detail/payload_span_exclusivity.hpp>

#include <libbsa/result.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>

namespace {

using libbsa::detail::payload_span_exclusivity;

// Deliberately not any archive family's wording: these tests prove the module
// echoes whatever the caller hands it rather than owning a message of its own.
constexpr std::string_view overlap_message = "caller-owned payload span diagnostic";

}  // namespace

TEST_CASE("payload_span_exclusivity accepts disjoint spans offered out of order",
          "[unit][payload_span_exclusivity]") {
    payload_span_exclusivity spans;

    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());
    REQUIRE(spans.insert(1024U, 64U, overlap_message).has_value());
    REQUIRE(spans.insert(2048U, 2048U, overlap_message).has_value());
    // Adjacency is not overlap: this span starts exactly where 1024+64 ends.
    REQUIRE(spans.insert(1088U, 960U, overlap_message).has_value());

    CHECK(spans.distinct_span_count() == 4U);
}

TEST_CASE("payload_span_exclusivity collapses byte-identical spans onto one representative",
          "[unit][payload_span_exclusivity]") {
    payload_span_exclusivity spans;

    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());
    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());
    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());

    CHECK(spans.distinct_span_count() == 1U);
}

TEST_CASE("payload_span_exclusivity rejects partial overlap with the caller's wording",
          "[unit][payload_span_exclusivity][malformed]") {
    payload_span_exclusivity spans;
    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());

    SECTION("a candidate that starts inside an accepted span") {
        auto inserted = spans.insert(4160U, 128U, overlap_message);

        REQUIRE_FALSE(inserted.has_value());
        CHECK(inserted.error().code == libbsa::error_code::format_error);
        CHECK(inserted.error().message == overlap_message);
    }

    SECTION("a candidate that ends inside an accepted span") {
        auto inserted = spans.insert(4032U, 128U, overlap_message);

        REQUIRE_FALSE(inserted.has_value());
        CHECK(inserted.error().code == libbsa::error_code::format_error);
        CHECK(inserted.error().message == overlap_message);
    }

    SECTION("a candidate sharing an offset with a different size") {
        auto inserted = spans.insert(4096U, 256U, overlap_message);

        REQUIRE_FALSE(inserted.has_value());
        CHECK(inserted.error().code == libbsa::error_code::format_error);
        CHECK(inserted.error().message == overlap_message);
    }

    SECTION("a candidate that strictly contains an accepted span") {
        auto inserted = spans.insert(4032U, 512U, overlap_message);

        REQUIRE_FALSE(inserted.has_value());
        CHECK(inserted.error().code == libbsa::error_code::format_error);
        CHECK(inserted.error().message == overlap_message);
    }
}

TEST_CASE("payload_span_exclusivity finds an overlap hidden behind duplicate spans",
          "[unit][payload_span_exclusivity][malformed]") {
    // Three or more spans with no special casing: the duplicates collapse, so
    // the candidate's immediate predecessor is the nearest distinct span rather
    // than a member of a duplicate run.
    payload_span_exclusivity spans;
    REQUIRE(spans.insert(1024U, 64U, overlap_message).has_value());
    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());
    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());
    REQUIRE(spans.insert(4096U, 128U, overlap_message).has_value());
    REQUIRE(spans.distinct_span_count() == 2U);

    auto inserted = spans.insert(4160U, 64U, overlap_message);

    REQUIRE_FALSE(inserted.has_value());
    CHECK(inserted.error().code == libbsa::error_code::format_error);
}

TEST_CASE("payload_span_exclusivity accepts empty spans without storing them",
          "[unit][payload_span_exclusivity]") {
    payload_span_exclusivity spans;

    REQUIRE(spans.insert(4096U, 0U, overlap_message).has_value());
    REQUIRE(spans.insert(4200U, 0U, overlap_message).has_value());

    CHECK(spans.distinct_span_count() == 0U);
}

TEST_CASE("payload_span_exclusivity preserves saturating end arithmetic near the 64-bit maximum",
          "[unit][payload_span_exclusivity][malformed]") {
    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    payload_span_exclusivity spans;

    SECTION("a wrapping end offset does not make an overlapping span look disjoint") {
        // Without saturation, max - 16 plus a length of 64 would wrap to 47 and
        // the second span would compare as if it sat below the first.
        REQUIRE(spans.insert(max - 16U, 64U, overlap_message).has_value());

        auto inserted = spans.insert(max - 8U, 64U, overlap_message);

        REQUIRE_FALSE(inserted.has_value());
        CHECK(inserted.error().code == libbsa::error_code::format_error);
        CHECK(inserted.error().message == overlap_message);
    }

    SECTION("a span below a saturating span is still accepted when disjoint") {
        REQUIRE(spans.insert(max - 16U, 64U, overlap_message).has_value());
        REQUIRE(spans.insert(max - 80U, 64U, overlap_message).has_value());

        CHECK(spans.distinct_span_count() == 2U);
    }
}

TEST_CASE("payload_span_exclusivity completes a large exact-duplicate workload",
          "[unit][payload_span_exclusivity][slow]") {
    // Exact duplicates are the worst case: every attempt runs the full
    // acceptance path and none trips an early exit, so the per-record linear
    // scan this module replaced would do over a hundred billion comparisons
    // here: over two minutes even at a billion comparisons a second, and far
    // worse on the debug and ASan lanes. The bound is absolute and sits between
    // those two costs with room on both sides -- the loop measures about 1.8
    // seconds on the slowest supported lane (ASan debug), so the ceiling is
    // more than an order of magnitude of headroom while still tripping on any
    // return to a per-record scan. The timer wraps only the loop, so process
    // and sanitizer startup do not eat into the margin. That makes this a
    // completion-bound hang guard, not a host-dependent timing gate and not a
    // speedup ratio. It asserts nothing about behavior -- behavior belongs to
    // the archive-opening seam -- and needs no instrumentation in library code.
    constexpr std::size_t attempts = 500'000U;
    constexpr auto ceiling = std::chrono::seconds{60};

    payload_span_exclusivity spans;
    std::size_t accepted = 0U;
    const auto started = std::chrono::steady_clock::now();
    for (std::size_t attempt = 0; attempt < attempts; ++attempt) {
        if (spans.insert(4096U, 128U, overlap_message).has_value()) {
            ++accepted;
        }
    }
    const auto duration = std::chrono::steady_clock::now() - started;

    // The counter exists only so the loop cannot be elided; it is reported, not
    // asserted on.
    INFO("accepted attempts: " << accepted);
    REQUIRE(duration < ceiling);
}
