#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
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
    metadata.offset = 0;
    metadata.compression = libbsa::compression_state::none;

    auto path = libbsa::normalize_archive_path("textures/actors/hero.dds");
    const libbsa::archive_view view{summary, std::vector{metadata}};

    return ok.has_value() && source.size() == 2 && write.has_value() && sink.bytes().size() == 2 && codec.has_value() &&
            codec.value() == libbsa::compression_algorithm::lz4_block && write_compression.has_value() &&
            write_compression.value() == libbsa::compression_state::lz4_frame && path.has_value() &&
            path.value().string() == "textures/actors/hero.dds" && view.contains("textures\\actors\\hero.dds")
        ? 0
        : 1;
}
