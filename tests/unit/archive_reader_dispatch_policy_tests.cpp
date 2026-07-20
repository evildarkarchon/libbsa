#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
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

std::string production_source_text() {
    std::string source;
    for (const auto& item : std::filesystem::recursive_directory_iterator{source_root() / "src"}) {
        if (!item.is_regular_file()) {
            continue;
        }
        const auto extension = item.path().extension();
        if (extension != ".cpp" && extension != ".hpp") {
            continue;
        }
        source += read_text_file(item.path());
        source.push_back('\n');
    }
    return source;
}

std::string remove_whitespace(std::string_view text) {
    std::string compact;
    compact.reserve(text.size());
    for (const auto character : text) {
        if (std::isspace(static_cast<unsigned char>(character)) == 0) {
            compact.push_back(character);
        }
    }
    return compact;
}

std::string remove_comments(std::string_view text) {
    auto uncommented = std::regex_replace(std::string{text}, std::regex{R"(//[^\r\n]*)"}, "");
    return std::regex_replace(uncommented, std::regex{R"(/\*[\s\S]*?\*/)"}, "");
}

/// Extracts a brace-balanced definition body without assuming its source file.
std::string declaration_body(std::string_view source, std::string_view declaration) {
    const auto declaration_start = source.find(declaration);
    REQUIRE(declaration_start != std::string_view::npos);

    const auto body_start = source.find('{', declaration_start);
    REQUIRE(body_start != std::string_view::npos);

    std::size_t depth = 0U;
    for (auto cursor = body_start; cursor < source.size(); ++cursor) {
        if (source[cursor] == '{') {
            ++depth;
        } else if (source[cursor] == '}') {
            REQUIRE(depth > 0U);
            --depth;
            if (depth == 0U) {
                return std::string{source.substr(body_start, cursor - body_start + 1U)};
            }
        }
    }

    FAIL("declaration body was not closed");
    return {};
}

/// Derives the extraction function member name from either a direct or aliased declaration.
std::optional<std::string> extraction_function_member_name(std::string_view compact_source,
                                                           std::string_view compact_state) {
    const std::regex direct_member{
        R"(result<void>\(\*([A-Za-z_][A-Za-z0-9_]*)\)\(constdetail::host_file_path&[^,]*,constentry_metadata&[^,]*,payload_sink&[^)]*\))"};
    std::match_results<std::string_view::const_iterator> direct_match;
    if (std::regex_search(compact_state.begin(), compact_state.end(), direct_match,
                          direct_member)) {
        return direct_match[1].str();
    }

    const std::regex callback_alias{
        R"(using([A-Za-z_][A-Za-z0-9_]*)=result<void>\(\*\)\(constdetail::host_file_path&[^,]*,constentry_metadata&[^,]*,payload_sink&[^)]*\);)"};
    const std::string source{compact_source};
    for (auto match = std::sregex_iterator{source.begin(), source.end(), callback_alias};
         match != std::sregex_iterator{}; ++match) {
        const auto alias = (*match)[1].str();
        const std::regex aliased_member{alias + R"(([A-Za-z_][A-Za-z0-9_]*);)"};
        std::match_results<std::string_view::const_iterator> member_match;
        if (std::regex_search(compact_state.begin(), compact_state.end(), member_match,
                              aliased_member)) {
            return member_match[1].str();
        }
    }
    return std::nullopt;
}

}  // namespace

TEST_CASE("Archive Entry Catalog policy forbids catalog callbacks in production sources",
          "[unit][archive_entry_catalog_policy]") {
    const auto source = remove_whitespace(production_source_text());
    constexpr auto forbidden_callback_shapes = std::to_array<std::string_view>({
        "result<std::vector<entry_metadata>>(*",
        "result<std::optional<entry_metadata>>(*",
        "result<bool>(*",
    });

    for (const auto shape : forbidden_callback_shapes) {
        INFO("forbidden catalog callback shape: " << shape);
        REQUIRE(source.find(shape) == std::string::npos);
    }
}

TEST_CASE("archive reader extraction policy keeps family dispatch out of public operations",
          "[unit][reader_extraction_dispatch_policy]") {
    const auto source = production_source_text();
    const auto compact_source = remove_whitespace(remove_comments(source));
    const auto state = remove_whitespace(
        remove_comments(declaration_body(source, "struct archive_reader::state")));
    const auto extraction_member = extraction_function_member_name(compact_source, state);
    REQUIRE(extraction_member.has_value());

    constexpr auto public_operations = std::to_array<std::string_view>({
        "archive_reader::metadata(",
        "archive_reader::entries(",
        "archive_reader::find(",
        "archive_reader::contains(",
        "archive_reader::extract(",
        "archive_reader::extract_bytes(",
        "archive_reader::extract_entries(",
    });
    constexpr auto forbidden_family_decisions = std::to_array<std::string_view>({
        "formats::",
        "archive_type::",
        "archive_variant::",
        "ba2_subtype::",
        "metadata.type",
        "metadata.variant",
        "metadata().type",
        "metadata().variant",
    });

    for (const auto operation : public_operations) {
        const auto body = remove_whitespace(remove_comments(declaration_body(source, operation)));
        INFO("public operation: " << operation);
        for (const auto decision : forbidden_family_decisions) {
            INFO("forbidden family decision: " << decision);
            REQUIRE(body.find(decision) == std::string::npos);
        }
    }

    constexpr auto extraction_operations = std::to_array<std::string_view>({
        "archive_reader::extract(",
        "archive_reader::extract_bytes(",
        "archive_reader::extract_entries(",
    });
    const std::regex direct_invocation{R"([A-Za-z_][A-Za-z0-9_]*->)" + *extraction_member +
                                       R"(\()"};
    for (const auto operation : extraction_operations) {
        const auto body = remove_whitespace(remove_comments(declaration_body(source, operation)));
        const auto lookup = body.find("find(");
        std::smatch invocation;
        INFO("extraction operation: " << operation);
        REQUIRE(lookup != std::string::npos);
        REQUIRE(std::regex_search(body, invocation, direct_invocation));
        REQUIRE(lookup < static_cast<std::size_t>(invocation.position()));
    }
}

TEST_CASE("archive reader extraction state stores only metadata catalog path and direct function",
          "[unit][reader_extraction_dispatch_policy]") {
    const auto source = production_source_text();
    const auto compact_source = remove_whitespace(remove_comments(source));
    const auto state = remove_whitespace(
        remove_comments(declaration_body(source, "struct archive_reader::state")));

    // Exactly four state values prevent a dispatch-only family discriminator or one-member
    // backend wrapper from surviving beside the required reader state.
    REQUIRE(std::count(state.begin(), state.end(), ';') == 4);
    REQUIRE(state.find("archive_metadata") != std::string::npos);
    REQUIRE(state.find("std::vector<entry_metadata>") != std::string::npos);
    REQUIRE(state.find("detail::host_file_path") != std::string::npos);
    REQUIRE(extraction_function_member_name(compact_source, state).has_value());
}

TEST_CASE("archive reader extraction sources contain no obsolete backend terminology",
          "[unit][reader_extraction_dispatch_policy]") {
    const auto source = production_source_text();
    constexpr auto obsolete_terms = std::to_array<std::string_view>({
        "reader_backend",
        "backend_identity",
        "backend_table",
    });

    for (const auto term : obsolete_terms) {
        INFO("obsolete backend term: " << term);
        REQUIRE(source.find(term) == std::string::npos);
    }
}
