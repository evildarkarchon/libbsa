#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <string_view>

TEST_CASE("local game fixtures are opt-in", "[requires-game-fixture][unit]") {
  const char* fixture_root = std::getenv("LIBBSA_GAME_FIXTURES");
  if (fixture_root == nullptr || std::string_view{fixture_root}.empty()) {
    SKIP("Set LIBBSA_GAME_FIXTURES or place local game archives under tests/fixtures/local; these files are not committed.");
  }

  REQUIRE_FALSE(std::string_view{fixture_root}.empty());
}
