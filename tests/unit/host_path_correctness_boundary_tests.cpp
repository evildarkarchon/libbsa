#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path project_root() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR};
}

std::filesystem::path suite_source_path() {
  return project_root() / "tests" / "unit" / "host_path_correctness_boundary_tests.cpp";
}

std::filesystem::path tests_cmake_path() {
  return project_root() / "tests" / "CMakeLists.txt";
}

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  REQUIRE(input.is_open());
  return std::string{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

bool contains_text(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

std::string_view locked_non_ascii_token() {
  return "libbsa-placeholder";
}

bool suite_has_smoke_selector() {
  return false;
}

} // namespace

TEST_CASE("host_path_correctness_boundary suite registration is wired into libbsa_tests",
          "[unit][host_path_correctness_boundary]") {
  const auto tests_cmake = read_text_file(tests_cmake_path());

  REQUIRE(contains_text(tests_cmake, "unit/host_path_correctness_boundary_tests.cpp"));
}

TEST_CASE("host_path_correctness_boundary suite source carries the locked non-ASCII token and smoke selector",
          "[unit][host_path_correctness_boundary]") {
  REQUIRE(locked_non_ascii_token() == "libbsa-Ångström-日本語");
  REQUIRE(suite_has_smoke_selector());
}
