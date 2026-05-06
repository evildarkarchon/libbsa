#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/ba2.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/detect.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <array>
#include <cstddef>
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

    auto path = libbsa::normalize_archive_path("textures/actors/hero.dds");
    const libbsa::archive_view view{summary, std::vector{metadata}};
    const libbsa::bsa_archive bsa{summary, std::vector{bsa_metadata}};
    const libbsa::ba2_archive ba2{ba2_summary, std::vector{ba2_metadata}};
    const auto bsa_paths = bsa.paths();
    const auto bsa_entry = bsa.entry("meshes/armor/iron.nif");
    const auto ba2_paths = ba2.paths();
    const auto ba2_entry = ba2.entry("textures/interface/lut.dds");
    const auto open_bsa_fn = &libbsa::open_bsa;
    const auto extract_bsa_entry_fn = &libbsa::extract_bsa_entry;
    const auto open_ba2_fn = &libbsa::open_ba2;
    const auto extract_ba2_entry_fn = &libbsa::extract_ba2_entry;

    return ok.has_value() && source.size() == 2 && write.has_value() && sink.bytes().size() == 2 && codec.has_value() &&
            codec.value() == libbsa::compression_algorithm::lz4_block && write_compression.has_value() &&
            write_compression.value() == libbsa::compression_state::lz4_frame && path.has_value() &&
            path.value().string() == "textures/actors/hero.dds" && view.contains("textures\\actors\\hero.dds") &&
            bsa_paths.size() == 1 && bsa.contains("meshes\\armor\\iron.nif") && bsa_entry.has_value() &&
            ba2_paths.size() == 1 && ba2.contains("textures\\interface\\lut.dds") && ba2_entry.has_value() &&
            open_bsa_fn != nullptr && extract_bsa_entry_fn != nullptr && open_ba2_fn != nullptr &&
            extract_ba2_entry_fn != nullptr
        ? 0
        : 1;
}
