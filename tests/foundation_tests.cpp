#include <catch2/catch_test_macros.hpp>

#include <libbsa/result.hpp>

#include <array>

namespace {

libbsa::result<int> propagate_malformed_archive_failure()
{
    auto parsed = libbsa::failure<int>({libbsa::error_code::malformed_archive, "truncated record"});
    if (!parsed.has_value()) {
        return parsed;
    }

    return libbsa::success(parsed.value());
}

} // namespace

TEST_CASE("result stores successful values", "[unit]")
{
    libbsa::result<int> value = libbsa::success(42);

    REQUIRE(value.has_value());
    CHECK(value.value() == 42);
}

TEST_CASE("result exposes categorized errors", "[unit]")
{
    auto failed = libbsa::failure<int>({libbsa::error_code::unsupported_format, "unsupported archive format"});

    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error().code == libbsa::error_code::unsupported_format);
    CHECK(failed.error().message == "unsupported archive format");
}

TEST_CASE("void result reports success and allows value observation", "[unit]")
{
    libbsa::result<void> completed = libbsa::success();

    REQUIRE(completed.has_value());
    CHECK_NOTHROW(completed.value());
}

TEST_CASE("void result exposes categorized errors", "[unit]")
{
    auto failed = libbsa::failure<void>({libbsa::error_code::io_failure, "read failed"});

    REQUIRE_FALSE(failed.has_value());
    CHECK(failed.error().code == libbsa::error_code::io_failure);
    CHECK(failed.error().message == "read failed");
}

TEST_CASE("failed results propagate unchanged through helper returns", "[unit]")
{
    auto propagated = propagate_malformed_archive_failure();

    REQUIRE_FALSE(propagated.has_value());
    CHECK(propagated.error().code == libbsa::error_code::malformed_archive);
    CHECK(propagated.error().message == "truncated record");
}

TEST_CASE("representative error codes are available", "[unit]")
{
    constexpr std::array codes{
        libbsa::error_code::unsupported_format,
        libbsa::error_code::malformed_archive,
        libbsa::error_code::io_failure,
        libbsa::error_code::decompression_failure,
    };

    CHECK(codes[0] == libbsa::error_code::unsupported_format);
    CHECK(codes[1] == libbsa::error_code::malformed_archive);
    CHECK(codes[2] == libbsa::error_code::io_failure);
    CHECK(codes[3] == libbsa::error_code::decompression_failure);
}
