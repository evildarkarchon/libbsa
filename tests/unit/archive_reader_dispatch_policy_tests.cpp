#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input{path};
    REQUIRE(input.is_open());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string function_body(std::string_view source, std::string_view signature) {
    const auto start = source.find(signature);
    REQUIRE(start != std::string::npos);

    const auto body_start = source.find('{', start);
    REQUIRE(body_start != std::string::npos);

    std::size_t depth = 0;
    for (std::size_t cursor = body_start; cursor < source.size(); ++cursor) {
        if (source[cursor] == '{') {
            ++depth;
        }
        if (source[cursor] == '}') {
            REQUIRE(depth > 0U);
            --depth;
            if (depth == 0U) {
                return std::string{source.substr(body_start, cursor - body_start + 1U)};
            }
        }
    }

    FAIL("function body was not closed");
    return {};
}

void require_absent_tokens(std::string_view body,
                           std::span<const std::string_view> forbidden_tokens) {
    for (const auto token : forbidden_tokens) {
        INFO("forbidden token: " << token);
        REQUIRE(body.find(token) == std::string_view::npos);
    }
}

std::string archive_reader_state_body(std::string_view source) {
    const auto start = source.find("struct archive_reader::state");
    REQUIRE(start != std::string::npos);

    const auto body_start = source.find('{', start);
    REQUIRE(body_start != std::string::npos);

    const auto end = source.find("};", body_start);
    REQUIRE(end != std::string::npos);
    return std::string{source.substr(body_start, end - body_start)};
}

}  // namespace

TEST_CASE(
    "archive_reader_dispatch_policy forbids repeated family dispatch in "
    "public reader methods",
    "[unit][archive_reader_dispatch_policy]") {
    const auto archive_text = read_text_file(source_root() / "src/archive.cpp");

    const auto entries_body = function_body(archive_text, "archive_reader::entries() const");
    const auto find_body = function_body(archive_text, "archive_reader::find(");
    const auto contains_body = function_body(archive_text, "archive_reader::contains(");
    const auto extract_body = function_body(archive_text, "archive_reader::extract(");
    const auto extract_bytes_body = function_body(archive_text, "archive_reader::extract_bytes(");
    const auto extract_entries_body =
        function_body(archive_text, "archive_reader::extract_entries(");

    constexpr auto forbidden_dispatch_tokens = std::to_array<std::string_view>({
        "metadata.variant",
        "metadata.type",
        "is_ba2_dx10",
        "backend_identity",
        "reader_backend_identity::",
        "tes3_bsa_backend",
        "tes4_bsa_backend",
        "ba2_gnrl_backend",
        "ba2_dx10_backend",
        "tes3_bsa_entries",
        "tes4_bsa_entries",
        "ba2_gnrl_entries",
        "ba2_dx10_entries",
        "find_tes3_bsa_entry",
        "find_tes4_bsa_entry",
        "find_ba2_gnrl_entry",
        "find_ba2_dx10_entry",
        "contains_tes3_bsa_entry",
        "contains_tes4_bsa_entry",
        "contains_ba2_gnrl_entry",
        "contains_ba2_dx10_entry",
        "extract_tes3_bsa_payload",
        "extract_tes4_bsa_payload_from_file",
        "extract_ba2_gnrl_payload",
        "extract_ba2_dx10_payload",
    });

    require_absent_tokens(entries_body, forbidden_dispatch_tokens);
    require_absent_tokens(find_body, forbidden_dispatch_tokens);
    require_absent_tokens(contains_body, forbidden_dispatch_tokens);
    require_absent_tokens(extract_body, forbidden_dispatch_tokens);
    require_absent_tokens(extract_bytes_body, forbidden_dispatch_tokens);
    require_absent_tokens(extract_entries_body, forbidden_dispatch_tokens);
}

TEST_CASE(
    "archive_reader_dispatch_policy forbids stored backend identity in "
    "reader state",
    "[unit][archive_reader_dispatch_policy]") {
    const auto archive_text = read_text_file(source_root() / "src/archive.cpp");
    const auto state_body = archive_reader_state_body(archive_text);

    CHECK(state_body.find("backend_identity") == std::string::npos);
}
