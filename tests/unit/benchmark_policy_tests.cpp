#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

void require_all_tokens(std::string_view text, std::initializer_list<std::string_view> tokens) {
    for (const auto token : tokens) {
        INFO("Missing token: " << token);
        REQUIRE(text.find(token) != std::string_view::npos);
    }
}

void require_no_tokens(std::string_view text, std::initializer_list<std::string_view> tokens) {
    for (const auto token : tokens) {
        INFO("Forbidden token: " << token);
        REQUIRE(text.find(token) == std::string_view::npos);
    }
}

}  // namespace

TEST_CASE(
    "benchmark_policy static boundary exposes explicit opt-in benchmark "
    "report tooling",
    "[unit][benchmark_policy][static_boundary]") {
    const auto root = source_root();
    const auto cmake = read_text_file(root / "CMakeLists.txt");
    const auto tests_cmake = read_text_file(root / "tests" / "CMakeLists.txt");

    require_all_tokens(
        cmake, {"LIBBSA_BUILD_BENCHMARKS", "libbsa_benchmarks", "benchmarks/libbsa_benchmarks.cpp",
                "libbsa_benchmark_report", "libbsa-benchmark.json", "libbsa-benchmark.md"});
    REQUIRE(tests_cmake.find("unit/benchmark_policy_tests.cpp") != std::string::npos);
}

TEST_CASE(
    "build_policy static boundary lets BUILD_TESTING disable test "
    "dependency discovery",
    "[unit][build_policy][static_boundary]") {
    const auto cmake = read_text_file(source_root() / "CMakeLists.txt");

    require_all_tokens(
        cmake,
        {"include(CTest)", "option(LIBBSA_BUILD_TESTS \"Build libbsa tests\" ${BUILD_TESTING})",
         "if(BUILD_TESTING AND LIBBSA_BUILD_TESTS)", "add_subdirectory(tests)"});
    REQUIRE(cmake.find("if(LIBBSA_BUILD_TESTS)") == std::string_view::npos);
    REQUIRE(cmake.find("enable_testing()") == std::string_view::npos);
}

TEST_CASE(
    "benchmark_policy static boundary keeps report generation out of "
    "default CTest timing gates",
    "[unit][benchmark_policy][static_boundary]") {
    const auto root = source_root();
    const auto cmake = read_text_file(root / "CMakeLists.txt");
    const auto tests_cmake = read_text_file(root / "tests" / "CMakeLists.txt");

    REQUIRE(cmake.find("add_custom_target(libbsa_benchmark_report") != std::string_view::npos);
    REQUIRE(cmake.find("add_test") == std::string_view::npos);
    require_no_tokens(tests_cmake, {"libbsa_benchmark_report", "libbsa_benchmarks",
                                    "libbsa-benchmark.json", "libbsa-benchmark.md", "elapsed_ms"});
}

TEST_CASE("benchmark_policy static boundary rejects fixed speedup threshold gates",
          "[unit][benchmark_policy][static_boundary]") {
    const auto root = source_root();
    const std::array files{
        root / "CMakeLists.txt",
        root / "benchmarks" / "libbsa_benchmarks.cpp",
        root / "benchmarks" / "README.md",
        root / "tests" / "unit" / "benchmark_policy_tests.cpp",
    };

    for (const auto& file : files) {
        const auto text = read_text_file(file);
        INFO("file: " << file.string());
        require_no_tokens(text, {"REQUIRE("
                                 "speedup",
                                 "CHECK("
                                 "speedup",
                                 "REQUIRE(.*"
                                 "speedup",
                                 "CHECK(.*"
                                 "speedup",
                                 "speedup "
                                 ">=",
                                 "speedup"
                                 "_threshold",
                                 "Google "
                                 "Benchmark"});
    }
}
