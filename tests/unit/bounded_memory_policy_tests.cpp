#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::filesystem::path source_root() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR};
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());

  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

} // namespace

TEST_CASE("bounded_memory_policy static boundary keeps BSA writer implementations independent of TES5Edit",
          "[unit][bounded_memory_policy][static_boundary]") {
  constexpr auto writer_sources = std::array{
      "src/formats/bsa/tes3_bsa_writer.cpp",
      "src/formats/bsa/tes4_bsa_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("TES5Edit boundary source: " << source);
    REQUIRE(text.find("TES5Edit") == std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy static boundary keeps BA2 writer implementations independent of TES5Edit",
          "[unit][bounded_memory_policy][static_boundary][ba2_writer_execution]") {
  constexpr auto writer_sources = std::array{
      "src/formats/ba2/ba2_gnrl_writer.cpp",
      "src/formats/ba2/ba2_dx10_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("TES5Edit boundary source: " << source);
    REQUIRE(text.find("TES5Edit") == std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy public API boundary hides private BSA writer helpers",
          "[unit][bounded_memory_policy][static_boundary][public-api]") {
  constexpr auto forbidden_tokens = std::array<std::string_view, 6U>{
      "tes3_writer_entry",
      "tes4_writer_entry",
      "prepared_entry",
      "write_tes3_bsa_archive",
      "write_tes4_bsa_archive",
      "formats::bsa",
  };
  const auto include_dir = source_root() / "include" / "libbsa";

  for (const auto& entry : std::filesystem::directory_iterator{include_dir}) {
    if (entry.path().extension() != ".hpp") {
      continue;
    }

    const auto text = read_text_file(entry.path());
    for (const auto token : forbidden_tokens) {
      INFO("public BSA writer helper boundary token: " << token << " in " << entry.path().string());
      REQUIRE(text.find(token) == std::string::npos);
    }
  }
}

TEST_CASE("bounded_memory_policy public API boundary hides private BA2 writer helpers",
          "[unit][bounded_memory_policy][static_boundary][public-api][ba2_writer_execution]") {
  constexpr auto forbidden_tokens = std::array<std::string_view, 7U>{
      "ba2_gnrl_writer_entry",
      "ba2_dx10_writer_entry",
      "prepared_entry",
      "write_ba2_gnrl_archive",
      "write_ba2_dx10_archive",
      "formats::ba2",
      "DirectXTex",
  };
  const auto include_dir = source_root() / "include" / "libbsa";

  for (const auto& entry : std::filesystem::directory_iterator{include_dir}) {
    if (entry.path().extension() != ".hpp") {
      continue;
    }

    const auto text = read_text_file(entry.path());
    for (const auto token : forbidden_tokens) {
      INFO("public BA2 writer helper boundary token: " << token << " in " << entry.path().string());
      REQUIRE(text.find(token) == std::string::npos);
    }
  }
}

TEST_CASE("bounded_memory_policy public API boundary keeps writer.hpp free of private codec and reference dependencies",
          "[unit][bounded_memory_policy][static_boundary][public-api][ba2_writer_execution]") {
  constexpr auto forbidden_tokens = std::array<std::string_view, 5U>{
      "DirectXTex",
      "DXGI",
      "libdeflate",
      "lz4",
      "TES5Edit",
  };

  const auto text = read_text_file(source_root() / "include" / "libbsa" / "writer.hpp");
  for (const auto token : forbidden_tokens) {
    INFO("public writer.hpp forbidden dependency boundary token: " << token);
    REQUIRE(text.find(token) == std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy targeted writer stages avoid byte-at-a-time disk source accumulation",
          "[unit][bounded_memory_policy][writer-source-io]") {
  constexpr auto writer_sources = std::array{
      "src/formats/bsa/tes4_bsa_prepare.cpp",
      "src/formats/bsa/tes4_bsa_layout.cpp",
      "src/formats/ba2/ba2_gnrl_prepare.cpp",
      "src/formats/ba2/ba2_dx10_prepare.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("writer source read loop policy source: " << source);
    CHECK(text.find("input.get(ch)") == std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy BA2 DX10 snapshot temp directories use hardened reservation",
          "[unit][bounded_memory_policy][ba2_dx10_writer][security]") {
  const auto text = read_text_file(source_root() / "src" / "formats" / "ba2" / "ba2_dx10_prepare.cpp") +
                    read_text_file(source_root() / "src" / "formats" / "ba2" / "ba2_dx10_snapshot_builder.cpp");

  constexpr auto required_tokens = std::array<std::string_view, 4U>{
      "BCryptGenRandom",
      "BCRYPT_USE_SYSTEM_PREFERRED_RNG",
      "std::filesystem::create_directory",
      "libbsa-dx10-snapshot-",
  };
  for (const auto token : required_tokens) {
    INFO("BA2 DX10 hardened snapshot reservation token: " << token);
    CHECK(text.find(token) != std::string::npos);
  }

  constexpr auto forbidden_tokens = std::array<std::string_view, 3U>{
      "static std::atomic_uint64_t counter",
      "fetch_add",
      "std::to_string(id)",
  };
  for (const auto token : forbidden_tokens) {
    INFO("BA2 DX10 predictable snapshot reservation token: " << token);
    CHECK(text.find(token) == std::string::npos);
  }
}
