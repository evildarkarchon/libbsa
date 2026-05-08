#include "formats/bsa/tes4_bsa_reader.hpp"

#include <detail/archive_path.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <limits>
#include <vector>

namespace libbsa::formats::bsa {
namespace {

constexpr std::size_t extraction_chunk_size = 64U * 1024U;

bool span_fits(std::size_t start, std::size_t length, std::size_t total) noexcept {
  return start <= total && length <= total - start;
}

result<std::size_t> checked_size(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds platform limits"};
  }
  return static_cast<std::size_t>(value);
}

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

result<std::span<const std::byte>> consumer_payload_span(std::span<const std::byte> stored_payload,
                                                         const entry_metadata& entry) {
  if (entry.embedded_name_prefix_size > stored_payload.size()) {
    return error{error_code::format_error, "TES4 BSA embedded-name prefix exceeds stored payload"};
  }
  // Embedded names are part of the on-disk payload but not the consumer-visible file bytes.
  return stored_payload.subspan(entry.embedded_name_prefix_size);
}

result<void> extract_stored_payload(std::span<const std::byte> stored_payload, const entry_metadata& entry,
                                    payload_sink& sink) {
  auto payload = consumer_payload_span(stored_payload, entry);
  if (!payload) {
    return payload.error();
  }
  if (entry.compression == entry_compression::none) {
    return write_in_chunks(sink, payload.value());
  }
  if (payload.value().size() < 4U) {
    return error{error_code::format_error, "TES4 BSA compressed payload size prefix is truncated"};
  }

  const auto expected_size = read_u32_le(payload.value().first(4U));
  if (expected_size != entry.raw_size) {
    return error{error_code::format_error, "TES4 BSA compressed payload size prefix does not match metadata"};
  }
  auto decoded = detail::decompress_payload_exact(compression_method_for(entry.compression), payload.value().subspan(4U),
                                                  static_cast<std::size_t>(expected_size));
  if (!decoded) {
    return decoded.error();
  }
  return write_in_chunks(sink, decoded.value());
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

result<void> extract_tes4_bsa_payload(std::span<const std::byte> stored_payload, const entry_metadata& entry,
                                      payload_sink& sink) {
  return extract_stored_payload(stored_payload, entry, sink);
}

} // namespace libbsa::formats::bsa
