#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_chunk_assembler.hpp"
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "formats/ba2/ba2_profile.hpp"
#include "formats/ba2/ba2_dx10_snapshot_builder.hpp"
#include "texture/dds_layout.hpp"
#include "texture/directxtex_analyzer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_source_dir() {
    return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "source";
}

std::filesystem::path seam_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_ba2_dx10_preparer_seam_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path seam_output_path(std::string name) {
    return seam_test_dir() / std::move(name);
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
    std::ifstream input{path};
    REQUIRE(input.is_open());
    return nlohmann::json::parse(input);
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.is_open());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    REQUIRE_FALSE(input.bad());
    return bytes;
}

void write_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.is_open());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

const nlohmann::json& valid_source_case(const nlohmann::json& manifest, std::string_view id) {
    const auto found =
        std::ranges::find_if(manifest.at("valid_cases"), [id](const nlohmann::json& source_case) {
            return source_case.at("id").get<std::string>() == id;
        });
    REQUIRE(found != manifest.at("valid_cases").end());
    return *found;
}

std::vector<std::byte> repeated_bytes(std::size_t size, std::uint8_t seed) {
    std::vector<std::byte> bytes(size);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::byte>(seed + static_cast<std::uint8_t>(index % 17U));
    }
    return bytes;
}

libbsa::formats::ba2::ba2_profile require_dx10_profile(
    libbsa::ba2_dx10_target target, const libbsa::ba2_dx10_writer_options& options) {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(target, options);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::ba2::ba2_profile require_dx10_profile(
    libbsa::ba2_dx10_target target = libbsa::ba2_dx10_target::fallout4) {
    return require_dx10_profile(target, libbsa::ba2_dx10_writer_options{});
}

libbsa::formats::ba2::ba2_dx10_writer_entry snapshot_entry(
    std::string archive_path, const libbsa::texture::dds_texture_layout& layout,
    std::string snapshot_prefix) {
    libbsa::formats::ba2::ba2_dx10_writer_entry entry;
    entry.archive_path_original = archive_path;
    entry.archive_path_canonical = archive_path;
    const auto cube_maps_raw = static_cast<std::uint16_t>(layout.is_cubemap ? 2049U : 2048U);
    entry.metadata = libbsa::texture_metadata{layout.width,
                                              layout.height,
                                              layout.mip_count,
                                              layout.dxgi_format,
                                              layout.array_size,
                                              layout.is_cubemap,
                                              0U,
                                              cube_maps_raw,
                                              {}};

    const std::uint32_t face_count = layout.is_cubemap ? 6U : 1U;
    for (std::uint32_t array_index = 0; array_index < layout.array_size; ++array_index) {
        for (std::uint32_t face_index = 0; face_index < face_count; ++face_index) {
            for (std::uint32_t mip = 0; mip < layout.mip_count; ++mip) {
                auto mip_size = libbsa::texture::mip_size_for_format(layout, mip);
                REQUIRE(mip_size.has_value());
                auto bytes = repeated_bytes(
                    static_cast<std::size_t>(mip_size.value()),
                    static_cast<std::uint8_t>(array_index * 31U + face_index * 7U + mip + 1U));
                const auto snapshot = seam_output_path(
                    snapshot_prefix + "-" + std::to_string(array_index) + "-" +
                    std::to_string(face_index) + "-" + std::to_string(mip) + ".bin");
                write_binary_file(snapshot, bytes);
                entry.subresources.push_back(libbsa::formats::ba2::ba2_dx10_subresource_snapshot{
                    array_index, face_index, mip, bytes.size(), snapshot});
            }
        }
    }
    return entry;
}

std::vector<std::byte> expected_chunk_bytes(const libbsa::texture::dds_texture_layout& layout,
                                            const libbsa::texture::planned_texture_chunk& planned) {
    std::vector<std::byte> expected;
    expected.reserve(static_cast<std::size_t>(planned.raw_size));
    for (std::uint32_t mip = planned.start_mip; mip <= planned.end_mip; ++mip) {
        auto mip_size = libbsa::texture::mip_size_for_format(layout, mip);
        REQUIRE(mip_size.has_value());
        auto bytes = repeated_bytes(static_cast<std::size_t>(mip_size.value()),
                                    static_cast<std::uint8_t>(planned.array_index * 31U +
                                                              planned.face_index * 7U + mip + 1U));
        expected.insert(expected.end(), bytes.begin(), bytes.end());
    }
    return expected;
}

void require_decoded_chunk_matches(const libbsa::formats::ba2::ba2_dx10_prepared_chunk& chunk,
                                   const libbsa::texture::dds_texture_layout& layout,
                                   const libbsa::texture::planned_texture_chunk& planned) {
    REQUIRE(chunk.payload.size() == chunk.packed_size);
    std::ostringstream stored_bytes{std::ios::binary};
    auto emitted = chunk.payload.emit(stored_bytes);
    REQUIRE(emitted.has_value());
    const auto stored_text = stored_bytes.str();
    const auto stored_payload =
        std::as_bytes(std::span<const char>{stored_text.data(), stored_text.size()});
    auto decoded = libbsa::detail::decompress_payload_exact(
        chunk.compression, stored_payload, static_cast<std::size_t>(planned.raw_size));
    REQUIRE(decoded.has_value());
    CHECK(decoded.value() == expected_chunk_bytes(layout, planned));
}

}  // namespace

static_assert(std::is_move_constructible_v<libbsa::formats::ba2::ba2_dx10_prepared_chunk>);
static_assert(std::is_nothrow_move_constructible_v<libbsa::formats::ba2::ba2_dx10_prepared_chunk>);
static_assert(!std::is_copy_constructible_v<libbsa::formats::ba2::ba2_dx10_prepared_chunk>);
static_assert(
    std::is_same_v<decltype(std::declval<libbsa::formats::ba2::ba2_dx10_prepared_chunk>().payload),
                   libbsa::detail::stored_payload>);

TEST_CASE(
    "BA2 DX10 snapshot builder preserves add-time snapshot immutability "
    "and target validation",
    "[unit][writer-stage][ba2_dx10_writer][parser_preparer_seam]") {
    const auto manifest =
        read_json_file(generated_source_dir() / "ba2_dx10_writer_sources_manifest.json");
    const auto& source_case = valid_source_case(manifest, "bc1_unorm");
    const auto original_bytes =
        read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>());
    auto original_source = libbsa::texture::analyze_dds_source(original_bytes);
    REQUIRE(original_source.has_value());

    const auto scratch_path = seam_output_path("snapshot-builder-source.dds");
    write_binary_file(scratch_path, original_bytes);
    const auto snapshot_dir = seam_output_path("snapshot-builder-snapshots");
    std::filesystem::remove_all(snapshot_dir);
    std::filesystem::create_directories(snapshot_dir);

    auto built = libbsa::formats::ba2::ba2_dx10_build_writer_entry_snapshot(
        source_case.at("archive_path").get<std::string>(), scratch_path.string(),
        libbsa::ba2_dx10_target::fallout4, snapshot_dir, 7U);

    REQUIRE(built.has_value());
    CHECK(built.value().metadata.dxgi_format == original_source.value().metadata.dxgi_format);
    REQUIRE(built.value().subresources.size() == original_source.value().subresources.size());
    const auto malformed_bytes =
        read_binary_file(generated_source_dir() / "ba2_dx10_malformed_truncated.dds");
    write_binary_file(scratch_path, malformed_bytes);
    std::filesystem::remove(scratch_path);

    for (std::size_t index = 0; index < built.value().subresources.size(); ++index) {
        const auto& snapshot = built.value().subresources[index];
        CHECK(read_binary_file(snapshot.snapshot_path) ==
              original_source.value().subresources[index].bytes);
    }

    const auto& starfield_only = valid_source_case(manifest, "bc7_unorm_srgb");
    auto unsupported = libbsa::formats::ba2::ba2_dx10_build_writer_entry_snapshot(
        starfield_only.at("archive_path").get<std::string>(),
        (generated_source_dir() / starfield_only.at("file").get<std::string>()).string(),
        libbsa::ba2_dx10_target::fallout4, snapshot_dir, 8U);
    REQUIRE_FALSE(unsupported.has_value());
    CHECK(unsupported.error().code == libbsa::error_code::format_error);
}

TEST_CASE(
    "BA2 DX10 chunk assembler streams snapshot-backed chunk assembly in "
    "texture order",
    "[unit][writer-stage][ba2_dx10_writer][parser_preparer_seam]") {
    SECTION("multi-mip ordering") {
        const libbsa::texture::dds_texture_layout layout{4U, 4U, 3U, 28U, 1U, false};
        auto source = snapshot_entry("textures/seam/multi.dds", layout, "seam-multi");
        auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
        REQUIRE(planned.has_value());
        REQUIRE(planned.value().size() == 1U);

        const auto profile = require_dx10_profile();
        auto chunk = libbsa::formats::ba2::ba2_dx10_assemble_chunk(
            profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);

        REQUIRE(chunk.has_value());
        CHECK(chunk.value().compression == libbsa::detail::compression_method::deflate);
        require_decoded_chunk_matches(chunk.value(), layout, planned.value()[0]);
    }

    SECTION("array and cubemap-array ordering") {
        const libbsa::texture::dds_texture_layout layout{4U, 4U, 1U, 28U, 2U, true};
        auto source = snapshot_entry("textures/seam/cube-array.dds", layout, "seam-cube-array");
        auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
        REQUIRE(planned.has_value());
        REQUIRE(planned.value().size() == 12U);

        for (const auto& chunk_plan : planned.value()) {
            const auto profile = require_dx10_profile();
            auto chunk = libbsa::formats::ba2::ba2_dx10_assemble_chunk(
                profile, libbsa::ba2_dx10_writer_options{}, source, chunk_plan);

            REQUIRE(chunk.has_value());
            require_decoded_chunk_matches(chunk.value(), layout, chunk_plan);
        }
    }
}

TEST_CASE(
    "BA2 DX10 chunk assembler fails on missing or truncated snapshot bytes "
    "before publish-visible output changes",
    "[unit][writer-stage][ba2_dx10_writer][parser_preparer_seam][publish]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 2U, 28U, 1U, false};
    auto source = snapshot_entry("textures/seam/truncated.dds", layout, "seam-truncated");
    auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
    REQUIRE(planned.has_value());
    REQUIRE_FALSE(source.subresources.empty());

    const auto output_path = seam_output_path("truncated-snapshot-publish-visible.ba2");
    const auto sentinel = repeated_bytes(17U, 0x42U);
    write_binary_file(output_path, sentinel);

    SECTION("truncated snapshot") {
        write_binary_file(source.subresources.front().snapshot_path, repeated_bytes(1U, 0x7FU));
        const auto profile = require_dx10_profile();
        auto chunk = libbsa::formats::ba2::ba2_dx10_assemble_chunk(
            profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);
        REQUIRE_FALSE(chunk.has_value());
        CHECK(chunk.error().code == libbsa::error_code::io_error);
    }

    SECTION("missing snapshot") {
        std::filesystem::remove(source.subresources.front().snapshot_path);
        const auto profile = require_dx10_profile();
        auto chunk = libbsa::formats::ba2::ba2_dx10_assemble_chunk(
            profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);
        REQUIRE_FALSE(chunk.has_value());
        CHECK(chunk.error().code == libbsa::error_code::io_error);
    }

    CHECK(read_binary_file(output_path) == sentinel);
}

TEST_CASE(
    "BA2 DX10 chunk assembler preserves Fallout 4 and Starfield v3 "
    "compression routing",
    "[unit][writer-stage][ba2_dx10_writer][parser_preparer_seam]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 1U, 28U, 1U, false};
    auto source = snapshot_entry("textures/seam/compression.dds", layout, "seam-compression");
    auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
    REQUIRE(planned.has_value());

    const auto fallout4_profile = require_dx10_profile();
    auto fallout4 = libbsa::formats::ba2::ba2_dx10_assemble_chunk(
        fallout4_profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);
    REQUIRE(fallout4.has_value());
    CHECK(fallout4.value().compression == libbsa::detail::compression_method::deflate);

    libbsa::ba2_dx10_writer_options starfield_deflate_options;
    starfield_deflate_options.starfield_compression_method =
        libbsa::formats::ba2::ba2_starfield_compression_deflate;
    const auto starfield_deflate_profile =
        require_dx10_profile(libbsa::ba2_dx10_target::starfield_v3, starfield_deflate_options);
    auto starfield_deflate = libbsa::formats::ba2::ba2_dx10_assemble_chunk(
        starfield_deflate_profile, starfield_deflate_options, source, planned.value()[0]);
    REQUIRE(starfield_deflate.has_value());
    CHECK(starfield_deflate.value().compression == libbsa::detail::compression_method::deflate);

    libbsa::ba2_dx10_writer_options starfield_lz4_options;
    starfield_lz4_options.starfield_compression_method =
        libbsa::formats::ba2::ba2_starfield_compression_lz4_block;
    const auto starfield_lz4_profile =
        require_dx10_profile(libbsa::ba2_dx10_target::starfield_v3, starfield_lz4_options);
    auto starfield_lz4 = libbsa::formats::ba2::ba2_dx10_assemble_chunk(
        starfield_lz4_profile, starfield_lz4_options, source, planned.value()[0]);
    REQUIRE(starfield_lz4.has_value());
    CHECK(starfield_lz4.value().compression == libbsa::detail::compression_method::lz4_block);
}
