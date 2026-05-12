#include "formats/ba2/ba2_gnrl_reader.hpp"

#include <detail/archive_path.hpp>
#include <detail/compression_router.hpp>
#include <detail/payload_stream.hpp>

#include <algorithm>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

result<void> stream_raw_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  if (entry.raw_size != entry.stored_size) {
    return error{error_code::format_error, "BA2 GNRL raw payload size does not match stored size"};
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open BA2 archive host path for extraction"};
  }
  return detail::stream_payload_range(input, entry.payload_offset, entry.stored_size, sink, extraction_chunk_size,
                                      "BA2 GNRL entry payload");
}

result<detail::compression_method> compression_method_for(const entry_metadata& entry) {
  switch (entry.compression) {
  case entry_compression::deflate:
    return detail::compression_method::deflate;
  case entry_compression::lz4_block:
    return detail::compression_method::lz4_block;
  case entry_compression::none:
    return error{error_code::format_error, "BA2 GNRL raw entries must not enter decompression routing"};
  case entry_compression::lz4_frame:
    return error{error_code::format_error, "BA2 GNRL does not support LZ4 frame payloads"};
  }
  return error{error_code::format_error, "BA2 GNRL entry has unknown compression metadata"};
}

result<void> extract_compressed_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open BA2 archive host path for extraction"};
  }

  auto stored = detail::read_payload_bytes_at(input, entry.payload_offset, entry.stored_size, "BA2 GNRL stored payload");
  if (!stored) {
    return stored.error();
  }
  auto expected_size = detail::checked_payload_size(entry.raw_size, "BA2 GNRL raw payload");
  if (!expected_size) {
    return expected_size.error();
  }
  auto method = compression_method_for(entry);
  if (!method) {
    return method.error();
  }
  // Corrupt BA2 compressed payloads are malformed archive bytes, so preserve the
  // codec's stable format_error result instead of attempting partial extraction.
  auto decoded = detail::decompress_payload_exact(method.value(), stored.value(), expected_size.value());
  if (!decoded) {
    return decoded.error();
  }
  return detail::write_payload_chunks(sink, decoded.value(), extraction_chunk_size, "BA2 GNRL decoded payload");
}

} // namespace

result<std::vector<entry_metadata>> ba2_gnrl_entries(std::span<const entry_metadata> entries) {
  return std::vector<entry_metadata>{entries.begin(), entries.end()};
}

result<std::optional<entry_metadata>> find_ba2_gnrl_entry(std::span<const entry_metadata> entries,
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

result<bool> contains_ba2_gnrl_entry(std::span<const entry_metadata> entries, std::string_view path) {
  auto found = find_ba2_gnrl_entry(entries, path);
  if (!found) {
    return found.error();
  }
  return found.value().has_value();
}

result<void> extract_ba2_gnrl_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  if (entry.has_embedded_name || entry.embedded_name_prefix_size != 0U) {
    return error{error_code::format_error, "BA2 GNRL entries must not carry embedded-name prefixes"};
  }
  if (entry.compression == entry_compression::none) {
    return stream_raw_payload(host_path, entry, sink);
  }
  return extract_compressed_payload(host_path, entry, sink);
}

} // namespace libbsa::formats::ba2
