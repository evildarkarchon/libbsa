#include <catch2/catch_test_macros.hpp>

#include <libbsa/ba2.hpp>
#include <libbsa/ba2_writer.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view gnrl_placeholder = "BA2 GNRL writer planning is not implemented";
constexpr std::string_view dds_placeholder = "BA2 DDS writer planning is not implemented";
constexpr std::string_view finalize_placeholder = "BA2 writer finalization is not implemented";

constexpr std::uint32_t magic_btdx = 0x58445442U;
constexpr std::uint32_t magic_gnrl = 0x4c524e47U;
constexpr std::uint64_t gnrl_record_size = 36U;

template <typename Result>
void require_unsupported_placeholder(const Result& result, std::string_view message)
{
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::unsupported_format);
    CHECK(result.error().message.find(message) != std::string::npos);
}

std::uint16_t le_u16(std::span<const std::byte> bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset])) |
           static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset + 1]) << 8U);
}

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

std::uint32_t expected_version(libbsa::ba2_write_target target)
{
    switch (target) {
    case libbsa::ba2_write_target::fallout4_gnrl_v1:
        return 1U;
    case libbsa::ba2_write_target::fallout4_gnrl_v7:
        return 7U;
    case libbsa::ba2_write_target::fallout4_gnrl_v8:
        return 8U;
    case libbsa::ba2_write_target::starfield_gnrl_v2:
        return 2U;
    case libbsa::ba2_write_target::starfield_gnrl_v3:
        return 3U;
    default:
        return 0U;
    }
}

std::uint32_t expected_header_size(libbsa::ba2_write_target target)
{
    switch (target) {
    case libbsa::ba2_write_target::starfield_gnrl_v2:
        return 32U;
    case libbsa::ba2_write_target::starfield_gnrl_v3:
        return 36U;
    default:
        return 24U;
    }
}

std::vector<libbsa::ba2_gnrl_memory_entry> gnrl_memory_entries()
{
    libbsa::ba2_gnrl_memory_entry mesh;
    mesh.path = "Meshes\\Armor\\Iron.NIF";
    mesh.payload = {std::byte{0x4e}, std::byte{0x49}, std::byte{0x46}};
    mesh.compression = libbsa::compression_policy::force_raw;

    libbsa::ba2_gnrl_memory_entry script;
    script.path = "scripts/quests/main.pex";
    script.payload = {std::byte{0x50}, std::byte{0x45}, std::byte{0x58}, std::byte{0x21}};
    script.compression = libbsa::compression_policy::force_raw;

    return {mesh, script};
}

std::vector<std::byte> finalize_to_bytes(const libbsa::ba2_write_plan& plan)
{
    libbsa::memory_sink sink;
    const auto finalized = libbsa::finalize_ba2_write(plan, sink);
    REQUIRE(finalized.has_value());
    return sink.bytes();
}

std::vector<std::byte> extract_ba2_bytes(const std::vector<std::byte>& bytes, std::string path)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    auto archive = libbsa::open_ba2(source);
    REQUIRE(archive.has_value());
    libbsa::memory_sink sink;
    auto extracted = libbsa::extract_ba2_entry(archive.value(), source, std::move(path), sink);
    REQUIRE(extracted.has_value());
    return sink.bytes();
}

const libbsa::planned_ba2_gnrl_entry& find_planned_gnrl_entry(const libbsa::ba2_write_plan& plan, std::string_view path)
{
    const auto found = std::find_if(plan.gnrl.entries.begin(), plan.gnrl.entries.end(), [path](const auto& entry) {
        return entry.path == path;
    });
    REQUIRE(found != plan.gnrl.entries.end());
    return *found;
}

} // namespace

TEST_CASE("constructs every BA2 writer target enum value", "[unit][ba2-writer]")
{
    const std::array targets{libbsa::ba2_write_target::fallout4_gnrl_v1,
                             libbsa::ba2_write_target::fallout4_gnrl_v7,
                             libbsa::ba2_write_target::fallout4_gnrl_v8,
                             libbsa::ba2_write_target::starfield_gnrl_v2,
                             libbsa::ba2_write_target::starfield_gnrl_v3,
                             libbsa::ba2_write_target::fallout4_dx10_v1,
                             libbsa::ba2_write_target::fallout4_dx10_v7,
                             libbsa::ba2_write_target::fallout4_dx10_v8,
                             libbsa::ba2_write_target::starfield_dx10_v3};

    CHECK(targets.size() == 9);
    CHECK(targets.front() == libbsa::ba2_write_target::fallout4_gnrl_v1);
    CHECK(targets.back() == libbsa::ba2_write_target::starfield_dx10_v3);
}

TEST_CASE("plans and finalizes BA2 GNRL archives for every required version", "[unit][ba2-writer][roundtrip]")
{
    const auto entries = gnrl_memory_entries();

    for (const auto target : {libbsa::ba2_write_target::fallout4_gnrl_v1,
                              libbsa::ba2_write_target::fallout4_gnrl_v7,
                              libbsa::ba2_write_target::fallout4_gnrl_v8,
                              libbsa::ba2_write_target::starfield_gnrl_v2,
                              libbsa::ba2_write_target::starfield_gnrl_v3}) {
        const auto plan = libbsa::plan_ba2_gnrl_write(target, std::span<const libbsa::ba2_gnrl_memory_entry>{entries});
        REQUIRE(plan.has_value());

        const auto bytes = finalize_to_bytes(plan.value());
        REQUIRE(bytes.size() == plan.value().total_size);
        CHECK(le_u32(bytes, 0) == magic_btdx);
        CHECK(le_u32(bytes, 4) == expected_version(target));
        CHECK(le_u32(bytes, 8) == magic_gnrl);
        CHECK(le_u32(bytes, 12) == entries.size());
        CHECK(le_u64(bytes, 16) == expected_header_size(target) + (entries.size() * gnrl_record_size));

        if (target == libbsa::ba2_write_target::starfield_gnrl_v3) {
            CHECK(le_u32(bytes, 32) == 0U);
        }

        const libbsa::memory_source source{std::span<const std::byte>{bytes}};
        const auto archive = libbsa::open_ba2(source);
        REQUIRE(archive.has_value());
        CHECK(archive.value().summary().subtype == magic_gnrl);
        CHECK(archive.value().summary().version == expected_version(target));
        CHECK(archive.value().summary().file_table_offset == plan.value().gnrl.file_table_offset);

        for (const auto& entry : entries) {
            const auto& planned = find_planned_gnrl_entry(plan.value(), entry.path == "Meshes\\Armor\\Iron.NIF" ? "meshes/armor/iron.nif" : entry.path);
            const auto metadata = archive.value().entry(entry.path);
            REQUIRE(metadata.has_value());
            CHECK(metadata.value().name_hash == planned.name_hash);
            CHECK(metadata.value().directory_hash == planned.directory_hash);
            CHECK(metadata.value().offset == planned.offset);
            CHECK(metadata.value().packed_size == entry.payload.size());
            CHECK(metadata.value().stored_size == entry.payload.size());
            CHECK(metadata.value().size == entry.payload.size());
            CHECK(metadata.value().compression == libbsa::compression_state::raw);
            CHECK(extract_ba2_bytes(bytes, entry.path) == entry.payload);
        }
    }
}

TEST_CASE("GNRL write plans expose native table and payload preview details", "[unit][ba2-writer][preview]")
{
    const auto entries = gnrl_memory_entries();
    const auto plan = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::starfield_gnrl_v3,
                                                  std::span<const libbsa::ba2_gnrl_memory_entry>{entries});

    REQUIRE(plan.has_value());
    CHECK(plan.value().native.subtype == libbsa::ba2_write_subtype::gnrl);
    CHECK(plan.value().native.version == 3U);
    CHECK(plan.value().native.header_size == 36U);
    CHECK(plan.value().native.compression_method == 0U);
    CHECK(plan.value().gnrl.file_table_offset == 36U + (entries.size() * gnrl_record_size));
    REQUIRE(plan.value().gnrl.table_regions.size() >= 2);
    CHECK(plan.value().gnrl.table_regions[0].name == "GNRL record table");
    CHECK(plan.value().gnrl.table_regions[1].name == "FileTableOffset name table");
    REQUIRE(plan.value().gnrl.entries.size() == entries.size());
    REQUIRE(plan.value().data_regions.size() == entries.size());

    for (const auto& planned : plan.value().gnrl.entries) {
        CHECK(planned.packed_size == 0U);
        CHECK(planned.unpacked_size > 0U);
        CHECK(planned.offset >= plan.value().gnrl.file_table_offset);
        CHECK(planned.compression == libbsa::compression_state::raw);
        REQUIRE(planned.data_region_id < plan.value().data_regions.size());
        const auto& region = plan.value().data_regions[planned.data_region_id];
        CHECK(region.offset == planned.offset);
        CHECK(region.stored_size == planned.unpacked_size);
        CHECK(region.unpacked_size == planned.unpacked_size);
        CHECK(region.compression == libbsa::compression_state::raw);
    }
}

TEST_CASE("BA2 DDS planning placeholders fail structurally", "[unit][ba2-writer]")
{
    libbsa::ba2_dds_memory_entry memory_entry{};
    memory_entry.path = "textures/a.dds";
    memory_entry.dds_bytes = {std::byte{0x44}, std::byte{0x44}, std::byte{0x53}, std::byte{0x20}};
    memory_entry.compression = libbsa::compression_policy::force_compressed;
    const std::vector memory_entries{memory_entry};

    libbsa::ba2_dds_disk_entry disk_entry{};
    disk_entry.host_path = "missing-texture.dds";
    disk_entry.path = memory_entry.path;
    disk_entry.compression = memory_entry.compression;
    const std::vector disk_entries{disk_entry};

    require_unsupported_placeholder(libbsa::plan_ba2_dds_write(libbsa::ba2_write_target::fallout4_dx10_v8,
                                                              std::span<const libbsa::ba2_dds_memory_entry>{memory_entries}),
                                    dds_placeholder);
    require_unsupported_placeholder(libbsa::plan_ba2_dds_write_from_disk(libbsa::ba2_write_target::starfield_dx10_v3,
                                                                        std::span<const libbsa::ba2_dds_disk_entry>{disk_entries}),
                                    dds_placeholder);
}

TEST_CASE("BA2 finalization placeholder fails structurally", "[unit][ba2-writer]")
{
    const libbsa::ba2_write_plan plan{};
    libbsa::memory_sink sink;

    require_unsupported_placeholder(libbsa::finalize_ba2_write(plan, sink), finalize_placeholder);
    CHECK(sink.bytes().empty());
}
