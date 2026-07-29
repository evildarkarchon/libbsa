#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_gnrl_prepare.hpp"
#include "formats/ba2/ba2_profile.hpp"
#include "formats/ba2/ba2_gnrl_serialize.hpp"
#include "formats/bsa/tes3_bsa_layout.hpp"
#include "formats/bsa/tes3_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_layout.hpp"
#include "formats/bsa/tes4_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_profile.hpp"
#include "formats/bsa/tes4_bsa_serialize.hpp"
#include "texture/dds_layout.hpp"

#include <detail/host_file_path.hpp>

#include <catch2/catch_test_macros.hpp>
#include <libbsa/archive.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<std::byte> bytes_from_text(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::filesystem::path stage_test_dir() {
    auto path = std::filesystem::temp_directory_path() / "libbsa_writer_stage_tests";
    std::filesystem::create_directories(path);
    return path;
}

std::filesystem::path stage_output_path(std::string name) {
    return stage_test_dir() / std::move(name);
}

void write_stage_binary_file(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

std::vector<std::byte> read_stage_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    REQUIRE_FALSE(input.bad());
    return bytes;
}

std::uint32_t read_stage_u32_le_at(std::span<const std::byte> bytes, std::size_t offset) {
    REQUIRE(offset <= bytes.size());
    REQUIRE(bytes.size() - offset >= sizeof(std::uint32_t));
    return std::to_integer<std::uint32_t>(bytes[offset]) |
           (std::to_integer<std::uint32_t>(bytes[offset + 1U]) << 8U) |
           (std::to_integer<std::uint32_t>(bytes[offset + 2U]) << 16U) |
           (std::to_integer<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

libbsa::formats::ba2::ba2_profile require_gnrl_profile(
    libbsa::ba2_gnrl_target target = libbsa::ba2_gnrl_target::fallout4,
    const libbsa::ba2_gnrl_writer_options& options = {}) {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_gnrl_writer(target, options);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::ba2::ba2_profile require_dx10_profile(
    libbsa::ba2_dx10_target target = libbsa::ba2_dx10_target::fallout4,
    const libbsa::ba2_dx10_writer_options& options = {}) {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(target, options);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::bsa::tes4_bsa_profile require_tes4_profile(
    libbsa::tes4_bsa_target target = libbsa::tes4_bsa_target::fallout3) {
    auto profile = libbsa::formats::bsa::make_tes4_bsa_profile_for_writer(target);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::ba2::ba2_gnrl_prepared_entry ba2_gnrl_memory_stage_entry(
    std::vector<std::byte> bytes) {
    const auto raw_size = static_cast<std::uint32_t>(bytes.size());
    return libbsa::formats::ba2::ba2_gnrl_prepared_entry{
        "Meshes/Stage.bin",
        "meshes/stage.bin",
        {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
        0U,
        0U,
        0U,
        0U,
        raw_size,
        libbsa::detail::stored_payload::from_owned_bytes(std::move(bytes))};
}

libbsa::formats::bsa::tes4_prepared_entry tes4_memory_stage_entry(std::string file_name,
                                                                  std::vector<std::byte> bytes) {
    const auto file_hash = static_cast<std::uint64_t>(file_name.size());
    return libbsa::formats::bsa::tes4_prepared_entry{
        "Meshes",  "meshes", std::move(file_name),
        file_hash, 0U,       libbsa::detail::stored_payload::from_owned_bytes(std::move(bytes))};
}

libbsa::detail::finalization_workspace reserve_stage_workspace(std::string output_name) {
    auto workspace = libbsa::detail::finalization_workspace::reserve(
        stage_output_path(std::move(output_name)), "TES4 writer stage test");
    REQUIRE(workspace.has_value());
    return std::move(workspace).value();
}

std::vector<libbsa::formats::bsa::tes4_prepared_folder> tes4_stage_folders(
    std::vector<std::pair<std::string, std::vector<std::byte>>> entries) {
    std::vector<libbsa::formats::bsa::tes4_prepared_entry> prepared_entries;
    prepared_entries.reserve(entries.size());
    for (auto& [file_name, bytes] : entries) {
        prepared_entries.push_back(tes4_memory_stage_entry(std::move(file_name), std::move(bytes)));
    }

    std::vector<libbsa::formats::bsa::tes4_prepared_folder> folders;
    folders.push_back(
        libbsa::formats::bsa::tes4_prepared_folder{"Meshes", 1U, std::move(prepared_entries)});
    return folders;
}

std::vector<std::byte> repeated_bytes(std::size_t size, std::uint8_t seed) {
    std::vector<std::byte> bytes(size);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::byte>(seed + static_cast<std::uint8_t>(index % 17U));
    }
    return bytes;
}

libbsa::formats::ba2::ba2_dx10_writer_entry ba2_dx10_stage_entry(
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
                auto bytes =
                    repeated_bytes(static_cast<std::size_t>(mip_size.value()),
                                   static_cast<std::uint8_t>(array_index + face_index + mip + 1U));
                const auto snapshot = stage_output_path(
                    snapshot_prefix + "-" + std::to_string(array_index) + "-" +
                    std::to_string(face_index) + "-" + std::to_string(mip) + ".bin");
                write_stage_binary_file(snapshot, bytes);
                entry.subresources.push_back(libbsa::formats::ba2::ba2_dx10_subresource_snapshot{
                    array_index, face_index, mip, bytes.size(), snapshot});
            }
        }
    }
    return entry;
}

libbsa::formats::ba2::ba2_dx10_prepared_entry ba2_dx10_prepared_stage_entry(
    std::string path, std::vector<std::byte> payload) {
    libbsa::formats::ba2::ba2_dx10_prepared_entry entry;
    entry.archive_path_original = std::move(path);
    entry.archive_path_canonical = entry.archive_path_original;
    entry.chunk_count = 2U;
    auto first = libbsa::formats::ba2::ba2_dx10_prepared_chunk{};
    first.raw_size = static_cast<std::uint32_t>(payload.size());
    first.packed_size = static_cast<std::uint32_t>(payload.size());
    first.compression = libbsa::detail::compression_method::deflate;
    first.stored_payload = payload;
    auto second = first;
    entry.chunks = {std::move(first), std::move(second)};
    return entry;
}

}  // namespace

TEST_CASE("tes3 writer preparation stage prepares and sorts minimal memory entries",
          "[unit][writer-stage][tes3_bsa_writer]") {
    auto entry = libbsa::formats::bsa::tes3_make_writer_entry("Textures\\Stage\\Probe.dds");
    REQUIRE(entry.has_value());
    entry.value().memory_bytes = bytes_from_text("tes3-stage");
    entry.value().from_memory = true;

    auto prepared = libbsa::formats::bsa::tes3_prepare_entries(
        std::span<const libbsa::formats::bsa::tes3_writer_entry>{&entry.value(), 1U});

    REQUIRE(prepared.has_value());
    REQUIRE(prepared.value().size() == 1U);
    CHECK(prepared.value()[0].archive_path_original == "Textures/Stage/Probe.dds");
    CHECK(prepared.value()[0].payload_size == 10U);
    CHECK(prepared.value()[0].from_memory);
}

TEST_CASE("tes3 writer preparation stage reports malformed disk entries",
          "[unit][writer-stage][tes3_bsa_writer]") {
    auto entry = libbsa::formats::bsa::tes3_make_writer_entry("meshes/stage/missing.nif");
    REQUIRE(entry.has_value());
    entry.value().host_path = "Z:/definitely/missing/libbsa-stage-source.nif";
    entry.value().from_memory = false;

    auto prepared = libbsa::formats::bsa::tes3_prepare_entries(
        std::span<const libbsa::formats::bsa::tes3_writer_entry>{&entry.value(), 1U});

    REQUIRE_FALSE(prepared.has_value());
    CHECK(prepared.error().code == libbsa::error_code::io_error);
}

TEST_CASE("tes3 writer layout stage assigns raw offsets and rejects oversized spans",
          "[unit][writer-stage][tes3_bsa_writer]") {
    std::vector<libbsa::formats::bsa::tes3_prepared_entry> entries(2U);
    entries[0].payload_size = 4U;
    entries[1].payload_size = 8U;

    auto assigned = libbsa::formats::bsa::tes3_assign_raw_offsets(entries);

    REQUIRE(assigned.has_value());
    CHECK(entries[0].raw_offset == 0U);
    CHECK(entries[1].raw_offset == 4U);

    entries[0].payload_size = std::numeric_limits<std::uint32_t>::max();
    entries[1].payload_size = 1U;
    auto oversized = libbsa::formats::bsa::tes3_assign_raw_offsets(entries);

    REQUIRE_FALSE(oversized.has_value());
    CHECK(oversized.error().code == libbsa::error_code::format_error);
}

TEST_CASE("ba2 gnrl writer preparation stage prepares minimal memory entries",
          "[unit][writer-stage][ba2_gnrl_writer]") {
    auto entry = libbsa::formats::ba2::ba2_gnrl_make_writer_entry("Meshes\\Stage\\Probe.bin", {});
    REQUIRE(entry.has_value());
    entry.value().memory_bytes = bytes_from_text("ba2-stage");
    entry.value().from_memory = true;

    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    const auto profile = require_gnrl_profile(libbsa::ba2_gnrl_target::fallout4, options);
    auto workspace = reserve_stage_workspace("ba2-gnrl-memory-prepare.ba2");
    auto prepared = libbsa::formats::ba2::ba2_gnrl_prepare_entries(
        profile, options,
        std::span<const libbsa::formats::ba2::ba2_gnrl_writer_entry>{&entry.value(), 1U}, 1U,
        workspace);

    REQUIRE(prepared.has_value());
    REQUIRE(prepared.value().size() == 1U);
    CHECK(prepared.value()[0].archive_path_original == "Meshes/Stage/Probe.bin");
    CHECK(prepared.value()[0].raw_size == 9U);
    CHECK(prepared.value()[0].payload.size() == 9U);
    CHECK(prepared.value()[0].extension[0] == std::byte{0x62});
}

TEST_CASE("ba2 gnrl writer layout stage toggles duplicate payload reuse",
          "[unit][writer-stage][ba2_gnrl_writer]") {
    const auto payload = bytes_from_text("shared");
    const auto profile = require_gnrl_profile();

    std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> distinct;
    distinct.push_back(ba2_gnrl_memory_stage_entry(payload));
    distinct.push_back(ba2_gnrl_memory_stage_entry(payload));
    std::uint64_t distinct_file_table_offset = 0;
    auto assigned_distinct = libbsa::formats::ba2::ba2_gnrl_assign_payload_offsets(
        distinct, profile, false, distinct_file_table_offset);

    REQUIRE(assigned_distinct.has_value());
    CHECK(distinct[0].payload_offset != distinct[1].payload_offset);
    CHECK(distinct[0].is_payload_representative);
    CHECK(distinct[1].is_payload_representative);

    std::vector<libbsa::formats::ba2::ba2_gnrl_prepared_entry> deduped;
    deduped.push_back(ba2_gnrl_memory_stage_entry(payload));
    deduped.push_back(ba2_gnrl_memory_stage_entry(payload));
    std::uint64_t deduped_file_table_offset = 0;
    auto assigned_deduped = libbsa::formats::ba2::ba2_gnrl_assign_payload_offsets(
        deduped, profile, true, deduped_file_table_offset);

    REQUIRE(assigned_deduped.has_value());
    CHECK(deduped[0].payload_offset == deduped[1].payload_offset);
    CHECK(deduped[0].is_payload_representative);
    CHECK_FALSE(deduped[1].is_payload_representative);
    CHECK(deduped_file_table_offset < distinct_file_table_offset);
}

TEST_CASE("ba2 gnrl writer serialization uses snapshots after original sources change",
          "[unit][writer-stage][ba2_gnrl_writer][snapshot]") {
    const auto payload = bytes_from_text("ba2 gnrl raw snapshot payload");
    const auto modified_source = stage_output_path("ba2-gnrl-snapshot-modified.bin");
    const auto deleted_source = stage_output_path("ba2-gnrl-snapshot-deleted.bin");
    write_stage_binary_file(modified_source, payload);
    write_stage_binary_file(deleted_source, payload);

    std::vector<libbsa::formats::ba2::ba2_gnrl_writer_entry> entries;
    for (const auto& [archive_path, source] :
         std::array{std::pair{std::string_view{"Meshes/Snapshot/Modified.bin"}, modified_source},
                    std::pair{std::string_view{"Meshes/Snapshot/Deleted.bin"}, deleted_source}}) {
        auto entry = libbsa::formats::ba2::ba2_gnrl_make_writer_entry(archive_path, {});
        REQUIRE(entry.has_value());
        entry.value().host_path = source.string();
        entry.value().from_memory = false;
        entries.push_back(std::move(entry).value());
    }

    libbsa::ba2_gnrl_writer_options options;
    options.compression = libbsa::archive_compression_policy::all_raw;
    options.deduplicate_payloads = true;
    const auto profile = require_gnrl_profile(libbsa::ba2_gnrl_target::fallout4, options);
    auto workspace = reserve_stage_workspace("ba2-gnrl-snapshot-stable-final.ba2");
    auto prepared =
        libbsa::formats::ba2::ba2_gnrl_prepare_entries(profile, options, entries, 2U, workspace);
    REQUIRE(prepared.has_value());

    write_stage_binary_file(modified_source, bytes_from_text("mutated original source"));
    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(deleted_source, removal_error));

    std::uint64_t file_table_offset = 0U;
    auto placed = libbsa::formats::ba2::ba2_gnrl_assign_payload_offsets(
        prepared.value(), profile, options.deduplicate_payloads, file_table_offset);
    REQUIRE(placed.has_value());
    REQUIRE(prepared.value().size() == 2U);
    CHECK(prepared.value()[0].payload_offset == prepared.value()[1].payload_offset);

    const auto output = stage_output_path("ba2-gnrl-snapshot-stable.ba2");
    auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
        profile, options, prepared.value(), file_table_offset, output);
    REQUIRE(serialized.has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    CHECK(opened.value().extract_bytes("Meshes/Snapshot/Modified.bin").value() == payload);
    CHECK(opened.value().extract_bytes("Meshes/Snapshot/Deleted.bin").value() == payload);
}

TEST_CASE("tes4 writer preparation stage prepares minimal memory folders",
          "[unit][writer-stage][tes4_bsa_writer]") {
    auto entry = libbsa::formats::bsa::tes4_make_writer_entry(
        "Meshes\\Stage\\Probe.nif", libbsa::entry_compression_policy::raw);
    REQUIRE(entry.has_value());
    entry.value().memory_bytes = bytes_from_text("tes4-stage");
    entry.value().from_memory = true;

    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    std::uint32_t file_flags = 0;
    auto workspace = reserve_stage_workspace("tes4-memory-prepare.bsa");

    auto folders = libbsa::formats::bsa::tes4_prepare_folders(
        std::span<const libbsa::formats::bsa::tes4_writer_entry>{&entry.value(), 1U}, profile,
        options, 1U, file_flags, workspace);

    REQUIRE(folders.has_value());
    REQUIRE(folders.value().size() == 1U);
    REQUIRE(folders.value()[0].entries.size() == 1U);
    CHECK(folders.value()[0].name == "Meshes\\Stage");
    CHECK(folders.value()[0].entries[0].file_name == "Probe.nif");
    CHECK(folders.value()[0].entries[0].payload.size() == 10U);
    CHECK(file_flags != 0U);
}

TEST_CASE("tes4 writer placement plan toggles exact Stored Payload reuse",
          "[unit][writer-stage][tes4_bsa_writer]") {
    const auto payload = bytes_from_text("shared");
    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;

    auto distinct = tes4_stage_folders({{"A.nif", payload}, {"B.nif", payload}});
    options.deduplicate_payloads = false;
    auto distinct_plan =
        libbsa::formats::bsa::tes4_plan_placements(std::move(distinct), profile, options, 0U);

    REQUIRE(distinct_plan.has_value());
    REQUIRE(distinct_plan.value().payloads.size() == 2U);
    REQUIRE(distinct_plan.value().folders.size() == 1U);
    REQUIRE(distinct_plan.value().folders[0].entries.size() == 2U);
    const auto distinct_first_index = distinct_plan.value().folders[0].entries[0].payload_index;
    const auto distinct_second_index = distinct_plan.value().folders[0].entries[1].payload_index;
    CHECK(distinct_first_index != distinct_second_index);
    CHECK(distinct_plan.value().payloads[distinct_first_index].offset !=
          distinct_plan.value().payloads[distinct_second_index].offset);

    auto deduped = tes4_stage_folders({{"A.nif", payload}, {"B.nif", payload}});
    options.deduplicate_payloads = true;
    auto deduped_plan =
        libbsa::formats::bsa::tes4_plan_placements(std::move(deduped), profile, options, 0U);

    REQUIRE(deduped_plan.has_value());
    REQUIRE(deduped_plan.value().payloads.size() == 1U);
    REQUIRE(deduped_plan.value().folders.size() == 1U);
    CHECK(deduped_plan.value().folders[0].entries[0].payload_index ==
          deduped_plan.value().folders[0].entries[1].payload_index);
    CHECK(deduped_plan.value().file_count == 2U);
}

TEST_CASE("tes4 writer placement plan uses resolved profile folder record sizing",
          "[unit][writer-stage][tes4_bsa_writer]") {
    struct layout_expectation {
        libbsa::tes4_bsa_target target;
        std::uint64_t folder_block_offset;
        std::uint32_t payload_offset;
    };
    constexpr std::array expectations{
        layout_expectation{libbsa::tes4_bsa_target::oblivion, 58U, 82U},
        layout_expectation{libbsa::tes4_bsa_target::fallout3, 58U, 82U},
        layout_expectation{libbsa::tes4_bsa_target::skyrim_se, 66U, 90U},
    };

    for (const auto& expected : expectations) {
        const auto profile = require_tes4_profile(expected.target);
        libbsa::tes4_bsa_writer_options options;
        options.compression_policy = libbsa::archive_compression_policy::all_raw;
        auto folders = tes4_stage_folders({{"A.nif", bytes_from_text("data")}});

        auto plan =
            libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);

        REQUIRE(plan.has_value());
        CHECK(plan.value().total_file_name_length == 6U);
        CHECK(plan.value().folders[0].folder_block_offset == expected.folder_block_offset);
        const auto payload_index = plan.value().folders[0].entries[0].payload_index;
        CHECK(plan.value().payloads[payload_index].offset == expected.payload_offset);
    }
}

TEST_CASE("tes4 writer serialization stage emits profile-selected folder record bytes",
          "[unit][writer-stage][tes4_bsa_writer]") {
    struct serialization_expectation {
        libbsa::tes4_bsa_target target;
        std::uint32_t version;
        std::uint32_t archive_flags;
        std::vector<std::byte> folder_record;
    };
    const std::array expectations{
        serialization_expectation{
            libbsa::tes4_bsa_target::oblivion,
            103U,
            0x0003U,
            {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x3A}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}}},
        serialization_expectation{
            libbsa::tes4_bsa_target::fallout3,
            104U,
            0x0103U,
            {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x3A}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}}},
        serialization_expectation{
            libbsa::tes4_bsa_target::skyrim_se,
            105U,
            0x0103U,
            {std::byte{0x01}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x42}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
             std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00}}},
    };

    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.embed_file_names = true;
    for (const auto& expected : expectations) {
        const auto profile = require_tes4_profile(expected.target);
        auto folders = tes4_stage_folders({{"A.nif", bytes_from_text("data")}});
        auto plan =
            libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);
        REQUIRE(plan.has_value());

        const auto output =
            stage_output_path("tes4-profile-record-v" + std::to_string(expected.version) + ".bsa");
        auto written = libbsa::formats::bsa::tes4_write_archive_bytes(plan.value(), output);

        REQUIRE(written.has_value());
        const auto bytes = read_stage_binary_file(output);
        CHECK(read_stage_u32_le_at(bytes, 4U) == expected.version);
        CHECK(read_stage_u32_le_at(bytes, 12U) == expected.archive_flags);
        REQUIRE(bytes.size() >= 36U + expected.folder_record.size());
        CHECK(std::vector<std::byte>{bytes.begin() + 36U,
                                     bytes.begin() + 36U + expected.folder_record.size()} ==
              expected.folder_record);
    }
}

TEST_CASE("tes4 writer placement preserves distinct zero-length entries without dedupe",
          "[unit][writer-stage][tes4_bsa_writer]") {
    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    auto folders = tes4_stage_folders({{"EmptyA.nif", {}}, {"EmptyB.nif", {}}});

    auto plan =
        libbsa::formats::bsa::tes4_plan_placements(std::move(folders), profile, options, 0U);

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().payloads.size() == 2U);
    const auto first_index = plan.value().folders[0].entries[0].payload_index;
    const auto second_index = plan.value().folders[0].entries[1].payload_index;
    CHECK(first_index != second_index);
    CHECK(plan.value().payloads[first_index].stored_size == 0U);
    CHECK(plan.value().payloads[second_index].stored_size == 0U);
    // BSArchPro records the same current data cursor for adjacent empty payloads.
    CHECK(plan.value().payloads[first_index].offset == plan.value().payloads[second_index].offset);
}

TEST_CASE("tes4 writer serialization uses snapshots after original sources change",
          "[unit][writer-stage][tes4_bsa_writer][writer-source-io]") {
    const auto payload = bytes_from_text("tes4 raw snapshot payload");
    const auto modified_source = stage_output_path("tes4-snapshot-modified.bin");
    const auto deleted_source = stage_output_path("tes4-snapshot-deleted.bin");
    write_stage_binary_file(modified_source, payload);
    write_stage_binary_file(deleted_source, payload);

    std::vector<libbsa::formats::bsa::tes4_writer_entry> entries;
    for (const auto& [archive_path, source] :
         std::array{std::pair{std::string_view{"Meshes/Snapshot/Modified.nif"}, modified_source},
                    std::pair{std::string_view{"Meshes/Snapshot/Deleted.nif"}, deleted_source}}) {
        auto entry = libbsa::formats::bsa::tes4_make_writer_entry(
            archive_path, libbsa::entry_compression_policy::raw);
        REQUIRE(entry.has_value());
        entry.value().host_path = source.string();
        entry.value().from_memory = false;
        entries.push_back(std::move(entry).value());
    }
    auto memory_entry = libbsa::formats::bsa::tes4_make_writer_entry(
        "Meshes/Snapshot/Memory.nif", libbsa::entry_compression_policy::raw);
    REQUIRE(memory_entry.has_value());
    memory_entry.value().memory_bytes = payload;
    memory_entry.value().from_memory = true;
    entries.push_back(std::move(memory_entry).value());

    const auto profile = require_tes4_profile();
    libbsa::tes4_bsa_writer_options options;
    options.compression_policy = libbsa::archive_compression_policy::all_raw;
    options.deduplicate_payloads = true;
    std::uint32_t file_flags = 0U;
    const auto output = stage_output_path("tes4-snapshot-stable.bsa");
    auto workspace = reserve_stage_workspace("tes4-snapshot-stable-final.bsa");

    auto folders = libbsa::formats::bsa::tes4_prepare_folders(entries, profile, options, 2U,
                                                              file_flags, workspace);
    REQUIRE(folders.has_value());

    write_stage_binary_file(modified_source, bytes_from_text("mutated original source"));
    std::error_code removal_error;
    REQUIRE(std::filesystem::remove(deleted_source, removal_error));
    REQUIRE_FALSE(removal_error);

    auto plan = libbsa::formats::bsa::tes4_plan_placements(std::move(folders).value(), profile,
                                                           options, file_flags);
    REQUIRE(plan.has_value());
    CHECK(plan.value().payloads.size() == 1U);

    auto written = libbsa::formats::bsa::tes4_write_archive_bytes(plan.value(), output);
    REQUIRE(written.has_value());

    auto opened = libbsa::archive_reader::open(output.string());
    REQUIRE(opened.has_value());
    for (const auto path : {"Meshes/Snapshot/Modified.nif", "Meshes/Snapshot/Deleted.nif",
                            "Meshes/Snapshot/Memory.nif"}) {
        auto extracted = opened.value().extract_bytes(path);
        REQUIRE(extracted.has_value());
        CHECK(extracted.value() == payload);
    }
}

TEST_CASE("ba2 dx10 writer preparation stage prepares a single-mip chunk",
          "[unit][writer-stage][ba2_dx10_writer]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 1U, 28U, 1U, false};
    auto source = ba2_dx10_stage_entry("Textures/Stage/Single.dds", layout, "dx10-single");
    auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
    REQUIRE(planned.has_value());
    REQUIRE(planned.value().size() == 1U);

    const auto profile = require_dx10_profile();
    auto chunk = libbsa::formats::ba2::ba2_dx10_prepare_chunk(
        profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);

    REQUIRE(chunk.has_value());
    CHECK(chunk.value().raw_size == planned.value()[0].raw_size);
    CHECK(chunk.value().packed_size > 0U);
    CHECK(chunk.value().start_mip == 0U);
    CHECK(chunk.value().end_mip == 0U);
}

TEST_CASE(
    "ba2 dx10 writer preparation stage preserves multi-mip snapshot "
    "chunk order",
    "[unit][writer-stage][ba2_dx10_writer]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 3U, 28U, 1U, false};
    auto source = ba2_dx10_stage_entry("Textures/Stage/MultiChunk.dds", layout, "dx10-multi-chunk");
    auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
    REQUIRE(planned.has_value());
    REQUIRE(planned.value().size() == 1U);
    REQUIRE(planned.value()[0].start_mip < planned.value()[0].end_mip);

    const auto profile = require_dx10_profile();
    auto chunk = libbsa::formats::ba2::ba2_dx10_prepare_chunk(
        profile, libbsa::ba2_dx10_writer_options{}, source, planned.value()[0]);

    REQUIRE(chunk.has_value());
    CHECK(chunk.value().raw_size == planned.value()[0].raw_size);
    CHECK(chunk.value().packed_size > 0U);
    CHECK(chunk.value().start_mip == planned.value()[0].start_mip);
    CHECK(chunk.value().end_mip == planned.value()[0].end_mip);

    auto decoded = libbsa::detail::decompress_payload_exact(
        chunk.value().compression, chunk.value().stored_payload,
        static_cast<std::size_t>(planned.value()[0].raw_size));
    REQUIRE(decoded.has_value());
    std::vector<std::byte> expected;
    expected.reserve(static_cast<std::size_t>(planned.value()[0].raw_size));
    for (std::uint32_t mip = planned.value()[0].start_mip; mip <= planned.value()[0].end_mip;
         ++mip) {
        auto mip_size = libbsa::texture::mip_size_for_format(layout, mip);
        REQUIRE(mip_size.has_value());
        auto bytes = repeated_bytes(static_cast<std::size_t>(mip_size.value()),
                                    static_cast<std::uint8_t>(mip + 1U));
        expected.insert(expected.end(), bytes.begin(), bytes.end());
    }
    CHECK(decoded.value() == expected);
}

TEST_CASE(
    "ba2 dx10 writer preparation stage rejects truncated snapshots "
    "before publishing output",
    "[unit][writer-stage][ba2_dx10_writer][publish]") {
    const libbsa::texture::dds_texture_layout layout{4U, 4U, 2U, 28U, 1U, false};
    auto source = ba2_dx10_stage_entry("Textures/Stage/Truncated.dds", layout, "dx10-truncated");
    REQUIRE_FALSE(source.subresources.empty());
    write_stage_binary_file(source.subresources[0].snapshot_path, bytes_from_text("short"));

    const auto output_path = stage_output_path("dx10-truncated-snapshot.ba2");
    const auto sentinel = bytes_from_text("existing BA2 DX10 sentinel");
    write_stage_binary_file(output_path, sentinel);
    libbsa::ba2_dx10_writer_options options;
    options.overwrite_existing = true;

    auto written = libbsa::formats::ba2::write_ba2_dx10_archive(libbsa::ba2_dx10_target::fallout4,
                                                                options, std::span{&source, 1U},
                                                                output_path.string(), 1U);

    REQUIRE_FALSE(written.has_value());
    CHECK(written.error().code == libbsa::error_code::io_error);
    CHECK(read_stage_binary_file(output_path) == sentinel);
}

TEST_CASE("ba2 dx10 writer preparation stage prepares multi-mip and cubemap entries",
          "[unit][writer-stage][ba2_dx10_writer]") {
    const libbsa::texture::dds_texture_layout multi_mip{4U, 4U, 2U, 28U, 1U, false};
    auto multi_entry = ba2_dx10_stage_entry("Textures/Stage/Multi.dds", multi_mip, "dx10-multi");
    const auto profile = require_dx10_profile();
    auto multi_prepared = libbsa::formats::ba2::ba2_dx10_prepare_entries(
        profile, libbsa::ba2_dx10_writer_options{}, std::span{&multi_entry, 1U}, 1U);

    REQUIRE(multi_prepared.has_value());
    REQUIRE(multi_prepared.value().size() == 1U);
    CHECK(multi_prepared.value()[0].mip_count == 2U);
    CHECK_FALSE(multi_prepared.value()[0].chunks.empty());

    const libbsa::texture::dds_texture_layout cubemap{4U, 4U, 1U, 28U, 1U, true};
    auto cubemap_entry = ba2_dx10_stage_entry("Textures/Stage/Cube.dds", cubemap, "dx10-cube");
    auto cubemap_prepared = libbsa::formats::ba2::ba2_dx10_prepare_entries(
        profile, libbsa::ba2_dx10_writer_options{}, std::span{&cubemap_entry, 1U}, 1U);

    REQUIRE(cubemap_prepared.has_value());
    REQUIRE(cubemap_prepared.value().size() == 1U);
    CHECK(cubemap_prepared.value()[0].cube_maps_raw == 2049U);
    CHECK(cubemap_prepared.value()[0].chunks.size() == 6U);
}

TEST_CASE("ba2 dx10 writer layout stage toggles duplicate chunk reuse",
          "[unit][writer-stage][ba2_dx10_writer]") {
    const auto payload = bytes_from_text("dx10-shared");
    const auto profile = require_dx10_profile();

    auto distinct =
        std::vector{ba2_dx10_prepared_stage_entry("Textures/Stage/Distinct.dds", payload)};
    std::uint64_t distinct_file_table_offset = 0;
    auto assigned_distinct = libbsa::formats::ba2::ba2_dx10_assign_payload_offsets(
        distinct, profile, false, distinct_file_table_offset);

    REQUIRE(assigned_distinct.has_value());
    REQUIRE(distinct[0].chunks.size() == 2U);
    CHECK(distinct[0].chunks[0].payload_offset != distinct[0].chunks[1].payload_offset);
    CHECK(distinct[0].chunks[0].owns_payload_bytes);
    CHECK(distinct[0].chunks[1].owns_payload_bytes);

    auto deduped =
        std::vector{ba2_dx10_prepared_stage_entry("Textures/Stage/Deduped.dds", payload)};
    std::uint64_t deduped_file_table_offset = 0;
    auto assigned_deduped = libbsa::formats::ba2::ba2_dx10_assign_payload_offsets(
        deduped, profile, true, deduped_file_table_offset);

    REQUIRE(assigned_deduped.has_value());
    REQUIRE(deduped[0].chunks.size() == 2U);
    CHECK(deduped[0].chunks[0].payload_offset == deduped[0].chunks[1].payload_offset);
    CHECK(deduped[0].chunks[0].owns_payload_bytes);
    CHECK_FALSE(deduped[0].chunks[1].owns_payload_bytes);
}
