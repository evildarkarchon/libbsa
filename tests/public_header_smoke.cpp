#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/ba2.hpp>
#include <libbsa/ba2_writer.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/bsa_writer.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/detect.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>
#include <libbsa/writer.hpp>

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace {

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

std::uint32_t bc1_mip_size(std::uint32_t width, std::uint32_t height) noexcept
{
    return std::max(1U, (width + 3U) / 4U) * std::max(1U, (height + 3U) / 4U) * 8U;
}

std::vector<std::byte> generated_public_bc1_dds(std::uint32_t width, std::uint32_t height, std::byte seed)
{
    constexpr std::uint32_t dds_header_size = 124U;
    constexpr std::uint32_t dds_pixel_format_size = 32U;
    constexpr std::uint32_t magic_dx10 = 0x30315844U;
    constexpr std::uint32_t dxgi_format_bc1_unorm = 71U;
    constexpr std::uint32_t dds_dimension_texture2d = 3U;
    constexpr std::uint32_t ddscaps_texture = 0x00001000U;

    std::vector<std::byte> bytes;
    bytes.reserve(4U + dds_header_size + 20U + bc1_mip_size(width, height));
    append_u32(bytes, 0x20534444U);
    append_u32(bytes, dds_header_size);
    append_u32(bytes, 0x00000001U | 0x00000002U | 0x00000004U | 0x00001000U | 0x00080000U);
    append_u32(bytes, height);
    append_u32(bytes, width);
    append_u32(bytes, bc1_mip_size(width, height));
    append_u32(bytes, 0U);
    append_u32(bytes, 1U);
    for (int i = 0; i < 11; ++i) {
        append_u32(bytes, 0U);
    }
    append_u32(bytes, dds_pixel_format_size);
    append_u32(bytes, 0x00000004U);
    append_u32(bytes, magic_dx10);
    for (int i = 0; i < 5; ++i) {
        append_u32(bytes, 0U);
    }
    append_u32(bytes, ddscaps_texture);
    append_u32(bytes, 0U);
    append_u32(bytes, 0U);
    append_u32(bytes, 0U);
    append_u32(bytes, 0U);
    append_u32(bytes, dxgi_format_bc1_unorm);
    append_u32(bytes, dds_dimension_texture2d);
    append_u32(bytes, 0U);
    append_u32(bytes, 1U);
    append_u32(bytes, 0U);

    const auto payload_size = bc1_mip_size(width, height);
    for (std::uint32_t index = 0; index < payload_size; ++index) {
        bytes.push_back(static_cast<std::byte>((std::to_integer<unsigned char>(seed) + index) & 0xffU));
    }
    return bytes;
}

bool can_write_and_reopen_bsa(libbsa::bsa_write_target target, const std::string& path)
{
    libbsa::bsa_memory_entry entry{};
    entry.path = path;
    entry.payload = {std::byte{0x41}, std::byte{0x42}, std::byte{0x43}};
    entry.compression = libbsa::compression_policy::force_raw;
    std::vector entries{entry};

    const auto plan = libbsa::plan_bsa_write(target, std::span<const libbsa::bsa_memory_entry>{entries});
    if (!plan.has_value()) {
        return false;
    }

    libbsa::memory_sink archive_sink;
    const auto finalized = libbsa::finalize_bsa_write(plan.value(), archive_sink);
    if (!finalized.has_value()) {
        return false;
    }

    const libbsa::memory_source archive_source{std::span<const std::byte>{archive_sink.bytes()}};
    const auto archive = libbsa::open_bsa(archive_source);
    if (!archive.has_value() || !archive.value().contains(path)) {
        return false;
    }

    libbsa::memory_sink extracted;
    const auto extracted_result = libbsa::extract_bsa_entry(archive.value(), archive_source, path, extracted);
    return extracted_result.has_value() && extracted.bytes() == entry.payload;
}

bool can_write_and_reopen_ba2_gnrl(libbsa::ba2_write_target target, libbsa::ba2_write_options options)
{
    libbsa::ba2_gnrl_memory_entry entry{};
    entry.path = "meshes/public_smoke_ba2.nif";
    entry.payload = {std::byte{0x4e}, std::byte{0x49}, std::byte{0x46}};
    entry.compression = libbsa::compression_policy::force_raw;
    std::vector entries{entry};

    const auto plan = libbsa::plan_ba2_gnrl_write(target, std::span<const libbsa::ba2_gnrl_memory_entry>{entries}, options);
    if (!plan.has_value()) {
        return false;
    }

    libbsa::memory_sink archive_sink;
    const auto finalized = libbsa::finalize_ba2_write(plan.value(), archive_sink);
    if (!finalized.has_value()) {
        return false;
    }

    const libbsa::memory_source archive_source{std::span<const std::byte>{archive_sink.bytes()}};
    const auto archive = libbsa::open_ba2(archive_source);
    if (!archive.has_value() || !archive.value().contains(entry.path)) {
        return false;
    }

    libbsa::memory_sink extracted;
    const auto extracted_result = libbsa::extract_ba2_entry(archive.value(), archive_source, entry.path, extracted);
    return extracted_result.has_value() && extracted.bytes() == entry.payload;
}

bool can_write_and_reopen_ba2_dds(libbsa::ba2_write_target target, libbsa::ba2_write_options options)
{
    libbsa::ba2_dds_memory_entry entry{};
    entry.path = "textures/public_smoke_ba2.dds";
    entry.dds_bytes = generated_public_bc1_dds(4, 4, std::byte{0x70});
    entry.compression = libbsa::compression_policy::force_raw;
    std::vector entries{entry};

    const auto plan = libbsa::plan_ba2_dds_write(target, std::span<const libbsa::ba2_dds_memory_entry>{entries}, options);
    if (!plan.has_value()) {
        return false;
    }

    libbsa::memory_sink archive_sink;
    const auto finalized = libbsa::finalize_ba2_write(plan.value(), archive_sink);
    if (!finalized.has_value()) {
        return false;
    }

    const libbsa::memory_source archive_source{std::span<const std::byte>{archive_sink.bytes()}};
    const auto archive = libbsa::open_ba2(archive_source);
    if (!archive.has_value() || !archive.value().contains(entry.path)) {
        return false;
    }

    const auto texture = archive.value().texture_metadata(entry.path);
    libbsa::memory_sink extracted;
    const auto extracted_result = libbsa::extract_ba2_entry(archive.value(), archive_source, entry.path, extracted);
    return texture.has_value() && texture.value().format.value == 71U && texture.value().width == 4U &&
           texture.value().height == 4U && extracted_result.has_value() && !extracted.bytes().empty();
}

} // namespace

int main()
{
    libbsa::result<void> ok = libbsa::success();
    const std::array bytes{std::byte{0x10}, std::byte{0x20}};
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    libbsa::memory_sink sink;
    const auto write = sink.write(std::span<const std::byte>{bytes});

    libbsa::archive_summary summary{};
    summary.format = libbsa::archive_format::sse_bsa;
    summary.version = 0x69;
    libbsa::archive_summary ba2_summary{};
    ba2_summary.format = libbsa::archive_format::fo4_ba2_gnrl;
    ba2_summary.version = 1;
    ba2_summary.subtype = 0x4c524e47;
    ba2_summary.file_count = 1;
    ba2_summary.file_table_offset = 64;

    libbsa::payload_codec_request codec_request{};
    codec_request.format = libbsa::archive_format::starfield_ba2_gnrl;
    codec_request.entry_state = libbsa::compression_state::archive_default;
    codec_request.compression_method = 3;
    const auto codec = libbsa::resolve_payload_codec(codec_request);
    const auto write_compression = libbsa::resolve_write_compression(libbsa::archive_format::sse_bsa,
                                                                      libbsa::compression_policy::force_compressed,
                                                                      false);

    libbsa::entry_metadata metadata{};
    metadata.path = "textures/actors/hero.dds";
    metadata.size = 2;
    metadata.packed_size = 2;
    metadata.stored_size = 2;
    metadata.offset = 0;
    metadata.compression = libbsa::compression_state::none;
    libbsa::entry_metadata bsa_metadata{};
    bsa_metadata.path = "meshes/armor/iron.nif";
    bsa_metadata.size = 4;
    bsa_metadata.packed_size = 4;
    bsa_metadata.stored_size = 4;
    bsa_metadata.offset = 128;
    bsa_metadata.compression = libbsa::compression_state::raw;
    libbsa::entry_metadata ba2_metadata{};
    ba2_metadata.path = "textures/interface/lut.dds";
    ba2_metadata.size = 4;
    ba2_metadata.packed_size = 4;
    ba2_metadata.stored_size = 4;
    ba2_metadata.offset = 96;
    ba2_metadata.compression = libbsa::compression_state::raw;

    const libbsa::dxgi_format format{71};
    libbsa::texture_chunk_metadata chunk{};
    chunk.mip_level = 0;
    chunk.offset = 96;
    chunk.packed_size = 4;
    chunk.size = 4;
    chunk.compression = libbsa::compression_state::raw;
    libbsa::texture_metadata texture{};
    texture.path = "textures/interface/lut.dds";
    texture.format = format;
    texture.width = 16;
    texture.height = 16;
    texture.mip_count = 1;
    texture.array_size = 1;
    texture.is_cubemap = false;
    texture.chunks.push_back(chunk);

    auto path = libbsa::normalize_archive_path("textures/actors/hero.dds");
    const libbsa::archive_view view{summary, std::vector{metadata}};
    const libbsa::bsa_archive bsa{summary, std::vector{bsa_metadata}};
    const libbsa::ba2_archive ba2{ba2_summary, std::vector{ba2_metadata}, std::vector{texture}};
    const auto bsa_paths = bsa.paths();
    const auto bsa_entry = bsa.entry("meshes/armor/iron.nif");
    const auto ba2_paths = ba2.paths();
    const auto ba2_entry = ba2.entry("textures/interface/lut.dds");
    const auto ba2_texture = ba2.texture_metadata("textures/interface/lut.dds");
    const auto missing_texture = ba2.texture_metadata("meshes/armor/iron.nif");
    const auto open_bsa_fn = &libbsa::open_bsa;
    const auto extract_bsa_entry_fn = &libbsa::extract_bsa_entry;
    const auto open_ba2_fn = &libbsa::open_ba2;
    const auto extract_ba2_entry_fn = &libbsa::extract_ba2_entry;

    libbsa::writer_target target{};
    target.format = libbsa::archive_format::fo4_ba2_gnrl;
    target.archive_default_compressed = false;
    target.supports_compression = true;
    target.supports_shared_data_regions = true;

    libbsa::writer_entry writer_first{};
    writer_first.path = "meshes/armor/iron.nif";
    writer_first.payload = {std::byte{0x01}, std::byte{0x02}};
    writer_first.compression = libbsa::compression_policy::force_raw;
    libbsa::writer_entry writer_second{};
    writer_second.path = "textures/interface/lut.dds";
    writer_second.payload = {std::byte{0x03}};
    writer_second.compression = libbsa::compression_policy::force_raw;
    std::vector writer_entries{writer_first, writer_second};
    const auto writer_plan = libbsa::plan_archive_write(
        target, std::span<const libbsa::writer_entry>{writer_entries}, libbsa::writer_options{.deduplicate = true});
    libbsa::memory_sink writer_sink;
    const auto writer_finalized = writer_plan.has_value() ? libbsa::finalize_archive_write(writer_plan.value(), writer_sink)
                                                           : libbsa::failure<void>({libbsa::error_code::unsupported_format,
                                                                                    "writer smoke planning failed"});
    const auto plan_writer_fn = &libbsa::plan_archive_write;
    const auto finalize_writer_fn = &libbsa::finalize_archive_write;
    const auto plan_bsa_write_fn = &libbsa::plan_bsa_write;
    const auto plan_bsa_write_from_disk_fn = &libbsa::plan_bsa_write_from_disk;
    const auto finalize_bsa_write_fn = &libbsa::finalize_bsa_write;
    const auto plan_ba2_gnrl_write_fn = &libbsa::plan_ba2_gnrl_write;
    const auto plan_ba2_gnrl_write_from_disk_fn = &libbsa::plan_ba2_gnrl_write_from_disk;
    const auto plan_ba2_dds_write_fn = &libbsa::plan_ba2_dds_write;
    const auto plan_ba2_dds_write_from_disk_fn = &libbsa::plan_ba2_dds_write_from_disk;
    const auto finalize_ba2_write_fn = &libbsa::finalize_ba2_write;
    const bool bsa_writer_smoke_ok =
        can_write_and_reopen_bsa(libbsa::bsa_write_target::tes3_morrowind, "meshes/public_smoke_tes3.nif") &&
        can_write_and_reopen_bsa(libbsa::bsa_write_target::oblivion_v103, "meshes/public_smoke_v103.nif") &&
        can_write_and_reopen_bsa(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104, "meshes/public_smoke_v104.nif") &&
        can_write_and_reopen_bsa(libbsa::bsa_write_target::skyrim_se_ae_v105, "meshes/public_smoke_v105.nif");
    libbsa::ba2_write_options starfield_method3{};
    starfield_method3.starfield_v3_compression_method = 3U;
    const bool ba2_writer_smoke_ok =
        can_write_and_reopen_ba2_gnrl(libbsa::ba2_write_target::fallout4_gnrl_v1, libbsa::ba2_write_options{}) &&
        can_write_and_reopen_ba2_gnrl(libbsa::ba2_write_target::starfield_gnrl_v3, starfield_method3) &&
        can_write_and_reopen_ba2_dds(libbsa::ba2_write_target::fallout4_dx10_v1, libbsa::ba2_write_options{}) &&
        can_write_and_reopen_ba2_dds(libbsa::ba2_write_target::starfield_dx10_v3, starfield_method3);

    return ok.has_value() && source.size() == 2 && write.has_value() && sink.bytes().size() == 2 && codec.has_value() &&
            codec.value() == libbsa::compression_algorithm::lz4_block && write_compression.has_value() &&
            write_compression.value() == libbsa::compression_state::lz4_frame && path.has_value() &&
            path.value().string() == "textures/actors/hero.dds" && view.contains("textures\\actors\\hero.dds") &&
            bsa_paths.size() == 1 && bsa.contains("meshes\\armor\\iron.nif") && bsa_entry.has_value() &&
            ba2_paths.size() == 1 && ba2.contains("textures\\interface\\lut.dds") && ba2_entry.has_value() &&
            format.value == 71 && libbsa::dxgi_format_name(format) == "BC1_UNORM" && ba2_texture.has_value() &&
            ba2_texture.value().format.value == 71 && ba2_texture.value().width == 16 && ba2_texture.value().height == 16 &&
            ba2_texture.value().mip_count == 1 && ba2_texture.value().array_size == 1 && !ba2_texture.value().is_cubemap &&
            ba2_texture.value().chunks.size() == 1 && ba2_texture.value().chunks.front().offset == 96 &&
            !missing_texture.has_value() && missing_texture.error().code == libbsa::error_code::malformed_archive &&
            open_bsa_fn != nullptr && extract_bsa_entry_fn != nullptr && open_ba2_fn != nullptr &&
            extract_ba2_entry_fn != nullptr && writer_plan.has_value() && writer_plan.value().entries.size() == 2 &&
            writer_plan.value().table_regions.size() == 4 && writer_plan.value().data_regions.size() == 2 &&
            writer_plan.value().total_size == writer_sink.bytes().size() && writer_finalized.has_value() &&
            plan_writer_fn != nullptr && finalize_writer_fn != nullptr && plan_bsa_write_fn != nullptr &&
            plan_bsa_write_from_disk_fn != nullptr && finalize_bsa_write_fn != nullptr && plan_ba2_gnrl_write_fn != nullptr &&
            plan_ba2_gnrl_write_from_disk_fn != nullptr && plan_ba2_dds_write_fn != nullptr &&
            plan_ba2_dds_write_from_disk_fn != nullptr && finalize_ba2_write_fn != nullptr && bsa_writer_smoke_ok &&
            ba2_writer_smoke_ok
        ? 0
        : 1;
}
