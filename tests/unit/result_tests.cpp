#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

TEST_CASE("result stores successful values", "[unit][public-api]") {
  libbsa::result<int> result{42};

  REQUIRE(result.has_value());
  REQUIRE(result);
  REQUIRE(result.value() == 42);
}

TEST_CASE("result exposes stable error codes", "[unit][public-api]") {
  libbsa::result<int> result{libbsa::error{libbsa::error_code::format_error, "bad"}};

  REQUIRE_FALSE(result.has_value());
  REQUIRE_FALSE(result);
  REQUIRE(result.error().code == libbsa::error_code::format_error);
}

TEST_CASE("void result represents success", "[unit][public-api]") {
  libbsa::result<void> result{};

  REQUIRE(result.has_value());
  REQUIRE(result);
}
