#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");
static_assert(std::is_enum_v<libbsa::archive_type>);
static_assert(std::is_enum_v<libbsa::archive_variant>);
static_assert(std::is_enum_v<libbsa::entry_compression>);
static_assert(std::is_enum_v<libbsa::tes4_bsa_target>);
static_assert(std::is_enum_v<libbsa::archive_compression_policy>);
static_assert(std::is_enum_v<libbsa::entry_compression_policy>);
static_assert(std::is_class_v<libbsa::tes3_bsa_writer_options>);
static_assert(std::is_class_v<libbsa::tes3_bsa_writer>);
static_assert(std::is_enum_v<libbsa::ba2_gnrl_target>);
static_assert(std::is_class_v<libbsa::ba2_gnrl_writer_options>);
static_assert(std::is_class_v<libbsa::ba2_gnrl_entry_options>);
static_assert(std::is_class_v<libbsa::ba2_gnrl_writer>);
static_assert(std::is_enum_v<libbsa::ba2_dx10_target>);
static_assert(std::is_class_v<libbsa::ba2_dx10_writer_options>);
static_assert(std::is_class_v<libbsa::ba2_dx10_writer>);
static_assert(std::is_default_constructible_v<libbsa::ba2_archive_metadata>);
static_assert(std::is_constructible_v<libbsa::tes4_bsa_writer, libbsa::tes4_bsa_target>);
static_assert(std::is_constructible_v<libbsa::tes4_bsa_writer,
                                      libbsa::tes4_bsa_target,
                                      libbsa::tes4_bsa_writer_options>);
static_assert(std::is_constructible_v<libbsa::ba2_gnrl_writer, libbsa::ba2_gnrl_target>);
static_assert(std::is_constructible_v<libbsa::ba2_gnrl_writer,
                                      libbsa::ba2_gnrl_target,
                                      libbsa::ba2_gnrl_writer_options>);
static_assert(std::is_constructible_v<libbsa::ba2_dx10_writer, libbsa::ba2_dx10_target>);
static_assert(std::is_constructible_v<libbsa::ba2_dx10_writer,
                                      libbsa::ba2_dx10_target,
                                      libbsa::ba2_dx10_writer_options>);
static_assert(std::is_abstract_v<libbsa::payload_sink>);

static_assert(std::is_default_constructible_v<libbsa::tes3_bsa_writer>);
static_assert(std::is_constructible_v<libbsa::tes3_bsa_writer, libbsa::tes3_bsa_writer_options>);

// BEGIN tes3_bsa_public_contract_assertions
static_assert(requires(libbsa::tes3_bsa_writer& writer, std::span<const std::byte> bytes) {
  { writer.options() } -> std::same_as<const libbsa::tes3_bsa_writer_options&>;
  { writer.add_bytes("Meshes/Memory.nif", bytes) } -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Textures/Disk.dds", "source.dds") } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.bsa") } -> std::same_as<libbsa::result<void>>;
});
// END tes3_bsa_public_contract_assertions

static_assert(requires(libbsa::tes4_bsa_writer& writer, std::span<const std::byte> bytes) {
  { writer.add_bytes("Meshes/Memory.nif", bytes) } -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Textures/Disk.dds", "source.dds") } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.bsa") } -> std::same_as<libbsa::result<void>>;
});

static_assert(requires(libbsa::ba2_gnrl_writer& writer,
                       std::span<const std::byte> bytes,
                       libbsa::ba2_gnrl_entry_options entry_options) {
  { libbsa::ba2_gnrl_target::fallout4 } -> std::same_as<libbsa::ba2_gnrl_target>;
  { libbsa::ba2_gnrl_target::starfield_v2 } -> std::same_as<libbsa::ba2_gnrl_target>;
  { libbsa::ba2_gnrl_target::starfield_v3 } -> std::same_as<libbsa::ba2_gnrl_target>;
  { writer.target() } -> std::same_as<libbsa::ba2_gnrl_target>;
  { writer.options() } -> std::same_as<const libbsa::ba2_gnrl_writer_options&>;
  { writer.add_bytes("Meshes/Memory.nif", bytes) } -> std::same_as<libbsa::result<void>>;
  { writer.add_bytes("Meshes/Memory.nif", bytes, libbsa::entry_compression_policy::raw) }
      -> std::same_as<libbsa::result<void>>;
  { writer.add_bytes("Meshes/Memory.nif", bytes, entry_options) } -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Meshes/Disk.nif", "source.nif") } -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Meshes/Disk.nif", "source.nif", libbsa::entry_compression_policy::compressed) }
      -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Meshes/Disk.nif", "source.nif", entry_options) } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.ba2") } -> std::same_as<libbsa::result<void>>;
});

// BEGIN ba2_dx10_public_contract_assertions
static_assert(requires(libbsa::ba2_dx10_writer& writer) {
  { libbsa::ba2_dx10_target::fallout4 } -> std::same_as<libbsa::ba2_dx10_target>;
  { libbsa::ba2_dx10_target::starfield_v3 } -> std::same_as<libbsa::ba2_dx10_target>;
  { writer.target() } -> std::same_as<libbsa::ba2_dx10_target>;
  { writer.options() } -> std::same_as<const libbsa::ba2_dx10_writer_options&>;
  { writer.add_file("Textures/Diffuse.dds", "source.dds") } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.ba2") } -> std::same_as<libbsa::result<void>>;
});
// END ba2_dx10_public_contract_assertions

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
  [[maybe_unused]] libbsa::tes4_bsa_writer_options writer_options{
      libbsa::archive_compression_policy::target_default, false, false, false};
  [[maybe_unused]] libbsa::tes3_bsa_writer_options tes3_writer_options{};
  [[maybe_unused]] auto target = libbsa::tes4_bsa_target::oblivion;
  [[maybe_unused]] auto ba2_target = libbsa::ba2_gnrl_target::starfield_v3;
  [[maybe_unused]] libbsa::ba2_gnrl_writer_options ba2_options{};
  [[maybe_unused]] libbsa::ba2_gnrl_entry_options ba2_entry_options{};
  [[maybe_unused]] auto ba2_dx10_target = libbsa::ba2_dx10_target::fallout4;
  [[maybe_unused]] auto ba2_dx10_starfield_target = libbsa::ba2_dx10_target::starfield_v3;
  [[maybe_unused]] libbsa::ba2_dx10_writer_options ba2_dx10_options{};
  [[maybe_unused]] auto compression = libbsa::entry_compression_policy::inherit;

  REQUIRE(result.has_value());
}

TEST_CASE("public_include_boundary DX10 writer contract exposes no raw override surface", "[unit][public-api]") {
  // Phase 9 intentionally corrects the stale SPEC raw/compressed override wording:
  // the public DX10 writer is DDS-host-file-only and compressed-only at archive level.
  const auto test_file_path = std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "unit" /
                              "public_include_boundary_tests.cpp";
  std::ifstream file{test_file_path};
  REQUIRE(file.is_open());

  std::ostringstream contents;
  contents << file.rdbuf();
  const auto text = contents.str();
  const auto begin = text.find("BEGIN ba2_dx10_public_contract_assertions");
  const auto end = text.find("END ba2_dx10_public_contract_assertions");
  REQUIRE(begin != std::string::npos);
  REQUIRE(end != std::string::npos);
  REQUIRE(begin < end);

  const auto dx10_contract = text.substr(begin, end - begin);
  constexpr auto forbidden_dx10_tokens = std::to_array<std::string_view>({"ba2_dx10_entry_options",
                                                                         "entry_compression_policy",
                                                                         "add_bytes",
                                                                         "chunk_compression"});
  for (const auto token : forbidden_dx10_tokens) {
    INFO("DX10 public contract token: " << token);
    REQUIRE(dx10_contract.find(token) == std::string::npos);
  }
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
