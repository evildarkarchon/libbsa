#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

TEST_CASE("archive_reader open reports unsupported archives in phase one", "[unit][public-api]") {
  auto result = libbsa::archive_reader::open("example.bsa");

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().code == libbsa::error_code::unsupported);
}

TEST_CASE("archive_reader open rejects empty host paths", "[unit][public-api]") {
  auto result = libbsa::archive_reader::open("");

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().code == libbsa::error_code::invalid_argument);
}
