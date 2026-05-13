#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.is_open());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

} // namespace

TEST_CASE("writer call sites use the neutral host_file helper seam", "[unit][host_file]") {
  const auto root = source_root();
  const auto legacy_include = std::string{"writer_"} + "disk_source";
  const auto legacy_exact = std::string{"read_"} + "disk_source_";
  const auto legacy_inspect = std::string{"inspect_"} + "disk_source_size";
  const auto legacy_chunk = std::string{"for_each_"} + "disk_source_chunk";
  constexpr auto cases = std::to_array<std::pair<std::string_view, std::string_view>>({
      {"src/formats/bsa/tes4_bsa_prepare.cpp", "tes4_prepare_source_context"},
      {"src/formats/bsa/tes4_bsa_layout.cpp", "tes4_dedupe_source_context"},
      {"src/formats/ba2/ba2_gnrl_prepare.cpp", "ba2_gnrl_prepare_source_context"},
      {"src/formats/ba2/ba2_dx10_prepare.cpp", "ba2_dx10_dds_source_context"},
  });

  for (const auto& [relative_path, context_name] : cases) {
    const auto text = read_text_file(root / relative_path);
    INFO("Source file: " << relative_path);
    REQUIRE(text.find("#include <detail/host_file.hpp>") != std::string::npos);
    REQUIRE(text.find("host_file_context") != std::string::npos);
    REQUIRE(text.find(context_name) != std::string::npos);
    REQUIRE(text.find(legacy_include) == std::string::npos);
    REQUIRE(text.find(legacy_exact) == std::string::npos);
    REQUIRE(text.find(legacy_inspect) == std::string::npos);
    REQUIRE(text.find(legacy_chunk) == std::string::npos);
  }
}

TEST_CASE("host_file helper surface exposes the shared host_file_path contract", "[unit][host_file]") {
  const auto root = source_root();
  constexpr auto contract_files = std::to_array<std::string_view>({
      "src/detail/host_file.hpp",
      "src/detail/host_file.cpp",
      "src/detail/host_file_path.hpp",
  });

  for (const auto relative_path : contract_files) {
    const auto text = read_text_file(root / relative_path);
    INFO("Source file: " << relative_path);
    REQUIRE(text.find("host_file_path") != std::string::npos);
  }

  const auto path_header = read_text_file(root / "src/detail/host_file_path.hpp");
  REQUIRE(path_header.find("resolve_host_file_path(") != std::string::npos);
}

TEST_CASE("migrated writer call sites build host_file_path contracts before shared helper reads", "[unit][host_file]") {
  const auto root = source_root();
  constexpr auto resolved_contract_cases = std::to_array<std::pair<std::string_view, std::string_view>>({
      {"src/formats/bsa/tes4_bsa_prepare.cpp", "resolve_tes4_source_path(entry.host_path)"},
      {"src/formats/bsa/tes4_bsa_layout.cpp", "resolve_tes4_dedupe_source_path(entry.raw_disk_host_path)"},
      {"src/formats/ba2/ba2_gnrl_prepare.cpp", "resolve_ba2_gnrl_source_path(entry.host_path)"},
      {"src/formats/ba2/ba2_dx10_prepare.cpp", "resolve_host_file_path(dds_host_path)"},
  });

  for (const auto& [relative_path, expected_text] : resolved_contract_cases) {
    const auto text = read_text_file(root / relative_path);
    INFO("Source file: " << relative_path);
    REQUIRE(text.find("host_file_path") != std::string::npos);
    REQUIRE(text.find(expected_text) != std::string::npos);
  }
}

TEST_CASE("archive_reader open stores the shared host_file_path contract", "[unit][host_file]") {
  const auto archive_text = read_text_file(source_root() / "src/archive.cpp");

  REQUIRE(archive_text.find("detail::host_file_path host_path;") != std::string::npos);
  REQUIRE(archive_text.find("resolve_host_file_path(host_path)") != std::string::npos);
}

TEST_CASE("archive_reader open routes detection and size probes through host_file helpers", "[unit][host_file]") {
  const auto archive_text = read_text_file(source_root() / "src/archive.cpp");

  REQUIRE(archive_text.find("read_host_file_prefix(") != std::string::npos);
  REQUIRE(archive_text.find("inspect_host_file_size(") != std::string::npos);
  REQUIRE(archive_text.find("std::ifstream input{std::string{host_path}, std::ios::binary}") == std::string::npos);
  REQUIRE(archive_text.find("std::ifstream input{std::string{host_path}, std::ios::binary | std::ios::ate}") ==
          std::string::npos);
}

TEST_CASE("parser entry declarations consume the shared host-file path contract", "[unit][host_file]") {
  const auto root = source_root();
  constexpr auto parser_headers = std::to_array<std::string_view>({"src/formats/bsa/tes3_bsa_parser.hpp",
                                                                   "src/formats/bsa/tes4_bsa_parser.hpp",
                                                                   "src/formats/ba2/ba2_gnrl_parser.hpp",
                                                                   "src/formats/ba2/ba2_dx10_parser.hpp"});

  for (const auto relative_path : parser_headers) {
    const auto text = read_text_file(root / relative_path);
    INFO("Source file: " << relative_path);
    REQUIRE(text.find("host_file_path") != std::string::npos);
    REQUIRE(text.find("std::string_view host_path") == std::string::npos);
  }
}

TEST_CASE("parser archive-file opens use the shared host_file seam", "[unit][host_file]") {
  const auto root = source_root();
  constexpr auto parser_sources = std::to_array<std::string_view>({"src/formats/bsa/tes3_bsa_parser.cpp",
                                                                   "src/formats/bsa/tes4_bsa_parser.cpp",
                                                                   "src/formats/ba2/ba2_gnrl_parser.cpp",
                                                                   "src/formats/ba2/ba2_dx10_parser.cpp"});

  for (const auto relative_path : parser_sources) {
    const auto text = read_text_file(root / relative_path);
    INFO("Source file: " << relative_path);
    REQUIRE(text.find("open_host_file(") != std::string::npos);
    REQUIRE(text.find("std::ifstream input{std::string{host_path}, std::ios::binary}") == std::string::npos);
  }
}
