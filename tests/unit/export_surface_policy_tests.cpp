#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/export.hpp>
#include <libbsa/libbsa.hpp>
#include <libbsa/result.hpp>
#include <libbsa/validation.hpp>
#include <libbsa/writer.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#ifndef LIBBSA_API
#error "libbsa/export.hpp must define LIBBSA_API for public headers"
#endif

namespace
{

  std::filesystem::path source_root()
  {
    return std::filesystem::path{LIBBSA_SOURCE_DIR};
  }

  std::string read_text_file(const std::filesystem::path &path)
  {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
  }

} // namespace

TEST_CASE("export_surface public export header documents macro modes", "[unit][public-api][export_surface]")
{
  const auto header = read_text_file(source_root() / "include/libbsa/export.hpp");

  REQUIRE(header.find("LIBBSA_API") != std::string::npos);
  REQUIRE(header.find("LIBBSA_STATIC_DEFINE") != std::string::npos);
  REQUIRE(header.find("LIBBSA_BUILDING_LIBRARY") != std::string::npos);
  REQUIRE(header.find("__declspec(dllexport)") != std::string::npos);
  REQUIRE(header.find("__declspec(dllimport)") != std::string::npos);
  REQUIRE(header.find("///") != std::string::npos);
}

TEST_CASE("export_surface root CMake installs export header and disables auto export",
          "[unit][public-api][export_surface]")
{
  const auto cmake = read_text_file(source_root() / "CMakeLists.txt");

  REQUIRE(cmake.find("include/libbsa/export.hpp") != std::string::npos);
  REQUIRE(cmake.find("LIBBSA_BUILDING_LIBRARY") != std::string::npos);
  REQUIRE(cmake.find("LIBBSA_STATIC_DEFINE") != std::string::npos);
  REQUIRE(cmake.find("WINDOWS_EXPORT_ALL_SYMBOLS ON") == std::string::npos);
  REQUIRE(cmake.find("WINDOWS_EXPORT_ALL_SYMBOLS TRUE") == std::string::npos);
}

TEST_CASE("export_surface public emitted APIs are explicitly annotated", "[unit][public-api][export_surface]")
{
  const auto archive_header = read_text_file(source_root() / "include/libbsa/archive.hpp");
  const auto writer_header = read_text_file(source_root() / "include/libbsa/writer.hpp");
  const auto validation_header = read_text_file(source_root() / "include/libbsa/validation.hpp");
  const auto result_header = read_text_file(source_root() / "include/libbsa/result.hpp");

  REQUIRE(archive_header.find("#include <libbsa/export.hpp>") != std::string::npos);
  REQUIRE(writer_header.find("#include <libbsa/export.hpp>") != std::string::npos);
  REQUIRE(validation_header.find("#include <libbsa/export.hpp>") != std::string::npos);
  REQUIRE(result_header.find("LIBBSA_API") == std::string::npos);

  constexpr auto required_archive_tokens = std::to_array<std::string_view>({
      "class LIBBSA_API payload_sink",
      "class LIBBSA_API bulk_extract_sink_factory",
      "static LIBBSA_API result<archive_reader> open",
      "LIBBSA_API result<archive_metadata> metadata",
  });
  for (const auto token : required_archive_tokens)
  {
    INFO("archive public API token: " << token);
    REQUIRE(archive_header.find(token) != std::string::npos);
  }

  constexpr auto required_writer_tokens = std::to_array<std::string_view>({
      "LIBBSA_API explicit tes4_bsa_writer",
      "LIBBSA_API tes3_bsa_writer()",
      "LIBBSA_API explicit ba2_gnrl_writer",
      "LIBBSA_API explicit ba2_dx10_writer",
  });
  for (const auto token : required_writer_tokens)
  {
    INFO("writer public API token: " << token);
    REQUIRE(writer_header.find(token) != std::string::npos);
  }

  REQUIRE(validation_header.find("LIBBSA_API bool is_valid() const noexcept") != std::string::npos);
  REQUIRE(validation_header.find("LIBBSA_API result<validation_report> validate_archive") != std::string::npos);
}
