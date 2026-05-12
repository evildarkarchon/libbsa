#include "formats/bsa/tes4_bsa_reader.hpp"

#include <detail/archive_path.hpp>
#include <detail/compression_router.hpp>
#include <detail/payload_stream.hpp>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

std::uint32_t read_u32_le(std::span<const std::byte> bytes) noexcept {
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[0])) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[1])) << 8U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[2])) << 16U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[3])) << 24U);
}

detail::compression_method compression_method_for(entry_compression compression) noexcept {
  switch (compression) {
  case entry_compression::none:
    return detail::compression_method::none;
  case entry_compression::deflate:
    return detail::compression_method::deflate;
  case entry_compression::lz4_frame:
    return detail::compression_method::lz4_frame;
  case entry_compression::lz4_block:
    return detail::compression_method::lz4_block;
  }
  return detail::compression_method::none;
}

result<void> extract_file_payload(std::ifstream& input, const entry_metadata& entry, payload_sink& sink) {
  if (entry.embedded_name_prefix_size > entry.stored_size) {
    return error{error_code::format_error, "TES4 BSA embedded-name prefix exceeds stored payload"};
  }
  const auto payload_offset = entry.payload_offset + entry.embedded_name_prefix_size;
  if (payload_offset < entry.payload_offset) {
    return error{error_code::format_error, "TES4 BSA consumer payload offset overflows"};
  }
  const auto payload_size = entry.stored_size - entry.embedded_name_prefix_size;
  if (entry.compression == entry_compression::none) {
    return detail::stream_payload_range(input, payload_offset, payload_size, sink, extraction_chunk_size, "TES4 BSA payload");
  }
  if (payload_size < 4U) {
    return error{error_code::format_error, "TES4 BSA compressed payload size prefix is truncated"};
  }

  auto expected_size_bytes = detail::read_payload_bytes_at(input, payload_offset, 4U,
                                                           "TES4 BSA compressed payload size prefix");
  if (!expected_size_bytes) {
    return expected_size_bytes.error();
  }
  const auto expected_size = read_u32_le(std::span<const std::byte>{expected_size_bytes.value().data(),
                                                                    expected_size_bytes.value().size()});
  if (expected_size != entry.raw_size) {
    return error{error_code::format_error, "TES4 BSA compressed payload size prefix does not match metadata"};
  }

  const auto compressed_payload_offset = payload_offset + 4U;
  if (compressed_payload_offset < payload_offset) {
    return error{error_code::format_error, "TES4 BSA compressed payload offset overflows"};
  }
  auto compressed_payload = detail::read_payload_bytes_at(input, compressed_payload_offset, payload_size - 4U,
                                                          "TES4 BSA compressed payload");
  if (!compressed_payload) {
    return compressed_payload.error();
  }

  auto decoded = detail::decompress_payload_exact(compression_method_for(entry.compression), compressed_payload.value(),
                                                  static_cast<std::size_t>(expected_size));
  if (!decoded) {
    return decoded.error();
  }
  return detail::write_payload_chunks(sink, decoded.value(), extraction_chunk_size, "TES4 BSA decoded payload");
}

} // namespace

result<std::vector<entry_metadata>> tes4_bsa_entries(std::span<const entry_metadata> entries) {
  return std::vector<entry_metadata>{entries.begin(), entries.end()};
}

result<std::optional<entry_metadata>> find_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path) {
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

result<bool> contains_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path) {
  auto found = find_tes4_bsa_entry(entries, path);
  if (!found) {
    return found.error();
  }
  return found.value().has_value();
}

result<void> extract_tes4_bsa_payload_from_file(std::string_view host_path,
                                                const entry_metadata& entry,
                                                payload_sink& sink) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path for TES4 BSA extraction"};
  }
  return extract_file_payload(input, entry, sink);
}

} // namespace libbsa::formats::bsa
