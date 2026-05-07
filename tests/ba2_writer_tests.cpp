#include <catch2/catch_test_macros.hpp>

#include <libbsa/ba2.hpp>
#include <libbsa/ba2_writer.hpp>

#include "texture/dds_validation.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view dds_placeholder = "BA2 DDS writer planning is not implemented";

constexpr std::uint32_t magic_btdx = 0x58445442U;
constexpr std::uint32_t magic_gnrl = 0x4c524e47U;
constexpr std::uint32_t magic_dx10 = 0x30315844U;
constexpr std::uint64_t gnrl_record_size = 36U;
constexpr std::uint32_t dds_header_size = 124U;
constexpr std::uint32_t dds_pixel_format_size = 32U;
constexpr std::uint32_t dxgi_format_bc1_unorm = 71U;
constexpr std::uint32_t dds_dimension_texture2d = 3U;
constexpr std::uint32_t dds_resource_misc_texturecube = 0x00000004U;
constexpr std::uint32_t ddscaps_complex = 0x00000008U;
constexpr std::uint32_t ddscaps_texture = 0x00001000U;
constexpr std::uint32_t ddscaps_mipmap = 0x00400000U;
constexpr std::uint32_t ddscaps2_cubemap_all_faces = 0x0000fe00U;

template <typename Result>
void require_unsupported_placeholder(const Result& result, std::string_view message)
{
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::unsupported_format);
    CHECK(result.error().message.find(message) != std::string::npos);
}

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
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
    case libbsa::ba2_write_target::fallout4_dx10_v1:
        return 1U;
    case libbsa::ba2_write_target::fallout4_dx10_v7:
        return 7U;
    case libbsa::ba2_write_target::fallout4_dx10_v8:
        return 8U;
    case libbsa::ba2_write_target::starfield_dx10_v3:
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
    case libbsa::ba2_write_target::starfield_dx10_v3:
        return 36U;
    default:
        return 24U;
    }
}

std::uint32_t bc1_mip_size(std::uint32_t width, std::uint32_t height) noexcept
{
    return std::max(1U, (width + 3U) / 4U) * std::max(1U, (height + 3U) / 4U) * 8U;
}

std::vector<std::byte> deterministic_dds_payload(std::size_t size, std::byte seed)
{
    std::vector<std::byte> payload(size);
    for (std::size_t index = 0; index < payload.size(); ++index) {
        payload[index] = static_cast<std::byte>((std::to_integer<unsigned char>(seed) + index) & 0xffU);
    }
    return payload;
}

std::vector<std::byte> generated_bc1_dds(std::uint32_t width,
                                         std::uint32_t height,
                                         std::uint32_t mip_count,
                                         std::uint32_t array_size,
                                         bool cubemap,
                                         std::byte seed)
{
    std::size_t payload_size = 0;
    for (std::uint32_t item = 0; item < array_size; ++item) {
        for (std::uint32_t mip = 0; mip < mip_count; ++mip) {
            payload_size += bc1_mip_size(std::max(1U, width >> mip), std::max(1U, height >> mip));
        }
    }

    std::vector<std::byte> bytes;
    bytes.reserve(4U + dds_header_size + 20U + payload_size);
    append_u32(bytes, 0x20534444U);
    append_u32(bytes, dds_header_size);
    append_u32(bytes, 0x00000001U | 0x00000002U | 0x00000004U | 0x00001000U | 0x00080000U |
                          (mip_count > 1 ? 0x00020000U : 0U));
    append_u32(bytes, height);
    append_u32(bytes, width);
    append_u32(bytes, bc1_mip_size(width, height) * array_size);
    append_u32(bytes, 0U);
    append_u32(bytes, mip_count);
    for (int i = 0; i < 11; ++i) {
        append_u32(bytes, 0U);
    }
    append_u32(bytes, dds_pixel_format_size);
    append_u32(bytes, 0x00000004U);
    append_u32(bytes, magic_dx10);
    for (int i = 0; i < 5; ++i) {
        append_u32(bytes, 0U);
    }
    append_u32(bytes, ddscaps_texture | (mip_count > 1 ? (ddscaps_complex | ddscaps_mipmap) : 0U) |
                          (array_size > 1 || cubemap ? ddscaps_complex : 0U));
    append_u32(bytes, cubemap ? ddscaps2_cubemap_all_faces : 0U);
    append_u32(bytes, 0U);
    append_u32(bytes, 0U);
    append_u32(bytes, 0U);
    append_u32(bytes, dxgi_format_bc1_unorm);
    append_u32(bytes, dds_dimension_texture2d);
    append_u32(bytes, cubemap ? dds_resource_misc_texturecube : 0U);
    // The DDS DX10 header stores cubemap arrays as cube counts; DirectXTex expands one cube to six faces.
    append_u32(bytes, cubemap ? std::max(1U, array_size / 6U) : array_size);
    append_u32(bytes, 0U);

    auto payload = deterministic_dds_payload(payload_size, seed);
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

libbsa::ba2_dds_memory_entry dds_entry(std::string path,
                                       std::vector<std::byte> dds_bytes,
                                       libbsa::compression_policy compression = libbsa::compression_policy::force_raw)
{
    libbsa::ba2_dds_memory_entry entry{};
    entry.path = std::move(path);
    entry.dds_bytes = std::move(dds_bytes);
    entry.compression = compression;
    return entry;
}

void require_valid_dds(std::span<const std::byte> bytes,
                       std::uint32_t width,
                       std::uint32_t height,
                       std::uint32_t mip_count,
                       std::uint32_t array_size,
                       bool cubemap)
{
    const auto validated = libbsa::detail::validate_dds(bytes);
    REQUIRE(validated.has_value());
    CHECK(validated.value().width == width);
    CHECK(validated.value().height == height);
    CHECK(validated.value().mip_count == mip_count);
    CHECK(validated.value().array_size == array_size);
    CHECK(validated.value().is_cubemap == cubemap);
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

libbsa::ba2_gnrl_memory_entry gnrl_entry(std::string path,
                                         std::vector<std::byte> payload,
                                         libbsa::compression_policy compression = libbsa::compression_policy::archive_default)
{
    libbsa::ba2_gnrl_memory_entry entry{};
    entry.path = std::move(path);
    entry.payload = std::move(payload);
    entry.compression = compression;
    return entry;
}

libbsa::ba2_gnrl_disk_entry gnrl_disk_entry(std::filesystem::path host_path,
                                            std::string path,
                                            libbsa::compression_policy compression = libbsa::compression_policy::archive_default)
{
    libbsa::ba2_gnrl_disk_entry entry{};
    entry.host_path = host_path.string();
    entry.path = std::move(path);
    entry.compression = compression;
    return entry;
}

std::vector<std::byte> ascii_bytes(std::string_view text)
{
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (char ch : text) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return bytes;
}

std::filesystem::path unique_temp_file(std::string_view stem)
{
    static int counter = 0;
    return std::filesystem::temp_directory_path() /
           (std::string{"libbsa-ba2-writer-"} + std::string{stem} + "-" + std::to_string(++counter) + ".bin");
}

void write_temp_file(const std::filesystem::path& path, std::span<const std::byte> bytes)
{
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    REQUIRE(output.good());
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    REQUIRE(output.good());
}

class failing_sink final : public libbsa::byte_sink {
public:
    explicit failing_sink(std::size_t fail_after) : fail_after_(fail_after) {}

    [[nodiscard]] libbsa::result<void> write(std::span<const std::byte> bytes) override
    {
        if (written_ + bytes.size() > fail_after_) {
            return libbsa::failure<void>({libbsa::error_code::io_failure, "first sink failure"});
        }
        written_ += bytes.size();
        return libbsa::success();
    }

private:
    std::size_t fail_after_{};
    std::size_t written_{};
};

void require_gnrl_archives_equivalent(std::span<const std::byte> memory_bytes,
                                      std::span<const std::byte> disk_bytes,
                                      std::span<const libbsa::ba2_gnrl_memory_entry> entries)
{
    const libbsa::memory_source memory_source{memory_bytes};
    const libbsa::memory_source disk_source{disk_bytes};
    const auto memory_archive = libbsa::open_ba2(memory_source);
    const auto disk_archive = libbsa::open_ba2(disk_source);
    REQUIRE(memory_archive.has_value());
    REQUIRE(disk_archive.has_value());
    CHECK(memory_archive.value().paths() == disk_archive.value().paths());

    for (const auto& entry : entries) {
        const auto memory_metadata = memory_archive.value().entry(entry.path);
        const auto disk_metadata = disk_archive.value().entry(entry.path);
        REQUIRE(memory_metadata.has_value());
        REQUIRE(disk_metadata.has_value());
        CHECK(memory_metadata.value().path == disk_metadata.value().path);
        CHECK(memory_metadata.value().size == disk_metadata.value().size);
        CHECK(memory_metadata.value().stored_size == disk_metadata.value().stored_size);
        CHECK(memory_metadata.value().compression == disk_metadata.value().compression);
        CHECK(memory_metadata.value().directory_hash == disk_metadata.value().directory_hash);
        CHECK(memory_metadata.value().name_hash == disk_metadata.value().name_hash);
        CHECK(extract_ba2_bytes(std::vector<std::byte>{memory_bytes.begin(), memory_bytes.end()}, entry.path) == entry.payload);
        CHECK(extract_ba2_bytes(std::vector<std::byte>{disk_bytes.begin(), disk_bytes.end()}, entry.path) == entry.payload);
    }
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
    const std::vector memory_entries{dds_entry("textures/a.dds", {std::byte{0x44}, std::byte{0x44}, std::byte{0x53}, std::byte{0x20}})};

    const auto malformed = libbsa::plan_ba2_dds_write(libbsa::ba2_write_target::fallout4_dx10_v8,
                                                     std::span<const libbsa::ba2_dds_memory_entry>{memory_entries});

    REQUIRE_FALSE(malformed.has_value());
    CHECK(malformed.error().code == libbsa::error_code::malformed_archive);
}

TEST_CASE("plans BA2 DX10 archives from in-memory DDS inputs", "[unit][ba2-writer][roundtrip][fixture]")
{
    const std::vector entries{dds_entry("textures/generated/one_mip.dds", generated_bc1_dds(4, 4, 1, 1, false, std::byte{0x10})),
                              dds_entry("textures/generated/multi_mip.dds", generated_bc1_dds(512, 512, 4, 1, false, std::byte{0x20})),
                              dds_entry("textures/generated/cubemap.dds", generated_bc1_dds(4, 4, 1, 6, true, std::byte{0x30})),
                              dds_entry("textures/generated/array.dds", generated_bc1_dds(4, 4, 1, 4, false, std::byte{0x40}))};

    for (const auto target : {libbsa::ba2_write_target::fallout4_dx10_v1,
                              libbsa::ba2_write_target::fallout4_dx10_v7,
                              libbsa::ba2_write_target::fallout4_dx10_v8,
                              libbsa::ba2_write_target::starfield_dx10_v3}) {
        const auto plan = libbsa::plan_ba2_dds_write(target, std::span<const libbsa::ba2_dds_memory_entry>{entries});
        REQUIRE(plan.has_value());
        CHECK(plan.value().native.subtype == libbsa::ba2_write_subtype::dx10);
        CHECK(plan.value().native.version == expected_version(target));
        CHECK(plan.value().dds.textures.size() == entries.size());

        const auto bytes = finalize_to_bytes(plan.value());
        CHECK(le_u32(bytes, 0) == magic_btdx);
        CHECK(le_u32(bytes, 4) == expected_version(target));
        CHECK(le_u32(bytes, 8) == magic_dx10);
        CHECK(le_u32(bytes, 12) == entries.size());
        CHECK(le_u64(bytes, 16) == plan.value().dds.file_table_offset);

        const libbsa::memory_source source{std::span<const std::byte>{bytes}};
        const auto archive = libbsa::open_ba2(source);
        REQUIRE(archive.has_value());

        const auto one_mip = archive.value().texture_metadata("textures/generated/one_mip.dds");
        REQUIRE(one_mip.has_value());
        CHECK(one_mip.value().format.value == dxgi_format_bc1_unorm);
        CHECK(one_mip.value().width == 4U);
        CHECK(one_mip.value().height == 4U);
        CHECK(one_mip.value().mip_count == 1U);
        CHECK(one_mip.value().array_size == 1U);
        REQUIRE(one_mip.value().chunks.size() == 1U);
        CHECK(one_mip.value().chunks.front().offset >= plan.value().dds.file_table_offset);
        require_valid_dds(extract_ba2_bytes(bytes, "textures/generated/one_mip.dds"), 4, 4, 1, 1, false);
        require_valid_dds(extract_ba2_bytes(bytes, "textures/generated/multi_mip.dds"), 512, 512, 4, 1, false);
        require_valid_dds(extract_ba2_bytes(bytes, "textures/generated/cubemap.dds"), 4, 4, 1, 6, true);
        require_valid_dds(extract_ba2_bytes(bytes, "textures/generated/array.dds"), 4, 4, 1, 4, false);
    }
}

TEST_CASE("DX10 write plans expose texture and chunk preview details", "[unit][ba2-writer][preview]")
{
    const std::vector entries{dds_entry("textures/generated/multi_mip.dds", generated_bc1_dds(512, 512, 4, 1, false, std::byte{0x50})),
                              dds_entry("textures/generated/cubemap.dds", generated_bc1_dds(4, 4, 1, 6, true, std::byte{0x60})),
                              dds_entry("textures/generated/array.dds", generated_bc1_dds(4, 4, 1, 4, false, std::byte{0x70}))};

    const auto plan = libbsa::plan_ba2_dds_write(libbsa::ba2_write_target::fallout4_dx10_v8,
                                                std::span<const libbsa::ba2_dds_memory_entry>{entries});

    REQUIRE(plan.has_value());
    REQUIRE(plan.value().dds.table_regions.size() >= 2U);
    CHECK(plan.value().dds.table_regions[0].name == "DX10 texture and chunk table");
    CHECK(plan.value().dds.table_regions[1].name == "FileTableOffset name table");
    REQUIRE(plan.value().dds.textures.size() == 3U);

    const auto& multi = plan.value().dds.textures[1];
    CHECK(multi.path == "textures/generated/multi_mip.dds");
    CHECK(multi.width == 512U);
    CHECK(multi.height == 512U);
    CHECK(multi.mip_count == 4U);
    REQUIRE(multi.chunks.size() == 2U);
    CHECK(multi.chunks[0].start_mip == 0U);
    CHECK(multi.chunks[0].end_mip == 0U);
    CHECK(multi.chunks[1].start_mip == 1U);
    CHECK(multi.chunks[1].end_mip == 3U);
    CHECK(multi.chunks[0].offset < multi.chunks[1].offset);
    CHECK(multi.chunks[0].packed_size == multi.chunks[0].unpacked_size);

    const auto& cubemap = plan.value().dds.textures[0];
    CHECK(cubemap.path == "textures/generated/cubemap.dds");
    CHECK(cubemap.is_cubemap);
    CHECK(cubemap.array_size == 6U);

    const auto& array = plan.value().dds.textures[2];
    CHECK(array.path == "textures/generated/array.dds");
    CHECK_FALSE(array.is_cubemap);
    CHECK(array.array_size == 4U);
}

TEST_CASE("BA2 finalization streams an empty plan without touching payload regions", "[unit][ba2-writer]")
{
    const libbsa::ba2_write_plan plan{};
    libbsa::memory_sink sink;

    const auto finalized = libbsa::finalize_ba2_write(plan, sink);

    REQUIRE(finalized.has_value());
    CHECK(sink.bytes().empty());
}

TEST_CASE("disk-backed and memory-backed BA2 GNRL inputs read back equivalently", "[unit][ba2-writer][disk][roundtrip]")
{
    const std::vector memory_entries{gnrl_entry("meshes/from-disk.nif", ascii_bytes("disk mesh"), libbsa::compression_policy::force_raw),
                                     gnrl_entry("textures/from-disk.dds", ascii_bytes("disk texture"), libbsa::compression_policy::force_raw)};
    const auto mesh_path = unique_temp_file("mesh");
    const auto texture_path = unique_temp_file("texture");
    write_temp_file(mesh_path, memory_entries[0].payload);
    write_temp_file(texture_path, memory_entries[1].payload);
    const std::vector disk_entries{gnrl_disk_entry(mesh_path, memory_entries[0].path, memory_entries[0].compression),
                                   gnrl_disk_entry(texture_path, memory_entries[1].path, memory_entries[1].compression)};

    const auto memory_plan = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v7,
                                                        std::span<const libbsa::ba2_gnrl_memory_entry>{memory_entries});
    const auto disk_plan = libbsa::plan_ba2_gnrl_write_from_disk(libbsa::ba2_write_target::fallout4_gnrl_v7,
                                                                std::span<const libbsa::ba2_gnrl_disk_entry>{disk_entries});

    REQUIRE(memory_plan.has_value());
    REQUIRE(disk_plan.has_value());
    require_gnrl_archives_equivalent(finalize_to_bytes(memory_plan.value()), finalize_to_bytes(disk_plan.value()), memory_entries);
}

TEST_CASE("disk-backed BA2 GNRL files are read during planning and not reopened during finalization", "[unit][ba2-writer][disk]")
{
    const auto host_path = unique_temp_file("owned-by-plan");
    const auto original = ascii_bytes("original BA2 disk bytes");
    write_temp_file(host_path, original);
    const std::vector disk_entries{gnrl_disk_entry(host_path, "meshes/owned.nif", libbsa::compression_policy::force_raw)};

    const auto plan = libbsa::plan_ba2_gnrl_write_from_disk(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                           std::span<const libbsa::ba2_gnrl_disk_entry>{disk_entries});
    REQUIRE(plan.has_value());
    write_temp_file(host_path, ascii_bytes("mutated bytes that must not leak into finalization"));
    std::filesystem::remove(host_path);

    const auto bytes = finalize_to_bytes(plan.value());

    CHECK(extract_ba2_bytes(bytes, "meshes/owned.nif") == original);
}

TEST_CASE("BA2 GNRL compression routes raw deflate and Starfield LZ4-block payloads", "[unit][ba2-writer][codec][roundtrip]")
{
    const auto raw_payload = ascii_bytes("raw bytes stay raw");
    const auto packed_payload = ascii_bytes("compressible BA2 payload compressible BA2 payload compressible BA2 payload");
    const std::vector entries{gnrl_entry("meshes/raw.nif", raw_payload, libbsa::compression_policy::force_raw),
                              gnrl_entry("meshes/packed.nif", packed_payload, libbsa::compression_policy::force_compressed)};

    const auto deflate_plan = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v8,
                                                         std::span<const libbsa::ba2_gnrl_memory_entry>{entries});
    REQUIRE(deflate_plan.has_value());
    CHECK(find_planned_gnrl_entry(deflate_plan.value(), "meshes/raw.nif").packed_size == 0U);
    CHECK(find_planned_gnrl_entry(deflate_plan.value(), "meshes/raw.nif").compression == libbsa::compression_state::raw);
    CHECK(find_planned_gnrl_entry(deflate_plan.value(), "meshes/packed.nif").packed_size > 0U);
    CHECK(find_planned_gnrl_entry(deflate_plan.value(), "meshes/packed.nif").compression == libbsa::compression_state::deflate);
    const auto deflate_bytes = finalize_to_bytes(deflate_plan.value());
    CHECK(extract_ba2_bytes(deflate_bytes, "meshes/raw.nif") == raw_payload);
    CHECK(extract_ba2_bytes(deflate_bytes, "meshes/packed.nif") == packed_payload);

    libbsa::ba2_write_options lz4_options{};
    lz4_options.archive_default_compressed = true;
    lz4_options.starfield_v3_compression_method = 3U;
    const std::vector starfield_entries{gnrl_entry("meshes/default.nif", packed_payload),
                                        gnrl_entry("meshes/forced-raw.nif", raw_payload, libbsa::compression_policy::force_raw)};
    const auto lz4_plan = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::starfield_gnrl_v3,
                                                     std::span<const libbsa::ba2_gnrl_memory_entry>{starfield_entries},
                                                     lz4_options);
    REQUIRE(lz4_plan.has_value());
    CHECK(lz4_plan.value().native.compression_method == 3U);
    CHECK(find_planned_gnrl_entry(lz4_plan.value(), "meshes/default.nif").compression == libbsa::compression_state::lz4_block);
    CHECK(find_planned_gnrl_entry(lz4_plan.value(), "meshes/forced-raw.nif").compression == libbsa::compression_state::raw);
    const auto lz4_bytes = finalize_to_bytes(lz4_plan.value());
    CHECK(extract_ba2_bytes(lz4_bytes, "meshes/default.nif") == packed_payload);
    CHECK(extract_ba2_bytes(lz4_bytes, "meshes/forced-raw.nif") == raw_payload);

    libbsa::ba2_write_options unsupported_method{};
    unsupported_method.archive_default_compressed = true;
    unsupported_method.starfield_v3_compression_method = 7U;
    const auto unsupported = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::starfield_gnrl_v3,
                                                        std::span<const libbsa::ba2_gnrl_memory_entry>{starfield_entries},
                                                        unsupported_method);
    REQUIRE_FALSE(unsupported.has_value());
    CHECK(unsupported.error().code == libbsa::error_code::unsupported_format);
}

TEST_CASE("BA2 GNRL dedup shares exact post-policy stored payloads", "[unit][ba2-writer][dedup]")
{
    const auto shared_payload = ascii_bytes("same BA2 source bytes same BA2 source bytes");
    const std::vector entries{gnrl_entry("meshes/a.nif", shared_payload, libbsa::compression_policy::force_compressed),
                              gnrl_entry("textures/a.dds", shared_payload, libbsa::compression_policy::force_compressed),
                              gnrl_entry("sounds/a.wav", ascii_bytes("different BA2 bytes"), libbsa::compression_policy::force_compressed)};
    libbsa::ba2_write_options dedup_options{};
    dedup_options.deduplicate = true;
    const auto dedup_plan = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v8,
                                                       std::span<const libbsa::ba2_gnrl_memory_entry>{entries},
                                                       dedup_options);
    REQUIRE(dedup_plan.has_value());
    const auto& first = find_planned_gnrl_entry(dedup_plan.value(), "meshes/a.nif");
    const auto& second = find_planned_gnrl_entry(dedup_plan.value(), "textures/a.dds");
    const auto& different = find_planned_gnrl_entry(dedup_plan.value(), "sounds/a.wav");
    CHECK(first.data_region_id == second.data_region_id);
    CHECK(first.offset == second.offset);
    CHECK(first.data_region_id != different.data_region_id);
    CHECK(dedup_plan.value().data_regions.size() == 2);

    const auto no_dedup_plan = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v8,
                                                          std::span<const libbsa::ba2_gnrl_memory_entry>{entries});
    REQUIRE(no_dedup_plan.has_value());
    CHECK(find_planned_gnrl_entry(no_dedup_plan.value(), "meshes/a.nif").offset !=
          find_planned_gnrl_entry(no_dedup_plan.value(), "textures/a.dds").offset);
    CHECK(no_dedup_plan.value().data_regions.size() == entries.size());
}

TEST_CASE("BA2 GNRL writer rejects invalid inputs and preserves sink failures", "[unit][ba2-writer][failure]")
{
    const std::vector duplicate_entries{gnrl_entry("Meshes/Armor/Iron.NIF", ascii_bytes("first")),
                                        gnrl_entry("meshes\\armor\\iron.nif", ascii_bytes("second"))};
    const auto duplicate = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                      std::span<const libbsa::ba2_gnrl_memory_entry>{duplicate_entries});
    REQUIRE_FALSE(duplicate.has_value());
    CHECK(duplicate.error().code == libbsa::error_code::malformed_archive);

    const std::vector invalid_path_entries{gnrl_entry("", ascii_bytes("payload"))};
    const auto invalid_path = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                         std::span<const libbsa::ba2_gnrl_memory_entry>{invalid_path_entries});
    REQUIRE_FALSE(invalid_path.has_value());
    CHECK(invalid_path.error().code == libbsa::error_code::malformed_archive);

    const auto existing_path = unique_temp_file("duplicate-existing");
    const auto missing_path = unique_temp_file("duplicate-missing");
    write_temp_file(existing_path, ascii_bytes("first"));
    std::filesystem::remove(missing_path);
    const std::vector duplicate_disk_entries{gnrl_disk_entry(existing_path, "Meshes/Armor/Iron.NIF"),
                                             gnrl_disk_entry(missing_path, "meshes\\armor\\iron.nif")};
    const auto duplicate_disk = libbsa::plan_ba2_gnrl_write_from_disk(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                                     std::span<const libbsa::ba2_gnrl_disk_entry>{duplicate_disk_entries});
    REQUIRE_FALSE(duplicate_disk.has_value());
    CHECK(duplicate_disk.error().code == libbsa::error_code::malformed_archive);

    const std::vector missing_disk_entries{gnrl_disk_entry(missing_path, "meshes/missing.nif", libbsa::compression_policy::force_raw)};
    const auto missing_disk = libbsa::plan_ba2_gnrl_write_from_disk(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                                   std::span<const libbsa::ba2_gnrl_disk_entry>{missing_disk_entries});
    REQUIRE_FALSE(missing_disk.has_value());
    CHECK(missing_disk.error().code == libbsa::error_code::io_failure);

    const std::string too_long_name(static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) + 1U, 'a');
    const std::vector overflow_entries{gnrl_entry("meshes/" + too_long_name + ".nif", ascii_bytes("payload"))};
    const auto overflow = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                     std::span<const libbsa::ba2_gnrl_memory_entry>{overflow_entries});
    REQUIRE_FALSE(overflow.has_value());
    CHECK(overflow.error().code == libbsa::error_code::malformed_archive);

    const std::vector misuse_entries{gnrl_entry("meshes/a.nif", ascii_bytes("payload"), libbsa::compression_policy::force_raw)};
    const auto subtype_misuse = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_dx10_v1,
                                                           std::span<const libbsa::ba2_gnrl_memory_entry>{misuse_entries});
    REQUIRE_FALSE(subtype_misuse.has_value());
    CHECK(subtype_misuse.error().code == libbsa::error_code::unsupported_format);

    const auto plan = libbsa::plan_ba2_gnrl_write(libbsa::ba2_write_target::fallout4_gnrl_v1,
                                                 std::span<const libbsa::ba2_gnrl_memory_entry>{misuse_entries});
    REQUIRE(plan.has_value());
    failing_sink sink{8};
    const auto finalized = libbsa::finalize_ba2_write(plan.value(), sink);
    REQUIRE_FALSE(finalized.has_value());
    CHECK(finalized.error().code == libbsa::error_code::io_failure);
    CHECK(finalized.error().message == "first sink failure");
}
