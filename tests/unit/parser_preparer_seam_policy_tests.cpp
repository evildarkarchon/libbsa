#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <span>
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

std::string function_body(std::string_view source, std::string_view signature) {
    const auto start = source.find(signature);
    REQUIRE(start != std::string_view::npos);

    const auto body_start = source.find('{', start);
    REQUIRE(body_start != std::string_view::npos);

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

void require_all_tokens(std::string_view text, std::span<const std::string_view> tokens) {
    for (const auto token : tokens) {
        INFO("missing token: " << token);
        REQUIRE(text.find(token) != std::string_view::npos);
    }
}

void require_absent_tokens(std::string_view body,
                           std::span<const std::string_view> forbidden_tokens) {
    for (const auto token : forbidden_tokens) {
        INFO("forbidden token: " << token);
        REQUIRE(body.find(token) == std::string_view::npos);
    }
}

}  // namespace

TEST_CASE("parser_preparer_seam_policy requires dedicated TES4 parser seams",
          "[unit][parser_preparer_seam_policy]") {
    const auto root = source_root();
    const auto parser = read_text_file(root / "src/formats/bsa/tes4_bsa_parser.cpp");
    const auto table_header = read_text_file(root / "src/formats/bsa/tes4_bsa_table.hpp");
    const auto payload_header =
        read_text_file(root / "src/formats/bsa/tes4_bsa_payload_descriptor.hpp");

    constexpr auto parser_evidence = std::to_array<std::string_view>({
        "#include \"formats/bsa/tes4_bsa_table.hpp\"",
        "#include \"formats/bsa/tes4_bsa_payload_descriptor.hpp\"",
        "read_tes4_bsa_raw_table",
        "make_tes4_bsa_payload_descriptor",
    });
    require_all_tokens(parser, parser_evidence);

    constexpr auto table_role_evidence = std::to_array<std::string_view>({
        "tes4_bsa_raw_table",
        "tes4_bsa_header_fields",
        "metadata_table_size",
        "folder_records",
        "folder_blocks",
        "file_names",
        "without materializing entries",
    });
    require_all_tokens(table_header, table_role_evidence);

    constexpr auto payload_role_evidence = std::to_array<std::string_view>({
        "tes4_bsa_payload_descriptor",
        "tes4_bsa_compression_for",
        "tes4_bsa_stored_payload_size",
        "embedded_prefix_size",
        "raw_size",
        "payload spans outside the archive or",
        "overlapping metadata",
    });
    require_all_tokens(payload_header, payload_role_evidence);

    const auto parse_body =
        function_body(parser, "result<tes4_bsa_archive> parse_tes4_bsa_archive_impl(");
    constexpr auto collapsed_table_and_payload_tokens = std::to_array<std::string_view>({
        "detail::binary_reader",
        "read_file_names",
        "read_folder_records",
        "read_folder_blocks",
        "read_u32_le",
        "embedded_prefix",
        "raw_size",
        "stored_size",
        "canonical_paths",
        "hash_tes4",
    });
    require_absent_tokens(parse_body, collapsed_table_and_payload_tokens);
}

TEST_CASE("parser_preparer_seam_policy requires dedicated BA2 DX10 preparer seams",
          "[unit][parser_preparer_seam_policy]") {
    const auto root = source_root();
    const auto prepare = read_text_file(root / "src/formats/ba2/ba2_dx10_prepare.cpp");
    const auto snapshot_header =
        read_text_file(root / "src/formats/ba2/ba2_dx10_snapshot_builder.hpp");
    const auto chunk_header = read_text_file(root / "src/formats/ba2/ba2_dx10_chunk_assembler.hpp");

    constexpr auto prepare_evidence = std::to_array<std::string_view>({
        "#include \"formats/ba2/ba2_dx10_snapshot_builder.hpp\"",
        "#include \"formats/ba2/ba2_dx10_chunk_assembler.hpp\"",
        "ba2_dx10_build_writer_entry_snapshot",
        "ba2_dx10_assemble_chunk",
        "ba2_dx10_assemble_planned_entry",
    });
    require_all_tokens(prepare, prepare_evidence);

    constexpr auto snapshot_role_evidence = std::to_array<std::string_view>({
        "ba2_dx10_validate_texture_format_for_target",
        "ba2_dx10_ensure_snapshot_directory",
        "ba2_dx10_build_writer_entry_snapshot",
        "loading, validating, and snapshotting DDS",
        "source bytes. Snapshot files intentionally outlive",
        "Snapshot files intentionally outlive the source path",
    });
    require_all_tokens(snapshot_header, snapshot_role_evidence);

    constexpr auto chunk_role_evidence = std::to_array<std::string_view>({
        "ba2_dx10_assemble_chunk",
        "ba2_dx10_assemble_planned_entry",
        "Snapshot bytes are streamed into only the current chunk",
        "buffer to preserve bounded-memory staging",
        "indexed",
        "work-result placement",
    });
    require_all_tokens(chunk_header, chunk_role_evidence);

    const auto make_entry_body =
        function_body(prepare, "result<ba2_dx10_writer_entry> ba2_dx10_make_writer_entry(");
    const auto prepare_chunk_body =
        function_body(prepare, "result<ba2_dx10_prepared_chunk> ba2_dx10_prepare_chunk(");
    const auto prepare_entries_body = function_body(
        prepare, "result<std::vector<ba2_dx10_prepared_entry>> ba2_dx10_prepare_entries(");

    constexpr auto snapshot_collapse_tokens = std::to_array<std::string_view>({
        "resolve_host_file_path",
        "read_host_file_exact",
        "analyze_dds_source",
        "write_snapshot",
        "create_directory",
        "BCryptGenRandom",
    });
    require_absent_tokens(make_entry_body, snapshot_collapse_tokens);

    constexpr auto chunk_collapse_tokens = std::to_array<std::string_view>({
        "plan_dx10_chunks(",
        "for_each_host_file_chunk(",
        "compress_payload(",
        "run_indexed_work(",
        "compression_method_for(",
        "collect_chunk_snapshots(",
    });
    require_absent_tokens(prepare_chunk_body, chunk_collapse_tokens);
    require_absent_tokens(prepare_entries_body, chunk_collapse_tokens);
}
