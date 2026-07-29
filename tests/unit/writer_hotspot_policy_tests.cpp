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

void require_all_tokens(std::string_view text, std::span<const std::string_view> tokens) {
    for (const auto token : tokens) {
        INFO("missing token: " << token);
        REQUIRE(text.find(token) != std::string_view::npos);
    }
}

void require_absent_tokens(std::string_view text, std::span<const std::string_view> tokens) {
    for (const auto token : tokens) {
        INFO("forbidden token: " << token);
        REQUIRE(text.find(token) == std::string_view::npos);
    }
}

std::string declaration_block(std::string_view source, std::string_view start_token,
                              std::string_view end_token) {
    const auto start = source.find(start_token);
    REQUIRE(start != std::string_view::npos);
    const auto end = source.find(end_token, start + start_token.size());
    REQUIRE(end != std::string_view::npos);
    return std::string{source.substr(start, end - start)};
}

std::vector<std::string> public_declaration_lines(std::string_view class_public_block) {
    std::vector<std::string> declarations;
    std::istringstream lines{std::string{class_public_block}};
    std::string line;
    while (std::getline(lines, line)) {
        const auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos) {
            continue;
        }
        line.erase(0U, first);
        if (line.starts_with("///") || line == "public:" || line == "class ba2_dx10_writer {") {
            continue;
        }
        if (line.find(';') != std::string::npos) {
            declarations.push_back(std::move(line));
        }
    }
    return declarations;
}

}  // namespace

TEST_CASE("writer_hotspot_policy requires truthful BA2 DX10 lifecycle docs",
          "[unit][writer_hotspot_policy][doc_structure]") {
    const auto root = source_root();
    const auto public_header = read_text_file(root / "include/libbsa/writer.hpp");
    const auto target_guide = read_text_file(root / "docs/target-format-guide.md");
    const auto integration_examples = read_text_file(root / "docs/integration-examples.md");
    const auto writer_source = read_text_file(root / "src/formats/ba2/ba2_dx10_writer.cpp");

    constexpr auto public_lifecycle_terms = std::to_array<std::string_view>({
        "BA2 DX10 writer",
        "write_to` consumes",
        "ordinary write attempt",
        "best-effort",
        "snapshot cleanup",
        "error_code::invalid_argument",
    });
    require_all_tokens(public_header, public_lifecycle_terms);

    constexpr auto maintainer_lifecycle_terms = std::to_array<std::string_view>({
        "BA2 DX10 temporary snapshot lifecycle",
        "successful `write_to` completion",
        "ordinary result-returning failure unwinding",
        "Destructor safety-net cleanup",
        "Residual abnormal-termination risk remains",
        "result<void>",
        "primary error",
    });
    require_all_tokens(target_guide, maintainer_lifecycle_terms);

    constexpr auto integration_lifecycle_terms = std::to_array<std::string_view>({
        "example_create_ba2_dx10",
        "Do not reuse a BA2 DX10 writer after `write_to`",
        "best-effort cleanup",
        "invalid_argument",
        "residual temp artifacts",
    });
    require_all_tokens(integration_examples, integration_lifecycle_terms);

    constexpr auto writer_lifecycle_source_terms = std::to_array<std::string_view>({
        "consumed",
        "cleanup_snapshot_dir",
        "snapshot_dir.clear()",
        "error_code::invalid_argument",
        "BA2 DX10 writer has already been consumed",
    });
    require_all_tokens(writer_source, writer_lifecycle_source_terms);

    constexpr auto forbidden_overclaims = std::to_array<std::string_view>({
        "crash-proof cleanup",
        "hard-termination cleanup",
        "OS-shutdown-proof cleanup",
        "guaranteed cleanup after crash",
        "guaranteed cleanup after forced termination",
    });
    require_absent_tokens(public_header, forbidden_overclaims);
    require_absent_tokens(target_guide, forbidden_overclaims);
    require_absent_tokens(integration_examples, forbidden_overclaims);
}

TEST_CASE(
    "writer_hotspot_policy keeps public BA2 DX10 writer declaration "
    "shape stable",
    "[unit][writer_hotspot_policy][public-api][doc_structure]") {
    const auto public_header = read_text_file(source_root() / "include/libbsa/writer.hpp");
    const auto options_block = declaration_block(public_header, "struct ba2_dx10_writer_options",
                                                 "/// Per-entry options for BA2 GNRL payload");
    const auto writer_public_block =
        declaration_block(public_header, "class ba2_dx10_writer", " private:");

    constexpr auto option_fields = std::to_array<std::string_view>({
        "bool overwrite_existing = false;",
        "bool deduplicate_payloads = false;",
        "std::uint32_t max_decoded_chunk_bytes = 0U;",
        "std::uint32_t starfield_unknown1 = 1U;",
        "std::uint32_t starfield_unknown2 = 0U;",
        "std::uint32_t starfield_compression_method = 3U;",
    });
    require_all_tokens(options_block, option_fields);

    const auto declarations = public_declaration_lines(writer_public_block);
    INFO("public ba2_dx10_writer declaration count: " << declarations.size());
    REQUIRE(declarations.size() == 12U);

    constexpr auto writer_declarations = std::to_array<std::string_view>({
        "LIBBSA_API explicit ba2_dx10_writer(ba2_dx10_target target);",
        "LIBBSA_API explicit ba2_dx10_writer(ba2_dx10_target target, "
        "ba2_dx10_writer_options options);",
        "LIBBSA_API ~ba2_dx10_writer();",
        "ba2_dx10_writer(const ba2_dx10_writer&) = delete;",
        "ba2_dx10_writer& operator=(const ba2_dx10_writer&) = delete;",
        "LIBBSA_API ba2_dx10_writer(ba2_dx10_writer&& other) noexcept;",
        "LIBBSA_API ba2_dx10_writer& operator=(ba2_dx10_writer&& other) "
        "noexcept;",
        "[[nodiscard]] LIBBSA_API ba2_dx10_target target() const noexcept;",
        "[[nodiscard]] LIBBSA_API const ba2_dx10_writer_options& options() const "
        "noexcept;",
        "LIBBSA_API result<void> add_file(std::string_view archive_path, "
        "std::string_view dds_host_path);",
        "LIBBSA_API result<void> write_to(std::string_view host_path) const;",
        "LIBBSA_API result<void> write_to(std::string_view host_path,",
        "write_execution_options execution) const;",
    });
    require_all_tokens(writer_public_block, writer_declarations);

    constexpr auto forbidden_expansion_tokens = std::to_array<std::string_view>({
        "add_bytes(",
        "add_memory(",
        "snapshot_dir",
        "cleanup_snapshot",
        "reset(",
        "retry",
    });
    require_absent_tokens(writer_public_block, forbidden_expansion_tokens);
}
