#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");
static_assert(std::is_enum_v<libbsa::archive_type>);
static_assert(std::is_enum_v<libbsa::archive_variant>);
static_assert(std::is_enum_v<libbsa::entry_compression>);
static_assert(std::is_default_constructible_v<libbsa::ba2_archive_metadata>);
static_assert(std::is_abstract_v<libbsa::payload_sink>);

TEST_CASE("public_include_boundary umbrella header exposes public boundary types", "[unit][public-api]") {
  [[maybe_unused]] libbsa::result<int> result{1};
  [[maybe_unused]] auto code = libbsa::error_code::unsupported;
  [[maybe_unused]] auto missing = libbsa::error_code::not_found;
  [[maybe_unused]] libbsa::ba2_archive_metadata ba2_metadata{123U, 456U, 3U};
  [[maybe_unused]] libbsa::archive_metadata archive{libbsa::archive_type::bsa,
                                                    libbsa::archive_variant::tes4,
                                                    103,
                                                    0,
                                                    1,
                                                    libbsa::entry_compression::deflate,
                                                    std::nullopt};
  [[maybe_unused]] libbsa::entry_metadata entry{"meshes/example.nif",
                                               "Meshes/Example.nif",
                                               10,
                                               8,
                                               128,
                                               0x0102030405060708ULL,
                                               libbsa::entry_compression::none,
                                               0,
                                               false,
                                               0};
  [[maybe_unused]] auto reader = libbsa::archive_reader::open("boundary-smoke.bsa");

  REQUIRE(result.has_value());
}

TEST_CASE("public_include_boundary excludes private Phase 2 implementation names", "[unit][public-api]") {
  constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                     "DXGI", "Windows.h", "DDS_HEADER_DXT10",
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
