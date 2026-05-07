#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/ba2.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/detect.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>
#include <libbsa/writer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

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
            plan_writer_fn != nullptr && finalize_writer_fn != nullptr
        ? 0
        : 1;
}
