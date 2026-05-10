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
