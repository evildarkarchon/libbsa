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

TEST_CASE("bounded_memory_policy BA2 writers do not publish final archives through whole byte vectors",
          "[unit][bounded_memory_policy][ba2_writer_execution]") {
  constexpr auto writer_sources = std::array{
      "src/formats/ba2/ba2_gnrl_writer.cpp",
      "src/formats/ba2/ba2_dx10_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("BA2 writer source: " << source);
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

TEST_CASE("bounded_memory_policy BA2 disk-backed payload emission is visibly streaming",
          "[unit][bounded_memory_policy][ba2_writer_execution]") {
  constexpr auto writer_sources = std::array{
      "src/formats/ba2/ba2_gnrl_writer.cpp",
      "src/formats/ba2/ba2_dx10_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("BA2 writer source: " << source);
    REQUIRE(text.find("64U * 1024U") != std::string::npos);
    REQUIRE(text.find("stream") != std::string::npos);
  }
}

TEST_CASE("bounded_memory_policy DX10 writer stores bounded snapshot paths instead of long-lived full DDS bytes",
          "[unit][bounded_memory_policy][ba2_writer_execution][DX10]") {
  const auto header = read_text_file(source_root() / "src/formats/ba2/ba2_dx10_writer.hpp");
  const auto source = read_text_file(source_root() / "src/formats/ba2/ba2_dx10_writer.cpp");

  REQUIRE(header.find("std::vector<std::byte> dds_bytes") == std::string::npos);
  REQUIRE(header.find("texture::dds_source_analysis source") == std::string::npos);
  REQUIRE(source.find("snapshot_path") != std::string::npos);
  REQUIRE(source.find("temp") != std::string::npos);
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

TEST_CASE("bounded_memory_policy BA2 writers do not reference TES5Edit in implementation",
          "[unit][bounded_memory_policy][ba2_writer_execution]") {
  constexpr auto writer_sources = std::array{
      "src/formats/ba2/ba2_gnrl_writer.cpp",
      "src/formats/ba2/ba2_dx10_writer.cpp",
  };

  for (const auto* source : writer_sources) {
    const auto text = read_text_file(source_root() / source);
    INFO("BA2 writer source: " << source);
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

TEST_CASE("bounded_memory_policy public headers do not expose private BA2 writer helpers",
          "[unit][bounded_memory_policy][public-api][ba2_writer_execution]") {
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
      INFO("public BA2 writer helper token: " << token << " in " << entry.path().string());
      REQUIRE(text.find(token) == std::string::npos);
    }
  }
}

TEST_CASE("bounded_memory_policy writer header keeps BA2 execution boundary dependency-light",
          "[unit][bounded_memory_policy][public-api][ba2_writer_execution]") {
  constexpr auto forbidden_tokens = std::array<std::string_view, 5U>{
      "DirectXTex",
      "DXGI",
      "libdeflate",
      "lz4",
      "TES5Edit",
  };

  const auto text = read_text_file(source_root() / "include" / "libbsa" / "writer.hpp");
  for (const auto token : forbidden_tokens) {
    INFO("public writer.hpp dependency token: " << token);
    REQUIRE(text.find(token) == std::string::npos);
  }
}
