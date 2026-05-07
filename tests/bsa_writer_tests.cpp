#include <catch2/catch_test_macros.hpp>

#include <libbsa/bsa.hpp>
#include <libbsa/bsa_writer.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::detail {

[[nodiscard]] std::uint64_t hash_tes4_path(std::string_view path);

} // namespace libbsa::detail

namespace {

constexpr std::uint32_t bsa_magic = 0x00415342;
constexpr std::uint32_t version_tes4 = 0x67;
constexpr std::uint32_t version_fo3 = 0x68;
constexpr std::uint32_t version_sse = 0x69;
constexpr std::uint32_t archive_pathnames = 0x0001;
constexpr std::uint32_t archive_filenames = 0x0002;
constexpr std::uint32_t archive_compress = 0x0004;
constexpr std::uint32_t file_meshes = 0x0001;
constexpr std::uint32_t file_textures = 0x0002;
constexpr std::uint32_t file_menus = 0x0004;
constexpr std::uint32_t file_sounds = 0x0008;
constexpr std::uint32_t file_size_compress = 0x40000000;

std::uint32_t le_u32(std::span<const std::byte> bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3])) << 24U);
}

std::uint64_t le_u64(std::span<const std::byte> bytes, std::size_t offset)
{
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + static_cast<std::size_t>(shift / 8)]))
                 << static_cast<unsigned>(shift);
    }
    return value;
}

std::vector<libbsa::bsa_memory_entry> tes4_family_entries()
{
    libbsa::bsa_memory_entry mesh{};
    mesh.path = "meshes/a.nif";
    mesh.payload = {std::byte{0x6d}, std::byte{0x65}, std::byte{0x73}, std::byte{0x68}};
    mesh.compression = libbsa::compression_policy::force_raw;

    libbsa::bsa_memory_entry texture{};
    texture.path = "textures/t.dds";
    texture.payload = {std::byte{0x44}, std::byte{0x44}, std::byte{0x53}, std::byte{0x20}, std::byte{0x01}};
    texture.compression = libbsa::compression_policy::force_raw;

    libbsa::bsa_memory_entry sound{};
    sound.path = "sounds/s.wav";
    sound.payload = {std::byte{0x52}, std::byte{0x49}, std::byte{0x46}, std::byte{0x46}};
    sound.compression = libbsa::compression_policy::force_raw;

    return {texture, sound, mesh};
}

std::uint32_t expected_version(libbsa::bsa_write_target target)
{
    switch (target) {
    case libbsa::bsa_write_target::oblivion_v103:
        return version_tes4;
    case libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104:
        return version_fo3;
    case libbsa::bsa_write_target::skyrim_se_ae_v105:
        return version_sse;
    case libbsa::bsa_write_target::tes3_morrowind:
        break;
    }
    return 0;
}

std::vector<std::byte> finalize_to_bytes(const libbsa::bsa_write_plan& plan)
{
    libbsa::memory_sink sink;
    const auto finalized = libbsa::finalize_bsa_write(plan, sink);
    REQUIRE(finalized.has_value());
    return sink.bytes();
}

const libbsa::planned_bsa_table_region& require_region(const libbsa::bsa_write_plan& plan, std::string_view name)
{
    const auto found = std::find_if(plan.table_regions.begin(), plan.table_regions.end(), [name](const auto& region) {
        return region.name == name;
    });
    REQUIRE(found != plan.table_regions.end());
    return *found;
}

std::vector<std::byte> extract_bytes(const std::vector<std::byte>& bytes, std::string path)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    auto archive = libbsa::open_bsa(source);
    REQUIRE(archive.has_value());
    libbsa::memory_sink sink;
    auto extracted = libbsa::extract_bsa_entry(archive.value(), source, std::move(path), sink);
    REQUIRE(extracted.has_value());
    return sink.bytes();
}

const libbsa::planned_bsa_entry& find_planned_entry(const libbsa::bsa_write_plan& plan, std::string_view path)
{
    const auto found = std::find_if(plan.entries.begin(), plan.entries.end(), [path](const auto& entry) {
        return entry.path == path;
    });
    REQUIRE(found != plan.entries.end());
    return *found;
}

} // namespace

TEST_CASE("plans and finalizes TES4-family raw BSA archives for every required version", "[unit][bsa-writer][roundtrip]")
{
    const auto entries = tes4_family_entries();

    for (const auto target : {libbsa::bsa_write_target::oblivion_v103,
                              libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                              libbsa::bsa_write_target::skyrim_se_ae_v105}) {
        const auto plan = libbsa::plan_bsa_write(target, std::span<const libbsa::bsa_memory_entry>{entries});
        REQUIRE(plan.has_value());

        const auto bytes = finalize_to_bytes(plan.value());
        REQUIRE(bytes.size() == plan.value().total_size);
        REQUIRE(bytes.size() >= 36);
        CHECK(le_u32(bytes, 0) == bsa_magic);
        CHECK(le_u32(bytes, 4) == expected_version(target));
        CHECK(le_u32(bytes, 8) == 36);
        CHECK(le_u32(bytes, 16) == 3);
        CHECK(le_u32(bytes, 20) == 3);
        CHECK(le_u32(bytes, 32) == (file_meshes | file_textures | file_sounds));

        const libbsa::memory_source source{std::span<const std::byte>{bytes}};
        const auto archive = libbsa::open_bsa(source);
        REQUIRE(archive.has_value());
        CHECK(archive.value().summary().version == expected_version(target));
        CHECK(archive.value().summary().folder_count == 3);
        CHECK(archive.value().summary().file_count == 3);

        for (const auto& entry : entries) {
            const auto normalized = entry.path;
            const auto metadata = archive.value().entry(normalized);
            REQUIRE(metadata.has_value());
            const auto& planned = find_planned_entry(plan.value(), normalized);
            CHECK(metadata.value().directory_hash == planned.folder_hash);
            CHECK(metadata.value().name_hash == planned.file_hash);
            CHECK(metadata.value().offset == planned.offset);
            CHECK(metadata.value().stored_size == planned.stored_size);
            CHECK(metadata.value().compression == libbsa::compression_state::raw);
            CHECK(extract_bytes(bytes, normalized) == entry.payload);
        }
    }
}

TEST_CASE("exposes native TES4 table regions and offsets before finalization", "[unit][bsa-writer]")
{
    const auto entries = tes4_family_entries();
    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::skyrim_se_ae_v105,
                                            std::span<const libbsa::bsa_memory_entry>{entries});
    REQUIRE(plan.has_value());

    const auto& header = require_region(plan.value(), "tes4 header");
    const auto& folder_records = require_region(plan.value(), "tes4 folder records");
    const auto& folder_blocks = require_region(plan.value(), "tes4 folder blocks");
    const auto& file_names = require_region(plan.value(), "tes4 file names");

    CHECK(header.offset == 0);
    CHECK(header.size == 36);
    CHECK(folder_records.offset == 36);
    CHECK(folder_records.size == 72);
    CHECK(folder_blocks.offset == 108);
    CHECK(file_names.offset > folder_blocks.offset);
    REQUIRE(plan.value().data_regions.size() == 3);
    for (const auto& entry : plan.value().entries) {
        CHECK(entry.offset >= file_names.offset + file_names.size);
        CHECK(entry.offset == plan.value().data_regions[entry.data_region_id].offset);
    }
}

TEST_CASE("computes TES4 FileFlags from entry extensions", "[unit][bsa-writer]")
{
    std::vector entries = tes4_family_entries();
    libbsa::bsa_memory_entry menu{};
    menu.path = "menus/main.xml";
    menu.payload = {std::byte{0x3c}, std::byte{0x78}, std::byte{0x2f}};
    menu.compression = libbsa::compression_policy::force_raw;
    entries.push_back(std::move(menu));

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                            std::span<const libbsa::bsa_memory_entry>{entries});
    REQUIRE(plan.has_value());
    const auto bytes = finalize_to_bytes(plan.value());

    CHECK(le_u32(bytes, 32) == (file_meshes | file_textures | file_sounds | file_menus));
}

TEST_CASE("orders TES4 folders and files by reference hashes independent of input order", "[unit][bsa-writer]")
{
    std::vector first_order = tes4_family_entries();
    std::vector second_order = first_order;
    std::reverse(second_order.begin(), second_order.end());

    const auto first = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                             std::span<const libbsa::bsa_memory_entry>{first_order});
    const auto second = libbsa::plan_bsa_write(libbsa::bsa_write_target::oblivion_v103,
                                              std::span<const libbsa::bsa_memory_entry>{second_order});
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    CHECK(finalize_to_bytes(first.value()) == finalize_to_bytes(second.value()));

    const auto bytes = finalize_to_bytes(first.value());
    const auto first_folder_record = std::size_t{36};
    const auto second_folder_record = first_folder_record + 16;
    const auto third_folder_record = second_folder_record + 16;
    const std::vector folder_hashes{le_u64(bytes, first_folder_record),
                                    le_u64(bytes, second_folder_record),
                                    le_u64(bytes, third_folder_record)};
    CHECK(std::is_sorted(folder_hashes.begin(), folder_hashes.end()));
    CHECK(find_planned_entry(first.value(), "meshes/a.nif").file_hash == libbsa::detail::hash_tes4_path("a.nif"));
    CHECK(find_planned_entry(first.value(), "textures/t.dds").file_hash == libbsa::detail::hash_tes4_path("t.dds"));
    CHECK(find_planned_entry(first.value(), "sounds/s.wav").file_hash == libbsa::detail::hash_tes4_path("s.wav"));
    CHECK((le_u32(bytes, 12) & (archive_pathnames | archive_filenames | archive_compress)) == (archive_pathnames | archive_filenames));
    CHECK((le_u32(bytes, 36 + 8) == 1));
}
