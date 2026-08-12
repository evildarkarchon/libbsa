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

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

}  // namespace

TEST_CASE("export_surface root CMake installs export header and disables auto export",
          "[unit][public-api][export_surface][export_surface_policy]") {
    const auto cmake = read_text_file(source_root() / "CMakeLists.txt");

    REQUIRE(cmake.find("include/libbsa/export.hpp") != std::string::npos);
    REQUIRE(cmake.find("LIBBSA_BUILDING_LIBRARY") != std::string::npos);
    REQUIRE(cmake.find("LIBBSA_STATIC_DEFINE") != std::string::npos);
    REQUIRE(cmake.find("WINDOWS_EXPORT_ALL_SYMBOLS ON") == std::string::npos);
    REQUIRE(cmake.find("WINDOWS_EXPORT_ALL_SYMBOLS TRUE") == std::string::npos);
}

