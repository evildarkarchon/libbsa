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

TEST_CASE("bounded_memory_policy BSA writers do not publish final archives through whole byte vectors",
          "[unit][bounded_memory_policy]") {
  constexpr auto writer_sources = std::array{
      "src/formats/bsa/tes3_bsa_writer.cpp",
      "src/formats/bsa/tes4_bsa_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("BSA writer source: " << source);
    REQUIRE(text.find("const auto bytes = writer.bytes()") == std::string::npos);
    REQUIRE(text.find("write_execution_options") != std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy BSA disk-backed payload emission is visibly streaming",
          "[unit][bounded_memory_policy]") {
  constexpr auto writer_sources = std::array{
      "src/formats/bsa/tes3_bsa_writer.cpp",
      "src/formats/bsa/tes4_bsa_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("BSA writer source: " << source);
    REQUIRE(text.find("disk") != std::string::npos);
    REQUIRE(text.find("stream") != std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy BSA writers do not reference TES5Edit in implementation",
          "[unit][bounded_memory_policy]") {
  constexpr auto writer_sources = std::array{
      "src/formats/bsa/tes3_bsa_writer.cpp",
      "src/formats/bsa/tes4_bsa_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("BSA writer source: " << source);
    REQUIRE(text.find("TES5Edit") == std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy public headers do not expose private BSA writer helpers",
          "[unit][bounded_memory_policy][public-api]") {
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
      INFO("public BSA writer helper token: " << token << " in " << entry.path().string());
      REQUIRE(text.find(token) == std::string::npos);
    }
  }
}
