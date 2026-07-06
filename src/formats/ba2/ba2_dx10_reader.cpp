#include "formats/ba2/ba2_dx10_reader.hpp"

#include "formats/ba2/ba2_profile.hpp"

#include <detail/archive_path.hpp>
#include <detail/compression_router.hpp>
#include <detail/host_file.hpp>
#include <detail/payload_stream.hpp>

#include "texture/dds_layout.hpp"

#include <algorithm>
#include <span>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

detail::host_file_context ba2_dx10_extraction_host_context() noexcept {
    return detail::host_file_context{
        "failed to open BA2 archive host path for DX10 extraction",
        "failed to inspect BA2 archive host path for DX10 extraction",
        "failed while reading BA2 DX10 chunk payload",
        "BA2 archive host path changed while reading DX10 chunk payload", "BA2 DX10 payload bytes"};
}

result<void> stream_raw_chunk(std::ifstream& input, const texture_chunk_metadata& chunk,
                              payload_sink& sink) {
    if (chunk.raw_size != chunk.stored_size) {
        return error{error_code::format_error,
                     "BA2 DX10 raw chunk size does not match stored size"};
    }
    return detail::stream_payload_range(input, chunk.payload_offset, chunk.stored_size, sink,
                                        extraction_chunk_size, "BA2 DX10 chunk payload");
}

result<void> extract_compressed_chunk(std::ifstream& input, const texture_chunk_metadata& chunk,
                                      payload_sink& sink) {
    auto method = ba2_compressed_payload_method(ba2_subtype::dx10, chunk.compression);
    if (!method) {
        return method.error();
    }
    // D-32: every compressed texture chunk must decode to exactly the
    // parser-declared raw size.
    return detail::decompress_payload_exact_to_sink(
        method.value(), input, chunk.payload_offset, chunk.stored_size, chunk.raw_size, sink,
        extraction_chunk_size, "BA2 DX10 compressed chunk");
}

}  // namespace

result<std::vector<entry_metadata>> ba2_dx10_entries(std::span<const entry_metadata> entries) {
    return std::vector<entry_metadata>{entries.begin(), entries.end()};
}

result<std::optional<entry_metadata>> find_ba2_dx10_entry(std::span<const entry_metadata> entries,
                                                          std::string_view path) {
    auto normalized = detail::normalize_archive_path(path);
    if (!normalized) {
        return normalized.error();
    }

    const auto found = std::lower_bound(
        entries.begin(), entries.end(), normalized.value().value,
        [](const entry_metadata& entry, const std::string& key) { return entry.path < key; });
    if (found == entries.end() || found->path != normalized.value().value) {
        return std::optional<entry_metadata>{};
    }
    return std::optional<entry_metadata>{*found};
}

result<bool> contains_ba2_dx10_entry(std::span<const entry_metadata> entries,
                                     std::string_view path) {
    auto found = find_ba2_dx10_entry(entries, path);
    if (!found) {
        return found.error();
    }
    return found.value().has_value();
}

result<void> extract_ba2_dx10_payload(const detail::host_file_path& host_path,
                                      const entry_metadata& entry, payload_sink& sink) {
    if (!entry.texture.has_value()) {
        return error{error_code::format_error, "BA2 DX10 extraction requires texture metadata"};
    }
    if (entry.has_embedded_name || entry.embedded_name_prefix_size != 0U) {
        return error{error_code::format_error,
                     "BA2 DX10 entries must not carry embedded-name prefixes"};
    }

    texture::dds_texture_layout layout{entry.texture->width,      entry.texture->height,
                                       entry.texture->mip_count,  entry.texture->dxgi_format,
                                       entry.texture->array_size, entry.texture->is_cubemap};
    auto header = texture::build_dds_dxt10_header(layout);
    if (!header) {
        return header.error();
    }

    // D-13/D-30: write the reconstructed DDS header first and keep later work
    // bounded to one chunk.
    auto wrote_header = detail::write_payload_exact(sink, header.value(), "BA2 DX10 DDS header");
    if (!wrote_header) {
        return wrote_header.error();
    }

    auto input = detail::open_host_file(host_path, ba2_dx10_extraction_host_context());
    if (!input) {
        return input.error();
    }

    // D-12: DirectXTex validation is intentionally not called here; tests
    // validate returned DDS bytes. D-15: entry.texture->chunks is already ordered
    // by the parser's logical_texture_segment/ source_chunk_index mapping, so
    // extraction reads parser-validated source chunks instead of raw order. D-31:
    // chunk compression comes from parsed metadata, never the texture's archive
    // path or extension.
    for (const auto& chunk : entry.texture->chunks) {
        if (chunk.compression == entry_compression::none) {
            auto streamed = stream_raw_chunk(input.value(), chunk, sink);
            if (!streamed) {
                return streamed.error();
            }
            continue;
        }
        auto extracted = extract_compressed_chunk(input.value(), chunk, sink);
        if (!extracted) {
            return extracted.error();
        }
    }
    return {};
}

}  // namespace libbsa::formats::ba2
