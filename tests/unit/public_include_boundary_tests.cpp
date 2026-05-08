#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");

TEST_CASE("public_include_boundary umbrella header exposes public boundary types", "[unit][public-api]") {
  [[maybe_unused]] libbsa::result<int> result{1};
  [[maybe_unused]] auto code = libbsa::error_code::unsupported;
  [[maybe_unused]] auto reader = libbsa::archive_reader::open("boundary-smoke.bsa");

  REQUIRE(result.has_value());
}

TEST_CASE("public_include_boundary excludes private Phase 2 implementation names", "[unit][public-api]") {
  constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4", "DirectXTex", "Windows.h",
                                                                     "TES5Edit", "std::expected", "bethesda_hash",
                                                                     "compression_router", "archive_path_key"});
  const auto include_dir = std::filesystem::path{LIBBSA_SOURCE_DIR} / "include" / "libbsa";

  for (const auto& entry : std::filesystem::directory_iterator{include_dir}) {
    if (entry.path().extension() != ".hpp") {
      continue;
    }

    std::ifstream file{entry.path()};
    REQUIRE(file.is_open());

    std::string line;
    while (std::getline(file, line)) {
      auto first = line.find_first_not_of(" \t");
      if (first == std::string::npos || line.compare(first, 2, "//") == 0) {
        continue;
      }
      for (const auto token : forbidden_tokens) {
        INFO("public boundary token: " << token << " in " << entry.path().string());
        REQUIRE(line.find(token) == std::string::npos);
      }
    }
  }
}
