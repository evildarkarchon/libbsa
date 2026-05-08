#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");

TEST_CASE("umbrella header exposes public boundary types", "[unit][public-api]") {
  [[maybe_unused]] libbsa::result<int> result{1};
  [[maybe_unused]] auto code = libbsa::error_code::unsupported;
  [[maybe_unused]] auto reader = libbsa::archive_reader::open("boundary-smoke.bsa");

  REQUIRE(result.has_value());
}
