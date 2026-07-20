#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct source_unit {
    std::filesystem::path path;
    std::string text;
    std::string code;
};

struct source_range {
    std::size_t begin;
    std::size_t end;
};

struct control_condition {
    std::string_view keyword;
    std::string_view text;
};

std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input{path};
    REQUIRE(input.is_open());

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool is_identifier_character(char character) noexcept {
    return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_';
}

bool contains_word(std::string_view text, std::string_view word) noexcept {
    for (auto position = text.find(word); position != std::string_view::npos;
         position = text.find(word, position + word.size())) {
        const bool starts_word = position == 0U || !is_identifier_character(text[position - 1U]);
        const auto after = position + word.size();
        const bool ends_word = after == text.size() || !is_identifier_character(text[after]);
        if (starts_word && ends_word) {
            return true;
        }
    }
    return false;
}

/// Removes comments and literals while preserving source positions and control-flow punctuation.
std::string code_only(std::string_view source) {
    enum class scan_state {
        code,
        line_comment,
        block_comment,
        string_literal,
        character_literal,
    };

    std::string output{source};
    scan_state state = scan_state::code;
    bool escaped = false;
    for (std::size_t index = 0U; index < source.size(); ++index) {
        const auto character = source[index];
        const auto next = index + 1U < source.size() ? source[index + 1U] : '\0';

        switch (state) {
            case scan_state::code:
                if (character == '/' && next == '/') {
                    output[index] = ' ';
                    state = scan_state::line_comment;
                } else if (character == '/' && next == '*') {
                    output[index] = ' ';
                    state = scan_state::block_comment;
                } else if (character == '"') {
                    output[index] = ' ';
                    state = scan_state::string_literal;
                    escaped = false;
                } else if (character == '\'') {
                    output[index] = ' ';
                    state = scan_state::character_literal;
                    escaped = false;
                }
                break;
            case scan_state::line_comment:
                if (character == '\n') {
                    state = scan_state::code;
                } else {
                    output[index] = ' ';
                }
                break;
            case scan_state::block_comment:
                if (character == '*' && next == '/') {
                    output[index] = ' ';
                    output[index + 1U] = ' ';
                    ++index;
                    state = scan_state::code;
                } else if (character != '\n') {
                    output[index] = ' ';
                }
                break;
            case scan_state::string_literal:
            case scan_state::character_literal: {
                const auto closing_character = state == scan_state::string_literal ? '"' : '\'';
                if (character != '\n') {
                    output[index] = ' ';
                }
                if (escaped) {
                    escaped = false;
                } else if (character == '\\') {
                    escaped = true;
                } else if (character == closing_character) {
                    state = scan_state::code;
                }
                break;
            }
        }
    }
    return output;
}

bool is_cpp_source(const std::filesystem::path& path) {
    constexpr std::array<std::string_view, 5> extensions{".cpp", ".cc", ".cxx", ".h", ".hpp"};
    const auto extension = path.extension().string();
    for (const auto candidate : extensions) {
        if (extension == candidate) {
            return true;
        }
    }
    return false;
}

/// Discovers production C++ units so policy ownership survives source-file moves and splits.
std::vector<source_unit> production_source_units() {
    std::vector<source_unit> units;
    for (const auto& directory : {source_root() / "include", source_root() / "src"}) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator{directory}) {
            if (!entry.is_regular_file() || !is_cpp_source(entry.path())) {
                continue;
            }
            auto text = read_text_file(entry.path());
            units.push_back(
                source_unit{entry.path().lexically_relative(source_root()), text, code_only(text)});
        }
    }
    return units;
}

/// Returns the closing brace paired with an already-validated opening brace.
std::size_t matching_brace(std::string_view code, std::size_t opening_brace) noexcept {
    std::size_t depth = 0U;
    for (auto position = opening_brace; position < code.size(); ++position) {
        if (code[position] == '{') {
            ++depth;
        } else if (code[position] == '}') {
            if (--depth == 0U) {
                return position;
            }
        }
    }
    return std::string_view::npos;
}

bool position_is_in_range(std::size_t position, const std::vector<source_range>& ranges) noexcept {
    for (const auto& range : ranges) {
        if (position >= range.begin && position < range.end) {
            return true;
        }
    }
    return false;
}

/// Finds an exact canonical class token while allowing ordinary whitespace before its name.
std::size_t find_class_declaration(std::string_view code, std::string_view profile_name,
                                   std::size_t start = 0U) noexcept {
    for (auto position = code.find("class", start); position != std::string_view::npos;
         position = code.find("class", position + 5U)) {
        const bool starts_word = position == 0U || !is_identifier_character(code[position - 1U]);
        const auto after_keyword = position + 5U;
        const bool ends_word =
            after_keyword == code.size() || !is_identifier_character(code[after_keyword]);
        if (!starts_word || !ends_word) {
            continue;
        }

        auto name_position = after_keyword;
        while (name_position < code.size() &&
               std::isspace(static_cast<unsigned char>(code[name_position])) != 0) {
            ++name_position;
        }
        if (code.substr(name_position, profile_name.size()) != profile_name) {
            continue;
        }
        const auto after_name = name_position + profile_name.size();
        if (after_name == code.size() || !is_identifier_character(code[after_name])) {
            return position;
        }
    }
    return std::string_view::npos;
}

/// Discovers class, member-function, and checked-factory scopes for a canonical profile type.
std::vector<source_range> profile_semantic_ranges(std::string_view code,
                                                  std::string_view profile_name) {
    std::vector<source_range> ranges;
    for (auto position = find_class_declaration(code, profile_name);
         position != std::string_view::npos;
         position = find_class_declaration(code, profile_name, position + 5U)) {
        const auto terminator = code.find_first_of("{;", position + 5U + profile_name.size());
        if (terminator == std::string_view::npos || code[terminator] != '{') {
            continue;
        }
        const auto closing_brace = matching_brace(code, terminator);
        if (closing_brace != std::string_view::npos) {
            ranges.push_back(source_range{position, closing_brace + 1U});
        }
    }

    const auto qualified_member = std::string{profile_name} + "::";
    const auto checked_factory_return = "result<" + std::string{profile_name} + ">";
    for (auto opening_brace = code.find('{'); opening_brace != std::string_view::npos;
         opening_brace = code.find('{', opening_brace + 1U)) {
        if (position_is_in_range(opening_brace, ranges)) {
            continue;
        }
        const auto prior_delimiter = opening_brace == 0U
                                         ? std::string_view::npos
                                         : code.find_last_of(";{}", opening_brace - 1U);
        const auto signature_start =
            prior_delimiter == std::string_view::npos ? 0U : prior_delimiter + 1U;
        const auto signature = code.substr(signature_start, opening_brace - signature_start);
        const auto parameter_list = signature.find('(');
        const auto member_qualifier = signature.find(qualified_member);
        const auto factory_return = signature.find(checked_factory_return);
        const bool is_member_definition =
            parameter_list != std::string_view::npos && member_qualifier < parameter_list;
        const bool is_checked_factory =
            parameter_list != std::string_view::npos && factory_return < parameter_list;
        if (!is_member_definition && !is_checked_factory) {
            continue;
        }
        const auto closing_brace = matching_brace(code, opening_brace);
        if (closing_brace != std::string_view::npos) {
            ranges.push_back(source_range{signature_start, closing_brace + 1U});
            opening_brace = closing_brace;
        }
    }
    return ranges;
}

/// Blanks profile-owned scopes while retaining offsets, newlines, and all surrounding code.
std::string without_ranges(std::string_view code, const std::vector<source_range>& ranges) {
    std::string remaining{code};
    for (const auto& range : ranges) {
        for (auto position = range.begin; position < range.end; ++position) {
            if (remaining[position] != '\n') {
                remaining[position] = ' ';
            }
        }
    }
    return remaining;
}

/// Detects a direct include of the other canonical profile without depending on its path.
bool includes_profile_header(std::string_view source, std::string_view profile_name) {
    std::istringstream lines{std::string{source}};
    for (std::string line; std::getline(lines, line);) {
        const auto first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line.compare(first, 8U, "#include") == 0 &&
            line.find(profile_name, first + 8U) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool is_tes4_bsa_unit(std::string_view code) noexcept {
    return code.find("tes4_") != std::string_view::npos ||
           code.find("archive_variant::tes4") != std::string_view::npos;
}

bool is_constant_declaration(std::string_view line, std::string_view token) noexcept {
    return line.find("inline constexpr") != std::string_view::npos &&
           line.find(token) != std::string_view::npos && line.find('=') != std::string_view::npos;
}

/// Detects direct TES4 version identities outside their syntax declarations.
bool uses_direct_tes4_version_identity(std::string_view code) {
    constexpr std::array<std::string_view, 3> version_tokens{
        "tes4_bsa_oblivion_version",
        "tes4_bsa_fallout3_version",
        "tes4_bsa_skyrim_se_version",
    };

    std::istringstream lines{std::string{code}};
    for (std::string line; std::getline(lines, line);) {
        for (const auto token : version_tokens) {
            if (line.find(token) != std::string::npos && !is_constant_declaration(line, token)) {
                return true;
            }
        }
    }
    return false;
}

/// Matches an integer token with optional unsigned/long suffixes, not a numeric substring.
bool contains_integer_literal(std::string_view text, std::string_view literal) noexcept {
    for (auto position = text.find(literal); position != std::string_view::npos;
         position = text.find(literal, position + literal.size())) {
        if (position != 0U && is_identifier_character(text[position - 1U])) {
            continue;
        }
        auto after = position + literal.size();
        while (after < text.size() && (text[after] == 'u' || text[after] == 'U' ||
                                       text[after] == 'l' || text[after] == 'L')) {
            ++after;
        }
        if (after == text.size() || !is_identifier_character(text[after])) {
            return true;
        }
    }
    return false;
}

/// Detects supported TES4 version literals outside their named syntax declarations.
bool uses_supported_tes4_version_literal(std::string_view code) {
    constexpr std::array<std::string_view, 9> version_literals{
        "103", "104", "105", "0x67", "0X67", "0x68", "0X68", "0x69", "0X69",
    };

    std::istringstream lines{std::string{code}};
    for (std::string line; std::getline(lines, line);) {
        if (line.find("inline constexpr") != std::string::npos &&
            line.find("_version") != std::string::npos) {
            continue;
        }
        for (const auto literal : version_literals) {
            if (contains_integer_literal(line, literal)) {
                return true;
            }
        }
    }
    return false;
}

/// Counts identifier-delimited occurrences of one source token.
std::size_t count_words(std::string_view text, std::string_view word) noexcept {
    std::size_t count = 0U;
    for (auto position = text.find(word); position != std::string_view::npos;
         position = text.find(word, position + word.size())) {
        const bool starts_word = position == 0U || !is_identifier_character(text[position - 1U]);
        const auto after = position + word.size();
        const bool ends_word = after == text.size() || !is_identifier_character(text[after]);
        if (starts_word && ends_word) {
            ++count;
        }
    }
    return count;
}

/// Extracts `if` and `switch` conditions without depending on function or file names.
std::vector<control_condition> control_conditions(std::string_view code) {
    std::vector<control_condition> conditions;
    for (std::size_t position = 0U; position < code.size(); ++position) {
        std::string_view keyword;
        if (code.substr(position, 2U) == "if" &&
            (position == 0U || !is_identifier_character(code[position - 1U])) &&
            (position + 2U == code.size() || !is_identifier_character(code[position + 2U]))) {
            keyword = "if";
        } else if (code.substr(position, 6U) == "switch" &&
                   (position == 0U || !is_identifier_character(code[position - 1U])) &&
                   (position + 6U == code.size() ||
                    !is_identifier_character(code[position + 6U]))) {
            keyword = "switch";
        } else {
            continue;
        }

        auto cursor = position + keyword.size();
        while (cursor < code.size() &&
               std::isspace(static_cast<unsigned char>(code[cursor])) != 0) {
            ++cursor;
        }
        if (code.substr(cursor, 9U) == "constexpr") {
            cursor += 9U;
            while (cursor < code.size() &&
                   std::isspace(static_cast<unsigned char>(code[cursor])) != 0) {
                ++cursor;
            }
        }
        if (cursor == code.size() || code[cursor] != '(') {
            continue;
        }

        const auto condition_start = cursor + 1U;
        std::size_t depth = 1U;
        for (++cursor; cursor < code.size(); ++cursor) {
            if (code[cursor] == '(') {
                ++depth;
            } else if (code[cursor] == ')') {
                --depth;
                if (depth == 0U) {
                    conditions.push_back(control_condition{
                        keyword, code.substr(condition_start, cursor - condition_start)});
                    position = cursor;
                    break;
                }
            }
        }
    }
    return conditions;
}

/// Recognizes the allowed equality check between two transported version values.
bool is_authoritative_version_validation(std::string_view condition) {
    const bool compares_values = condition.find("==") != std::string_view::npos ||
                                 condition.find("!=") != std::string_view::npos;
    return compares_values && count_words(condition, "version") >= 2U &&
           !uses_direct_tes4_version_identity(condition) &&
           !uses_supported_tes4_version_literal(condition) && !contains_word(condition, "target");
}

/// Returns whether a condition chooses TES4 behavior from a direct version or target identity.
bool branches_on_direct_tes4_identity(std::string_view condition) {
    if (contains_word(condition, "target")) {
        return true;
    }
    if (!contains_word(condition, "version") || is_authoritative_version_validation(condition)) {
        return false;
    }
    return condition.find("==") != std::string_view::npos ||
           condition.find("!=") != std::string_view::npos ||
           uses_direct_tes4_version_identity(condition) ||
           uses_supported_tes4_version_literal(condition);
}

/// Detects direct TES4 version or target identities in conditional-expression selectors.
bool direct_tes4_identity_controls_ternary(std::string_view code) {
    for (auto question = code.find('?'); question != std::string_view::npos;
         question = code.find('?', question + 1U)) {
        const auto boundary = code.find_last_of(";{}\n", question);
        const auto condition_start = boundary == std::string_view::npos ? 0U : boundary + 1U;
        const auto condition = code.substr(condition_start, question - condition_start);
        if (branches_on_direct_tes4_identity(condition)) {
            return true;
        }
    }
    return false;
}

/// Detects inheritance on a canonical profile class without inspecting its private layout.
bool declares_profile_base(std::string_view code, std::string_view profile_name) {
    for (auto position = find_class_declaration(code, profile_name);
         position != std::string_view::npos;
         position = find_class_declaration(code, profile_name, position + 5U)) {
        const auto end = code.find_first_of("{;", position + 5U + profile_name.size());
        if (end != std::string_view::npos &&
            code.find(':', position + 5U + profile_name.size()) < end) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST_CASE("TES4 BSA Profile exclusively owns direct version and target policy selection",
          "[unit][tes4_bsa_profile][tes4_bsa_profile_ownership_policy]") {
    CHECK(branches_on_direct_tes4_identity("version == 105U"));
    CHECK(branches_on_direct_tes4_identity("target == tes4_bsa_target::skyrim_se"));
    CHECK_FALSE(branches_on_direct_tes4_identity("header.version != profile.version()"));
    CHECK_FALSE(branches_on_direct_tes4_identity("parsed.version != resolved.version()"));
    CHECK(direct_tes4_identity_controls_ternary("version == 105U ? selected : fallback"));

    constexpr std::string_view prefixed_profile_class_source = R"(
        class tes4_bsa_profile_adapter {
            bool selects(std::uint32_t version) {
                return version == 105U;
            }
        };
    )";
    const auto prefixed_class_ranges =
        profile_semantic_ranges(prefixed_profile_class_source, "tes4_bsa_profile");
    CHECK(prefixed_class_ranges.empty());
    CHECK(uses_supported_tes4_version_literal(prefixed_profile_class_source));

    constexpr std::string_view adjacent_helper_source = R"(
        result<tes4_bsa_profile> resolve_profile(std::uint32_t version) {
            switch (version) { case 105U: return make_profile(); }
        }
        bool accepts_profile(result<tes4_bsa_profile> resolved, std::uint32_t version) {
            if (version == 105U) { return resolved.has_value(); }
            return false;
        }
        bool unrelated_helper(std::uint32_t version) {
            if (version == 105U) { return true; }
            return false;
        }
    )";
    const auto synthetic_profile_ranges =
        profile_semantic_ranges(adjacent_helper_source, "tes4_bsa_profile");
    REQUIRE(synthetic_profile_ranges.size() == 1U);
    const auto synthetic_non_profile =
        without_ranges(adjacent_helper_source, synthetic_profile_ranges);
    CHECK(uses_supported_tes4_version_literal(synthetic_non_profile));
    const auto synthetic_conditions = control_conditions(synthetic_non_profile);
    REQUIRE(synthetic_conditions.size() == 2U);
    for (const auto& condition : synthetic_conditions) {
        CHECK(branches_on_direct_tes4_identity(condition.text));
    }

    const auto units = production_source_units();
    REQUIRE_FALSE(units.empty());

    std::size_t tes4_profile_scopes = 0U;
    std::size_t ba2_profile_scopes = 0U;
    std::string tes4_profile_code;
    std::string ba2_profile_code;
    for (const auto& unit : units) {
        const auto tes4_ranges = profile_semantic_ranges(unit.code, "tes4_bsa_profile");
        const auto ba2_ranges = profile_semantic_ranges(unit.code, "ba2_profile");
        if (!tes4_ranges.empty()) {
            tes4_profile_scopes += tes4_ranges.size();
            CHECK_FALSE(includes_profile_header(unit.text, "ba2_profile"));
            for (const auto& range : tes4_ranges) {
                const auto profile_code = unit.code.substr(range.begin, range.end - range.begin);
                tes4_profile_code.append(profile_code);
                CHECK(profile_code.find("ba2_profile") == std::string::npos);
            }
        }
        if (!ba2_ranges.empty()) {
            ba2_profile_scopes += ba2_ranges.size();
            CHECK_FALSE(includes_profile_header(unit.text, "tes4_bsa_profile"));
            for (const auto& range : ba2_ranges) {
                const auto profile_code = unit.code.substr(range.begin, range.end - range.begin);
                ba2_profile_code.append(profile_code);
                CHECK(profile_code.find("tes4_bsa_profile") == std::string::npos);
            }
        }
        if (!is_tes4_bsa_unit(unit.code)) {
            continue;
        }

        const auto non_profile_code = without_ranges(unit.code, tes4_ranges);
        INFO("TES4 production source: " << unit.path.string());
        CHECK_FALSE(uses_direct_tes4_version_identity(non_profile_code));
        CHECK_FALSE(uses_supported_tes4_version_literal(non_profile_code));
        CHECK(non_profile_code.find("tes4_bsa_target::") == std::string::npos);
        CHECK_FALSE(direct_tes4_identity_controls_ternary(non_profile_code));
        for (const auto& condition : control_conditions(non_profile_code)) {
            INFO("control-flow condition: " << condition.text);
            if (condition.keyword == "switch") {
                CHECK_FALSE(contains_word(condition.text, "version"));
                CHECK_FALSE(contains_word(condition.text, "target"));
            } else {
                CHECK_FALSE(branches_on_direct_tes4_identity(condition.text));
            }
        }
    }

    REQUIRE(tes4_profile_scopes > 0U);
    REQUIRE(ba2_profile_scopes > 0U);
    CHECK_FALSE(declares_profile_base(tes4_profile_code, "tes4_bsa_profile"));
    CHECK_FALSE(declares_profile_base(ba2_profile_code, "ba2_profile"));
}
