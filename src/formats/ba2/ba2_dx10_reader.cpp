#include "formats/ba2/ba2_dx10_reader.hpp"

#include <detail/archive_path.hpp>
#include <detail/compression_router.hpp>

#include "texture/dds_layout.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

result<std::size_t> checked_size(std::uint64_t value, std::string_view description) {
  if (value > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, std::string{description} + " exceeds platform limits"};
  }
  return static_cast<std::size_t>(value);
}

result<void> write_all(payload_sink& sink, std::span<const std::byte> bytes) {
  auto written = sink.write(bytes);
  if (!written) {
    return written.error();
  }
  if (written.value() != bytes.size()) {
    return error{error_code::io_error, "payload sink accepted a partial chunk"};
  }
  return {};
}

result<void> write_in_chunks(payload_sink& sink, std::span<const std::byte> bytes) {
  for (std::size_t offset = 0; offset < bytes.size();) {
    const auto chunk_size = std::min(extraction_chunk_size, bytes.size() - offset);
    auto written = write_all(sink, bytes.subspan(offset, chunk_size));
    if (!written) {
      return written.error();
    }
    offset += chunk_size;
  }
  return {};
}

result<void> validate_chunk_stream_limits(const texture_chunk_metadata& chunk) {
  if (chunk.payload_offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, "BA2 DX10 chunk payload offset exceeds stream limits"};
  }
  if (chunk.stored_size > static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max())) {
    return error{error_code::format_error, "BA2 DX10 chunk stored payload exceeds stream limits"};
  }
  return {};
}

result<void> stream_raw_chunk(std::ifstream& input, const texture_chunk_metadata& chunk, payload_sink& sink) {
  auto limits = validate_chunk_stream_limits(chunk);
  if (!limits) {
    return limits.error();
  }
  if (chunk.raw_size != chunk.stored_size) {
    return error{error_code::format_error, "BA2 DX10 raw chunk size does not match stored size"};
  }

  input.clear();
  input.seekg(static_cast<std::streamoff>(chunk.payload_offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, "failed to seek to BA2 DX10 chunk payload"};
  }

  std::vector<std::byte> buffer(extraction_chunk_size);
  std::uint64_t remaining = chunk.stored_size;
  while (remaining != 0U) {
    const auto chunk_size = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
    input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk_size));
    if (input.bad()) {
      return error{error_code::io_error, "failed while reading BA2 DX10 chunk payload"};
    }
    if (static_cast<std::size_t>(input.gcount()) != chunk_size) {
      return error{error_code::format_error, "BA2 DX10 chunk payload span is outside the archive"};
    }

    auto written = write_all(sink, std::span<const std::byte>{buffer.data(), chunk_size});
    if (!written) {
      return written.error();
    }
    remaining -= chunk_size;
  }
  return {};
}

result<std::vector<std::byte>> read_stored_chunk(std::ifstream& input, const texture_chunk_metadata& chunk) {
  auto limits = validate_chunk_stream_limits(chunk);
  if (!limits) {
    return limits.error();
  }
  auto stored_size = checked_size(chunk.stored_size, "BA2 DX10 stored chunk");
  if (!stored_size) {
    return stored_size.error();
  }

  input.clear();
  input.seekg(static_cast<std::streamoff>(chunk.payload_offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, "failed to seek to BA2 DX10 chunk payload"};
  }

  std::vector<std::byte> payload(stored_size.value());
  input.read(reinterpret_cast<char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
  if (input.bad()) {
    return error{error_code::io_error, "failed while reading BA2 DX10 chunk payload"};
  }
  if (static_cast<std::size_t>(input.gcount()) != payload.size()) {
    return error{error_code::format_error, "BA2 DX10 chunk payload span is outside the archive"};
  }
  return payload;
}

result<detail::compression_method> compression_method_for(const texture_chunk_metadata& chunk) {
  switch (chunk.compression) {
  case entry_compression::deflate:
    return detail::compression_method::deflate;
  case entry_compression::lz4_block:
    return detail::compression_method::lz4_block;
  case entry_compression::none:
    return error{error_code::format_error, "BA2 DX10 raw chunks must not enter decompression routing"};
  case entry_compression::lz4_frame:
    return error{error_code::format_error, "BA2 DX10 does not support lz4_frame chunk payloads"};
  }
  return error{error_code::format_error, "BA2 DX10 chunk has unknown compression metadata"};
}

result<void> extract_compressed_chunk(std::ifstream& input, const texture_chunk_metadata& chunk, payload_sink& sink) {
  auto stored = read_stored_chunk(input, chunk);
  if (!stored) {
    return stored.error();
  }
  auto expected_size = checked_size(chunk.raw_size, "BA2 DX10 raw chunk");
  if (!expected_size) {
    return expected_size.error();
  }
  auto method = compression_method_for(chunk);
  if (!method) {
    return method.error();
  }
  // D-32: every compressed texture chunk must decode to exactly the parser-declared raw size.
  auto decoded = detail::decompress_payload_exact(method.value(), stored.value(), expected_size.value());
  if (!decoded) {
    return decoded.error();
  }
  return write_in_chunks(sink, decoded.value());
}

} // namespace

result<std::vector<entry_metadata>> ba2_dx10_entries(std::span<const entry_metadata> entries) {
  return std::vector<entry_metadata>{entries.begin(), entries.end()};
}

result<std::optional<entry_metadata>> find_ba2_dx10_entry(std::span<const entry_metadata> entries,
                                                          std::string_view path) {
  auto normalized = detail::normalize_archive_path(path);
  if (!normalized) {
    return normalized.error();
  }

  const auto found = std::lower_bound(entries.begin(), entries.end(), normalized.value().value,
                                      [](const entry_metadata& entry, const std::string& key) {
                                        return entry.path < key;
                                      });
  if (found == entries.end() || found->path != normalized.value().value) {
    return std::optional<entry_metadata>{};
  }
  return std::optional<entry_metadata>{*found};
}

result<bool> contains_ba2_dx10_entry(std::span<const entry_metadata> entries, std::string_view path) {
  auto found = find_ba2_dx10_entry(entries, path);
  if (!found) {
    return found.error();
  }
  return found.value().has_value();
}

result<void> extract_ba2_dx10_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  if (!entry.texture.has_value()) {
    return error{error_code::format_error, "BA2 DX10 extraction requires texture metadata"};
  }
  if (entry.has_embedded_name || entry.embedded_name_prefix_size != 0U) {
    return error{error_code::format_error, "BA2 DX10 entries must not carry embedded-name prefixes"};
  }

  texture::dds_texture_layout layout{entry.texture->width,
                                     entry.texture->height,
                                     entry.texture->mip_count,
                                     entry.texture->dxgi_format,
                                     entry.texture->array_size,
                                     entry.texture->is_cubemap};
  auto header = texture::build_dds_dxt10_header(layout);
  if (!header) {
    return header.error();
  }

  // D-13/D-30: write the reconstructed DDS header first and keep later work bounded to one chunk.
  auto wrote_header = write_all(sink, header.value());
  if (!wrote_header) {
    return wrote_header.error();
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open BA2 archive host path for DX10 extraction"};
  }

  // D-12: DirectXTex validation is intentionally not called here; tests validate returned DDS bytes.
  // D-15: entry.texture->chunks is already ordered by the parser's logical_texture_segment/
  // source_chunk_index mapping, so extraction reads parser-validated source chunks instead of raw order.
  // D-31: chunk compression comes from parsed metadata, never the texture's archive path or extension.
  for (const auto& chunk : entry.texture->chunks) {
    if (chunk.compression == entry_compression::none) {
      auto streamed = stream_raw_chunk(input, chunk, sink);
      if (!streamed) {
        return streamed.error();
      }
      continue;
    }
    auto extracted = extract_compressed_chunk(input, chunk, sink);
    if (!extracted) {
      return extracted.error();
    }
  }
  return {};
}

} // namespace libbsa::formats::ba2
