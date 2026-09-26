#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream stream{path};
    REQUIRE(stream.is_open());

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::string trim_copy(std::string value) {
    const auto first = std::find_if(value.begin(), value.end(),
                                    [](unsigned char ch) { return !std::isspace(ch); });
    const auto last = std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
                          return !std::isspace(ch);
                      }).base();

    if (first >= last) {
        return {};
    }
    return std::string{first, last};
}

}  // namespace

TEST_CASE(
    "target_format_policy package consumer examples have docs and CTest "
    "smoke gates",
    "[unit][target_format_policy][package_consumer][doc_structure]") {
    const auto root = source_root();
    const auto docs = read_text_file(root / "docs/integration-examples.md");
    const auto package_source = read_text_file(root / "tests/package-consumer/main.cpp");
    const auto package_cmake = read_text_file(root / "tests/package-consumer/CMakeLists.txt");
    const auto smoke_cmake = read_text_file(root / "tests/package-consumer/smoke.cmake");
    const auto runtime_copy_check =
        read_text_file(root / "tests/package-consumer/verify-runtime-dll-copy.cmake");
    const auto tests_cmake = read_text_file(root / "tests/CMakeLists.txt");

    constexpr std::array<std::string_view, 8> examples{
        "example_open_list_extract",    "example_bulk_extract",     "example_create_tes3_bsa",
        "example_create_tes4_bsa",      "example_create_ba2_gnrl",  "example_create_ba2_dx10",
        "example_handle_result_errors", "example_validate_archive",
    };

    for (const auto example : examples) {
        INFO("Missing integration example: " << example);
        REQUIRE(docs.find("## `" + std::string{example} + "`") != std::string::npos);
        REQUIRE(package_source.find(std::string{example}) != std::string::npos);
    }

    REQUIRE(package_source.find("#include <libbsa/libbsa.hpp>") != std::string::npos);
    REQUIRE(package_source.find("ba2_dx10_target::starfield_v2") != std::string::npos);
    REQUIRE(docs.find("ba2_dx10_target::starfield_v2") != std::string::npos);
    constexpr std::array<std::string_view, 4> forbidden_direct_public_includes{
        "#include <libbsa/archive.hpp>",
        "#include <libbsa/writer.hpp>",
        "#include <libbsa/validation.hpp>",
        "#include <libbsa/result.hpp>",
    };
    for (const auto include : forbidden_direct_public_includes) {
        INFO("Package consumer must use umbrella header only: " << include);
        REQUIRE(package_source.find(std::string{include}) == std::string::npos);
    }

    constexpr std::array<std::pair<std::string_view, std::string_view>, 4> helper_flows{
        std::pair{"find()", "reader.find("},
        std::pair{"contains()", "reader.contains("},
        std::pair{"extract()", "reader.extract("},
        std::pair{"extract_bytes()", "reader.extract_bytes("},
    };
    for (const auto& [doc_token, source_token] : helper_flows) {
        INFO("Missing documented lookup/extraction helper flow: " << doc_token);
        REQUIRE(docs.find(std::string{doc_token}) != std::string::npos);
        REQUIRE(package_source.find(std::string{source_token}) != std::string::npos);
    }

    REQUIRE(package_cmake.find("find_package(libbsa CONFIG REQUIRED)") != std::string::npos);
    REQUIRE(package_cmake.find("target_link_libraries(libbsa_package_consumer "
                               "PRIVATE libbsa::libbsa)") != std::string::npos);
    REQUIRE(package_cmake.find("add_test(NAME libbsa_package_consumer_run") != std::string::npos);
    REQUIRE(package_source.find("libbsa::error_code::io_error") != std::string::npos);

    constexpr std::array<std::string_view, 4> package_runtime_families{
        "TES3 BSA",
        "TES4-family BSA",
        "BA2 GNRL",
        "BA2 DX10",
    };
    for (const auto family : package_runtime_families) {
        INFO("Missing package-consumer runtime family token: " << family);
        REQUIRE(docs.find(std::string{family}) != std::string::npos);
        REQUIRE(package_source.find(std::string{family}) != std::string::npos);
    }

    constexpr std::array<std::string_view, 9> runtime_source_tokens{
        "run_installed_package_archive_runtime_smoke",
        "archive_runtime_case",
        "verify_archive_runtime_case",
        "example_validate_archive",
        "libbsa::validate_archive",
        "reader.extract(",
        "reader.extract_bytes(",
        "example_bulk_extract(",
        "build_tiny_bc1_dds_dxt10_source",
    };
    for (const auto token : runtime_source_tokens) {
        INFO("Missing package-consumer runtime proof source token: " << token);
        REQUIRE(package_source.find(std::string{token}) != std::string::npos);
    }

    REQUIRE(docs.find("package_consumer_smoke") != std::string::npos);
    REQUIRE(docs.find("installed `libbsa::libbsa` target") != std::string::npos);
    REQUIRE(docs.find("opens, validates, and extracts") != std::string::npos);
    REQUIRE(smoke_cmake.find("--install") != std::string::npos);
    REQUIRE(smoke_cmake.find("CMAKE_PREFIX_PATH") != std::string::npos);

    constexpr std::array<std::string_view, 6> forbidden_fixture_dependencies{
        ".gsd/",     ".planning/",           ".audits/",
        "TES5Edit/", "LIBBSA_GAME_FIXTURES", "LIBBSA_BSARCHPRO_EXPECTED",
    };
    for (const auto token : forbidden_fixture_dependencies) {
        INFO("Package-consumer smoke must not depend on ignored/local fixtures: " << token);
        REQUIRE(package_source.find(std::string{token}) == std::string::npos);
        REQUIRE(package_cmake.find(std::string{token}) == std::string::npos);
        REQUIRE(smoke_cmake.find(std::string{token}) == std::string::npos);
        REQUIRE(runtime_copy_check.find(std::string{token}) == std::string::npos);
    }

    REQUIRE(tests_cmake.find("NAME package_consumer_smoke") != std::string::npos);
    REQUIRE(tests_cmake.find("NAME package_consumer_runtime_dll_copy") != std::string::npos);
    REQUIRE(tests_cmake.find("LABELS \"package_consumer;target_format_policy\"") !=
            std::string::npos);
}

